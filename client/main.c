#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// all header files
#include "../core/types.h"
#include "queue.h"

// global variable definitions
#include "globals.c"

Queue actions_queue;
pthread_mutex_t actions_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t actions_queue_cond = PTHREAD_COND_INITIALIZER;
bool connected = false;
char host[500] = "";

Player players[MAX_PLAYERS];
pthread_mutex_t players_mutex = PTHREAD_MUTEX_INITIALIZER;

// data structure implementations
#include "player.c"
#include "queue.c"

// program components
#include "network.c"

void sleep_ms(int milliseconds) {
  struct timespec ts;
  ts.tv_sec = milliseconds / 1000;
  ts.tv_nsec = (milliseconds % 1000) * 1000000;
  nanosleep(&ts, NULL);
}

// the main program starting point: window initialization and event polling
int main(int argc, char* argv[]) {
  if (argc >= 2) { strncpy(host, argv[1], 499); }

  QU_initalize(&actions_queue, 100);

  // initalize all players to inactive
  // shouldn't need a lock here since this happens
  // before accepting any connections or creating threads
  for (size_t i = 0; i < MAX_PLAYERS; i++) {
    players[i].active = false;
    players[i].x = 0;
    players[i].y = 0;
  }

  pthread_t network_thread;
  pthread_create(&network_thread, NULL, network_worker_handler, NULL);
  pthread_detach(network_thread);

  // dont create the window until we're connected
  while (!connected) { sleep_ms(200); }

  // Pointers for the window and renderer
  SDL_Window* window = NULL;
  SDL_Renderer* renderer = NULL;
  // Flag to control the main loop
  SDL_bool gameIsRunning = SDL_TRUE;
  SDL_Event event;

  // 1. Initialize SDL's video subsystem
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return 1;
  }

  // 2. Create the window
  window = SDL_CreateWindow(
      "SDL2 Window",    // Window title
                        // SDL_WINDOWPOS_CENTERED, // Initial x position
                        // SDL_WINDOWPOS_CENTERED, // Initial y position
      0,                // Initial x position
      0,                // Initial y position
      800,              // Width in pixels
      600,              // Height in pixels
      SDL_WINDOW_SHOWN  // Window flags
  );
  if (window == NULL) {
    fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  // 3. Create a renderer for hardware accelerated rendering (modern approach)
  renderer = SDL_CreateRenderer(
      window,  // Window to render to
      -1,      // Initialize the first supported rendering driver
      // SDL_RENDERER_ACCELERATED // Use hardware acceleration
      SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC  // Use hardware acceleration
  );

  if (renderer == NULL) {
    fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  //  Load an image into RAM (Surface)
  // Note: SDL_LoadBMP is built-in. For PNG, you'd use IMG_Load from SDL_image.
  SDL_Surface* tempSurface = SDL_LoadBMP("assets/blackbuck.bmp");
  if (!tempSurface) { printf("Unable to load image! SDL_Error: %s\n", SDL_GetError()); }

  // 2. "Upload" that image to the GPU (Texture)
  SDL_Texture* playerTexture = SDL_CreateTextureFromSurface(renderer, tempSurface);

  // 3. Free the RAM immediately; we don't need the Surface anymore
  SDL_FreeSurface(tempSurface);

  // Define position and size
  // for the main player
  SDL_Rect dstRect = {0, 0, 0, 0};
  dstRect.x = 100;  // Position X
  dstRect.y = 100;  // Position Y
  dstRect.w = 64;   // Width
  dstRect.h = 64;   // Height

  // define the position and size for all other players
  SDL_Rect dstRects[MAX_PLAYERS];

  for (size_t i = 0; i < MAX_PLAYERS; i++) {
    // dstRects[i].x = 100;  // Position X
    // dstRects[i].y = 100;  // Position Y
    dstRects[i].w = 64;  // Width
    dstRects[i].h = 64;  // Height
  }

  Uint64 last_time = SDL_GetTicks64();  // Use 64-bit for modern SDL2
  float posX = 100.0f;                  // Use floats for smooth movement
  float posY = 100.0f;                  // Use floats for smooth movement
  float speed = 300.0f;                 // 300 pixels per second

  MoveAction move_action = {.x = 0, .y = 0};

  // 4. Main game loop
  while (gameIsRunning) {
    Uint64 current_time = SDL_GetTicks64();
    float deltaTime = (float)(current_time - last_time) / 1000.0f;  // Convert ms to seconds
    last_time = current_time;

    // Handle events
    while (SDL_PollEvent(&event) != 0) {
      // User requests quit
      if (event.type == SDL_QUIT) gameIsRunning = SDL_FALSE;

      // // Example: Move with Keyboard
      // if (event.type == SDL_KEYDOWN) {
      //     switch (event.key.keysym.sym) {
      //         case SDLK_RIGHT: dstRect.x += 10; break;
      //         case SDLK_LEFT:  dstRect.x -= 10; break;
      //     }
      // }
    }

    // 2. Apply Movement using Delta Time
    const Uint8* keyboard_state = SDL_GetKeyboardState(NULL);
    // const Uint32 mouse_state = SDL_GetMouseState(NULL, NULL);

    if (keyboard_state[SDL_SCANCODE_RIGHT]) posX += speed * deltaTime;
    if (keyboard_state[SDL_SCANCODE_LEFT]) posX -= speed * deltaTime;
    if (keyboard_state[SDL_SCANCODE_DOWN]) posY += speed * deltaTime;
    if (keyboard_state[SDL_SCANCODE_UP]) posY -= speed * deltaTime;

    // action.x = (int16_t)((int)posX - dstRect.x);
    // action.y = (int16_t)((int)posY - dstRect.y);

    // check if the player ended up moving at all
    if (move_action.x != (int16_t)posX || move_action.y != (int16_t)posY) {
      move_action.x = (int16_t)posX;
      move_action.y = (int16_t)posY;
      printf("I am moving to %hd and %hd\n", move_action.x, move_action.y);

      // Moved! Add that movement to the queue
      pthread_mutex_lock(&actions_queue_mutex);
      if (QU_is_full(&actions_queue)) printf("Queue full\n");
      else QU_enqueue(&actions_queue, (MoveAction){.x = move_action.x, .y = move_action.y});

      pthread_cond_signal(&actions_queue_cond);
      pthread_mutex_unlock(&actions_queue_mutex);
    }

    // 3. Update the Rect (casting float back to int for the GPU)
    dstRect.x = (int)posX;
    dstRect.y = (int)posY;

    // Rendering
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Set draw color to black
    SDL_RenderClear(renderer);                       // Clear the screen with the draw color

    // ... Add your drawing code here ...

    // Render the texture to the screen at the location of dstRect
    // Passing NULL for the source rect means "use the whole image"
    SDL_RenderCopy(renderer, playerTexture, NULL, &dstRect);

    pthread_mutex_lock(&players_mutex);
    for (size_t i = 0; i < MAX_PLAYERS; i++) {
      if (players[i].active) {
        dstRects[i].x = players[i].x;
        dstRects[i].y = players[i].y;
        SDL_RenderCopy(renderer, playerTexture, NULL, &dstRects[i]);
      }
    }
    pthread_mutex_unlock(&players_mutex);

    SDL_RenderPresent(renderer);  // Update the screen
  }

  // 5. Cleanup and exit
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}

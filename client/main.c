#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// all header files
#include "../core/stroke_manager.h"
#include "../core/types.h"
#include "../core/uncommitted_strokes_queue.h"
#include "queue.h"

// global variable definitions
#include "globals.c"

Queue actions_queue;
StrokeManager stroke_manager;
UncommittedStrokesQueue uncommitted_strokes_queue;
bool connected = false;
char host[500] = "";
uint16_t my_player_index;

Player players[MAX_PLAYERS];
pthread_mutex_t players_mutex = PTHREAD_MUTEX_INITIALIZER;

// data structures/utils implementations
#include "../core/network_utils.c"
#include "../core/stroke_manager.c"
#include "../core/uncommitted_strokes_queue.c"
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

  // int *array = malloc(sizeof(int) * 10);
  // free(array);
  // return array[5]; // BOOM

  SM_initalize(&stroke_manager, 128);
  USQ_initalize(&uncommitted_strokes_queue, 16);
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
  bool gameIsRunning = true;
  SDL_Event event;

  // 1. Initialize SDL's video subsystem
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return 1;
  }

  // 2. Create the window
  window = SDL_CreateWindow("SDL2 Window", 800, 600, 0);

  if (window == NULL) {
    fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  // 3. Create a renderer for hardware accelerated rendering (modern approach)
  renderer = SDL_CreateRenderer(
      window,  // Window to render to
      NULL     // Initialize the first supported rendering driver
               // SDL_RENDERER_ACCELERATED // Use hardware acceleration
               // SDL_RENDERER_ACCELERATED |
               //     SDL_RENDERER_PRESENTVSYNC // Use hardware acceleration
               // SDL_PROP_RENDERER_CREATE_PRESENT_VSYNC_NUMBER
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
  SDL_DestroySurface(tempSurface);

  // Define position and size
  // for the main player
  SDL_FRect dstRect = {0, 0, 0, 0};
  dstRect.x = 100;  // Position X
  dstRect.y = 100;  // Position Y
  dstRect.w = 64;   // Width
  dstRect.h = 64;   // Height

  // define the position and size for all other players
  SDL_FRect dstRects[MAX_PLAYERS];

  for (size_t i = 0; i < MAX_PLAYERS; i++) {
    // dstRects[i].x = 100;  // Position X
    // dstRects[i].y = 100;  // Position Y
    dstRects[i].w = 64;  // Width
    dstRects[i].h = 64;  // Height
  }

  Uint64 last_time = SDL_GetTicks();  // Use 64-bit for modern SDL2
  float posX = 100.0f;                // Use floats for smooth movement
  float posY = 100.0f;                // Use floats for smooth movement
  float speed = 300.0f;               // 300 pixels per second

  MoveAction move_action = {.x = 0, .y = 0};

  float mouseX = -3000;
  float mouseY = -3000;

  bool is_mouse_left_button_down = false;
  UncommittedStroke* current_uncommitted_stroke;

  // 4. Main game loop
  while (gameIsRunning) {
    Uint64 current_time = SDL_GetTicks();
    float deltaTime = (float)(current_time - last_time) / 1000.0f;  // Convert ms to seconds
    last_time = current_time;

    // 2. Apply Movement using Delta Time
    const bool* keyboard_state = SDL_GetKeyboardState(NULL);
    SDL_GetMouseState(&mouseX, &mouseY);

    // Handle events
    while (SDL_PollEvent(&event) != 0) {
      // User requests quit
      if (event.type == SDL_EVENT_QUIT) gameIsRunning = false;

      // Example: Move with Keyboard
      if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.key) {
          case SDLK_Z: {
            if (keyboard_state[SDL_SCANCODE_LCTRL]) {
              printf("Attempting undo\n");
              USQ_undo_stroke(&uncommitted_strokes_queue, my_player_index);
            }
            break;
          }
        }
      }

      // Example: Move with Keyboard
      if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event.button.button & SDL_BUTTON_LEFT) {
          is_mouse_left_button_down = true;
          print_players();
          current_uncommitted_stroke = USQ_enqueue(
              &uncommitted_strokes_queue, (Stroke){
                                              .color_r = 255,
                                              .color_g = 255,
                                              .color_b = 255,
                                              .points_len = 0,
                                              .shape = 0,
                                              .undo = false,
                                              .player_index = my_player_index
                                          }
          );
        }
      }

      if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        printf("Am i running?\n");
        if (event.button.button & SDL_BUTTON_LEFT) {
          is_mouse_left_button_down = false;
          current_uncommitted_stroke->finished = true;
          USQ_drain(&uncommitted_strokes_queue, &stroke_manager);
        }
      }
    }

    if (keyboard_state[SDL_SCANCODE_RIGHT]) posX += speed * deltaTime;
    if (keyboard_state[SDL_SCANCODE_LEFT]) posX -= speed * deltaTime;
    if (keyboard_state[SDL_SCANCODE_DOWN]) posY += speed * deltaTime;
    if (keyboard_state[SDL_SCANCODE_UP]) posY -= speed * deltaTime;

    if (is_mouse_left_button_down) {
      printf(
          "x: %d, y: %d. index=%u\n", (int)mouseX, (int)mouseY,
          stroke_manager.strokes_len - (uint32_t)1
      );
      USQ_add_point(
          current_uncommitted_stroke,
          (StrokePoint){.opacity = 1, .x = (int16_t)mouseX, .y = (int16_t)mouseY}
      );
    }

    // check if the player ended up moving at all
    if (move_action.x != (int16_t)posX || move_action.y != (int16_t)posY) {
      move_action.x = (int16_t)posX;
      move_action.y = (int16_t)posY;
      printf("I am moving to %hd and %hd\n", move_action.x, move_action.y);

      // Moved! Add that movement to the queue
      pthread_mutex_lock(&actions_queue.mutex);
      if (QU_is_full(&actions_queue)) printf("Queue full\n");
      else QU_enqueue(&actions_queue, (MoveAction){.x = move_action.x, .y = move_action.y});

      pthread_cond_signal(&actions_queue.cond);
      pthread_mutex_unlock(&actions_queue.mutex);
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
    SDL_RenderTexture(renderer, playerTexture, NULL, &dstRect);

    pthread_mutex_lock(&players_mutex);
    for (size_t i = 0; i < MAX_PLAYERS; i++) {
      if (players[i].active) {
        dstRects[i].x = players[i].x;
        dstRects[i].y = players[i].y;
        SDL_RenderTexture(renderer, playerTexture, NULL, &dstRects[i]);
      }
    }
    pthread_mutex_unlock(&players_mutex);

    // Permenant history
    size_t point_cursor = 0;
    for (size_t s = 0; s < stroke_manager.strokes_len; s++) {
      Stroke* stroke = &stroke_manager.strokes[s];
      size_t stroke_end = point_cursor + stroke->points_len;

      if (stroke->undo) {
        point_cursor = stroke_end;
        continue;
      }

      SDL_SetRenderDrawColor(renderer, stroke->color_r, stroke->color_b, stroke->color_g, 255);
      for (size_t i = point_cursor + 1; i < stroke_end; i++) {
        SDL_SetRenderDrawColor(
            renderer, stroke->color_r, stroke->color_b, stroke->color_g,
            stroke_manager.points[i].opacity
        );
        SDL_RenderLine(
            renderer, stroke_manager.points[i].x, stroke_manager.points[i].y,
            stroke_manager.points[i - 1].x, stroke_manager.points[i - 1].y
        );
      }
      point_cursor = stroke_end;
    }

    // Active strokes
    for (int i = 0; i < uncommitted_strokes_queue.len; i++) {
      UncommittedStroke* current_ptr =
          uncommitted_strokes_queue
              .array[(uncommitted_strokes_queue.begin + i) % uncommitted_strokes_queue.capacity];

      Stroke* current_stroke = &current_ptr->stroke;
      if (current_stroke->undo) continue;

      for (uint32_t j = 1; j < current_stroke->points_len; j++) {
        SDL_SetRenderDrawColor(
            renderer, (unsigned char)current_stroke->color_r,
            (unsigned char)current_stroke->color_b, (unsigned char)current_stroke->color_g,
            (unsigned char)current_ptr->points[j].opacity
        );

        SDL_RenderLine(
            renderer, current_ptr->points[j].x, current_ptr->points[j].y,
            current_ptr->points[j - 1].x, current_ptr->points[j - 1].y
        );
      }
    }

    SDL_RenderPresent(renderer);  // Update the screen
  }

  // 5. Cleanup and exit
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}

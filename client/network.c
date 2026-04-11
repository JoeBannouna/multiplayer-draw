#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>

// all headers
#include "../core/types.h"
#include "queue.h"

// global variables
#include "globals.c"

#define PORT "3490"
#define MAXDATASIZE 500

int sockfd;
_Atomic bool is_network_thread_running = true;

void handle_packet(HeaderMessage* header, void* buf) {
  if (header->type == PLAYER_JOIN_EVENT) {
    PlayerPositionUpdatePacket* cast_helper = (PlayerPositionUpdatePacket*)buf;

    PlayerPositionUpdatePacket player = {
        .player_index = (int16_t)ntohs((uint16_t)cast_helper->player_index),
        .x = (int16_t)ntohs((uint16_t)cast_helper->x),
        .y = (int16_t)ntohs((uint16_t)cast_helper->y),
    };

    join_player(player);

  } else if (header->type == PLAYER_MOVE_UPDATE) {
    PlayerPositionUpdatePacket* cast_helper = (PlayerPositionUpdatePacket*)buf;

    printf(
        "Player %hd moving at %hd and %hd\n", (int16_t)ntohs((uint16_t)cast_helper->player_index),
        (int16_t)ntohs((int16_t)cast_helper->x), ntohs(cast_helper->y)
    );

    move_player_position(
        (int16_t)ntohs((uint16_t)cast_helper->player_index),
        (int16_t)ntohs((uint16_t)cast_helper->x), (int16_t)ntohs((uint16_t)cast_helper->y)
    );
  }
}

// keeps looping until n bytes are recieved
// returns -1 for errors
// returns 1 for connection closed
// returns 0 on success
int recv_exact(int sockfd, void* buf, size_t n) {
  printf("Running recv_exact\n");
  ssize_t total_bytes = 0;
  char* ptr = (char*)buf;

  while ((size_t)total_bytes < n) {
    ssize_t bytes = recv(sockfd, ptr + total_bytes, n - (size_t)total_bytes, 0);
    if (bytes == -1) return -1;
    if (bytes == 0) return 1;
    total_bytes += bytes;
  }

  return 0;
}

void* receive_server_updates() {
  while (is_network_thread_running) {
    // move this part to updates_listener_thread and wrap it in a while loop
    // ssize_t numbytes;
    // char buf[MAXDATASIZE];
    // if ((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
    //   perror("recv");
    //   exit(1);
    // }

    HeaderMessage header;
    int status = recv_exact(sockfd, &header, sizeof(HeaderMessage));
    if (status == 1) {
      printf("Connection closed!\n");
      break;
    }
    if (status == -1) {
      perror("recv header");
      break;
    }

    header.length = ntohs(header.length);
    header.type = ntohs(header.type);

    if (header.length == 0) {
      handle_packet(&header, NULL);
      continue;
    }

    if (header.length > MAX_HEADER_LENGTH) {
      printf("Suspicously large payload length %u\n", header.length);
      break;
    }

    char body[header.length];
    status = recv_exact(sockfd, body, header.length);
    if (status == 1) {
      printf("Connection closed!\n");
      break;
    }
    if (status == -1) {
      perror("recv header");
      break;
    }

    handle_packet(&header, body);

    // if (numbytes == 0) {
    //   printf("Connection closed!\n");
    //   is_network_thread_running = false;
    //   break;
    // }

    // buf[numbytes] = '\0';

    // printf("client: received '%s'\n", buf);
  }

  return NULL;
}

// get sockaddr, IPv4 or IPv6:
void* get_in_addr(struct sockaddr* sa) {
  if (sa->sa_family == AF_INET) { return &(((struct sockaddr_in*)sa)->sin_addr); }

  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void* network_worker_handler() {
  struct addrinfo hints, *servinfo, *p;
  int rv;
  int yes = 1;
  char s[INET6_ADDRSTRLEN];

  if (strnlen(host, 499) == 0) {
    printf("Select the host plz: ");
    scanf("%s", host);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(host, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(1);
  }

  // loop through all the results and bind to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      perror("client: connect");
      close(sockfd);
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "server: failed to find address\n");
    exit(1);
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), s, sizeof s);
  printf("sizeof s: %ld\n ", sizeof(s));
  printf("client: connected to %s\n", s);

  freeaddrinfo(servinfo);  // all done with this structure

  // flag for main to start SDL2 window
  connected = true;

  HeaderMessage header;
  header.length = htons(sizeof(MoveAction));
  header.type = htons(MOVE_ACTION);  // move action type

  // spawn a different thread for listening to server updates and adding the
  // updates to the queue so that the main thread can dequeue server updates and
  // show them on the UI

  pthread_t updates_listener_thread;
  pthread_create(&updates_listener_thread, NULL, receive_server_updates, NULL);

  while (is_network_thread_running) {
    pthread_mutex_lock(&actions_queue_mutex);
    while (QU_is_empty(&actions_queue)) {
      pthread_cond_wait(&actions_queue_cond, &actions_queue_mutex);
    }

    printf("Dequeueing\n");

    MoveAction data = QU_dequeue(&actions_queue);

    pthread_mutex_unlock(&actions_queue_mutex);

    data.x = (int16_t)htons((uint16_t)data.x);
    data.y = (int16_t)htons((uint16_t)data.y);

    send(sockfd, &header, sizeof(header), 0);
    send(sockfd, &data, sizeof(data), 0);
  }

  is_network_thread_running = false;
  close(sockfd);

  pthread_join(updates_listener_thread, NULL);

  return NULL;
}

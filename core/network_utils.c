#include <stdio.h>
#include <sys/socket.h>

int recv_exact(int sockfd, void* buf, size_t n) {
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

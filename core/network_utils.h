#ifndef NETWORK_UTILS_H
#define NETWORK_UTILS_H

#include <sys/types.h>

// keeps looping until n bytes are recieved
// returns -1 for errors
// returns 1 for connection closed
// returns 0 on success
int recv_exact(int sockfd, void* buf, size_t n);

#endif
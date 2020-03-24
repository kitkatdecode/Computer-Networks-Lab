#ifndef _RSOCKET_H
#define _RSOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define SOCK_MRP 10018
#define T 2
#define p 0.05

int r_socket(int domain, int type, int protocol);
int r_bind(int sockfd, const struct sockaddr *addr,
                socklen_t addrlen);
int r_sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t r_recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen);
int r_close(int fd);
int dropMessage(float p);
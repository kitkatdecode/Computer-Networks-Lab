#ifndef _RSOCKET_H
#define _RSOCKET_H

#include <sys/types.h>
#include <sys/socket.h>

#define SOCK_MRP 10018
#define TP 2    /* retransmission interval  */
#define DROP_PROB  0.5 /* msg dropping probability */
#define MAX_MSG_SIZE 100
#define MAX_TABLE_SIZE 100

/* drop counter for testing */
int transmissions;

/* Global Functions */
int r_socket(int, int, int);
int r_bind(int, const struct sockaddr *, socklen_t);
size_t r_sendto(int, const void *, size_t, int,
                      const struct sockaddr *, socklen_t);
size_t r_recvfrom(int, void *, size_t, int,
                        struct sockaddr *, socklen_t *);
int r_close(int);
int dropMessage(float);

#endif
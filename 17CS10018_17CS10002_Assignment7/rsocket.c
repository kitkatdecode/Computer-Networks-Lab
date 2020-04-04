/* MRP API FUNCTIONS */

#include "rsocket.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/time.h>
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <errno.h> 
#include <signal.h>
#include <time.h>

#define TIMER_INTV 25000

/* packet data struct */
typedef struct MRP_Packet{
    char data[MAX_MSG_SIZE];
    size_t data_len;
    struct sockaddr dest_addr;
    struct sockaddr src_addr;
    
} MRP_Packet;

/* packet to be sent , contains msg id */
typedef struct transmit_packt {
    int id;
    int sock_id;
    int is_ack;
    MRP_Packet msg;
} transmit_packt;

/* ack pkt for time recording */
typedef struct ack_packt {
    time_t t; /* time in seconds */
    transmit_packt pkt;
}ack_packt;

/* Global table/buffer pointers */
transmit_packt* send_buffer;
MRP_Packet* receive_buffer;
ack_packt* unacknowledged_msg_table;
int* received_msg_id_table;

/* msg id counter */
int MSG_ID = 0;

int send_buffer_count = 0;
int receive_buffer_count = 0;
int unack_counter = 0;
int recv_id_count =0;

void handleReceive(int sockfd);
void handleRetransmit(int sockfd);
void handleTransmit(int sockfd);

void handleACKMsgRecv(transmit_packt pkt);
void handleAppMsgRecv(transmit_packt pkt, int sockfd);

void timer_handler(int sig);

/*Global socket fd for use in  signal handler */
int sock_fd;

/* socket creation and memory allocation */
int r_socket(int domain, int type, int protocol) {
    if(type == SOCK_MRP) { /* checking for socket type */
        int sockfd = socket(domain, SOCK_DGRAM, protocol);
        sock_fd = sockfd;
        if(sockfd < 0){
            return -1;
        }

        /* memory allocation for different tables */
        if((send_buffer = (transmit_packt*)malloc(sizeof(transmit_packt)*MAX_TABLE_SIZE)) == NULL) {
            r_close(sockfd);
            return -1;
        }

        if((receive_buffer = (MRP_Packet*)malloc(sizeof(MRP_Packet)*MAX_TABLE_SIZE)) == NULL) {
            
            r_close(sockfd);
            return -1;
        }

        if((unacknowledged_msg_table = (ack_packt*)malloc(sizeof(ack_packt)*MAX_TABLE_SIZE)) == NULL) {
            r_close(sockfd);
            return -1;
        }

        if((received_msg_id_table = (int*)malloc(sizeof(int)*MAX_TABLE_SIZE)) == NULL) {
            r_close(sockfd);
            return -1;
        }

        /* Initialize setitimer() */
        struct itimerval timer;

        /* Install timer_handler as the signal handler for SIGALRM. */
        signal(SIGALRM, timer_handler);
        /* Configure the timer to expire after 250 msec... */
        timer.it_value.tv_sec = 0;
        timer.it_value.tv_usec = TIMER_INTV;
        /* ... and every 250 msec after that. */
        timer.it_interval.tv_sec = 0;
        timer.it_interval.tv_usec = TIMER_INTV;
        /* Start a timer. It counts down whenever this process is
        executing. */
        setitimer (ITIMER_REAL, &timer, NULL);

        return sockfd;
    }
    else {
        errno = 1;
        return -1;
    }
}

/* bind call */
int r_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return bind(sockfd, addr, addrlen);
}

size_t r_sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen) {
    
    while(1){
        if(send_buffer_count < MAX_TABLE_SIZE) { /* checking for space in send buffer */
            /* inserting in send buffer */
            send_buffer[send_buffer_count].id = MSG_ID++; /* msg id */
            send_buffer[send_buffer_count].sock_id = sockfd;
            send_buffer[send_buffer_count].msg.dest_addr = *dest_addr;
            addrlen = sizeof(struct sockaddr);
            getsockname(sockfd,&send_buffer[send_buffer_count].msg.src_addr,&addrlen);
            send_buffer[send_buffer_count].msg.data_len = len;
            send_buffer[send_buffer_count].is_ack = 0;
            memcpy(send_buffer[send_buffer_count].msg.data, buf, len);
            send_buffer_count++;
            // printf("%s",send_buffer[send_buffer_count-1].msg.data);
            return len;
        }

        // sleep(0.001);
    }

}

size_t r_recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen) {
    while(1){
        if(receive_buffer_count > 0) {  /* checking msg in receive buffer */
            int l;
            if(len < receive_buffer[0].data_len) l = len;
            else l = receive_buffer[0].data_len;

            /* copying first msg to buf */
            memcpy(buf, receive_buffer[0].data, l);

            /* deleting first msg */
            for(int i=0;i<receive_buffer_count-1;i++){
                receive_buffer[i] = receive_buffer[i+1];
            }
            bzero(receive_buffer[receive_buffer_count].data, receive_buffer[receive_buffer_count].data_len);
            receive_buffer_count--;

            return l;
        }

        sleep(0.5);
    }
}

/* freeing tables and closing socket */
int r_close(int fd) {
    if(send_buffer) {
        bzero(send_buffer, sizeof(transmit_packt)*MAX_TABLE_SIZE);
        free(send_buffer);
    }

    if(receive_buffer) {
        bzero(receive_buffer, sizeof(MRP_Packet)*MAX_TABLE_SIZE);
        free(receive_buffer);
    }
    
    if(unacknowledged_msg_table) {
        bzero(unacknowledged_msg_table, sizeof(ack_packt)*MAX_TABLE_SIZE);
        free(unacknowledged_msg_table);
    }
    
    if(received_msg_id_table) {
        bzero(received_msg_id_table, sizeof(int)*MAX_TABLE_SIZE);
        free(received_msg_id_table);
    }
    
    /*
    * Disable the real time interval timer
    */
    struct itimerval value;
    getitimer( ITIMER_REAL, &value );

    value.it_value.tv_sec = 0;
    value.it_value.tv_usec = 0;

    setitimer( ITIMER_REAL, &value, NULL );
    return close(fd);
}

/* random drop msg */
int dropMessage(float p) {
    float g = (float)rand()/(float)(RAND_MAX);
    if(g < p)return 1;
    return 0;
}

/* handle received msgs */
void handleReceive(int sockfd) {
    int r_size;
    transmit_packt buffer;
    while(1){
        r_size = recvfrom(sockfd, &buffer, sizeof(transmit_packt), MSG_DONTWAIT,NULL, NULL);

        if(r_size > 0) {
            /* random drop msg */
            if(dropMessage(DROP_PROB)) {
                return;
            }

            /* if ACK msg */
            if(buffer.is_ack){
                handleACKMsgRecv(buffer);
            }

            else {
                handleAppMsgRecv(buffer, sockfd);
            }

            return;
        }

        else {
            if(errno == EAGAIN || errno == EWOULDBLOCK)break;
        }
    }
}

/* handling ack msgs */
void handleACKMsgRecv(transmit_packt pkt){ 
    for(int i=0; i<unack_counter;i++) {
        if(unacknowledged_msg_table[i].pkt.id == pkt.id) {
            /* if msg found then delete from unacknowledged msg table */
            for(int j=i;j<unack_counter-1;j++){
                unacknowledged_msg_table[j]= unacknowledged_msg_table[j+1];
            }
            bzero(&unacknowledged_msg_table[unack_counter], sizeof(ack_packt));
            unack_counter--;
            return;
        }
    }
}

/* handling application msgs */
void handleAppMsgRecv(transmit_packt pkt, int sockfd) {
    for(int i=0; i<recv_id_count;i++) {
        if(received_msg_id_table[i] == pkt.id) {
            /* if received then sending ACK */
            transmit_packt ack;
            ack.is_ack = 1;
            ack.id = pkt.id;
            sendto(sockfd, &ack, sizeof(ack), MSG_DONTWAIT, &pkt.msg.src_addr, sizeof(struct sockaddr));
            return;
        }
    }

    /* if not received then adding to received buffer */
    receive_buffer[receive_buffer_count].dest_addr = pkt.msg.dest_addr;
    receive_buffer[receive_buffer_count].src_addr = pkt.msg.src_addr;
    receive_buffer[receive_buffer_count].data_len = pkt.msg.data_len;
    memcpy(receive_buffer[receive_buffer_count].data, pkt.msg.data, pkt.msg.data_len);
    
    received_msg_id_table[recv_id_count] = pkt.id;
    recv_id_count++;
    receive_buffer_count++;

    /* sending ACK */
    transmit_packt ack;
    ack.is_ack = 1;
    ack.id = pkt.id;
    sendto(sockfd, &ack, sizeof(ack), MSG_DONTWAIT, &pkt.msg.src_addr, sizeof(struct sockaddr));

}

/* handle retransmission */
void handleRetransmit(int sockfd) {
    time_t t; 
    for(int i=0;i<unack_counter;i++){
        t = time(NULL);
        /* sending again if timeout */
        if((t-unacknowledged_msg_table[i].t) >= TP) {
            sendto(sockfd, &unacknowledged_msg_table[i].pkt, sizeof(transmit_packt), MSG_DONTWAIT,
                             &unacknowledged_msg_table[i].pkt.msg.dest_addr, sizeof(struct sockaddr));
            unacknowledged_msg_table[i].t = time(NULL);
            transmissions++;
        }
    }
}

/* handle transmission */
void handleTransmit(int sockfd){
    /* return if unacknowledged msg table is full */
    if(unack_counter >= MAX_TABLE_SIZE) return;

    for(int i=0;i<send_buffer_count; i++) {
        if(sendto(sockfd, &send_buffer[i], sizeof(transmit_packt), MSG_DONTWAIT,
                             &send_buffer[i].msg.dest_addr, sizeof(struct sockaddr)) < 0) {
            if(errno != EAGAIN && errno != EWOULDBLOCK) {
                r_close(sockfd);
                exit(EXIT_FAILURE);
            }

        }
        else {
            transmissions++;
            unacknowledged_msg_table[unack_counter].pkt = send_buffer[i];
            unacknowledged_msg_table[unack_counter].t = time(NULL);
            unack_counter++;

            /* deleting from send buffer */
            for(int j = i;j<send_buffer_count-1;j++){
                send_buffer[j] = send_buffer[j+1];
            }
            bzero(&send_buffer[send_buffer_count], sizeof(transmit_packt));
            send_buffer_count--;
            i--;
        }

        if(unack_counter >= MAX_TABLE_SIZE) break;
    }
}

void timer_handler(int sig) {
    handleReceive(sock_fd);

    handleRetransmit(sock_fd);

    handleTransmit(sock_fd);
}
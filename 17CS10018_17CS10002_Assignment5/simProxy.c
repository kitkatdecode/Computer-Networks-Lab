/*SIMPLE PROXY SERVER */

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <errno.h> 
#include <netdb.h> 
#include <fcntl.h> 

#define PORT 8181
#define MAX_CONNECTION 300
#define REMOTE_PROXY_ADDR "172.16.2.30"
#define REMOTE_PROXY_PORT 8080

int main() {

    struct sockaddr_in serv_addr, inst_serv_addr;

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    memset(&inst_serv_addr, 0, sizeof(struct sockaddr_in));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    inst_serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, REMOTE_PROXY_ADDR, &inst_serv_addr.sin_addr);
    inst_serv_addr.sin_port = htons(REMOTE_PROXY_PORT);

    int listenfd;
    int inst_serv_connect_fd[MAX_CONNECTION];
    int client_fd[MAX_CONNECTION];
    int conn_cnt = 0;

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("listenfd socket creation failed");
		exit(EXIT_FAILURE);
    }

    if(bind(listenfd, (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		perror("listenfd binding failed");
		exit(EXIT_FAILURE);
	}

    if(listen(listenfd, 10)== -1){
        perror("listenfd : listen failed");
        exit(EXIT_FAILURE);
    }

    fd_set readfd;
    int sel_ret;
    int maxfd;

    while(1) {
        FD_ZERO(&readfd);
        FD_SET(listenfd, &readfd);
        FD_SET(STDIN_FILENO, &readfd);
        maxfd = listenfd;
        for(int i=0; i < conn_cnt; i++) {
            FD_SET(client_fd[i]);
            FD_SET(inst_serv_connect_fd[i]);
            if(client_fd[i] > maxfd) maxfd = client_fd[i];
            if(inst_serv_connect_fd[i] > maxfd) maxfd = inst_serv_connect_fd[i];
        }

        sel_ret = select(maxfd + 1, &readfd, NULL, NULL, NULL);

        

    }

    return 0;
}

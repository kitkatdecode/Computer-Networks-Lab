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


#define MAX_CONNECTION 300
#define BUFFER_SIZE 1024

int main(int argc,char* argv[]) {

    /* exit in case of port number and proxy server addr not mentioned */
    if(argc != 4 ) {
        printf("Proper Listen port and Proxy server address not provided\n");
        return 0;
    }

    struct sockaddr_in serv_addr, inst_serv_addr, client_addr;
    socklen_t len;
    len = sizeof(client_addr);
    
    int PORT, REMOTE_PROXY_PORT;
    /* proxy server address */
    char *REMOTE_PROXY_ADDR = argv[2]; 
    /* listen port and proxy port */
    PORT = atoi(argv[1]);
    REMOTE_PROXY_PORT = atoi(argv[3]);

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    memset(&inst_serv_addr, 0, sizeof(struct sockaddr_in));

    /* configuring simProxy server addr */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    /* configurung institute proxy server addr */
    inst_serv_addr.sin_family = AF_INET;
    inet_pton(AF_INET, REMOTE_PROXY_ADDR, &inst_serv_addr.sin_addr);
    inst_serv_addr.sin_port = htons(REMOTE_PROXY_PORT);

    /* sockfd for simProxy listening */
    int listenfd;
    /* fd's connected to inst server for one-to-one mapping to client(browser) connection */
    int inst_serv_connect_fd[MAX_CONNECTION];
    /* fd's for clients(browser) */
    int client_fd[MAX_CONNECTION];
    /* counting active connections */
    int conn_cnt = 0;

    /* initializing all client fd's to -1 */ 
    for(int i=0; i < MAX_CONNECTION; i++) {
        client_fd[i] = -1;
        inst_serv_connect_fd[i] = -1;
    }

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("listenfd socket creation failed");
		exit(EXIT_FAILURE);
    }

    /* making listenfd NON_BLOCKING */
    fcntl(listenfd, F_SETFL, O_NONBLOCK);

    if(bind(listenfd, (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		perror("listenfd binding failed");
		exit(EXIT_FAILURE);
	}

    if(listen(listenfd, 10)== -1){
        perror("listenfd : listen failed");
        exit(EXIT_FAILURE);
    }

    /* fd read set */
    fd_set readfd;
    int sel_ret;
    int maxfd;

    while(1) {
        /* setting all fd's into readfd */
        FD_ZERO(&readfd);
        FD_SET(listenfd, &readfd);
        FD_SET(STDIN_FILENO, &readfd);
        maxfd = listenfd;
        for(int i=0; i < conn_cnt; i++) {
            FD_SET(client_fd[i], &readfd);
            FD_SET(inst_serv_connect_fd[i], &readfd);
            if(client_fd[i] > maxfd) maxfd = client_fd[i];
            if(inst_serv_connect_fd[i] > maxfd) maxfd = inst_serv_connect_fd[i];
        }

        sel_ret = select(maxfd + 1, &readfd, NULL, NULL, NULL);

        if(sel_ret < 0) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        /* checking for exit command */
        if(FD_ISSET(STDIN_FILENO, &readfd)) {
            char cmd[30];
            scanf("%s", cmd);

            if(!strcmp(cmd, "exit")) {

                /* closing all connections and exiting */
                for(int j = 0; j < conn_cnt; j++){
                    close(client_fd[j]);
                    close(inst_serv_connect_fd[j]);
                }
                close(listenfd);
                return 0;
            }
        }

        /* accepting connections */
        if(FD_ISSET(listenfd, &readfd)) {
            int sockfd;
            while(1){
                sockfd = accept(listenfd, (struct sockaddr *)&client_addr, &len);
                if(sockfd < 0) {
                    if(errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    }
                    else {
                        perror("accept error");
                        exit(EXIT_FAILURE);
                    }
                }

                else {
                    if(conn_cnt < MAX_CONNECTION) {   /* checking too many connections */
                        
                        printf("Connection accepted from %s:%d\n", 
                                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                        fflush(stdout);
                        
                        client_fd[conn_cnt] = sockfd;
                        inst_serv_connect_fd[conn_cnt] = socket(AF_INET, SOCK_STREAM, 0);

                        /* connecting to institute proxy server */
                        if(connect(inst_serv_connect_fd[conn_cnt], 
                                (const struct sockaddr *)&inst_serv_addr, sizeof(struct sockaddr)) == -1){
                            perror("connection failed(to Inst. proxy server)");
                            exit(EXIT_FAILURE);
                        }
                        
                        /* making fd's NON_BLOCKING */
                        fcntl(client_fd[conn_cnt], F_SETFL, O_NONBLOCK);
                        fcntl(inst_serv_connect_fd[conn_cnt], F_SETFL, O_NONBLOCK);
                        /* incrementing connection count */
                        conn_cnt++;
                    }

                    else {   /* disconnecting extra connections */
                        close(sockfd);
                    }
                }
            }
        }

        int flag = 0;  /* flag used for handling closing connection */

        /* forwarding HTTP/HTTPS packets from one TCP side to other */
        for(int i=0; i < conn_cnt; i++) {
            
            /* forwarding from client to Inst. proxy server */
            if(FD_ISSET(client_fd[i], &readfd)) {
                int read_size, write_size;
                char buffer[BUFFER_SIZE];
                while(1){
                    memset(buffer, 0, BUFFER_SIZE);
                    read_size = read(client_fd[i], buffer, BUFFER_SIZE);
                    if(read_size == 0) {  /* handling connection closed */
                        
                        /* closing inst side connection */
                        close(inst_serv_connect_fd[i]);
                        /* updating fd's lists */
                        for(int j = i; j < conn_cnt-1; j++){
                            client_fd[j] = client_fd[j+1];
                            inst_serv_connect_fd[j] = inst_serv_connect_fd[j+1];
                        }
                        client_fd[conn_cnt-1] = -1;
                        inst_serv_connect_fd[conn_cnt-1] = -1;
                        conn_cnt--;
                        flag = 1;
                        break;
                    }

                    if(read_size < 0) {  /* checking for end of packet */
                        if(errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        else {
                            perror("from browser read error");
                            exit(EXIT_FAILURE);
                        }
                    }

                    else {  /* writing to institute side */
                        write_size= write(inst_serv_connect_fd[i], buffer, read_size);
                        if(write_size < 0) {
                            perror("to inst server write");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }

            /* continue to updated fd's lists in case of client connection break */
            if(flag) {
                i--;
                flag = 0;
                continue;
            }

            /* forwarding from Inst. proxy server to client(browser) */
            if(FD_ISSET(inst_serv_connect_fd[i], &readfd)) {
                int read_size, write_size;
                char buffer[BUFFER_SIZE];
                while(1){
                    memset(buffer, 0, BUFFER_SIZE);
                    read_size = read(inst_serv_connect_fd[i], buffer, BUFFER_SIZE);
                    if(read_size == 0) {  /* handling connection closed */
                        
                        /* closing inst side connection */
                        close(client_fd[i]);
                        /* updating fd's lists */
                        for(int j = i; j < conn_cnt-1; j++){
                            inst_serv_connect_fd[j] = inst_serv_connect_fd[j+1];
                            client_fd[j] = client_fd[j+1];
                        }
                        client_fd[conn_cnt-1] = -1;
                        inst_serv_connect_fd[conn_cnt-1] = -1;
                        conn_cnt--;
                        break;
                    }

                    if(read_size < 0) {   /* checking for end of packet */
                        if(errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        else {
                            perror("from inst server read error");
                            exit(EXIT_FAILURE);
                        }
                    }

                    else {  /* writing to client side */
                        write_size= write(client_fd[i], buffer, read_size);
                        if(write_size < 0) {
                            perror("to browser write");
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
        }

    }

    return 0;
}

/*SIMPLE HTTP PROXY SERVER */

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
#define BUFFER_SIZE 8192
#define MAX_HEADER_SIZE 65535
#define MAX_PKT_SIZE 65535   /* size of local buffers */

#define SUPPORTED_METHOD  "GET POST"    /* supported http methods by this server */
#define HOST_FOUND 1
#define NO_HDR 0
#define HTTP_NOT_SUPPORTED -1
#define HOST_NOTFOUND -2

void concatenate(char p[], char q[], int len);
void replaceSubstring(char string[],char sub[],char new_str[]);
int parseHeader(char pkt[], char **destIp, int *destPort);

/* Main program */
int main(int argc, char* argv[]) {
    /* exit in case of port number not mentioned */
    if(argc != 2 ) {
        printf("Proper Listen port not given\n");
        return 0;
    }

    struct sockaddr_in serv_addr, dest_serv_addr, client_addr;
    socklen_t len;
    len = sizeof(client_addr);
    
    int PORT;
    /* listen port */
    PORT = atoi(argv[1]);

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));

    /* configuring simHTTPProxy server addr */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    /* sockfd for simProxy listening */
    int listenfd;
    /* fd's connected to inst server for one-to-one mapping to client(browser) connection */
    int dest_serv_connect_fd[MAX_CONNECTION];
    /* fd's for clients(browser) */
    int client_fd[MAX_CONNECTION];
    /* write buffers for browser sockets */
    char *browser_write_buffer[MAX_CONNECTION];
    /* write buffers for destination sockets */
    char *dest_write_buffer[MAX_CONNECTION];
    /* counting active connections */
    int conn_cnt = 0;

    /* initializing all client fd's to -1 */ 
    for(int i=0; i < MAX_CONNECTION; i++) {
        client_fd[i] = -1;
        dest_serv_connect_fd[i] = -1;
        dest_write_buffer[i] = (char *)malloc(MAX_PKT_SIZE*sizeof(char));
        browser_write_buffer[i] = (char *)malloc(MAX_PKT_SIZE*sizeof(char));
        memset(browser_write_buffer[i], 0, MAX_PKT_SIZE);
        memset(dest_write_buffer[i], 0, MAX_PKT_SIZE);
    }

    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("listenfd socket creation failed");
		exit(EXIT_FAILURE);
    }

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0){
        perror("setsockopt(SO_REUSEADDR) failed");
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

    /* fd sets */
    fd_set readfd, writefd;
    int sel_ret;
    int maxfd;

    int sel_err_count =0;

    while(1) {
        /* setting all fd's into readfd */
        FD_ZERO(&readfd);
        FD_ZERO(&writefd);
        FD_SET(listenfd, &readfd);
        FD_SET(STDIN_FILENO, &readfd);
        maxfd = listenfd;
        for(int i=0; i < conn_cnt; i++) {
            /* set fd in write fd if writebuffer in not empty */
            if(strlen(browser_write_buffer[i]) > 0) {
                FD_SET(client_fd[i], &writefd);
            }
            FD_SET(client_fd[i], &readfd);
            
            if(dest_serv_connect_fd[i] != -1) {
                /* set fd in write fd if writebuffer in not empty */
                if(strlen(dest_write_buffer[i]) > 0) {
                    FD_SET(dest_serv_connect_fd[i], &writefd);
                }
                FD_SET(dest_serv_connect_fd[i], &readfd);
            }
            if(client_fd[i] > maxfd) maxfd = client_fd[i];
            if(dest_serv_connect_fd[i] > maxfd) maxfd = dest_serv_connect_fd[i];
        }

        sel_ret = select(maxfd + 1, &readfd, &writefd, NULL, NULL);

        if(sel_ret < 0) {
            perror("select error");
            sel_err_count++;
            if(sel_err_count < 5)continue;
            else exit(EXIT_FAILURE);
        }

        sel_err_count = 0;

        /* checking for exit command */
        if(FD_ISSET(STDIN_FILENO, &readfd)) {
            char cmd[30];
            scanf("%s", cmd);

            if(!strcmp(cmd, "exit")) {

                /* closing all connections and exiting */
                for(int j = 0; j < conn_cnt; j++){
                    close(client_fd[j]);
                    if(dest_serv_connect_fd[j]!=-1)close(dest_serv_connect_fd[j]);
                }
                close(listenfd);
                /* free dynamically allocated memory */
                for(int i=0; i < MAX_CONNECTION; i++) {
                    memset(browser_write_buffer[i], 0, MAX_PKT_SIZE);
                    memset(dest_write_buffer[i], 0, MAX_PKT_SIZE);
                    if(browser_write_buffer[i])free(browser_write_buffer[i]);
                    if(dest_write_buffer[i])free(dest_write_buffer[i]);
                }
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
                        continue;
                    }
                }

                else {
                    if(conn_cnt < MAX_CONNECTION) {   /* checking too many connections */
                        
                        printf("Connection accepted from %s:%d\n", 
                                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                        fflush(stdout);
                        
                        client_fd[conn_cnt] = sockfd;
                        
                        /* making fd NON_BLOCKING */
                        fcntl(client_fd[conn_cnt], F_SETFL, O_NONBLOCK);
                    
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
                char http_pkt[MAX_HEADER_SIZE];
                int hdr_found = 0;
                int buffer_flag = 0;
    
                memset(http_pkt,0,MAX_HEADER_SIZE);
                while(1){
                    memset(buffer, 0, BUFFER_SIZE);
                    read_size = read(client_fd[i], buffer, BUFFER_SIZE);
                    
                    if(read_size == 0) {  /* handling connection closed */
                        
                        /* closing dest side connection */
                        if(dest_serv_connect_fd[i] != -1) close(dest_serv_connect_fd[i]);
                        close(client_fd[i]);
                        /* updating fd's lists */
                        for(int j = i; j < conn_cnt-1; j++){
                            client_fd[j] = client_fd[j+1];
                            dest_serv_connect_fd[j] = dest_serv_connect_fd[j+1];
                            memcpy(browser_write_buffer[j], browser_write_buffer[j+1], MAX_PKT_SIZE);
                            memcpy(dest_write_buffer[j], dest_write_buffer[j+1], MAX_PKT_SIZE);
                        }
                        client_fd[conn_cnt-1] = -1;
                        dest_serv_connect_fd[conn_cnt-1] = -1;
                        memset(dest_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                        memset(browser_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
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
                            
                            /* closing dest side connection */
	                        if(dest_serv_connect_fd[i] != -1) close(dest_serv_connect_fd[i]);
                            close(client_fd[i]);
	                        /* updating fd's lists */
	                        for(int j = i; j < conn_cnt-1; j++){
	                            client_fd[j] = client_fd[j+1];
	                            dest_serv_connect_fd[j] = dest_serv_connect_fd[j+1];
                                memcpy(browser_write_buffer[j], browser_write_buffer[j+1], MAX_PKT_SIZE);
                                memcpy(dest_write_buffer[j], dest_write_buffer[j+1], MAX_PKT_SIZE);
	                        }
	                        client_fd[conn_cnt-1] = -1;
	                        dest_serv_connect_fd[conn_cnt-1] = -1;
                            memset(dest_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                            memset(browser_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
	                        conn_cnt--;
	                        flag = 1;
	                        break;
	                    }
                    }

                    else {  /* writing to dest side */
                        if(!hdr_found) {
                            char *destIp;
                            int destPort;
                            concatenate(http_pkt,buffer,1024);
                            int ret = parseHeader(http_pkt, &destIp, &destPort);
                            if(ret == HOST_FOUND) {
                                hdr_found = 1;
                                dest_serv_connect_fd[i] = socket(AF_INET, SOCK_STREAM, 0);
                                memset(&dest_serv_addr, 0, sizeof(struct sockaddr_in));
                                    
                                /* configuring destination server addr */
                                dest_serv_addr.sin_family = AF_INET;
                                inet_pton(AF_INET, destIp, &dest_serv_addr.sin_addr);
                                dest_serv_addr.sin_port = htons(destPort);
                                /* making fd non blocking */
                                fcntl(dest_serv_connect_fd[i], F_SETFL, O_NONBLOCK);
                                
                                /* connecting to destination server */
                                if(connect(dest_serv_connect_fd[i], 
                                        (const struct sockaddr *)&dest_serv_addr, sizeof(struct sockaddr)) == -1){
                                    /* handling non blocking connect .. otherwise waiting if in progress and blocks here */
                                    if(errno != EINPROGRESS) {  
                                        perror("connection failed(to dest_server)");
                                        close(client_fd[i]);
                                        for(int j = i; j < conn_cnt-1; j++){
                                            client_fd[j] = client_fd[j+1];
                                            dest_serv_connect_fd[j] = dest_serv_connect_fd[j+1];
                                            memcpy(browser_write_buffer[j], browser_write_buffer[j+1], MAX_PKT_SIZE);
                                            memcpy(dest_write_buffer[j], dest_write_buffer[j+1], MAX_PKT_SIZE);
                                        }
                                        client_fd[conn_cnt-1] = -1;
                                        memset(dest_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                                        memset(browser_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                                        conn_cnt--;
                                        flag = 1;
                                        break;
                                    }
                                    else { /* locally store pkt in buffer */
                                        concatenate(dest_write_buffer[i], http_pkt, strlen(http_pkt));
                                        buffer_flag = 1;
                                    }
                                }
                                
                                if(destIp) free(destIp);

                                if(!buffer_flag){
                                    write_size= send(dest_serv_connect_fd[i], http_pkt, strlen(http_pkt), MSG_NOSIGNAL);
                                    if(write_size < 0) {
                                        if(errno == EAGAIN || errno == EWOULDBLOCK) {
                                            /* locally store pkt in buffer */
                                            concatenate(dest_write_buffer[i], http_pkt, strlen(http_pkt));
                                            buffer_flag = 1;
                                        }
                                        else {
                                            perror("to dest_server write");

                                            /* closing dest side connection */
                                            if(dest_serv_connect_fd[i] != -1) close(dest_serv_connect_fd[i]);
                                            close(client_fd[i]);
                                            /* updating fd's lists */
                                            for(int j = i; j < conn_cnt-1; j++){
                                                client_fd[j] = client_fd[j+1];
                                                dest_serv_connect_fd[j] = dest_serv_connect_fd[j+1];
                                                memcpy(browser_write_buffer[j], browser_write_buffer[j+1], MAX_PKT_SIZE);
                                                memcpy(dest_write_buffer[j], dest_write_buffer[j+1], MAX_PKT_SIZE);
                                            }
                                            client_fd[conn_cnt-1] = -1;
                                            dest_serv_connect_fd[conn_cnt-1] = -1;
                                            memset(dest_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                                            memset(browser_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                                            conn_cnt--;
                                            flag = 1;
                                            break;
                                        }
                                    }
                                }
                            }

                            else if(ret == NO_HDR) continue;
                            else break;
                        }

                        else {
                            
                            if(buffer_flag) concatenate(dest_write_buffer[i], buffer, read_size);
                            else {
                                write_size= send(dest_serv_connect_fd[i], buffer, read_size, MSG_NOSIGNAL);
                                if(write_size < 0) {
                                    if(errno == EAGAIN || errno == EWOULDBLOCK) {
                                        /* locally store pkt in buffer */
                                        concatenate(dest_write_buffer[i], buffer, read_size);
                                        buffer_flag = 1;
                                    }
                                    else {
                                        perror("to dest server write");

                                        /* closing dest side connection */
                                        if(dest_serv_connect_fd[i] != -1) close(dest_serv_connect_fd[i]);
                                        close(client_fd[i]);
                                        /* updating fd's lists */
                                        for(int j = i; j < conn_cnt-1; j++){
                                            client_fd[j] = client_fd[j+1];
                                            dest_serv_connect_fd[j] = dest_serv_connect_fd[j+1];
                                            memcpy(browser_write_buffer[j], browser_write_buffer[j+1], MAX_PKT_SIZE);
                                            memcpy(dest_write_buffer[j], dest_write_buffer[j+1], MAX_PKT_SIZE);
                                        }
                                        client_fd[conn_cnt-1] = -1;
                                        dest_serv_connect_fd[conn_cnt-1] = -1;
                                        memset(dest_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                                        memset(browser_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                                        conn_cnt--;
                                        flag = 1;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            
            /* continue to updated fd's lists in case of connection break */
            if(flag) {
                i--;
                flag = 0;
                continue;
            }

            /* forwarding from destination server to client(browser) */
            if(dest_serv_connect_fd[i] != -1) {
                if(FD_ISSET(dest_serv_connect_fd[i], &readfd)) {
                    int read_size, write_size;
                    char buffer[BUFFER_SIZE];
                    int buffer_flag = 0;
            
                    while(1){
                        memset(buffer, 0, BUFFER_SIZE);
                        read_size = read(dest_serv_connect_fd[i], buffer, BUFFER_SIZE);
                        if(read_size == 0) {  /* handling connection closed */
                            
                            /* closing connections */
                            close(client_fd[i]);
                            close(dest_serv_connect_fd[i]);
                            /* updating fd's lists */
                            for(int j = i; j < conn_cnt-1; j++){
                                dest_serv_connect_fd[j] = dest_serv_connect_fd[j+1];
                                client_fd[j] = client_fd[j+1];
                                memcpy(browser_write_buffer[j], browser_write_buffer[j+1], MAX_PKT_SIZE);
                                memcpy(dest_write_buffer[j], dest_write_buffer[j+1], MAX_PKT_SIZE);
                            }
                            client_fd[conn_cnt-1] = -1;
                            dest_serv_connect_fd[conn_cnt-1] = -1;
                            memset(dest_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                            memset(browser_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                            conn_cnt--;
                            flag = 1;
                            break;
                        }

                        else if(read_size < 0) {   /* checking for end of packet */
                            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                                break;
                            }
                            else {
                                perror("from dest server read error");
                                
                                /* closing connection */
                                close(client_fd[i]);
                                close(dest_serv_connect_fd[i]);
                                /* updating fd's lists */
                                for(int j = i; j < conn_cnt-1; j++){
                                    dest_serv_connect_fd[j] = dest_serv_connect_fd[j+1];
                                    client_fd[j] = client_fd[j+1];
                                    memcpy(browser_write_buffer[j], browser_write_buffer[j+1], MAX_PKT_SIZE);
                                    memcpy(dest_write_buffer[j], dest_write_buffer[j+1], MAX_PKT_SIZE);
                                }
                                client_fd[conn_cnt-1] = -1;
                                dest_serv_connect_fd[conn_cnt-1] = -1;
                                memset(dest_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                                memset(browser_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                                conn_cnt--;
                                flag = 1;
                                break;
                            }
                        }

                        else {  /* writing to client side */
                            if(buffer_flag) concatenate(browser_write_buffer[i], buffer, read_size);
                            else {
                                write_size= send(client_fd[i], buffer, read_size, MSG_NOSIGNAL);
                                if(write_size < 0) {
                                    if(errno == EAGAIN || errno == EWOULDBLOCK) {
                                        concatenate(browser_write_buffer[i], buffer, read_size);
                                        buffer_flag = 1;
                                    }
                                    else {
                                        perror("to browser write");
                                        
                                        /* closing connection */
                                        close(dest_serv_connect_fd[i]);
                                        close(client_fd[i]);
                                        /* updating fd's lists */
                                        for(int j = i; j < conn_cnt-1; j++){
                                            dest_serv_connect_fd[j] = dest_serv_connect_fd[j+1];
                                            client_fd[j] = client_fd[j+1];
                                            memcpy(browser_write_buffer[j], browser_write_buffer[j+1], MAX_PKT_SIZE);
                                            memcpy(dest_write_buffer[j], dest_write_buffer[j+1], MAX_PKT_SIZE);
                                        }
                                        client_fd[conn_cnt-1] = -1;
                                        dest_serv_connect_fd[conn_cnt-1] = -1;
                                        memset(dest_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                                        memset(browser_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                                        conn_cnt--;
                                        flag = 1;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            /* continue to updated fd's lists in case of connection break */
            if(flag) {
                i--;
                flag = 0;
                continue;
            }

            /* forwarding from browser write buffer to browser side */
            if(FD_ISSET(client_fd[i], &writefd)) {
                int write_size;
                write_size= send(client_fd[i], browser_write_buffer[i], strlen(browser_write_buffer[i]), MSG_NOSIGNAL);
        
                if(write_size < 0) {
                    if(errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("to browser (write)");
                        /* empty write buffer */
                        memset(browser_write_buffer[i], 0, MAX_PKT_SIZE);

                        /* closing dest side connection */
                        close(dest_serv_connect_fd[i]);
                        /* closing client side connection */
                        close(client_fd[i]);
                        /* updating fd's lists and empty buffers*/
                        for(int j = i; j < conn_cnt-1; j++){
                            dest_serv_connect_fd[j] = dest_serv_connect_fd[j+1];
                            client_fd[j] = client_fd[j+1];
                            memcpy(browser_write_buffer[j], browser_write_buffer[j+1], MAX_PKT_SIZE);
                            memcpy(dest_write_buffer[j], dest_write_buffer[j+1], MAX_PKT_SIZE);
                        }
                        client_fd[conn_cnt-1] = -1;
                        dest_serv_connect_fd[conn_cnt-1] = -1;
                        memset(dest_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                        memset(browser_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                        conn_cnt--;
                        flag = 1;
                        break;
                    }
                }

                else {  /* packet written */
                    /* empty write buffer */
                    memset(browser_write_buffer[i], 0, MAX_PKT_SIZE);
                }
            }

            /* continue to updated fd's lists in case of connection break */
            if(flag) {
                i--;
                flag = 0;
                continue;
            }

            /* forwarding from dest write buffer to dest server side */
            if(FD_ISSET(dest_serv_connect_fd[i], &writefd)) {
                int write_size;
                write_size= send(dest_serv_connect_fd[i], dest_write_buffer[i], strlen(dest_write_buffer[i]), MSG_NOSIGNAL);
        
                if(write_size < 0) {
                    if(errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("to dest (write)");
                        /* empty write buffer */
                        memset(dest_write_buffer[i], 0, MAX_PKT_SIZE);

                        /* closing dest side connection */
                        close(dest_serv_connect_fd[i]);
                        /* closing client side connection */
                        close(client_fd[i]);
                        /* updating fd's lists */
                        for(int j = i; j < conn_cnt-1; j++){
                            dest_serv_connect_fd[j] = dest_serv_connect_fd[j+1];
                            client_fd[j] = client_fd[j+1];
                            memcpy(browser_write_buffer[j], browser_write_buffer[j+1], MAX_PKT_SIZE);
                            memcpy(dest_write_buffer[j], dest_write_buffer[j+1], MAX_PKT_SIZE);
                        }
                        client_fd[conn_cnt-1] = -1;
                        dest_serv_connect_fd[conn_cnt-1] = -1;
                        memset(dest_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                        memset(browser_write_buffer[conn_cnt-1], 0, MAX_PKT_SIZE);
                        conn_cnt--;
                        break;
                    }
                }

                else {  /* packet written */
                    /* empty write buffer */
                    memset(dest_write_buffer[i], 0, MAX_PKT_SIZE);
                }
            }
        }

    }

    return 0;
}

int parseHeader(char pkt[], char **destIp, int *destPort){
    int i = 0;
    char *hdr_delim = "\r\n\r\n";
    char *h_end = NULL;
    /* searching header end */
    if((h_end = strstr(pkt, hdr_delim)) == NULL) return NO_HDR;

    char header[MAX_HEADER_SIZE];
    memset(header, 0, MAX_HEADER_SIZE);
    memcpy(header, pkt, (int)(h_end - pkt));
    
    int ret;
    char *delim=", :\r\n";
    char *http_delim = "http://";
    char *tok = NULL;
    char *hdr_dup = NULL;
    char *host_delim = "Host: ";
    char *host = NULL;
    char *port_delim = "Port: ", *alt_port_delim = ":";
    char *port = NULL;
    char *host_start, *port_start;

    do {
        hdr_dup = strdup(header);
        tok = strtok(hdr_dup, delim);
        /* checking for Supported http method */
        if(strstr(SUPPORTED_METHOD,tok) == NULL) {
            ret = HTTP_NOT_SUPPORTED;
            break;
        }

        /* locating host addr and Port */
        host_start = strstr(header, host_delim);
        if(host_start)host_start = host_start + strlen(host_delim);
        port_start = strstr(header, port_delim);
        if(port_start)port_start = port_start + strlen(port_delim);

        if(host_start) {
            host = strdup(host_start);
            tok = strtok(host,delim);
            host = strdup(tok);
            
            struct hostent *host_entry;
            char *IP_add;

            host_entry = gethostbyname(host);
            if(host_entry == NULL) {  /* invalid host name */
                ret = HOST_NOTFOUND;
                break; 
            }
        
            /* host IP if correctly found */
            IP_add = inet_ntoa(*(struct in_addr *)host_entry->h_addr_list[0]);
            *destIp = strdup(IP_add);
            
        }

        else { /* if host is not in pkt */
            ret = HOST_NOTFOUND;
            break;
        }

        if(port_start) {    /* if port number specified by "Port: " keyword in header */
            port = strdup(port_start);
            tok = strtok(port,delim);
            port = strdup(tok);
            *destPort = atoi(port);
        }
        else {
            /* if port number specified by "host:port" format in header */
            host = strdup(host_start);
            host = strtok(host, "\r\n");
            port_start = strstr(host, alt_port_delim);
            if(port_start){
                port_start++;
                port = strdup(port_start);
                tok = strtok(port,delim);
                port = strdup(tok);
                *destPort = atoi(port);
            }
            /* if port number not specified then default port 80 */
            else *destPort = 80;
        }

        /* making url relative */
        char *http_addr = NULL, *temp = NULL;
        hdr_dup = strdup(pkt);
        http_addr = strstr(hdr_dup, http_delim);
        temp = http_addr + strlen(http_delim);
        temp = strtok(temp, "/");
        replaceSubstring(pkt, http_addr, "");
        
        printf("%s\n", header);
        fflush(stdout);
        ret = HOST_FOUND;
    }while(0);
  
    /* free dynamically allocated memory */
    if(hdr_dup) free(hdr_dup);
    if(host) free(host);
    if(port) free(port);
    return ret;
}

void concatenate(char p[], char q[], int len) {
   int c, d;
   c = 0;
   while (p[c] != '\0') {
      c++;      
   }
   d = 0;
   while (q[d] != '\0' && d < len) {
      p[c] = q[d];
      d++;
      c++;    
   }
   p[c] = '\0';
}

void replaceSubstring(char string[],char sub[],char new_str[])
{
    int stringLen,subLen,newLen;
    int i=0,j,k;
    int flag=0,start,end;
    stringLen=strlen(string);
    subLen=strlen(sub);
    newLen=strlen(new_str);

    for(i=0;i<stringLen;i++)
    {
        flag=0;
        start=i;
        for(j=0;string[i]==sub[j];j++,i++)
                if(j==subLen-1)
                        flag=1;
        end=i;
        if(flag==0)
                i-=j;
        else
        {
            for(j=start;j<end;j++)
            {
                for(k=start;k<stringLen;k++)
                        string[k]=string[k+1];
                stringLen--;
                i--;
            }

            for(j=start;j<start+newLen;j++)
            {
                for(k=stringLen;k>=j;k--)
                        string[k+1]=string[k];
                string[j]=new_str[j-start];
                stringLen++;
                i++;
            }
        }
    }
}

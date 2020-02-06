/* SERVER */
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
#include <dirent.h>

#define PORT 8100
#define BUFFERSIZE 1024

void handle_udp_client(int udp_sockid);
void handle_tcp_client(int sockfd);

int max(int x, int y) { 
    if (x > y) 
        return x; 
    else
        return y; 
}

int main() {
    
	struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);

    int udp_sockid, tcp_sockid;

    // setting udp socket
	if((udp_sockid = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("UDP socket creation failed");
		exit(EXIT_FAILURE);
	}

	if(bind(udp_sockid, (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		perror("binding failed");
		exit(EXIT_FAILURE);
	}

    /* setting TCP socket */
    
    if((tcp_sockid = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("TCP socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&serv_addr, 0, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);

	if(bind(tcp_sockid, (const struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
		perror("binding failed");
		exit(EXIT_FAILURE);
	}

    if(listen(tcp_sockid, 10)== -1){
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    fd_set readfd;
    int sel_ret;

    printf("SERVER: Waiting for connection...\n");
    fflush(stdout);

    while(1) {
        FD_ZERO(&readfd);
        FD_SET(udp_sockid, &readfd);
        FD_SET(tcp_sockid, &readfd);
        sel_ret = select(max(tcp_sockid, udp_sockid)+1, &readfd, NULL, NULL, NULL);

        if(sel_ret < 0) {
            perror("SERVER: select error");
            exit(EXIT_FAILURE);
        }

        if(FD_ISSET(tcp_sockid, &readfd)) {
            struct sockaddr_in client_addr;
            socklen_t len;
            len = sizeof(client_addr);
            int sockfd = accept(tcp_sockid,(struct sockaddr *)&client_addr, &len);
            printf("SERVER: TCP Request Received\n");
            fflush(stdout);
            switch(fork()){
                    
                    case -1:
                        perror("SERVER: [TCP] child creation failed");
                        exit(EXIT_FAILURE);
                    case 0:			/* CHILD */
                        
                        handle_tcp_client(sockfd);
                        exit(0);
                    default:		/* PARENT */
                        fflush(stdout);     
            }
        }

        if(FD_ISSET(udp_sockid, &readfd)) {
            printf("SERVER: UDP Request Received\n");
            fflush(stdout);
            handle_udp_client(udp_sockid);
        }

        printf("SERVER: Waiting for connection...\n");
        fflush(stdout);
    }
    
} 

void handle_udp_client(int udp_sockid) {
    
    char buffer[BUFFERSIZE];
    struct sockaddr_in client_addr;
    socklen_t len;
    int n;
    len = sizeof(client_addr);
    memset(buffer, 0, BUFFERSIZE);

    n = recvfrom(udp_sockid, buffer, BUFFERSIZE, 0, (struct sockaddr *)&client_addr, &len);
    buffer[n] = '\0';
    printf("UDP_CLIENT: %s\n", buffer);
    fflush(stdout);

    struct hostent *host_entry;
    char *IP_add;

    host_entry = gethostbyname(buffer);
    if(host_entry == NULL) {  // host name not found
        sendto(udp_sockid, "NOTFOUND", strlen("NOTFOUND"), 0, (struct sockaddr *)&client_addr, len);
        printf("UDP_SERVER: host name not found\n");
        fflush(stdout);  
    }

    else {
        IP_add = inet_ntoa(*(struct in_addr *)host_entry->h_addr_list[0]);
        sendto(udp_sockid, (const char *)IP_add, strlen(IP_add), 0, (struct sockaddr *)&client_addr, len);
        printf("UDP_SERVER: IP sent to Client\n");
        fflush(stdout);
    }

    return ;
}

void concatenate(char p[], char q[]) {
   int c, d;
   c = 0;
   while (p[c] != '\0') {
      c++;      
   }
   d = 0;
   while (q[d] != '\0') {
      p[c] = q[d];
      d++;
      c++;    
   }
   p[c] = '\0';
}

void handle_tcp_client(int sockfd) {
	char buffer[BUFFERSIZE];
	int read_size, write_size;
	int file_fd;

	printf("TCP_SERVER: Client Connected.....waiting for client's message...\n");
    fflush(stdout);

	memset(buffer, 0, BUFFERSIZE);
	read_size = read(sockfd, buffer, BUFFERSIZE);		// reading subdirectories name
	buffer[read_size] = '\0';
	printf("TCP_CLIENT: %s\n", buffer);
	fflush(stdout);

	char path[15] = "image/";
	char sl[2]="/";
	concatenate(path, buffer);
	
	concatenate(path,sl);
	
	struct dirent *de;
	DIR *dr = opendir(path);
	
	if (dr == NULL)  //  NULL if couldn't open directory 
    { 
        printf("TCP_SERVER: Could not open subdirectory\n" );
        fflush(stdout);
        write(sockfd, "directory_not_found", BUFFERSIZE);
        close(sockfd);
        exit(0); 
    } 
   
    while ((de = readdir(dr)) != NULL){ 
            if(strcmp(de->d_name,".") && strcmp(de->d_name,"..")) {
            	char path1[40];
            	memset(path1, 0, 40);
            	concatenate(path1, path);
            	concatenate(path1, de->d_name);
            	
            	if((file_fd = open(path1, O_RDONLY)) == -1) {
			        printf("TCP_SERVER: File Not Found\n");
			        printf("TCP_SERVER: closing connection...\n");
                    write(sockfd, "directory_not_found", BUFFERSIZE);
			        close(sockfd);
			        exit(0);
			    }

			    else {
                    printf("TCP_SERVER: Sending %s\n",path1);
			        while(read_size = read(file_fd, buffer, BUFFERSIZE)) {
			            write_size = write(sockfd, buffer, BUFFERSIZE);
			            memset(buffer, 0x00, BUFFERSIZE);
			        }
			        write_size = write(sockfd, "__DONE__", BUFFERSIZE);          // one image done
			        memset(buffer, 0x00, BUFFERSIZE);
			        close(file_fd);
			    }
            }
  	}
  	write_size = write(sockfd, "END", strlen("END"));
  	printf("TCP_SERVER: Images sent to the client\n");
    printf("TCP_SERVER: closing connection...\n");
    fflush(stdout);
    close(sockfd);
    closedir(dr);
}
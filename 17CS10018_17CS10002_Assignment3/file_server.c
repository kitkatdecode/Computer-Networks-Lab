/* S E R V E R */

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <fcntl.h> 

#define PORT 8181
#define BUFFERSIZE 100


int main(){
	int sockid;
	struct sockaddr_in ser_addr, cli_addr;


	if((sockid = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&ser_addr, 0, sizeof(ser_addr));

	ser_addr.sin_family = AF_INET;
	ser_addr.sin_addr.s_addr = INADDR_ANY;
	ser_addr.sin_port = htons(PORT);

	if(bind(sockid, (const struct sockaddr *) &ser_addr, sizeof(ser_addr)) < 0){
		perror("binding failed");
		exit(EXIT_FAILURE);
	}

    if(listen(sockid, 1)== -1){
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    size_t read_size, write_size;
	socklen_t len;
	char buffer[BUFFERSIZE];
	int file_fd, sockfd;

	while(1){
        len = sizeof(cli_addr);
        printf("SERVER: waiting for connection.....\n");
        fflush(stdout);

        if((sockfd = accept(sockid, (struct sockaddr *)&cli_addr, &len)) == -1){
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        printf("SERVER: Client Connected.....waiting for client's message...\n");
        fflush(stdout);

		memset(buffer, 0, BUFFERSIZE);
		read_size = read(sockfd, buffer, BUFFERSIZE);
		buffer[read_size] = '\0';
		printf("CLIENT: %s\n", buffer);
		fflush(stdout);

		if((file_fd = open(buffer, O_RDONLY)) == -1) {
            printf("SEREVR: File Not Found\n");
            printf("SERVER: closing connection...\n");
            close(sockfd);
        }

        else {
            while(read_size = read(file_fd, buffer, BUFFERSIZE)) {
                write_size = write(sockfd, buffer, read_size);
                //printf("%s\n", buffer);
                memset(buffer, 0x00, BUFFERSIZE);
            }
            printf("SERVER: File sent to the client\n");
            printf("SERVER: closing connection...\n");
            fflush(stdout);
            close(file_fd);
            close(sockfd);
        }	
	}

	close(sockid);
	return 0;

}
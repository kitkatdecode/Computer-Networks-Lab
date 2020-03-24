/* S E R V E R */

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <rsocket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define PORT 70036
#define BUFFERSIZE 1024


int main(){
	int sockid;
	struct sockaddr_in ser_addr, cli_addr;


	if((sockid = r_socket(AF_INET, SOCK_MRP, 0)) < 0){
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

	printf("Server Running.....waiting for client's message...\n");
	fflush(stdout);

	int n;
	socklen_t len;
	char buffer[BUFFERSIZE];
	FILE *fp;

	len = sizeof(cli_addr);

	while(1){
		memset(buffer, 0, BUFFERSIZE);
		n = recvfrom(sockid, buffer, BUFFERSIZE, 0, (struct sockaddr *)&cli_addr, &len);
		buffer[n] = '\0';
		printf("CLIENT: %s\n", buffer);
		fflush(stdout);

		fp = fopen(buffer, "r");
		
		if(fp == NULL){
			char err_msg[] = "NOTFOUND ";
			strcat(err_msg, buffer);

			printf("SERVER: File not found\n");
			fflush(stdout);

			sendto(sockid, err_msg, strlen(err_msg), 0, (struct sockaddr *)&cli_addr, len);
		}
	
		else {
			fscanf(fp, "%s", buffer);
			sendto(sockid, buffer, strlen(buffer), 0, (struct sockaddr *)&cli_addr, len);
			
			printf("SERVER: File found, sending to Client\n");
			fflush(stdout);
			
			while(1){
				n = recvfrom(sockid, buffer, BUFFERSIZE, 0, (struct sockaddr *)&cli_addr, &len);
				buffer[n] = '\0';

				printf("CLIENT: %s\n", buffer);
				fflush(stdout);
				memset(buffer, 0, BUFFERSIZE);
				
				if(fscanf(fp, "%s", buffer) == EOF) break;

				sendto(sockid, buffer, strlen(buffer), 0, (struct sockaddr *)&cli_addr, len);

				if(!strcmp(buffer,"END"))break;
			}
			fclose(fp);
			printf("SERVER: File sent to the Client\n");
			fflush(stdout);
		}
	}

	close(sockid);
	return 0;

}
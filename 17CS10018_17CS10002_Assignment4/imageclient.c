/* IMAGE C L I E N T */

#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>

#define PORT 8100
#define BUFFER_SIZE 1024

int main(){
	int sockfd;
	struct sockaddr_in servaddr;
	socklen_t addrlen;

	// Creating socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));

	// Server info
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = INADDR_ANY;
	addrlen = sizeof(struct sockaddr);

	if(connect(sockfd, (const struct sockaddr *)&servaddr, addrlen) == -1){  // connecting to tcp server
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    int no_of_images = 0;
    char buffer[BUFFER_SIZE];

    char* dir;
    printf("Enter the directory name: ");
    scanf("%s",dir);

	if(write(sockfd, dir, strlen(dir))==-1){			// sending directory name
    	perror("connection closed");
    	exit(EXIT_FAILURE);
	}

    while(1){
    	int read_len;
    	memset(buffer, 0, BUFFER_SIZE);

    	if((read_len = read(sockfd, buffer, BUFFER_SIZE))== 0){		// reading images
    	    printf("Connection Closed\n");
            exit(1);
    	}

    	if(!strcmp(buffer,"__DONE__")){							// received one image
    		no_of_images++;
    		continue;
    	}

    	if(!strcmp(buffer,"END")){										// received all images
    		printf("Num of images received = %d\n",no_of_images);
            close(sockfd);
    		return 0;
    	}

        if(!strcmp(buffer,"directory_not_found")){                         // received one image
            printf("Incorrect directory: Connection Closed\n");
            close(sockfd);
            return 0;
        }

    }
    
    return 0;
}

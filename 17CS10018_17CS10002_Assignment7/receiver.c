/* R E C E I V E R */

#include <string.h>
#include "rsocket.h"
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 70037
#define BUFFER_SIZE 1

int main(){
    int sockfd;
    struct sockaddr_in sendaddr,recvaddr;

    // creating sockfd
    sockfd = socket(AF_INET, SOCK_MRP,0);
    if(sockfd < 0){
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&recvaddr, 0, sizeof(recvaddr));

    // Server info
    recvaddr.sin_family = AF_INET;
    recvaddr.sin_port = htons(PORT);
    recvaddr.sin_addr.s_addr = INADDR_ANY;

    if(r_bind(sockfd, (const struct sockaddr *) &recv_addr, sizeof(recv_addr)) < 0){
		perror("binding failed");
		exit(EXIT_FAILURE);
	}

    char buffer;

    while(recvfrom(sockfd, &buffer,1, 0, 
                            (const struct sockaddr *) &send_addr, sizeof(send_addr)) == 1)
        printf("%c",buffer);
    r_close(sockfd);
    return 0;
}
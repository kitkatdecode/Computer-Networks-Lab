#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8100
#define BUFFER_SIZE 1024

int main(){
    int sockfd;
    struct sockaddr_in servaddr;

    // creating sockfd
    sockfd = socket(AF_INET, SOCK_DGRAM,0);
    if(sockfd < 0){
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Server info
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    int n;
    int recv_msg_len;
    char buffer[BUFFER_SIZE];

    // sending filename and creating a new file if hello is received
    char domain_name[BUFFER_SIZE];
    printf("Enter the domain name : ");
    scanf("%s", domain_name);
    sendto(sockfd, (const char *)domain_name, strlen(domain_name), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    
    memset(buffer, 0, BUFFER_SIZE);
    recv_msg_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
    buffer[recv_msg_len] = '\0';
    
    // Error handling
    char not_found[] = "NOTFOUND";
    
    if(!strcmp(buffer, not_found)){
        printf("Error: in finding domain IP\n");
        return 0;
    }

    // printing and exiting
    printf("IP : %s\n", buffer);
    fflush(stdout);
    return 0;
}
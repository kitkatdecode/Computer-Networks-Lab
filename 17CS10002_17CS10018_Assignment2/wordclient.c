/* C L I E N T */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 8181
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
    char filename[BUFFER_SIZE];
    printf("Enter the filename : ");
    scanf("%s", filename);
    sendto(sockfd, (const char *)filename, strlen(filename), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    
    recv_msg_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
    buffer[recv_msg_len] = '\0';
    printf("SERVER: %s\n", buffer);
    fflush(stdout);
    
    char not_found[] = "NOTFOUND ";
    
    if(!strcmp(buffer, strcat(not_found, filename))){
        printf("Error: File %s Not Found\n", filename);
        return 0;
    }
    
    FILE *file_ptr;
    file_ptr = fopen("recv_file.txt", "w");

    // sending requests and printing in file
    char word[20];
    int i = 1;
    while(1){
        memset(word, 0, 20);
        sprintf(word, "WORD%d", i);
        sendto(sockfd, (const char *)word, strlen(word), 0, 
                        (const struct sockaddr *)&servaddr, sizeof(servaddr));

        memset(buffer, 0, BUFFER_SIZE);
        recv_msg_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, NULL, NULL);
        buffer[recv_msg_len] = '\0';
        printf("SERVER: %s\n", buffer);
        fflush(stdout);
        
        if(strcmp(buffer,"END"))fprintf(file_ptr, "%s\n", buffer);
        else
        {
            fclose(file_ptr);
            close(sockfd);
            return 0;
        }
        i++;
    }

    return 0;
}
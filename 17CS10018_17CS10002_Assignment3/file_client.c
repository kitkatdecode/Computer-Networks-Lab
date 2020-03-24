/* C L I E N T */

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
#define BUFFER_SIZE 100

int words_in(char buffer[], char *prev){
    char *delim=",;: .\t\n";
    char *tok, *ptr, *s;
    
    s = strdup(buffer);
    
    tok = strtok(s,delim);
    int i=0;
    while(tok != NULL){
        i++;
        tok=strtok(NULL,delim);
    }
    
    if(strchr(delim, *prev) == NULL && strchr(delim, buffer[0]) == NULL) i=i-1;
    *prev = buffer[strlen(buffer)-1];

    return i;
}

int main(){
    int sockfd;
    struct sockaddr_in servaddr;
    socklen_t addrlen;

    // creating sockfd
    sockfd = socket(AF_INET, SOCK_STREAM,0);
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

    if(connect(sockfd, (const struct sockaddr *)&servaddr, addrlen) == -1){
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    int w_len;
    int recv_msg_len;
    char buffer[BUFFER_SIZE];

    // sending filename and creating a new file if hello is received
    char filename[BUFFER_SIZE];
    printf("Enter the filename : ");
    scanf("%s", filename);
    
    if((w_len = write(sockfd, filename, strlen(filename)))==-1){
        perror("connection closed");
        exit(EXIT_FAILURE);
    }

    char prev = ' ';
    int n_bytes = 0, n_words = 0;
    int file_fd;
    memset(buffer, 0x00, BUFFER_SIZE);
    if((recv_msg_len = read(sockfd, buffer, BUFFER_SIZE)) == 0){		// file not found ..connection closed 
        printf("File Not Found\n");
        return 0;
    }

    else{
        
        file_fd = open("recv_file.txt", O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR);	// output file

        n_bytes += recv_msg_len;
        n_words += words_in(buffer, &prev);

        if((w_len = write(file_fd, buffer, recv_msg_len))==0) {
            perror("file error");
        }
        int n_pkt =1;
        while(1){
            memset(buffer, 0x00, BUFFER_SIZE);
            if((recv_msg_len = read(sockfd, buffer, BUFFER_SIZE))== 0){		// file received ..connection closed
                close(file_fd);
                printf("The file transfer is successful.\nSize of the file = %d bytes, Number of words = %d\n", n_bytes, n_words);
                printf("Number of TCP packets received = %d\n", n_pkt);
                fflush(stdout);
                return 0;
            }
            
            n_pkt+=1;
            n_bytes += recv_msg_len;
            n_words += words_in(buffer, &prev);
            w_len = write(file_fd, buffer, recv_msg_len);

        }
    }

    return 0;
}
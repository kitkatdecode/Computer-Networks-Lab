/* S E N D E R */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h> 
#include <rsocket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#define PORT 70036
#define RECV_PORT 70037
#define MSGSIZE 1024


int main(){
	int sockid;
	struct sockaddr_in send_addr, recv_addr;


	if((sockid = r_socket(AF_INET, SOCK_MRP, 0)) < 0){
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&send_addr, 0, sizeof(send_addr));

    // sender's info
	send_addr.sin_family = AF_INET;
	send_addr.sin_addr.s_addr = INADDR_ANY;
	send_addr.sin_port = htons(PORT);

    memset(&recv_addr, 0, sizeof(recv_addr));

    // receiver's info
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(RECV_PORT);
    recv_addr.sin_addr.s_addr = INADDR_ANY;

	if(r_bind(sockid, (const struct sockaddr *) &send_addr, sizeof(send_addr)) < 0){
		perror("binding failed");
		exit(EXIT_FAILURE);
	}

	printf("Sender Running\n");
	fflush(stdout);

    char msg[MSGSIZE];
    memset(msg, 0, MSGSIZE);
    printf("Enter the message : ");
    scanf("%s", msg);

    printf("sending <=100 characters\n");

    // sending message character wise
    int msglen = strlen(msg);
    if(msglen>100)
        msglen = 100;
    r_sendto(sockid, (const char *)msg[0], strlen(msg[0]), 0,
                    (const struct sockaddr *)&recv_addr, sizeof(recv_addr));
    for(int i=1;i<msglen;i++){
        r_sendto(sockid, (const char *)msg[i], strlen(msg[i]), 0,
                    (const struct sockaddr *)&recv_addr, sizeof(recv_addr));
    }

    r_close(sockid);
    return 0;
}
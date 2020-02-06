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
//#include <sys/select.h>

#define PORT 8181
#define BUFFERSIZE 22000


int main() {
	int fd,rc,wr,fd_o;
	char buffer[BUFFERSIZE];
	fd = open("images/im1/1.jpg", O_RDONLY);
	fd_o=open("recv_file.jpg", O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
	rc = read(fd,buffer,BUFFERSIZE);
	wr=write(fd_o,buffer,sizeof(buffer));

	return 0;
}
    
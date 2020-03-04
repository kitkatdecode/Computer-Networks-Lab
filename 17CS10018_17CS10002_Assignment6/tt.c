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

#define MAX_CONNECTION 300
#define BUFFER_SIZE 1024
#define MAX_HEADER_SIZE 65535
#define MAX_PKT_SIZE 65535

#define SUPPORTED_METHOD  "GET POST"
#define HOST_FOUND 1
#define NO_HDR 0
#define HTTP_NOT_SUPPORTED -1
#define HOST_NOTFOUND -2

int parseHeader(char pkt[], char **destIp, int *destPort){
    int i = 0;
    char *hdr_delim = "\r\n\r\n";
    char *h_end = NULL;
    if((h_end = strstr(pkt, hdr_delim)) == NULL) return NO_HDR;
    char header[MAX_HEADER_SIZE];
    memset(header, 0, MAX_HEADER_SIZE);
    memcpy(header, pkt, (int)(h_end - pkt));

    printf("%s\n", header);
        
    int ret;
    char *delim=", \n";
    char *tok = NULL;
    char *hdr_dup = NULL;
    char *host_delim = "Host: ";
    char *host = NULL;
    char *port_delim = "Port: ";
    char *port = NULL;
    char *host_start, *port_start;

    do {
        hdr_dup = strdup(header);
        tok = strtok(hdr_dup, delim);
        if(strstr(SUPPORTED_METHOD,tok) == NULL) {
            ret = HTTP_NOT_SUPPORTED;
            printf("\nNOT SUPPORTED\n\n");
            break;
        }

        
        host_start = strstr(header, host_delim);
        if(host_start)host_start = host_start + strlen(host_delim);
        port_start = strstr(header, port_delim);
        if(port_start)port_start = port_start + strlen(port_delim);

        if(host_start) {
            host = strdup(host_start);
            tok = strtok(host,delim);
            host = strdup(tok);

            struct hostent *host_entry;
            char *IP_add;

            host_entry = gethostbyname(host);
            if(host_entry == NULL) {  // host name not found
                ret = HOST_NOTFOUND;
                printf("\nHOST NOT FOUND\n%s\n", host);
            
                break; 
            }
            printf("%s\n",host);
            IP_add = inet_ntoa(*(struct in_addr *)host_entry->h_addr_list[0]);
            *destIp = strdup(IP_add);
        }

        else {
            ret = HOST_NOTFOUND;
            printf("\nHOST NOT FOUND 2\n\n");
            
            break;
        }

        if(port_start) {
            port = strdup(port_start);
            tok = strtok(port,delim);
            port = strdup(tok);
            *destPort = atoi(port);
        }
        else {
            *destPort = 80;
        }

        printf("%s\n", header);
        ret = HOST_FOUND;
    }while(0);

    
    if(hdr_dup) free(hdr_dup);
    if(host) free(host);
    if(port) free(port);
    return ret;
}

int main() {
	char *h="GET http://detectportal.firefox.com/success.txt HTTP/1.1 Host: detectportal.firefox.com\nUser-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefo/68.0\r\n\r\n";

	char *ip;
	int port;

	int r = parseHeader(h,&ip,&port);
	if(r>0)printf("%s\n%d\n",ip,port);

	return 0;
}
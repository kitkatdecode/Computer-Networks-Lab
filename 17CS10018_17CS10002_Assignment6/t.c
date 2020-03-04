#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h> 
#include <unistd.h> 
#include <arpa/inet.h> 

#define MAX_HEADER_SIZE 65535
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

    int ret;
    char *delim=", \n";
    char *tok = NULL, *method = "GET POST";
    char *hdr_dup = NULL;
    char *host_delim = "Host: ";
    char *host = NULL;
    char *port_delim = "Port: ";
    char *port = NULL;
    char *host_start, *port_start;

    do {
        hdr_dup = strdup(header);
        tok = strtok(hdr_dup, delim);
        if(strstr(method,tok) == NULL) {
            ret = HTTP_NOT_SUPPORTED;
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
                break; 
            }
        
            IP_add = inet_ntoa(*(struct in_addr *)host_entry->h_addr_list[0]);
            *destIp = strdup(IP_add);
        }

        else {
            ret = HOST_NOTFOUND;
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

        printf("%s\n%lu\n%lu\n", header, strlen(header), sizeof(header));
        ret = HOST_FOUND;
    }while(0);

    
    if(hdr_dup) free(hdr_dup);
    printf("1\n");
    if(host) free(host);
    printf("1\n");
    if(port) free(port);
    printf("1\n");
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
/* 
 * File : http_client_20161606.cc
 *
 * Date : 2021/04/09
 * Author : 20161606 Minbo-Shim

 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAXDATASIZE 1000

int main(int argc, char* argv[]){
	
	char urlPrefix[10];
	char hostname[100] = "\0";
	char filePath[100] = "\0";
	char port[8] = "\0";
	char* tmp;
	char* header;
	char* data;
	int content_length = 0;
	int received_length = 0;
	int headerCapacity = MAXDATASIZE;
	
	// FLAG
	int headerCompleted = 0;
	int contentLengthSpecified = 0;

	FILE* fp;	// output file pointer.

	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo;
	int rv;
	char s[INET_ADDRSTRLEN];

	if(argc != 2) {
		fprintf(stderr, "usage: http_client http://hostname[:port][/path/to/file]\n");
		exit(1);
	}

	// parsing "http://" and check
	strncpy(urlPrefix, argv[1], strlen("http://"));

	if(strcmp(urlPrefix, "http://") != 0){
		fprintf(stderr, "usage: http_client http://hostname[:port][/path/to/file]\n");
		exit(1);
	}
	memmove(argv[1], argv[1]+strlen("http://"), strlen(argv[1])-strlen("http://")+1);

	// parsing hostname & port & filePath
	for(int i = strlen("http://")+1 ; i < strlen(argv[1]) ; i++) {
		if(argv[1][i] == ':'){	// if there is port number
			strcpy(hostname, strtok(argv[1], ":"));
			tmp = strtok(NULL, "/");
			if(tmp != NULL)
				strcpy(port, tmp);
			tmp = strtok(NULL, " ");
			if(tmp != NULL)
				strcpy(filePath, tmp);
			break;
		}
		else if(argv[1][i] == '/'){	// if there is no port number 
			strcpy(hostname, strtok(argv[1], "/"));
			if(argv[1][i+1] != '\0')
				strcpy(filePath, strtok(NULL, " "));
			break;
		}
	}
	if(hostname[0] == '\0')
		strcpy(hostname, strtok(argv[1], " "));
	if(port[0] == '\0')
		strcpy(port, "80");

	// Init hints
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	// Create socket using servinfo
	if((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
		perror("client: socket");
		exit(1);
	}

	// TCP connect
	if(connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
		close(sockfd);
		perror("connect");
		exit(1);
	}

	// Debug
	//inet_ntop(servinfo->ai_family, &((struct sockaddr_in*)servinfo->ai_addr)->sin_addr, s, sizeof s);
	//printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo);

	// Write message
	sprintf(buf, "GET /%s HTTP/1.1\r\nHost: %s:%s\r\n\r\n",filePath, hostname, port);

	// Send message
	if(send(sockfd, buf, strlen(buf), 0) == -1) {
		perror("send");
		close(sockfd);
		exit(1);
	}

	// Receive message
	header = (char*)malloc(sizeof(char)*headerCapacity);
	header[0] = '\0';

	while((numbytes = recv(sockfd, buf, sizeof buf, 0)) != 0){
		if(numbytes == -1){
			perror("recv");
			close(sockfd);
			exit(1);
		}

		buf[numbytes] = '\0';
		rewind(stdout);

		if(!headerCompleted){
			if((tmp = strstr(buf, "\r\n\r\n")) == NULL){
				// Not all header has been delivered yet.
				headerCapacity += MAXDATASIZE;
				if(!(header = (char*)realloc(header, sizeof(char)*headerCapacity))){
					fprintf(stderr, "Insufficient memory\n");
					exit(1);
				}
				strcat(header, buf);
			}
			else{
				/*
				 [addr] buf | buf+1 | buf+2 | --- | tmp | tmp+1 | --- | tmp+4 | --- 
				 [val ] <-----------header ------------> <--\r\n\r\n-> <-- data---->
				 */
				int lastHeaderIdx = (int)(tmp-buf);

				// Split the string ( header | data )
				// by inserting NULL character inside
				buf[lastHeaderIdx] = '\0';
				strcat(header, buf);
				headerCompleted = 1;	
			}
		}

		// After receiving entire header, 
		if(headerCompleted){
			char* search;

			if((search = strstr(header, "Content-Length: ")) != NULL){
				contentLengthSpecified = 1;
			}
			else if((search = strstr(header, "Content-length: ")) != NULL){
				contentLengthSpecified = 1;
			}
			else if((search = strstr(header, "content-length: ")) != NULL){
				contentLengthSpecified = 1;
			}

			if(contentLengthSpecified){
				search = strtok(search, ":");
				content_length = atoi(strtok(NULL, " "));
				data = (char*)malloc(sizeof(char) * content_length);
				strcat(data, tmp+4);
				received_length = strlen(data);
			}
			break;
		}
	}
	
	// Print status code
	printf("%s\n", strtok(header, "\r\n"));

	// if content-length  not specified
	if(!contentLengthSpecified){
		printf("Content-Length not specified\n");
		exit(1);
	}

	// Open output file.
	if((fp = fopen("20161606.out", "w")) == NULL){
		fprintf(stderr, "fopen error\n");
		exit(1);
	}
	
	// Receive rest of data
	while((received_length < content_length) &&
			((numbytes = recv(sockfd, buf, sizeof buf, 0)) != 0)){
		if(numbytes == -1){
			perror("recv");
			close(sockfd);
			exit(1);
		}
		buf[numbytes] = '\0';
		strcat(data, buf);	
		received_length += numbytes;
	}
	
	// Write data to the file
	fprintf(fp, "%s", data);
	printf("%d bytes written to 20161606.out\n", content_length);

	// close socket
	close(sockfd);

	return 0;
}

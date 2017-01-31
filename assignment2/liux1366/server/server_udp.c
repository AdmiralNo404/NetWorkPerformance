#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "../message.h"

#define TRIALS 5

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int fileSend(FILE* myfd, int sock, struct sockaddr_in *cli, int clilen) {
	/*** Get file size ***/
	struct stat stats;
	int d = fileno(myfd);
	fstat(d, &stats);
	int size = stats.st_size;
	/*** Get max seq number ***/
	int max = (size - 1) / BUF_SZ + 1;
	
	/*** main loop ***/
	int finished = 0;
	int seq = 1;
	while (!finished) {
		/*** Read from file ***/
		char buffer[BUF_SZ];
		int cread = fread(&buffer, sizeof(char), BUF_SZ, myfd);
		if (seq == max) {
			finished = 1;
		}
		
		/*** Construct response message ***/
		struct msg_t response;
		initMsg(&response, 3, seq, max, cread, buffer);
		sendto(sock, &response, sizeof(response), 0, (struct sockaddr *) cli, clilen);
		seq++;
		
		/*** Receive acknowledge message ***/
		struct msg_t ack;
		recvfrom(sock, &ack, sizeof(ack), 0, (struct sockaddr *) cli, &clilen);
		
		if (ack.msg_type != 4) {
			printf("Failed to receive ACK!\n");
			close(sock);
			exit(1);
		}
		
		printMsg(&ack, 0);
	}
}

int main(int argc, char** argv) {
	int sockfd, portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	struct hostent *hostp;
	char* hostaddrp;
	char s[INET6_ADDRSTRLEN];
	
	/*** Check input format ***/
	if (argc < 2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}
	
	/*** Create a new socket ***/
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("server: socket");
		exit(1);
	}
	
	/*** Fill in sockaddr struct ***/
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    portno = atoi(argv[1]);
    serv_addr.sin_port = portno;
    
    /*** Bind name to socket ***/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("server: bind");
		exit(1);
	}
	
	/*** Receive a datagram from a client ***/
	clilen = sizeof(cli_addr);
	struct msg_t request;
	recvfrom(sockfd, &request, sizeof(request), 0, (struct sockaddr *) &cli_addr, &clilen);
	
	/*** Determine the sender address ***/
	gethostbyaddr((const char *)&cli_addr.sin_addr.s_addr, sizeof(cli_addr.sin_addr.s_addr), AF_INET);
	inet_ntop(cli_addr.sin_family, get_in_addr((struct sockaddr *) &cli_addr), s, sizeof(s));
	printf("server: got datagram from %s\n", s);
	
	printMsg(&request, 0);
	
	/*** Try to open file ***/
	FILE* myfd = fopen(request.payload, "r");
	if (myfd == NULL) {
		struct msg_t res_get_err;
		initMsg(&res_get_err, 2, 0, 1, 0, "");
		sendto(sockfd, &res_get_err, sizeof(res_get_err), 0, (struct sockaddr *) &cli_addr, clilen);
		perror("open");
		exit(1);
	}
	
	/*** Send the file ***/
	fileSend(myfd, sockfd, &cli_addr, clilen);
	
	/*** Wait for client to finish ***/
	struct msg_t finish;
	recvfrom(sockfd, &finish, sizeof(finish), 0, (struct sockaddr *) &cli_addr, &clilen);
	printMsg(&finish, SERVER);
	
	/*** Close connection ***/
	close(sockfd);
	
	return 1;
}

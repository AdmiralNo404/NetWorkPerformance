#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

int main(int argc, char** argv) {
	int sockfd;
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
	int portno = atoi(argv[1]);
	
	/*** Create a new socket ***/
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("server: socket");
		exit(1);
	}
	
	/*** Fill in sockaddr struct ***/
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = portno;
    
    /*** Bind name to socket ***/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("server: bind");
		exit(1);
	}
	
	while (1) {
		/*** Receive a datagram from a client ***/
		clilen = sizeof(cli_addr);
		int n;
		recvfrom(sockfd, &n, sizeof(n), 0, (struct sockaddr *) &cli_addr, &clilen);
		printf("got: %d\n", n);
		sendto(sockfd, &n, sizeof(n), 0, (struct sockaddr *) &cli_addr, clilen);
	}
	
	return 1;
}

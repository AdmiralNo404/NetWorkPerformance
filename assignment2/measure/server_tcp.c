#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#define BUFSZ 256
#define BACKLOG 5

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char** argv) {
	int sockfd, newsockfd;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	char s[INET6_ADDRSTRLEN];
	char buffer[BUFSZ];
	
	/*** Check input format ***/
	if (argc < 2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}
	int portno = atoi(argv[1]);
	
	/*** Create a new socket ***/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
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
	
	/*** Listen for requests ***/
	if (listen(sockfd, BACKLOG) < 0) {
		perror("listen");
		exit(1);
	}
	clilen = sizeof(cli_addr);
	
	/*** Accept connection ***/
	if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) < 0) {
		perror("accept");
		exit(1);
	}
	
	inet_ntop(cli_addr.sin_family, get_in_addr((struct sockaddr *) &cli_addr), s, sizeof s);
	printf("server: got connection from %s\n", s);
	
	/*** Receive message ***/
	if ((recv(newsockfd, buffer, BUFSZ, 0)) < 0) {
		perror("recv");
		exit(1);
	}
	
	printf("The client said: %s\n", buffer);
	
	/*** Reply ***/
	if ((send(newsockfd, "Hello client!", 14, 0)) < 0) {
		perror("send");
		exit(1);
	}
	
	return 1;
	
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "../message.h"

#define BACKLOG 5

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int fileSend(FILE* fd, int sock) {
	/*** Get file size ***/
	struct stat stats;
	int d = fileno(fd);
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
		int cread = fread(buffer, sizeof(char), BUF_SZ, fd);
		if (seq == max) {
			finished = 1;
		}
		
		/*** Construct response message ***/
		struct msg_t response;
		initMsg(&response, 3, seq, max, cread, buffer);
		//send(sock, &response, sizeof(response), 0);
		if (send(sock, &response, sizeof(response), 0) < 0) {
			perror("send");
			close(sock);
			exit(1);
		}
		seq++;
		
		/*** Receive acknowledge message ***/
		struct msg_t ack;
		//recv(sock, &ack, sizeof(ack), 0);
		if (recv(sock, &ack, sizeof(ack), MSG_WAITALL) < 0) {
			perror("recv");
			close(sock);
			exit(1);
		}
		
		if (ack.msg_type != 4) {
			printf("Failed to receive ACK!\n");
			close(sock);
			exit(1);
		}
		
		printMsg(&ack, SERVER);
		
	}
	
	return 1;
}

int main(int argc, char** argv) {
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	char s[INET6_ADDRSTRLEN];
	
	/*** Check input format ***/
	if (argc < 2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

	/*** Create a new socket ***/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("server: socket");
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
	
	/*** Recieve request ***/
	struct msg_t request;
	if ((recv(newsockfd, &request, sizeof(request), MSG_WAITALL)) < 0) {
		perror("recv");
		exit(1);
	}
	
	printMsg(&request, SERVER);
	
	/*** Check message type ***/
	if (request.msg_type == 1) {
		FILE* myfd = fopen(request.payload, "r");
		if (myfd == NULL) {
			struct msg_t res_get_err;
			initMsg(&res_get_err, 2, 0, 1, 0, "");
			send(newsockfd, &res_get_err, sizeof(res_get_err), 0);
			perror("open");
			exit(1);
		}
		
		/*** Send the file ***/
		fileSend(myfd, newsockfd);
		
	} else {
		/*** Respond with invalid message ***/
		struct msg_t invalid;
		initMsg(&invalid, 0, 0, 1, 0, "");
		send(newsockfd, &invalid, sizeof(invalid), 0);
		printf("Invalid message type: %s, expecting %s\n", str_map[request.msg_type], str_map[1]);
		exit(1);
	}
	
	/*** Wait for client to finish ***/
	struct msg_t finish;
	if ((recv(newsockfd, &finish, sizeof(finish), MSG_WAITALL)) < 0) {
		perror("recv");
		exit(1);
	}
	printMsg(&finish, SERVER);
	
	/*** Close connection ***/
	close(newsockfd);
	close(sockfd);
	
	return 0;
}

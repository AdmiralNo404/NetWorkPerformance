#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFSZ 256
#define BACKLOG 5

int main(int argc, char** argv) {
	int sockfd, newsockfd;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	
	/*** Check input format ***/
	if (argc < 2) {
		printf("Usage: %s <ds_port>\n", argv[0]);
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
    serv_addr.sin_port = htons(0);
        
    /*** Bind name to socket ***/
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("server: bind");
		exit(1);
	}
	
	/******************* Register with dir-server *********************/
	int ds_sock, ds_port;
	struct sockaddr_in ds_addr;
	struct hostent *host;
	//char ds_name[] = "127.0.0.1";
	char ds_name[] = "apollo.cselabs.umn.edu";
	//char ds_name[] = "atlas.cselabs.umn.edu";
	
	/*** Create a new socket ***/
	if ((ds_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("server: socket");
	}
	
	/*** Fill out sockaddr struct ***/
	if ((host = gethostbyname(ds_name)) == NULL) {
		printf("%s: unknown host '%s'\n", argv[0], ds_name);
		exit(1);
	}
	bzero((char *) &ds_addr, sizeof(ds_addr));
	ds_addr.sin_family = AF_INET;
	bcopy((char *)host->h_addr, 
		(char *)&ds_addr.sin_addr.s_addr,
		host->h_length);
	ds_port = atoi(argv[1]);
	ds_addr.sin_port = ds_port;
	
	/*** Connecting ***/
	if (connect(ds_sock, (struct sockaddr *) &ds_addr, sizeof(ds_addr)) < 0) {
		printf("%s: Unable to connect to %s\n", argv[0], ds_name);
		perror("Aborting");
		exit(1);
	} else {
		printf("Connection to server %s established\n", ds_name);
	}
	
	/*** Print ip and port ***/
	struct sockaddr_in this_addr;
	socklen_t this_len = sizeof(this_addr);
	getsockname(ds_sock, (struct sockaddr*)&this_addr, &this_len);
	char this_ip[16];
	inet_ntop(this_addr.sin_family, &this_addr.sin_addr.s_addr, this_ip, 16);
	
	struct sockaddr_in that_addr;
	socklen_t that_len = sizeof(that_addr);
	getsockname(sockfd, (struct sockaddr*)&that_addr, &that_len);
	int this_port = that_addr.sin_port;
	
	//int that_port = this_addr.sin_port;
	//printf("%d %d\n", this_port, that_port);
	
	printf("%s %d\n", this_ip, this_port);
	
	/*** Send register message ***/
	char msg[BUFSZ];
	sprintf(msg, "register %s %d\r\n", this_ip, this_port);
	send(ds_sock, &msg, sizeof(msg), 0);
	
	/*** Receive response and print ***/
	char res[BUFSZ];
	recv(ds_sock, &res, sizeof(res), MSG_WAITALL);
	printf("%s", res);
	/******************************************************************/
	
	/*** Listen for requests ***/
	if (listen(sockfd, BACKLOG) < 0) {
		perror("listen");
		exit(1);
	}
	clilen = sizeof(cli_addr);
	
	/*** The main accept loop ***/
	while(1) {
		/*** Accept connection ***/
		puts("Waiting for connection...");
		if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) < 0) {
			perror("accept");
			exit(1);
		}
		
		/*** Start to receive data ***/
		int done = 0;
		int n_msg = 0;
		while (!done) {
			char data[BUFSZ];
			recv(newsockfd, &data, sizeof(data), MSG_WAITALL);
			
			int rc1 = strcmp(data, "eos");
			int rc2 = strcmp(data, "fin");
			if (rc1 == 0) { // which means that one session of data upload has ended
				char ack[BUFSZ] = "ack";
				send(newsockfd, &ack, sizeof(ack), 0);
				
				printf("Received %dKB\n", n_msg/(1024/BUFSZ));
				n_msg = 0; // reset the count for another data
			} else if (rc2 == 0) { // which means all upload has finished
				done = 1;
				puts("Communication with client has ended, shutting down connection...");
			} else {
				n_msg++;
			}
		}
		
		/*** Close connection ***/
		close(newsockfd);
	
	}
	
	return 0;
}

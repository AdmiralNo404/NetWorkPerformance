#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#define BUFSZ 256

int main(int argc, char** argv) {
	int sock;
	struct sockaddr_in server;
	struct hostent *host;
	char buffer[BUFSZ];
	
	/*** Check input format ***/
	if (argc < 3) {
		printf("Usage: %s <server-ip> <port>\n", argv[0]);
		exit(1);
	}
	char *hostname = argv[1];
	int portno = atoi(argv[2]);
	
	/*** Create socket ***/
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("%s: Unable to open socket\n",argv[0]);
		perror("Aborting");
		exit(1);
	}
	
	/*** Fill out sockaddr struct ***/
	if ((host = gethostbyname(hostname)) == NULL) {
		printf("%s: unknown host '%s'\n", argv[0], hostname);
		exit(1);
	}
	bzero((char *) &server, sizeof(server));
	server.sin_family = AF_INET;
	bcopy((char *)host->h_addr, 
		(char *)&server.sin_addr.s_addr,
		host->h_length);
	server.sin_port = portno;
	
	/*** Connecting ***/
	if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
		printf("%s: Unable to connect to %s\n", argv[0], hostname);
		perror("Aborting");
		exit(1);
	} else {
		printf("Connection to server %s established\n", hostname);
	}
	
	/*** Send message ***/
	if (send(sock, "Hello server!", 14, 0) < 0) {
		printf("%s: Error writing on stream socket", argv[0]);
		perror("Aborting");
		close(sock);
		exit(1);
	}
	
	/*** Receive reply ***/
	if ((recv(sock, buffer, BUFSZ, 0)) < 0) {
		perror("recv");
		exit(1);
	}
	
	printf("The server said: %s\n", buffer);
	
	/*** Close connection ***/
	close(sock);
	
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

int main(int argc, char** argv) {
	int sock;
	int serverlen;
	struct sockaddr_in server;
	struct hostent *host;
	
	/*** Check input format ***/
	if (argc < 3) {
		printf("Usage: %s <server-ip> <port>\n", argv[0]);
		exit(1);
	}
	char *hostname = argv[1];
	int portno = atoi(argv[2]);
	
	/*** Create socket ***/
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("%s: Unable to open socket\n",argv[0]);
		perror("Aborting");
		exit(1);
	}
	
	/*** Fill out sockaddr struct ***/
	if ((host = gethostbyname(hostname)) == NULL) {
		printf("%s: unknown host '%s'\n", argv[0], argv[1]);
		exit(1);
	}
	bzero((char *) &server, sizeof(server));
	server.sin_family = AF_INET;
	bcopy((char *)host->h_addr, 
		(char *)&server.sin_addr.s_addr,
		host->h_length);
	server.sin_port = portno;
	
	int n = 0;
	serverlen = sizeof(server);
	while (1) {
		printf("enter a number: ");
		int i;
		scanf("%d", &i);
		struct timeval begin, end;
		gettimeofday(&begin, NULL);
		sendto(sock, &i, sizeof(i), 0, (struct sockaddr *) &server, serverlen);
		int r;
		recvfrom(sock, &r, sizeof(r), 0, (struct sockaddr *) &server, &serverlen);
		gettimeofday(&end, NULL);
		long int diff = (long int) (end.tv_sec - begin.tv_sec);
		long int diff_u = (long int) (end.tv_usec - begin.tv_usec);
		long int true_diff = diff * 1000000 + diff_u;
		printf("echo: %d\n", r);
		printf("RTT = %ld microseconds\n", true_diff);
		n++;
	}
	
	close(sock);
	
}

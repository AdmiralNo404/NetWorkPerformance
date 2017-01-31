#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "../message.h"

int fileRecv(char* filename, int sock, struct sockaddr_in *ser, int serlen, struct msmt *m) {
	FILE* myfd = fopen(filename, "w+");
	struct timeval begin, end;
	long int timediff = 0, timediff_u = 0;
	int no_msg = 0, no_byte = 0;
	int retval;
	int finished = 0;
	while (!finished) {
		struct msg_t response;
		// sockaddr struct to get the sender address
		struct sockaddr_in sender;
		int senderlen;
		
		if (gettimeofday(&begin, NULL) < 0) {
			perror("gettimeofday");
			exit(1);
		}
		recvfrom(sock, &response, sizeof(response), 0, (struct sockaddr *) &sender, &senderlen);
		if (gettimeofday(&end, NULL) < 0) {
			perror("gettimeofday");
			exit(1);
		}
		
		long int diff = (long int) (end.tv_sec - begin.tv_sec);
		long int diff_u = (long int) (end.tv_usec - begin.tv_usec);
		long int true_diff = diff * 1000000 + diff_u;
		timediff += true_diff;
		no_msg++;
		no_byte += response.payload_len;
		
		printMsg(&response, 1);
		
		switch (response.msg_type) {
			case 2: {  // massage type is get_err
				printf("File %s not found on server!\n", filename);
				fclose(myfd);
				remove(filename);
				retval = -1;
				finished = 1;
			} case 3: {  // message type is get_resp
				int r = fwrite(response.payload, sizeof(char), response.payload_len, myfd);
				if (r < 0) {
					perror("fwrite");
					exit(1);
				}
				/*** Construct acknowledge message ***/
				struct msg_t ack;
				initMsg(&ack, 4, response.cur_seq, response.max_seq, 0, "");
				r = sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr *) ser, serlen);
				if (r < 0) {
					perror("sendto");
					exit(1);
				}
				// Check if it's the end of file			
				if (response.cur_seq == response.max_seq) {
					printf("File download successful!\n");
					fclose(myfd);
					retval = 0;
					finished = 1;
				}
				break;
			} default: {
				printf("Unrecognized message type!\n");
				retval = -1;
				finished = 1;
			}
		}
	}
	
	if (retval == 0) {
		m->timediff = timediff;
		m->no_msg = no_msg;
		m->no_byte = no_byte;
	}
	return retval;
}

int main(int argc, char** argv) {
	int sock, portno;
	int serverlen;
	struct sockaddr_in server;
	struct hostent *host;
	
	/*** Check input format ***/
	if (argc < 4) {
		printf("Usage: %s <server-ip> <port> <filename>\n", argv[0]);
		exit(1);
	}
	
	/*** Create socket ***/
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("%s: Unable to open socket\n",argv[0]);
		perror("Aborting");
		exit(1);
	}
	
	/*** Fill out sockaddr struct ***/
	if ((host = gethostbyname(argv[1])) == NULL) {
		printf("%s: unknown host '%s'\n", argv[0], argv[1]);
		exit(1);
	}
	bzero((char *) &server, sizeof(server));
	server.sin_family = AF_INET;
	bcopy((char *)host->h_addr, 
		(char *)&server.sin_addr.s_addr,
		host->h_length);
	portno = atoi(argv[2]);
	server.sin_port = portno;
	
	/*** Construct request message ***/
	struct msg_t request;
	initMsg(&request, 1, 0, 1, strlen(argv[3]), argv[3]);
	
	/*** Send the message ***/
	serverlen = sizeof(server);
	sendto(sock, &request, sizeof(request), 0, (struct sockaddr *) &server, serverlen);
	
	/*** Receive the file ***/
	struct msmt m;
	fileRecv(argv[3], sock, &server, serverlen, &m);
	printMsmt(&m);
	
	/*** Notify the end of session ***/
	struct msg_t finish;
	initMsg(&finish, 5, 0, 1, 0, "");
	int r = sendto(sock, &finish, sizeof(finish), 0, (struct sockaddr *) &server, serverlen);
	if (r < 0) {
		perror("sendto");
		exit(1);
	}
	
	/*** End the connection ***/
	close(sock);
	
	return 1;
}

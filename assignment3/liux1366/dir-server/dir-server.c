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

struct entry {
	char ip[16];
	int port;
	struct entry* next;
};

void sendList(int sock, struct entry* p) {
	struct entry* cur = p;
	struct entry* next = cur->next;
	if (next != NULL) {
		char succ[BUFSZ] = "success\r";
		send(sock, succ, sizeof(succ), 0);
		while (next != NULL) {
			char msg[BUFSZ];
			sprintf(msg, "%s %d\r", cur->ip, cur->port);
			send(sock, msg, sizeof(msg), 0);
			cur = next;
			next = cur->next;
		}
		char fin[] = "\r\n";
		send(sock, fin, sizeof(fin), 0);
	} else {
		char fail[] = "failure\r\n";
		send(sock, fail, sizeof(fail), 0);
	}
}

int ifDup(struct entry* head, struct entry* new) {
	struct entry* cur = head;
	struct entry* next = cur->next;
	if (next == NULL) {
		return 0;
	} else {
		while (next != NULL) {
			int rc = strcmp(cur->ip, new->ip);
			if (rc == 0) {
				cur->port = new->port;
				return 1;
			} else {
				cur = next;
				next = cur->next;
			}
		}
		return 0;
	}
}

int main(int argc, char** argv) {
	int sockfd, newsockfd, portno;
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
	
	/*** Psudo DB to record register info (just a list) ***/
	struct entry* head = malloc(sizeof(struct entry));
	head->next = NULL;
	
	/*** The main accept loop ***/
	while(1) {		
		/*** Accept connection ***/
		if ((newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen)) < 0) {
			perror("accept");
			exit(1);
		}
		
		/*** Recieve message ***/
		int finished = 0;
		while(!finished) {
			char buffer[BUFSZ] = {};
			if ((recv(newsockfd, &buffer, sizeof(buffer), MSG_WAITALL)) < 0) {
				perror("recv");
				exit(1);
			}
			
			/*** check end of message ***/
			char* p = strstr(buffer, "\r\n");
			if (p != NULL) {
				finished = 1;
			}
			
			/*** Parse the message ***/
			char* msg = strtok(buffer, "\r\n");
			char* token = strtok(msg, "\r");
			while (token) {
				puts(token);
				int rc = strcmp(token, "list-servers");
				if (rc == 0) {
					sendList(newsockfd, head);
				} else {
					char type[16];
					char ip[16];
					int port;
					sscanf(token, "%s %s %d", &type, &ip, &port);
					//printf("%s %d\n", ip, port);
					int rc = strcmp(type, "register");
					if (rc == 0) {
						struct entry* new = malloc(sizeof(struct entry));
						strcpy(new->ip, ip);
						new->port = port;
						if (ifDup(head, new)) {
							char succ[] = "success\r\n";
							send(newsockfd, succ, sizeof(succ), 0);
						} else {
							new->next = head;
							head = new;
							char succ[] = "success\r\n";
							send(newsockfd, succ, sizeof(succ), 0);
						}
					} else {
						char fail[] = "failure\r\n";
						send(newsockfd, fail, sizeof(fail), 0);
					}
				}
				token = strtok(NULL, "\r");
			}
			
		}
		
		/*** Close connection ***/
		close(newsockfd);
	
	}
	
	close(sockfd);
	
	return 0;
}

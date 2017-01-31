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

struct entry {
	char ip[16];
	int port;
	struct entry* next;
};

int printList(struct entry* head) {
	struct entry* cur = head;
	struct entry* next = cur->next;
	int c = 0;
	while (next != NULL) {
		printf("%s %d\n", cur->ip, cur->port);
		cur = next;
		next = cur->next;
		c++;
	}
	return c;
}

void rand_str(char *array, int n) {
	int i;
	for (i = 0; i < n; i++) {
		int randomChar = rand()%(26+26+10);
		if (randomChar < 26)
			array[i] = 'a' + randomChar;
		else if (randomChar < 26+26)
			array[i] = 'A' + randomChar - 26;
		else
			array[i] = '0' + randomChar - 26 - 26;
	}
	array[n] = 0;
}

int getEntry(struct entry* head, int i, struct entry* carry) {
	struct entry* cur = head;
	struct entry* next = cur->next;
	while ((i > 0) && (next != NULL)) {
		cur = next;
		next = cur->next;
		i--;
	}
	if (i == 0) {
		strcpy(carry->ip, cur->ip);
		carry->port = cur->port;
		return 0;
	} else {
		return -1;
	}
}

int main(int argc, char** argv) {
	int sock, ds_port, db_port;
	struct sockaddr_in ds, db;
	struct hostent *host;
	char ds_name[] = "apollo.cselabs.umn.edu";
	//char ds_name[] = "atlas.cselabs.umn.edu";
	char db_name[] = "atlas.cselabs.umn.edu";
	
	/*** Check input format ***/
	if (argc < 3) {
		printf("Usage: %s <ds-port> <db-port>\n", argv[0]);
		exit(1);
	}
	
	/************* Part 1: Connecting to the dir-server ***************/
	/*** Create socket ***/
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("%s: Unable to open socket\n",argv[0]);
		perror("Aborting");
		exit(1);
	}
	
	/*** Fill out sockaddr struct ***/
	if ((host = gethostbyname(ds_name)) == NULL) {
		printf("%s: unknown host '%s'\n", argv[0], ds_name);
		exit(1);
	}
	bzero((char *) &ds, sizeof(ds));
	ds.sin_family = AF_INET;
	bcopy((char *)host->h_addr, 
		(char *)&ds.sin_addr.s_addr,
		host->h_length);
	ds_port = atoi(argv[1]);
	ds.sin_port = ds_port;
	
	/*** Connecting ***/
	if (connect(sock, (struct sockaddr *) &ds, sizeof(ds)) < 0) {
		printf("%s: Unable to connect to %s\n", argv[0], ds_name);
		perror("Aborting");
		exit(1);
	} else {
		printf("Connection to server %s established\n", ds_name);
	}
	
	/*** Get client ip address (for later parts) ***/
	struct sockaddr_in this_addr;
	socklen_t this_len = sizeof(this_addr);
	getsockname(sock, (struct sockaddr*)&this_addr, &this_len);
	char this_ip[16];
	inet_ntop(this_addr.sin_family, &this_addr.sin_addr.s_addr, this_ip, 16);
	printf("Client ip: %s\n", this_ip);
	
	/*** Send the request ***/
	char msg[BUFSZ] = "list-servers\r\n";
	if (send(sock, &msg, sizeof(msg), 0) < 0) {
		printf("%s: Error writing on stream socket", argv[0]);
		perror("Aborting");
		close(sock);
		exit(1);
	}
	
	/*** Psudo DB to record register info (just a list) ***/
	struct entry* head = malloc(sizeof(struct entry));
	head->next = NULL;
	
	/*** Recieve message ***/
	int finished = 0;
	while(!finished) {
		char buffer[BUFSZ];
		bzero((char *) &buffer, sizeof(buffer));
		if ((recv(sock, &buffer, sizeof(buffer), MSG_WAITALL)) < 0) {
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
			int rc = strcmp(token, "failure");
			if (rc == 0) {
				puts("No available server found!");
				exit(0);
			}
			rc = strcmp(token, "success");
			if (rc != 0) {
				char ip[16];
				int port;
				sscanf(token, "%s %d", &ip, &port);
				struct entry* new = malloc(sizeof(struct entry));
				strcpy(new->ip, ip);
				new->port = port;
				new->next = head;
				head = new;
			}
			token = strtok(NULL, "\r");
		}
	}
	
	/*** End the connection ***/
	close(sock);

	/**************** Part 2: Connecting to app-server ****************/
	/*** Print out all available servers in a form ***/
	puts("\nAvailable Servers:");
	int max = printList(head);
	puts("");
	
	int x = 0;
	while (x < max) {
		struct entry choose;
		getEntry(head, x, &choose);
		char server_ip[16];
		strcpy(server_ip, choose.ip);
		int server_port = choose.port;
		printf("Connecting %s at port %d...\n", server_ip, server_port);
		
		int sock2;
		struct hostent *host2;
		/*** Create socket ***/
		if ((sock2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			printf("%s: Unable to open socket\n",argv[0]);
			perror("Aborting");
			exit(1);
		}
		
		/*** Fill out sockaddr struct ***/
		struct sockaddr_in server;
		if ((host2 = gethostbyname(server_ip)) == NULL) {
			printf("%s: unknown host '%s'\n", argv[0], server_ip);
			exit(1);
		}
		bzero((char *) &server, sizeof(server));
		server.sin_family = AF_INET;
		bcopy((char *)host2->h_addr, 
			(char *)&server.sin_addr.s_addr,
			host2->h_length);
		server.sin_port = server_port;
		
		/*** Connecting ***/
		if (connect(sock2, (struct sockaddr *) &server, sizeof(server)) < 0) {
			printf("%s: Unable to connect to %s\n", argv[0], server_ip);
			perror("Aborting");
			exit(1);
		} else {
			printf("Connection to server %s established\n", server_ip);
		}
		
		/*** Uploading some data ***/
		/*** Inform the server how many times the upload will be performed ***/
		/*** As well as how many send for each repeat ***/
		int s = 10; // KB
		int e = 4; // data size increment times
		while (e > 0) {
			int t = 5; // repeat the upload for 5 times
			int n = 1024 / BUFSZ; // 1KB / BUFSZ to determine how many loops in order to send 1 KB
			int l = s * n; // total number of loops
			
			// Accumulating total time
			long int acc = 0;
			int c = 0;
			while (c < t) {
				/*** Start to upload ***/
				int i = 0;
				struct timeval begin, end;
				long int timediff = 0;
				while (i < l) {
					char data[BUFSZ];
					// generate random char array
					rand_str(data, BUFSZ);
					gettimeofday(&begin, NULL);
					send(sock2, data, sizeof(data), 0);
					gettimeofday(&end, NULL);
					
					long int diff = (long int) (end.tv_sec - begin.tv_sec);
					long int diff_u = (long int) (end.tv_usec - begin.tv_usec);
					long int true_diff = diff * 1000000 + diff_u;
					timediff += true_diff;
					
					i++;
				}
				
				/*** Indicate END of upload ***/
				/*** Wait for ACK from server ***/
				char eos[BUFSZ] = "eos"; // end of session
				char ack[BUFSZ];
				gettimeofday(&begin, NULL);
				send(sock2, &eos, sizeof(eos), 0);
				recv(sock2, &ack, sizeof(ack), MSG_WAITALL);
				gettimeofday(&end, NULL);
				
				long int diff = (long int) (end.tv_sec - begin.tv_sec);
				long int diff_u = (long int) (end.tv_usec - begin.tv_usec);
				long int true_diff = diff * 1000000 + diff_u;
				timediff += true_diff;
				
				int rc = strcmp(ack, "ack");
				if (rc != 0) {
					puts("Failed to receive ACK from server!");
					exit(0);
				}
				
				acc += timediff;
				c++;
			}
						
			/*** Compute the average time ***/
			long int avg = acc / t;
			printf("Average time taken to upload %dKB data to %s is: %ld microseconds\n", s, server_ip, avg);
			
			/****************** Connect to db-server ******************/
			int db_sock;
			struct hostent *db_host;
			/*** Create socket ***/
			if ((db_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				printf("%s: Unable to open socket\n",argv[0]);
				perror("Aborting");
				exit(1);
			}
			
			/*** Fill out sockaddr struct ***/
			if ((db_host = gethostbyname(db_name)) == NULL) {
				printf("%s: unknown host '%s'\n", argv[0], db_name);
				exit(1);
			}
			bzero((char *) &db, sizeof(db));
			db.sin_family = AF_INET;
			bcopy((char *)db_host->h_addr, 
				(char *)&db.sin_addr.s_addr,
				db_host->h_length);
			db_port = atoi(argv[2]);
			db.sin_port = htons(db_port);
			
			/*** Connecting ***/
			if (connect(db_sock, (struct sockaddr *) &db, sizeof(db)) < 0) {
				printf("%s: Unable to connect to %s at %d\n", argv[0], db_name, db_port);
				perror("Aborting");
				exit(1);
			} else {
				printf("Connection to server %s established\n", db_name);
			}
			
			/*** Send performance data to db-server ***/
			char perf[BUFSZ];
			sprintf(perf, "set-record %s %s %d %dKB %ld\r\n", this_ip, server_ip, server_port, s, avg);
			send(db_sock, &perf, sizeof(perf), 0);
			
			/*** Receive response ***/
			char db_res[BUFSZ];
			recv(db_sock, &db_res, sizeof(db_res), 0);
			char* token = strtok(db_res, "\r\n");
			puts(token);
			
			/*** Close connection to db-server ***/
			shutdown(db_sock, SHUT_RDWR);
			
			s *= 10;
			e--;
		}
		
		/*** Notify the server that the client is finished ***/
		char fin[BUFSZ] = "fin";
		send(sock2, &fin, sizeof(fin), 0);
		/*** End the connection ***/
		close(sock2);
		
		x++; // next server
	}
	
	/****************** Part 3: Connect to db-server ******************/
	int db_sock;
	struct hostent *db_host;
	/*** Create socket ***/
	if ((db_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("%s: Unable to open socket\n",argv[0]);
		perror("Aborting");
		exit(1);
	}
	
	/*** Fill out sockaddr struct ***/
	if ((db_host = gethostbyname(db_name)) == NULL) {
		printf("%s: unknown host '%s'\n", argv[0], db_name);
		exit(1);
	}
	bzero((char *) &db, sizeof(db));
	db.sin_family = AF_INET;
	bcopy((char *)db_host->h_addr, 
		(char *)&db.sin_addr.s_addr,
		db_host->h_length);
	db_port = atoi(argv[2]);
	db.sin_port = htons(db_port);
	
	/*** Connecting ***/
	if (connect(db_sock, (struct sockaddr *) &db, sizeof(db)) < 0) {
		printf("%s: Unable to connect to %s at %d\n", argv[0], db_name, db_port);
		perror("Aborting");
		exit(1);
	} else {
		printf("Connection to server %s established\n", db_name);
	}
	
	/*** Get performance record from db-server ***/
	char get[] = "get-records\r\n";
	send(db_sock, &get, sizeof(get), 0);
	
	/*** Receive response ***/
	finished = 0;
	while(!finished) {
		char buffer[BUFSZ+1];
		bzero((char *) &buffer, sizeof(buffer));
		if ((recv(db_sock, &buffer, sizeof(char)*(BUFSZ), 0)) < 0) {
			perror("recv");
			exit(1);
		}
		buffer[BUFSZ] = '\0';
		
		/*** check end of message ***/
		char* p = strstr(buffer, "\n");
		if (p != NULL) {
			// the end of message
			finished = 1;
		}
		
		/*** Parse the message ***/
		int idx;
		for (idx = 0; idx < BUFSZ; idx++) {
			if (buffer[idx] == '\r') {
				buffer[idx] = '\n';
			}
		}
		printf("%s", buffer);
		/***/
	}
	puts("");
	
	/*** Close connection to db-server ***/
	shutdown(db_sock, SHUT_RDWR);
	
	return 1;
}

all: tcp udp
tcp: tcp-server tcp-client
tcp-server: server_tcp.c
	gcc -g -o server_tcp server_tcp.c
tcp-client: client_tcp.c
	gcc -g -o client_tcp client_tcp.c
udp: udp-server udp-client
udp-server: server_udp.c
	gcc -g -o server_udp server_udp.c
udp-client: client_udp.c
	gcc -g -o client_udp client_udp.c
clean:
	rm -f server_tcp client_tcp server_udp client_udp

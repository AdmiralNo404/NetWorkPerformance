clients: tcp-client udp-client
tcp-client: client_tcp.c ../message.h
	gcc -o client_tcp client_tcp.c
udp-client: client_udp.c ../message.h
	gcc -o client_udp client_udp.c

clean:
	rm -f client_tcp client_udp *.txt

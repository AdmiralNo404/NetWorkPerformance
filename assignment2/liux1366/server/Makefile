servers: tcp-server udp-server
tcp-server: server_tcp.c ../message.h
	gcc -o server_tcp server_tcp.c
udp-server: server_udp.c ../message.h
	gcc -o server_udp server_udp.c

clean:
	rm -f server_tcp server_udp
clean-all: clean
	rm -f *.txt

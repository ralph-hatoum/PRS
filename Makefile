make: server 

server: server-6.c
	gcc -pthread server-6.c -o server


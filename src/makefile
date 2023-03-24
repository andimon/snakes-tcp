all: server client

server: snake-server.c helper.c
	gcc -o server snake-server.c helper.c -lncurses -lpthread

client: snake-client.c helper.c
	gcc -o client snake-client.c helper.c -lncurses -lpthread

clean: 
	rm server client

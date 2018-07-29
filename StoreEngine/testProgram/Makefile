all: server client
server: server.c 
	gcc $^ -o $@ -g -pthread
client: client.c 
	gcc $^ -o $@ -g
.PHONY:clean
clean:
	rm server client

server: server.o
	./server.o

client: client.o
	./client.o localhost "default message"

server.o: server.c
	gcc server.c -o server.o

client.o:
	gcc client.c -o client.o
clean: 
	rm -rf *.o
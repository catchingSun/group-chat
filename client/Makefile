client : client.o
	gcc  client.o -o client -lpthread -g
client.o : client.c chat.h
	gcc -Wall -O -g -c client.c -o client.o 
clean :
	rm -f *.o

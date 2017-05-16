server : server.o s_functions.o 
	gcc -o server server.o s_functions.o -lpthread -g
server.o : server.c server.h list.h
	gcc -g -c server.c 
s_functions.o : s_functions.c server.h
	gcc -g -c s_functions.c
clean :
	rm -f *.o

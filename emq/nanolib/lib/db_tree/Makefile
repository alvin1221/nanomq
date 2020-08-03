CC = gcc
CFLAGS = -Wall -g
DEPS = mqtt_db.h zmalloc.h hash.h
OBJ = main.o mqtt_db.o zmalloc.o hash.o


%.o:%.c hash.cc $(DEPS) 
	$(CC) -c -o $@ $< $(CFLAGS) 
	g++ -c -o hash.o hash.cc


nanolib:$(OBJ) 
	$(CC) -o nanolib $(OBJ) -lstdc++
	rm -f $(OBJ) 
	./nanolib



.PHONY:clean
clean:
	rm -f $(OBJ) nanolib

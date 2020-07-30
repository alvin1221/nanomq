CC = gcc
CFLAGS = -Wall
DEPS = mqtt_db.h zmalloc.h
OBJ =  main.o mqtt_db.o zmalloc.o

	
%.o:%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

nanolib:$(OBJ) 
	$(CC) -o nanolib $(OBJ) 
	rm -f $(OBJ) 
	./nanolib 


.PHONY:clean
clean:
	rm -f $(OBJ) nanolib

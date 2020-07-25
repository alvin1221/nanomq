CC = gcc
CFLAGS = -Wall
DEPS = mqtt_db.h
OBJ =  main.o mqtt_db.o 

	
%.o:%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

nanomqlib:$(OBJ) 
	$(CC) -o nanolib $(OBJ) 
	rm -f $(OBJ) 

.PHONY:clean
clean:
	rm -f $(OBJ) nanolib

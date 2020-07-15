objects= main.o 
demo:$(objects)
	gcc -o demo $(objects)
	rm -f *.o demo
main.o:main.c handle_connect.c
	gcc -c -o main.o main.c
handle_connect.o:handle_connect.c mqtt_protocol.h  mosquitto_base.h send_connack.c 
	gcc -c -o handle_connect.o handle_connect.c
send_connack.o:send_connack.c mqtt_protocol.h  mosquitto_base.h 
	gcc -c -o send_connack.o send_connack.c
mosquitto_property.o:mosquitto_property.c
	gcc -c -o mosquitto_property.o mosuqitto_property.c 
mosquitto_base.o:mosquitto_base.c mosquitto_base.h
	gcc -c -o mosquitto_base.o mosquitto_base.c
	
.PHONY:clean
clean:
	rm -f *.o demo

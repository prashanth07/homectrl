##default c flags used

CFLAGS	=	-Wall -g


serial.o : serial.c
	gcc $(CFLAGS) -c serial.c

######################################################################################
##Make Section for the adapter
######################################################################################

adapter : adapter.o serial.o
	gcc $(CFLAGS) adapter.o serial.o -o adapter

adapter.o : adapter.c serial.h common.h
	gcc $(CFLAGS) -c adapter.c



######################################################################################
##Make Section for the arm_server
######################################################################################

arm_server : arm_server.o arm_serial.o arm_scheduler.o
	arm-linux-gcc $(CFLAGS) server.o arm_serial.o scheduler.o -o arm_server
	rm server.o scheduler.o

arm_server.o : server.c serial.h common.h
	arm-linux-gcc $(CFLAGS) -c server.c

arm_serial.o : arm_serial.c
	arm-linux-gcc $(CFLAGS) -c arm_serial.c

arm_scheduler.o : scheduler.c scheduler.h
	arm-linux-gcc $(CFLAGS) -c scheduler.c

######################################################################################
##Make Section for the x86_server
######################################################################################

x86_server : server.o serial.o scheduler.o
	gcc $(CFLAGS) server.o serial.o scheduler.o -o x86_server
	rm server.o scheduler.o

server.o : server.c serial.h common.h
	gcc $(CFLAGS) -c server.c

scheduler.o : scheduler.c scheduler.h
	gcc $(CFLAGS) -c scheduler.c

######################################################################################
##Make Section for the client
######################################################################################

client : client.o
	gcc $(CFLAGS) client.o -o client

client.o: client.c common.h
	gcc -c $(CFLAGS) client.c



######################################################################################
##Make Section for clean
######################################################################################

clean:
	rm *.o adapter arm_server x86_server client

######################################################################################
##Make Section for all
######################################################################################
all:
	make client adapter x86_server

######################################################################################
##Make Section for install 	//don't know is this the right way!!!!
######################################################################################

install:
	cp client /usr/bin/
	cp adapter /usr/bin/
	cp x86_server /usr/bin/

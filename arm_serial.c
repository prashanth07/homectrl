/* arm_serial.c all the functions related to  serial port*/
/* Serial-Programming Howto by Gary Frerking, used as a refference */

#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <termios.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BAUDRATE B19200
#define MODEMDEVICE "/dev/ttyAM1"

#include "common.h"

#define TRUE 1
#define FALSE 0

//Maximum size of confirms that we may get
#define MAX_CONFIRM 6

#define STX 0x02
#define ETX 0x03

/*
 *Initializes the serial port as per required by irms3
 *returns the descriptor for the device
 */
int init_serial ()
{
	int fd;
	struct termios newtio;
	
	/*open the modem device*/
	fd = open (MODEMDEVICE, O_RDWR | O_NOCTTY);
	if(fd < 0){
		fprintf (stderr,"Cannot open device %s",MODEMDEVICE);
		exit(-1);
	}
	
	/*
	 *19200 baud
	 *8 data bits
	 *odd parity
	 *1 stop bit
	 *handshaking using the Clear To Send (CTS) and Request To Send (RTS) lines.
	 */
	bzero(&newtio,sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | PARENB | PARODD | CREAD;
	newtio.c_iflag = 0;
	newtio.c_oflag = 0;
	
	/*Set input mode to noncanonical and no echo*/
	newtio.c_lflag=0;
	
	newtio.c_cc[VTIME] = 0;
	newtio.c_cc[VMIN]  = 1;
	
	tcflush(fd,TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);
	
	return fd;
}

int send_data (int fd, char *frame, int n)
{
	
	
	//write frame or Command
	if (write (fd, frame, n) != n) {
		fprintf (stderr,"write() failed");
		exit (1);
        }
	
	
	/*
	 *Serial port on arm board does not support handshaking using RTS and CTS
	 *IRMS3 needs RTS and CTS signals for flow control
	 *we figured out, even in this senario data transmission is possible but only one way
	 *so here we are not reading acknowledgements.
	 */
	return 0;
	
}


/*
//test code
int main (int argc, char *argv[])
{
	int fd;
	char frame[20];
	
	fd = init_serial ();
	
	frame[0] = 0x02;
	strcpy (&frame[1],"0000");
	frame[5] = 0x03;
	send_data (fd, frame,6);
	
	while (1){
		frame[0]=0x02;
		strcpy (&frame[1],"06070F001234015678");
		frame[19]=0x03;
		
		send_data (fd, frame,20);
		sleep(10);
	}
	return 0;
}*/

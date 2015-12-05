/* serial.c all the functions related to  serial port*/
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
#define MODEMDEVICE "/dev/ttyS0"

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
	/*for the confirms from irms3*/
	char buf[2*MAX_CONFIRM];
	int res, stx_pos ,etx_pos;
	int i;
	
	//write frame or Command
	if (write (fd, frame, n) != n) {
		fprintf (stderr,"write() failed");
		exit (1);
        }
	
	
	//read the confirm
	/*
	 *stx_pos is the index for 0x2 and etx_pos for 0x3
	 */
	res = 0;
	etx_pos=0;
	while ( etx_pos == 0 ) {
		i=res;
		res = res + read (fd, &buf[res], 1);
		
		for (; i<res ; i++)
			if (buf[i] == STX)
				stx_pos = i;
			else if (buf[i] == ETX )
				etx_pos = i;
	}
	
	
	
	//analyze the confirm
	if (buf[stx_pos + 1] == '8') {
		switch (buf[stx_pos + 2]) {
			
			case '0':	//OK everything's fine
				break;
				
			case '1':	//NOT_OK
				fprintf (stderr, "WARNING:confirm:NOT_OK info_byte: %c%c",
					 buf[stx_pos+3],buf[stx_pos+4]);
				break;
				
			case '2':	//DONT_KNOW
				fprintf (stderr, "WARNING:confirm:DONT_KNOW info_byte: %c%c",
					buf[stx_pos+3],buf[stx_pos+4]);
				break;
				
			case '3':	
				fprintf (stderr, "WARNING:confirm:DISRUPTED info_byte: %c%c",
					buf[stx_pos+3],buf[stx_pos+4]);
				break;
				
			case '4':	
				fprintf (stderr, "WARNING:confirm:NOT_STARTED info_byte: %c%c",
					 buf[stx_pos+3],buf[stx_pos+4]);
				break;
				
			case '5':
				fprintf (stderr, "WARNING:confirm:CMD_ERROR info_byte: %c%c",
					 buf[stx_pos+3],buf[stx_pos+4]);
				break;	
				
			case '6':
				fprintf (stderr, "WARNING:confirm:CMD_OVERRUN info_byte: %c%c",
					 buf[stx_pos+3],buf[stx_pos+4]);
				break;
				
			case '8':	
				fprintf (stderr, "WARNING:confirm:MS_IDLE info_byte: %c%c",
					 buf[stx_pos+3],buf[stx_pos+4]);
				break;
				
			case '9':
				fprintf (stderr, "WARNING:confirm:MS_BUSY info_byte: %c%c",
					 buf[stx_pos+3],buf[stx_pos+4]);
				break;
				
			case 'A':	
				fprintf (stderr, "WARNING:confirm:DATA_FOLL info_bytes: %c%c",
					 buf[stx_pos+3],buf[stx_pos+4]);
				break;
				
			default:
				fprintf (stderr,"ok");
				break;
		}
		
	}
	else {	//program should not come here
		fprintf (stderr, "Error Sending data & getting confirm\n"
				"Check the serial port connection\n" );
		exit (1);
	}
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

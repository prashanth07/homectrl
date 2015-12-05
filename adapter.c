/*
 *program to generate a .hct file for a RC6 mode0 remote
 */

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
#include <ctype.h>

#define MAXREAD 100

#include "serial.h"
#include "common.h"

int adapter (int fd);

int main ()
{
	int fd,cnt;

	/*Initialize the serial port*/
	fd = init_serial ();
	
	cnt = adapter (fd);
	printf ("Total number of entries = %d",cnt);
	return 0;
}


int adapter (int fd)
{
	int count=0;
	char name[MAX_LEN];
	char filename[MAX_LEN];
	char frame[MAXREAD];			//ASCII string
	FILE *fp;
	int res,i;
	
	printf ("\nEnter the Device Name : ");
	scanf ("%[^\n]s", filename);
	
	for (i=0 ;filename[i]!='\0';i++)
		filename[i] = tolower(filename[i]);
	strcat (filename,".hct");
	
	if ((fp = fopen(filename,"w")) == NULL) {
		fprintf (stderr , "Cannot open the file %s", filename);
		exit(1);
	}
	
	//init IRMS3 req
	strcpy (frame, "\0020000\003");
	send_data (fd, frame, strlen(frame));
		
	//configure rx-mask
	strcpy (frame, "\0020E0000317F\003");
	send_data (fd, frame, strlen(frame));
	
	while (1) {
		printf ("\nEnter the Key Name : ");
		scanf ("%s", name);
		
		for (i=0 ;name[i] != '\0';i++)
			name[i] = tolower(name[i]);
		/*Typing \stop should break out of the loop*/
		if ( strcmp (name, "\\stop") == 0)
			break;
		
		//flush all the input data that are not read
		tcflush (fd, TCIFLUSH);
SAME_KEY:		
		fprintf (stderr,"\nPress Key %s:",name);
		res = 0;
		//each frame 10 bytes so read atleast 3 frames
		while (res <= 20){
			res = res + read (fd, &frame[res], MAXREAD);
		}
		
		//loop <no of frames read> times
		for (i=1 ; i<res/10 ; i++) {
			if (strncmp((frame+5), (frame+i*10+5), 4)) {
				fprintf (stderr, "Press the same Key......");
				goto SAME_KEY;
			}
		}
		
		fprintf (fp, "%d %s %c%c%c%c\n"
			,count,name,*(frame+5),*(frame+6),*(frame+7),*(frame+8));
		count ++;
		fprintf (stderr,"%s key done\n", name);
		fflush(fp);
	}
	fclose(fp);
	
	return count;
}

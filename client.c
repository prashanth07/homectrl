
/*
 *Client program
 */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>


/*Include all definations that are common across the client and server
 */
#include "common.h"

/*No of seconds to wait for a reply from server*/
#define TIMEOUT 1


void get_options (int argc,char *argv[]);
void print_usage (FILE *stream, int exitcode);

void irms_login (char *);
void irms_logout ();
void init_client ();
void build_frame ();
void schedule_task ();

void disp_dev (char *name);
void cmd_hex (char *dev, char *req_cmd, char *hex_code);
void get_ip ( char ip_add[] );

/*Signal handlers*/
void SIGALRM_handler (int signal_number);
void SIGIO_handler (int signal_number);


const char *program_name=NULL;

char *device = NULL;
char *command = NULL;
char *schdl=NULL;
char *repetation_factor="01";

int sock; //socket

/*server address structure*/
struct sockaddr_in echoServAddr;


/*
 *Main program
 */

int main (int argc,char *argv[])
{
	/*initialize the client*/
	init_client();
	
	/*decode command line arguments*/
	get_options (argc,argv);
	
	if (device != NULL && command != NULL) {
		if(schdl != NULL)
			schedule_task ( );
		else
			build_frame ( );
		
		pause();
	}
	else if (device != NULL) {
		disp_dev (device);
	}
	else {
		print_usage (stdout,0);
	}
	
	return 0;
}


/*
 *READS cammand line variables and sets up corresponding variabler
 */
 
void get_options (int argc, char *argv[])
{
	const char* const short_options = "hd:c:i:or:s:";
	
	static const struct option long_options[] = {
		{"help",	0,	NULL,	'h'},
		{"login",	1,	NULL,	'i'},
		{"logout",	0,	NULL,	'o'},
		{"device",	1,	NULL,	'd'},
		{"command",	1,	NULL,	'c'},
		{"repete",	1,	NULL,	'r'},
		{"schedule",	1,	NULL,	's'},
		{NULL,		0,	NULL,	0}
	};
	int next_option;
	
	program_name = argv[0];
	
	do{
		next_option = getopt_long (argc, argv, short_options, long_options, NULL);
		
		switch (next_option) {
			case 'h' :
				print_usage (stdout, 0);
				break;
			case 'i':
				irms_login(optarg);
				break;
			case 'o':
				irms_logout();
				break;
			case 'r':
				repetation_factor = optarg;
				break;
			case 'd':
				device = optarg;
				break;
			case 'c':
				command = optarg;
				break;
			case 's':
				schdl = optarg;
				break;
			case -1:	//all options are done
					break;
			default:
				exit (-1);
		}
	} while (next_option != -1);
}



/*
 *This Subroutine initializes the client
 *And sets up differnt action for different signals
 *Returns the socket (descriptor) to be used for IO
 */

void init_client ( )
{
	struct sigaction sa_alrm,sa_io;
	
	//Stuff related to siganl SIGALRM
	sa_alrm.sa_handler = SIGALRM_handler;
	if (sigfillset (&sa_alrm.sa_mask) < 0){
		fprintf (stderr,"Error Setting mask");
		exit (-1);
	}
	sa_alrm.sa_flags = 0;
	
	if (sigaction (SIGALRM, &sa_alrm, 0) < 0){
		fprintf (stderr, "sigaction for SIGALRM failed");
		exit(-1);
	}
	//done
	
	//stuff related to SIGIO
	sa_io.sa_handler = SIGIO_handler;
	if (sigfillset (&sa_io.sa_mask) < 0){
		fprintf (stderr,"Error Setting mask");
		exit (-1);
	}
	sa_io.sa_flags = 0;
	
	if (sigaction (SIGIO, &sa_io, 0) < 0){
		fprintf (stderr, "sigaction for SIGALRM failed");
		exit(-1);
	}
	//done
	
	//Create a socket for communication
	sock = socket(PF_INET,SOCK_DGRAM, 0);
	if(sock < 0){
		fprintf (stderr, "socket() failed");
		exit (1);
	}
	
	//Read server IP from configuration file
	char ip[16];
	get_ip (ip);
	
	
	//set up the server address structure
	bzero (&echoServAddr, sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET;
	echoServAddr.sin_addr.s_addr = inet_addr (ip);
	echoServAddr.sin_port = htons (PORT);
		
	//set the socket to recieve SIGNAL IO
	if (fcntl (sock, F_SETOWN, getpid ()) < 0) {
		fprintf (stderr, "Unable to set the socket to recieve SIGIO");
		exit (1);
	}

	if (fcntl(sock, F_SETFL, O_NONBLOCK | FASYNC) < 0){
		fprintf (stderr, "Unable to set the socket to NONBLOCK and ASYNCHRONOUS");
		exit (1);
	}
}



/*To handle the timeout senario*/

void SIGALRM_handler (int signal_number)
{
	fprintf (stderr,"Error:Timeout-No response from Server");
	close(sock);
	exit (1);
}




/*
 *Signal handler to handle the response of the server
 */

void SIGIO_handler (int signal_number)
{
	sigset_t mask_set,old_set;
	unsigned int fromsize;
	unsigned char buf[MAX_BUF_LEN];
	struct sockaddr_in fromAddr;
	
	//mask all signals
	sigfillset (&mask_set);
	if(sigprocmask (SIG_SETMASK, &mask_set, &old_set) < 0) {
		fprintf (stderr,"SIGALRM_handler:Cannot mask the signals");
		exit (1);
	}
	
	fromsize = sizeof(fromAddr);
	recvfrom (sock, buf, MAX_BUF_LEN, 0, (struct sockaddr *)&fromAddr, &fromsize);
	
	if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr){
		fprintf(stderr,"recieved a packet from a unknown server");
		return;
	}
	else {
		switch (buf[0]) {
			case LOGIN_SUCCESS:
				fprintf(stderr,"login successful\n");
				exit(LOGIN_SUCCESS);
				break;
			
			case LOGOUT_SUCCESS: 
				fprintf (stderr, "logout successful\n");
				exit(LOGOUT_SUCCESS);
				break;	
			
			case SUCCESS:
				fprintf (stderr, "Success\n");
				exit(SUCCESS);
				break;
			
			case ERR_ILLEGAL_CMD:
				fprintf (stderr, "Error (Code %d)Illegal Command\n",
					 (unsigned char)buf[0]);
				exit(ERR_ILLEGAL_CMD);
				break;
				
			case ERR_LOGIN_FAILURE:
				fprintf (stderr, "Error (Code %d) Login Failure\n",
					 (unsigned char)buf[0]);
				exit(ERR_LOGIN_FAILURE);
				break;
				
			case ERR_NOT_LOGGED_IN:
				fprintf (stderr, "Error (Code %d) Not logged in\n",
					 (unsigned char)buf[0]);
				exit(ERR_NOT_LOGGED_IN);
				break;
				
			case ERR_CONNECTION_FULL:
				fprintf (stderr, "Error (Code %d) Connection denied All slots full\n"
					 ,(unsigned char)buf[0]);
				exit(ERR_CONNECTION_FULL);
				break;
				
			case ERR_UNKNOWN_PACKET:
 				fprintf (stderr, "Error (Code %d) Server Reply:Unknown packet\n"
						,(unsigned char)buf[0]);
				exit(ERR_UNKNOWN_PACKET);
				break;
				
			case ERR_SCHEDULER:
				fprintf (stderr, "Error (Code %d) Server Reply:Scheduler Error\n"
						,(unsigned char)buf[0]);
				exit(ERR_SCHEDULER);
				break;
				
			default :
				fprintf (stderr,"Unknown Response\n") ;
				exit(1);
				break;
		}
		close(sock);
		exit(0);
	}
	//restore signal mask
	if(sigprocmask(SIG_SETMASK, &old_set, &mask_set) < 0) {
		fprintf (stderr,"SIGALRM_handler:Cannot mask the signals");
		exit (1);
	}
}




/*
 *prints the way the command should be used 
 */
 
void print_usage (FILE *stream, int exitcode)
{
	fprintf (stream,"USAGE: \t%s -i <passwd>\n\t%s -o \n"
			"\t%s -d <device> -c <command> [-r <repetation factor in hex>]"
			"[-s HH:MM:[[:SS]:R]]\n",
			program_name,program_name,program_name);
	exit (exitcode);
}



/*
 *Sends the frame corresponding to the login request
 */

void irms_login(char *passwd)
{
	char frame[256];
	int a;
	frame[0] = LOGIN_BYTE;
	
	//printf ("Enter the password : ");
	strcpy (&frame[2],passwd);
	//scanf ("%s",&frame[2]);
	frame[1] = (unsigned char)strlen (&frame[2]);
	
	if ( (a=sendto (sock, frame, 2+strlen(&frame[2]) ,0 , (struct sockaddr *)&echoServAddr,
	     sizeof(echoServAddr))) != 2+strlen(&frame[2]) ) {
		fprintf (stderr,"Error in sendto, wrote only %d chars errno %d",a,errno);
		exit (1);
	}
	
	alarm (TIMEOUT);
	pause();
}



/*
 *send the frame corresponding to logout request
 */

void irms_logout()
{
	char frame=LOGOUT_BYTE;
	
	if (sendto (sock, &frame, 1 ,0 , (struct sockaddr *)&echoServAddr,
	    sizeof(echoServAddr)) != 1) {
		fprintf (stderr,"Error in sendto errno %d",errno);
		exit (1);
	}
	
	alarm (TIMEOUT);
	pause ( );
}


/*
 *Builds the frame only for the case ,
 *in which both the device and commands are specified
 */

void build_frame ( )
{
	char frame[MAX_BUF_LEN];
	
	frame[0] = COMMAND_BYTE;
	
	//Assumed RC6 standard mode 0
	frame[1] = RC6mode0;

	if (strlen (repetation_factor) == 2){
		frame[2] = toupper (repetation_factor[0]);
		frame[3] = toupper(repetation_factor[1]);
	} else if (strlen (repetation_factor) == 1){
		frame[2] = '0';
		frame[3] = toupper(repetation_factor[0]);
	} else {
		print_usage (stderr,1);
	}
	
	if (((!isdigit(frame[2])) && (frame[2] >= 'F' || frame[2] <='A')) ||
		      ((!isdigit(frame[3])) && (frame[3] >= 'F' || frame[3] <='A')) ) {
		print_usage (stderr, 1);
	}
		      	//bytes 4 thro 7 of frame written by cmd_hex
	cmd_hex (device, command, &frame[4]);

	
	if (sendto (sock, frame, 8 ,0 , (struct sockaddr *)&echoServAddr,
	    sizeof(echoServAddr)) != 8) {
		fprintf (stderr,"Error in sendto");
		exit (1);
	}
	alarm (TIMEOUT);
}


/*
 *Displays the possible commands that can be given for a particular device
 */

void disp_dev (char *name)
{
	int i ;
	FILE *fp;
	char action[10], cmd[5], lowname[20];
	char dev[MAX_LEN];
		
	char *path;
	if ((path = getenv ("HOME"))==NULL){
		fprintf (stderr,"Please set the HOME variable");
		exit(1);
	}
	
	strcpy (dev,path);
	
	strcat (dev,"/.irms/");
	
	//conveting device name to lowercase
	for(i=0;name[i];i++)
		name[i] = tolower(name[i]);
	
	strcat (dev,name);
	
	//converting to filename by adding .hct extension
	strcat(dev, ".hct");
	fprintf (stderr, "%s",dev);
	fp = fopen (dev, "r");
	
	if (!fp) {
		fprintf (stderr, "error in opening\n");
		exit (-1);
	}
	else {
		printf ("\npossible actions for %s are:\n\n", name);
		do {
			if (fscanf(fp, "%d%s%s", &i, action, cmd)==3)
				printf ("%.2d.%s\n",i, action) ;
		} while(!feof(fp));
		fclose(fp);
	}
	return;
}



/*
 *Converts the input strings device name and command name to
 *device address and commands to send to server
 */

void cmd_hex(char req_dev[], char req_cmd[], char hex_code[])
{
	FILE *fp;
	int i;
	char prsnt_act[MAX_LEN], prsnt_code[MAX_LEN];
	char dev[MAX_LEN];
	
	char *path;
	if ((path = getenv ("HOME") ) == NULL) {
		fprintf (stderr,"Please set the HOME variable");
		exit(1);
	}
	
	strcpy (dev,path);
	
	strcat (dev,"/.irms/");
	//conveting device name to lowercase
	for(i=0;req_dev[i];i++)
		req_dev[i] = tolower(req_dev[i]);
	
	strcat (dev,req_dev);
	//converting to filename by adding .hct extension
	strcat(dev, ".hct");
	

	//conveting command to lowercase
	for(i=0;req_cmd[i];i++)
		req_cmd[i] = tolower(req_cmd[i]);
	
	fp = fopen (dev, "r");
	
	if(!fp){
		fprintf (stderr, "error in opening\n");
		exit (-1);
	}
	else{
		int flag=1; 	//1 => command not found
		do {
			if(fscanf (fp, "%d%s%s", &i, prsnt_act, prsnt_code) == 3){
				if (!strcmp (prsnt_act, req_cmd)){
					for(i=0 ; i<4 ; i++) 
						hex_code[i] = prsnt_code[i];
					
					flag=0;
					break;
				}
			}
		} while (!feof(fp));
		
		fclose (fp);
		
		if (flag){
			fprintf (stderr, "requsted command not found\n");
			exit (-1);
		}
	}
}


/*
 *Schedules the task to be executed in future
 *-s HH:MM:SS:R
 *hour:0 to 23
 *minutes: 0 :59
 *repete: no of days
 */
 
void schedule_task ( )
{
	int hour,minutes;
	int repete = 0,seconds=0,len;
	char frame[MAX_BUF_LEN];

	time_t t1,t2=0;
	struct tm *tmp;
	
	if (sscanf(schdl,"%d:%d:%d:%d",&hour,&minutes,&seconds,&repete) < 2)
		print_usage (stderr,-1);
	
	t1 = time(NULL);
	tmp = localtime (&t1);	//generate structure for present time
	
	tmp->tm_sec = seconds;
	tmp->tm_min = minutes;
	tmp->tm_hour = hour;
	
	t1 = mktime (tmp);
	if (repete != -1)
		t2 = (time_t) repete * 24*60*60;
	
	
	frame[0] = SCHEDULE_BYTE;
	
	//Assumed RC6 standard mode 0
	frame[1] = RC6mode0;

	if (strlen (repetation_factor) == 2)
	{
		frame[2] = toupper (repetation_factor[0]);
		frame[3] = toupper(repetation_factor[1]);
	} 
	else if (strlen (repetation_factor) == 1)
	{
		frame[2] = '0';
		frame[3] = toupper(repetation_factor[0]);
	} 
	else {
		print_usage (stderr,1);
	}
	
	if (((!isdigit(frame[2])) && (frame[2] >= 'F' || frame[2] <='A')) ||
	    ((!isdigit(frame[3])) && (frame[3] >= 'F' || frame[3] <='A')) ) 
	{
		print_usage (stderr, 1);
	}
		      	//bytes 4 thro 7 of frame written by cmd_hex
	cmd_hex (device, command, &frame[4]);
	
	len=8;
	len = len + sprintf (&frame[8],"#%u#%u#",(unsigned long)t1,(unsigned long)t2);
	
	if (sendto (sock, frame, len ,0 , (struct sockaddr *)&echoServAddr,
	    sizeof(echoServAddr)) != len) {
		    fprintf (stderr,"Error in sendto");
		    exit (1);
	}
	
	alarm (TIMEOUT);
	
	/*int i;
	for (i=0;frame[i]!='\0' || i <2;i++)
		fprintf(stderr,"[%c %d]",frame[i],frame[i]);
	fprintf (stderr,"\nlength %d %d\n",i, len);*/
	//exit (0);
}



/*
 *This code reads the Server IP address from the cinfiguration file
 *config file $HOME/.irms/irms.conf
 */
 
void get_ip ( char ip_add[] )		
{
	char name[MAX_LEN], *mode = "r";
	char tmp[80], c, word[20];
	FILE *fp;
	char *path;

	if ((path = getenv ("HOME"))==NULL){
		fprintf (stderr,"Please set the HOME variable");
		exit(1);
	}
	strcpy (name,path);
	strcat (name,"/.irms/irms.conf");
	
	fp = fopen (name, mode);
	
	if ( !fp ) {
		fprintf  (stderr, "error opening irms.conf file\n");
		exit (1);
	}
	
	//fgetc, ungetc or fseek?(fseek clears EoF?)
	
	while ( !(feof(fp)) ) {
		c = fgetc(fp);
		if( c == '#')
			//comment line.. reead and forget
			fgets(tmp, 80, fp);
		else if( c == ' ')
			continue;
		else {
		//actual line
			ungetc(c, fp);
			//read it word by word 
			fscanf(fp, "%s", word);
			
			if(!strcmp("IP_address:", word)) {
				//if ip is found read it into ip_add[] and return from function
				fscanf(fp, "%s", ip_add);
				fclose (fp);
				return;
			}
				
		}
		
		
	}
	/*comes here only when ip_addrss is not found*/
	fclose(fp);
	printf("ip address nt found in irms.conf file\n");
	return;
}


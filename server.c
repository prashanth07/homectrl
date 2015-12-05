//NOTE: Varible list of all function prototypes should be modified suitably..... :)

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

/*Include definations related to serial port stuff*/
#include "serial.h"

/*Include all definations that are common across the client and server*/
#include "common.h"

/*Scheduler related definations*/
#include "scheduler.h"

/*
 *maximum number of connections allowed @ any time
 *Having a huge number is bad idea as it may delay the execution time of signal handler
 */
#define MAX_CONN 3


/*
 *Maximum time before auto logout i.e logout timer due to idle
 *it's not exact as we have only one timer at our disposal
 *the auto logout time would vary between MAX_TIMER to 1.5*MAX_TIMER
 */
#define MAX_TIMER 60*5

#define STX 0x02
#define ETX 0x03


int init_server(int serial_fd);
inline int authenticate (struct sockaddr_in *clientAddr);
void to_irms (int serial_fd, char *buf);
void reply (unsigned char var, int sock, struct sockaddr *clientAddr);

void remove_ip_from_list (int sock,struct sockaddr_in  *clientAddr );
void add_ip_to_list (int sock,struct sockaddr_in  *clientAddr);
void send_command (int serial_fd, int sock, struct sockaddr_in *clientAddr);
struct node * schedule_command (struct node *job_list, int sock, struct sockaddr_in *clientAddr);

/*Siganl handlers*/
void SIGIO_handler (int signal_number);
void SIGALRM_handler (int signal_number);


char buf [MAX_BUF_LEN];
char reply_buf [MAX_BUF_LEN];

//Just a temporary solution for login
//TODO:Remove the global declaration and make it more flexible, read from a file similar to ///etc/passwd
char *passwd = "hello";


struct node *job_list=NULL;


/*
 *decides whether to execute the if() block in main 
 *modified in more than one threads so to prevent race condition sig_atomic_t
 *io_count is incremented on recepit of an IO signal
 *schdl_count is incremented for the requirement of an scheduled task
 */

sig_atomic_t io_count = 0;
sig_atomic_t schdl_count = 0;


struct {
	struct sockaddr_in clnt_addr;
	time_t last_used;
	int in_use;	
} login_list [MAX_CONN];




/*
 *main function
 *cmd format  #./server
 */

int main(int argc, char *argv[])
{
	int sock, serial_fd;
	unsigned int recv_msg_len,client_len;
	struct sockaddr_in clientAddr;
	
	
	/*initialize the serial port*/
	serial_fd = init_serial ( );
	
	/*setup or initialize network
	 *setup to recieve SIGIO on completion of receving of UDP packet
	 *initialize the irms3
	 */
	fprintf (stderr, "initializing serial port ......done\n");
	sock = init_server ( serial_fd );
	
	/*
	 *Setting up the first alarm 
	 *subsequent alarm setted up by the SIGALRM_handler
	 */
	alarm (MAX_TIMER>>3);
	
	while (1) {
		if (io_count) {
			//read a message
			client_len = sizeof (clientAddr);
			recv_msg_len = recvfrom (sock, buf, MAX_BUF_LEN, 0,
					(struct sockaddr *) &clientAddr, &client_len);
			
			if (recv_msg_len < 0) {
				fprintf (stderr, "recvfrom() failed %d %d"
						,recv_msg_len,errno);
				exit (1);
			}
			
			int i;
			sigset_t mask_set,old_set;
			
			fprintf (stderr,"Buf:");
			for (i=0 ; i<recv_msg_len ;i++)
				fprintf (stderr,"%c",buf[i]);
			fprintf (stderr,"\\\t");
			
			switch (buf[0]) {
				case LOGIN_BYTE:
					add_ip_to_list (sock, &clientAddr);
					break;
					
				case LOGOUT_BYTE:
					remove_ip_from_list(sock, &clientAddr);
					break;
					
				case COMMAND_BYTE:
					send_command (serial_fd, sock, &clientAddr);
					break;
					
				case SCHEDULE_BYTE:
					//disable all the signals
					sigfillset (&mask_set);
					if (sigprocmask (SIG_SETMASK, &mask_set, &old_set) < 0){
						fprintf (stderr,"SIGALRM_handler:Cannot mask the signals");
						exit (1);
					}
					//done
	
					job_list = schedule_command (job_list, sock, &clientAddr);
	
					//restore the signal mask
					if (sigprocmask (SIG_SETMASK, &old_set, &mask_set) < 0){
						fprintf (stderr,"SIGALRM_handler:Cannot mask the signals");
						exit (1);
					}
					//done
					break;
				
				default:
					fprintf(stderr,"\n");
					reply(ERR_UNKNOWN_PACKET, sock, (struct sockaddr *)&clientAddr);
					break;
			}

			io_count--;
		}
		//wait for a signal
		pause ( );
		if (schdl_count) {
			sigset_t mask_set,old_set;
			
			
			while (schdl_count) {
				to_irms (serial_fd, &(job_list->jb.code));
				
				//disable all the signals
				sigfillset (&mask_set);
				if (sigprocmask (SIG_SETMASK, &mask_set, &old_set) < 0){
					fprintf (stderr,"SIGALRM_handler:Cannot mask the signals");
					exit (1);
				}
				//done
				fprintf (stderr, "in section critical\n");
				if (job_list->jb.repete_interval) {
					job_list->jb.exec_time += job_list->jb.repete_interval;
					job_list = add_job (job_list, job_list->jb);
					job_list = remove_job (job_list);
				} 
				else
					job_list = remove_job (job_list);
				
				//restore the signal mask
				if (sigprocmask (SIG_SETMASK, &old_set, &mask_set) < 0) {
					fprintf (stderr,"SIGALRM_handler:Cannot mask the signals");
					exit (1);
				}
				//done
				//print_list (job_list);
				schdl_count--;
				
			}
		}
	}
	return 0;
}




/*
 *Initializes the server.
 *	1.Initializes the signal handlers and all structures associated
 *	2.Initializes all the stuff related to the network
 *	3.Initializes the irms3 to send requestes to it
 */

int init_server(int serial_fd)
{
	struct sigaction sa_alrm, sa_io;
	struct sockaddr_in servAddr;
	int sock,i;
	
	/***********************Setting up of signals for the server******************/
	
	/*initialize the login_list structure*/
	for (i=0 ; i<MAX_CONN ;i++)
		(login_list + i)->in_use=0;
	
	//Stuff related to signal SIGALRM
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
	fprintf(stderr,"Setting up Signals..... done\n");
	/**********************Setting up the network for the server****************/
	sock = socket (PF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		fprintf (stderr, "error opeaning socket");
		exit (1);
	}
	
	/*Construct the server address structure*/
	
	bzero (&servAddr,sizeof(servAddr));
	servAddr.sin_family = AF_INET;			//Address Family
	servAddr.sin_addr.s_addr = htonl (INADDR_ANY);	//incomming addresses
	servAddr.sin_port = htons (PORT);		//Port to listen
	
	/*bind to the address*/
	if (bind (sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0){
		fprintf (stderr,"bind() failed");
		exit (1);
	}
	
	/* socket should recieve the SIGIO signal  */
	if ( fcntl (sock, F_SETOWN, getpid()) < 0){
		fprintf (stderr,"fcntl() failed for F_SETOWN");
		exit (1);
	}

	/* cofigure socket for asynchronous and nonblocking io */
	if (fcntl(sock, F_SETFL, O_NONBLOCK | FASYNC) < 0){
		fprintf (stderr, "fcntl() failed for O_NONBLOCK and FASYNC");
		exit (1);
	}
	fprintf (stderr,"Setting up Network........done\n");
	/************************initialize IRMS3 ***************************/
	char frame[6];
	
	frame[0] = 0x02;
	/*to send requests to irms 3 for sending IR signals*/
	strcpy (&frame[1],"0000");
	frame[5] = 0x03;
	
	send_data (serial_fd, frame,6);
	
	return sock;
}



/*
 *SIGIO_handler
 *on receving SIGIO increment the status ..indicating the availability of a UDP packet
 */

void SIGIO_handler (int signal_number)
{
	sigset_t mask_set,old_set;
	
	//disable all the signals
	sigfillset (&mask_set);
	if (sigprocmask (SIG_SETMASK, &mask_set, &old_set) < 0){
		fprintf (stderr,"SIGALRM_handler:Cannot mask the signals");
		exit (1);
	}
	//done
	
	io_count++;
	
	//restore the signal mask
	if (sigprocmask (SIG_SETMASK, &old_set, &mask_set) < 0){
		fprintf (stderr,"SIGALRM_handler:Cannot mask the signals");
		exit (1);
	}
	//done
}




/*
 *SIGALRM_handler
 *This routine is responsible for auto logout after the duration as prescribed 
 *by the MAX_TIMER
 */

void SIGALRM_handler (int signal_number)
{
	time_t present_time;
	sigset_t mask_set,old_set;
	register int i;
	
	//disable all the signals
	sigfillset (&mask_set);
	if (sigprocmask (SIG_SETMASK, &mask_set, &old_set) < 0) {
		fprintf (stderr,"SIGALRM_handler:Cannot mask the signals");
		exit (1);
	}
	//done
	
	/*The section of code below is meant for auto logout purpose
	 */
	present_time = time (NULL);
	
	for (i=0 ; i < MAX_CONN ;i++) {
		if ( ((login_list+i)->in_use) && 
		    ( (present_time - (login_list+i)->last_used) > (time_t)MAX_TIMER) ) {
			
			(login_list + i) -> in_use = 0;
			fprintf (stderr,"Auto Logout:remove_ip_from_list() client_ip %s\n"
					,inet_ntoa((login_list+i)->clnt_addr.sin_addr));
		
		}
	}
	/*Auto logout section done*/
	
	/*
	 *To execute scheduled tasks
	 *in some calls to alarm +1 is added only to make sure we dont do alarm(0) which will disable alarm
	 */
	
	struct node *tmp = job_list;
	
	if (job_list == NULL) {
		alarm (MAX_TIMER >> 3);
	}
	else if (job_list->jb.exec_time  <= present_time ){
		
		do {
			schdl_count++;
			tmp = tmp->next;
		} while ((tmp != NULL) && (tmp->jb.exec_time  <= present_time));
		
		if ( (tmp != NULL) && 
				(tmp->jb.exec_time - present_time < (time_t)(MAX_TIMER>>3)))
			alarm (tmp->jb.exec_time - present_time + 1);
		else
			alarm (MAX_TIMER >> 3);
	}
	else {
		if (job_list->jb.exec_time - present_time < (time_t)(MAX_TIMER>>3))
			alarm (job_list->jb.exec_time - present_time + 1);
		else
			alarm (MAX_TIMER >> 3);
	}
	
	//restore the signal mask
	if (sigprocmask (SIG_SETMASK, &old_set, &mask_set) < 0) {
		fprintf (stderr,"SIGALRM_handler:Cannot mask the signals");
		exit (1);
	}
	//done
}




/*
 *adds an IP address to the login_list
 *Checks for a free slot and fills in and send a login_succes to client
 *if no slot available then send a error to client
 */

void add_ip_to_list (int sock, struct sockaddr_in  *clientAddr)
{
	int i;
	
	if (!strncmp (&buf[2], passwd, strlen(passwd))) {
		for ( i=0 ; i<MAX_CONN ; i++ ) {
			if (!(login_list+i)->in_use) {
				
				(login_list+i)->in_use = 1;
				login_list[i].clnt_addr.sin_addr.s_addr =
						clientAddr->sin_addr.s_addr;
				(login_list+i)->last_used = time(NULL);
				reply(LOGIN_SUCCESS, sock, (struct sockaddr *)clientAddr);
				break;
			}
		}
		
		if (i == MAX_CONN) {
			reply (ERR_CONNECTION_FULL, sock, (struct sockaddr *)clientAddr);
			fprintf (stderr,"add_ip_to_list() limit, Conn.Refused client ip:%s\n"
					,inet_ntoa(clientAddr->sin_addr) );
			return;
		}
	}
	else {
		reply (ERR_LOGIN_FAILURE, sock, (struct sockaddr *)clientAddr);
		
		fprintf (stderr,"add_ip_to_list() wrong passwd Conn. Refused client ip:%s\n"
				,inet_ntoa(clientAddr->sin_addr) );
		return;
	}
		
	fprintf(stderr,"add_ip_list() client ip :%s\n",inet_ntoa(clientAddr->sin_addr));
}




/*
 *called on a logout request from user
 *Removes the specified ip from the list only if corresponding in_use is set to 1
 */

void remove_ip_from_list (int sock, struct sockaddr_in *clientAddr)
{
	int i;
	
	for (i=0 ; i<MAX_CONN ;i++){
		if ((login_list+i)->in_use  && 
		   ((login_list+i)->clnt_addr.sin_addr.s_addr==clientAddr->sin_addr.s_addr)){
			
			(login_list+i)->in_use=0;
			fprintf (stderr,"remove_ip_from_list() client_ip %s\n"
					,inet_ntoa(clientAddr->sin_addr));
			reply (LOGOUT_SUCCESS, sock, (struct sockaddr *)clientAddr);
			return;
		}
	}
	
	reply (ERR_NOT_LOGGED_IN, sock, (struct sockaddr *)clientAddr);
	
	fprintf (stderr,"remove_ip_from_list() NOT_LOGGED_IN client_ip %s\n"
			,inet_ntoa(clientAddr->sin_addr));
}




/*
 *build complete command and send it to irms thro serial
 *NOTE:buf should be pointing to the mode code in the frame
 */

void to_irms (int serial_fd ,char *buf)
{
	char frame[MAX_BUF_LEN];
	static char toggle = '0';
	unsigned int count;
	
	if (buf[0] == RC6mode0) {
		frame[0] = STX;
		frame[1] = '0';		//info byte
		frame[2] = '6';
		frame[3] = '0';		//Byte Count
		frame[4] = '4';
		frame[5] = buf[1];	//repetation Factor
		frame[6] = buf[2];
		frame[7] = '0';		//toggle bit
		frame[8] = toggle;
		
		//frame 9 thro 12 copied from buf
		strncpy (&frame[9], &buf[3],4);
		frame[13] = ETX;
		count = 14;
	}
	else {
		fprintf (stderr,"Standard Not Suported");
	}
	
	send_data (serial_fd, frame, count);
	
	//change the status of toggle byte
	if (toggle == '1')
		toggle ='0';
	else 
		toggle = '1';
	
}




/*
 *var indicates the kind of replay
 *var = LOGIN_SUCCESS      =>login successful
 *var = SUCCESS            =>successful execution of request
 *var = ERR_ILLEGAL_CMD    =>error "illegal cmd"
 *var = ERR_NOT_LOGGED_IN  =>error "not yet logged in"
 *var = ERR_CONNECTION_FULL =>error "connection denied as all slots are filled"
 */
 
void reply (unsigned char var, 
	    int sock, struct sockaddr *clientAddr)
{
	int len;
	
	reply_buf[0] = var;
	len=1;
	
	sendto (sock, reply_buf, len, 0, clientAddr, sizeof(*clientAddr));
}



/*
 *Authenticates the user 
 *return value 1 => he is logged in
 *		0 => not logged in 
 */
 
inline int authenticate (struct sockaddr_in *clientAddr)
{
	int i;
	
	for(i=0 ; i<MAX_CONN ;i++){
		if ( ((login_list+i)->clnt_addr.sin_addr.s_addr ==
				     clientAddr->sin_addr.s_addr) 
				     && ((login_list+i)->in_use != 0) ) {
			(login_list+i)->last_used = time(NULL);
			return 1;
		}
	}
	return 0;
}



/*
 *sends command to serial port using to_irms and 
 *replies correspondingly to the request sender
 */
 
void send_command (int serial_fd, 
		   int sock, struct sockaddr_in *clientAddr)
{
	
	if (authenticate (clientAddr)) {
		fprintf (stderr, "Handling request from IP:%s\n",
			 inet_ntoa(clientAddr->sin_addr));
		
		//second argument pointing to mode number
		to_irms (serial_fd,&buf[1]);
		
		reply (SUCCESS, sock, (struct sockaddr*)clientAddr);
	}
	else {
		fprintf (stderr,"command from a ip not logged in IP:%s\n"
				,inet_ntoa (clientAddr->sin_addr));
		reply (ERR_NOT_LOGGED_IN, sock, (struct sockaddr *)clientAddr);
	}
}


/*
 *Schedule command as
 */
 
struct node * schedule_command (struct node *job_list, 
				int sock, struct sockaddr_in *clientAddr)
{
	struct job a;
	int i;
	
	if (!authenticate (clientAddr)) {
		fprintf (stderr,"command from a ip not logged in IP:%s\n"
				,inet_ntoa(clientAddr->sin_addr));
		
		reply(ERR_NOT_LOGGED_IN,sock,(struct sockaddr *)clientAddr);
		return job_list;
	}
	
	/*Set up the job structure
	 */
	for (i=0 ; i<7 ; i++) {
		a.code[i] = buf[i+1];
		fprintf (stderr,"[%d %c]",a.code[i],a.code[i]);
	}
	sscanf (&buf[8],"#%u#%u#",&(a.exec_time),&(a.repete_interval));
	
	
	/*
	*Make Sure that the scheduled time is not from past
	*/
	if ((a.exec_time < time(NULL) ) ){
		reply (ERR_SCHEDULER, sock, (struct sockaddr*)clientAddr);
		
		fprintf (stderr, "Scheduler: error past time\n");
		return job_list;
	}
	
	job_list = add_job (job_list,a);
	
	reply (SUCCESS,sock,(struct sockaddr *)clientAddr);	
	print_list(job_list);
	fprintf (stderr,"Scheduled command from a ip IP:%s @ %u with rpt %u\n"
			,inet_ntoa(clientAddr->sin_addr),a.exec_time, a.repete_interval );
	
	return job_list;
}

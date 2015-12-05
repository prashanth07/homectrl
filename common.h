#ifndef IRMS_COMMON
#define IRMS_COMMON

/*Port used*/
#define PORT 55555

/*Length of the Maximum size of the buffer used to communicate between server and client*/
#define MAX_BUF_LEN 256


/*Maximum length of the commands and device names*/
#define MAX_LEN 32


/*reply from server to client*/
#define LOGIN_SUCCESS 		255
#define SUCCESS			254
#define LOGOUT_SUCCESS		253

#define ERR_ILLEGAL_CMD 	252
#define ERR_LOGIN_FAILURE 	251
#define ERR_NOT_LOGGED_IN 	250
#define ERR_CONNECTION_FULL	249
#define ERR_SCHEDULER		248
#define ERR_UNKNOWN_PACKET	247

/*Some Stabdards defined for the device*/
/*Addition of new standards requires modifying function to_irms() */
#define RC6mode0 0


/*Some defined packets and there header Byte(The First byte of any frame sent)*/
#define LOGIN_BYTE 0
#define LOGOUT_BYTE 1
#define COMMAND_BYTE 2
#define SCHEDULE_BYTE 3

#endif

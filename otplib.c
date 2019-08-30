/*
 * optlib.c
 * Alice O'Herin
 * Mar 17, 2019
 */

/* 
 * shared library functions
 */

 
/* LIBRARIES */
#include "otplib.h"


/* FUNCTION DEFINITIONS */
/*
 * Function: isValidPort
 * Description: checks that port number is valid (not reserved, in range)
 * Pre-condition: int port
 * Post-condition: returns TRUE if valid
 */
bool isValidPort(int p)
{
	if (p <= 1023 || p > 65535)
		return FALSE;
	else
		return TRUE;
}

/* NAME
 *  initialize
 * SYNOPSYS 
 * 	returns valid socket file descriptor
 */
int initialize(char *host, char *port, socktype st)
{
	struct addrinfo hints;					
	struct addrinfo *res;					// points to linked list of addrinfos
	struct addrinfo *p;						// for iterating through linked list
	int status, sockfd;
	
	memset(&hints, '\0', sizeof(hints));	// ensure the struct is empty
	hints.ai_family = AF_INET; 				// IPv4
	hints.ai_socktype = SOCK_STREAM;		// TCP
	hints.ai_flags = AI_PASSIVE;			// fills in IP address automatically
	
	// attempt to get the address info for connecting to, error check
	status = getaddrinfo(host, port, &hints, &res);
	if (status != 0)
	{
		fprintf(stderr, "Error: Failed to connect on port %s. getaddrinfo: %s\n", port, gai_strerror(status));
		return -1;
	}
	
	// iterate through linkedlist to find valid addrinfo
	for (p = res; p != NULL; p = p->ai_next)
	{
		// get the socket file descriptor, error check
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd == -1)
		{
			// perror("Error: socket()");
			continue;
		}
		
		// bind socket (as server) or connect (as client), error check
		if (st == BIND)
			status = bind(sockfd, p->ai_addr, p->ai_addrlen);
		else if (st == CONNECT)
			status = connect(sockfd, p->ai_addr, p->ai_addrlen);
		if (status == -1)
		{
			close(sockfd);
			// perror("Error: bind()");
			continue;
		}
		
		// exit the loop if successful
		break;
	}
	
	// free linked list
	freeaddrinfo(res);
	
	// iterated through entire linked list without successful connection
	if (p == NULL)
	{
		fprintf(stderr, "Error: Failed to connect on port %s.\n", port);
		return -1;
	}
	
	// else, success
	return sockfd;
}


/* NAME
 *  checkBg
 * SYNOPSYS 
 * 	checks for completed child processes
 *  updates array and number
 */
void checkBg(int arr[], int *num)
{
	int method = -5;
	pid_t check;
	while ((check = waitpid(-1, &method, WNOHANG)) > 0)
	{
		// remove returned pid from array
		int i;
		for (i = 0; i < *num; i++)
		{
			if (arr[i] == check)
			{
				arr[i] = arr[*num - 1];
				arr[*num - 1] = 0;
				break;
			}
		}
		(*num)--;
	}
}


/* NAME
 *  otp_send
 * SYNOPSYS 
 * 	sends string to file descriptor in format "<msg length> <msg>"
 *  returns bytes sent or -1 (error)
 */
int otp_send(int sockfd, char *msg)
{
	// get original message length as string
	int msglen = strlen(msg);
	char msglen_str[11];
	memset(msglen_str, '\0', sizeof(msglen_str));
	sprintf(msglen_str, "%d ", strlen(msg));
	
	// get total length of message, including prepended character count
	int msglen_strlen = strlen(msglen_str);
	int tosend = msglen_strlen + msglen;
	
	// allocate and copy header-ed message
	char *str = (char *) calloc(tosend + 1, sizeof(char));
	sprintf(str, "%s%s", msglen_str, msg);
	str[tosend] = '\0';
	
	// loop to send
	int sent_total = 0;
	int sent = 0;
	while (sent_total < tosend)
	{
		sent = send(sockfd, str + sent_total, tosend, 0);
		if (sent == -1)
		{
			perror("Error: send()");
			return -1;
		}
		else if (sent == 0)
		{
			fprintf(stderr, "Connection closed: incomplete send().\n");
			return -1;
		}
		else
		{
			// loop until bytes leave buffer
			int bytes_left = -5;
			do
			{
			  ioctl(sockfd, TIOCOUTQ, &bytes_left); 
			} while (bytes_left > 0);

			if (bytes_left < 0)
			  error("ioctl error");

			// update variables
			sent_total = sent_total + sent;
			tosend = tosend - sent;
		}
	}
	
	free(str);
	return sent_total;
}


/* NAME
 *  otp_recv
 * SYNOPSYS 
 * 	receives string from file descriptor in format "<msg length> <msg>"
 *  returns <msg> as dynamically allocated string
 */
char * otp_recv(int sockfd)
{
	int numbytes = -5;
	char strlen_buf[11];
	int strlen_rcvd = 0;
	
	memset(strlen_buf, '\0', sizeof(strlen_buf));
	
	// loop to recv prepended msg length single char at a time
	while (strlen_rcvd == 0 || strlen_buf[strlen_rcvd - 1] != ' ')
	{
		// printf("Starting recv loop %d\n", loopnum);
		numbytes = recv(sockfd, strlen_buf + strlen_rcvd, 1, 0);
		if (numbytes == -1)
		{
			perror("Error: recv() msg length");
			return NULL;
		}
		// check connection closed
		else if (numbytes == 0)
		{
			return NULL;
		}
		
		strlen_rcvd = strlen_rcvd + numbytes;
		strlen_buf[strlen_rcvd] = '\0';
	}
	
	// remove trailing space, convert length to int value, allocate memory
	strlen_buf[strlen_rcvd - 1] = '\0';
	int length = strtol(strlen_buf, NULL, 10);
	char *str = (char *) calloc(length + 1, sizeof(char));
	
	strlen_rcvd = 0;
	while (strlen_rcvd < length)
	{
		numbytes = recv(sockfd, str + strlen_rcvd, length - strlen_rcvd, 0);
		// check failed
		if (numbytes == -1)
		{
			perror("Error: recv() message");
			return NULL;
		}
		// check connection closed
		else if (numbytes == 0)
		{
			printf("Connection closed by server.\n");
			return NULL;
		}
			
		strlen_rcvd = strlen_rcvd + numbytes;
		str[strlen_rcvd] = '\0';
	}
		
	// successful receive
	return str;
}


/* NAME
 *  hasValidChars
 * SYNOPSYS 
 * 	checks characters in string are ASCII A to Z or space
 */
bool hasValidChars(char *str)
{
	if (str == NULL)
		return FALSE;
	int length = strlen(str);
	int i;
	for (i = 0; i < length; i++)
	{
		int c = str[i];
		// check that the char is A-Z or space
		if (!((c >= 65 && c <= 90) || (c == 32)))
			return FALSE;
	}
	
	return TRUE;
}


/* NAME
 *  f_tostring
 * SYNOPSYS 
 * 	reads from file and stores in dynamically allocated string
 */
char * f_tostring(char *filename)
{
	int bin = -5;				// bytes read in to buffer each loop
	int totalread = 0;			// total read so far
	int msglen = 4096;			// length of dynamically allocated, resizing string
	char *msg = (char *) calloc(msglen, sizeof(char));
	msg[0] = '\0';				// msg must be null-terminated to perform string lib functions
	char buff[msglen];			// buffer to hold read contents each loop
	memset(buff, '\0', sizeof(buff));
	
	// open file, set file pointer to start
	int fd = open(filename, O_RDONLY);
	if (fd == -1) 
	{
		fprintf(stderr, "File Not Found: %s.\n", filename);
		return NULL;
	}
	lseek(fd, 0, SEEK_SET);
	
	// loop to read from file
	while ((bin = read(fd, buff, sizeof(buff) - 1)) > 0)
	{	
		if (bin == -1)
		{
			fprintf(stderr, "Error: read()\n");
			return NULL;
		}
		buff[bin] = '\0';
		totalread = totalread + bin;
		
		// resize msg string by doubling
		if (totalread >= msglen)
		{
			msglen = msglen * 2;
			msg = (char *)realloc(msg, msglen);
		}
		
		// append newly read characters
		strcat(msg, buff);
		
		memset(buff, '\0', sizeof(buff));
	}
	
	// strip off last newline
	if (msg[totalread - 1] == '\n')
		msg[totalread - 1] = '\0';
	
	return msg;
}

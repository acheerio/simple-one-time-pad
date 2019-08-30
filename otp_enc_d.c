/*
* opt_enc_d.c
* Alice O'Herin
* Mar 17, 2019
*/

/* 
* encryption server
*/


/* LIBRARIES */
#include "otplib.h"


/* MACROS */
#define BACKLOG 5
#define MAXCXNS 5
#define ACCEPTID "enc"


/* GLOBAL VARIABLES */
int listenfd = 0;
int sockfd = 0;							// file descriptor for socket
char *id = NULL;						// id of connection, to be verified
char *plain = NULL;						// contents of plaintext
char *key = NULL;						// contents of key
char *code = NULL;						// encoded message


/* FUNCTION DECLARATIONS */
void memclean();
void closesock();
void waitkids();
void catchSIGINT(int signo);
char * encode(char * plain, char * key);


/* FUNCTION DEFINITIONS */
/* NAME
 *  memclean
 * SYNOPSYS 
 * 	frees dynamic memory
 */
void memclean() {
	if (id)
		free(id);
	if (plain)
		free(plain);
	if (key)
		free(key);
	if (code)
		free(code);
}


/* NAME
 *  closesock
 * SYNOPSYS 
 * 	closes sockets
 */
void closesock() {
	if (listenfd > 0)
		close(listenfd);
	if (sockfd > 0)
		close(sockfd);
}


/* NAME
 *  waitkids
 * SYNOPSYS 
 * 	waits for child processes
 */
void waitkids() {
	int method;
	while (wait(&method) != -1);
}


/* NAME
 *  catchSIGINT
 * SYNOPSYS 
 * 	signal handler for SIGINT (Ctrl-C)
 *	exit with cleanup
 */
void catchSIGINT(int signo)
{
	exit(130);
}


/* NAME
 *  encode
 * SYNOPSYS 
 * 	uses key to encrypt plaintext
 */
char * encode(char * plain, char * key)
{
	// map char -> int, A-Z = 0-25, Space = 26
	int dict[91];
	memset(dict, 0, sizeof(dict));
	int i;
	for (i = 0; i <= 25; i++)
	{
		dict[i + 65] = i;
	}
	dict[32] = 26;
	
	// map int -> char, 0-25 = A-Z, 26 = Space
	char dict2[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

	// encode
	int length = strlen(plain);
	char *code = calloc(length + 1, sizeof(char));
	int j;
	for (j = 0; j < length; j++)
	{
		int pci = plain[j]; 		// ASCII value of plaintext char
		int pc = dict[pci];			// convert to 0 - 26 value
		int kci = key[j];			// ASCII value of key char
		int kc = dict[kci];			// convert to 0 - 26 value
		
		int enci = (pc + kc) % 27;
		char encc = dict2[enci];	// convert encoded int val -> char
		
		code[j] = encc;
	}
	
	code[j] = '\0';
	return code;
}


/* NAME
 *  main
 * SYNOPSYS 
 * 	simple server - verifies client, encodes received plaintext with key
 *  and sends back cipher
 * USAGE
 *  otp_enc_d <port num>
 */
int main(int argc, char *argv[]) {	
	// register cleanup functions
	atexit(memclean);
	atexit(closesock);
	atexit(waitkids);
	
	// SIGINT cleanup
	struct sigaction SIGINT_action = {0};
	SIGINT_action.sa_handler = catchSIGINT;
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = SA_RESTART;
	sigaction(SIGINT, &SIGINT_action, NULL);
	
	int status = -5;
	char port[8];
	memset(port, '\0', sizeof(port));
	
	// check that program was executed with two arguments
	if (argc != 2) {
		fprintf(stderr, "Incorrect number of arguments.\n");
		exit(1);
	}
	
	// get port as string
	strcpy(port, argv[1]);
	
	// check for valid port number
	if (!isValidPort(strtol(port, NULL, 10))) {
		fprintf(stderr, "Invalid port number.\n");
		exit(1);
	}
	
	// get socket file descriptor with port, listen
	listenfd = initialize("localhost", port, BIND);
	if (listenfd == -1) {
		exit(1);
	}
	
	status = listen(listenfd, BACKLOG);
	if (status == -1) {
		perror("listen()");
		exit(1);
	}
	// printf("Server open on %s\n", port);
	
	// initialize variables for use in loop
	struct sockaddr_in caddr = {0};			// holds info about client address
	socklen_t caddr_size = sizeof(caddr);	// needed for getnameinfo()
	int kids[MAXCXNS];						// array of current cxn proc ids
	int numkids = 0;						// count of current cxns
	int childPid;							// process ID for forked connection
	
	memset(kids, 0, sizeof(kids));
	
	// loop to accept connections
	while (1) {
		// printf("Waiting for connections...\n");
		
		// accept client connection
		memset(&caddr, 0, sizeof(caddr));
		sockfd = accept(listenfd, (struct sockaddr *)&caddr, &caddr_size);
		if (sockfd == -1) {
			perror("accept()");
			continue;
		}
		
		// if connections > MAXCXNS, reject new connection
		checkBg(kids, &numkids);		
		if (numkids >= MAXCXNS) {
			fprintf(stderr, "Error: %d connections, rejecting new connection.\n", MAXCXNS);
			close(sockfd);
			sockfd = 0;
			continue;
		}
		
		// fork child process to encode
		childPid = fork();
		switch (childPid)
		{
			case -1:
			{
				perror("fork()");
				break;
			}
			case 0:		// child process
				// get and verify connection id
				id = otp_recv(sockfd);
				if (!id) {
					exit(2);
				}
				status = strcmp(id, ACCEPTID);
				if (status == 0) {
					if (otp_send(sockfd, "OK") < 1)
						exit(2);
				}
				else {
					otp_send(sockfd, "INVALID ID");
					exit(2);
				}
								
				// recv plaintext
				if (!(plain = otp_recv(sockfd))) {
					fprintf(stderr, "Error: Did not receive plaintext file.\n");
					exit(1);
				}
				// recv key
				if (!(key = otp_recv(sockfd))) {
					fprintf(stderr, "Error: Did not receive key file.\n");
					exit(1);
				}
				
				// error checking: valid characters, length
				if (!(hasValidChars(plain) && hasValidChars(key))) {
					fprintf(stderr, "Error: Invalid characters in file.\n");
					exit(1);
				}
				if (strlen(plain) > strlen(key)) {
					fprintf(stderr, "Error: Key too short.\n");
					exit(1);
				}
				
				// send encoded message
				code = encode(plain, key);
				otp_send(sockfd, code);
				
				// cleanup
				exit(0);
				break;
			default:
			{		
				// add current connection
				kids[numkids] = childPid;
				numkids++;
			}
		}
		
		close(sockfd);
		sockfd = 0;
	}

	// printf("Shutting down server.\n");
    return 0;
}

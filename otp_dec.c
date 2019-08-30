/*
* opt_dec.c
* Alice O'Herin
* Mar 17, 2019
*/

/* 
* decryption client
*/


/* LIBRARIES */
#include "otplib.h"


/* MACROS */
#define MYID "dec"


/* GLOBAL VARIABLES */
int sockfd = 0;							// file descriptor for socket
char *reply = NULL;						// server response to id
char *plain = NULL;						// contents of plaintext
char *key = NULL;						// contents of key
char *code = NULL;						// encoded message


/* FUNCTION DECLARATIONS */
void memclean();
void closesock();


/* FUNCTION DEFINITIONS */
/* NAME
 *  memclean
 * SYNOPSYS 
 * 	frees dynamic memory
 */
void memclean() {
	if (reply)
		free(reply);
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
	if (sockfd > 0)
		close(sockfd);
}


/* NAME
 *  main
 * SYNOPSYS 
 * 	simple client - connects, sends ciphertext and key,
 *  receives back and prints cipher
 * USAGE
 *  otp_enc <ciphertext file> <key file> <port num>
 */
int main(int argc, char *argv[]) {
	
	// register cleanup functions
	atexit(memclean);
	atexit(closesock);
	
	// variables
	int status = -5;
	char port[8];
	memset(port, '\0', sizeof(port));
	
	// check that program was executed with 4 arguments
	if (argc != 4) {
		fprintf(stderr, "Incorrect number of arguments.\n");
		exit(2);
	}
	
	// get ciphertext and key from file
	code = f_tostring(argv[1]);
	if (!code)
		exit(1);
	key = f_tostring(argv[2]);
	if (!key)
		exit(1);
	
	// error checking: valid characters, length
	if (!(hasValidChars(code) && hasValidChars(key))) {
		fprintf(stderr, "Error: Invalid characters in file.\n");
		exit(1);
	}
	if (strlen(code) > strlen(key)) {
		fprintf(stderr, "Error: Key too short.\n");
		exit(1);
	}
	
	// get port as string
	strcpy(port, argv[3]);
	
	// check for valid port number
	if (!isValidPort(atoi(port))) {
		fprintf(stderr, "Error: Unable to connect. Invalid port number %s.\n", port);
		exit(2);
	}
	
	// connect, get socket file descriptor
	sockfd = initialize("localhost", port, CONNECT);
	if (sockfd == -1) {
		exit(2);
	}
	
	// authenticate, send id, wait for reply
	otp_send(sockfd, MYID);
	reply = otp_recv(sockfd);
	if (!(reply && strcmp(reply, "OK") == 0)) {
		fprintf(stderr, "Error: ID mismatch, connection to port %s rejected.\n", port);
		fprintf(stderr, "(Specifically, otp_dec cannot connect to otp_enc_d.)\n");
		exit(2);
	}
	
	// send ciphertext, key
	if ((status = otp_send(sockfd, code)) < 0) {
		fprintf(stderr, "Error: otp_send() unable to send ciphertext\n");
		exit(1);
	}
	if ((status = otp_send(sockfd, key)) < 0) {
		fprintf(stderr, "Error: otp_send() unable to send key\n");
		exit(1);
	}
	
	// receive code
	code = otp_recv(sockfd);
	if (code)
		printf("%s\n", code);
	
    return 0;
}

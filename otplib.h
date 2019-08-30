#ifndef OTPLIB_H
#define OTPLIB_H


/*
 * optlib.h
 * Alice O'Herin
 * Mar 17, 2019
 */

/* 
 * shared library functions (header file)
 */
 

/* LIBRARIES */
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h> 
#include <sys/socket.h>


/* STRUCTS AND ENUMS */
typedef enum bool {FALSE, TRUE} bool;
typedef enum socktype {CONNECT, BIND} socktype;


/* FUNCTION DECLARATIONS */
bool isValidPort(int p);
int initialize(char *host, char *port, socktype st);
void checkBg(int arr[], int *num);
int otp_send(int sockfd, char *msg);
char * otp_recv(int sockfd);
bool hasValidChars(char *str);
char * f_tostring(char *filename);

#endif
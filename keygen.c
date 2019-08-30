/*
 * keygen.c
 * Alice O'Herin
 * Mar 17, 2019
 */

/* 
 * random key generator
 */


/* LIBRARIES */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


/* STRUCTS AND ENUMS */
typedef enum bool {FALSE, TRUE} bool;


/* GLOBAL VARIABLES */
const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";


/* FUNCTION DECLARATIONS */
bool isPositiveInt(char *arg);
bool hasValidArgs(int argc, char *arg);


/* FUNCTION DEFINITIONS */
/* NAME
 *  isPositiveInt
 * SYNOPSYS 
 * 	checks that string is a valid representation of positive int
 */
bool isPositiveInt(char *arg)
{
	// is it negative
	if (arg[0] == '-')
		return FALSE;
	
	// is each char a valid digit
	int i;
	int length = strlen(arg);
	for (i = 1; i < length; i++)
	{
		if (!isdigit(arg[i]))
			return FALSE;
	}
	
	/* missing a check for overflow but this should suffice
	for our purposes here */
	
	return TRUE;
}


/* NAME
 *  hasValidArgs
 * SYNOPSYS 
 * 	checks that main() was called with valid syntax / argument
 */
bool hasValidArgs(int argc, char *arg)
{
	// check that program was executed with two arguments
	if (argc < 2)
	{
		fprintf(stderr, "Error: Missing keylength. Usage: keygen [keylength]\n");
		return FALSE;
	}
	// prints an error but will still return true, ignoring extra arguments
	else if (argc >= 3)
	{
		fprintf(stderr, "Error: Too many arguments.\n");
	}
	// check for negatives, non-ints
	if (!isPositiveInt(arg))
	{
		fprintf(stderr, "Error: Keylength must be positive integer.\n");
		return FALSE;
	}
	
	return TRUE;
}


/* NAME
 *  main
 * SYNOPSYS 
 * 	prints <number> random characters (A-Z or ' ') plus newline
 *  total chars = <number> + 1
 * USAGE
 *  keygen <number>
 */
int main(int argc, char *argv[]) {
	
	srand(time(NULL));
	
	if (!hasValidArgs(argc, argv[1]))
	{
		exit(1);
	}
	
	// convert from str -> int
	int length = strtol(argv[1], NULL, 10);
	
	// loop and print
	int i;
	for (i = 0; i < length; i++)
	{
		int r = rand() % 27;
		printf("%c", alphabet[r]);
	}
	
	printf("\n");
	
	return 0;
}
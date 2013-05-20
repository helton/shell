#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE   1<<10
#define MAX_COMMAND_COUNT 	1<<7
#define MAX_PARAMETER_COUNT	1<<7
#define TRUE 1
#define PRINT_ERROR fprintf(stderr, "Error: %s\n", strerror(errno));

char *copyString(char *);
void executeCommands();
void freeCommands();
char *getString(char *, int);
int  parseCommandParameters(char *, char **);
int  parseCommands(char *, char **);
void parseLine(char *);
void printCommands();
void printPrompt();
void readInput();
void setupSignalHandlers();
void signalActionSIGINT_SIGSTPHandler(int );

typedef struct {
	char *command;
	char *parameters[MAX_PARAMETER_COUNT];
	int parameterCount;
} commandInfo;

int commandCount = 0;
commandInfo **commands;

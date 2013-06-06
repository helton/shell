#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 	1<<10
#define MAX_COMMAND_COUNT 	1<<7
#define MAX_PARAMETER_COUNT	1<<7
#define TRUE 1
#define DEFAULT -1
#define PRINT_ERROR fprintf(stderr, "Error: %s\n", strerror(errno));

void closePipes(int *);
char *copyString(char *);
void executeCommands();
void freeCommands();
char *getString(char *, int);
void openPipes(int *);
int  parseCommandParameters(char *, char **);
int  parseCommands(char *, char **);
void parseLine(char *);
void printCommands();
void printPrompt();
void readInput();
void setupSignalHandlers();
void signalActionSIGINT_SIGSTPHandler(int);
void waitChildProcesses(pid_t *);

typedef struct {
	char *command;
	char *parameters[MAX_PARAMETER_COUNT];
	int inputIndex;
	int outputIndex;
	int parameterCount;
} commandInfo;

enum status {
	STOPPED,
	RUNNING
};

int commandCount = 0;
commandInfo **commands;
sig_atomic_t currentStatus = RUNNING;

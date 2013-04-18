/*
 * Shell.c
 *
 *  Created on: Apr 18, 2013
 *      Author: helton
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 255
#define FALSE 0
#define TRUE  1
#define PRINT_ERROR fprintf(stderr, "Error: %s\n", strerror(errno));

void executeCommand(char **);
void forkToExecuteComand(char **command);
void parseCommand(char *, char **);
void printPrompt();
void readCommand(char *);
void readCommands();
void setupSignalActionHandlers();
void signalActionSIGINT_SIGSTPHandler(int);

sig_atomic_t canExecuteNextCommand = TRUE;

int main(int argc, char **argv) {
	setupSignalActionHandlers();
	readCommands();
	return 0;
}

void executeCommand(char **command) {
	if (!canExecuteNextCommand) {
		canExecuteNextCommand = TRUE;
		printf("\n");
	}
	else {
		if (!strcmp(command[0], "exit")) {
			exit(0);
		}
		else if (!strcmp(command[0], "cd")) {
			if (chdir(command[1] ? command[1] : getenv("HOME"))) {
				PRINT_ERROR
			}
		}
		else {
			forkToExecuteComand(command);
		}
	}
}

void forkToExecuteComand(char **command) {
	if (fork()) {
		int status;
		wait(&status);
	}
	else {
		execvp(command[0], command);
		PRINT_ERROR
	}
}

void parseCommand(char *command, char **parsedCommand) {
    int indexArgs = 0;
    char *process = command;
    process = strtok(process, " \t");
    while (process) {
    	parsedCommand[indexArgs++] = process;
        process = strtok(NULL, " \t");
    }
    parsedCommand[indexArgs] = NULL;
}

void printPrompt() {
    char *cwd, *user, *home, hostname[MAX_BUFFER_SIZE];
    if ((cwd = getcwd(NULL, MAX_BUFFER_SIZE)) &&
    	(!gethostname(hostname, sizeof(hostname))) &&
    	(user = getenv("USER")) && (home = getenv("HOME"))) {
    	if (!strncasecmp(home, cwd, strlen(home))) {
    		cwd = (char *) cwd + strlen(home) - 1;
    		cwd[0] = '~';
    	}
    	printf("[MySh] %s@%s:%s$ ", user, hostname, cwd);
    }
}

void readCommand(char *command) {
	char *parsedCommand[MAX_BUFFER_SIZE];
	parseCommand(command, parsedCommand);
	executeCommand(parsedCommand);
}

void readCommands() {
	char command[MAX_BUFFER_SIZE];
	do {
		printPrompt();
		readCommand(gets((char *)&command));
	} while(TRUE);
}

void setupSignalActionHandlers() {
	struct sigaction signalActionSIGINT_SIGSTP;
	memset (&signalActionSIGINT_SIGSTP, 0, sizeof (signalActionSIGINT_SIGSTP));
	signalActionSIGINT_SIGSTP.sa_handler = &signalActionSIGINT_SIGSTPHandler;
	sigaction(SIGINT,  &signalActionSIGINT_SIGSTP, NULL);  // Ctrl + C signal
	sigaction(SIGTSTP, &signalActionSIGINT_SIGSTP, NULL);  // Ctrl + Z signal
}

void signalActionSIGINT_SIGSTPHandler(int signalNumber) {
	canExecuteNextCommand = FALSE;
}

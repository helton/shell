/*
 * Shell.c
 *
 *  Created on: Apr 18, 2013
 *      Author: helton
 */

#include "shell.h"

int main(int argc, char **argv) {
	setupSignalHandlers();
	readInput();
	return EXIT_SUCCESS;
}

char *copyString(char *source) {
	char *dest = NULL;
	if (source) {
		dest = malloc(sizeof(source));
		strcpy(dest, source);
	}
	return dest;
}

void executeCommands() {
	pid_t pid;
	int i, j, fds[(commandCount-1)*2];
	for (i = 0; i < (commandCount-1)*2; i+=2) {
		pipe(fds + i);
	}
	for (i = 0; i < commandCount; i++) {
		if (!strcmp(commands[i]->command, "exit")) {
			exit(EXIT_SUCCESS);
		}
		else if (!strcmp(commands[i]->parameters[0], "cd")) {
			if (chdir((!commands[i]->parameterCount) ? getenv("HOME") : commands[i]->parameters[1])) {
				PRINT_ERROR
			}
		}
		else {
			pid = fork();
			if (pid == -1) {
				PRINT_ERROR
			}
			else if (!pid) {
				if (commandCount > 0 && i > 0) {
					dup2(fds[(i - 1) * 2], fileno(stdin));
				}
				if (commandCount > 0 && i < commandCount-1) {
					dup2(fds[((i + 1) * 2)-1], fileno(stdout));
				}
				for (j = 0; j < (commandCount-1)*2; j++) {
					close(fds[j]);
				}
				execvp(commands[i]->parameters[0], commands[i]->parameters);
				PRINT_ERROR
				exit(EXIT_FAILURE);
			}
		}
	}
	for (i = 0; i < (commandCount-1)*2; i++) {
		close(fds[i]);
	}
	while (TRUE) {
	    int status;
	    pid_t done = wait(&status);
	    if (done == -1) {
	        if (errno == ECHILD) {
	        	break;
	        }
	        else {
	        	if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
	        		PRINT_ERROR
	        	}
	        }
	    }
	}
}

void freeCommands() {
	int i, j;
	for (i = 0; i < commandCount; i++) {
		for (j = 0; j < commands[i]->parameterCount; j++) {
			free(commands[i]->parameters[j]);
		}
		free(commands[i]);
	}
	free(commands);
}

char *getString(char *buffer, int length) {
	char *p;
	fgets(buffer, length, stdin);
	if ((p = strchr(buffer, '\n')) != NULL) {
		*p = '\0';
	}
	return buffer;
}

int parseCommandParameters(char *command, char **parsedParameters) {
    int count = 0;
    char *process = command;
    process = strtok(process, " \t");
    while (process) {
    	parsedParameters[count] = malloc(sizeof(process));
    	strcpy(parsedParameters[count++], process);
        process = strtok(NULL, " \t");
    }
    parsedParameters[count] = NULL;
    return count-1;
}

int parseCommands(char *line, char **commandList) {
	int count = 0;
    char *strCommand = line;
    strCommand = strtok(strCommand, "|");
    while (strCommand) {
    	commandList[count++] = strCommand;
    	strCommand = strtok(NULL, "|");
    }
    commandList[count] = NULL;
    return count;
}

void parseLine(char *line) {
	commandInfo *currentCommand;
	char *commandList[MAX_COMMAND_COUNT];
    commandCount = parseCommands(line, commandList);
    commands = malloc(sizeof(commandInfo)*commandCount);
    int i;
    for (i = 0; i < commandCount; i++) {
    	currentCommand = malloc(sizeof(commandInfo));
    	currentCommand->command = malloc(sizeof(commandList[i]));
		strcpy(currentCommand->command, commandList[i]);
		char *commandCopy = copyString(currentCommand->command);
		currentCommand->parameterCount = parseCommandParameters(commandCopy, currentCommand->parameters);
		free(commandCopy);
		commands[i] = currentCommand;
    }
    executeCommands();
    //printCommands();
    freeCommands();
}

void printCommands() {
	int i, j;
	for (i = 0; i < commandCount; i++) {
		printf("\n--------------------------------------");
		printf("\n# Command = %s", commands[i]->command);
		printf("\n# ParameterCount = %d", commands[i]->parameterCount);
		for (j = 0; j <= commands[i]->parameterCount; j++) {
			printf("\n - Parameter[%d] = %s", j, commands[i]->parameters[j]);
		}
		printf("\n--------------------------------------\n");
	}
}

void printPrompt() {
    char *cwd, *user, *home, hostname[MAX_BUFFER_SIZE];
	fflush(stdin);
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

void readInput() {
	char line[MAX_BUFFER_SIZE];
	do {
		printPrompt();
		parseLine(getString(line, MAX_BUFFER_SIZE));
	} while(TRUE);
}

void setupSignalHandlers() {
	struct sigaction signalActionSIGINT_SIGSTP;
	memset (&signalActionSIGINT_SIGSTP, 0, sizeof (signalActionSIGINT_SIGSTP));
	signalActionSIGINT_SIGSTP.sa_handler = &signalActionSIGINT_SIGSTPHandler;
	sigaction(SIGINT,  &signalActionSIGINT_SIGSTP, NULL);  // Ctrl + C signal
	sigaction(SIGTSTP, &signalActionSIGINT_SIGSTP, NULL);  // Ctrl + Z signal
}

void signalActionSIGINT_SIGSTPHandler(int signalNumber) {
	pid_t pid;
	while (!(pid = waitpid(-1, NULL, WNOHANG))) {
		kill(pid, SIGKILL);
	}
}

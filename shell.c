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

void closePipes(int *fds) {
	if (fds) {
		int i;
		for (i = 0; i < (commandCount - 1) * 2; i++) {
			close(fds[i]);
		}
	}
}

char *copyString(char *source) {
	char *dest = NULL;
	if (source) {
		dest = malloc(strlen(source));
		strcpy(dest, source);
	}
	return dest;
}

void executeCommands() {
	if (currentStatus == STOPPED) {
		currentStatus = RUNNING;
		return;
	}
	pid_t pid, *childProcesses;
	int i, fds[(commandCount - 1) * 2];
	openPipes(commandCount ? &fds[0] : NULL);
	childProcesses = malloc(sizeof(pid_t) * commandCount);
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
			else if (pid){
				childProcesses[i] = pid;
			}
			else  {
				if (commands[i]->inputIndex != DEFAULT) {
					dup2(fds[commands[i]->inputIndex], fileno(stdin));
				}
				if (commands[i]->outputIndex != DEFAULT) {
					dup2(fds[commands[i]->outputIndex], fileno(stdout));
				}
				closePipes(commandCount ? &fds[0] : NULL);
				execvp(commands[i]->parameters[0], commands[i]->parameters);
				PRINT_ERROR
				exit(EXIT_FAILURE);
			}
		}
	}
	closePipes(commandCount ? &fds[0] : NULL);
	waitChildProcesses(&childProcesses[0]);
	free(childProcesses);
}

void freeCommands() {
	int i, j;
	for (i = 0; i < commandCount; i++) {
		for (j = 0; j < commands[i]->parameterCount+1; j++) {
			free(commands[i]->parameters[j]);
		}
		free(commands[i]->command);
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

void openPipes(int *fds) {
	if (fds) {
		int i;
		for (i = 0; i < (commandCount - 1) * 2; i += 2) {
			pipe(fds + i);
		}
	}
}

int parseCommandParameters(char *command, char **parsedParameters) {
    int count = 0;
    char *process = command;
    process = strtok(process, " \t\"");
    while (process) {
   		parsedParameters[count++] = copyString(process);
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
	if (feof(stdin)) {
		printf("exit\n");
		exit(EXIT_SUCCESS);
	}
	else {
		commandCount = parseCommands(line, commandList);
		commands = malloc(sizeof(commandInfo)*commandCount);
		int i;
		for (i = 0; i < commandCount; i++) {
			currentCommand = malloc(sizeof(commandInfo));
			currentCommand->command = copyString(commandList[i]);
			char *commandCopy = copyString(currentCommand->command);
			currentCommand->parameterCount = parseCommandParameters(commandCopy, currentCommand->parameters);
			currentCommand->inputIndex  = i == 0 ? DEFAULT : (2 * i - 2);
			currentCommand->outputIndex = i == commandCount - 1 ? DEFAULT : (2 * i + 1);
			free(commandCopy);
			commands[i] = currentCommand;
		}
		executeCommands();
		//printCommands();
		freeCommands();
	}
}

void printCommands() {
	int i, j;
	for (i = 0; i < commandCount; i++) {
		printf("\n--------------------------------------");
		printf("\n# Command = %s", commands[i]->command);
		if (commands[i]->inputIndex != DEFAULT) {
			printf("\n# In:  fds[%d]", commands[i]->inputIndex);
		}
		else {
			printf("\n# In:  stdin");
		}
		if (commands[i]->outputIndex != DEFAULT) {
			printf("\n# Out: fds[%d]", commands[i]->outputIndex);
		}
		else {
			printf("\n# Out: stdout");
		}
		printf("\n# ParameterCount = %d", commands[i]->parameterCount);
		for (j = 0; j <= commands[i]->parameterCount; j++) {
			printf("\n - Parameter[%d] = %s", j, commands[i]->parameters[j]);
		}
		printf("\n--------------------------------------\n");
	}
}

void printPrompt() {
    char *cwd, *user, *home, *path, hostname[MAX_BUFFER_SIZE];
    if ((cwd = getcwd(NULL, MAX_BUFFER_SIZE)) &&
    	(!gethostname(hostname, sizeof(hostname))) &&
    	(user = getenv("USER")) && (home = getenv("HOME"))) {
    	if (!strncasecmp(home, cwd, strlen(home))) {
    		path = cwd;
    		path = (char *) path + strlen(home) - 1;
    		path[0] = '~';
    	}
    	printf("[MySh] %s@%s:%s$ ", user, hostname, path);
    	free(cwd);
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
	currentStatus = STOPPED;
	printf("\n");
}

void waitChildProcesses(pid_t *childProcesses) {
	int i;
	if (childProcesses) {
		for (i = 0; i < commandCount; i++) {
			int status;
			pid_t waitReturnedValue = waitpid(childProcesses[i], &status, 0);
			if ((waitReturnedValue == -1) && (!WIFEXITED(status) || WEXITSTATUS(status) != 0)) {
				fprintf(stderr, "\n-> Interrupted process killed (pid = %d).\n", childProcesses[i]);
				kill(childProcesses[i], SIGTERM);
			}
		}
	}
}

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

char buffer[1000];
char *seperatedBuffer[100];
char *history[100];
char *commandList[10];
int commandNum = 3;
int commandLen;
int historyLen = -1;
//handling signal
int pid = 0;
int exitCode = 0;
//for I/O redirection
int metaHistry = -1;
int metaPos = -1;
int fd;
int temp;
void ioRedirection(char** command, int len);

void echo(char** command, int n) {

    if(n == 2 && !(strcmp(command[1], "$?"))) {
        printf("%d\n", exitCode);
        return;
    }

    for (int i=1; i<n; i++) {
        printf("%s", command[i]);
        if (i < n-1) printf(" ");
    }

    printf("\n");

}

void addHistory(char** command, int n) {

    for (int i =0; i< historyLen; i++) {
        free(history[i]);
        history[i] = NULL;
    }

    for (int i=0; i<n; i++) {
        history[i] = strdup(command[i]);
    }

    historyLen = n;

}

void createCommand(char** list) {

    list[0] = "echo";
    list[1] = "!!";
    list[2] = "exit";

}

void seperateInput(char* command, char** output) {

    char* words = strtok(command, " ");
    int len;

    int i = 0;
    while (words != NULL) {
        //use for I/O redirection
        if (!strcmp(words, "<") || !strcmp(words, ">")) metaPos = i;

        output[i++] = words;
        words = strtok(NULL, " ");
    }
    len = i;

    //get rid of '\n' from the last argument
    int lastchar = strlen(output[len-1]) - 1;
    output[len-1][lastchar] = '\0';
    if (lastchar == 0) {
        output[len-1] = '\0';
        len--;
    }

    //clear the old remaining commands
    while(i < historyLen) {
        output[i++] = NULL;
    }

    commandLen = len;
}


int foreground(char** command, int len) {

    int status = 0;

    //set last element to NULL
    for (int i=0; i<len; i++) {
        if (command[i][0] == '\0') command[i] = NULL;
    }
    command[len] = NULL;

    if ((pid=fork()) < 0) {
        perror ("Fork failed");
        exit(errno);
    }
    if (!pid) {
    	if (metaPos != -1) {
    		if (!strcmp(seperatedBuffer[metaPos], ">")) dup2(fd, 1);
    		else if (!strcmp(seperatedBuffer[metaPos], "<")) dup2(fd, 0);
    	}
        execvp(command[0], command);
        //end process in case of unsupported command
        exit(-1);
    }

    if (pid) {
        waitpid(pid, &status, 0);
        exitCode = WEXITSTATUS(status);
    }
        return status;
}

//update: 0 = don't update history, 1 = update history
void processInput(char** command, int wordCount, int update) {

    //when called '!!' and there is no last command
    if ((strcmp("!!", command[0]) == 0) && historyLen == -1) return;

    int commandID = 0;
    for (int i=0; i<commandNum; i++) {
        if (strcmp(command[0], commandList[i]) == 0) {
            commandID = i+1;
            break;
        }
    }

    int status;
    switch (commandID) {
    //default
    case 0:
        status = foreground(command, wordCount);
        //if child process was interrupted errno = EINTR
        if (errno == EINTR) printf("\n");
        if (status && errno != EINTR && exitCode == 255 && strcmp(command[0], "./icsh")) printf("bad command\n");

        //reset errno (so that it won't affect the prompt loop)
        errno = 0;
        break;
    //command = 'echo'
    case 1:
        echo(command, wordCount);
        exitCode = 0;
        break;
    //command = '!!'
    case 2:
    	if (metaHistry < historyLen && metaHistry != -1) {
			if (!strcmp(history[metaHistry], "<") || !strcmp(history[metaHistry], ">")) {
				metaPos = metaHistry;
				ioRedirection(history, historyLen);
		        return;
			}
    	}
		processInput(history, historyLen, 0);
        exitCode = 0;
        return;
    //command = 'exit'
    case 3:
        printf("goodbye:)\n");
        //free all history
        for (int i=0; i<historyLen; i++) {
            free(history[i]);
        }
        exit(atoi(command[1]) & 0xff);
    }

    if (update == 1)
    addHistory(command, wordCount);
}


void ioRedirection(char** command, int len) {
    char *file1[10];
    char *file2 = command[metaPos+1];
    int f1Len = 0;

    for (f1Len=0; f1Len<metaPos; f1Len++) {
        file1[f1Len] = command[f1Len];
    }

    //out redirect
    if (!strcmp(command[metaPos], ">")) {
        temp = dup(1);
        fd = open(file2, O_TRUNC | O_CREAT | O_WRONLY, 0666);

        if (fd <= 0) {
            fprintf(stderr, "Couldn't open a file\n");
            exit(errno);
        }

        dup2(fd, 1);
        processInput(file1, f1Len, 0);
        dup2(temp, 1);

    }

    //in redirect
    if (!strcmp(command[metaPos], "<")) {
    	temp = dup(0);
        fd = open(file2, O_RDONLY);

        if (fd <= 0) {
            fprintf(stderr, "Couldn't open a file\n");
            exit(errno);
        }

        foreground(file1, f1Len);

    }

    close(fd);

    metaHistry = metaPos;
    metaPos = -1;

    //update history only when command is not "!!"
    if (strcmp(seperatedBuffer[0], "!!"))
    addHistory(command, len);
    return;
}

void signalHandler (int sig, siginfo_t *sip, void *notused) {

    if (pid != 0) {
        kill(pid, sig);
        pid = 0;
    }

   return;

}

void createSigHandler() {
    struct sigaction action;
    action.sa_sigaction = signalHandler;
    sigfillset (&action.sa_mask);
    action.sa_flags = SA_SIGINFO;

    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTSTP, &action, NULL);

}

int main(int argc, char** argv) {

    createCommand(commandList);

    createSigHandler();

    //run from script
    if (argc == 2) {
        FILE* fp = fopen(argv[1], "r");

        while(fgets(buffer, 1000, fp)) {
            if (buffer[0] == '\n') continue;

            seperateInput(buffer, seperatedBuffer);
            processInput(seperatedBuffer, commandLen, 1);
        }
        fclose(fp);
        return 0;
    }

    printf("Starting IC shell\n");

    while (1) {
        printf("icsh $ ");
        fgets(buffer, 1000, stdin);
        if (buffer[0] == '\n' || errno == EINTR) {
            if (errno == EINTR) printf("\n");
            errno = 0;
            continue;
        }

        seperateInput(buffer, seperatedBuffer);
        //I/O redirection
        if (metaPos != -1) {
            ioRedirection(seperatedBuffer, commandLen);
            continue;
        }
        processInput(seperatedBuffer, commandLen, 1);
    }
}




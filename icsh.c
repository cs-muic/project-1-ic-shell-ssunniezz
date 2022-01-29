#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

char *history[100];
char *commandList[10];
int commandNum = 3;
int historyLen = -1;

void echo(char** command, int n) {

    for (int i=1; i<n; i++) {
        printf("%s", command[i]);
        if (i < n-1) printf(" ");
    }

}

void addHistory(char** command, int n) {

    for (int i=0; i<n; i++) {
        free(history[i]);
        history[i] = strdup(command[i]);
    }

    historyLen = n;

}

void createCommand(char** list) {

    list[0] = "echo";
    list[1] = "!!\n";
    list[2] = "exit";

}

int seperateInput(char* command, char** output) {

    char* words = strtok(command, " ");

    int i = 0;
    while (words != NULL) {
        output[i++] = words;
        words = strtok(NULL, " ");
    }

    return i;
}

int foreground(char** command) {

    int pid;
    int status;

    //remove '\n' from last argument and set last element to NULL
    int i = 0;
    while (command[i] != NULL) {
        int last = strlen(command[i]) - 1;
        if (command[i][last] == '\n') {
            command[i][last] = '\0';
            command[++i] = NULL;
            break;
        }
        i++;
    }

    if ((pid=fork()) < 0) {
        perror ("Fork failed");
        exit(errno);
    }
    if (!pid) {
        execvp(command[0], command);
        exit(1);
    }

    if (pid) {
        waitpid(pid, &status, 0);
        return status;
    }
}

//update: 0 = don't update history, 1 = update history
void processInput(char** command, int wordCount, int update) {

    //when called '!!' and there is no last command
    if ((strcmp("!!\n", command[0]) == 0) && historyLen == -1) return;

    int commandID = 0;
    for (int i=0; i<commandNum; i++) {
        if (strcmp(command[0], commandList[i]) == 0) {
            commandID = i+1;
            break;
        }
    }


    switch (commandID) {
    //default
    case 0:
        if (foreground(command))
        printf("bad command\n");
        break;
    //command = 'echo'
    case 1:
        echo(command, wordCount);
        break;
    case 2:
    //command = '!!'
        processInput(history, historyLen, 0);
        return;
    //command = 'exit'
    case 3:
        printf("goodbye:)\n");
        exit(atoi(command[1]) & 0xff);
    }

    if (update == 1)
    addHistory(command, wordCount);
}

int main(int argc, char** argv) {

    char buffer[1000];
    char *seperatedBuffer[100];
    createCommand(commandList);

    //run from script
    if (argc == 2) {
        FILE* fp = fopen(argv[1], "r");

        while(fgets(buffer, 1000, fp)) {
            if (buffer[0] == '\n') continue;

            int wordCount = seperateInput(buffer, seperatedBuffer);
            processInput(seperatedBuffer, wordCount, 1);
        }
        fclose(fp);
        return 0;
    }

    printf("Starting IC shell\n");

    while (1) {
        printf("icsh $ ");
        fgets(buffer, 1000, stdin);
        if (buffer[0] == '\n') continue;

        int wordCount = seperateInput(buffer, seperatedBuffer);
        processInput(seperatedBuffer, wordCount, 1);
    }
}

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *history[100];
char *commandList[10];
int commandNum = 3;
int historyLen = -1;

void echo(char** command, int n) {
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

int seperateInput(char* command, char** output) {
    
    char* words = strtok(command, " ");
    int len;

    int i = 0;
    while (words != NULL) {
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
    return len;

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


    switch (commandID) {
    case 0:
        printf("bad command\n");
        break;
    case 1:
        echo(command, wordCount);
        break;
    case 2:
        processInput(history, historyLen, 0);
        return;
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

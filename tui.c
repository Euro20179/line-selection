#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "keys.h"

#define MAX_KEY_LENGTH 10
#define bool _Bool
#define true 1
#define false 0

int selectedLine = 0;

struct buffer{
    int size;
    char* buffer;
};

struct buffer readChars(int size){
    char buf[size];
    struct termios savedState;
    struct termios newState;
    unsigned char echoBit;

    //This fails whhen reading stdin
    if(-1 == tcgetattr(STDIN_FILENO, &savedState)){
	struct buffer rv = {.size = 0, .buffer = ""};
	return rv;
    }
    newState = savedState;
    //no echo
    newState.c_lflag &= ~(ECHO | ICANON);
    //wait for 1 char
    newState.c_cc[VMIN] = 1;
    if(-1 == tcsetattr(STDIN_FILENO, TCSANOW, &newState)){
	struct buffer rv = {.size = 0, .buffer = ""};
	return rv;
    }
    int bufLength;
    bool escapeSequence = false;
    for(bufLength = 0; bufLength < size; bufLength++){
	if(bufLength != 0 && escapeSequence == false){
	    break;
	}
	buf[bufLength] = getchar();
	if(buf[bufLength] == 27){
	    escapeSequence = true;
	}
	//91 is [, 48-57 are numbers, 79 is O, 59 is ;
	else if((escapeSequence && buf[bufLength] != 91) && (buf[bufLength] > 57 || buf[bufLength] < 48) && buf[bufLength] != 79 && buf[bufLength] != 59){
	    escapeSequence = false;
	}
    }
    if(-1 == tcsetattr(STDIN_FILENO, TCSANOW, &savedState)){
	struct buffer rv = {.size = 0, .buffer = ""};
	return rv;
    }
    char bufferBuffer[bufLength];
    struct buffer rv = {.size = bufLength, .buffer = bufferBuffer};
    for(int i = 0; i < bufLength; i++){
	bufferBuffer[i] = buf[i];
    }
    //apparently special key presses not being read if it is the first key press is a flushing issue?
    fflush(stdout);
    return rv;
}

int getKeyRepr(int size, char* fullKeyPress){
    int repr = fullKeyPress[0];
    if(size > 1){
	for(int i = 2; i < size; i++){
	    repr *= fullKeyPress[i];
	}
    }
    return repr;
}

//USE FREE
char* getKeyStrRepr(int size, char* fullKeyPress){
    char* buf = malloc(MAX_KEY_LENGTH);
    if(size > 1){
	buf[0] = '[';
	for(int i = 1; i < size; i++){
	    buf[i - 1] = fullKeyPress[i];
	}
	buf[size - 1] = '\0';
	return buf;
    }
    else{
	buf[0] = fullKeyPress[0];
	buf[1] = '\0';
	return buf;
    }
}

void printKey(int size, char* fullKeyPress){
    int numRepr = getKeyRepr(size, fullKeyPress);

    char* keyRepr = getKeyStrRepr(size, fullKeyPress);
    printf("%d: %s\n", numRepr, keyRepr);
    free(keyRepr);
}

void clear(){
    fprintf(stderr, "\033[1;1H\033[2J");
}

void printLines(const char* lines){
    int lineNo = 0;
    bool printedColor = false;
    for(int i = 0; i < strlen(lines); i++){
	if(lineNo == selectedLine && printedColor == false){
	    fprintf(stderr, "%s", "\033[42m");
	    printedColor = true;
	}
	if(lines[i] == '\n'){
	    if(lineNo == selectedLine){
		fprintf(stderr, "%s", "\033[0m");
		printedColor = false;
	    }
	    lineNo++;
	}
	fprintf(stderr, "%c", lines[i]);
    }
    fprintf(stderr, "\033[0m\n");
}

const int getLineCount(const char* string){
    int count = 0;
    for(int i = 0; i < strlen(string); i++){
	if(string[i] == '\n'){
	    count++;
	}
    }
    return count;
}

void printStrLine(const char* str, int lineNo){
    int currentLine = 0;
    for(int i = 0; i < strlen(str); i++){
	if(str[i] == '\n'){
	    currentLine++;
	    continue;
	}
	else if(currentLine == lineNo){
	    printf("%c", str[i]);
	}
	else if(currentLine > lineNo){
	    break;
	}
    }
    printf("\n");
}

char* createString(int size){
    char* alloc = malloc(size);
    alloc[0] = '\0';
    return alloc;
}

char* increaseStringSize(char* str, int newSize){
    char* alloc = malloc(newSize);
    strcpy(alloc, str);
    free(str);
    return alloc;
}

void delString(char* str){
    free(str);
}


int main(const int argc, const char* argv[]){
    if(argc < 2){
	fprintf(stderr, "No input file given\n");
	return 1;
    }
    const char* fp = argv[1];
    FILE* fd = fopen(fp, "r");
    if(fd == NULL){
	fprintf(stderr, "An error occured while trying to open the file\n");
	return 1;
    }
    int currentTextSize = 10;
    char* lines = createString(currentTextSize);
    char ch;
    int charNo;
    while(read(fileno(fd), &ch, 1) > 0){
	lines[charNo] = ch;
	charNo++;
	if(charNo > currentTextSize){
	    currentTextSize *= 2;
	    lines = increaseStringSize(lines, currentTextSize);
	}
    }

    while(lines[strlen(lines) - 1] == '\n'){
	lines[strlen(lines) - 1] = '\0';
    }

    const int lineCount = getLineCount(lines);
    clear();
    printLines(lines);
    while(true){
	struct buffer buff = readChars(MAX_KEY_LENGTH);
	clear();
	char* keyPress = buff.buffer;
	const int keyPressSize = buff.size;
	int keyNumber = getKeyRepr(keyPressSize, keyPress);
	switch(keyNumber){
	    case 0: continue;
	    case 10:
		printStrLine(lines, selectedLine);
		return 0;
	    case j:
	    case DOWN:
		if(selectedLine < lineCount){
		    selectedLine++;
		}
		break;
	    case k:
	    case UP:
		if(selectedLine > 0){
		    selectedLine--;
		}
		break;
	}
	printLines(lines);
    }
}

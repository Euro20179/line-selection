#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include "keys.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#define MAX_KEY_LENGTH 10
#define bool _Bool
#define true 1
#define false 0

static int COLS;
static int LINES;

int selectedLine = 0;

struct buffer{
    int size;
    char* buffer;
};

typedef struct{
    bool doNumbering;
} opts;

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

int getFirstCharIndexOfLineNumber(int lineNo, const char* lines){
    int line = 0;
    for(int i = 0; i < strlen(lines); i++){
	if(lines[i] == '\n'){
	    line++;
	}
	if(line == lineNo){
	    return i;
	}
    }
    return -1;
}

void printLines(int currentLine, const char* lines, bool doNumbering){
    int lineNo = currentLine;
    int printedLineCount = 0;
    bool printedColor = false;
    int start = getFirstCharIndexOfLineNumber(currentLine, lines);
    if(lines[start] == '\n')start++;
    for(int i = start; i < strlen(lines); i++){
	if(i == start && doNumbering) //print the line number on the first character
	    fprintf(stderr, "%d: ", lineNo);

	if(lineNo == selectedLine && printedColor == false){
	    fprintf(stderr, "%s", "\033[42m");
	    printedColor = true;
	}
	//print current character before checking if new line so that current line number is displayed first
	fprintf(stderr, "%c", lines[i]);
	if(lines[i] == '\n'){
	    if(printedLineCount + 3 >= LINES)
		break;
	    if(lineNo == selectedLine){
		fprintf(stderr, "%s", "\033[0m");
		printedColor = false;
	    }
	    lineNo++;
	    printedLineCount++;
	    if(doNumbering){
		//print line number at start of line
		fprintf(stderr, "%d: ", lineNo);
	    }
	}
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

opts getOpts(int argc, char* argv[]){
    int opt;
    opts options = { false };
    while ((opt = getopt(argc, argv, "n")) != -1){
	switch(opt){
	    case 'n':
		options.doNumbering = true;
		return options;
	}
    }
    return options;
}

int keyCodeToInt(int keyCode){
    if(keyCode < 48 || keyCode > 57)
	return -1;
    return keyCode - 48;
}

void printAtBottomOfScreen(char* t){
    fprintf(stderr, "\033[10;0H%s", t);
}

int main(int argc, char* argv[]){
    struct winsize size;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    COLS = size.ws_col;
    LINES = size.ws_row;

    opts options = getOpts(argc, argv);
    if(argc < 2){
	fprintf(stderr, "No input file given\n");
	return 1;
    }
    const char* fp = argv[optind];
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
    printLines(selectedLine, lines, options.doNumbering);
    while(true){
	struct buffer buff = readChars(MAX_KEY_LENGTH);
	clear();
	char* keyPress = buff.buffer;
	const int keyPressSize = buff.size;
	int keyNumber = getKeyRepr(keyPressSize, keyPress);
	switch(keyNumber){
	    case 0: continue;
	    case q: return 5; //slightly different return codes just in case scripting wants to do something with it
	    case Q: return 6; //slightly different return codes just in case scripting wants to do something with i
	    case 10:
		printStrLine(lines, selectedLine);
		return 0;
	    case ZERO:
	    case ONE:
	    case TWO:
	    case THREE:
	    case FOUR:
	    case FIVE:
	    case SIX:
	    case SEVEN:
	    case EIGHT:
	    case NINE:
		int i = keyCodeToInt(keyNumber);
		if(lineCount >= i){
		    printStrLine(lines, i);
		    return 0;
		}
		break;
	    case DOLLAR:
		printStrLine(lines, lineCount);
		return 0;
	    case PLUS: {
		char lineToGoTo[6];
		lineToGoTo[0] = '\0';
		while(true){
		    clear();
		    printLines(selectedLine, lines, options.doNumbering);
		    struct buffer buff = readChars(MAX_KEY_LENGTH);
		    char* keyPress = buff.buffer;
		    const int keyPressSize = buff.size;
		    int keyNumber = getKeyRepr(keyPressSize, keyPress);
		    if(keyNumber == 10) break;
		    int i = keyCodeToInt(keyNumber);
		    if(i == -1) continue;
		    strncat(lineToGoTo, (char*)&keyNumber, 1);
		}
		int realLine = atoi(lineToGoTo);
		if(realLine <= lineCount){
		    selectedLine = realLine;
		}
		clear();
#if DEBUG == 1
		    printf("%s\n", lineToGoTo);
#endif
		break;
	    }
	    case n:
	    case j:
	    case DOWN:
		if(selectedLine < lineCount){
		    selectedLine++;
		}
		break;
	    case p:
	    case k:
	    case UP:
		if(selectedLine > 0){
		    selectedLine--;
		}
		break;
	}
#if DEBUG == 1
	printKey(keyPressSize, keyPress);
#endif
	printLines(selectedLine, lines, options.doNumbering);
    }
    delString(lines);
}

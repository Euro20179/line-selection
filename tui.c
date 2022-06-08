#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#define BUF_SIZE 10
#define bool _Bool
#define true 1
#define false 0

struct buffer{
    int size;
    char* buffer;
};

struct buffer readChars(int size){
    char buf[size];
    struct termios savedState;
    struct termios newState;
    unsigned char echoBit;
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

int main(const int argc, const char* argv[]){
    while(true){
	struct buffer buff = readChars(BUF_SIZE);
	char* keyPress = buff.buffer;
	const int keyPressSize = buff.size;
	int keyNumRepr = keyPress[0];
	if(keyPressSize > 1){
	    //-1 because we dont need the escape character and [, as all special keys start with that
	    //not -2 because we need space for null byte at the end
	    char specialKeypress[keyPressSize - 1];
	    //offset by 2 for similar reason above
	    for(int i = 2; i < keyPressSize; i++){
		keyNumRepr *= keyPress[i];
		specialKeypress[i - 2] = keyPress[i];
	    }
	    specialKeypress[keyPressSize - 2] = '\0';
	    printf("%d: %s\n", keyNumRepr, specialKeypress);
	}
	else printf("%d: %c\n", keyPress[0], keyPress[0]);
    }
}

build:
	gcc -DDEBUG=1 -o tui *.c
release:
	gcc -o tui *.c


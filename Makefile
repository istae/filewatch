dest = ../../bin
all:
	gcc -std=c99 main.c -o ${dest}/fw
	cp ${dest}/fw ${dest}/filewatch

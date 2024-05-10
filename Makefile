# Makefile

x24clock: x24clock.c
	gcc -o x24clock x24clock.c -lX11 -lm

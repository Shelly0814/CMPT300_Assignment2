## This is a simple Makefile with lost of comments 
## Check Unix Programming Tools handout for more info.
# Define what compiler to use and the flags.
CC=gcc
CXX=g++
CCFLAGS= -g -Wall -Werror
# Compile all .c files into .o files
# % matches all (like * in a command)
# $< is the source file (.c file)
all:
	gcc -o haha haha.c
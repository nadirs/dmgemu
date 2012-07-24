CC=gcc
CFLAGS=-pg -Wall -Wextra -Wshadow -fstack-protector -O3 -std=c99
SRC=$(wildcard *.c)

program_NAME := dmgemu
program_SRCS := $(wildcard *.c)
program_OBJS := ${program_SRCS:.c=.o}

.PHONY: all clean distclean

all: $(program_NAME)

$(program_NAME): $(program_OBJS)
	$(CC) $(program_OBJS) -o $(program_NAME)

clean:
	@- $(RM) $(program_NAME)
	@- $(RM) $(program_OBJS)
	@- $(RM) ./.*.swp ./*~ ./*.gch

distclean: clean

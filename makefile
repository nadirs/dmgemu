CC=gcc
CFLAGS=-Wall -Wextra -Wshadow -fstack-protector -O3 -std=c99
SRC=$(wildcard *.c)

program_NAME := dmgemu
program_C_SRCS := $(wildcard *.c)
program_C_OBJS := ${program_C_SRCS:.c=.o}
program_OBJS := $(program_C_OBJS)

.PHONY: all clean distclean

all: $(program_NAME)

$(program_NAME): $(program_OBJS)
	$(CC) $(program_OBJS) -o $(program_NAME)

clean:
	@- $(RM) $(program_NAME)
	@- $(RM) $(program_OBJS)
	@- $(RM) ./.*.swp ./*~

distclean: clean

#dmgemu: $(SRC)
#	$(CC) -o  $@ $^ $(CFLAGS)
#
#clean:
#	rm -f ./dmgemu 

CC=gcc
CFLAGS=-Wall -Wextra -Wshadow -fstack-protector -O3 -std=c99
#CFLAGS=-g -D__USE_FIXED_PROTOTYPES__ -ansi -std=c99
#
# Compiler flags:
# -g    -- Enable debugging
# -Wall -- Turn on all warnings (not used since it gives away
#       the bug in this program)
#   -D__USE_FIXED_PROTOTYPES__
#       -- Force the compiler to use the correct headers
#   -ansi   -- Don't use GNU extensions. Stick to ANSI C.

cpu: cpu.c cpu.h
	$(CC) $(CFLAGS) -o ./cpu ./cpu.c

clean:
	rm -f ./cpu

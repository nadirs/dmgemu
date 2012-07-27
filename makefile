CFLAGS = -pg -Wall -Wextra -Wshadow -fstack-protector -O3 -std=c99
OBJFLAGS = -fPIC

program_NAME := libdmgemu.so

vpath %.h src
OBJDIR := ./build
OBJDIR_SRC := $(OBJDIR)/src
program_SRCS := $(wildcard ./src/*.c)
program_OBJS := $(patsubst %, $(OBJDIR_SRC)/%.o, asm cpu memory myrandom)

mkdir_build_src = $(shell test -d $(OBJDIR_SRC) || mkdir -p $(OBJDIR_SRC))

.PHONY: all clean distclean

all: $(program_NAME)

$(program_NAME): $(program_OBJS)
	$(CC) $(program_OBJS) -o $@ -shared

$(OBJDIR_SRC)/asm.o: ./src/asm.c
	$(mkdir_build_src)
	$(CC) $< -o $@ $(OBJFLAGS) -c

$(OBJDIR_SRC)/cpu.o: ./src/cpu.c
	$(mkdir_build_src)
	$(CC) $< -o $@ $(OBJFLAGS) -c

$(OBJDIR_SRC)/memory.o: ./src/memory.c
	$(mkdir_build_src)
	$(CC) $< -o $@ $(OBJFLAGS) -c

$(OBJDIR_SRC)/myrandom.o: ./src/myrandom.c
	$(mkdir_build_src)
	$(CC) $< -o $@ $(OBJFLAGS) -c

clean:
	@- $(RM) $(program_NAME)
	@- $(RM) -r ./build/
	@- $(RM) ./src/.*.swp ./src/*~ ./src/*.gch

distclean: clean

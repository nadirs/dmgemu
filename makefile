CFLAGS = -pg -Wall -Wextra -Wshadow -fstack-protector -O3 -std=c99
OBJFLAGS = -fPIC
test_NAME := testdmgemu

program_NAME := libdmgemu.so

vpath %.h src:tests
OBJDIR := ./build
OBJDIR_SRC := $(OBJDIR)/src
OBJDIR_TESTS := $(OBJDIR)/tests
program_SRCS := $(wildcard ./src/*.c)
program_OBJS := $(patsubst %, $(OBJDIR_SRC)/%.o, asm cpu memory myrandom)
test_SRCS := $(wildcard ./tests/*.c)
test_OBJS := $(patsubst ./tests/%.c, $(OBJDIR_TESTS)/%.o, $(test_SRCS))

mkdir_build_src = $(shell test -d $(OBJDIR_SRC) || mkdir -p $(OBJDIR_SRC))
mkdir_build_tests = $(shell test -d $(OBJDIR_TESTS) || mkdir -p $(OBJDIR_TESTS))

.PHONY: all clean distclean

all: $(program_NAME) $(test_NAME)

$(program_NAME): $(program_OBJS)
	$(CC) $(program_OBJS) -o $@ -shared

$(test_NAME): $(test_OBJS) $(program_OBJS)
	$(CC) $(test_OBJS) $(program_OBJS) -o $@ $(CFLAGS)

# SRC
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

# TESTS
$(OBJDIR_TESTS)/asm_test.o: ./tests/asm_test.c
	$(mkdir_build_tests)
	$(CC) $< -o $@ $(OBJFLAGS) -c

clean:
	@- $(RM) $(program_NAME)
	@- $(RM) $(test_NAME)
	@- $(RM) -r ./build/
	@- $(RM) ./src/.*.swp ./src/*~ ./src/*.gch
	@- $(RM) ./tests/.*.swp ./tests/*~ ./tests/*.gch

distclean: clean

RM ?= $(shell which rm)
CWD ?= $(shell pwd)
BUILD_LIBRARY_PATH = $(CWD)/../build/lib

## test source files
SOURCES += $(wildcard *.c)

## test target names which is just the
## source file without the .c extension
TARGETS = $(SOURCES:.c=)

## test compiler flags
CFLAGS += -Wall
CFLAGS += -Werror
CFLAGS += -I ../build/include
CFLAGS += -I ../deps
CFLAGS += -L $(BUILD_LIBRARY_PATH)
CFLAGS += -g

ifeq (Darwin, $(shell uname))
  CFLAGS += -framework Foundation
endif

## test dependency source files
DEPS += $(wildcard ../deps/*/*.c)

## we need to set the LD_LIBRARY_PATH environment variable
## so our test executables can load the built library at runtim
export LD_LIBRARY_PATH = $(BUILD_LIBRARY_PATH)
export DYLD_LIBRARY_PATH = $(BUILD_LIBRARY_PATH)

ifneq (1,$(NO_BRIEF))
-include ../mk/brief.mk
endif

.PHONY: all
all: $(TARGETS)
	@for t in $^; do          \
	  printf '\n## %s\n' $$t; \
		./$$t;                  \
	  printf '...\n' $$t;     \
  done

$(TARGETS): $(SOURCES)
	$(CC) -o $@ $(wildcard ../src/*.c) $@.c $(DEPS) $(CFLAGS) -D OK_EXPECTED=`cat *.c|grep 'ok('|wc -l`

.PHONY: clean
clean:
	@$(RM) $(TARGETS)

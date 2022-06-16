CFLAGS = -Wextra -Wall -D_GNU_SOURCE -g -MMD -Iinclude -Ilinenoise \
	 -Ielfutils/libdw -Ielfutils/libelf
LDFLAGS = -Wl,-rpath="$(CURDIR)" -L. -ldw

CURDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
OUT ?= build
LIBDW = libdw.so
BINARY = $(OUT)/raid
SHELL_HACK := $(shell mkdir -p $(OUT))

GIT_HOOKS := .git/hooks/applied

CSRCS = $(shell find ./src -name '*.c')
CSRCS += $(shell find ./linenoise -name 'linenoise.c')
_COBJ =  $(notdir $(CSRCS))
COBJ = $(_COBJ:%.c=$(OUT)/%.o)

vpath %.c $(sort $(dir $(CSRCS)))

all: $(GIT_HOOKS) $(BINARY) $(LIBDW)

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

$(LIBDW):
	@scripts/install-libdw.sh

$(OUT)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(BINARY): $(COBJ) $(LIBDW)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

run: $(BINARY)
	$(BINARY) bin/hello
clean:
	$(RM) $(COBJ)
	$(RM) $(BINARY)
	$(RM) $(OUT)/*.d

-include $(OUT)/*.d

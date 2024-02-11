TEST_FILES := validate_cap
TEST_PROGS := test_execve

BINARIES := $(TEST_FILES) $(TEST_PROGS)

CFLAGS += -O3 -g -std=gnu99 -Wall
LDLIBS += -lcap-ng -lrt -ldl

all: $(BINARIES)

clean:
	$(RM) $(BINARIES)

include ../lib.mk


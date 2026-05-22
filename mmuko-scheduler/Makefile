# MMUKO OS Makefile
# github.com/obinexus/mmuko-os

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
MMUKO_CPPFLAGS = -DMMUKO_SCHEDULER_TEST
SCHEDULER_CPPFLAGS = -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L
LDLIBS = -lm

ifeq ($(OS),Windows_NT)
    EXEEXT = .exe
    CLEAN_CMD = del /Q
    CLEAN_REDIRECT = 2>NUL
else
    EXEEXT =
    CLEAN_CMD = rm -f
    CLEAN_REDIRECT =
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        LDLIBS += -lrt
    endif
endif

TARGET = mmuko-os$(EXEEXT)
SCHEDULER_TARGET = scheduler$(EXEEXT)
HELLO_TARGET = hello$(EXEEXT)

MMUKO_SRCS = mmuko.c mmuko_scheduler.c
MMUKO_OBJS = $(MMUKO_SRCS:.c=.o)
SCHEDULER_OBJS = scheduler.o
HELLO_OBJS = hello.o

.PHONY: all clean test scheduler hello run-hello

all: $(TARGET)

$(TARGET): $(MMUKO_OBJS)
	$(CC) $(CFLAGS) -o $@ $(MMUKO_OBJS) $(LDLIBS)

scheduler: $(SCHEDULER_TARGET)

$(SCHEDULER_TARGET): $(SCHEDULER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(SCHEDULER_OBJS) $(LDLIBS)

hello: $(HELLO_TARGET)

$(HELLO_TARGET): $(HELLO_OBJS)
	$(CC) $(CFLAGS) -o $@ $(HELLO_OBJS)

mmuko.o: mmuko.c mmuko.h
	$(CC) $(MMUKO_CPPFLAGS) $(CFLAGS) -c $< -o $@

mmuko_scheduler.o: mmuko_scheduler.c mmuko.h
	$(CC) $(MMUKO_CPPFLAGS) $(CFLAGS) -c $< -o $@

scheduler.o: scheduler.c
	$(CC) $(SCHEDULER_CPPFLAGS) $(CFLAGS) -c $< -o $@

hello.o: hello.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-$(CLEAN_CMD) $(MMUKO_OBJS) $(SCHEDULER_OBJS) $(HELLO_OBJS) $(TARGET) $(SCHEDULER_TARGET) $(HELLO_TARGET) $(CLEAN_REDIRECT)

test: $(TARGET)
ifeq ($(OS),Windows_NT)
	.\$(TARGET)
else
	./$(TARGET)
endif

run-hello: scheduler hello
ifeq ($(OS),Windows_NT)
	.\$(SCHEDULER_TARGET) --program $(HELLO_TARGET) --interval 5s --repeat 2
else
	./$(SCHEDULER_TARGET) --program ./$(HELLO_TARGET) --interval 5s --repeat 2
endif

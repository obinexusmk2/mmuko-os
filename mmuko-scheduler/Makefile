# MMUKO-SCHEDULER Makefile — module-separated build
# github.com/obinexus/mmuko-os

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2
MMUKO_CPPFLAGS    = -DMMUKO_SCHEDULER_TEST
SCHEDULER_CPPFLAGS = -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L
LDLIBS  = -lm

ifeq ($(OS),Windows_NT)
    EXEEXT      = .exe
    CLEAN_CMD   = del /Q
    CLEAN_REDIR = 2>NUL
else
    EXEEXT      =
    CLEAN_CMD   = rm -f
    CLEAN_REDIR =
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        LDLIBS += -lrt
    endif
endif

TARGET          = mmuko-os$(EXEEXT)
SCHEDULER_TARGET = scheduler$(EXEEXT)
HELLO_TARGET    = hello$(EXEEXT)

# Orchestrator + all separated modules
MMUKO_SRCS = \
    mmuko.c \
    mmuko_scheduler.c \
    src/mmuko_tower.c \
    src/mmuko_process.c \
    src/mmuko_priority.c \
    src/mmuko_governance.c \
    src/mmuko_sched_modes.c \
    src/mmuko_rebalancer.c

MMUKO_OBJS      = $(MMUKO_SRCS:.c=.o)
SCHEDULER_OBJS  = scheduler.o
HELLO_OBJS      = hello.o

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

# Build rules for each module
mmuko.o: mmuko.c mmuko.h
	$(CC) $(MMUKO_CPPFLAGS) $(CFLAGS) -c $< -o $@

mmuko_scheduler.o: mmuko_scheduler.c mmuko.h include/mmuko_tower.h include/mmuko_process.h \
                   include/mmuko_priority.h include/mmuko_governance.h \
                   include/mmuko_sched_modes.h include/mmuko_rebalancer.h
	$(CC) $(MMUKO_CPPFLAGS) $(SCHEDULER_CPPFLAGS) $(CFLAGS) -c $< -o $@

src/mmuko_tower.o: src/mmuko_tower.c include/mmuko_tower.h mmuko.h
	$(CC) $(SCHEDULER_CPPFLAGS) $(CFLAGS) -c $< -o $@

src/mmuko_process.o: src/mmuko_process.c include/mmuko_process.h include/mmuko_tower.h mmuko.h
	$(CC) $(SCHEDULER_CPPFLAGS) $(CFLAGS) -c $< -o $@

src/mmuko_priority.o: src/mmuko_priority.c include/mmuko_priority.h mmuko.h
	$(CC) $(SCHEDULER_CPPFLAGS) $(CFLAGS) -c $< -o $@

src/mmuko_governance.o: src/mmuko_governance.c include/mmuko_governance.h include/mmuko_tower.h mmuko.h
	$(CC) $(SCHEDULER_CPPFLAGS) $(CFLAGS) -c $< -o $@

src/mmuko_sched_modes.o: src/mmuko_sched_modes.c include/mmuko_sched_modes.h \
                          include/mmuko_governance.h include/mmuko_priority.h \
                          include/mmuko_process.h include/mmuko_tower.h mmuko.h
	$(CC) $(SCHEDULER_CPPFLAGS) $(CFLAGS) -c $< -o $@

src/mmuko_rebalancer.o: src/mmuko_rebalancer.c include/mmuko_rebalancer.h \
                         include/mmuko_priority.h mmuko.h
	$(CC) $(SCHEDULER_CPPFLAGS) $(CFLAGS) -c $< -o $@

scheduler.o: scheduler.c
	$(CC) $(SCHEDULER_CPPFLAGS) $(CFLAGS) -c $< -o $@

hello.o: hello.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-$(CLEAN_CMD) $(MMUKO_OBJS) $(SCHEDULER_OBJS) $(HELLO_OBJS) \
	    $(TARGET) $(SCHEDULER_TARGET) $(HELLO_TARGET) $(CLEAN_REDIR)

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

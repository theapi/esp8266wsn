# set the compiler
CC := gcc

# set the compiler flags
CFLAGS := -O0 --std=c11 -Wall

# add header files here
HDRS :=

# add source files here
SRCS := udp.c

# generate names of object files
OBJS := $(SRCS:.c=.o)

# name of executable
EXEC := monitor

# default recipe
all: $(EXEC)

# recipe for building the final executable
$(EXEC): $(OBJS) $(HDRS) Makefile
	$(CC) -o $@ $(OBJS) $(CFLAGS)

# recipe to clean the workspace
clean:
	rm -f $(EXEC) $(OBJS) *.o

.PHONY: all clean

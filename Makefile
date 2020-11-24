COMPILER ?= mpicc

CFLAGS ?= -O2 -Wall -Wno-variadic-macros -pedantic $(GCC_SUPPFLAGS)
LDFLAGS ?= -g 
LDLIBS = -lm

EXECUTABLE = player/parminimaxab

SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:src/%.c=player/%.o)

all: release

release: $(OBJS)
	$(COMPILER) $(LDFLAGS) -o $(EXECUTABLE) $(OBJS) $(LDLIBS) 

player/%.o: src/%.c
	$(COMPILER) $(CFLAGS) -o $@ -c $<

clean:
	rm -f player/*.o
	rm ${EXECUTABLE} 

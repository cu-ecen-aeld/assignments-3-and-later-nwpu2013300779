
$(info CROSS_COMPILE:$(CROSS_COMPILE))
CC=$(CROSS_COMPILE)gcc
SZ=${CROSS_COMPILE}size
$(info CC:${CC})
CFlags=-Wall -O0 -g -std=c11 -D_POSIX_C_SOURCE=200112L

target=aesdsocket

srcs=aesdsocket.c

$(info Sourcs: $(srcs))
objs:=$(srcs:.c=.o)
$(info Objetcs:$(objs))

default:$(target)
all: $(target)

$(target):$(objs)
	@echo "linking ..."
	$(CC) $(CFlags) -o $@ $<
	$(SZ) $(target)
	file $(target) >fileresult.txt

$(objs):$(srcs)
	$(CC) $(CFlags) -c $<



clean:
	rm -rf $(objs) $(target) 1.txt
run:
	./$(target) > 1.txt
gdb:
	gdb ./$(target) -x .gdbinit

CC=${CROSS_COMPILE}gcc
SZ=${CROSS_COMPILE}size
$(info CC:${CC});
CFlags=-Wall -g -std=c11

target=writer

srcs=writer.c

$(info Sourcs: $(srcs))
objs:=$(srcs:.c=.o)
$(info Objetcs:$(objs))

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
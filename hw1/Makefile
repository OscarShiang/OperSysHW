OBJS = generator.o
CFLAG = -std=gnu11 -pthread

all: generator

generator: generator.o
	$(CC) -o $@ $^ $(CFLAG)

%.o: %.c
	$(CC) -c $^ $(CFLAG)

format:
	@clang-format -i *.[ch]

clean:
	$(RM) $(OBJS) generator

.PHONY: format clean

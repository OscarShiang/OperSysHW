CC = gcc
CFLAG = -std=c11 -g -pthread

all: sort generator

sort: sort.o
	$(CC) -o $@ $^ $(CFLAG)

generator: generator.o
	$(CC) -o $@ $^ $(CFLAG)

%.o: %.c
	$(CC) -c $^ $(CFLAG)

test: sort
	@mkdir -p chunk
	@$(RM) chunk/*
	./sort -i input.txt -o output.txt

format:
	@clang-format -i *.[ch]

clean:
	$(RM) sort generator *.o

.PHONY: format test clean

CFLAG = -std=c11 -pthread -g

all: sort generator

sort: sort.o
	$(CC) -o $@ $^ $(CFLAG)

generator: generator.o
	$(CC) -o $@ $^ $(CFLAG)

%.o: %.c
	$(CC) -c $^ $(CFLAG)

test: sort
	@$(RM) chunk/*
	./sort -i input.txt -o output.txt
	@ls -l chunk | wc -l

format:
	@clang-format -i *.[ch]

clean:
	$(RM) sort generator *.o

.PHONY: format test clean

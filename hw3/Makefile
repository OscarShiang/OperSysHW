CC = gcc
CFLAGS = -std=c11 -g -pthread

BIN = kv_store
OBJS = kv_store.o include/utils.o include/bucket.o

all: $(BIN)

ifeq ("$(VERBOSE)", "1")
    Q :=
    VECHO = @true
else
    Q := @
    VECHO = @printf
endif

%.o: %.c
	$(VECHO) "   CC\t$@\n"
	$(Q)$(CC) -c $< $(CFLAGS) -o $@

$(BIN): $(OBJS)
	$(VECHO) "   LD\t$@\n"
	$(Q)$(CC) -o $@ $^ $(CFLAGS)

test: hw3example.input
	./kv_store hw3example.input

debug: $(BIN)
	gdb --args ./kv_store test.input

format: 
	@clang-format -i */*.[ch]

clean: 
	$(RM) $(BIN) $(OBJS)

.PHONY: test format clean

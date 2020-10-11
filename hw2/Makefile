CC = gcc
CFLAGS = -std=c11 -pthread -g

C2J_OBJS = csv2json.o include/utils.o
SPAWN_OBJS = spawner.o include/random.o

all: csv2json spawner

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

csv2json: $(C2J_OBJS)
	$(VECHO) "   LD\t$@\n"
	$(Q)$(CC) -o $@ $^ $(CFLAGS)

spawner: $(SPAWN_OBJS)
	$(VECHO) "   LD\t$@\n"
	$(Q)$(CC) -o $@ $^ $(CFLAGS)

format: 
	@clang-format -i *.[ch]

clean: 
	$(RM) csv2json $(C2J_OBJS) spawner $(SPAWN_OBJS)

.PHONY: format clean

CC = gcc
CFLAGS = -Wall -Wextra
LDLIBS = -lbpf -lX11

out = build

all: kerpad

$(out)/%.bpf.o: bpf/%.bpf.c
	ecc $< -o $(out)

$(out)/%.o: user/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(out)/usrpad.o: $(out)/kerpad.bpf.o

kerpad: $(out)/usrpad.o
	$(CC) $^ -o $@ $(LDLIBS)

clean:
	rm -f $(out)/* kerpar *~ */*~

.PHONY: all clean

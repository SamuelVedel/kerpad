CC = gcc
CFLAGS = -Wall -Wextra
LDLIBS = -lbpf

OUT = build
BPF = bpf
USR = user

all: kerpad

$(OUT)/%.bpf.o: $(BPF)/%.bpf.c
	ecc $< -o $(OUT) -n

$(OUT)/%.o: $(USR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

kerpad: $(OUT)/usrpad.o $(OUT)/kerpad.bpf.o
	$(CC) $< -o $@ $(LDLIBS)

kerpad: $(out)/usrpad.o
	$(CC) $^ -o $@ $(LDLIBS)

clean:
	rm -f $(OUT)/* kerpar *~ */*~

.PHONY: all clean

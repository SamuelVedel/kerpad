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

coorpad: $(BPF)/coorpad.bpf.c $(BPF)/coorpad.h
	cp $(BPF)/coorpad.h $(OUT)
	ecc $^ -o $(OUT)

run_coorpad:
	ecli $(OUT)/package.json

clean:
	rm -f $(OUT)/* kerpar *~ */*~

.PHONY: all clean coorpad run_coorpad

CC = gcc
CFLAGS = -Wall -Wextra
LDLIBS = -lbpf #-lsystemd

OUT = build
BPF = bpf
USR = user

BPF_OBJECT ?= $(shell pwd)/$(OUT)/kerpad.bpf.o

all: kerpad

$(OUT)/%.bpf.o: $(BPF)/%.bpf.c
	ecc $< -o $(OUT) -n

$(OUT)/%.o: $(USR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

kerpad: $(OUT)/usrpad.o $(OUT)/kerpad.bpf.o
	$(CC) $< -o $@ $(LDLIBS)

kerpad.service: kerpad.service.template
	cat kerpad.service.template | sed "s/<bpf-object-path>/$(shell echo $(BPF_OBJECT) | sed 's/\//\\\//g')/" > kerpad.service

install: kerpad kerpad.service
	sudo cp ./kerpad /usr/bin/kerpad
	sudo cp kerpad.service /usr/lib/systemd/system/kerpad.service

coorpad: $(BPF)/coorpad.bpf.c $(BPF)/coorpad.h
	cp $(BPF)/coorpad.h $(OUT)
	ecc $^ -o $(OUT)

run_coorpad: coorpad
	sudo ecli $(OUT)/package.json

clean:
	rm -f $(OUT)/* kerpar kerpad.service *~ */*~

.PHONY: all clean coorpad run_coorpad install

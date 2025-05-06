CC = gcc
CFLAGS = -Wall -Wextra
LDLIBS = 

OUT = build
SRC = src

KERPAD_ARGS ?=

all: kerpad

$(OUT)/%.bpf.o: $(BPF)/%.bpf.c
	ecc $< -o $(OUT) -n

$(OUT)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# dependencies
$(OUT)/touchpad.o: $(SRC)/touchpad.h $(SRC)/util.h
$(OUT)/mouse.o: $(SRC)/touchpad.h $(SRC)/util.h
$(OUT)/main.o: $(SRC)/touchpad.h $(SRC)/util.h

kerpad: $(OUT)/main.o $(OUT)/touchpad.o $(OUT)/mouse.o
	$(CC) $^ -o $@ $(LDLIBS)

kerpad.service: kerpad.service.template
	cat kerpad.service.template | sed "s/<args>/$(shell echo $(KERPAD_ARGS) | sed 's/\//\\\//g')/" > kerpad.service

install: kerpad kerpad.service
	sudo cp ./kerpad /usr/bin/kerpad
	sudo cp kerpad.service /usr/lib/systemd/system/kerpad.service

clean:
	rm -f $(OUT)/* kerpad kerpad.service *~ */*~

.PHONY: all clean coorpad run_coorpad install

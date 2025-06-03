CC = gcc
CFLAGS = -Wall -Wextra
LDLIBS = 

OUT = build
SRC = src

KERPAD_ARGS ?=

PANDOC ?= $(shell which pandoc 2> /dev/null)

ifeq ($(PANDOC),)
all: kerpad
else
all: kerpad kerpad.1.gz
endif


$(OUT)/%.o: $(SRC)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# dependencies
$(OUT)/touchpad.o: $(SRC)/touchpad.h $(SRC)/util.h
$(OUT)/mouse.o: $(SRC)/touchpad.h $(SRC)/util.h
$(OUT)/main.o: $(SRC)/touchpad.h $(SRC)/util.h
$(OUT)/util.o: $(SRC)/util.h

kerpad: $(OUT)/main.o $(OUT)/touchpad.o $(OUT)/mouse.o $(OUT)/util.o
	$(CC) $^ -o $@ $(LDLIBS)

kerpad.service: kerpad.service.template
	cat kerpad.service.template | sed "s/<args>/$(shell echo $(KERPAD_ARGS) | sed 's/\//\\\//g')/" > kerpad.service

kerpad.1: kerpad.1.md
	pandoc $< -s -t man -o $@

kerpad.1.gz: kerpad.1
	gzip -kf $<

ifeq ($(PANDOC),)
install: kerpad kerpad.service
else
install: kerpad kerpad.service kerpad.1.gz
endif
	sudo cp ./kerpad /usr/bin/kerpad
	sudo cp kerpad.service /usr/lib/systemd/system/kerpad.service
ifneq ($(PANDOC),)
	sudo cp kerpad.1.gz /usr/share/man/man1/kerpad.1.gz
endif

uninstall:
	sudo rm -f /usr/bin/kerpad
	sudo rm -f /usr/lib/systemd/system/kerpad.service
	sudo rm -f /usr/share/man/man1/kerpad.1.gz

clean:
	rm -f $(OUT)/* kerpad kerpad.service *~ */*~
ifneq ($(PANDOC),)
	rm -f kerpad.1 kerpad.1.gz
endif

.PHONY: all clean install uninstall

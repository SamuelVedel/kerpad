CC = gcc
CFLAGS = -Wall -Wextra
LDLIBS = 

OUT = build
SRC = src

KERPAD_INSTALL = /usr/bin/kerpad
SERVICE_INSTALL = /usr/lib/systemd/system/kerpad.service
MAN_INSTALL = /usr/share/man/man1/kerpad.1.gz

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

$(KERPAD_INSTALL): kerpad
	sudo cp $< $@

$(SERVICE_INSTALL): kerpad.service
	sudo cp $< $@

$(MAN_INSTALL): kerpad.1.gz
	sudo cp $< $@

install_kerpad: $(KERPAD_INSTALL)
install_service: $(SERVICE_INSTALL)
install_man: $(MAN_INSTALL)

ifeq ($(PANDOC),)
install: $(KERPAD_INSTALL) $(SERVICE_INSTALL)
else
install: $(KERPAD_INSTALL) $(SERVICE_INSTALL) $(MAN_INSTALL)
endif

uninstall:
	sudo rm -f $(KERPAD_INSTALL)
	sudo rm -f $(SERVICE_INSTALL)
	sudo rm -f $(MAN_INSTALL)

clean:
	rm -f $(OUT)/* kerpad kerpad.service *~ */*~ kerpad.1 kerpad.1.gz

.PHONY: all clean install uninstall install_kerpad install_service install_man

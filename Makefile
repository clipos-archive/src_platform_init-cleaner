# SPDX-License-Identifier: LGPL-2.1-or-later
# Copyright © 2013-2018 ANSSI. All Rights Reserved.
CC = gcc
LDLIBS = -lclip

.PHONY: all mrproper

all: init-cleaner

init-cleaner: init-cleaner.c init-cleaner.h
	$(CC) $(LDLIBS) $(CFLAGS) -o $@ $<

mrproper:
	rm init-cleaner

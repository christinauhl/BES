#!/bin/bash

#ToDo: make makefile

gcc52 myfind.c -o myfind -DDEBUG -Wall -Werror -Wextra -Wstrict-prototypes -Wformat=2 -pedantic -fno-common -ftrapv -O3 -g -std=gnu11 && /var/tmp/test-find/full

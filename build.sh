#!/bin/sh

set -xe

cc -o main main-asm.c rb-tree.c -std=c89 -pedantic -Wall -Wextra -pipe -ggdb -fomit-frame-pointer -O3 -Wno-clobbered

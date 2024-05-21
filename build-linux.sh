#!/bin/sh

set -xe

gcc main.c -o wireworld -Wall -Wextra -pedantic -O3 -I ./raylib/raylib-5.0_linux_amd64/include/ -L ./raylib/raylib-5.0_linux_amd64/lib/ -l:libraylib.a -lm

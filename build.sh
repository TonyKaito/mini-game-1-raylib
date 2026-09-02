#!/bin/sh

set -xe

# For Windows
CFLAGS="-Wall -Wextra -I./raylib/include"
LIBS="-L./raylib/lib -lraylib -lopengl32 -lgdi32 -lwinmm"
	
gcc $CFLAGS main.c -o main.exe $LIBS
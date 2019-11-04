#
# Makefile
#
# Copyright (C) 2019  Patrick "p2k" Schneider
#
# This file is part of SoloMIPS.
#
# SoloMIPS is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# SoloMIPS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with SoloMIPS.  If not, see <http://www.gnu.org/licenses/>.
#

ifeq ($(OS),Windows_NT)
BIN_SUFFIX=.exe
else
BIN_SUFFIX=
endif

TOOLCHAIN_GCC=../toolchain/bin/mips-linux-gnu-gcc$(BIN_SUFFIX)

LD=../build/solomips-ld

ifneq (,$(wildcard $(TOOLCHAIN_GCC)))
CC=$(TOOLCHAIN_GCC)
else
CC?=mips-linux-gnu-gcc
endif

.PHONY: all docker

all: test1.bin test2.bin test3.bin test4.bin test5.bin

%.bin: %.o
	$(LD) -o $@ $<

%.o: %.S
	$(CC) -march=r3000 -mfp32 -mgp32 -mno-plt -c -o $@ $<

%.o: %.c
	$(CC) -march=r3000 -mfp32 -mgp32 -mno-plt -c -o $@ $<

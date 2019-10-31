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

.PHONY: all docker

all: test1.S.o test2.S.o test3.c.o test4.S.o

docker:
	docker build -t gcc-mips .

%.S.o: %.S
	docker run --rm -v $(PWD):/src gcc-mips mips-linux-gnu-gcc -march=r3000 -mfp32 -mgp32 -mno-plt -c -o $@ $<

%.c.o: %.c
	docker run --rm -v $(PWD):/src gcc-mips mips-linux-gnu-gcc -march=r3000 -mfp32 -mgp32 -mno-plt -O2 -c -o $@ $<

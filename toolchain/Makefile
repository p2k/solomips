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

GCC_VSN=9.2.0
BINUTILS_VSN=2.33.1
TARGET=mips-unknown-linux-gnu
PROGRAM_PREFIX=mips-linux-gnu-
PREFIX=$(PWD)

ifeq ($(OS),Windows_NT)
BIN_SUFFIX=.exe
else
BIN_SUFFIX=
endif

.PHONY: all clean buildclean distclean

all: bin/$(PROGRAM_PREFIX)gcc$(BIN_SUFFIX)

clean:
	rm -rf bin include lib libexec $(TARGET) share

buildclean:
	rm -rf build-binutils build-gcc

distclean: buildclean
	rm -rf binutils-$(BINUTILS_VSN) gcc-$(GCC_VSN) binutils-$(BINUTILS_VSN).tar.xz gcc-$(GCC_VSN).tar.xz

bin/$(PROGRAM_PREFIX)gcc$(BIN_SUFFIX): build-gcc/gcc/libgcc.a
	cd build-gcc && make install

build-gcc/gcc/libgcc.a: build-gcc/Makefile
	cd build-gcc && make -j4

build-gcc/Makefile: bin/$(PROGRAM_PREFIX)ld$(BIN_SUFFIX) gcc-$(GCC_VSN)/prerequisites_downloaded
	mkdir -p build-gcc
	cd build-gcc \
		&& ../gcc-$(GCC_VSN)/configure \
			--target=$(TARGET) --prefix=$(PREFIX) --program-prefix=$(PROGRAM_PREFIX) \
			--enable-languages=c --without-headers \
  		--with-gnu-ld --with-gnu-as \
  		--disable-shared --disable-threads \
  		--disable-libmudflap --disable-libgomp \
  		--disable-libssp --disable-libquadmath \
  		--disable-libatomic --disable-multilib

gcc-$(GCC_VSN)/prerequisites_downloaded: gcc-$(GCC_VSN)/configure gcc-$(GCC_VSN)/contrib/download_prerequisites
	cd gcc-$(GCC_VSN) && ./contrib/download_prerequisites
	touch $@

gcc-$(GCC_VSN)/configure: gcc-$(GCC_VSN).tar.xz
	tar xJf $< && touch $@

gcc-$(GCC_VSN).tar.xz:
	wget ftp://ftp.gnu.org/gnu/gcc/gcc-$(GCC_VSN)/gcc-$(GCC_VSN).tar.xz

bin/$(PROGRAM_PREFIX)ld$(BIN_SUFFIX): build-binutils/ld/ld-new$(BIN_SUFFIX)
	cd build-binutils && make install

build-binutils/ld/ld-new$(BIN_SUFFIX): build-binutils/Makefile
	cd build-binutils && make -j4

build-binutils/Makefile: binutils-$(BINUTILS_VSN)/configure
	mkdir -p build-binutils
	cd build-binutils \
		&& ../binutils-$(BINUTILS_VSN)/configure \
			--target=$(TARGET) --prefix=$(PREFIX) --program-prefix=$(PROGRAM_PREFIX)

binutils-$(BINUTILS_VSN)/configure: binutils-$(BINUTILS_VSN).tar.xz
	tar xJf $< && touch $@

binutils-$(BINUTILS_VSN).tar.xz:
	wget ftp://ftp.gnu.org/gnu/binutils/binutils-$(BINUTILS_VSN).tar.xz

dte
===

Dexterous Text Editor

Copyright 2010 Timo Hirvonen <tihirvon@gmail.com>


Introduction
------------

dte is a small and easy to use text editor. Colors and bindings can be
fully customized to your liking.

It has some features useful to programmers, like ctags support and it
can parse compiler errors, but it does not aim to become an IDE.


Installation
------------

The only dependencies are libc and ncurses.

To compile this program you need GNU make and a modern C-compiler
(tested with gcc and clang).

You need to specify all options for both `make` and `make install`.
Alternatively you can put your build options into a `Config.mk` file.

	make prefix=$HOME
	make prefix=$HOME install

The default prefix is `/usr/local` and `DESTDIR` works as usual. See the
top of the Makefile for more information.

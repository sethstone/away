#!/bin/sh

make clean
rm `find . -name Makefile`
rm `find . -name Makefile.in`
rm COPYING INSTALL aclocal.m4 config.* configure install-sh missing mkinstalldirs stamp-h*


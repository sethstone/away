#!/bin/sh

if [ -f Makefile ];then
  make clean
fi
rm `find . -name Makefile`
rm `find . -name Makefile.in`
rm COPYING INSTALL aclocal.m4 config.* configure install-sh missing mkinstalldirs stamp-h*


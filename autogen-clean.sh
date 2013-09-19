#!/bin/sh

# Copyright (C) <2011> Christoph Reiter <christoph.reiter@gmx.at>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

if [ -f Makefile ]; then
  echo "Making make distclean..."
  make distclean
fi
echo "Removing autogenned files..."
rm -f depcomp install-sh libtool ltmain.sh missing INSTALL stamp-h1 aclocal.m4
rm -f config.guess config.h config.h.in config.log config.status config.sub
rm -f configure config.h.in~
rm -f Makefile Makefile.in 
rm -f src/Makefile src/Makefile.in
rm -rf src/.deps
rm -rf autom4te.cache
echo "Done."

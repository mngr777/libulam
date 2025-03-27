#!/bin/sh

mkdir -p autoconf-aux m4
touch AUTHORS ChangeLog INSTALL NEWS README COPYING

aclocal
libtoolize
automake --add-missing
autoconf

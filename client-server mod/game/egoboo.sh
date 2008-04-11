#!/bin/sh

# exit on any error
set -e

if [ ! -d ~/.egoboo ]; then
  mkdir ~/.egoboo
  cp -a /usr/share/egoboo/setup.txt /usr/share/egoboo/controls.txt \
    /usr/share/egoboo/players ~/.egoboo
  ln -s /usr/share/egoboo/basicdat /usr/share/egoboo/modules ~/.egoboo
fi

cd ~/.egoboo

exec /usr/libexec/egoboo "$@"

#!/bin/sh
make clean
make
sync
echo kopiruji do /usr/local/bin
cp -f rc-window-to-monitor /usr/local/bin
sync
echo odstranuji docasne soubory
make clean
echo hotovo



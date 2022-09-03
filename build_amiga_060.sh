#!/bin/sh
make CC="m68k-amigaos-gcc -noixemul -m68060 -m68881 -fbbb=- -fno-peephole2 -ffast-math" CXX="m68k-amigaos-g++ -noixemul -m68060 -m68881 -fbbb=- -fno-peephole2 -ffast-math" AS="vasm -Fhunk -quiet -phxass -m68060 -DM68060" AR=m68k-amigaos-ar RANLIB=m68k-amigaos-ranlib PLATFORM=AMIGA AMISUFFIX=".060" $*

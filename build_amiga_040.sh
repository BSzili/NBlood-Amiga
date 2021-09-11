#!/bin/sh
make CC="m68k-amigaos-gcc -noixemul -m68040 -m68881 -fbbb=- -fno-peephole2" CXX="m68k-amigaos-g++ -noixemul -m68040 -m68881 -fbbb=- -fno-peephole2" AS="vasm -Fhunk -quiet -phxass -m68040" AR=m68k-amigaos-ar RANLIB=m68k-amigaos-ranlib PLATFORM=AMIGA AMISUFFIX=".040" $*

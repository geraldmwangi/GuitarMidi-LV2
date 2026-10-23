# ----------------------------------------------------------------------------
#
#  Copyright (C) 2020 Fons Adriaensen <fons@linuxaudio.org>
#    
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# ----------------------------------------------------------------------------


# Compilation flags
#
CXXFLAGS += -O2 -Wall


# Targets
#
all:	speedtest vspeedtest upstest jackproc


speedtest:	speedtest.o
	g++ -o $@ speedtest.o -lzita-resampler


vspeedtest:	vspeedtest.o
	g++ -o $@ vspeedtest.o -lzita-resampler


upstest:	upstest.o
	g++ -o $@ upstest.o -lzita-resampler


jackproc:	jackproc.o lfqueue.o
	g++ -o $@ jackproc.o lfqueue.o -lzita-resampler -ljack


clean:
	/bin/rm -f *.o *.u *.so *~ speedtest vspeedtest upstest jackproc zz*


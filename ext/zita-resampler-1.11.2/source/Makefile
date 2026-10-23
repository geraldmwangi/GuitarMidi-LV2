# ----------------------------------------------------------------------------
#
#  Copyright (C) 2006-2023 Fons Adriaensen <fons@linuxaudio.org>
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


# Modify as required.
#
SUFFIX := $(shell uname -m | sed -e 's/^unknown/$//' -e 's/^i.86/$//' -e 's/^x86_64/$/64/')
PREFIX ?= /usr/local
INCDIR ?= $(PREFIX)/include
LIBDIR ?= $(PREFIX)/lib$(SUFFIX)

MAJVERS = 1
MINVERS = 11.2
VERSION = $(MAJVERS).$(MINVERS)
DISTDIR = zita-resampler-$(VERSION)

CPPFLAGS += -I. -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS
CPPFLAGS += -DENABLE_SSE2
#CPPFLAGS += -DENABLE_NEON  
CXXFLAGS += -Wall -fPIC -O2 -ffast-math
CXXFLAGS += -march=native
LDFLAGS += 
LDLIBS +=


ZITA-RESAMPLER_SO = libzita-resampler.so
ZITA-RESAMPLER_MAJ = $(ZITA-RESAMPLER_SO).$(MAJVERS)
ZITA-RESAMPLER_MIN = $(ZITA-RESAMPLER_MAJ).$(MINVERS)
ZITA-RESAMPLER_DEP =
ZITA-RESAMPLER_O = resampler.o vresampler.o resampler-table.o cresampler.o
ZITA-RESAMPLER_H = zita-resampler/resampler.h zita-resampler/resampler-table.h \
	zita-resampler/vresampler.h zita-resampler/cresampler.h


$(ZITA-RESAMPLER_MIN): $(ZITA-RESAMPLER_O)
	$(CXX) -shared $(LDFLAGS) -Wl,-soname,$(ZITA-RESAMPLER_MAJ) -o $(ZITA-RESAMPLER_MIN) $(ZITA-RESAMPLER_O) $(ZITA-RESAMPLER_DEP)

$(ZITA-RESAMPLER_O):	$(ZITA-RESAMPLER_H)


install:	$(ZITA-RESAMPLER_MIN)
	install -d $(DESTDIR)$(INCDIR)/zita-resampler
	install -d $(DESTDIR)$(LIBDIR)
	install -m 644 $(ZITA-RESAMPLER_H)   $(DESTDIR)$(INCDIR)/zita-resampler
	install -m 755 $(ZITA-RESAMPLER_MIN) $(DESTDIR)$(LIBDIR)
	ln -sf $(ZITA-RESAMPLER_MIN) $(DESTDIR)$(LIBDIR)/$(ZITA-RESAMPLER_SO)
	ln -sf $(ZITA-RESAMPLER_MIN) $(DESTDIR)$(LIBDIR)/$(ZITA-RESAMPLER_MAJ)

uninstall:
	/bin/rm -rf $(DESTDIR)$(INCDIR)/zita-resampler
	/bin/rm -rf $(DESTDIR)$(LIBDIR)/libzita-resampler*

clean:
	/bin/rm -rf *~ *.o *.a *.d *.so.* zita-resampler/*~


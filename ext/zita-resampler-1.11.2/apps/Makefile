# ----------------------------------------------------------------------------
#
#  Copyright (C) 2006-2022 Fons Adriaensen <fons@linuxaudio.org>
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
#  along with this program.  If not, see <http:#www.gnu.org/licenses/>.
#
# ----------------------------------------------------------------------------


PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= /usr/share/man/man1

VERSION = 1.10.1
CPPFLAGS += -MMD -MP -DVERSION=\"$(VERSION)\"
CXXFLAGS += -O2  -ffast-math -Wall
CXXFLAGS += -march=native


all:	zresample zretune zresample.1.gz zretune.1.gz


ZRESAMPLE_O = zresample.o audiofile.o dither.o
zresample:	LDLIBS += -lzita-resampler -lsndfile -lrt
zresample:	$(ZRESAMPLE_O)
	$(CXX) $(LDFLAGS) -o $@ $(ZRESAMPLE_O) $(LDLIBS)
$(ZRESAMPLE_O):
-include $(ZRESAMPLE_O:%.o=%.d)


ZRETUNE_O = zretune.o audiofile.o dither.o
zretune:	LDLIBS += -lzita-resampler -lsndfile -lrt
zretune:	$(ZRETUNE_O)
	$(CXX) $(LDFLAGS) -o $@ $(ZRETUNE_O) $(LDLIBS)
$(ZRETUNE_O):
-include $(ZRETUNE_O:%.o=%.d)


zresample.1.gz:	zresample.1
	gzip -c zresample.1 > zresample.1.gz

zretune.1.gz:	zretune.1
	gzip -c zretune.1 > zretune.1.gz


install:	all
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(MANDIR)
	install -m 755 zresample $(DESTDIR)$(BINDIR)
	install -m 755 zretune $(DESTDIR)$(BINDIR)
	install -m 644 zresample.1.gz $(DESTDIR)$(MANDIR)
	install -m 644 zretune.1.gz $(DESTDIR)$(MANDIR)

uninstall: 
	/bin/rm -f $(DESTDIR)$(BINDIR)/zresample
	/bin/rm -f $(DESTDIR)$(BINDIR)/zretune
	/bin/rm -f $(DESTDIR)$(MANDIR)/zresample.1.gz
	/bin/rm -f $(DESTDIR)$(MANDIR)/zretune.1.gz

clean:
	/bin/rm -f *~ *.o *.a *.d *.so *.gz zresample zretune


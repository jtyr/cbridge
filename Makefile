####
## Author: Jiri Tyr (c) 2009
####

NAME = cbridge
DESTDIR =
VERSION = 0.4
DISTVNAME = $(NAME)-$(VERSION)

BINDIR = $(DESTDIR)/usr/bin

PERLRUN = perl
DIST_CP = best
TAR = tar
TARFLAGS = cvf
COMPRESS = gzip --best
RM_RF = rm -rf
RM_F = rm -f
TEST = test
ECHO = echo
CP = cp
MKDIR_P = mkdir -p
CC = gcc
OPTFLAGS = -O2 -Wall


.PHONY: all install uninstall manifest dist clean


all :
	$(CC) $(OPTFLAGS) -o ./src/$(NAME) ./src/$(NAME).c


install :
	$(MKDIR_P) \
	$(BINDIR)
	$(CP) ./src/$(NAME) $(BINDIR)


uninstall :
	$(RM_RF) $(BINDIR)/$(NAME)


manifest :
	$(PERLRUN) "-MExtUtils::Manifest=mkmanifest" -e mkmanifest
	echo 'Makefile' >> ./MANIFEST


dist : clean create_distdir
	$(TEST) -e MANIFEST
	$(TAR) $(TARFLAGS) $(DISTVNAME).tar $(DISTVNAME)
	$(RM_RF) $(DISTVNAME)
	$(COMPRESS) $(DISTVNAME).tar

create_distdir :
	$(RM_RF) $(DISTVNAME)
	$(PERLRUN) "-MExtUtils::Manifest=manicopy,maniread" \
		-e "manicopy(maniread(),'$(DISTVNAME)', '$(DIST_CP)');"


clean :
	$(RM_RF) $(DISTVNAME)*
	$(RM_F) ./src/cbridge
	$(RM_F) ./src/listener

########################################################################
#                                                                      #
#    *****                       *****                                 #
#      *****                   *****                                   #
#        *****               *****                                     #
#          *****           *****                                       #
#  ***************       ***************                               #
#  *****************   *****************                               #
#  ***************       ***************                               #
#          *****           *****           TheNetNode                  #
#        *****               *****         Portable                    #
#      *****                   *****       Network                     #
#    *****                       *****     Software                    #
#                                                                      #
# File contrib/pfhadd/makefile (maintained by: DF6LN)                  #
#                                                                      #
# Copyright (C) 1998 - 2004 NORD><LINK e.V. Braunschweig               #
#                                                                      #
# This program is free software; you can redistribute it and/or modify #
# it under the terms of the NORD><LINK ALAS (Allgemeine Lizenz fuer    #
# Amateurfunk Software) as published by Hans Georg Giese (DF2AU)       #
# on 13/Oct/1992; either version 1, or (at your option) any later      #
# version.                                                             #
#                                                                      #
# This program is distributed WITHOUT ANY WARRANTY only for further    #
# development and learning purposes. See the ALAS (Allgemeine Lizenz   #
# fuer Amateurfunk Software).                                          #
#                                                                      #
# You should have received a copy of the NORD><LINK ALAS (Allgemeine   #
# Lizenz fuer Amateurfunk Software) along with this program; if not,   #
# write to NORD><LINK e.V., Hinter dem Berge 5, D-38108 Braunschweig   #
#                                                                      #
# Dieses Programm ist PUBLIC DOMAIN, mit den Einschraenkungen durch    #
# die ALAS (Allgemeine Lizenz fuer Amateurfunk Software), entweder     #
# Version 1, veroeffentlicht von Hans Georg Giese (DF2AU),             #
# am 13.Oct.1992, oder (wenn gewuenscht) jede spaetere Version.        #
#                                                                      #
# Dieses Programm wird unter Haftungsausschluss vertrieben, aus-       #
# schliesslich fuer Weiterentwicklungs- und Lehrzwecke. Naeheres       #
# koennen Sie der ALAS (Allgemeine Lizenz fuer Amateurfunk Software)   #
# entnehmen.                                                           #
#                                                                      #
# Sollte dieser Software keine ALAS (Allgemeine Lizenz fuer Amateur-   #
# funk Software) beigelegen haben, wenden Sie sich bitte an            #
# NORD><LINK e.V., Hinter dem Berge 5, D-38108 Braunschweig            #
#                                                                      #
########################################################################

#
# Makefile fuer pfhadd
#

PFH		:= contrib/pfhadd
PFHFILES	:= $(PFH)/pfhadd.c $(PFH)/makefile

.PHONY:		pfhadd pfhclean

all:		pfhadd
clean:		pfhclean
pfhadd:		$(BIN)/pfhadd

ifeq ($(SYSTEM), LINUX)
.PHONY:		pfhdist pfhinstall pfhgo32

bigdist:	pfhdist
install:	pfhinstall
go32:		pfhgo32
pfhgo32:	$(BIN)/pfhgo32

$(BIN)/pfhadd:	$(PFHFILES) $(CLEANER)
		$(CLEANER) $(PFH)/pfhadd.c
		$(CC) $(CFLAGS_LIN) $(PFH)/pfhadd.c $(LDFLAGS_LIN) -o $@
		sync
		echo $@ done.

ifeq ($(MIPS), YES)
$(BIN)/pfhadd:
		echo Program \'pfhadd\' skipped since PACSAT is disabled for MIPS devices !
else
$(BIN)/pfhgo32:	$(PFHFILES) $(CLEANER)
		$(CLEANER) $(PFH)/pfhadd.c
		$(CC_GO32) $(CFLAGS_GO32) $(PFH)/pfhadd.c -o $@
		chmod 644 $@
		sync
		echo $@ done.
endif

pfhclean:
		rm -f $(BIN)/pfhadd $(BIN)/pfhgo32{,.exe} $(PFH)/*~

pfhdist:
		cp --parents $(PFHFILES) $(DISTDIR)

pfhinstall:
		echo $@
		install -m 0700 $(BIN)/pfhadd $(TNNPACSAT)pfhadd
else
#
# Hier (fast) das gleiche fuer Dose
#
$(BIN)/pfhadd:	$(PFHFILES) $(CLEANER)
		$(CLEANER) $(PFH)/pfhadd.c
		$(CC) $(CFLAGS_GO32) $(PFH)/pfhadd.c -o $@
		echo $@ done.

pfhclean:
		erase $(subst /,\\,$(BIN))\pfhadd
		erase $(subst /,\\,$(BIN))\pfhadd.exe
		erase $(subst /,\\,$(PFH))\*.bak
endif

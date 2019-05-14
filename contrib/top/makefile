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
# File contrib/pfhadd/makefile (maintained by: DG8BR)                  #
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

TOP		:= contrib/top
TOPFILES	:= $(TOP)/top_gnu.c $(TOP)/makefile

.PHONY:		top topclean

all:		top
clean:		topclean
top:		$(BIN)/top

ifeq ($(SYSTEM), LINUX)
.PHONY:		topdist topinstall topgo32

bigdist:	topdist
install:	topinstall
go32:		topgo32
topgo32:	$(BIN)/topgo32

$(BIN)/top:	$(TOPFILES) $(CLEANER)
		$(CLEANER) $(TOP)/top_gnu.c
		$(CC) $(CFLAGS_LIN) $(TOP)/top_gnu.c $(LDFLAGS_LIN) -o $@
		sync
		echo $@ done.

$(BIN)/topgo32:	$(TOPFILES) $(CLEANER)
		$(CLEANER) $(TOP)/top_gnu.c
		$(CC_GO32) $(CFLAGS_GO32) $(TOP)/top_gnu.c -o $@
		chmod 644 $@
		sync
		echo $@ done.

topclean:
		rm -f $(BIN)/top $(BIN)/topgo32{,.exe} $(TOP)/*~

topdist:
		cp --parents $(TOPFILES) $(DISTDIR)

topinstall:
		echo $@
		install -m 0700 $(BIN)/top $(TNNUSEREXE)/top
		cp $(TOP)/config.top $(TNNDIR)/config.top
else
#
# Hier (fast) das gleiche fuer Dose
#
$(BIN)/top:	$(TOPFILES) $(CLEANER)
		$(CLEANER) $(TOP)/top_gnu.c
		$(CC) $(CFLAGS_GO32) $(TOP)/top_gnu.c -o $@
		echo $@ done.

topclean:
		erase $(subst /,\\,$(BIN))\top
		erase $(subst /,\\,$(BIN))\top.exe
		erase $(subst /,\\,$(TOP))\*.bak
endif

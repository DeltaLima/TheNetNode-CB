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
# File contrib/onlhelp/makefile (maintained by: DF6LN)                 #
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
# Makefile fuer help
#
ifeq ($(SYSTEM), LINUX)
HELP		:= contrib/onlhelp
HELPFILES	:= $(HELP)/oh.c $(HELP)/makefile
HELPTEXT	:= $(HELP)/ohs.txt $(HELP)/ohu.txt

.PHONY:		help helpclean helpdist helpinstall

all:		help
clean:		helpclean
bigdist:	helpdist
install:	helpinstall

help:		$(BIN)/help

$(BIN)/help:	$(HELP)/help
		cp $(HELP)/help $@
		echo $@ done.

$(HELP)/help:	$(HELPFILES) $(CLEANER)
		$(CLEANER) $(HELP)/oh.c
		$(CC) -pipe $(HELP)/oh.c $(LDFLAGS_LIN) -o $@
		sync
		echo $@ done.

helpclean:
		rm -f $(BIN)/help $(HELP)/*~ $(HELP)/help

helpdist:
		cp --parents $(HELPFILES) $(HELPTEXT) $(DISTDIR)

helpinstall:	$(BIN)/help
		echo $@
		install -m 0700 $(BIN)/help $(TNNUSEREXE)help
		cd $(TNNSYSEXE); ln -sf $(TNNUSEREXE)help syshelp
		cp $(HELPTEXT) $(TNNDIR)
endif

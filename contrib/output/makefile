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
# File contrib/output/makefile (maintained by: DF6LN)                  #
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
# Makefile fuer output
#
#

OUTPUT			:= contrib/output
OUTPUTFILES		:= $(OUTPUT)/output.c $(OUTPUT)/makefile

.PHONY:			output outputclean

all:			output
clean:			outputclean

output:			$(BIN)/output


# erstmal fuer Linux
ifeq ($(SYSTEM), LINUX)
.PHONY:			outputdist outputinstall outputgo32

bigdist:		outputdist
install:		outputinstall
go32:			outputgo32
outputgo32:		$(BIN)/outputgo32

# hier wird tatsaechlich uebersetzt
# (oder auch nicht, MeshCube und WRT54G haben keinen par. Port !)
ifeq ($(MIPS), YES)
$(BIN)/output:
			echo Program \'output\' skipped, not available for your system yet !
else
$(BIN)/output:		$(OUTPUTFILES) $(CLEANER)
			$(CLEANER) $(OUTPUT)/output.c
			$(CC) $(CFLAGS_LIN) -O2 $(OUTPUT)/output.c $(LDFLAGS_LIN) -o $@
			sync
			echo $@ done.
endif

$(BIN)/outputgo32:	$(OUTPUTFILES) $(CLEANER)
			$(CLEANER) $(OUTPUT)/output.c
			$(CC_GO32) $(CFLAGS_GO32) $(OUTPUT)/output.c -o $@
			chmod 644 $@
			sync
			echo $@ done.

outputclean:
			rm -f $(BIN)/output $(BIN)/outputgo32{,.exe} $(OUTPUT)/*~

outputdist:
			cp --parents $(OUTPUTFILES) $(DISTDIR)

outputinstall:
			install -m 0700 $(BIN)/output $(TNNSYSEXE)/output
else
#
# Hier (fast) das gleiche fuer Dose
#
$(BIN)/output:		$(OUTPUTFILES) $(CLEANER)
			$(CLEANER) $(OUTPUT)/output.c
			$(CC) $(CFLAGS_GO32) $(OUTPUT)/output.c -o $@
			echo $@ done.

outputclean:
			erase $(subst /,\\,$(BIN))\output
			erase $(subst /,\\,$(BIN))\output.exe
			erase $(subst /,\\,$(OUTPUT))\*.bak
endif

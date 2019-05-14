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
# File contrib/makefile (maintained by: DF6LN)                         #
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
# Geruest fuer ein makefile in contrib/programmname mit dem das Programm
# "prg" uebersetzt werden soll, das im TNN-Unterverzeichnis userexe laufen
# soll.
#
# Das Programm soll grundsaetzlich in das Verzeichnis $(BIN) uebersetzt
# werden. Die in den anderen Makefiles verwendeten Targets duerfen nicht
# noch einmal verwendet werden. Daher werden fuer "clean", "dist", etc.
# jeweils zusaetzliche abhaengige Targets definiert, unter denen dann die
# tatsaechliche Ausfuehrung stattfindet. Folgende Targets sollten
# beruecksichtigt werden:
#
# prg           - (ist durch den Programmnamen zu ersetzen) uebersetzen des
#                 Programms in das Verzeichnis $(BIN). Evtl. zusaetzlich
#                 generierte Dateien sollten im Verzeichnis des Programms
#                 bleiben (contrib/programmname). Das Programm kann fuer LINUX
#                 und / oder GO32 uebersetzt werden, oder auch fuer GO32 unter
#                 DOS.
#
# all           - hier wird prg zusaetzlich eingetragen, so dass es bei einem
#                 Befehl "make" ohne zusaetzliche Parameter mit uebersetzt
#                 wird.
#
# clean         - loeschen des compilierten Programms in $(BIN) und aller
#                 anderen mit den normalen Libraries neu generierbaren Files.
#                 Wenn spezielle Libraries benoetigt werden (z.b. von p2c),
#                 sind besondere Massnahmen zu ueberlegen, damit nicht etwas
#                 falsches geloescht wird (s. contrib/onlhelp/makefile).
#
# bigdist       - (nur bei Linux) kopieren der fuer das Uebersetzen des
#                 Programms benoetigten Dateien in das Verzeichnis
#                 $(DISTDIR)/contrib/programmname - dazu gehoeren die *.c und
#                 *.h Dateien sowie das Makefile und evtl. weitere Dateien wie
#                 z.B. Hilfetexte.
#
# install       - (nur fuer Linux) kopieren des fertigen Programms sowie der
#                 ggf. benoetigten weiteren Dateien (Hilfe etc.) in die
#                 passenden Verzeichnisse $(TNNDIR), $(TNNUSEREXE),
#                 $(TNNSYSEXE), usw. - abhaengig von der gewuenschten Funktion
#
# go32          - (nur fuer Linux) wenn das Programm in einer GO32-Version
#                 unter Linux uebersetzt werden kann, sollte es hier auch mit
#                 eingetragen werden, damit es bei einem Befehl "make go32"
#                 mit uebersetzt wird.
#

PRGNAM		:= contrib/programmname
PRGFILES	:= $(PRGNAM)/prg.c $(PRGNAM)/makefile

# Phony Targets eintragen, die fuer LINUX und DOS gueltig sind
.PHONY:		prg prgclean

# Lokale Erweiterung eintragen fuer "all" und "clean"
all:		prg
clean:		prgclean

# das wird bei "make prg" tatsaechlich uebersetzt:
prg:		$(BIN)/prg

# nun wirds Systemabhaengig - erstmal Linux
ifeq ($(SYSTEM), LINUX)
# Phony Targets eintragen, die nur fuer LINUX gueltig sind
.PHONY:		prgdist prginstall prggo32

# Lokale Erweiterung fuer "dist" eintragen
bigdist:	prgdist
# Lokale Erweiterung fuer "install" eintragen
install:	prginstall
# Lokale Erweiterung fuer "go32" eintragen
go32:		prggo32
# das wird bei "make prggo32" tatsaechlich uebersetzt:
prggo32:	$(BIN)/prggo32

# hier wird tatsaechlich uebersetzt
$(BIN)/prg:	$(PRGFILES) $(CLEANER)
		$(CLEANER) $(PRG)/prg.c
		$(CC) $(CFLAGS_LIN) $(PRG)/prg.c -o $@
		sync
		echo $@ done.

$(BIN)/prggo32:	$(PRGFILES) $(CLEANER)
		$(CLEANER) $(PRG)/prg.c
		$(CC_GO32) $(CFLAGS_GO32) $(PRG)/prg.c -o $@
		chmod 644 $@
		sync
		echo $@ done.

prgclean:
		rm -f $(BIN)/prg $(BIN)/prggo32{,.exe} $(PRG)/*~

prgdist:
		cp --parents $(PRGFILES) $(DISTDIR)

prginstall:
		install -m 0700 $(BIN)/prg $(TNNUSEREXE)/prg
else
#
# Hier (fast) das gleiche fuer Dose
#
$(BIN)/prg:	$(PRGFILES) $(CLEANER)
		$(CLEANER) $(PRG)/prg.c
		$(CC) $(CFLAGS_GO32) $(PRG)/prg.c -o $@
		echo $@ done.

prgclean:
		erase $(subst /,\\,$(BIN))\prg
		erase $(subst /,\\,$(BIN))\prg.exe
		erase $(subst /,\\,$(PRG))\*.bak
endif

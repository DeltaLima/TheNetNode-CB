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
# File makefile (maintained by: DF6LN)                                 #
#                                                                      #
# This file is part of "TheNetNode" - Software Package                 #
#                                                                      #
# Copyright (C) 1998 - 2006 NORD><LINK e.V. Braunschweig               #
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
# Makefile fuer TNN 1.79
# Zusaetzlich zur Linux-Version von TNN kann man mit diesem Makefile
# die GO32-Version uebersetzen, sowohl unter Linux als auch unter DOS.
# Weiterhin wird unter Linux die Crosscompilierung unterstuetzt.
#

#
# Wo sind wir denn heut zu Hause?
#
UNAME	:= $(wildcard /bin/uname)
ifneq ($(strip $(UNAME)),)
SYSTEM	:= $(shell uname -s)
endif
#
ifeq ($(SYSTEM), Linux)
SYSTEM	:= LINUX
else
SYSTEM	:= DOS
endif
#
#
# Source Verzeichnisse fuer Linux und Dose - Trennungszeichen immer "/",
# das wird so auch vom Dosen-Compiler verstanden
#
BIN			:= bin
DOC			:= doc
HDR			:= include
LIN			:= os/linux
GO32			:= os/go32
SRC			:= src
INI			:= $(LIN)/ini
CLEANER			:= $(BIN)/cleaner
CONTRIB			:= contrib

ifeq ($(SYSTEM), LINUX)
#
# Irgendeine besondere (Ziel-)Maschine ?
#
ifeq ($(MIPS),)
MIPS	:= $(shell uname -m)
endif
#
ifeq ($(MIPS), mips)
MIPS	:= YES
else
MIPS    := NO
endif
#
# Bei Linux gibt es auch eine Installation in das folgende Hauptverzeichnis
#
TNNDIR			:= /usr/local/tnn/
#TNNDIR			:= /var/lib/tnn/
#
# Pfad zur Initialisierungsdatei tnn.ini bei Linux
#
INSTCFG			:= $(TNNDIR)
#INSTCFG		:= /etc/tnn/
#
# Pfad zur aufuehrbaren Datei tnn (fuer Linux)
INSTBIN			:= $(TNNDIR)
#INSTBIN		:= /usr/bin
#
# Die folgenden Unterverzeichnisse zum TNN-Hauptverzeichnis sollten nicht
# geaendert werden.
#
TNNDIR			:= $(patsubst %//,%/,$(TNNDIR)/)
TNNTEXTCMD		:= $(TNNDIR)textcmd/
TNNUSEREXE		:= $(TNNDIR)userexe/
TNNSYSEXE		:= $(TNNDIR)sysexe/
TNNPACSAT		:= $(TNNDIR)pacsat/
TNNMSG			:= $(TNNDIR)msg/
#
# Ein Verteil-Archiv wird auch nur bei Linux generiert
# Der Pfad DISTDIR wird relativ zu / angegeben!
#                       ============
#
NAME			:= $(shell basename `pwd`)
DISTDIR			:= usr/local/src/tnn/$(NAME)
endif


WARNINGS		:= -Wall -Wpointer-arith -Wcast-align
WARNINGS		+= -Waggregate-return -Wstrict-prototypes
WARNINGS		+= -Wimplicit-function-declaration

#
# Compiler
#
# Fuer Linux wird der normale Linux-GCC verwendet. Fuer die DOS/GO32-Version
# wird unter DOS das DJGPP-GCC-Paket verwendet. Unter Linux kann man die
# DOS/GO32-Version ebenfalls uebersetzen! Benoetigt wird dafuer der
# Linux -> MS-DOS GCC Cross-Compiler von J.J. van der Heijden - verfuegbar
# unter sunsite.unc.edu:/pub/Linux/devel/msdos
#
ifeq ($(SYSTEM), LINUX)
#
# Der native gcc dieser Architektur, er wird fuer die Uebersetzung des
# Cleaners benoetigt, da dieser beim Crosscompilieren nicht fuer das Zielsystem,
# sondern fuer den Host compiliert werden muss !
#
HOSTCC      := gcc
#
# Der zum Uebersetzen genutzte Compiler kann beim make-Aufruf uebersteuert werden
# (make CC=gcc-foobar). Ist CC nicht definiert, dann nehmen wir den gcc des Host.
#
#
ifeq ($(CC), "")
CC              := gcc
endif
CC_GO32         := gcc-go32
else
CC_GO32		:= gcc
endif
#
# Compiler-Flags
#
# VORSICHT! TNN ist nicht getestet mit eingeschalteten Optimierungen, ausser
# an den notwendigen Stellen (VANESSA). Option -O2 (oder aehnlich) also
# keinesfalls anfuegen.
#
CFLAGS			:= -funsigned-char $(WARNINGS)
# Wegkommentieren, wenn fuer Pentium uebersetzt werden soll
# CFLAGS			+= -mcpu=pentium
# Die folgende Zeile wegkommentieren, wenn fuer 386 uebersetzt werden soll
# CFLAGS			+= -m386
# Fuer ganz mutige : Compiler-Optimierungen einschalten (siehe oben !!!)
# CFLAGS			+= -O2
# Die folgende Zeile kommentieren, wenn keine Debugsymbole gewuenscht sind.
# Noch mehr Debug gibt es mit -g3, weniger mit -g1, -g entspricht -g2. (Linux)
CFLAGS			+= -g
ifeq ($(SYSTEM), LINUX)
CFLAGS			+= -pipe -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
#CFLAGS   += -pedantic
#CFLAGS   += -O
endif

CFLAGS			+= -I$(HDR) -I$(SRC)

#
# Nun haben wir die gemeinsamen Compiler-Flags fuer GO32 und fuer Linux.
# Eventuell trotz obiger Warnung eingeschaltete Optimierungen werden entfernt.
# Dazu kommen jetzt noch die Systemspezifischen Include-Verzeichnisse und fuer
# die Linux-TNN die von TNN zu verwendenden Verzeichnisse.
#
CFLAGS		:= $(subst -O ,,$(subst -O2 ,,$(subst -O3 ,,$(CFLAGS))))
CFLAGS_GO32	:= $(CFLAGS) -I$(GO32)
CFLAGS_LIN	:= $(CFLAGS) -I$(LIN)
CFLAGS_LIN	+= -DINIPATH=\"$(patsubst %//,%/,$(INSTCFG)/)\"
CFLAGS_LIN	+= -DTEXTPATH=\"$(TNNDIR)\"
CFLAGS_LIN	+= -DTEXTCMDPATH=\"$(TNNTEXTCMD)\"
CFLAGS_LIN	+= -DUSEREXEPATH=\"$(TNNUSEREXE)\"
CFLAGS_LIN	+= -DSYSEXEPATH=\"$(TNNSYSEXE)\"
CFLAGS_LIN	+= -DPACSATPATH=\"$(TNNPACSAT)\"
CFLAGS_LIN	+= -DMSGPATH=\"$(TNNMSG)\"

# MIPS-Systeme bekommen noch ein besonderes Kennzeichen
ifeq ($(MIPS), YES)
CFLAGS_LIN  += -DMIPS=$(MIPS)
endif

#
# Source-, Header- und Object-Files
#
L3SRC			:= l3inp.c l3misc.c l3nbr.c l3netrom.c l3thenet.c
L3SRC			+= l3rtt.c l3tab.c l3var.c l3vc.c
L3TCP			+= l3sock.c l3tcp.c l1tcpip.c l1telnet.c l1httpd.c l1ipconv.c l1irc.c
COMMONSRC		:= buffer.c callstr.c cvs_cmds.c cvs_cvsd.c cvs_serv.c
COMMONSRC		+= cvs_cvrt.c file.c global.c graph.c
COMMONSRC		+= l2dama.c l2misc.c l2rx.c l2stma.c l2timer.c l2tx.c
COMMONSRC		+= l3ip.c $(L3SRC) $(L3TCP) l4.c l7.c l7ccp.c l7cmds.c
COMMONSRC		+= l7conn.c l7host.c l7hstcmd.c l7ip.c l7moni.c
COMMONSRC		+= l7showl3.c l7time.c l7utils.c main.c mh.c
COMMONSRC		+= pacsat.c pacserv.c profil.c profiler.c speech.c
VERSRC			:= $(SRC)/version.c

LINUXSRCS		:= $(patsubst %,$(SRC)/%,$(COMMONSRC))
LINUXSRCS		+= $(LIN)/init.c $(LIN)/linux.c $(LIN)/l1linux.c
LINUXSRCS		+= $(LIN)/axipx.c $(LIN)/ax25ip.c $(LIN)/kernelip.c
LINUXSRCS		+= $(LIN)/kernelax.c $(LIN)/6pack.c
LINUXSRCS		+= $(LIN)/l1attach.c $(LIN)/ostcpip.c $(LIN)/osipconv.c
LINUXOBJS		:= $(LINUXSRCS:.c=.o)
VANSRC			:= $(LIN)/vanlinux.c
VANOBJ			:= $(LIN)/vanlinux.o
VEROBJL			:= $(SRC)/version.o
L3SRCS			:= $(patsubst %,$(SRC)/%,$(L3SRC))
L3OBJL			:= $(L3SRCS:.c=.o)

GO32_SRCS		:= $(GO32)/api.c $(GO32)/go32.c $(GO32)/16550.c
GO32_SRCS		+= $(GO32)/l1.c $(patsubst %,$(SRC)/%,$(COMMONSRC))
GO32_OBJS		:= $(GO32_SRCS:.c=.obj)
GO32L1			:= $(GO32)/extdev.c $(GO32)/kiss.c $(GO32)/loop.c
GO32L1			+= $(GO32)/par.c $(GO32)/scc.c $(GO32)/tokenrng.c
GO32L1			+= $(GO32)/vanessa.c $(GO32)/kiss.h $(GO32)/vanessa.h
VEROBJG			:= $(SRC)/version.obj
L3OBJG			:= $(L3SRCS:.c=.obj)

HDRNAMS			:= all.h allmodif.h conversd.h cvs_cmds.h
HDRNAMS			+= function.h global.h host.h icmp.h ip.h ipv.h l2.h l2s.h
HDRNAMS			+= l3sock.h l3tcp.h l1httpd.h l1tcpip.h l1telnet.h
HDRNAMS			+= l3global.h l3thenet.h l4.h l7.h profil.h profiler.h stat.h
HDRNAMS			+=  system.h speech.h tnn.h typedef.h

LINUXHDRS		:= $(patsubst %,$(HDR)/%,$(HDRNAMS))
LINUXHDRS		+= $(LIN)/linux.h $(LIN)/vanlinux.h $(LIN)/linclude.h
LINUXHDRS		+= $(LIN)/kernelip.h $(LIN)/kernelax.h $(LIN)/ax25ip.h
GO32HDRS		:= $(patsubst %,$(HDR)/%,$(HDRNAMS)) $(GO32)/api.h 
GO32HDRS		+= $(GO32)/api32.h $(GO32)/hardware.h $(GO32)/pc.h
L3HDR			:= $(HDR)/l3local.h

MAKEFILE		:= makefile
OTHER1			:= alas.txt $(SRC)/config.c $(INI)/ax25ip.cfg
OTHER1			+= $(INI)/{tnn179.tnb,tnnini.all} $(L3HDR)
OTHER1			+= $(SRC)/cleaner.c $(SRC)/update.c history/*.his
OTHER2			:= $(DOC)/{conversd.{xhf,g},tnn179.pdf}
OTHER2			+= $(CONTRIB)/makefile

.SUFFIXES:
.SILENT:
.EXPORT_ALL_VARIABLES:

.PHONY:			all tnn upd tnngo32 updgo32 clean cleaner
.PHONY:			mindist dist bigdist install baseinstall

%.o:			%.c $(MAKEFILE) $(CLEANER)
			echo -n 'Compiling $@ '
			$(CLEANER) $<
			$(CC) -c $(CFLAGS_LIN) -o $@ $<
			echo done.

%.obj:			%.c $(MAKEFILE) $(CLEANER)
			echo -n 'Compiling $@ '
			$(CLEANER) $<
			$(CC_GO32) -c $(CFLAGS_GO32) -o $@ $<

all:			tnn upd

$(LINUXOBJS):		$(LINUXHDRS) $(MAKEFILE)
$(VEROBJL):		$(VERSRC) $(LINUXOBJS) $(VANOBJ)
$(VEROBJG):		$(VERSRC) $(GO32_OBJS)
$(GO32_OBJS):		$(GO32HDRS) $(MAKEFILE)
$(SRC)/global.o:	$(SRC)/config.c
$(SRC)/global.obj:	$(SRC)/config.c
$(GO32)/l1.obj:		$(GO32L1)
$(L3OBJL) $(L3OBJG):	$(L3HDR)

cleaner:		$(CLEANER)
tnngo32:		$(BIN)/tnngo32

ifeq ($(SYSTEM), LINUX)
#
# erstmal kommt Linux
#

# externe Libs
LIB := -lutil

.PHONY:		go32 _dist _mindist _install

tnn:		$(BIN)/tnn
go32:		tnngo32 updgo32
upd:		$(BIN)/upd
updgo32:	$(BIN)/updgo32

# bin-Verzeichnis erstellen
$(BIN):
ifneq ($(wildcard $(BIN)), $(BIN))
		echo Creating directory $(BIN)
		- mkdir $(BIN)
endif

# Linux VANESSA Treiber
$(VANOBJ):	$(MAKEFILE) $(CLEANER)
$(VANOBJ):	$(VANSRC) $(LIN)/vanlinux.h $(LINUXHDRS)
		echo -n 'Compiling $@ '
		$(CLEANER) $(VANSRC) $(LIN)/vanlinux.h
		$(CC) -c -O2 $(CFLAGS_LIN) -o $@ $(VANSRC)
		echo done.

# TNN-Executable linken
$(BIN)/tnn:	$(LINUXOBJS) $(VANOBJ) $(VEROBJL) $(CLEANER)
		echo -n 'Linking $@ ' 
		$(CLEANER) $(LINUXHDRS)
		$(CC) $(LINUXOBJS) $(VANOBJ) $(VEROBJL) $(LDFLAGS_LIN) $(LIB) -o $@
		echo done.

# Sourcecode-Cleaner
$(CLEANER):	$(SRC)/cleaner.c $(LINUXHDRS) $(MAKEFILE)
ifneq ($(wildcard $(BIN)), $(BIN))
		echo Creating directory $(BIN)
		- mkdir $(BIN)
endif
		echo -n 'Building $@ '
		$(HOSTCC) $(CFLAGS_LIN) $(SRC)/cleaner.c $(LDFLAGS_LIN) -o $@
		echo done.

# TNN GO32
$(BIN)/tnngo32:	$(GO32_OBJS) $(VEROBJG) $(CLEANER)
		$(CLEANER) $(GO32HDRS)
		$(CC_GO32) $(GO32_OBJS) $(VEROBJG) -o $@
		chmod 644 $@
		echo $@ done.

# Updater
$(BIN)/upd:	$(BIN) $(SRC)/update.c $(LINUXHDRS) $(MAKEFILE)
		$(CC) $(CFLAGS_LIN) $(SRC)/update.c $(LDFLAGS_LIN) -o $@
		echo $@ done.

# Updater fuer GO32
$(BIN)/updgo32:	$(BIN) $(SRC)/update.c $(GO32HDRS) $(MAKEFILE)
		$(CC_GO32) $(CFLAGS_GO32) $(SRC)/update.c -o $@
		chmod 644 $@
		echo $@ done.

# Aufraeumen
clean:
		rm -f $(LINUXOBJS) $(VANOBJ) $(GO32_OBJS) $(VEROBJL) $(VEROBJG)
		rm -f $(NAME).tar.bz2 .macros
		rm -f {.,$(SRC),$(LIN),$(INI),$(GO32),$(HDR),$(DOC)}/*~
		rm -f {$(GO32),history}/*~
		rm -f $(BIN)/{cleaner,{upd,tnn}{,go32{,.exe}}}
		rm -rf bin
ifeq ("$(findstring 179,$(shell pwd))","179")
		rm -rf usr
endif

TNNINI		:= $(INI)/tnn.ini
TNNPAS		:= $(INI)/tnn179.pas
#
# Mit "make mindist" wird der Source verpackt, soweit zum uebersetzen noetig,
# mit "make dist" wird zusaetzlich die Doku verpackt und mit "make bigdist"
# sind auch die Teile im Verzeichnis contrib/ dabei. Hier ist allerdings kein
# Unterschied zu sehen zwischen "dist" und "bigdist", sondern nur in
# contrib/*/makefile.
#
dist bigdist:	_mindist
ifeq ("$(findstring 179,$(shell pwd))","179")
		cp --parents $(OTHER2) $(DISTDIR)
		echo $(NAME).tar.bz2
		tar -cf $(NAME).tar $(DISTDIR)
		bzip2 $(NAME).tar
		rm -rf usr
else
		echo Invalid Directory Name.
endif

mindist:	_mindist
ifeq ("$(findstring 179,$(shell pwd))","179")
		echo $(NAME).tar.bz2
		tar -cf $(NAME).tar $(DISTDIR)
		bzip2 $(NAME).tar
		rm -rf usr
else
		echo Invalid Directory Name.
endif

_mindist:	_dist $(TNNINI) $(TNNPAS)
		cp --parents $(LINUXSRCS) $(LINUXHDRS) $(VANSRC) $(DISTDIR)
		cp --parents $(GO32_SRCS) $(GO32HDRS) $(GO32L1) $(DISTDIR)
		cp --parents $(VERSRC) $(MAKEFILE) $(DISTDIR)
		cp --parents $(OTHER1) $(DISTDIR)

_dist:
ifeq ("$(findstring 179,$(shell pwd))","179")
		rm -rf usr $(NAME).tar.bz2
		mkdir --parents $(DISTDIR)
else
		echo Invalid Directory Name.
endif
#
# install nur fuer Linux-Version - GO32-Version muss von Hand kopiert werden
#
install:	all _install
		install -m 0700 $(BIN)/tnn $(INSTBIN)/tnn

_install:
		install -m 0700 -d $(TNNDIR)
		install -m 0700 -d $(TNNSYSEXE)
		install -m 0700 -d $(TNNTEXTCMD)
		install -m 0700 -d $(TNNUSEREXE)
		install -m 0700 -d $(TNNPACSAT)
		install -m 0700 -d $(TNNMSG)

baseinstall:	install $(TNNINI) $(TNNPAS)
		install -m 0700 -d $(INSTCFG)
		install -m 0600 $(TNNINI) $(INSTCFG)/tnn.ini
		install -m 0600 $(INI)/tnnini.all $(TNNDIR)/tnnini.all
		install -m 0600 $(INI)/ax25ip.cfg $(TNNDIR)/ax25ip.cfg
		install -m 0600 $(TNNPAS) $(TNNDIR)/tnn179.pas
		install -m 0600 $(INI)/tnn179.tnb $(TNNDIR)/tnn179.tnb
		install -m 0600 $(DOC)/conversd.xhf $(TNNDIR)/conversd.xhf

$(TNNINI):	$(MAKEFILE)
	echo $@
	echo "# This is an example for a tnn.ini-file" >$(TNNINI)
	echo "# For a more comprehensive example see tnnini.all" >$(TNNINI)
	echo "# -----------------------------------------------" >$(TNNINI)
	echo "# File permissions for files created by TNN" >$(TNNINI)
	echo "# octal value as used by umask(2)" >>$(TNNINI)
	echo "# default value = 000 (world readable/writable)" >>$(TNNINI)
	echo "# use 077 to limit access to the owner of TNN" >>$(TNNINI)
	echo "#perms 077" >>$(TNNINI)
	echo "# Working directory for TNN" >$(TNNINI)
	echo "#tnn_dir $(TNNDIR)" >>$(TNNINI)
	echo "# Unix-Socket for TNT hostmode interface (optional)" >>$(TNNINI)
	echo "#tnn_socket $(TNNDIR)tnn-socket" >>$(TNNINI)
	echo "# Program to start before using any hardware ports" >>$(TNNINI)
	echo "# (optional) - don't use any parameters!" >>$(TNNINI)
	echo "#tnn_start kill_other_processes" >>$(TNNINI)
	echo "# Number of buffers (optional; default = 10000)" >>$(TNNINI)
	echo "buffers 10000" >>$(TNNINI)
	echo "# file containing process id (mandatory)" >>$(TNNINI)
	echo "tnn_procfile tnn.pid" >>$(TNNINI)
	echo "# rounds per second (if missing, default 100)" >>$(TNNINI)
	echo "#rounds 200" >>$(TNNINI)
	echo "#-------------------------------------------------" >>$(TNNINI)
	echo "# device 1" >>$(TNNINI)
	echo "device /dev/ttyS0" >>$(TNNINI)
	echo "# lockfile for device 1" >>$(TNNINI)
	echo "tnn_lockfile /var/lock/LCK..ttyS0" >>$(TNNINI)
	echo "# speed on device 1" >>$(TNNINI)
	echo "speed 38400" >>$(TNNINI)
	echo "# type of KISS on device 1:" >>$(TNNINI)
	echo "# 0 = KISS, 1 = SMACK, 2 = RMNC-KISS," >>$(TNNINI)
	echo "# 3 = Tokenring (1st device only)," >>$(TNNINI)
	echo "# 4 = Vanessa, 5 = SCC, 6 = TF (experimental!)," >>$(TNNINI)
	echo "# 7 = IPX (1 IPX port only)," >>$(TNNINI)
	echo "# 8 = AX25IP (1 AX25IP port only)" >>$(TNNINI)
	echo "# 10 = Kernel-AX.25, 11 = DG1KJD Kernel-AX.25" >>$(TNNINI)
	echo "# 12 = 6PACK" >>$(TNNINI)
	echo "kisstype 3" >>$(TNNINI)
	echo "# L2-Port associated with device 1" >>$(TNNINI)
	echo "# use several port lines, if kisstype = 3" >>$(TNNINI)
	echo "port 0" >>$(TNNINI)
	echo "port 1" >>$(TNNINI)
	echo "port 2" >>$(TNNINI)
	echo "port 3" >>$(TNNINI)
	echo "port 4" >>$(TNNINI)
	echo "port 5" >>$(TNNINI)
	echo "port 6" >>$(TNNINI)
	echo "port 7" >>$(TNNINI)
	echo "port 8" >>$(TNNINI)
	echo "port 9" >>$(TNNINI)
	echo "port 10" >>$(TNNINI)
	echo "port 11" >>$(TNNINI)
	echo "port 12" >>$(TNNINI)
	echo "port 13" >>$(TNNINI)
	echo "port 14" >>$(TNNINI)
	echo "port 15" >>$(TNNINI)
	echo "#-------------------------------------------------" >>$(TNNINI)
	echo "# device 2" >>$(TNNINI)
	echo "#device /dev/ttyS1" >>$(TNNINI)
	echo "# lockfile for device 2" >>$(TNNINI)
	echo "#tnn_lockfile /var/lock/LCK..ttyS1" >>$(TNNINI)
	echo "# speed on device 2" >>$(TNNINI)
	echo "#speed 38400" >>$(TNNINI)
	echo "# type of KISS on device 2:" >>$(TNNINI)
	echo "# 0 = KISS, 1 = SMACK, 2 = RMNC-KISS," >>$(TNNINI)
	echo "# 4 = Vanessa, 5 = SCC, 6 = TF (experimental!)," >>$(TNNINI)
	echo "# 7 = IPX (1 IPX port only)," >>$(TNNINI)
	echo "# 8 = AX25IP (1 AX25IP port only)" >>$(TNNINI)
	echo "# 10 = Kernel-AX.25, 11 = DG1KJD Kernel-AX.25" >>$(TNNINI)
	echo "# 12 = 6PACK" >>$(TNNINI)
	echo "#kisstype 1" >>$(TNNINI)
	echo "# L2-Port associated with device 2" >>$(TNNINI)
	echo "#port 1" >>$(TNNINI)

$(TNNPAS):	$(MAKEFILE)
	echo $@
	echo "; TheNetNode Configuration File" >$(TNNPAS)
	echo ";" >>$(TNNPAS)
	echo "; DO NOT CHANGE THE ORDER OF THE CONFIGURATION LINES !" >>$(TNNPAS)
	echo "; DO NOT CLEAR ANY LINES !" >>$(TNNPAS)
	echo ";" >>$(TNNPAS)
	echo "; NET/ROM-Sysop-Password, 80 Characters (01234567890123...)" >>$(TNNPAS)
	echo -n "1234567890123456789012345678901234567890" >>$(TNNPAS)
	echo    "1234567890123456789012345678901234567890" >>$(TNNPAS)
	echo ";" >>$(TNNPAS)
	echo "; Console Password" >>$(TNNPAS)
	echo "Geheim" >>$(TNNPAS)
	echo ";" >>$(TNNPAS)
	echo "; Node Ident (Test)" >>$(TNNPAS)
	echo "Test  " >>$(TNNPAS)
	echo ";" >>$(TNNPAS)
	echo "; Node MyCall (XX0XX)" >>$(TNNPAS)
	echo "XX0XX" >>$(TNNPAS)
	echo ";" >>$(TNNPAS)
	echo "; Workpath, Path to the Help-Files ($(TNNDIR))" >>$(TNNPAS)
	echo "; TNN should be started from this path." >>$(TNNPAS)
	echo "$(TNNDIR)" >>$(TNNPAS)
	echo ";" >>$(TNNPAS)
	echo "; Path to the executable Text-Files ($(TNNTEXTCMD))" >>$(TNNPAS)
	echo "$(TNNTEXTCMD)" >>$(TNNPAS)
	echo ";" >>$(TNNPAS)
	echo "; Path to the extern Programs for User ($(TNNUSEREXE))" >>$(TNNPAS)
	echo "$(TNNUSEREXE)" >>$(TNNPAS)
	echo ";" >>$(TNNPAS)
	echo "; Path to the extern Programs only for Sysop ($(TNNSYSEXE))" >>$(TNNPAS)
	echo "$(TNNSYSEXE)" >>$(TNNPAS)
	echo ";" >>$(TNNPAS)
	echo "; Path to the PACSAT-Files ($(TNNPACSAT))" >>$(TNNPAS)
	echo "$(TNNPACSAT)" >>$(TNNPAS)

mac:
		$(CC) $(CFLAGS_LIN) -E -dM $(HDR)/tnn.h > .macros

else
#
# nun kommt Dose
#
tnn:			tnngo32
upd:			$(BIN)/updgo32

$(BIN):
ifneq ($(wildcard $(BIN)), $(BIN))
			echo $(BIN)
			- mkdir $(subst /,\\,$(BIN))
endif

clean:
			erase $(subst /,\\,$(BIN))\*
			erase $(subst /,\\,$(BIN))\*.exe
			erase $(subst /,\\,$(SRC))\*.o*
			erase $(subst /,\\,$(SRC))\*.bak
			erase $(subst /,\\,$(GO32))\*.o*
			erase $(subst /,\\,$(GO32))\*.bak
			erase $(subst /,\\,$(DOC))\*.bak
			erase $(subst /,\\,$(INCLUDE))\*.bak
			erase $(subst /,\\,$(HISTORY))\*.bak

$(BIN)/tnngo32:		$(GO32_OBJS) $(VEROBJG) $(CLEANER)
			$(CLEANER) $(GO32HDRS)
			$(CC_GO32) $(GO32_OBJS) $(VEROBJG) -o $@
			echo $@ done.

$(CLEANER):		$(SRC)/cleaner.c $(GO32HDRS) $(MAKEFILE)
ifneq ($(wildcard $(BIN)), $(BIN))
			echo $(BIN)
			- mkdir $(BIN)
endif
			$(CC) $(CFLAGS_GO32) $(SRC)/cleaner.c -o $@
			echo $@ done.

$(BIN)/updgo32:		$(BIN) $(SRC)/update.c $(GO32HDRS) $(MAKEFILE)
			$(CC) $(CFLAGS_GO32) $(SRC)/update.c -o $@
			echo $@ done.

mindist dist bigdist install baseinstall:
			echo Please update your Operating System.
			echo The target $@ is only valid for Linux.

endif
#
# Die Makefiles in contrib/* sollen auch noch verarbeitet
# werden (wenn vorhanden)
#
MF			:= $(wildcard contrib/*/makefile)
ifneq ($(strip $(MF)),)
include $(MF)
endif

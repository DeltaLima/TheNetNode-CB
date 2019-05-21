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
# File makefile (maintained by: DG8BR)                                 #
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
# Makefile fuer MSG und MSY
#

MSGMSY		:= contrib/msgmsy
MSGFILES	:= $(MSGMSY)/{ms[gy]{_lin.c,.his,.inf},msg.{usr,sys},makefile}

.PHONY:		msg msy msgclean

all:		msg msy
clean:		msgclean

msg:		$(BIN)/msg
msy:		$(BIN)/msy

ifeq ($(SYSTEM), LINUX)
.PHONY:		msgdist msginstall msggo32 msygo32

bigdist:	msgdist
install:	msginstall
go32:		msggo32 msygo32
msggo32:	$(BIN)/msggo32
msygo32:	$(BIN)/msygo32

$(BIN)/msg:	$(MSGMSY)/msg_lin.c $(MSGMSY)/makefile $(CLEANER)
		$(CLEANER) $(MSGMSY)/msg_lin.c
		$(CC) $(CFLAGS_LIN) $(LDFLAGS_LIN) -o $@ $(MSGMSY)/msg_lin.c
		echo $@ done.

$(BIN)/msy:	$(MSGMSY)/msy_lin.c $(MSGMSY)/makefile $(CLEANER)
		$(CLEANER) $(MSGMSY)/msy_lin.c
		$(CC) $(CFLAGS_LIN) $(LDFLAGS_LIN) -o $@ $(MSGMSY)/msy_lin.c
		echo $@ done.

$(BIN)/msggo32:	$(MSGMSY)/msg_lin.c $(MSGMSY)/makefile $(CLEANER)
		$(CLEANER) $(MSGMSY)/msg_lin.c
		$(CC) $(CFLAGS_GO32) -o $@ $(MSGMSY)/msg_lin.c
		chmod 644 $@
		echo $@ done.

$(BIN)/msygo32:	$(MSGMSY)/msy_lin.c $(MSGMSY)/makefile $(CLEANER)
		$(CLEANER) $(MSGMSY)/msy_lin.c
		$(CC) $(CFLAGS_GO32) -o $@ $(MSGMSY)/msy_lin.c
		chmod 644 $@
		echo $@ done.

msgclean:
		rm -f $(BIN)/ms[gy] $(BIN)/ms[gy]go32{,.exe} $(MSGMSY)/*~

msgdist:
		cp --parents $(MSGFILES) $(DISTDIR)

msginstall:
		echo $@
		install -m 0700 $(BIN)/msg $(TNNUSEREXE)msg
		install -m 0700 $(BIN)/msy $(TNNSYSEXE)msy
		cp $(MSGMSY)/msg.usr $(TNNMSG)
		cp $(MSGMSY)/msg.sys $(TNNMSG)
else
#
# Nun kommt Dose
#
$(BIN)/msg:	$(MSGMSY)/msg_lin.c $(MSGMSY)/makefile $(CLEANER)
		$(CLEANER) $(MSGMSY)/msg_lin.c
		$(CC) $(CFLAGS_GO32) -o $@ $(MSGMSY)/msg_lin.c
		echo $@ done.

$(BIN)/msy:	$(MSGMSY)/msy_lin.c $(MSGMSY)/makefile $(CLEANER)
		$(CLEANER) $(MSGMSY)/msy_lin.c
		$(CC) $(CFLAGS_GO32) -o $@ $(MSGMSY)/msy_lin.c
		echo $@ done.

msgclean:
		erase $(subst /,\\,$(BIN))\msg
		erase $(subst /,\\,$(BIN))\msy
		erase $(subst /,\\,$(BIN))\msg.exe
		erase $(subst /,\\,$(BIN))\msy.exe
		erase $(subst /,\\,$(MSGMSY))\*.bak
endif

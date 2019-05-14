/************************************************************************/
/*                                                                      */
/*    *****                       *****                                 */
/*      *****                   *****                                   */
/*        *****               *****                                     */
/*          *****           *****                                       */
/*  ***************       ***************                               */
/*  *****************   *****************                               */
/*  ***************       ***************                               */
/*          *****           *****           TheNetNode                  */
/*        *****               *****         Portable                    */
/*      *****                   *****       Network                     */
/*    *****                       *****     Software                    */
/*                                                                      */
/* File include/tnn.h (maintained by: ???)                              */
/*                                                                      */
/* This file is part of "TheNetNode" - Software Package                 */
/*                                                                      */
/* Copyright (C) 1998 - 2008 NORD><LINK e.V. Braunschweig               */
/*                                                                      */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the NORD><LINK ALAS (Allgemeine Lizenz fuer    */
/* Amateurfunk Software) as published by Hans Georg Giese (DF2AU)       */
/* on 13/Oct/1992; either version 1, or (at your option) any later      */
/* version.                                                             */
/*                                                                      */
/* This program is distributed WITHOUT ANY WARRANTY only for further    */
/* development and learning purposes. See the ALAS (Allgemeine Lizenz   */
/* fuer Amateurfunk Software).                                          */
/*                                                                      */
/* You should have received a copy of the NORD><LINK ALAS (Allgemeine   */
/* Lizenz fuer Amateurfunk Software) along with this program; if not,   */
/* write to NORD><LINK e.V., Hinter dem Berge 5, D-38108 Braunschweig   */
/*                                                                      */
/* Dieses Programm ist PUBLIC DOMAIN, mit den Einschraenkungen durch    */
/* die ALAS (Allgemeine Lizenz fuer Amateurfunk Software), entweder     */
/* Version 1, veroeffentlicht von Hans Georg Giese (DF2AU),             */
/* am 13.Oct.1992, oder (wenn gewuenscht) jede spaetere Version.        */
/*                                                                      */
/* Dieses Programm wird unter Haftungsausschluss vertrieben, aus-       */
/* schliesslich fuer Weiterentwicklungs- und Lehrzwecke. Naeheres       */
/* koennen Sie der ALAS (Allgemeine Lizenz fuer Amateurfunk Software)   */
/* entnehmen.                                                           */
/*                                                                      */
/* Sollte dieser Software keine ALAS (Allgemeine Lizenz fuer Amateur-   */
/* funk Software) beigelegen haben, wenden Sie sich bitte an            */
/* NORD><LINK e.V., Hinter dem Berge 5, D-38108 Braunschweig            */
/*                                                                      */
/************************************************************************/

#include "system.h"

/*---------------- Header-Files fuer alle Compiler ---------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

/*---------- Header-Files abhaengig vom Compiler / Betriebssystem ------*/

#ifdef __LINUX__
#include "linclude.h"
#endif

#ifdef __WIN32__
#include "winclude.h"
#endif /* WIN32 */

#ifdef PC
#include <process.h>
#if defined(__DOS16__) || defined(__FALCON__)
#include <alloc.h>
#endif
#include <dir.h>
#include <io.h>
#include <conio.h>
#include <values.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dos.h>
#include <stdarg.h>
#include <unistd.h>
#endif

#ifdef ST
#include <process.h>
#include <ext.h>
#endif

/*------------------ Header-Files fuer TNN -----------------------------*/

#include "allmodif.h"
#include "all.h"
#include "l2.h"
#include "typedef.h"
#include "l2s.h"
#ifdef __LINUX__
#include "linux.h"
#endif
#ifdef __WIN32__
#include "win32.h"
#endif /* WIN32 */
#include "function.h"
#include "global.h"
#include "l3global.h"
#include "l4.h"
#include "l7.h"
#include "ip.h"
#ifdef IPROUTE
#include "icmp.h"
#include "ipv.h"
#endif
#include "host.h"
#include "stat.h"
#include "profiler.h"
#ifdef SPEECH
#include "speech.h"
#endif
#ifdef USERPROFIL
#include "profil.h"
#endif /* USERPROFIL */
#ifdef THENETMOD
#include "l3thenet.h"
#endif /* THENETMOD */
#ifdef ATTACH
#include "l1attach.h"
#endif /* ATTACH */
#ifdef TCP_STACK
#include "l3sock.h"
#include "l3tcp.h"
#endif /* TCP_STACK. */
#ifdef L1TCPIP
#include "l1tcpip.h"
#endif /* L1TCPIP. */
#ifdef OS_STACK
#include "ostcpip.h"
#endif /* OS_STACK. */

/* End of include/tnn.h */

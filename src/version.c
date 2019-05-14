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
/* File src/version.c (maintained by: ???)                              */
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

#include "tnn.h"

/*----------------------------------------------------------------------*/
/* Meldetext und Version, Datum und Zeit automatisch eingebunden        */
/*----------------------------------------------------------------------*/

/* Die naechsten beiden Zeilen werden bei einem Hauptrelease geaendert  */
#define __NAME__     "TheNetNode"
#define __VER__      "1.79"

/*#define DEBUG */

#define __AUTHOR__   "cb"       /* Monogramm des Autors/Programmierers  */
#define __PATCH__    "53"       /* Patch-Level des Autors               */

/* Zuerst legen wir die variierenden Texte mal fest                     */
#ifdef __LINUX__
#ifdef MIPS
#  define __SYSTEM__                   " (Linux/MIPS) (CB)"
#else
#  define __SYSTEM__                   " (Linux) (CB)"
#endif
#else
#  ifdef __WIN32__
#    define __SYSTEM__                 " (Win32) (CB)"
#  else
#    ifdef __GO32__
#      define __SYSTEM__               " (GO32) (CB)"
#    else
#      ifdef ST
#        define __SYSTEM__             " (ST) (CB)"
#      else
#        ifdef MC68302
#          define __SYSTEM__           " (TNC3) (CB)"
#        else
#          define __SYSTEM__           " (unknown) (CB)"
#       endif
#     endif
#   endif
# endif
#endif
/* Aufbau der Versionskennung, diesmal was uebersichtlicher **************/

char signon[]  = __NAME__ __SYSTEM__
                      ", " __VER__ __AUTHOR__ __PATCH__ " (";

char version[] = __NAME__ __SYSTEM__
                      ", " "Version " __VER__ __AUTHOR__ __PATCH__
#ifdef DEBUG
                      " ("__DATE__")\r"
                      ";        BETA-TEST SOFTWARE, USE ONLY FOR DEBUG PURPOSES.\r";
#else
                      " ("__DATE__")\r";
#endif

char cfgfile[] = "TNN179";
#ifdef AUTO_UPDATE
char oldcfgfile[] = "TNN178";
#endif

#ifdef MAKRO_NOLOGINSTR
char loginstr[] = "TNN V" __VER__ __SYSTEM__;
#else
char loginstr[] = "TNN V" __VER__ __SYSTEM__ "\r";
#endif

char infostr[] = "TNN179";

char author[] = "*** CB-Version by DAA531, http://dig531.dyndns.org. ***\r";

/* End of src/version.c */

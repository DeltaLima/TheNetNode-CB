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
/* File include/system.h (maintained by: ???)                           */
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

/* Hardwareabhaengige Funktionen werden entsprechend der folgenden      */
/* Definitionen fuer den jeweiligen Rechner passend compiliert          */

/* Hardwareabhaengige Funktionen werden entsprechend der folgenden      */
/* Definitionen fuer den jeweiligen Rechner passend compiliert          */

#ifdef __MSDOS__
#define PC                              /* Compilieren fuer PC          */
#define __DOS16__
#endif

#ifdef _FALCLIB_                        /* Compilieren fuer FALCON      */
#undef __DOS16__
#define __FALCON__
#define MAX_PEERS 8
#endif

#ifdef __GO32__
#define PC                              /* Compilieren fuer DJGPP/GNU   */
#undef __DOS16__
#define huge
#define far
#define MAX_NODES 1000
#define MAX_PEERS 32
#endif

#ifdef __linux__                        /* Compilieren fuer Linux       */
#define __LINUX__
#undef __DOS16__
#undef __DOS32__
#define huge
#define far
#define MAX_NODES 1000
#define MAX_PEERS 32
#define NO_DISKDRIVE
#endif

#ifdef WIN32                            /* Compilieren fuer Win32       */
#ifdef _MSC_VER
#define __WIN32__
#endif
#undef __DOS16__
#undef __DOS32__
#define huge
#define far
#define MAX_NODES 1000
#define MAX_PEERS 32
#define NO_DISKDRIVE
#endif /* WIN32 */

#ifdef __TOS__
#if !defined(ST) && !defined(MC68302)  /* wird im Projektfile definiert */
#error "68k-Zielsystem nicht spezifiziert!"
#endif
#define MC68K                            /* Fuer 68k spezifische Sachen */
#define huge
#define ptrdiff_t size_t
#define BIG_ENDIAN
#define MAX_NODES 1000
#endif

#ifndef MAX_PEERS
#define MAX_PEERS 16
#endif

#ifndef MAX_NODES
#define MAX_NODES 500
#endif

/* Um sicherzustellen, dass entweder ST oder PC definiert ist...        */
#if defined ( MC68K ) && defined ( PC )
#error "Entweder MC68K oder PC. Beides geht nicht!"
#endif

#if ! defined ( PC ) && ! defined ( ST ) && ! defined ( MC68302 ) && ! defined ( __FALCON__ ) && ! defined ( __LINUX__ ) && ! defined ( __WIN32__ )
#error "Computertyp nicht spezifiziert!"
#endif

/* End of include/system.h */

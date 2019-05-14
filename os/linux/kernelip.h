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
/* File os/linux/kernelip.h (maintained by: DG9OBU)                     */
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

#include <linux/if.h>

#include <linux/version.h>  /* Die Version des Linux-Kernels */

/* Das tun-Interface und dessen Header gibt es erst seit 2.4.x im Kernel */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
#include <linux/if_tun.h>
#else
#warning Kernel-Version is < 2.4.0, using internally defined values for tun-Interface !
#endif

#include <linux/if_ether.h>

/*
  Notwendiges aus if_tun.h, da es aber bei 2.2.x-Style noch kein if_tun.h
  im Kerneltree gibt, sind die Sachen hier zusaetzlich mit drin. Wird das
  if_tun.h trotz des Kernel-Versionschecks nicht gefunden, dann das Include
  fuer if_tun.h einfach auskommentieren.

  Alles was eigentlich der Kernel schon definieren sollte pruefen wir, falls
  das #include fuer if_tun.h fehlt, dann definieren wir die Werte selbst.
  Eventuell ist fuer alte Kernel auch das TUN-Device anzupassen !
*/

#ifndef IFF_TUN
#define IFF_TUN   0x0001
#endif

#ifndef IFF_TAP
#define IFF_TAP   0x0002
#endif

#ifndef IFF_NO_PI
#define IFF_NO_PI 0x1000
#endif

#ifndef TUNSETIFF
#define TUNSETIFF (('T'<< 8) | 202)
#endif

/* unser bevorzugtes TUN-Device */
#define IFIP "/dev/net/tun"

/* Data type definitions --------------------------------------- */

struct kip
{
    BOOLEAN if_available;
    BOOLEAN if_active;
    int if_style;
    int if_fd;
    ipaddr kernel_ip;
    UBYTE maskbits;
    ULONG bytes_tx;
    ULONG bytes_rx;
};

/* End of os/linux/kernelip.h */

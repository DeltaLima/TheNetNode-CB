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
/* File os/go32/loop.c (maintained by: ???)                             */
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

static int loop_major = 0;

static void loopback(void) {
  LHEAD  *l2flp;
  MBHEAD *txfhd;
  MBHEAD *rxfhd;
  int     port;
  int     rxport;
  int     len;

  l2flp = txl2fl;
  for (port=0; port<L2PNUM; l2flp++, port++) /* jeden Port durchlaufen  */
  {
    if (portpar[port].major == loop_major) {
      while (kick[port]) {            /* was zum senden...            */
        ulink((LEHEAD *)(txfhd = (MBHEAD *) l2flp->head));/*Zeiger holen*/
        rxport = port^1;
        if (portpar[rxport].major != loop_major)
          rxport = port;
        /*if (rand() % 15) {*/
          len = cpymbflat(blkbuf, txfhd);
          rxfhd = cpyflatmb(blkbuf, len);
          rxfhd->l2port = rxport;
          relink((LEHEAD *)rxfhd, (LEHEAD *)rxfl.tail);
        /*}*/
        relink((LEHEAD *)txfhd,      /* als gesendet betrachten und in */
          (LEHEAD *)stfl.tail);      /* die gesendet Liste umhaengen   */
        kick[port] = ((LHEAD *)l2flp->head != l2flp);
      }
    }
  }
}

static int register_loopback(void)
{
  MAJOR *m;

  m = register_major();
  m->name    = "LOOPBACK";
  m->handle  = loopback;
  return(loop_major = num_major);
}


/* End of os/go32/loop.c */

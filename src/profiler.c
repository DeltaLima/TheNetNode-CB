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
/* File src/profiler.c (maintained by: DL1XAO)                          */
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

#ifdef PROFILING
static void prof_reset(int num);
static void prof_dump(void);

#define N_PROFILES (PROF_SUM + 1)

#ifdef MC68K
#define HIST  200
#define CLOCK ULONG
#endif

#ifdef __LINUX__
#define HIST  2000
#define CLOCK struct timeval
#endif

#ifdef __GO32__
#define HIST  2000
#define CLOCK ULONG
#endif

typedef struct
{
  CLOCK t_start;
  ULONG histo[HIST + 1];
} PROFILE_T;

static PROFILE_T profiles[N_PROFILES];
static int       prof_onoff = 0;

#ifdef MC68K
void prof_start(int num)
{
  if (prof_onoff)
    profiles[num].t_start = tic10;
}

void prof_stop(int num)
{
  ULONG t;
  PROFILE_T *p;

  if (prof_onoff)
  {
    p = &profiles[num];
    t = tic10 - p->t_start;
    if (t > HIST)
      t = HIST;
    p->histo[(int) t]++;
  }
}
#endif

#ifdef __LINUX__
void prof_start(int num)
{
  struct timeval   tv;
  struct timezone  tz;

  if (prof_onoff)
  {
    gettimeofday(&tv, &tz);
    profiles[num].t_start = tv;
  }
}

void prof_stop(int num)
{
  ULONG            t;
  PROFILE_T       *p;
  struct timeval   tv;
  struct timezone  tz;

  if (prof_onoff)
  {
    gettimeofday(&tv, &tz);
    p = &profiles[num];
    t = (tv.tv_sec - p->t_start.tv_sec) * 1000000 +
        (tv.tv_usec - p->t_start.tv_usec);
    if (t > HIST)
      t = HIST;
    p->histo[(int) t]++;
  }
}
#endif

#ifdef __GO32__
void prof_start(int num)
{
  if (prof_onoff)
    profiles[num].t_start = uclock();
}

void prof_stop(int num)
{
  ULONG t;
  PROFILE_T *p;

  if (prof_onoff)
  {
    p = &profiles[num];
    t = uclock() - p->t_start;
    if (t > HIST)
      t = HIST;
    p->histo[(int) t]++;
  }
}
#endif

static void prof_reset(int num)
{
  memset(&profiles[num], 0, sizeof(PROFILE_T));
}

static void prof_dump(void)
{
  FILE *fp;
  int   i, j;

  if ((fp = xfopen("profile.dat", "wt")) != NULL)
  {
    for (i = 0; i <= HIST; i++)
    {
      for (j = 0; j < (N_PROFILES - 1); j++)
        fprintf(fp, "%lu\t", profiles[j].histo[i]);
      fprintf(fp, "%lu\n", profiles[j].histo[i]);
    }
    fclose(fp);
    putmsg("profiler data saved\r");
  }
  else
    putmsg("file error!\r");
}

void ccp_profile(void)
{
  int i;

  if (issyso() == TRUE)
  {
    skipsp(&clicnt, &clipoi);
    *(clipoi + clicnt) = NUL;

    if (!stricmp(clipoi, "on"))
    {
      prof_onoff = 1;
      putmsg("profiler on\r");
    }
    else if (!stricmp(clipoi, "off"))
    {
      prof_onoff = 0;
      putmsg("profiler off\r");
    }
    else if (!stricmp(clipoi, "reset"))
    {
      for (i = 0; i < N_PROFILES; i++)
        prof_reset(i);
      putmsg("profiler data cleared\r");
    }
    else if (!stricmp(clipoi, "save"))
    {
      prof_dump();
    }
    else if (prof_onoff)
      putmsg("profile is on\r");
    else
      putmsg("profile is off\r");
  }
  else
    invmsg();
}

#endif

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
/* File src/graph.c (maintained by: DG9AML)                             */
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

/* Description
   +-GRAPH----------------------------------------------+
   |+-Hour Element-------------------------------------+|
   || +-1. Peak---+  +-2. Peak---+  ...  +-60. Peak--+ ||
   || |longint min|  |longint min|       |longint min| ||
   || |longint ave|  |longint ave|       |longint ave| ||
   || |longint max|  |longint max|       |longint max| ||
   || +-60 sec.---+  +-60 sec.---+       +-60 sec.---+ ||
   |+--------------------------------------------------+|
   |+-Day Element--------------------------------------+|
   || +-1. Peak---+  +-2. Peak---+  ...  +-48. Peak--+ ||
   || |longint min|  |longint min|       |longint min| ||
   || |longint ave|  |longint ave|       |longint ave| ||
   || |longint max|  |longint max|       |longint max| ||
   || +-1800 sec.-+  +-1800 sec.-+       +-1800 sec.-+ ||
   |+--------------------------------------------------+|
   |+-Week Element-------------------------------------+|
   || +-1. Peak---+  +-2. Peak---+  ...  +-56. Peak--+ ||
   || |longint min|  |longint min|       |longint min| ||
   || |longint ave|  |longint ave|       |longint ave| ||
   || |longint max|  |longint max|       |longint max| ||
   || +-10800 sec.+  +-10800 sec.+       +-10800 sec.+ ||
   |+--------------------------------------------------+|
   +----------------------------------------------------+
 */

/* Description
   +-PORTGRAPH------------------------------------------+
   |+-Hour Element-------------------------------------+|
   || +-1. Peak---+  +-2. Peak---+  ...  +-60. Peak--+ ||
   || |longint rx |  |longint rx |       |longint rx | ||
   || |longint tx |  |longint tx |       |longint tx | ||
   || +-60 sec.---+  +-60 sec.---+       +-60 sec.---+ ||
   |+--------------------------------------------------+|
   |+-Day Element--------------------------------------+|
   || +-1. Peak---+  +-2. Peak---+  ...  +-48. Peak--+ ||
   || |longint rx |  |longint rx |       |longint  rx| ||
   || |longint tx |  |longint tx |       |longint  tx| ||
   || +-1800 sec.-+  +-1800 sec.-+       +-1800 sec.-+ ||
   |+--------------------------------------------------+|
   |+-Week Element-------------------------------------+|
   || +-1. Peak---+  +-2. Peak---+  ...  +-56. Peak--+ ||
   || |longint rx |  |longint rx |       |longint rx | ||
   || |longint tx |  |longint tx |       |longint tx | ||
   || +-10800 sec.+  +-10800 sec.+       +-10800 sec.+ ||
   |+--------------------------------------------------+|
   +----------------------------------------------------+
 */

#include "tnn.h"

#ifdef GRAPH
TGRAPH          graph;

#define GRAPH_HOUR 1
#define GRAPH_DAY  2
#define GRAPH_WEEK 3
#define MAXULONG 0xffffffffL

static void     showgraph(int, TGRAPHELEMENT *, const char *, char *, int, MBHEAD *);
static void     graph_clear_peak(TGRAPHELEMENT *, int, int);
#ifdef PORTGRAPH
static void     showportgraph(int, TPORTGRAPHELEMENT *, const char *, int, char *, MBHEAD *);
static void     graph_clear_trxpeak(TPORTGRAPHELEMENT *, int);
#endif
static void     graph_clear_element(int, int);

static void     graph_actual_longint(TGRAPHPEAK *, ULONG);
static void     graph_actual_peak(TGRAPHELEMENT *, ULONG);
static void     graph_actual_element(void);

static void     graph_average_peak(TGRAPHELEMENT *, int);
static void     graph_average(int);

void
graph_clear_peak(TGRAPHELEMENT *selected, int timesel, int clearmode)
{
  TGRAPHPEAK      dummy;
  dummy.max =
  dummy.ave = (clearmode == 0) ? 0L : MAXULONG;
  dummy.min = MAXULONG;

  switch (timesel)
    {
      case GRAPH_HOUR:
        selected->hour[graph.hour_pos] = dummy;
        break;
      case GRAPH_DAY:
        selected->day[graph.day_pos] = dummy;
        break;
      case GRAPH_WEEK:
        selected->week[graph.week_pos] = dummy;
    }                           /* switch */
}

#ifdef PORTGRAPH
void
graph_clear_trxpeak(TPORTGRAPHELEMENT *selected, int timesel)
{
  TPORTGRAPHPEAK      dummy;

  dummy.rx =
  dummy.tx = MAXULONG;

  switch (timesel)
  {
    case GRAPH_HOUR:
      selected->hour[graph.hour_pos] = dummy;
      break;
    case GRAPH_DAY:
      selected->day[graph.day_pos] = dummy;
      break;
    case GRAPH_WEEK:
      selected->week[graph.week_pos] = dummy;
  } /* switch */
}
#endif

void
graph_clear_element(int timesel, int clearmode)
{
#ifdef PORTGRAPH
  int i;
#endif

  graph_clear_peak(&graph.freebuffer, timesel,clearmode);
  graph_clear_peak(&graph.circuit, timesel,clearmode);
  graph_clear_peak(&graph.l2link, timesel,clearmode);
  graph_clear_peak(&graph.node, timesel,clearmode);
  graph_clear_peak(&graph.roundpsec, timesel,clearmode);
  graph_clear_peak(&graph.throughput, timesel,clearmode);
#ifdef PORTGRAPH
  for (i = 0; i < L2PNUM; i++)
  {
    graph_clear_trxpeak(&graph.info[i], timesel);
    graph_clear_trxpeak(&graph.reject[i], timesel);
    graph_clear_trxpeak(&graph.frmr[i], timesel);
    graph_clear_trxpeak(&graph.sabm[i], timesel);
    graph_clear_trxpeak(&graph.disc[i], timesel);
    graph_clear_trxpeak(&graph.dm[i], timesel);
  }
#endif
}

void
graph_actual_longint(TGRAPHPEAK *selected, ULONG newvalue)
{
  if (selected->max < newvalue)
    selected->max = newvalue;
  if (selected->min > newvalue)
    selected->min = newvalue;
  selected->ave += newvalue;
}

void
graph_actual_peak(TGRAPHELEMENT *selected, ULONG newvalue)
{
  graph_actual_longint(&selected->hour[graph.hour_pos], newvalue);
  graph_actual_longint(&selected->day[graph.day_pos], newvalue);
  graph_actual_longint(&selected->week[graph.week_pos], newvalue);
}

#ifdef PORTGRAPH
void
graph_actual_trxpeak(TPORTGRAPHELEMENT *selected, BOOLEAN tx)
{
  if (graph.enabled == TRUE)
  {
    if (tx)
    {
      if (selected->hour[graph.hour_pos].tx == MAXULONG)
        selected->hour[graph.hour_pos].tx = 0;
      if (selected->day[graph.day_pos].tx == MAXULONG)
        selected->day[graph.day_pos].tx = 0;
      if (selected->week[graph.week_pos].tx == MAXULONG)
        selected->week[graph.week_pos].tx = 0;
      selected->hour[graph.hour_pos].tx++;
      selected->day[graph.day_pos].tx++;
      selected->week[graph.week_pos].tx++;
    }
    else
    {
      if (selected->hour[graph.hour_pos].rx == MAXULONG)
        selected->hour[graph.hour_pos].rx = 0;
      if (selected->day[graph.day_pos].rx == MAXULONG)
        selected->day[graph.day_pos].rx = 0;
      if (selected->week[graph.week_pos].rx == MAXULONG)
        selected->week[graph.week_pos].rx = 0;
      selected->hour[graph.hour_pos].rx++;
      selected->day[graph.day_pos].rx++;
      selected->week[graph.week_pos].rx++;
    }
  }
}
#endif

void
graph_actual_element(void)
{
  graph_actual_peak(&graph.freebuffer, nmbfre);
  graph_actual_peak(&graph.circuit, nmbcir);
  graph_actual_peak(&graph.l2link, nmblks);
  graph_actual_peak(&graph.node, (ULONG)netp->num_nodes);
  graph_actual_peak(&graph.roundpsec, (ULONG)rounds_pro_sec);
  graph_actual_peak(&graph.throughput, thbps * 8L);
}

void
graph_average_peak(TGRAPHELEMENT *selected, int timesel)
{
  switch (timesel)
    {
      case GRAPH_HOUR:
        if (graph.hour_slot > 0)
        {
          selected->hour[graph.hour_pos].ave /= graph.hour_slot;
        }
        break;
      case GRAPH_DAY:
        if (graph.day_slot > 0)
        {
          selected->day[graph.day_pos].ave /= graph.day_slot;
        }
        break;
      case GRAPH_WEEK:
        if (graph.week_slot > 0)
        {
          selected->week[graph.week_pos].ave /= graph.week_slot;
        }
    }                           /* switch */
}

void
graph_average(int timesel)
{
  graph_average_peak(&graph.freebuffer, timesel);
  graph_average_peak(&graph.circuit, timesel);
  graph_average_peak(&graph.l2link, timesel);
  graph_average_peak(&graph.node, timesel);
  graph_average_peak(&graph.roundpsec, timesel);
  graph_average_peak(&graph.throughput, timesel);
}


/************************************************************************/
/* Funktion graphclear(void)                                            */
/*                                                                      */
/* loescht die Graphdaten                                               */
/************************************************************************/
void
graphclear(void)
{
  for (graph.hour_pos = 0;
       graph.hour_pos < GRAPH_STD_ELEMENTS;
       graph.hour_pos++)
    graph_clear_element(GRAPH_HOUR,1);
  for (graph.day_pos = 0;
       graph.day_pos < GRAPH_DAY_ELEMENTS;
       graph.day_pos++)
    graph_clear_element(GRAPH_DAY,1);
  for (graph.week_pos = 0;
       graph.week_pos < GRAPH_WEK_ELEMENTS;
       graph.week_pos++)
    graph_clear_element(GRAPH_WEEK,1);

  graph.hour_pos = 0;
  graph.day_pos = 0;
  graph.week_pos = 0;
  graph.hour_slot = 0;
  graph.day_slot = 0;
  graph.week_slot = 0;

  graph.enabled = FALSE;     /* wird nach einer Minute auf TRUE gesetzt */
}

/************************************************************************/
/* GRAPH                                                                */
/*                                                                      */
/* Funktion: zeigt einen Textgraphen an.                                */
/************************************************************************/
void
ccpgraph(void)
{
  MBHEAD         *mbp;
  int             timesel = 0,
                  expand = 0;
  char            selchr = 0;
  char            timestr[6],
                  selstr[6],
                  prestr[6],
                  expandstr[6];
#ifdef PORTGRAPH
  int             port = 0,
                  portfound = 0;
  char            portstr[6];
#endif
  char            prechar[3] = "#*+";  /* Presentationchar des Graphen  */

  mbp = getmbp();
  putchr('\r', mbp);

  /* nur der Sysop darf Daten loeschen */
  if (issyso() && strnicmp((char *)clipoi, "CLEAR", 5) == 0)
  {
    graphclear();
#ifdef SPEECH
    putstr(speech_message(133), mbp);
#else
    putstr("Graph cleared!\r", mbp);
#endif
    prompt(mbp);
    seteom(mbp);
    return;
  }
  clipoi[clicnt] = NUL;
#ifdef PORTGRAPH
  portstr[0] = NUL;
#endif
  timestr[0] = NUL;
  selstr[0] = NUL;
  prestr[0] = NUL;
  expandstr[0] = NUL;
#ifdef PORTGRAPH
  sscanf((char *) clipoi, "%5s %5s %5s %5s %5s",
                          portstr,
                          timestr,
                          selstr,
                          prestr,
                          expandstr);
  /* ueberpruefen, ob portgraph oder graph gewaehlt wurde */
  if (sscanf(portstr, "%d", &port) == 1)
  { /* Portangabe gefunden */
    portfound = 1;
    strcpy(prechar, """TR""");
  }
  else
  {
    strcpy(expandstr, prestr);
    strcpy(prestr, selstr);
    strcpy(selstr, timestr);
    strcpy(timestr, portstr);
  }
#else
  sscanf((char *) clipoi, "%5s %5s %5s %5s",
                          timestr,
                          selstr,
                          prestr,
                          expandstr);
#endif

  /* Zeiteingabe ermitteln */
  switch (toupper(timestr[0]))
  {
    case 'H':
      timesel = GRAPH_HOUR;
      break;
    case 'D':
      timesel = GRAPH_DAY;
      break;
    case 'W':
      timesel = GRAPH_WEEK;
      break;
    default:
      timesel = 0;
  }

  /* Falls Zeiteingabe weggelassen wurde, ist GRAPH_HOUR aktuell */
  if (!timesel)
  {
    timesel = GRAPH_HOUR;
    strcpy(expandstr, prestr);
    strcpy(prestr, selstr);
    strcpy(selstr, timestr);
  }

  selchr = toupper(selstr[0]);

  /* Darstellungszeichen ermitteln */
#ifdef PORTGRAPH
  if (portfound)
  {
    if (strlen(prestr) == 4 && prestr[0] == '"' && prestr[3] == '"')
    {
      prechar[0] = prestr[1];
      prechar[1] = prestr[2];
    }
    else
      strcpy(expandstr, prestr);
  }
  else
  {
#endif
    if (strlen(prestr) == 5 && prestr[0] == '"' && prestr[4] == '"')
    {
      prechar[0] = prestr[1];
      prechar[1] = prestr[2];
      prechar[2] = prestr[3];
    }
    else
      strcpy(expandstr, prestr);
#ifdef PORTGRAPH
  }
#endif
  /* Expandmode feststellen */
  if (strcmp(expandstr, "+") == 0)
    expand = 1;
  else
  {
    if (strlen(expandstr) != 0)
      selchr = 0;
  }
#ifdef PORTGRAPH
  /* Die Portangabe pruefen */
  if (port < 0 || port >= L2PNUM)
  {
    selchr = 0;
  }

  if (portfound)
  {
    switch (selchr)
    {
      case '*':
      case 'I':
        showportgraph(timesel, &graph.info[port], "Info frames",
                      port, prechar, mbp);
        if (selchr != '*')
          break;
      case 'R':
        showportgraph(timesel, &graph.reject[port], "Reject frames",
                      port, prechar, mbp);
        if (selchr != '*')
          break;
      case 'F':
        showportgraph(timesel, &graph.frmr[port], "FRMR frames",
                      port, prechar, mbp);
        if (selchr != '*')
          break;
      case 'S':
        showportgraph(timesel, &graph.sabm[port], "SABM frames",
                      port, prechar, mbp);
        if (selchr != '*')
          break;
      case 'C':
        showportgraph(timesel, &graph.disc[port], "DISC frames",
                      port, prechar, mbp);
        if (selchr != '*')
          break;
      case 'M':
        showportgraph(timesel, &graph.dm[port], "DM frames",
                      port, prechar, mbp);
        break;
      default:
        selchr = 0;
    }
  }
  else
  {
#endif
    switch (selchr)
    {
      case '*':
      case 'B':
        showgraph(timesel, &graph.throughput, "Throughput (Baud):",
                  prechar, expand, mbp);
        if (selchr != '*')
          break;
      case 'F':
        showgraph(timesel, &graph.freebuffer, "Free buffers:",
                  prechar, expand, mbp);
        if (selchr != '*')
          break;
      case 'R':
        showgraph(timesel, &graph.roundpsec, "Rounds per seconds:",
                  prechar, expand, mbp);
        if (selchr != '*')
          break;
      case 'N':
        showgraph(timesel, &graph.node, "Nodes:", prechar, expand, mbp);
        if (selchr != '*')
          break;
      case 'C':
        showgraph(timesel, &graph.circuit, "Circuits:", prechar, expand, mbp);
        if (selchr != '*')
          break;
      case 'L':
        showgraph(timesel, &graph.l2link, "L2-Links:", prechar, expand, mbp);
        break;
      default:
        selchr = 0;
     }
#ifdef PORTGRAPH
  }
#endif
  if (selchr == 0)
  {
#ifdef PORTGRAPH
#ifdef SPEECH
    putstr(speech_message(134),mbp);
#else
    putstr("SYSTEMGRAPH:\r",mbp);
#endif
#endif
#ifdef SPEECH
    putstr(speech_message(135), mbp);
#else
    putstr("(G)raph (H)our (B)aud\r"
           "        (D)ay  (C)ircuits\r"
           "        (W)eek (F)ree buffers\r"
           "               (L)2-Links\r"
           "               (N)odes\r"
           "               (R)ounds\r"
           "               (*) All\r", mbp);
#endif
#ifdef PORTGRAPH
#ifdef SPEECH
    putstr(speech_message(136), mbp);
    putstr(speech_message(137), mbp);
#else
    putstr("PORTGRAPH:\r"
           "(G)raph <PortNr> (H)our (I)nfo frames\r"
           "                 (D)ay  (R)eject frames\r"
           "                 (W)eek (F)rmr frames\r"
           "                        (S)abm frames\r"
           "                        dis(C) frames\r"
           "                        d(M) frames\r"
           "                        (*) All\r", mbp);
#endif
#endif
  }
  prompt(mbp);
  seteom(mbp);
}

/************************************************************************/
/* showportgraph()                                                      */
/*                                                                      */
/* Funktion: Generiert einen Portgraph                                  */
/*                                                                      */
/* int timesel:      GRAPH_HOUR: Ueberblick 1 Stunde                    */
/*                   GRAPH_DAY:  Ueberblick 24 Stunden                  */
/*                   GRAPH_WEEK: Ueberblick 7 Tage                      */
/*                                                                      */
/* TPORTGRAPHELEMENT select: Welcher Datenbestand ausgegeben werden soll*/
/*                                                                      */
/* char *titel           Ueberschrift des Diagrammes                    */
/*                                                                      */
/* int port              Portangabe (fuer Titel)                        */
/*                                                                      */
/* char *presentation:   Darstellung                                    */
/*                                                                      */
/* MBHEAD *mhp:          Stringuebergabe                                */
/************************************************************************/
#ifdef PORTGRAPH
void
showportgraph(int timesel, TPORTGRAPHELEMENT *selected, const char *titel,
              int port, char *prechar, MBHEAD *mbp)
{
  TPORTGRAPHPEAK  peak[60];     /* 60 entspricht dem groessten Peak     */
                                /* Array aus TGRAPH                     */
  ULONG           raster = 0L;  /* noetig fuer Y-Achse (Werteachse)     */
  ULONG           werttrx,
                  werttx,
                  wertrx;
  ULONG           maximum,
                  minimum,
                  average;      /* Maximalwerte von wertmax, wertmin,.. */
  WORD            elements,
                  pos;
  WORD            averagenum;   /* gibt belegte peak[] Anzahl an        */
  int             lines = GRAPH_LINES;
  int             i,
                  j;
  int             start_min,    /* Fuer Anzeigeberechnung des Neustartes*/
                  start_hour,
                  start_day;
  time_t          start_diff;
  char            zeile[128];   /* Leerzeilenballast entfernen          */

  switch (timesel)
  {
    case GRAPH_DAY:
      elements = GRAPH_DAY_ELEMENTS;
      for (i = 0; i < elements; i++)
        peak[i] = selected->day[i];
      pos = graph.day_pos;
      break;
    case GRAPH_WEEK:
      elements = GRAPH_WEK_ELEMENTS;
      for (i = 0; i < elements; i++)
        peak[i] = selected->week[i];
      pos = graph.week_pos;
      break;
    default:
      elements = GRAPH_STD_ELEMENTS;
      for (i = 0; i < elements; i++)
        peak[i] = selected->hour[i];
      pos = graph.hour_pos;
  }

  /* Titel uebergeben */
  putprintf(mbp, "%s (Port %u):", titel,port);

  /* Maximum,Mittel und Minimum Wert bestimmen */
  maximum = average = 0L;
  minimum = MAXULONG;
  averagenum = 0;

  for (i = 0; i < elements; i++)
  {
    werttrx =   ((peak[i].rx == MAXULONG) ? 0 : peak[i].rx)
              + ((peak[i].tx == MAXULONG) ? 0 : peak[i].tx);
    if (   maximum < werttrx
        && (peak[i].rx != MAXULONG || peak[i].tx != MAXULONG))
      maximum = werttrx;
    if (minimum > werttrx)
      minimum = werttrx;
    if (peak[i].rx != MAXULONG || peak[i].tx != MAXULONG)
    {
      average += werttrx;
      averagenum++;
    }
  } /* for elements */

  if (minimum == MAXULONG)
    minimum = 0;
  if (maximum == MAXULONG)
    maximum = 0;
  if (maximum > 0)
    putprintf(mbp, "  Maximum: %lu", (ULONG)maximum);
  if (averagenum > 0 && average > 0)
#ifdef SPEECH
      putprintf(mbp, speech_message(127), (ULONG)average / averagenum);
#else
      putprintf(mbp, "  Average: %lu", (ULONG)average / averagenum);
#endif
  putprintf(mbp, "  Minimum: %lu\r\r", (ULONG)minimum);

  /* Zeilenraster ermitteln */
  raster = maximum / GRAPH_LINES;
  if (raster == 0 || maximum % GRAPH_LINES > 0)
    raster++;

  /* genaue Zeilenanzahl ermitteln */
  lines = maximum / raster;     /* schwankt zwischen 5 und GRAPH_LINES  */
  if (maximum % raster > 0)
    lines++;
  if (lines < 5)
    lines = 5;                  /* min. sollen es 5 Zeilen sein         */

  lines++;
  /* Die einzelnen Zeilen generieren */
  do
  {
    lines--;
    zeile[0] = NUL;
    putprintf(mbp, "%6lu|", (ULONG)raster * lines);
    for (i = 0; i < elements; i++)
    {
      if (timesel == GRAPH_HOUR)
        j = (i + pos + 1 >= GRAPH_STD_ELEMENTS)
            ? pos + i + 1 - GRAPH_STD_ELEMENTS
            : i + pos + 1;
      else
        j = i;

      wertrx = (peak[j].rx == MAXULONG) ? 0 : peak[j].rx;
      werttx = (peak[j].tx == MAXULONG) ? 0 : peak[j].tx;

      if ((wertrx + werttx) >= raster * lines)
      {
        putstr(zeile, mbp);
        if (werttx >= raster * lines)
          /* Kontrollzeichen abfangen */
#ifdef __WIN32__
          putchr((char)((prechar[0] < 32) ? 'T' : prechar[0]), mbp);
#else
          putchr((prechar[0] < 32) ? 'T' : prechar[0], mbp);
#endif /* WIN32 */
           else
#ifdef __WIN32__
          putchr((char)((prechar[1] < 32) ? 'R' : prechar[1]), mbp);
#else
          putchr((prechar[1] < 32) ? 'R' : prechar[1], mbp);
#endif /* WIN32 */
                   zeile[0] = NUL;
      }
      else
                  strcat(zeile, " ");
    }                           /* for i<elements */
    putchr('\r', mbp);

  }
  while (lines != 1);    /* naechste Zeile */

  /* untere Zeile generieren */
  putprintf(mbp, "%7s", "+");

  /* Neustart Datum ermitteln */
  start_min = localtime(&start_time)->tm_min;
  start_hour = localtime(&start_time)->tm_hour;
  start_day = (localtime(&start_time)->tm_wday != 0)
              ? localtime(&start_time)->tm_wday - 1
              : 6;
  start_diff = sys_time - start_time;

  for (i = 0; i < elements; i++)
  {
    j = (i + pos + 1 >= GRAPH_STD_ELEMENTS)
        ? pos + i + 1 - GRAPH_STD_ELEMENTS
        : i + pos + 1;

    if (   (   (timesel == GRAPH_DAY && i == graph.day_pos)
            || (timesel == GRAPH_WEEK && i == graph.week_pos))
        && graph.enabled == TRUE)
      putchr('A', mbp);
    else
      if (   (   timesel == GRAPH_HOUR
              && graph.enabled == TRUE
              && j == start_min
              && start_diff < 3600 - 60)
          || (   timesel == GRAPH_DAY
              && i == 2 * start_hour + start_min / 30
              && start_diff < 86400 - 1800)
          || (   timesel == GRAPH_WEEK
              && i == 8 * start_day + start_hour / 3
              && start_diff < 604800 - 10800))
        putchr('N', mbp);
      else
        putchr('-', mbp);
  }
  switch (timesel)
  {
    case GRAPH_DAY:
#ifdef SPEECH
        putprintf(mbp, speech_message(128),
                " ", " ");
#else
        putprintf(mbp, "\r%7s00  02  04  06  08  10  12  14  16  18  20  22\r"
                "%9s01  03  05  07  09  11  13  15  17  19  21  23 Hour\r\r",
                " ", " ");
#endif
      break;
    case GRAPH_WEEK:
#ifdef SPEECH
        putprintf(mbp, speech_message(129), " ", " ", " ");
#else
        putprintf(mbp, "\r%7s0 0 1 1 0 0 1 1 0 0 1 1 0 0"
                     " 1 1 0 0 1 1 0 0 1 1 0 0 1 1\r"
                     "%7s0 6 2 8 0 6 2 8 0 6 2 8 0 6 2"
                     " 8 0 6 2 8 0 6 2 8 0 6 2 8 h\r"
                     "%7sMonday  Tuesday Wednes. Thursd."
                     " Friday  Saturd. Sunday\r\r", " ", " ", " ");
#endif
      break;
    default:
#ifdef SPEECH
      putprintf(mbp, speech_message(130), " ");
#else
      putprintf(mbp, " Elap. time\r%6s-3600 -3240 -2880 -2520 -2160 "
                     "-1800 -1440 -1080 -720  -360  0 Seconds\r\r", " ");
#endif
  } /* switch */
}
#endif

/************************************************************************/
/* showgraph()                                                          */
/*                                                                      */
/* Funktion: Generiert einen Graph                                      */
/*                                                                      */
/* int timesel:      GRAPH_HOUR: Ueberblick 1 Stunde                    */
/*                   GRAPH_DAY:  Ueberblick 24 Stunden                  */
/*                   GRAPH_WEEK: Ueberblick 7 Tage                      */
/*                                                                      */
/* TGRAPHELEMENT select: Welcher Datenbestand ausgegeben werden soll    */
/*                                                                      */
/* char *titel           Ueberschrift des Diagrammes                    */
/*                                                                      */
/* char *presentation:   1. Char:  Minimum Darstellung                  */
/*                       2. Char:  Average Darstellung                  */
/*                       3. Char:  Maximum Darstellung                  */
/*                                                                      */
/* int expand:           0: Diagramm nicht strecken                     */
/*                       1: Diagramm zwischen Max und Min strecken      */
/*                                                                      */
/* MBHEAD *mhp:          Stringuebergabe                                */
/************************************************************************/
void
showgraph(int timesel, TGRAPHELEMENT *selected, const char *titel,
          char *prechar, int expand, MBHEAD *mbp)
{
  TGRAPHPEAK      peak[60];     /* 60 entspricht dem groessten Peak     */
                                /* Array aus TGRAPH                     */
  ULONG           raster = 0L;  /* noetig fuer Y-Achse (Werteachse)     */
  ULONG           wertmax,
                  wertave,
                  wertmin;      /* Werte aus peak[]                     */
  ULONG           maximum,
                  minimum,
                  average;      /* Maximalwerte von wertmax, wertmin,.. */
  ULONG           expandwert;   /* enthaelt minimum oder 0              */
  WORD            lastline = 1; /* bei expand=1, 0. Zeile mit ausgeben  */
  WORD            elements,
                  slot,
                  pos;
  WORD            averagenum;   /* gibt belegte peak[] Anzahl an        */
  int             lines = GRAPH_LINES;
  int             i,
                  j;
  int             start_min,   /* Fuer Anzeigeberechnung des Neustartes */
                  start_hour,
                  start_day;
  time_t          start_diff;
  char            zeile[128];   /* Leerzeilenballast entfernen          */

  switch (timesel)
  {
    case GRAPH_DAY:
      elements = GRAPH_DAY_ELEMENTS;
      for (i = 0; i < elements; i++)
        peak[i] = selected->day[i];
      slot = graph.day_slot;
      pos = graph.day_pos;
      break;
    case GRAPH_WEEK:
      elements = GRAPH_WEK_ELEMENTS;
      for (i = 0; i < elements; i++)
        peak[i] = selected->week[i];
      slot = graph.week_slot;
      pos = graph.week_pos;
      break;
    default:
      elements = GRAPH_STD_ELEMENTS;
      for (i = 0; i < elements; i++)
        peak[i] = selected->hour[i];
      slot = graph.hour_slot;
      pos = graph.hour_pos;
  }

  /* Titel uebergeben */
  putprintf(mbp, "%s", titel);

  /* Maximum,Mittel und Minimum Wert bestimmen */
  maximum = average = 0L;
  minimum = MAXULONG;
  averagenum = 0;

  for (i = 0; i < elements; i++)
  {
    wertmax = peak[i].max;
    wertave = (i != pos) ? peak[i].ave : (slot > 0) ? peak[i].ave / slot : 0;
    wertmin = peak[i].min;

    if (maximum < wertmax && wertmax != MAXULONG)
      maximum = wertmax;
    if (minimum > wertmin)
      minimum = wertmin;
    if (wertave != MAXULONG)
    {
      average += wertave;
      averagenum++;
    }
  }                             /* for elements */

  if (minimum == MAXULONG)
    minimum = 0;
  if (maximum == MAXULONG)
    maximum = 0;
  if (maximum > 0)
    putprintf(mbp, "  Maximum: %lu", (ULONG)maximum);
  if (averagenum > 0 && average > 0)
#ifdef SPEECH
      putprintf(mbp, speech_message(127), (ULONG)average / averagenum);
#else
      putprintf(mbp, "  Average: %lu", (ULONG)average / averagenum);
#endif
  putprintf(mbp, "  Minimum: %lu\r\r", (ULONG)minimum);

  /* expand=1: Alle Zeilen unter Minimum weggelassen. Es stehen somit   */
  /* 5 bis GRAPH_LINES Zeilen fuer die Werte zwischen Min und Max       */
  /* zur Verfuegung.                                                    */
  if (expand == 1)
    maximum -= minimum;

  /* Zeilenraster ermitteln */
  raster = maximum / GRAPH_LINES;
  if (raster == 0 || maximum % GRAPH_LINES > 0)
    raster++;

  /* genaue Zeilenanzahl ermitteln */
  lines = maximum / raster;     /* schwankt zwischen 5 und GRAPH_LINES */
  if (maximum % raster > 0)
    lines++;
  if (lines < 5)
    lines = 5;                  /* min. sollen es 5 Zeilen sein */

  /* Damit Minimalzeile im Expandmode mitangezeigt wird */
  if (expand == 1 && minimum > 0)
    lastline = 0;
  else
    lastline = 1;
  lines++;
  if (expand == 1)
    expandwert = minimum;
  else
    expandwert = 0;
  /* Die einzelnen Zeilen generieren */
  do
  {
    lines--;
    zeile[0] = NUL;
    putprintf(mbp, "%6lu|", (ULONG)raster * lines + expandwert);
    for (i = 0; i < elements; i++)
    {
      if (timesel == GRAPH_HOUR)
        j = (i + pos + 1 >= GRAPH_STD_ELEMENTS)
            ? pos + i + 1 - GRAPH_STD_ELEMENTS
            : i + pos + 1;
      else
        j = i;

      wertmax = peak[j].max;
      wertave = (j != pos)
                ? peak[j].ave
                : (slot > 0)
                  ? peak[j].ave / slot
                  : 0;
      wertmin = peak[j].min;

      if (wertmax >= raster * lines + expandwert && wertmax != MAXULONG)
      {
        putstr(zeile, mbp);
        if (wertave >= raster * lines + expandwert && wertave != MAXULONG)
        {
          if (wertmin >= raster * lines + expandwert && wertmin != MAXULONG)
            /* Kontrollzeichen abfangen */
#ifdef __WIN32__
            putchr((char)((prechar[0] < 32) ? '#' : prechar[0]), mbp);
#else
            putchr((prechar[0] < 32) ? '#' : prechar[0], mbp);
#endif /* WIN32 */
          else
#ifdef __WIN32__
            putchr((char)((prechar[1] < 32) ? '*' : prechar[1]), mbp);
#else
            putchr((prechar[1] < 32) ? '*' : prechar[1], mbp);
#endif /* WIN32 */
        }
        else
#ifdef __WIN32__
          putchr((char)((prechar[2] < 32) ? '+' : prechar[2]), mbp);
#else
          putchr((prechar[2] < 32) ? '+' : prechar[2], mbp);
#endif /* WIN32 */
          zeile[0] = NUL;
      }
      else
        strcat(zeile, " ");

    }                           /* for i<elements */

    putchr('\r', mbp);

  }
  while (lines != lastline);    /* naechste Zeile */

  /* untere Zeile generieren */
  putprintf(mbp, "%7s", "+");

  /* Neustart Datum ermitteln */
  start_min = localtime(&start_time)->tm_min;
  start_hour = localtime(&start_time)->tm_hour;
  start_day = (localtime(&start_time)->tm_wday != 0)
              ? localtime(&start_time)->tm_wday - 1
              : 6;
  start_diff = sys_time - start_time;

  for (i = 0; i < elements; i++)
  {
    j = (i + pos + 1 >= GRAPH_STD_ELEMENTS)
        ? pos + i + 1 - GRAPH_STD_ELEMENTS
        : i + pos + 1;

    if (   (   (timesel == GRAPH_DAY && i == graph.day_pos)
            || (timesel == GRAPH_WEEK && i == graph.week_pos))
        && graph.enabled == TRUE)
      putchr('A', mbp);
    else
      if (   (   timesel == GRAPH_HOUR
              && graph.enabled == TRUE
              && j == start_min
              && start_diff < 3600 - 60)
          || (   timesel == GRAPH_DAY
              && i == 2 * start_hour + start_min / 30
              && start_diff < 86400 - 1800)
          || (   timesel == GRAPH_WEEK
              && i == 8 * start_day + start_hour / 3
              && start_diff < 604800 - 10800))
        putchr('N', mbp);
      else
        putchr('-', mbp);
  }
  switch (timesel)
  {
    case GRAPH_DAY:
#ifdef SPEECH
      putprintf(mbp, speech_message(128), " ", " ");
#else
      putprintf(mbp, "\r%7s00  02  04  06  08  10  12  14  16  18  20  22\r"
                     "%9s01  03  05  07  09  11  13  15  17  19  21  23"
                     " Hour\r\r", " ", " ");
#endif
      break;
    case GRAPH_WEEK:
#ifdef SPEECH
      putprintf(mbp, speech_message(129), " ", " ", " ");
#else
      putprintf(mbp, "\r%7s0 0 1 1 0 0 1 1 0 0 1 1 0 0"
                     " 1 1 0 0 1 1 0 0 1 1 0 0 1 1\r"
                     "%7s0 6 2 8 0 6 2 8 0 6 2 8 0 6 2"
                     " 8 0 6 2 8 0 6 2 8 0 6 2 8 h\r"
                     "%7sMonday  Tuesday Wednes. Thursd."
                     " Friday  Saturd. Sunday\r\r", " ", " ", " ");
#endif
      break;
    default:
#ifdef SPEECH
      putprintf(mbp, speech_message(130), " ");
#else
      putprintf(mbp, " Elap. time\r%6s-3600 -3240 -2880 -2520 -2160 "
                     "-1800 -1440 -1080 -720  -360  0 Seconds\r\r", " ");
#endif
  }                           /* switch */
}

/************************************************************************/
/* Funktion graphtimer(void)                                            */
/*                                                                      */
/************************************************************************/
void
graph_timer(void)
{
  static int      graph_time = 0;
  int             min,
                  hour,
                  day;

  graph_time--;

  /* Alle GRAPH_INTERVAL Sekunden werden die Daten verarbeitet */
  if (graph_time <= 0L && graph.enabled == TRUE)
  {
    graph_time = GRAPH_INTERVAL;                /* Wartezeit neu setzen */

    min = sys_localtime->tm_min;
    hour = sys_localtime->tm_hour;
    day = (sys_localtime->tm_wday != 0) ? sys_localtime->tm_wday - 1 : 6;

    /* Jede Minute ein neues Datenfeld vorbereiten */
    /* Feldposition aktualisieren           dg9aml */

    if (graph.hour_pos != min)
    {
/* alte Spalte ist abgeschlossen, kann gemittelt werden                 */
      graph_average(GRAPH_HOUR);
/* neue Spalte vorbereiten, Werte zuruecksetzen                         */
      graph.hour_slot = 0;
/* Falls mehr als eine Min. vergangen ist, mehrere Elemente loeschen    */
      while ( min != graph.hour_pos)
      {
        graph.hour_pos++;
        if (graph.hour_pos == GRAPH_STD_ELEMENTS)
          graph.hour_pos = 0;
        graph_clear_element(GRAPH_HOUR, 1);        /* Komplett loeschen */
      }
      graph.hour_pos = min;
      graph_clear_element(GRAPH_HOUR, 0);
    }
    /* Jede halbe Stunde ein neues Datenfeld vorbeiten dg9aml */
    if (graph.day_pos != 2 * hour + min / 30)
    {
      graph_average(GRAPH_DAY);
      graph.day_slot = 0;
      graph.day_pos = 2 * hour + min / 30;
      graph_clear_element(GRAPH_DAY, 0);   /* loeschen der neuen Spalte */
    }

    /* Alle 3 Stunden ein neues Datenfeld vorbeiten */
    if (graph.week_pos != 8 * day + hour / 3)
    {
      graph_average(GRAPH_WEEK);
      graph.week_slot = 0;
      graph.week_pos = 8 * day + hour / 3;
      graph_clear_element(GRAPH_WEEK, 0);  /* loeschen der neuen Spalte */
    }
    graph_actual_element();    /* aktualisiert Element in hour,day,week */
    graph.hour_slot++;
    graph.day_slot++;
    graph.week_slot++;
  }                             /* if graph.enabled */
}

/************************************************************************/
/* Aktuelle Graph-Daten speichern.                                      */
/*                                                                      */
/* Rueckgabe:   TRUE:  Abspeichern erfolgreich                          */
/*              FALSE: Fehler beim Abspeichern                          */
/************************************************************************/
BOOLEAN
save_graph(void)
{
  FILE    *fp;                  /* File-Pointer                         */
  BOOLEAN  ret = TRUE;          /* Return-Wert                          */
  char     file[128];           /* Puffer fuer Filenamen                */

  sprintf(file, "%sGRAPH.STA", confpath);

  if ((fp = xfopen(file,"wb")) != NULL)              /* File oeffnen    */
  {
    if (fwrite(&graph, sizeof(graph), 1, fp) != 1)
    {
      ret = FALSE;
      xprintf("*** GRAPH.STA write error ***\n");
    }
    fclose(fp);                                      /* File schliessen */
    return(ret);
  }
  xprintf("*** GRAPH.STA open error ***\n");
  return(FALSE);
}

/************************************************************************/
/* Aktuelle Statistik-Daten laden.                                      */
/*                                                                      */
/* Rueckgabe:   TRUE bei erfolgreichem Einladen                         */
/*              FALSE bei Fehler                                        */
/************************************************************************/
BOOLEAN
load_graph(void)
{
  FILE    *fp;                  /* File-Pointer                         */
  LONG     filelength;          /* Laenge von GRAPH.STA                 */
  BOOLEAN  ret = TRUE;          /* Returnwert                           */
  char     file[MAXPATH+1];     /* Puffer fuer Filenamen                */

  sprintf(file, "%sGRAPH.STA", confpath);

  if ((fp = xfopen(file,"rb")) != NULL)             /* File oeffnen     */
  {
    fseek(fp, 0L, SEEK_END);            /* Dateilaenge ermitteln        */
    filelength = ftell(fp);
    rewind(fp);

    if (filelength != (LONG) sizeof(graph))
    {                                   /* Stimmt Dateilaenge?          */
      ret = FALSE;
      xprintf("*** GRAPH.STA has wrong size ***\n");
    }
    else
    {                                   /* Laenge stimmt                */
      if (fread(&graph, sizeof(graph), 1, fp) != 1)
      {
        ret = FALSE;
      }
    }
    fclose(fp);                         /* Datei schliessen             */
    return(ret);
  }
  /* GRAPH.STA konnte nicht geoeffnet werden */
  xprintf("*** GRAPH.STA - open error ***\n");
  return(FALSE);
}
#endif /* ifdef GRAPH */

/* End of src/graph.c */

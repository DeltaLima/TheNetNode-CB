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
/* File src/buffer.c (maintained by: DF6LN)                             */
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

#ifdef BUFFER_DEBUG
static void bufstat(void);
#endif

#ifdef INSANE_BUFFER_DEBUG
static void MBPTRCHECK(MBHEAD *);
#endif

static char huge *minmem(void);
static char huge *maxmem(void);

/************************************************************************\
*                                                                        *
* "initialize head"                                                      *
*                                                                        *
* Listenkopf, auf den hd zeigt, initialisieren :                         *
*                                                                        *
*                                                                        *
*                                              +----------------------+  *
*                                              | +------------------+ |  *
*                                              | |                  | |  *
*            +--------+                        v v   +--------+     | |  *
*     hd --->|        |         ->        hd ------->|        |-----+ |  *
*            +--------+                              +--------+       |  *
*            |        |                              |        |-------+  *
*            +--------+                              +--------+          *
*            |        |                              |        |          *
*                                                                        *
\************************************************************************/
void
inithd(LHEAD *hd)
{
  hd->head = hd->tail = hd;
}

/************************************************************************/
/* Anfang des freien Speichers                                          */
/*----------------------------------------------------------------------*/
static char huge *
minmem(void)
{
  return (RAMBOT);
}

/************************************************************************/
/* Ende des freien Speichers                                            */
/*----------------------------------------------------------------------*/
static char huge *
maxmem(void)
{
  return (RAMTOP);
}

/************************************************************************/
/* Buffer-Verwaltung initialisieren.                                    */
/*----------------------------------------------------------------------*/
void
init_buffers(void)
{
  MAX_BUFFER huge *actbp;
  ULONG           n;

  nmblks = nmbfre = nmblks_max = 0;

  actbp = (MAX_BUFFER *)minmem();
  n = (ULONG)((maxmem() - minmem()) / sizeof(MAX_BUFFER));

#ifdef __WIN32__
  nmbfre_max = (unsigned short)n;
#else
  nmbfre_max = n;
#endif /* WIN32 */

  while (n--)
  {
#ifdef BUFFER_DEBUG
    ((MBHEAD *)actbp)->owner = ALLOC_LEHEAD;
#endif
    dealoc((MBHEAD *)actbp++);
  }

  nmbfre_min = nmbfre;
}

/************************************************************************\
*                                                                        *
* action      :  Element aus Liste aushaengen.                           *
*                                                                        *
*                                                                        *
*                      le ---+                                           *
*              vor           |     raus                hinter            *
*            +--------+      +-->+--------+          +--------+          *
*      ----->|        |--------->| prevle |--------->|        |----->    *
*            +--------+          +--------+          +--------+          *
*      <-----|        |<---------| nextle |<---------|        |<-----    *
*            +--------+          +--------+          +--------+          *
*            |        |          |        |          |        |          *
*                                                                        *
*                                                                        *
*                                \/                                      *
*                                                                        *
*                                                                        *
*             raus                    vor             hinter             *
*           +--------+              +--------+      +--------+           *
*    le --->| nextle |        ----->|        |----->|        |----->     *
*           +--------+              +--------+      +--------+           *
*           | prevle |        <-----|        |<-----|        |<-----     *
*           +--------+              +--------+      +--------+           *
*           |        |              |        |      |        |           *
*                                                                        *
*                                                                        *
 ************************************************************************
*                                                                        *
* parameter   :  le      - Zeiger auf auszuhaengendes Listenelement      *
*                                                                        *
* returns     :  le                                                      *
*                                                                        *
\************************************************************************/
LEHEAD *
ulink(LEHEAD *le)
{
  le->prevle->nextle = le->nextle;    /* Hinliste ohne le               */
  le->nextle->prevle = le->prevle;    /* Rueckliste ohne le             */
  return (le);                        /* Zeiger auf das Element zurueck */
}

/************************************************************************\
*                                                                        *
* action      :  Element in Liste einhaengen.                            *
*                                                                        *
*                                                                        *
*             neu                     vor             hinter             *
*           +--------+              +--------+      +--------+           *
*   new --->| nextle |     pred --->|        |----->|        |----->     *
*           +--------+              +--------+      +--------+           *
*           | prevle |        <-----|        |<-----|        |<-----     *
*           +--------+              +--------+      +--------+           *
*           |        |              |        |      |        |           *
*                                                                        *
*                                                                        *
*                                \/                                      *
*                                                                        *
*                                                                        *
*                     new ---+                                           *
*              vor           |     neu                 hinter            *
*            +--------+      +-->+--------+          +--------+          *
*   pred --->|        |--------->| prevle |--------->|        |----->    *
*            +--------+          +--------+          +--------+          *
*      <-----|        |<---------| nextle |<---------|        |<-----    *
*            +--------+          +--------+          +--------+          *
*            |        |          |        |          |        |          *
*                                                                        *
*                                                                        *
 ************************************************************************
*                                                                        *
* parameter   :  new     - Zeiger auf einzuhaengendes Listenelement      *
*                pred    - Zeiger auf Listenelement, hinter dem new      *
*                          eingehaengt werden soll                       *
*                                                                        *
* returns     :  new                                                     *
*                                                                        *
\************************************************************************/
LEHEAD *
relink(LEHEAD *new, LEHEAD *pred)
{
  new->nextle = pred->nextle;           /* Vorzeiger im neuen Element   */
  new->prevle = pred;                   /* Rueckzeiger im neuen Element */
  new->nextle->prevle = new;            /* Rueckzeiger dahinter         */
  pred->nextle = new;                   /* Vorzeiger davor              */
  return (new);                         /* Zeiger auf neues Element     */
}

/************************************************************************\
*                                                                        *
* action      :  "allocate buffer"                                       *
*                                                                        *
*                Leeren Buffer aus der Freiliste holen, Programmneustart *
*                wenn keine Buffer mehr in Freiliste.                    *
*                                                                        *
 ************************************************************************
*                                                                        *
* r/w globals :  nmbfre  - Anzahl der Buffer in Freiliste freel          *
*                freel   - verkettete Liste der freien Buffer            *
*                                                                        *
* locals      :  s.u.                                                    *
*                                                                        *
* returns     :  Zeiger auf freien Buffer, fall vorhanden                *
*                                                                        *
\************************************************************************/
LEHEAD *
#ifdef BUFFER_DEBUG
allocb(int owner)
#else
allocb(void)
#endif
{
  LEHEAD         *ret;                 /* Buffer Rueckgabewert          */

  nmbfre--;                            /* 1 Buffer weniger              */
  if (nmbfre < nmbfre_min)
    nmbfre_min = nmbfre;
  if (!nmbfre)                         /* wenn nicht genug frei         */
  {
#ifdef BUFFER_DEBUG
    bufstat();                         /* Buffernutzung protokollieren  */
#endif
    HALT("allocb: no free buffers !"); /* dann Rechner neu starten..    */
  }
  ret = ulink((LEHEAD *)freel.head);   /* Buffer aus Liste aushaengen   */
#ifdef BUFFER_DEBUG
  if (ret->owner != ALLOC_NO_OWNER)
  {
    bufstat();                         /* Buffernutzung protokollieren  */
    HALT("allocb: buffer with owner in free-list !");
  }

  ret->owner = (UBYTE)owner;
#endif
  return (ret);                        /* Zeiger auf Freibuffer zurueck */
}

/************************************************************************\
*                                                                        *
* "deallocate"                                                           *
*                                                                        *
* Buffer, auf den bp zeigt, initialisieren als neuen Messagebufferhead   *
* (rwndmb()) und deallokieren, d.h. in die Freiliste freel einhaengen    *
* und den Freibufferzaehler nmbfre inkrementieren.                       *
*                                                                        *
*                                                                        *
*            +--------+                                                  *
*     bp --->|        |         deallokieren                             *
*            +--------+                                                  *
*            |        |                                                  *
*            +--------+                                                  *
*            |        |                                                  *
*                                                                        *
\************************************************************************/
void
dealoc(MBHEAD *bp)
{
#ifdef INSANE_BUFFER_DEBUG
  MBPTRCHECK(bp);
#endif

  bp->mbl.head =                /* als Messagehead initialisieren       */
    bp->mbl.tail =              /* Bufferlistenkopf                     */
    &bp->mbl;                   /* initialisieren                       */
  bp->mbpc = 0;                 /* Message leer                         */
  bp->mbgc = 0;                 /* Rest initialisieren                  */
  bp->mbbp = NULL;
#ifdef BUFFER_DEBUG
  if (bp->owner != ALLOC_NO_OWNER)
    bp->owner = ALLOC_NO_OWNER;
  else
    HALT("dealoc: buffer with no owner !"); /* Rechner neu starten..    */
#endif
  rwndmb(bp);                   /* Buffer an Freiliste anhaengen        */
  relink((LEHEAD *)bp, (LEHEAD *)freel.tail);
  ++nmbfre;                     /* 1 Freibuffer mehr                    */
  if (nmbfre > nmbfre_max)      /* > nmbfre_max darf nicht sein!        */
    HALT("dealoc: more buffers free than allocated !"); /* neu starten  */
}

/************************************************************************\
*                                                                        *
* "deallocate message buffer"                                            *
*                                                                        *
* Einen kompletten Messagespeicher, auf dessen Kopf mbhd zeigt,          *
* deallokieren, d.h. sowohl den Messagebufferhead als auch alle an       *
* dessen Messagebufferliste haengende Datenbuffer deallokieren.          *
*                                                                        *
*                                                                        *
*            +--------+           deallokieren                           *
*    mbhd -->|        |                                                  *
*            +--------+                                                  *
*            |        |                                                  *
*            +--------+      +--------+                +--------+        *
*      a --->|        |----->|        |--->        --->|        |---> a  *
*            +  mbl   +      +--------+      ...       +--------+        *
*      b <---|        |<-----|        |<---        <---|        |<--- b  *
*            +--------+      +--------+                +--------+        *
*            |        |      |        |                |        |        *
*                                                                        *
\************************************************************************/
void
dealmb(MBHEAD *mbhd)
{
  MB             *bp;                           /* Datenbufferzeiger    */

#ifdef INSANE_BUFFER_DEBUG
  MBPTRCHECK(mbhd);
#endif

/* alle Datenbuffer                                                     */
  while ((bp = (MB *)mbhd->mbl.head) != (MB *)&mbhd->mbl)
    dealoc((MBHEAD *)ulink((LEHEAD *)bp));
  dealoc(mbhd);                                 /* Am Ende den Kopf     */
}

/************************************************************************\
*                                                                        *
* "deallocate message list"                                              *
*                                                                        *
* Komplette Messageliste, auf deren Listenkopf mlp zeigt, deallokieren.  *
* D.h. alle Messagespeicher (jeweils Kopf und daran haengende            *
* Datenbuffer) deallokieren.                                             *
*                                                                        *
*                                                                        *
*            +--------+    +--------+               +--------+           *
*    mlp --->| head   |--->|        |--->       --->|        |---> mlp   *
*            +--------+    +--------+      ...      +--------+           *
*      b <---| tail   |<---|        |<---       <---|        |<--- b     *
*            +--------+    +--------+               +--------+           *
*                          |        |---> \         |        |---> \     *
*                          +        +      |        +        +      |    *
*                          |        |<--- /|        |        |<--- /|    *
*                          +--------+      |        +--------+      |    *
*                          |        |      |        |        |      |    *
*                                          |                        |    *
*      deallokieren                        |------------------------|    *
*                                            siehe unten dealmb()        *
*                                                                        *
\************************************************************************/
/*#include "coredump.c" */
void
dealml(LEHEAD *mlp)
{
  MBHEAD         *bp;           /* Zeiger auf Messagebufferhead         */

#if 0
  int             i;

  for (bp = (MBHEAD *)mlp->nextle, i = 0;
       bp != (MBHEAD *)mlp;
       bp = (MBHEAD *)bp->nextmh)
    if (++i > 10000)
    {                           /* Fehler !!!                           */
      coredump();
      i = *((char *)NULL);      /* Zugriff auf NULL-Pointer             */
      exit(-1);                 /* notfalls normaler exit               */
    }
#endif

  LOOP                          /* fuer alle Messagebufferheads         */
  {                             /* in Messagespeicherliste :            */
    bp = (MBHEAD *)mlp->nextle; /* Zeiger auf naechsten Msgbhead        */
    if (mlp == (LEHEAD *)bp)    /* Schwanz beisst Kopf -> fertig        */
      break;                    /* sonst Messagespeicher deallok.       */
    dealmb((MBHEAD *)ulink((LEHEAD *)bp));
  }
}

/************************************************************************\
*                                                                        *
* action    :  "rewind message buffer"                                   *
*                                                                        *
*              Message-Buffer (Kopf und Datenbufferliste) zuruecksetzen. *
*              Get-Counter auf 0 setzen, Buffer-Pointer so setzen, dass  *
*              beim naechsten getchr() auf das das erste Datenbyte des   *
*              ersten Datenbuffers zugegriffen wird.                     *
*              (Dies muss wie folgt geschehen, da der Get-Counter in     *
*              jedem Fall auf 0 stehen muss und in getchr() auf % 32     *
*              fuer das Positionieren auf den naechsten Datenbuffer      *
*              abgetestet wird.)                                         *
*                                                                        *
 ************************************************************************
*                                                                        *
* parameter :  mbhd    - Zeiger auf Kopf des Message-Buffers             *
*                                                                        *
\************************************************************************/
void
rwndmb(MBHEAD *mbhd)
{
#ifdef INSANE_BUFFER_DEBUG
  MBPTRCHECK(mbhd);
#endif

/* Kopf wie Datenbufferende                                             */
  mbhd->mbbp = (char *)(((MAX_BUFFER huge *)&(mbhd->mbl)) + 1);
  mbhd->mbgc = 0;                                       /* Bufferanfang */
}

/************************************************************************\
*                                                                        *
* action     : "put character"                                           *
*                                                                        *
*              Zeichen in Messagebuffer schreiben, Put-Counter erhoehen  *
*              und Buffer-Pointer setzen. Ist der aktuelle Datenbuffer   *
*              im Messagebuffer voll, dann neuen Datenbuffer allokieren  *
*              und ans Datenbufferlistenende des Messagebuffers          *
*              anhaengen.                                                *
*                                                                        *
 ************************************************************************
*                                                                        *
* parameter  : ch      - in den Buffer zu schreibendes Zeichen           *
*              mbhd    - Zeiger auf den Messagebuffer-Kopf, in den ch    *
*                        zu schreiben ist                                *
*                                                                        *
* r/o globals: -                                                         *
*                                                                        *
* r/w globals: -                                                         *
*                                                                        *
* locals     : -                                                         *
*                                                                        *
* returns    : -                                                         *
*                                                                        *
\************************************************************************/
void
putchr(char ch, MBHEAD *mbhd)
{
#ifdef INSANE_BUFFER_DEBUG
  MBPTRCHECK(mbhd);
#endif

  if (mbhd->mbpc++ % sizeof_MBDATA == 0)
    mbhd->mbbp = ((MB *)(relink((LEHEAD *)allocb(ALLOC_LEHEAD),
                                (LEHEAD *)mbhd->mbl.tail)))->data;

  *mbhd->mbbp++ = ch;
}

/************************************************************************\
*                                                                        *
* action     : "get character"                                           *
*                                                                        *
*              Zeichen aus einem Messagebuffer holen. Datenbuffer-Poiner *
*              setzen und Get-Count erhoehen. Uebergang in der           *
*              Datenbufferliste vom Ende eines Datenbuffers zum          *
*              naechsten ausfuehren.                                     *
*                                                                        *
 ************************************************************************
*                                                                        *
* parameter  : mbhd    - Zeiger auf Kopf des Messagebuffers, aus dem     *
*                        das Zeichen gelesen werden soll                 *
*                                                                        *
* r/o globals: -                                                         *
*                                                                        *
* r/w globals: -                                                         *
*                                                                        *
* locals     : -                                                         *
*                                                                        *
* returns    : aus dem Buffer gelesenes Zeichen                          *
*                                                                        *
\************************************************************************/
#ifdef __WIN32__
unsigned char
#else
char
#endif /* WIN32 */
getchr(MBHEAD *mbhd)
{
#ifdef INSANE_BUFFER_DEBUG
  MBPTRCHECK(mbhd);
#endif

  if (mbhd->mbgc++ % sizeof_MBDATA == 0)
    mbhd->mbbp = ((MB *)((MAX_BUFFER huge *)(mbhd->mbbp) - 1))->nextmb->data;
  return (*mbhd->mbbp++);
}

#ifndef MC68K
/************************************************************************
 * Function        : 16 Bits aus einem Buffer lesen
 *
 * Inputs        : Zeiger auf den Buffer
 *
 * Returns        : unsigned integer
 *
 * Operation    : Liest 2 Bytes und vertauscht sie
 *----------------------------------------------------------------------*/
UWORD
get16(MBHEAD *mbhd)
{
  UWORD           retval;

#ifdef INSANE_BUFFER_DEBUG
  MBPTRCHECK(mbhd);
#endif

  retval = ((UWORD)(getchr(mbhd)) << 8);
  retval |= (UWORD)getchr(mbhd);
  return (retval);
}

/************************************************************************
 * Function       : Schreibt ein Word in den Buffer
 *
 * Inputs        : Der Wert (Word), Zeiger auf den Buffer
 *
 * Returns        : nothing
 *
 * Operation    :
 *----------------------------------------------------------------------*/
void
put16(UWORD value, MBHEAD *mbhd)
{
#ifdef INSANE_BUFFER_DEBUG
  MBPTRCHECK(mbhd);
#endif

  putchr((BYTE)((value >> 8) & 0xff), mbhd);
  putchr((BYTE)(value & 0xff), mbhd);
}

/************************************************************************
 * Function        : 32 Bits aus einem Buffer lesen
 *
 * Inputs        : Zeiger auf den Buffer
 *
 * Returns        : unsigned long integer
 *
 * Operation    : Liest 2 Words und vertauscht sie
 *----------------------------------------------------------------------*/
ULONG
get32(MBHEAD *mbhd)
{
  ULONG           retval;

#ifdef INSANE_BUFFER_DEBUG
  MBPTRCHECK(mbhd);
#endif

  retval = ((ULONG)(getchr(mbhd)) << 24);
  retval |= ((ULONG)(getchr(mbhd)) << 16);
  retval |= ((ULONG)(getchr(mbhd)) << 8);
  retval |= (ULONG)getchr(mbhd);
  return (retval);
}

/************************************************************************
 * Function       : Schreibt ein Long in den Buffer
 *
 * Inputs        : Der Wert (Long), Zeiger auf den Buffer
 *
 * Returns        : nothing
 *
 * Operation    :
 *----------------------------------------------------------------------*/
void
put32(ULONG value, MBHEAD *mbhd)
{
#ifdef INSANE_BUFFER_DEBUG
  MBPTRCHECK(mbhd);
#endif

  putchr((BYTE)((value >> 24) & 0xff), mbhd);
  putchr((BYTE)((value >> 16) & 0xff), mbhd);
  putchr((BYTE)((value >> 8) & 0xff), mbhd);
  putchr((BYTE)(value & 0xff), mbhd);
}
#else
UWORD
get16(MBHEAD *mbhd)
{
  UWORD           retval;
  char           *r = (UBYTE *)&retval;
#if defined(BUFFER_DEBUG) && defined(__LINUX__) || defined(__WIN32__)
  MBPTRCHECK(mbhd);
#endif

  *r++ = getchr(mbhd);
  *r = getchr(mbhd);
  return (retval);
}

void
put16(UWORD value, MBHEAD *mbhd)
{
  char           *r = (UBYTE *)&value;
#if defined(BUFFER_DEBUG) && defined(__LINUX__) || defined(__WIN32__)
  MBPTRCHECK(mbhd);
#endif

  putchr(*r++, mbhd);
  putchr(*r, mbhd);
}

ULONG
get32(MBHEAD *mbhd)
{
  ULONG           retval;
  char           *r = (UBYTE *)&retval;
#if defined(BUFFER_DEBUG) && defined(__LINUX__) || defined(__WIN32__)
  MBPTRCHECK(mbhd);
#endif

  *r++ = getchr(mbhd);
  *r++ = getchr(mbhd);
  *r++ = getchr(mbhd);
  *r = getchr(mbhd);
  return (retval);
}

void
put32(ULONG value, MBHEAD *mbhd)
{
  char           *r = (UBYTE *)&value;
#if defined(BUFFER_DEBUG) && defined(__LINUX__) || defined(__WIN32__)
  MBPTRCHECK(mbhd);
#endif

  putchr(*r++, mbhd);
  putchr(*r++, mbhd);
  putchr(*r++, mbhd);
  putchr(*r, mbhd);
}
#endif

/************************************************************************\
*                                                                        *
* "split copy"                                                           *
*                                                                        *
* Die Bytes aus dem Messagebuffer, auf dessen Kopf mbhd zeigt, in den    *
* Framebuffer, auf dessen Kopf fbp zeigt, kopieren. Es werden hoechstens *
* max Bytes kopiert, hat die Message mehr Bytes, so wird ein neuer       *
* Messagebuffer angelegt, die restlichen Messagebytes werden in diesen   *
* Buffer kopiert, der neue Buffer wird hinter den alten Messagebuffer    *
* gehaengt, der Putcount des alten Buffers wird auf max gestellt, das    *
* "more follows"-Flag morflg des neuen Buffers wird geloescht, l2fflg    *
* wird uebertragen.                                                      *
*                                                                        *
* Return :  TRUE  - der Messagebuffer wurde aufgesplittet                *
*           FALSE - sonst                                                *
*                                                                        *
\************************************************************************/

BOOLEAN
splcpy(WORD max, MBHEAD *fbp, MBHEAD *mbhd)
{
  char huge      *mbbpsa;             /* Sicherung mbbp                 */
  BOOLEAN         split;              /* TRUE: Split erfolgt            */
  WORD            mbgcsa;             /* Sicherung mbgc                 */
  WORD            mbgc2;              /* mbgc alt -> mbpc alt           */
  WORD            n;                  /* Zaehler                        */
  MBHEAD         *mbhd2;              /* Kopfzeiger neuer Messagebuffer */

  split = FALSE;                      /* zunaechst nichts gesplittet    */
  mbbpsa = mbhd->mbbp;                /* Bufferpointer sichern          */
  mbgcsa = mbhd->mbgc;                /* Getcounter sichern             */
  for (n = 0; mbhd->mbgc < mbhd->mbpc && n < max; ++n)
    putchr(getchr(mbhd), fbp);        /* maximal max Bytes kopieren     */
  if (mbhd->mbgc < mbhd->mbpc)        /* noch Bytes ueber -> Split !    */
  {
    mbgc2 = mbhd->mbgc;               /* Getcount fuer spaeter merken   */
    mbhd2 = (MBHEAD *)allocb(ALLOC_MBHEAD); /* neuen Buffer erzeugen    */
    while (mbhd->mbgc < mbhd->mbpc)   /* die restlichen Bytes in diesen */
      putchr(getchr(mbhd), mbhd2);    /* Buffer kopieren                */
    rwndmb(mbhd2);                    /* neuen Buffer rewinden          */
    mbhd2->morflg = FALSE;            /* noch dem neuen folgt keiner    */
    mbhd2->l2fflg = mbhd->l2fflg;     /* Frameflag uebertragen          */
    mbhd2->repeated = 0;              /* noch nicht erneut gesendet     */
    relink((LEHEAD *)mbhd2,
           (LEHEAD *)mbhd);           /* neu. Buf. hinter alten haengen */
    mbhd->mbpc = mbgc2;               /* alter Buffer nur max Zeichen ! */
    split = TRUE;                     /* wir mussten splitten           */
  }
  mbhd->mbbp = mbbpsa;                /* Bufferpointer restaurieren     */
  mbhd->mbgc = mbgcsa;                /* Getcount restaurieren          */
  return (split);                     /* Split oder nicht               */
}

#ifdef BUFFER_DEBUG
/* Schreibt die aktuelle Buffer-Benutzung in eine Datei */
/* Wichtig: hier nichts benutzen, was irgendwie wieder  */
/* neue Buffer belegen oder freigeben wuerde !          */
static
void bufstat(void)
{
  MAX_BUFFER huge *actbp;
  ULONG            i, n;
  ULONG            used[ALLOC_MAXELEMENTE + 1];
  FILE            *fp;

  if ((fp = fopen("buffer.log", "a+")) != NULL)
  {
    for (i = 0; i < ALLOC_MAXELEMENTE + 1; i++)
      used[i] = 0;

    actbp = (MAX_BUFFER *)RAMBOT;
    n = (ULONG)((RAMTOP - RAMBOT) / sizeof(MAX_BUFFER));

    fprintf(fp, "\rTotal Buffers : %ld (%u free) at %u Bytes each\r", n, nmbfre_max, (unsigned int)sizeof(MAX_BUFFER));

    while (n--)
    {
      if ((UBYTE) ((USRBLK *)actbp)->owner >= ALLOC_MAXELEMENTE)
      {
        used[ALLOC_MAXELEMENTE]++;
        fprintf(fp,"Wrong Owner: Buffer:%lu Owner:%d\r", n, (UBYTE) ((USRBLK *)actbp)->owner);
      }
      else
       used[(UBYTE) ((USRBLK *)actbp)->owner]++;
      actbp++;
    }

    fprintf(fp, "Free   : by owner %lu, by counter %u\r", used[ALLOC_NO_OWNER], nmbfre);
    fprintf(fp, "LEHEAD : %lu\r", used[ALLOC_LEHEAD]);
    fprintf(fp, "MBHEAD : %lu\r", used[ALLOC_MBHEAD]);
    fprintf(fp, "USRBLK1: %lu\r", used[ALLOC_USRBLK1]);
    fprintf(fp, "USRBLK2: %lu\r", used[ALLOC_USRBLK2]);
    fprintf(fp, "L2LINK : %lu\r", used[ALLOC_L2LINK]);
    fprintf(fp, "MB     : %lu\r", used[ALLOC_MB]);
    fprintf(fp, "MONBUF : %lu\r", used[ALLOC_MONBUF]);
    fprintf(fp, "CQBUF  : %lu\r", used[ALLOC_CQBUF]);
    fprintf(fp, "IPROUTE: %lu\r", used[ALLOC_IP_ROUTE]);
    fprintf(fp, "ARPTAB : %lu\r", used[ALLOC_ARP_TAB]);
    fprintf(fp, "MHEARD : %lu\r", used[ALLOC_MHEARD]);
    fprintf(fp, "PACSAT : %lu\r", used[ALLOC_PACSATBLK]);
#ifdef TCP_STACK
    fprintf(fp, "TCPSTA : %lu\r", used[ALLOC_TCPSTACK]);
#endif /* TCP_STACK */
#ifdef L1TCPIP
    fprintf(fp, "TCPIP  : %lu\r", used[ALLOC_L1TCPIP]);
#endif /* L1TCPIP */
#ifdef L1HTTPD
    fprintf(fp, "HTTPDRX: %lu\r", used[ALLOC_L1HTTPD_RX]);
    fprintf(fp, "HTTPDTX: %lu\r", used[ALLOC_L1HTTPD_TX]);
#endif /* L1HTTPD */
    fprintf(fp, "INPOPT : %lu\r", used[ALLOC_INPOPT]);

    if (used[0] != 0L)
      fprintf(fp, "???    : %lu\r", used[0]);

    fprintf(fp, "Errors : %lu\r", used[ALLOC_MAXELEMENTE]);

    fclose(fp);
  }
}

#ifdef INSANE_BUFFER_DEBUG
/************************************************************************/
/* Log-Funktion zum Debugging                                           */
/************************************************************************/
static void
TOLOG(const char *format, ...)
{
  FILE *fp;
  va_list arg_ptr;
  struct timeval tv;
  static char str[30];
  char *ptr;

  if ((fp = fopen("buffer.log", "a+")) != NULL)
  {
    gettimeofday(&tv, NULL);
    ptr = ctime(&tv.tv_sec);
    strcpy(str, &ptr[11]);
    fprintf(fp, "%s:", str);

    va_start(arg_ptr, format);
    vfprintf(fp, format, arg_ptr);
    va_end(arg_ptr);

    fprintf(fp, "\n");
    fclose(fp);
  }
}

/************************************************************************\
* Prueft, ob der uebergebene Pointer im Bufferbereich liegt              *
\************************************************************************/
static void
MBPTRCHECK(MBHEAD* checkme)
{
  /* Ist der uebergebene Zeiger im gueltigen Bereich ? */
  if (((char*)checkme < RAMBOT) || ((char*)checkme > RAMTOP))
  {
    TOLOG("An error occured while accessing one of my buffers, the\r");
    TOLOG("desired buffer at adress %p is outside of my allocted\r", checkme);
    TOLOG("memory range from %p to %p (%ld bytes) !\r", RAMBOT, RAMTOP, (RAMTOP - RAMBOT));
    TOLOG("Here is the actual usage of the buffer system:\r");

    bufstat();

    TOLOG("I will crash now for conservation of the stack ...\r");
    HALT("MBPTRCHECK: crash !\r");
  }
}
#endif /* INSANE_BUFFER_DEBUG */

#endif /* BUFFER_DEBUG */

/* End of src/buffer.c */

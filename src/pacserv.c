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
/* File src/pacserv.c (maintained by: ???)                              */
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
#ifdef PACSAT

#define    blocksize 244                        /* Info+CRC abziehen    */

#define    PF_O                            0x02
#define    PF_E                            0x20
#define    PF_N                            0x40

/* Header ID's of mandatory Headers */
#define    PH_HEADER_END                   0
#define    PH_FILE_NUMBER                  1
#define    PH_FILE_NAME                    2
#define    PH_FILE_EXT                     3
#define    PH_FILE_SIZE                    4
#define    PH_CREATE_TIME                  5
#define    PH_LAST_MODIFIED_TIME           6
#define    PH_SEU_FLAG                     7
#define    PH_FILE_TYPE                    8
#define    PH_BODY_CHECKSUM                9
#define    PH_HEADER_CHECKSUM              10
#define    PH_BODY_OFFSET                  11

/* Header ID's of extended Headers */
#define    PH_SOURCE                       16
#define    PH_AX25_UPLOADER                17
#define    PH_UPLOAD_TIME                  18
#define    PH_DOWNLOAD_COUNT               19
#define    PH_DESTINATION                  20
#define    PH_AX25_DOWNLOADER              21
#define    PH_DOWNLOAD_TIME                22
#define    PH_EXPIRE_TIME                  23
#define    PH_PRIORITY                     24

/* Header ID's of optional Headers */
#define    PH_COMPRESSION_TYPE             32
#define    PH_BBS_MESSAGE_TYPE             33
#define    PH_BULLETIN_ID_NUMBER           34
#define    PH_TITLE                        35
#define    PH_KEYWORDS                     36
#define    PH_FILE_DESCRIPTION             37
#define    PH_COMPRESSION_DESCRIPTION      38
#define    PH_USER_FILE_NAME               39

typedef struct pactxdesc
{
    LONG                 fid;
    FILE                *filehandle;
    LONG                 filesize;
    LONG                 filetime;
    UWORD                port;
    LONG                 pos;
    const char          *destcall;
    const char          *via;
} PACTXDESC;

typedef struct pacstatedesc
{
    WORD                 busy;
    WORD                 last;
    LONG                 actfid;
    LONG                 fid200;
    PACTXDESC            now;
} PACSTATEDESC;

ULONG        pacsat_delay[L2PNUM];
PACSTATEDESC pacstate[L2PNUM];

static char *make_name(LONG, char *);
static void garbage(void);
static UWORD calc_crc(char, UWORD);
static LONG fsize(char *);
static WORD init_bcast(LONG, char, PACTXDESC *);
static void add_frm(MBHEAD *, char *, WORD);
static void add_crc(MBHEAD *);
static BOOLEAN send_frame(PACTXDESC *);
static BOOLEAN send_bcast2(WORD);
static void send_bcast(WORD);

/************************************************************************/
/*                                                                      */
/* Filesystem-Dateiname fuer FID x                                      */
/*                                                                      */
/************************************************************************/
static char *make_name(LONG fid, char *s)
{
  sprintf(s, "%s%lx.DAT", pacsatpath, fid);
  normfname(s);
  return(s);
}

/************************************************************************/
/*                                                                      */
/* Nachrichten werden als laufende Nummern gespeichert. Beim Start von  */
/* TheNetNode werden die 1. und letzte Nummer ermittelt. In der Mitte   */
/* sollten keine Nachrichten fehlen.                                    */
/*                                                                      */
/************************************************************************/
void filesystem_init(void)
{
  struct ffblk fb;
  char         file[MAXPATH];
  char         search[MAXPATH];
  LONG         fid;
  WORD         next, x;

/* nach einem Absturz falls die CONFIG nicht gespeichert wurde          */
  do
  {
    last_fid++;
    make_name(last_fid, file);
  } while (!xaccess(file, 0));

  if (xaccess(make_name(first_fid, file), 0) ||
      xaccess(make_name(last_fid, file), 0))
  {
/* neu-Abgleich des File-Systems                                        */
    xprintf("--- working PACSAT-Files ...\n");
    first_fid = LONG_MAX;
    last_fid = 1L;

    strcpy (file,pacsatpath);
    strcat (file,"*.DAT");
    normfname(file);
    next = 0;

    if (xfindfirst(file, &fb, 0) == 0)
    {
      while (next == 0)
      {
        strcpy(search, "%lx.DAT");
        normfname(search);
        sscanf(fb.ff_name, search, &fid);
        if (fid > last_fid)
          last_fid = fid;
        if (fid < first_fid)
          first_fid = fid;
        if ((next = xfindnext(&fb)) != 0)
          break;
      }
    }

    if (first_fid == LONG_MAX)
    {
      first_fid = sys_time;
      last_fid = first_fid - 1L;
    }
  }
/* die Ports initialisieren                                             */
  for (x = 0; x < L2PNUM; x++)
  {
    pacstate[x].busy = FALSE;
    pacstate[x].last = FALSE;
    pacstate[x].actfid = first_fid - 1;
    pacstate[x].fid200 = last_fid;
    pacsat_delay[x] = 0;
  }
}

/************************************************************************/
/*                                                                      */
/* Garbage-Collection der Mailbox                                       */
/*                                                                      */
/************************************************************************/
static void garbage(void)
{
  char name[MAXPATH];

  while (first_fid + (LONG) pacsat_free <= last_fid)
  {
    make_name(first_fid++, name);
    xremove(name);
  }

  if (first_fid >= last_fid) filesystem_init();
}

/************************************************************************/
/*                                                                      */
/* PACSAT-Server initialisiern                                          */
/*                                                                      */
/************************************************************************/
void pacsat_init(void)
{
  filesystem_init();
  /*garbage();*/
}

static UWORD calc_crc(char c, UWORD crc)
{
        UWORD hi1, hi2;

        hi1 = ((crc >> 8) & 0xff);
        hi2 = ((crc & 0xff) << 8);
        return((crctab[hi1 ^ c] ^ hi2));
}

/************************************************************************/
/*                                                                      */
/* Dateigroesse ermitteln                                               */
/*                                                                      */
/************************************************************************/
static LONG fsize(char *name)
{
  struct stat s;

  stat(name,&s);
  return(s.st_size);
}

/************************************************************************/
/*                                                                      */
/* Das Temporaerfile "tempname" ins PACSAT-Fileformat uebernehmen. Alle */
/* PACSAT-Nachrichten werden im pacsatpath Verzeichnis gespeichert. Der */
/* Dateiname ist die 8-Stellige FileId (Hex, long) und die Endung .DAT  */
/*                                                                      */
/************************************************************************/
void new_file(char *tempname)
{
  char e_name[MAXPATH];
  char file1[MAXPATH], file2[MAXPATH];
  char s[20], s2[20];

/* Neueingaenge aus dem Hintergrund, z.B. Netzwerk                      */
  do
  {
    last_fid++;
    make_name(last_fid, file1);
  } while (!xaccess(file1, 0));

/* jetzt den Header hinzufuegen                                         */
  strcpy(file2, tempname);
  strcpy(e_name, pacsatpath);
  strcat(e_name, "PFHADD");
#ifndef __LINUX__
  strcat(e_name, ".EXE");
#endif
  normfname(e_name);
  sprintf(s, "%lx", last_fid);
  callss2str(s2, calofs(UPLINK, userpo->uid));

  if (spawnl(P_WAIT, e_name, e_name, file2, file1, s, s2, NULL) != 0)
    last_fid--;
  else garbage();
}

/************************************************************************/
/* PacSat-Broadcast vorbereiten, Broadcast-Struktur fuellen             */
/************************************************************************/
static WORD init_bcast(LONG fid, char port, PACTXDESC *tp)
{
  char name[MAXPATH];
  struct stat st;

  make_name(fid, name);
  tp->fid = fid;
  tp->port = port;
  tp->filesize = fsize(name);
  if ((tp->filehandle = fopen(name, "rb")) == NULL)
    return(FALSE);
  tp->pos = -1L;
  fstat(fileno(tp->filehandle), &st);
  tp->filetime = st.st_atime;
  tp->destcall = "QST   \142";
  tp->via = "";
  return(TRUE);
}

/************************************************************************/
/* Ab der Adresse (!) Data Count Bytes in den Message-Buffer kopieren   */
/************************************************************************/
static void add_frm(MBHEAD *mbp, char *x, WORD count)
{
    while (count-- > 0)
    {
        mbp->l4time = calc_crc(*x, mbp->l4time); /* CRC updaten */
        putchr(*(x++), mbp);
    }
}

#ifdef PAC_DEBUG
UWORD gen_crc(UWORD crc, char data)
{
    UWORD y;
    crc ^= ((UWORD)data) << 8;
    for (y = 0; y < 8; y++)
        if (crc & 0x8000)
            crc = (crc << 1) ^ 0x1021;
        else
            crc <<= 1;
    return(crc);
}

/* Debug-Funktion, CRC auf Richtigkeit ueberpruefen */
void ckcrc(MBHEAD *mbp)
{
    UWORD crc = 0;

    rwndmb(mbp);
    /* Ueberprueft auf "traditioneller" Weise die CRC (ohne Tabelle) */
    /* damit kann man auf Nummer sicher gehen beim senden ...        */
    /* nur fuer Tests!                                               */
    while (mbp->mbpc - mbp->mbgc > 2) crc = gen_crc(crc, getchr(mbp));
    crc = gen_crc(crc, getchr(mbp)); /* 1. CRC Byte */
    if (gen_crc(crc, getchr(mbp)))
#ifdef SPEECH
        printf(speech_message(293));
#else
        printf("CRC Error\n");
#endif
}
#endif

/***************************************************************************/
/* CRC fuer das Frame anhaengen (haben wir in l4time berechnet, s. add_frm */
/***************************************************************************/
static void add_crc(MBHEAD *mbp)
{
#ifdef __WIN32__
    putchr((char)((mbp->l4time >> 8) & 0xff), mbp);
    putchr((char)(mbp->l4time & 0xff), mbp);
#else
    putchr((mbp->l4time >> 8) & 0xff, mbp);
    putchr(mbp->l4time & 0xff, mbp);
#endif /* WIN32 */

#ifdef PAC_DEBUG
    ckcrc(mbp); /* CRC ueberpruefen */
#endif
}

/************************************************************************/
/* Ein Frame broadcasten                                                */
/************************************************************************/
static BOOLEAN send_frame(PACTXDESC *tp)
{
    char    flags = 0x00; /* Flag: Offset im Header enthalten */
    char    type  = 0x00;
    char    data_buf[blocksize+1], *cp;
    WORD    data_size;
    WORD    ret = TRUE;
    ULONG   offset = 0L;

    MBHEAD *mbp;

    mbp = (MBHEAD *) allocb(ALLOC_MBHEAD);
    mbp->l4time = 0; /* Benutzen wir fuer die CRC */

    if (tp->pos == -1L) { /* den PACSAT-Directory-Entry broadcasten */

/* ACHTUNG ! PFH groesser dir_blocksize muessten in 2 Durchgaengen ge-  */
        flags |= PF_E; /* broadcastet werden, das ist hier nicht vorgesehen */

        tp->pos = 0L; /* beim naechsten mal das File selber broadcasten */
        if (tp->fid == last_fid) flags |= PF_N; /* neuster Eintrag ? */
        /* den Body-Offset bestimmen */
        if (tp->filehandle != NULL)
        {
            fseek(tp->filehandle, 0L, SEEK_SET);
            fread(data_buf, blocksize, 1, tp->filehandle);
            for (cp = data_buf + 2;
                 (cp < data_buf+blocksize) &&
                 (cp[2]);
                 cp += cp[2] + 3);
            data_size = cp - data_buf;
        }
        else return(FALSE);

        /* jetzt den Directory Header aufbauen */
        add_frm(mbp, &flags, 1);
        add_frm(mbp, (char *) &tp->fid, 4);
        add_frm(mbp, (char *) &offset, 4);
        add_frm(mbp, (char *) &tp->filetime, 4); /* t_old */
        add_frm(mbp, (char *) &tp->filetime, 4); /* t_new */
        if (data_size > 254 - mbp->mbpc) data_size = 254 - mbp->mbpc;
        add_frm(mbp, data_buf, data_size);
        add_crc(mbp); /* CRC an die Daten anhaengen */

        rwndmb(mbp);  /* Frame als UI-Frame senden */
        mbp->l2fflg = 0xBD; /* PID 0xBD */
#ifdef __WIN32__
        sdui(tp->via, tp->destcall, myid, (char)tp->port, mbp);
#else
        sdui(tp->via, tp->destcall, myid, tp->port, mbp);
#endif /* WIN32 */
        dealmb(mbp); /* Wegwerfen, sdui kopiert selbstaendig um */

        return(TRUE);
    }
    else flags = PF_O;

    /* PACSAT-Header aufbauen */
    add_frm(mbp, &flags, 1);
    add_frm(mbp, (char *) &tp->fid, 4);
    add_frm(mbp, (char *) &type, 1);
    add_frm(mbp, (char *) &tp->pos, 3);

    if ((LONG) blocksize >= /* wenn maximaler Dateninhalt */
        (tp->filesize - tp->pos) )  /* nicht gebraucht wird */
    {
        data_size = (WORD) (tp->filesize - tp->pos); /* nur noch den Rest senden   */
        ret = FALSE;
    }
    else data_size = blocksize;

#ifdef PAC_DEBUG
    printf("QST: msg=%lx offset=%lu, len=%d\n",tp->fid, tp->pos, data_size);
#endif

    if (tp->filehandle != NULL)
    {
        fseek(tp->filehandle, tp->pos, 0);
        fread(data_buf, data_size, 1, tp->filehandle);    /* Info lesen     */
        add_frm(mbp, data_buf, data_size);                /* Daten kopieren */
        tp->pos += (LONG) data_size;                      /* Offset erhoehen */
        if (tp->pos >= tp->filesize)
        {
           fclose(tp->filehandle);/* fertig -> Datei zu */
        }
    }
    else return(FALSE); /* es gibt nix mehr zu lesen ... */

    add_crc(mbp); /* CRC an die Daten anhaengen */

    rwndmb(mbp);  /* Frame als UI-Frame senden */
    mbp->l2fflg = 0xBB; /* PID 0xBB */
#ifdef __WIN32__
    sdui(tp->via, tp->destcall, myid, (char)tp->port, mbp);
#else
    sdui(tp->via, tp->destcall, myid, tp->port, mbp);
#endif /* WIN32 */
    dealmb(mbp); /* Wegwerfen, sdui kopiert selbstaendig um */

    return(ret);
}

/************************************************************************/
/* eine Broadcastrunde auf einem bestimmen Port durchfuehren.           */
/************************************************************************/
static BOOLEAN send_bcast2(WORD port)
{
  WORD x;

  /* Falls die Datei nicht existiert */
  if (pacstate[port].now.filehandle == NULL) return(FALSE);

  /* senden bis nix mehr da ist oder maximale Frame-Anzahl erreicht */
  for (x = 0; x < pacsat_frames; x++)
    if (!send_frame(&pacstate[port].now)) return(FALSE);

  return(TRUE);
}

/************************************************************************/
/* Broadcast-Steuerung                                                  */
/*                                                                      */
/* Die Routine sendet alle Files zwischen first_fd und last_fid.        */
/* Das aktuell gesendete File wird in actfid gespeichert.               */
/* Sind mehr als 200 Files vorhanden, wird last auf TRUE gesetzt        */
/* Wenn LAST=TRUE ist, dann wird bei jedem zweiten Durchgang ein File   */
/* aus den letzten 200 empfangenen Files gesendet.                      */
/*                                                                      */
/* Bsp:                                                                 */
/* Files 1-350 sind vorhanden. es wird gesendet:                        */
/* 001-350-002-349-003-348-004-347- ...                                 */
/* 199-150-200-350-201-349- ...                                         */
/* im Bsp. wird nicht beruecksichtigt, dass evtl neue Files einlaufen.  */
/*                                                                      */
/************************************************************************/
static void send_bcast(WORD port)
{
  PACSTATEDESC *ps = &pacstate[port];

  /* wenn noch im Transfer dann senden */
  if (ps->busy) ps->busy = send_bcast2(port);

  /* wenn nichts mehr zu senden dann neuen Transfer beginnen */
  if (!ps->busy)
  {
    if (ps->last)            /* Wenn eine zwischenschieben */
    {
      ps->fid200--;

      /* wieder mit der letzten anfangen */
      if (ps->fid200 < (last_fid - 200))
        ps->fid200 = last_fid;

      /* sicherheitshalber */
      if (ps->fid200 < first_fid)
        ps->fid200 = last_fid;

#ifdef __WIN32__
      init_bcast(ps->fid200, (char)port, &ps->now);
#else
      init_bcast(ps->fid200, port, &ps->now);
#endif /* WIN32 */
      ps->last = FALSE;
    }
    else
    {
      ps->actfid++;
      if (ps->actfid > last_fid)
         ps->actfid = first_fid;
#ifdef __WIN32__
      init_bcast(ps->actfid, (char)port, &ps->now);
#else
      init_bcast(ps->actfid, port, &ps->now);
#endif /* WIN32 */
      if (last_fid - first_fid > 200)
         ps->last = TRUE;
    }
    ps->busy = TRUE;
  }
}


/****************************************************************************/
/* Broadcast Service, wird staendig in MAIN aufgerufen. Gesendet wird wenn  */
/* der Sender nicht mehr beschaeftigt ist und mindestens der Verzoegerungs- */
/* Timer abgelaufen ist (fuer Einstiege die nebenher Broadcasten).          */
/****************************************************************************/
void pacsrv(void)
{
#ifdef __WIN32__
  ULONG zeit;
#else
  UWORD zeit;
#endif /* WIN32 */
  static ULONG lasttic = 0;
  WORD   x;

  if (last_fid < first_fid) return;

  zeit = tic10 - lasttic;
  lasttic = tic10;

  for (x = 0; x < L2PNUM; x++)
    if (pacsat_enabled[x])
      if (!iscd(x))
      {
        if (pacsat_delay[x] <= zeit)
        {
          send_bcast(x);
          pacsat_delay[x] = pacsat_timer; /* Timer wieder aufziehen  */
        }
        else
#ifdef __WIN32__
          pacsat_delay[x] -= (UWORD)zeit;
#else
          pacsat_delay[x] -= zeit;
#endif /* WIN32 */
      }
}


#endif /* PACSAT */
/* End of src/pacserv.c */

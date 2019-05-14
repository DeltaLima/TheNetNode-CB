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
/* File os/go32/tokenrng.c (maintained by: ???)                         */
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

#define TR_BUFFERSIZE      0x3000       /* Groesse jedes Buffers        */
                                        /* 16 Kanaele, je ein Frame     */
                                        /* jedes Frame max 328 Bytes    */
                                        /* jedes Byte transponiert      */
                                        /* = 0x2900                     */

static WORD    rx_state;
static BOOLEAN tr_res, tr_tst;
static UWORD   tr_cmd;
WORD           tokenflag;
LONG           recovery_count;
static LONG    token_pro_sec = 0;       /* Anzahl Token/Sekunde         */
static LONG    token_max_sec = 0;       /* Maximale Anzahl              */
static LONG    token_min_sec = 0;       /* Minimale Anzahl              */
static LONG    token_count   = 0;       /* Zaehler fuer Token           */
static LONG    lost_token    = 0;       /* Anzahl der verlorenen Token  */

extern int     incnt;

static void    tokenring_put_frame(void);
static void    tokenring_get_frame(void);
static void    send_token(void);
static void    tokenring_config_tnc(void);
static void    tokenring_reset_tnc(void);
static void    tokenring_send_test(void);

static int     tok_major = 0;
static ULONG   token_sent = 0L;

/* Fehlerstatistik */

unsigned      bad_frames;

/************************************************************************/
/* Tokenring Init                                                       */
/************************************************************************/
static BOOLEAN tokenring_l1init(void)
{
#ifdef PC
  int  adr, irq;
#endif /* PC */
#ifdef ST
  char device[16];
#endif /* ST */
  int  bd;

  tr_res = tr_tst = tr_cmd = FALSE;
#ifdef PC
  read_envcom("TOKENCOM", &tkcom, &adr, &irq);
  if (tkcom == -1 || blkbuf == NULL) return(FALSE); /* Tokenring unerwuenscht */

  if ((tkcom = open_rs232(tkcom,adr,irq,TR_BUFFERSIZE,TR_BUFFERSIZE)) == -1)
    return(FALSE);
#endif /* PC */

#ifdef ST
  read_envdev("TOKENCOM", device, "MODEM1");
  if ((tkcom = open_rs232(device)) == -1)
    return(FALSE);
#endif /* ST */

  if ((bd = setbaud(tkcom, tkbaud)) != tkbaud)
    xprintf("Invalid tokenring baudrate in config\n\a");
  tkbaud = bd;
  xprintf(" setting to %u00 8N2.\n", tkbaud);

  send_token();                         /* Erstes Token abschicken      */
  token_sent = tic10;                   /* Zeit merken fuer Timeout     */
  return(TRUE);
}

/************************************************************************/
/* Tokenring Exit                                                       */
/************************************************************************/
static void tokenring_exit(void)
{
  if (tkcom != -1)
    close_rs232(tkcom);
}

/************************************************************************/
/* Tokenring Control                                                    */
/* es werden interne Merkflags benutzt, um den Ablauf zu beschleunigen. */
/* (nach DL1XAO)                                                        */
/************************************************************************/
static void tokenring_ctl(int req, int port)
{
  switch(req) {
    case L1CRES: tr_res = TRUE;
                 break;
    case L1CCMD: tr_cmd = 10;
                 break;
    case L1CTST: tr_tst = TRUE;
  }
}

/************************************************************************/
/* Level1 RX/TX  fuer TOKENRING                                         */
/*               wird staendig in der main() Hauptschleife aufgerufen.  */
/************************************************************************/
static void tokenring(void)
{
  int  port;

  tokenflag = FALSE;                    /* default: Fehler              */
  tokenring_get_frame();                /* Frames aus Ringbuffer holen  */

  if (tokenflag == FALSE)               /* nichts gekommen?             */
  {
    if (tic10 > (token_sent + TOKENTIMEOUT)) /* Tokenring im Timeout    */
    {
      clear_rs232(tkcom);               /* RS232 Buffer loeschen        */
      clear_rs232(tkcom);               /* RS232 Buffer loeschen        */
      clear_rs232(tkcom);               /* RS232 Buffer loeschen        */
      rx_state = 0;                     /* RX-State auf FEND            */

      if (lost_token++ >= 400)
        HALT("tokenring");              /* Rechner neu starten          */

      if ((lost_token % 100) == 0)      /* neue Baudrate probieren      */
      {
        switch (tkbaud)
        {
               case 96:  tkbaud = 192; break;  /* no luck, try 19200 Baud */
               case 192: tkbaud = 384; break;  /* no luck, try 38400 Baud */
               case 384: tkbaud = 96;  break;  /* go back to 9600 Baud    */
        }
        setbaud(tkcom, tkbaud);
        xprintf ("*** Autobaud - Token ring: %u00 bit/s. \n",tkbaud);
      }

      send_token();         /* Neues Token auf den Ring legen            */
      token_sent = tic10;   /* Zeit merken wann Token gesendet           */
      recovery_count++;     /* Dieser Wert wird auch bei STAT angezeigt! */

      for (port = 0; port < L2PNUM; port++)
        if (portpar[port].major == tok_major)
          commandflag[port] = TRUE;    /* neue Parameter an TNC, RESET.. */
      tr_cmd = 2;           /* Flag setzen "es ist was zu tun"           */

      if (!incnt && show_recovery == TRUE)
      {
        xprintf("*** Token-Recovery (%ld) : %s", lost_token, ctime(&sys_time));
        notify(1, "*** Token-Recovery (%lu)", lost_token);
      }
    }
  }
  else
    lost_token = 0;
}

/************************************************************************/
/* tokenring_put_frame()  -  Frame(s) im Ringpuffer ablegen             */
/* Beschleunigte Variante nach DL1XAO                                   */
/*----------------------------------------------------------------------*/
static void tokenring_put_frame(void)
{
  int        port;
  UBYTE      ch, *out;
  WORD       *kp = kick;
  PORTINFO   *pp = portpar;
  LHEAD      *l2flp = txl2fl;
  MBHEAD     *txfhdl;

  for (port = 0; port < L2PNUM; port++, pp++, kp++, l2flp++) {
    if (    pp->major != tok_major
        || *kp == FALSE )                /* Port hat nichts zum Senden */
      continue;

    cd_timer[port] = tic10;        /* Wir senden, Belegt-Timer starten */

    ulink((LEHEAD *)(txfhdl = (MBHEAD *) l2flp->head));/*Zeiger holen*/
    TX_BEG();                      /* Sender bereit */
    TX_CHAR(FEND);                 /* Frame beginnt */
    TX_CHAR(port);                 /* Port */
    TX_CHAR(0x00);                 /* Datenframe */
    while (txfhdl->mbgc < txfhdl->mbpc) /* solange Daten vorhanden sind */
      switch(ch = getchr(txfhdl)) {
        case FEND: TX_CHAR(FESC);
                   TX_CHAR(TFEND);
                   break;
        case FESC: TX_CHAR(FESC);
                   TX_CHAR(TFESC);
                   break;

        default:   TX_CHAR(ch);
      }

    TX_CHAR(FEND);                       /* Frame bendet */
    rs232_write(tkcom, blkbuf, out);     /* und senden...     */

    relink((LEHEAD *)txfhdl,         /* als gesendet betrachten und in */
          (LEHEAD *)stfl.tail);      /* die gesendet Liste umhaengen    */

    *kp = (l2flp->head != l2flp);
  } /* for */
}

/************************************************************************/
/* tokenring_get_frame() - Frame(s) aus Ringbuffer holen                */
/*----------------------------------------------------------------------*/
static void tokenring_get_frame(void)
{
  static WORD k;
  static int  port;
  static MBHEAD *rxfhd = NULL;
  UBYTE       ch, *in = blkbuf;
  int         n;

  n = rs232_read(tkcom, blkbuf, BLOCKSIZE);
  while (n) {                       /* Zeichen auswerten                */
    ch = *in++;                     /* Zeichen holen                    */
    n--;
    switch (rx_state)                   /* ueber Status verzweigen      */
    {
      case WFEND:                       /* Frame Anfang erwartet        */
          if (ch == FEND)
            rx_state = GPORT;           /* Frameanfang entdeckt         */
          continue;

      case GPORT:
          switch (ch)                   /* zuletzt FEND bekommen        */
          {
            case FEND:  continue;       /* noch ein FEND: uebergehen    */

            case 0xFF:  rx_state = TOKEN; /* Token Anfang               */
                        continue;

            default:    port = ch & 0x7F;          /* Port von TNC         */

/* Ueberpruefen ob der Port ueberhaupt gueltig ist    */
                        if (   (   port >= L2PNUM
                                || portpar[port].major != tok_major)
                            && portpar[port].major != NO_MAJOR)
                        {
                          rx_state = WFEND; /* wieder auf Frame warten    */
                          bad_frames++;
                          continue;        /* gleich abbrechen..          */
                        }
                        rx_state = GTYPE;       /* next state: <cmd>    */
          }
          continue;

      case GTYPE:                       /* Port bekannt:                */
          switch (ch)                   /* nun <cmd> auswerten...       */
          {
            case 0x00:  rx_state = GFRAM;   /* es folgen [<daten>]..    */
                        if (rxfhd)          /* wenn Frame aktiv, auf den Muell */
                          relink((LEHEAD *)rxfhd, (LEHEAD *)trfl.tail);
                        (rxfhd = (MBHEAD *)allocb(ALLOC_MBHEAD))->l2port = port;
                        continue;

            case 0x0C:  k = 0;          /* Port hat RESET gemacht       */
                        rx_state = RESET;
                        continue;

            case 0x0E:  rx_state = DAMAP; /* DAMA Frame sent, PTT       */
                        continue;

            default:    rx_state = WFEND; /* Kommando unbekannt.        */
          }
          continue;

      case GFRAM:                       /* <daten> vom TNC holen        */
          switch (ch)
          {
            case FEND:                  /* nur wenn Frame aktiv,        */
                        if (rxfhd) {    /*     Befehl ausfuehren        */
                          relink((LEHEAD *)rxfhd, (LEHEAD *)rxfl.tail);
                          rxfhd = NULL; /* kein RX-Frame aktiv          */
                        }
                        rx_state = WFEND; /* Das war's.. fertig!        */
                        continue;

            case FESC:  rx_state++;     /* FESC wird zu FESC-TFESC      */
                        continue;       /* Rueckwandlung..              */

          }                             /* noch mehr...                 */
          break;

      case GFRMT:
          switch (ch)                   /* TFEND/TFESC-Rueckwandlung    */
          {
            case TFEND: ch = FEND;
                        break;

            case TFESC: ch = FESC;
                        break;

            default:    rx_state = WFEND;
                        continue;
          }
          rx_state--;
          break;

      case TOKEN:                   /* Token-Anfang gefunden            */
          switch (ch)               /* wird immer an Port 0xff gesendet */
          {
            case 6:     rx_state = TOKN2; /* Token OK                   */
                        continue;
            default:    rx_state = WFEND; /* ungueltig, kein Token!     */
          }
          continue;

      case TOKN2:
          if (ch == FEND)                  /* gueltiges Token empfangen  */
          {
            token_count++;

            if (tr_res)                    /* Ist TNC zu resetten?      */
              tokenring_reset_tnc();

            if (tr_cmd)
              if (--tr_cmd == 0)           /* Ist TNC zu konfigurieren? */
                tokenring_config_tnc();

            if (tr_tst)                    /* Test-Folge zu senden?     */
              tokenring_send_test();

            tokenflag = TRUE;
            tokenring_put_frame();
            send_token();
            token_sent = tic10;           /* Zeit merken */
          }
          rx_state = WFEND;
          continue;

      case RESET:                       /* Port hat RESET gemacht       */
          switch (ch)
          {
            case FEND:  tr_cmd = commandflag[port] = TRUE;
                        portstat[port].reset_count++;
                        rx_state = WFEND;
                        continue;

            default:    if (++k > 6)
                          rx_state = WFEND;
          }
          continue;

      case DAMAP:                       /* DAMA Frame gesendet, PTT    */
          if (ch == FEND)
            cd_timer[port] = 0;         /* Kanal frei: Timer stoppen   */
          rx_state = WFEND;
          continue;

    } /* switch(rxstate) */

    putchr(ch, rxfhd);                  /* Zeichen in Frame               */

    if (rxfhd->mbpc > L2MFLEN)          /* Framelaengencheck              */
      rx_state = WFEND;
  }
}

/************************************************************************/
/* Token auf Ring legen                                                 */
/*----------------------------------------------------------------------*/
void send_token(void)
{
  static UBYTE token[6] = { FEND, 0xFF, 0x06, FEND, 0, 0 };
  rs232_write(tkcom, token, &token[4]);
}

/************************************************************************/
/* Jeden TNC mit Parametern versorgen                                   */
/*----------------------------------------------------------------------*/
void tokenring_config_tnc(void)
{
  UBYTE   port, *out;
  WORD    *cf = commandflag;
  PORTINFO *pp = portpar;

  for (port = 0; port < L2PNUM; port++, pp++, cf++) {
    if (    pp->major != tok_major
        || *cf != TRUE                  /* der nicht...                 */
        )
      continue;                         /* ..ueberspringen              */

    TX_BEG();                           /* Sendung starten              */
    TX_CHAR(FEND);                      /* TX-Delay einstellen          */
    TX_CHAR(port);
    TX_CHAR(1);
    TX_CHAR(pp->txdelay);
    TX_CHAR(FEND);
    TX_CHAR(FEND);                      /* Persistance einstellen       */
    TX_CHAR(port);
    TX_CHAR(2);
    TX_CHAR(pp->persistance);
    TX_CHAR(FEND);
    TX_CHAR(FEND);                      /* Slottime einstellen          */
    TX_CHAR(port);
    TX_CHAR(3);
    TX_CHAR(pp->slottime);
    TX_CHAR(FEND);
    TX_CHAR(FEND);                      /* Tailtime einstellen          */
    TX_CHAR(port);
    TX_CHAR(4);
    TX_CHAR(TAILTIME);
    TX_CHAR(FEND);
    TX_CHAR(FEND);                      /* Fullduplex an/aus            */
    TX_CHAR(port);
    TX_CHAR(5);
    TX_CHAR((pp->l1mode & MODE_d) != 0 ? 1 : 0);
    TX_CHAR(FEND);
    TX_CHAR(FEND);                      /* DAMA an/aus                  */
    TX_CHAR(port);
    TX_CHAR(6);
    TX_CHAR(dama(port) ? 1 : 0);
    TX_CHAR(FEND);
    rs232_write(tkcom, blkbuf, out);    /* und senden...     */
    *cf = FALSE;
  }
  tr_cmd = 0;
}


/************************************************************************/
/* Reset Befehl an alle TNCs geben                                      */
/*----------------------------------------------------------------------*/
static void tokenring_reset_tnc(void)
{
 int      port;
 UBYTE   *out;
 PORTINFO *pp = portpar;

 for (port = 0; port < L2PNUM; port++, pp++) /* Alle TNCs durchgehen   */
 {
    if (   pp->major != tok_major
        || pp->reset_port != TRUE       /* soll dieser Reset bekommen?  */
        )
      continue;

    TX_BEG();
    TX_CHAR(FEND);
    TX_CHAR(port);
    TX_CHAR(0x0D);
    TX_CHAR(FEND);
    rs232_write(tkcom, blkbuf, out);    /* und senden...     */
    pp->reset_port = FALSE;
  }
  tr_res = FALSE;                       /* RESET erledigt...            */
}

/************************************************************************/
/* TEST-Befehl ausfuehren, dazu 4 KByte L/H-Folgen senden (Keine Flags!)*/
/*----------------------------------------------------------------------*/
void tokenring_send_test(void)
{
  UBYTE    port, *out;
  WORD    *tf = testflag;
  UWORD    count;
  PORTINFO *pp = portpar;

  for (port = 0; port < L2PNUM; port++, pp++, tf++) {
    if (   pp->major != tok_major      /* Nicht fuer uns...            */
        || *tf != TRUE                  /* der nicht...                 */
       )
      continue;                         /* ..ueberspringen              */

    TX_BEG();
    TX_CHAR(FEND);
    TX_CHAR(port);                      /*  der TEST Port               */
    TX_CHAR(0x00);
    rs232_write(tkcom, blkbuf, out);    /* und senden...                */

    count = pp->speed / 8 * 10;         /* 10 sec lang                  */
    if (count < 1024)
      count = 1024;

    memset(blkbuf, 0x00, (size_t)(BLOCKSIZE - 2));

    while (count) {                     /* und senden...     */
      if (count > BLOCKSIZE - 2) {
        rs232_write(tkcom, blkbuf, &blkbuf[BLOCKSIZE - 1]);
        count -= (BLOCKSIZE - 2);
      }
      else {
        out = &blkbuf[count];
        TX_CHAR(FEND);
        count = 0;
      }
    }

    TX_CHAR(FEND);
    rs232_write(tkcom, blkbuf, out);

    *tf = FALSE;                        /* TEST erledigt..              */
  }
  tr_tst = FALSE;
}

static WORD tokenring_dcd(PORTINFO *port)
{
  int minor = port->minor;
  ULONG *cdt;

  cdt = &cd_timer[minor];

  if (   dama(minor)
      && *cdt) {                      /* belegt..                       */
    if ((tic10 - *cdt) > CD_TIMEOUT)  /* Timeout abgelaufen?            */
      *cdt = 0;                       /* Timer stoppen                  */
    if (*cdt)                         /* Timer laeuft: Port ist belegt..*/
      return(PTTFLAG);                /* Einen belegten Port gefunden!  */
  }
  return(FALSE);                      /* keine Kanalkontrolle           */
}

static int tokenring_attach(int port, int unused_minor, BOOLEAN check_only)
{
  if (tkcom > -1) { /* Geraetekanal bereit? */
    if (!check_only)
      /* Minor wird auf Untergeraet (TNC) gesetzt, es wird aber nur ein Ring
         unterstuezt. Eigentlich waeren die Ringe die Minors ... */
      portpar[port].minor = port;
    return(1);
  }
  return(0); /* versuchte Doppeleintragung */
}

static void tokenring_info(int what, int port, MBHEAD *mbp)
{
  switch (what) {
    case HW_INF_STAT :
      if (tkcom >= -1) {
        putstr("\rTokenring-Statistics:\r\r", mbp);
        putprintf(mbp,   "        Tokens/sec: %8lu %8lu %8lu\r",
                       token_min_sec, token_pro_sec, token_max_sec);
        if (token_max_sec)
          putprintf(mbp, "    TOKENRING load: %lu%%\r",
            100 - (((ULONG)token_pro_sec)*100L) / ((ULONG)token_max_sec));
        putprintf(mbp, "\r  Token ring speed: %u00 Bit/s.\r"
                         "  Token-Recoveries: %u\r"
                         "        Bad-Frames: %u\r",
                         tkbaud, recovery_count, bad_frames);
      }
      break;
    case HW_INF_CLEAR :
      token_max_sec =
      token_min_sec = 0;
      recovery_count =
      bad_frames = 0;
      /* durchfallen! */
    default :
      default_l1info(what, port, mbp);
      break;
  }
}

static void tokenring_timer(UWORD ticks)
{
  static UWORD delay = 0;

  delay += ticks;
  if (delay > 6000) { /* eine Minute vergangen */
    token_pro_sec = (token_count / 60L);
    if (token_max_sec && token_min_sec) {
      token_max_sec =   token_pro_sec > token_max_sec
                      ? token_pro_sec
                      : token_max_sec;
      token_min_sec =   token_pro_sec > token_min_sec
                      ? token_min_sec
                      : token_pro_sec;
    } else
      token_min_sec =
      token_max_sec =
      token_pro_sec;
      token_count = 0;
    delay %= 6000;
  }
}

static int register_tokenring(void)
{
  MAJOR *m;

  if (tokenring_l1init()) {
    m = register_major();
    m->name   = "TOKENRING";
    m->exit   = tokenring_exit;
    m->handle = tokenring;
    m->ctl    = tokenring_ctl;
    m->dcd    = tokenring_dcd;
    m->info   = tokenring_info;
    m->timer  = tokenring_timer;
    m->attach = tokenring_attach;
    return(tok_major = num_major);
  }
  return(0);
}

/* End of os/go32/tokenrng.c */

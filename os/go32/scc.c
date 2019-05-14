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
/* File os/go32/scc.c (maintained by: ???)                              */
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

#if defined(__GO32__)
#define SCC_RXSIZE      8192            /* bei DPMI kann es mehr sein   */
#else
#define SCC_RXSIZE      1024            /* Groesse eines Buffers        */
#endif

#define SCC_TXSIZE      512

#define SCCCHANS    8

#define PCLK        4915200L

/*** Z8530 ******************************************************************/

/* Write Registers */

#define RES_EX_INT  0x10        /* WR0 */
#define RES_TX_IP   0x28
#define RES_ERR     0x30
#define RES_TX_CRC  0x80
#define RES_EOM     0xC0

#define EX_IE       0x01        /* WR1 */
#define TX_IE       0x02
#define RX_IE       0x10

#define RX_DIS      0xC8        /* WR3 */
#define RX_EN       0xC9

#define SDLC        0x20        /* WR4 */

#define TX_DIS      0x61        /* WR5 */
#define TX_EN       0x69
#define RTS         0x02

#define SDLC_FLAG   0x7E        /* WR7 */

#define MINT_EN     0x0A        /* WR9 */
#define HW_RES      0xC0

#define NRZ         0x80        /* WR10 */
#define NRZI        0xA0
#define ABUNDER     0x04

#define TRxC_RTxC   0x20        /* WR11 */
#define DPLL_RTxC   0x64
#define DPLL_BRG    0x74
#define TRxOUT_BRG  0x02
#define TRxOUT_DPLL 0x03

#define DPLL_EN     0x23        /* WR14 */
#define DPLL_DIS    0x60
#define DPLL_SRCBRG 0x80
#define DPLL_NRZI   0xE0
#define BRG_EN      0x63

#define ZC_IE       0x02        /* WR15 */
#define DCD_IE      0x08
#define SYNC_IE     0x10
#define CTS_IE      0x20

/* Read Registers */

#define DCD         0x08        /* RR0 */
#define HUNT        0x10
#define CTS         0x20
#define TX_EOM      0x40
#define ABORT       0x80

#define RESIDUE     0x0E        /* RR1 */
#define RES8        0x06
#define RX_OVR      0x20
#define CRC_ERR     0x40
#define SDLC_EOF    0x80

#define MIN_LEN       15

/*** Macros *****************************************************************/

#define LOBYTE(val)         ((val) & 0xFF)
#define HIBYTE(val)         ((val) >> 8)

#define set(var, bit)       var |=  (1 << bit)
#define res(var, bit)       var &= ~(1 << bit)

#define SCCDELAY            inp (0xE4)      /* non-existing port (2us) */

#ifndef outp
#define outp(p,b) outportb(p,b)
#endif
#ifndef inp
#define inp(p) inportb(p)
#endif

#define WR0(val)            outp ((SCCDELAY, scc->ctrl), val)
#define WR8(val)            WR (8, val)
#define WR(reg, val)        outp ((SCCDELAY, outp (scc->ctrl, reg), \
                                   SCCDELAY, scc->ctrl), val)

#define RR0                 inp ((SCCDELAY, scc->ctrl))
#define RR8                 inp ((SCCDELAY, scc->data))
#define RR(reg)             inp ((SCCDELAY, outp (scc->ctrl, reg), \
                                  SCCDELAY, scc->ctrl))

#define not0(par)           (par ? par : 1)

enum {TX_IDLE, TX_DWAIT, TX_DELAY, TX_ACTIVE, TX_FLUSH, TX_TAIL};

typedef struct __scc_struct
{
    int      l2port;

    unsigned chan;
    unsigned data, ctrl;

    unsigned baud;

    int      flags;

    int      dcd;
    int      rx_err;
    unsigned rx_len;

    int      tx_state;
    unsigned tx_timer;
    char    *tx_buf;
    char    *tx_ptr;
    int      tx_len;

    MBHEAD  *rxfhd;

    UBYTE    rr0;

    unsigned errors;

    unsigned tailtime;
    unsigned slottime;
    unsigned persistance;
    unsigned txdelay;

    unsigned overruns;
    unsigned underruns;
    unsigned crcerrors;
    unsigned lenerrors;
    unsigned buferrors;
} __scc_struct;

typedef struct __scc_board {
  char     *name;
  int       chans;
  unsigned  base;
  unsigned  irq;
  int       data[SCCCHANS];
  int       ctrl;
} __scc_board;

static __scc_struct sccchans[SCCCHANS];
static int scc_major = 0;

static unsigned sccbase;
static unsigned sccirq;
static unsigned sccboard;
static int      sccoldmask;
static unsigned sccminors;

static UWORD   *sccbuf, *sccin, *sccout;

static __scc_board sccboardtab[] =
{
  {"DSCC", 8, 0x300, 7, {1, 0, 3, 2, 5, 4, 7, 6}, 16},  /* DSCC */
  {"OSCC", 4, 0x150, 3, {3, 1, 7, 5, 0, 0, 0, 0}, -1},  /* OSCC */
  {"USCC", 4, 0x300, 7, {0, 1, 2, 3, 0, 0, 0, 0},  4},  /* USCC */
  {NULL  , 0,     0, 0, {0, 0, 0, 0, 0, 0, 0, 0},  0}
};

static void scc_int(int);
static void int_tx(__scc_struct *);
static void int_rx(__scc_struct *);
static void int_ex(__scc_struct *);
static void int_sp(__scc_struct *);
static void scc_clk (__scc_struct *, int);
static int  scc_init(void);
static void scc_exit(void);
static void scc_timer(UWORD);

/* Wenn der Sender keine Daten mehr ausstehen hat, wird ein weiteres
 * Frame in den Sendebuffer kopiert. */
static void scc_put_frame(__scc_struct *scc)
{
  MBHEAD         *fbp;
  int             port;
  LHEAD          *l2flp;

  if (scc->tx_len == 0) { /* erst senden wenn alles raus ist */
    if ((port = scc->l2port) == -1) return;
    if (kick[port]) {
      /* ein weiteres Frame aus der Sendeliste holen */
      l2flp = &txl2fl[port];
      ulink((LEHEAD *)(fbp = (MBHEAD *) l2flp->head));
      kick[port] = ((LHEAD *)l2flp->head != l2flp);

      /* Frame aus dem Buffer in den SCC-Buffer kopieren. Hier wird
         der Interrupt nicht gesperrt, da tx_len zuletzt und atomic
         gesetzt wird. */
      scc->tx_len = cpymbflat(scc->tx_ptr = scc->tx_buf, fbp);

      relink((LEHEAD *)fbp,            /* als gesendet betrachten und in */
            (LEHEAD *)stfl.tail);      /* die gesendet Liste umhaengen   */

      /* Wenn der sender noch nicht an ist, starten wir die Kanal-
         Arbitrierung, ansonsten stoppen wir ein eventuelles TXTAIL
         und gehen wieder zum senden ueber. */

      disable();
      switch (scc->tx_state) {
        case TX_IDLE:
          scc->tx_timer = scc->flags & MODE_d ? 1 : not0 (scc->slottime);
          scc->tx_state = TX_DWAIT;
          break;

        case TX_TAIL:
          scc->tx_timer = 1;
          scc->tx_state = TX_DELAY;
      }
      enable();
    }
  }
}

/* Der SCC-Empfangs-Ringbuffer wird geleert. Dort werden von allen Kanaelen
 * Empfangsdaten gesammelt. Das Format entspricht dem alten l1put().
 */
static void scc_get_frame(void)
{
  int       port, len;
  MBHEAD  **rxfbpp;
  unsigned  action;
  __scc_struct *scc;

  LOOP {
    disable();
    if (sccout == sccin) {
      enable();
      break;
    }
    action = *sccout++;            /* Zeichen aus dem Ringbuffer lesen */
    if (sccout >= sccbuf+SCC_RXSIZE)
      sccout = sccbuf;
    enable();

    scc = sccchans + ((action>>8) & 0x7F);

    if ((port = scc->l2port) == -1)
      continue;

    rxfbpp = &scc->rxfhd;                    /* Adresse RX-Framezeiger */
    if (!(action & 0x8000))                  /* Zeichen oder Befehl ?  */
      {                                      /* Zeichen :              */
        if (!*rxfbpp)                        /* RX-Frame aktiv ?       */
          {
            *rxfbpp = (MBHEAD *)allocb(ALLOC_MBHEAD); /* nein - neues anlegen   */
            (*rxfbpp)->l2port = port;        /*        fuer port       */
          }
        putchr(action & 0xFF,*rxfbpp);       /* Zeichen in Frame       */
      }
    else                                     /* Befehl :               */
      if (*rxfbpp)                           /* nur wenn Frame aktiv   */
        {                                    /* Befehl ausfuehren      */
          if (action & 0x0001)               /* Frame in Muelleimer    */
              relink((LEHEAD *)*rxfbpp, (LEHEAD *)trfl.tail);
          else {                             /* Frame in RX-Liste      */
            len = (action>>1)&3;
            if ((*rxfbpp)->mbpc > len)       /* FCS eleminieren        */
              (*rxfbpp)->mbpc -= len;        /* das erste Byte kommt   */
                                             /* noch vor HUNT          */
            relink((LEHEAD *)*rxfbpp, (LEHEAD *)rxfl.tail);
          }
          *rxfbpp = NULL;                    /* kein RX-Frame aktiv    */
        }
  }
}

static void scc(void)
{
  __scc_struct *scc;

  if (scc_major == 0)
    return;

  scc_get_frame();
  for (scc = sccchans; scc < sccchans+sccminors; scc++) {
    scc_put_frame(scc);
  }
}

static WORD scc_dcd(PORTINFO *port)
{
  __scc_struct *scc;
  int           state = 0;

  scc = &sccchans[port->minor];

  if (scc->tx_state)
    state |= PTTFLAG;
  if (scc->dcd)
    state |= DCDFLAG;
  return(state);
}

static void scc_ctl(int req, int port)
{
  __scc_struct *scc;
  int           minor;
  PORTINFO     *p;

  p = portpar+port;
  minor = p->minor;
  scc = &sccchans[minor];

  switch (req) {
    case L1CCMD :
    case L1CRES :
      if (p->speed > 384)
        p->speed = 384;
      if (p->speed < 3)
        p->speed = 3;
      scc->baud = (unsigned) (p->speed * 100);
      scc->slottime = p->slottime;
      scc->persistance = p->persistance;
      scc->txdelay = p->txdelay;
      scc->tailtime = p->speed == 3 ? 4 : TAILTIME;
      scc->flags = p->l1mode;

      WR (4, SDLC);
      WR (10, scc->flags & MODE_z ? NRZ | ABUNDER : NRZI | ABUNDER);
      WR (7, SDLC_FLAG);
      WR (3, RX_DIS);
      WR (5, TX_DIS);

      scc_clk (scc, scc->tx_state != TX_IDLE);

      WR (3, RX_EN);
      WR (5, scc->flags & MODE_t ? TX_EN : TX_DIS);

      WR (15, SYNC_IE | DCD_IE);
      WR0 (RES_EX_INT);
      WR0 (RES_EX_INT);
      WR (1, EX_IE | TX_IE | RX_IE);

      scc->rr0 = RR0;

      /* nochmal zur Sicherheit den CHIP-interrupt-master-enable an */
      WR (2, (scc->chan>>1) << 4);
      WR (9, MINT_EN);
      break;
  }
  default_l1ctl(req, port); /* Flags loeschen */
};

static int scc_istome(int major, char *devname)
{
  char   name[21], *cp;
  int    minor = 0;

  strncpy(name, devname, 20); /* Minor bestimmen und abschneiden */
  name[20] = 0;
  for (cp = name; isalpha(*cp); cp++); /* Zahl suchen */
  if (isdigit(*cp)) minor = atoi(cp);
  *cp = 0; /* kann nicht schaden */

  if (minor < 0 || minor >= sccminors)
    return(NO_MINOR); /* falscher Geraetekanal (minor) */

  return(minor);
}

static int scc_attach(int port, int minor, BOOLEAN check_only)
{
  __scc_struct *scc;

  scc = &sccchans[minor];

  if (scc_major) {
    if (scc->l2port == -1) {
      if (!check_only) {
        scc->l2port = port;
        portpar[port].minor = minor;
        scc_ctl(L1CRES, port);
      }
      return(1);
    }
    if (scc->l2port == port)
      return(1);
  }
  return(0); /* versuchte Doppeleintragung */
}

static int scc_detach(int port)
{
  __scc_struct *scc = &sccchans[portpar[port].minor];

  WR (1, 0);

  if (scc->rxfhd) {
    dealmb(scc->rxfhd);
    scc->rxfhd = NULL;
  }
  scc->l2port = -1;
  return(1);
}

static void scc_info(int what, int port, MBHEAD *mbp)
{
  int           minor, cnt;
  __scc_struct *scc;

  minor = portpar[port].minor;

  switch (what) {
    case HW_INF_IDENT :
    case HW_INF_INFO :
      putprintf(mbp, "%s%u", sccboardtab[sccboard].name, minor);
      break;
    case HW_INF_STAT :
      for (minor = cnt = 0, scc = sccchans; minor < sccminors; minor++, scc++)
        if (scc->l2port != -1) {
          if (cnt++ == 0)
            putstr("\rSCC-Statistics:\r\r", mbp);
          putprintf(mbp, "  SCC%u       RxOvr: %5u  TxUnd: %5u  RxCRC: %5u  RxBuf: %5u\r",
                         minor, scc->overruns, scc->underruns, scc->crcerrors, scc->buferrors);
        }
      break;
    case HW_INF_CLEAR :
      scc = &sccchans[minor];
      scc->overruns =
      scc->underruns =
      scc->crcerrors =
      scc->lenerrors =
      scc->buferrors = 0;
      /* durchfallen */
    default :
      default_l1info(what, port, mbp);
  }
}

/* SCC-Modes:
   t            -> Sendetakt extern (DF9IC)
   e            -> Sendetakt BRG (32-fach)
                -> Sendetakt DPLL (32-fach)
   r            -> Empfangstakt extern (DF9IC)
                -> Empfangstakt DPLL
*/

static void sccput(UWORD action)
{
  *sccin++ = action;
  if (sccin >= sccbuf+SCC_RXSIZE)
    sccin = sccbuf;
}

#define rxput(port, val)    sccput (((port) << 8) | (val))
#define rxeof(port, fcs)    sccput (((port) << 8) | 0x8000 | (fcs<<1))
#define rxdiscard(port)     sccput (((port) << 8) | 0x8001)

/*** SCC Clock **************************************************************/

static void scc_clk (__scc_struct *scc, int tx_on)
{
    UWORD tconst;

    if (scc->flags & MODE_t) /* externer Takt */
    {
        WR (11, TRxC_RTxC);
        WR (14, DPLL_DIS);
    }
    else
    {
        tconst = PCLK / 64L / scc->baud;
        if (tx_on)
            tconst *= 32;
        tconst -= 2;

        WR (12, LOBYTE (tconst));
        WR (13, HIBYTE (tconst));

        WR (11, tx_on ? DPLL_BRG | TRxOUT_BRG
                      : (scc->flags & MODE_e ? DPLL_RTxC | TRxOUT_BRG
                                             : DPLL_RTxC | TRxOUT_DPLL));

        WR (14, DPLL_SRCBRG);
        if ((scc->flags & MODE_z) == 0) WR (14, DPLL_NRZI);
        WR (14, DPLL_EN);
    }
}

/*** SCC TX *****************************************************************/

static void scc_tx (__scc_struct *scc, int tx_on)
{
    /* wenn MODE_t nicht gesetzt ist, muessen wir bei TX umschalten */
    if ((scc->flags & MODE_t) == 0)
    {
        WR (3, RX_DIS);
        WR (5, TX_DIS);

        scc_clk (scc, tx_on);

        if (tx_on)
            WR (5, TX_EN | RTS);
        else
            WR (3, RX_EN);
    }
    else
        WR (5, tx_on ? TX_EN | RTS : TX_EN);
}

/*** Timer ******************************************************************/

static void scc_timer (UWORD ticks)
{
    __scc_struct *scc;

    if (scc_major == 0)
      return;

    for (scc = sccchans; scc < sccchans+sccminors; scc++)
      if (scc->l2port != -1)
      {
        disable ();

        if (scc->tx_timer)
        {
          if (scc->tx_timer <= ticks)
          {
            scc->tx_timer = 0;

            switch (scc->tx_state)
            {
              case TX_DWAIT:
                if (   (scc->flags & MODE_d)
                    || (   (!scc->dcd)
                        && ((rand()&0xff) <= scc->persistance)
                       )
                   )
                {
                  scc_tx(scc, TRUE);
                  scc->tx_timer = not0(scc->txdelay);
                  scc->tx_state = TX_DELAY;
                }
                else
                  scc->tx_timer = not0(scc->slottime);
                break;

              case TX_DELAY:
                WR0 (RES_TX_CRC);
                WR8 (*scc->tx_ptr++);
                WR0 (RES_EOM);
                scc->tx_state = TX_ACTIVE;
                break;

              case TX_TAIL:
                scc_tx(scc, FALSE);
                scc->tx_state = TX_IDLE;
            }
          }
          else
            scc->tx_timer -= ticks;
        }
        enable ();
      }
}

/*** SCC Interrupt **********************************************************/

static void (*handler[]) (__scc_struct *) = {int_tx, int_ex, int_rx, int_sp};

//#define DEBUG(x) _farpokeb(_dos_ds,0xB8000, x);

static void scc_int(int foo)
{
    __scc_struct *scc;
    UBYTE         rr2;

    //DEBUG(ch++);

    for (scc = sccchans; scc < sccchans+sccminors; )
        if (RR (3))
        {
            scc++;
            rr2 = RR (2);
            handler[(rr2 >> 1) & 3] (&sccchans[(rr2 >> 3) ^ 1]);
            scc = sccchans;
        }
        else
            scc += 2;
}

/*** TX Handler *************************************************************/

static void int_tx(__scc_struct *scc)
{
    switch (scc->tx_state)
    {
      case TX_ACTIVE:
        if (RR0 & TX_EOM)
        {
          scc->underruns++;
          scc->errors++;
          scc->tx_ptr = scc->tx_buf; /* txrewind */
          scc->tx_state = TX_FLUSH;
        }
        else
          if (scc->tx_ptr >= scc->tx_buf + scc->tx_len) /* Frameende? */
          {
            WR (10, scc->flags & MODE_z ? NRZ : NRZI);
            WR0 (RES_TX_IP);
            scc->tx_state = TX_FLUSH;
            scc->tx_len = 0; /* Frame gesendet */
          } else
            WR8 (*scc->tx_ptr++);
        break;

    case TX_FLUSH:
        WR (10, scc->flags & MODE_z ? NRZ | ABUNDER : NRZI | ABUNDER);

        if (scc->tx_ptr < scc->tx_buf + scc->tx_len) /* noch Daten? */
        {
            WR0 (RES_TX_CRC);
            WR8 (*scc->tx_ptr++);
            WR0 (RES_EOM);
            scc->tx_state = TX_ACTIVE;
        }
        else
        {
            WR0 (RES_TX_IP);
            scc->tx_timer = scc->tailtime; /* + clkstep */
            scc->tx_state = TX_TAIL;
        }
    }
}

/*** Extern Status Change ***************************************************/

static void int_ex (__scc_struct *scc)
{
    static UBYTE rr0, delta;

    delta = (rr0 = RR0) ^ scc->rr0;

    if (scc->l2port != -1)
    {
        if (scc->rx_len && rr0 & (ABORT | HUNT))
        {
            rxdiscard (scc->chan);
            scc->rx_len = 0;
            scc->rx_err = FALSE;
        }

        if (delta & (scc->flags & MODE_c ? HUNT : DCD))
        {
            scc->dcd =   (scc->flags & MODE_c)
                       ? (!(rr0 & HUNT))
                       : (rr0 & DCD);
        }
    }

    scc->rr0 = rr0;
    WR0 (RES_EX_INT);
}

/*** RX Handler *************************************************************/

static void int_rx (__scc_struct *scc)
{
    UBYTE rr8;

    rr8 = RR8;

    if (!scc->rx_err)
    {
        /* scc->rx_err = */ rxput (scc->chan, rr8);
        scc->rx_len++;
    }
}

/*** Special RX Condition ***************************************************/

static void int_sp (__scc_struct *scc)
{
    UBYTE rr1;

    rr1 = RR (1);
    RR8;

    if (rr1 & RX_OVR)
    {
        scc->overruns++;
        scc->errors++;
        scc->rx_err = TRUE;
    }

    if (rr1 & SDLC_EOF)
    {
        if (   !scc->rx_err
            && scc->rx_len >= MIN_LEN+1
            && (rr1 & (CRC_ERR | RESIDUE)) == RES8)
        {
            rxeof (scc->chan, 1);
        }
        else
            if (scc->rx_len) {
                rxdiscard (scc->chan);
                /*if (scc->rx_len > 2) {
                  if (scc->rx_len < MIN_LEN+1) scc->lenerrors++;
                  if ((rr1 & (CRC_ERR | RESIDUE)) != RES8) scc->crcerrors++;
                }*/
            }
        scc->rx_len = 0;
        scc->rx_err = FALSE;
    }

    WR0 (RES_ERR);
}

/*** SCC Hardware Reset *****************************************************/

static void scc_hw_reset (void)
{
    __scc_struct *scc;
    UWORD         delay;

    for (scc = sccchans; scc < &sccchans[sccminors]; scc += 2)
    {
        RR0;
        WR (9, HW_RES);
        for (delay = 100; delay--;) SCCDELAY;
    }
}

/*** SCC Detect *************************************************************/
static int scc_detect(int max)
{
    __scc_struct *scc;

    for (scc = sccchans; scc < sccchans+max; scc++)
    {
        RR0;
        WR (13, 0x55); if (RR (13) != 0x55) break;
        WR (13, 0xAA); if (RR (13) != 0xAA) break;
    }
    return((int) (scc - sccchans));
}

/* Boardtyp auswaehlen, Defaultparameter setzen */
static int scc_init(void)
{
  char         *str;
  __scc_board  *board;
  __scc_struct *scc;
  int           chan;

  for (sccboard = 0, board = sccboardtab; board->name; board++, sccboard++) {
    if ((str = getenv(board->name)) != NULL) {
      sccbase = board->base;
      sccirq  = board->irq;
      sscanf(str, "%x,%u", &sccbase, &sccirq);
      if (sccbase && sccirq) {

        if ((sccbuf = malloc(SCC_RXSIZE*sizeof(UWORD))) == NULL)
          return(0);

        sccin = sccout = sccbuf;

        for (chan = 0, scc = sccchans; chan < SCCCHANS; chan++, scc++) {
          scc->l2port = -1;
          scc->rxfhd  = NULL;
          scc->chan   = chan;
          scc->data   = sccbase + board->data[chan];
          scc->ctrl   = scc->data + board->ctrl;
          if ((scc->tx_buf = malloc(SCC_TXSIZE)) == NULL) {
            while (--scc >= sccchans)
              free(scc->tx_buf);
            free(sccbuf);
            return(0);
          }
          scc->tx_len = 0;
        }

        scc_hw_reset();

        disable(); /* Interrupt aus */
        allocirq(sccirq, 0, scc_int, 0); /* neuen Vektor setzen */
        sccoldmask = getmask(sccirq);
        maskon(sccirq);
        enable();

        xprintf("--- /dev/SCC (0x%03X,IRQ%u), ", sccbase, sccirq);
        sccminors = scc_detect(board->chans);
        if ((sccminors & 1) == 0) {
          if (sccminors) {
            printf("card is %s, %u channels\n", board->name, sccminors);
            return(1);
          } else xprintf("%s not detected\n", board->name);
        } else xprintf("channel number is odd\n");
      }
    }
  }
  return(0); /* keine Karte gefunden/konfiguriert */
}

static void scc_exit(void)
{
  if (scc_major) {
    if (sccoldmask) maskoff(sccirq);
    freeirq(sccirq);
    scc_hw_reset();
  }
}

static int register_scc(void)
{
  MAJOR *m;

  if (scc_init()) {
    m = register_major();
    m->name    = "USCC/DSCC/OSCC";
    m->istome  = scc_istome;
    m->exit    = scc_exit;
    m->handle  = scc;
    m->ctl     = scc_ctl;
    m->dcd     = scc_dcd;
    m->attach  = scc_attach;
    m->detach  = scc_detach;
    m->info    = scc_info;
    m->timer   = scc_timer;
    return(scc_major = num_major);
  }
  return(0);
}

/* End of os/go32/scc.c */

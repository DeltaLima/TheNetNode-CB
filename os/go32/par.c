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
/* File os/go32/par.c (maintained by: ???)                              */
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
#define PAR_RXSIZE      8192            /* bei DPMI kann es mehr sein   */
#else
#define PAR_RXSIZE      1024            /* Groesse eines Buffers        */
#endif

#define PAR_TXSIZE      1024

#define PARCHANS           2

#define MIN_LEN           15

/* PAR */
#define TXD             0x01
#define PTT             0x02
#define BURST           0x04
#define DCDBIT          0x10
#define RXD             0x20

#define SDLC_FLAG       0x7E
#define STUFF           5

/*** Macros *****************************************************************/

#define LOBYTE(val)         ((val) & 0xFF)
#define HIBYTE(val)         ((val) >> 8)

 #if 0
#ifndef outp
#define outp(p,b) outportb(p,b)
#endif
#ifndef inp
#define inp(p) inportb(p)
#endif
 #endif

#define not0(par)           (par ? par : 1)

#define TX_IDLE   0
#define TX_DWAIT  1
#define TX_DELAY  2
#define TX_ACTIVE 3
#define TX_FLUSH  4
#define TX_TAIL   5

/* PAR */
#define TXD         0x01
#define PTT         0x02
#define BURST       0x04
#define DCDBIT      0x10
#define RXD         0x20

#define IRQEN       0x10

#define DCDCOUNT    5
#define DCDABORT    5
#define DCDLIMIT    15

/* L1 */
#define MIN_LEN     15

#define STUFF       5
#define FLAG        6
#define ABRT        7
#define FREE        30

/* CRC */
#define CRC_RESET   0xFFFF
#define CRC_MASK    0x8408
#define CRC_CHECK   0xF0B8

typedef struct __par_struct
{
    int      l2port;

    int      parbase;
    int      parirq;
    int      paroldmask;

    int      port;
    int      irq;

    int      bit_cnt;
    int      idle;
    int      shift;
    int      lastbit;
    int      frm_len;
    unsigned short crc;
    unsigned short fcs;
    int      dcd_cnt;
    int      dcd;
    int      sync;

    int      tx_state;
    char    *tx_buf;
    char    *tx_ptr;
    int      tx_len;
    int      tx_bitcnt;
    int      tx_idle;
    int      tx_bit;
    int      tx_timer;
    unsigned short tx_fcs;
    unsigned tx_shift;

    unsigned long scr;

    int      slottime;
    int      persistance;
    int      tailtime;
    int      txdelay;
    int      flags;

    unsigned burstbuf;

    unsigned overruns;
    unsigned underruns;
    unsigned crcerrors;
    unsigned lenerrors;

    MBHEAD   *rxfhd;
} __par_struct;

static __par_struct par96tab[PARCHANS];
static int par_major = 0;

static UWORD   *parbuf, *parin, *parout;

static void par_int(int);

/* Wenn der Sender keine Daten mehr ausstehen hat, wird ein weiteres
 * Frame in den Sendebuffer kopiert. */
static void par_put_frame(__par_struct *par)
{
  MBHEAD         *fbp;
  int             port;
  LHEAD          *l2flp;

  if (par->tx_len == 0) { /* erst senden wenn alles raus ist */
    if ((port = par->l2port) == -1) return;
    if (kick[port]) {
      /* ein weiteres Frame aus der Sendeliste holen */
      l2flp = &txl2fl[port];
      ulink((LEHEAD *)(fbp = (MBHEAD *) l2flp->head));
      kick[port] = ((LHEAD *)l2flp->head != l2flp);

      par->tx_len = cpymbflat(par->tx_ptr = par->tx_buf, fbp);

      relink((LEHEAD *)fbp,            /* als gesendet betrachten und in */
            (LEHEAD *)stfl.tail);      /* die gesendet Liste umhaengen   */

      disable();

      if (par->tx_state == TX_IDLE)
      {
        par->tx_timer = not0 (par->slottime);
        par->tx_state = TX_DWAIT;
      }

      enable();
    }
  }
}

/* Der PAR-Empfangs-Ringbuffer wird geleert. Dort werden von allen Kanaelen
 * Empfangsdaten gesammelt. Das Format entspricht dem alten l1put().
 */
static void par_get_frame(void)
{
  int           minor;
  MBHEAD      **rxfbpp;
  unsigned      action;
  __par_struct *par = par96tab;

#ifndef __GO32__
  UWORD    *_parin;

  disable();
  _parin = parin;
  enable();
#else
#define _parin parin
#endif

  while (parout != _parin) {
    action = *parout++;            /* Zeichen aus dem Ringbuffer lesen */
    if (parout >= parbuf+PAR_RXSIZE)
      parout = parbuf;

    minor = (action>>8) & 0x7F;

    par = par96tab + minor;
    rxfbpp = &par->rxfhd;                    /* Adresse RX-Framezeiger */
    if (!(action & 0x8000))                  /* Zeichen oder Befehl ?  */
      {                                      /* Zeichen :              */
        if (!*rxfbpp)                        /* RX-Frame aktiv ?       */
          {
            *rxfbpp = (MBHEAD *)allocb();    /* nein - neues anlegen   */
            (*rxfbpp)->l2port = par->l2port; /*        fuer port       */
          }
        putchr(action & 0xFF,*rxfbpp);       /* Zeichen in Frame       */
      }
    else                                     /* Befehl :               */
      if (*rxfbpp)                           /* nur wenn Frame aktiv   */
        {                                    /* Befehl ausfuehren      */
          if (action & 0x0001)               /* Frame in Muelleimer    */
              relink((LEHEAD *)*rxfbpp, (LEHEAD *)trfl.tail);
          else {                             /* Frame in RX-Liste      */
            if ((*rxfbpp)->mbpc > 2)         /* FCS eleminieren        */
              (*rxfbpp)->mbpc -= 2;          /* das erste Byte kommt   */
                                             /* noch vor HUNT          */
            relink((LEHEAD *)*rxfbpp, (LEHEAD *)rxfl.tail);
          }
          *rxfbpp = NULL;                    /* kein RX-Frame aktiv    */
        }
  }
}

static void par(void)
{
  __par_struct *par;

  par_get_frame();
  for (par = par96tab; par < par96tab+PARCHANS; par++)
    if (par->parbase)
      par_put_frame(par);
}

static WORD par_dcd(PORTINFO *port)
{
  __par_struct *par;
  int           state = 0;

  par = &par96tab[port->minor];

  if (par->tx_state)
    state |= PTTFLAG;
  if (par->dcd)
    state |= DCDFLAG;
  return(state);
}

static void par_ctl(int req, int port)
{
  __par_struct *par;
  int           minor;
  PORTINFO     *p;

  p = portpar+port;
  minor = p->minor;
  par = &par96tab[minor];

  switch (req) {
    case L1CCMD :
    case L1CRES :
      if (p->speed != 192)
        p->speed = 96;
      par->slottime = p->slottime;
      par->persistance = p->persistance;
      par->txdelay = p->txdelay;
      par->tailtime = TAILTIME;
      par->flags = p->l1mode;
      par->tx_state =
      par->tx_timer = 0;
      par->tx_len = 0;
      par->dcd = 0;
      outp (par->parbase, PTT | BURST);
      outp (par->parbase + 2, inp (par->parbase + 2) | IRQEN);
      break;
  }
  default_l1ctl(minor, req); /* Flags loeschen */
};

static int par_attach(int port, int minor, BOOLEAN check_only)
{
  __par_struct *par;

  par = &par96tab[minor];

  if (par_major) {
    if (par->l2port == -1) {
      if (!check_only) {
        par->l2port = port;
        portpar[port].minor = minor;
        par_ctl(L1CRES, port);
      }
      return(1);
    }
    if (par->l2port == port)
      return(1);
  }
  return(0); /* versuchte Doppeleintragung */
}

static int par_detach(int port)
{
  __par_struct *par = &par96tab[portpar[port].minor];

  par->l2port = -1;

  outp (par->parbase + 2, inp (par->parbase + 2) & ~IRQEN);
  outp (par->parbase, PTT | BURST);

  return(1);
}

static void par_info(int what, int port, MBHEAD *mbp)
{
  int           minor, cnt;
  __par_struct *par;

  minor = portpar[port].minor;

  switch (what) {
    case HW_INF_IDENT :
    case HW_INF_INFO :
      putprintf(mbp, "PAR%u", minor);
      break;
    case HW_INF_STAT :
      for (minor = cnt = 0, par = par96tab; minor < PARCHANS; minor++, par++)
        if (par->l2port != -1) {
          if (cnt++ == 0)
            putstr("\rPAR-Statistics:\r\r", mbp);
          putprintf(mbp, "  PAR%u       RxOvr: %5u  TxUnd: %5u  RxCRC: %5u  RxLen: %5u\r",
                         minor, par->overruns, par->underruns, par->crcerrors, par->lenerrors);
        }
      break;
    case HW_INF_CLEAR :
      par = &par96tab[minor];
      par->overruns =
      par->underruns =
      par->crcerrors =
      par->lenerrors = 0;
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

static void parput(UWORD action)
{
  *parin++ = action;
  if (parin >= parbuf+PAR_RXSIZE)
    parin = parbuf;
}

#define rxput(port, val)    parput (((port) << 8) | (val))
#define rxeof(port)         parput (((port) << 8) | 0x8000)
#define rxdiscard(port)     parput (((port) << 8) | 0x8001)

/*** HDLC ******************************************************************/

void hdlc_put_word(int minor, struct __par_struct *par, unsigned burstbuf)
{
    int            bit;
    int            dcd;
    int            cnt;

    dcd = par->flags & MODE_c;

    for (cnt = 16; cnt--; )
    {
        bit = burstbuf & 1;
        burstbuf >>= 1;

        par->scr = (par->scr << 1) | (bit != par->lastbit);
        par->lastbit = bit;

        bit =   !((UWORD) par->scr & 1)
              ^ !((UWORD) par->scr & 0x1000)
              ^ !(        par->scr & 0x20000L);

        if (!bit)
        {
            if (par->idle < STUFF)
            {
                par->idle = 0;
                goto shift_bit;
            }

            if (par->idle == FLAG)
            {
                if (   dcd
                    && par->dcd_cnt < DCDLIMIT
                    && ++par->dcd_cnt == DCDCOUNT)
                {
                    par->dcd = TRUE;
                }

                if (par->frm_len)
                {
                    if (   par->sync
                        && par->frm_len >= MIN_LEN+2
                        && par->bit_cnt == 1
                        && par->fcs     == CRC_CHECK)
                    {
                        rxeof(minor);
                    }
                    else
                        rxdiscard(minor);

                    par->frm_len = 0;
                }

                par->bit_cnt = 8;
                par->sync    = TRUE;
                par->crc     = CRC_RESET;
            }

            par->idle = 0;
        }
        else
        {
            if (par->idle < FREE)
            {
                if (++par->idle < ABRT)
                {
shift_bit:          if (par->sync)
                    {
                        if (par->shift >>= 1, bit)
                            par->shift |= 0x80;

                        par->crc = (par->crc >> 1) ^ ((bit ^
                                     par->crc) & 1 ? CRC_MASK : 0);

                        if (--par->bit_cnt == 0)
                        {
                            /* par->sync = ! */
                            rxput (minor, par->shift);
                            par->fcs     = par->crc;
                            par->bit_cnt = 8;
                            par->frm_len++;
                        }
                    }
                }
                else
                {
                    if (dcd)
                    {
                        if (par->dcd_cnt > DCDABORT)
                            par->dcd_cnt -= DCDABORT;
                        else
                            par->dcd_cnt = 0;

                        if (par->dcd_cnt < DCDCOUNT)
                        {
                            par->dcd = FALSE;
                        }
                    }

                    par->sync = FALSE;
                }
            }
        }
    }
}

unsigned hdlc_get_word(int minor, __par_struct *par)
{
    int           bit;
    int           cnt;
    int           shift;

    for (cnt = 16; cnt--; ) {
      /* wenn wir Daten senden, muessen wir das Stuffing beruecksichtigen */
      if (   par->tx_idle == STUFF
          && (   par->tx_state == TX_ACTIVE
              || par->tx_state == TX_FLUSH))
      {
        par->tx_bit ^= 1;
        par->tx_idle = 0;
      } else
      {
        if (!par->tx_bitcnt)
        {
          switch (par->tx_state)
          {
            case TX_DELAY:
              if (par->tx_timer)
              {
                par->tx_shift  = SDLC_FLAG;
                par->tx_bitcnt = 8;
                break;
              }
              par->tx_fcs   = CRC_RESET;
              par->tx_idle  = 0;
              par->tx_state = TX_ACTIVE;

            case TX_ACTIVE:
              if (par->tx_ptr >= par->tx_buf + par->tx_len) {
                par->tx_state  = TX_FLUSH;
                par->tx_shift  = ~par->tx_fcs;
                par->tx_bitcnt = 16;
                par->tx_len    = 0;
              } else {
                par->tx_shift  = *par->tx_ptr++;
                par->tx_bitcnt = 8;
              }
              break;

            case TX_FLUSH:
              par->tx_timer  = TAILTIME;
              par->tx_state  = TX_TAIL;

            case TX_TAIL:
              par->tx_shift  = SDLC_FLAG;
              par->tx_bitcnt = 8;
              break;
          }
        }

        if (par->tx_shift & 1)
          par->tx_idle++;
        else {
          par->tx_bit ^= 1;
          par->tx_idle = 0;
        }

        par->tx_fcs = (par->tx_fcs >> 1) ^ ((par->tx_shift ^
                       par->tx_fcs) & 1 ? CRC_MASK : 0);

        par->tx_shift >>= 1;
        par->tx_bitcnt--;
      }

      par->scr =   (par->scr << 1)
                 | (bit =    par->tx_bit
                         ^ !((UWORD) par->scr & 0x800)
                         ^ !(        par->scr & 0x10000L));

      shift >>= 1;
      if (bit)
        shift |= 0x8000;
    }

    return(shift);
}

/*** PAR RX *****************************************************************/

void par_rx(int minor)
{
    unsigned       burstbuf;
    int            port_inp;
    int            port_out;
    int            cnt;
    __par_struct  *par = par96tab+minor;

    port_out = par->parbase;
    port_inp = par->parbase+1;
    /* Die Daten als Burst lesen, damit wieder Platz in der FIFO ist */
    for (cnt = 16; cnt--; )
    {
        burstbuf >>= 1;
        if (inp(port_inp) & RXD)
          burstbuf |= 0x8000;
        outp (port_out, PTT);
        outp (port_out, PTT | BURST);
    }
    /* DCD fuer PicPar/PicPar97 */
    if (!(par->flags & MODE_c))
        par->dcd = inp (port_inp) & DCDBIT;

#if 0
    hdlc_put_word(minor, par, burstbuf);
#endif
}

/*** PAR TX *****************************************************************/

void par_tx(int minor)
{
    int           bit;
    int           cnt;
    int           port_out;
    unsigned      shift;
    __par_struct *par = par96tab+minor;

    port_out = par->parbase;
    /* zuerst werden die vorbereiteten Daten gesendet */
    shift    = par->burstbuf;
    for (cnt = 16; cnt--; ) {
      bit     = shift & 1;
      shift >>= 1;
      outp(port_out, bit);
      outp(port_out, bit | BURST);
    }

#if 0
    par->burstbuf = hdlc_get_word(minor, par);
#endif
}

static void par_timer (UWORD ticks)
{
    __par_struct *par;
    int           minor;

    if (par_major == 0)
      return;

    for (minor = 0, par = par96tab; minor < PARCHANS; minor++, par++)
      if (par->l2port != -1) {

#if 1
        hdlc_put_word(minor, par, hdlc_get_word(minor, par));
#endif

        disable ();

        if (par->tx_timer)
          if (par->tx_timer <= ticks)
          {
            par->tx_timer = 0;

            switch (par->tx_state)
            {
              case TX_DWAIT:
                if (   (par->flags & MODE_d)
                    || (   (!par->dcd)
                        && ((rand()&0xff) <= par->persistance)
                       )
                   ) {
                  par->tx_timer = not0(par->txdelay);
                  par->tx_state = TX_DELAY;
                } else
                  par->tx_timer = not0(par->slottime);
                break;

              case TX_TAIL:
                outp(par->parbase, PTT | BURST);
                par->tx_state = TX_IDLE;
            }
          } else
            par->tx_timer -= ticks;

        enable ();
      }
}


/*** PAR Interrupt **********************************************************/

void par_int(int minor)
{
    if (par96tab[minor].tx_state > TX_DWAIT)
        par_tx(minor);
    else
        par_rx(minor);
}

/* Defaultparameter setzen */
static int par_init(void)
{
  char          tmp[10];
  char         *str;
  __par_struct *par;
  int           minor;
  unsigned      parbase;
  unsigned      parirq;
  int           used;

  used = 0;
  for (par = par96tab, minor = 0; minor < PARCHANS; minor++, par++) {
    par->parbase = 0;
    par->l2port = -1;
    sprintf(tmp, "PAR%u", minor);
    if ((str = getenv(tmp)) != NULL) {
      parbase = parirq = 0;
      sscanf(str, "%x,%u", &parbase, &parirq);
      if (parbase && parirq) {
        if ((parbuf = malloc(PAR_RXSIZE*sizeof(UWORD))) == NULL)
          return(0);
        parin = parout = parbuf;
        if ((par->tx_buf = malloc(PAR_TXSIZE)) == NULL)
          return(0);
        par->tx_len = 0;
        par->rxfhd = NULL;

        par->parirq = parirq;
        par->parbase = parbase;

        disable(); /* Interrupt aus */
        allocirq(parirq, 0, par_int, 0); /* neuen Vektor setzen */
        par->paroldmask = getmask(parirq);
        maskon(parirq);
        enable();

        printf("--- /dev/PAR%u (0x%03X,IRQ%u) detected.\n", minor, parbase, parirq);

        used++;
      }
    }
  }
  return(used); /* eine Karte gefunden/konfiguriert ? */
}

static void par_exit(void)
{
  __par_struct *par;
  int           minor;

  if (par_major) {
    for (minor = 0, par = par96tab; minor < PARCHANS; minor++, par++)
      if (par->parbase) {
        if (par->l2port) par_detach(par->l2port);
        if (par->paroldmask) maskoff(par->parirq);
        freeirq(par->parirq);
      }
  }
}

static int register_par(void)
{
  MAJOR *m;

  if (par_init()) {
    m = register_major();
    m->name    = "PAR96";
    m->exit    = par_exit;
    m->handle  = par;
    m->ctl     = par_ctl;
    m->dcd     = par_dcd;
    m->attach  = par_attach;
    m->detach  = par_detach;
    m->info    = par_info;
    m->timer   = par_timer;
    return(par_major = num_major);
  }
  return(0);
}

#undef rxput
#undef rxeof
#undef rxdiscard

/* End of os/go32/par.c */

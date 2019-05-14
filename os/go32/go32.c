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
/* File os/go32/go32.c (maintained by: ???)                             */
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
#include "pc.h"
#include <crt0.h>
#include <sys/farptr.h>
#include <sys/movedata.h>
#include <pc.h>

#define WATCH_TIME      6000            /* ca.5 Minuten Timeout         */
#define PIC             0x20            /* Interrupt-Controller         */
#define PIT             0x40            /* Timer Port 0                 */

int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY;
unsigned int_level = 0;
int GNU32BUFFERS = 5000;
char *dbg_module = "";
int tim_factor;
int conscom = -1;       /* Schnittstellen-Handle fuer Console           */
int portadress = 0;
static int oldtimer = 0;
FILE *aus;                                         /* Fuer den Wachhund */
char debugfile[MAXPATH];
WORD watchdog;                     /* wurde schon mal extern deklariert */
static BOOLEAN flag = TRUE;
struct tm *zeit;

/**************************************************************************/
/* vsnprintf fuer DOSe DG8BR                                              */
/*------------------------------------------------------------------------*/
int vsnprintf(char *str, int NOTIFYVSNPFBUFF, const char *format, va_list arg_ptr)
{
  char str2[4096];
  int  n = 0;

  if (vsprintf(str2, format, arg_ptr) == EOF)
    return(EOF);

  if ((n = strlen(str2)) > NOTIFYVSNPFBUFF)
    return(n);

  strncpy(str, str2, NOTIFYVSNPFBUFF);
  return(n);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void randomize(void)
{
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
unsigned long coreleft(void)
{
  return(_go32_dpmi_remaining_physical_memory());
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOLEAN ishput(void)
{
  return(conscom == -1 ? FALSE : rs232_out_status(conscom));
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOLEAN ishget()
{
  if (kbhit())
  {
    if (ishmod)
      hgetc();       /* im Hostmode Console ignorieren */
    else
      return(TRUE);  /* im Terminalmode melden, dass was da ist */
  }
  if (conscom != -1)
    return(rs232_in_status(conscom));
  return(FALSE);     /* keine Umleitung, also auch nix da */
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
char hgetc(void)
{
  #define Prefix 0
  #define ALT_X '-'
  char retval;

  if (kbhit()) {
    if ((retval = getch()) == Prefix)
      if (getch() == ALT_X)
        quit_program(0);
    return(retval);
  }
  if (conscom != -1)
    if (rs232_in_status(conscom))
      return(rs232_in(conscom));
  return(0);
}

/**************************************************************************/
/*                                                                        */
/* Zeichen an die Console ausgeben. Wenn eine Umleitung besteht, das      */
/* Zeichen zusaetzlich noch dort ausgeben. Auf die PC-Console wird im     */
/* Hostmode nichts geschrieben.                                           */
/*                                                                        */
/*------------------------------------------------------------------------*/
void hputc(char c)
{
  if (!ishmod) { /* keine Ausgaben im Hostmode */
    if (c == CR) /* nur fuer PC/Console notwenig! */
      putchar('\n');
    else
      putchar(c);
  }
  if (conscom != -1)
    rs232_out(conscom, c);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
BOOLEAN init_hardware(int argc, char *argv[])
{
  char *tz_string;
  int baseadress[3];
  int lpt = 0;

  #define MEMORY_NEEDED (GNU32BUFFERS*sizeof(MAX_BUFFER))

  randomize();

  if (getenv("TNN32BUFFERS") != NULL)
    sscanf(getenv("TNN32BUFFERS"), "%u", &GNU32BUFFERS);

  if (getenv("TOGGLEPORT") != NULL)
  {
     sscanf(getenv("TOGGLEPORT"), "%u", &lpt);

     baseadress[0] = _farpeekw(_dos_ds,0x0040*16 + 0x08);  /* LPT 1 */
     baseadress[1] = _farpeekw(_dos_ds,0x0040*16 + 0x0a);  /* LPT 2 */
     baseadress[2] = _farpeekw(_dos_ds,0x0040*16 + 0x0c);  /* LPT 3 */
     portadress = baseadress[lpt -1];
     printf("\nfound LPT at $%x.\n",portadress);
     portadress = portadress + 2;        /* getoggelt wird 2 hoeher */
     toggle_lpt(); /* schon mal schalten, sonst werden es 2 Minuten */
  }
  if ((RAMBOT = (char *) calloc(1,MEMORY_NEEDED)) == NULL)
  {
     printf("\n\007\n*** WARNING: Not enough memory !\n\n");
     exit(-1);
  }

  RAMTOP = RAMBOT + MEMORY_NEEDED;

  /**********************************************************************/
  /* Komandozeile auswerten                                             */
  /**********************************************************************/
  if (argc > 1) hputs("*** WARNING: Parameters ignored");

  /**********************************************************************/
  /* Systemzeit und Zeitberechung initialisieren                        */
  /**********************************************************************/
  tz_string = getenv("TZ");             /* Feststellen, ob TimeZone     */
  if (tz_string == NULL) {              /* gesetzt ist, sonst           */
     putenv("TZ=UTC0");                 /* mit UTC0 vorbelegen          */
     puts("*** WARNING: Timezone set to UTC");
  }
  tzset();                              /* Zeitzone auswerten           */

  consfile = stdout;

  return(FALSE);
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void init_console(void)
{
  int  dev,adr,irq,bd;

  read_envcom("CONSOLE", &dev, &adr, &irq);
  if (dev != -1) {
    conscom = open_rs232(dev,adr,irq,512,512);
    bd = setbaud(conscom, 192);
    xprintf(" setting to %u00 8N1.\n", bd);
    xprintf("--- CONSOLE attached to /dev/COM%u\n", dev+1);
  }
}

/**************************************************************************/
/*                                                                        */
/*------------------------------------------------------------------------*/
void exit_console(void)
{
  if (conscom != -1)
    close_rs232(conscom);
}

/**************************************************************************/
/* Uebergabe von Daten an die Shell (nur Linux)                           */
/*------------------------------------------------------------------------*/
BOOLEAN l7tosh(MBHEAD *mbp)
{
  return(FALSE);
}

/**************************************************************************/
/* Shell-Server (nur Linux)                                               */
/*------------------------------------------------------------------------*/
void shellsrv(void)
{
}

/**************************************************************************/
/* Ausfuehren einer neuen Software                                        */
/*------------------------------------------------------------------------*/
void tnnexec(char *file)
{
  char filename[MAXPATH+1];

  strncpy(filename, file, MAXPATH);
  filename[MAXPATH] = 0;
  if (xaccess(filename,0)==0) {
    l1exit();
    /* bei DOS16 -> done_umb() */
    exit_timer();
    exit_hardware();
    execl(filename, filename, NULL);
    perror("EXEC");
    HALT("tnnexec");
  }
}

/**************************************************************************/
/* Ausfuehren eines Befehls                                               */
/*------------------------------------------------------------------------*/
BOOLEAN tnnshell(char *cmdline)
{
  char cmd[MAXPATH+MAXPATH+2], *name;

  strncpy(cmd, cmdline, MAXPATH);
  cmd[MAXPATH] = 0;
  name = tempnam(textpath, "sh");
  if (name) {
    strcat(cmd, " > ");
    strcat(cmd, name);
    system(cmd);
    strcpy(cmdline, normfname(name));
    free(name);
  } else cmdline[0] = 0;

  return(FALSE);
}

/**************************************************************************/
/* Hardware deinitialisieren                                              */
/*------------------------------------------------------------------------*/
void exit_hardware()
{
  free((void *) RAMBOT);
}

/**************************************************************************/
/* System neu starten (GNU32 -> Beenden und Rechner neu starten)          */
/*------------------------------------------------------------------------*/
void reboot_system()
{
  __dpmi_regs r;
  l1exit();

  r.x.ax = 47;                                        /* Pointer setzen  */
  r.x.cs = 0xffff;                                     /* Segment im DOS */
  r.x.ip = 0x0000;                                      /* Offset im DOS */
  r.x.ss = r.x.sp = 0;                  /* Sollen laut DPMI auf 0 stehen */
  __dpmi_simulate_real_mode_procedure_retf(&r);      /* Kaltstartroutine */
}

/**************************************************************************/
/* An LPT1 den Pin 16(RESET) toggeln. Hardware-Watchdog                   */
/**************************************************************************/
void
toggle_lpt (void)
{
  if (portadress != 0)
  {
    if (flag)
    {
      writebit(portadress, 0x04, 1);                /* Pin 16 einschalten */
      flag = FALSE;
    }
    else
    {
      writebit(portadress, 0x04, 0);                /* Pin 16 ausschalten */
      flag = TRUE;
    }
  }
}

/**************************************************************************/
/* Wenn moeglich Interrupts wieder freigeben                              */
/*------------------------------------------------------------------------*/
void decEI(void)
{
  if (--int_level == 0)
     enable();
}

/**************************************************************************/
/* Interrupts sperren in kritischen Bereichen.                            */
/*------------------------------------------------------------------------*/
void DIinc(void)
{
  int_level++;
  disable();
}

/* Disable hardware interrupt */
int
maskoff(unsigned irq)
{
  if (irq < 8)
  {
    setbit(0x21, (char)(1 << irq));
  }
  else
    if (irq < 16)
    {
      irq -= 8;
      setbit(0xa1, (char)(1 << irq));
    }
    else
    {
      return(-1);
    }
  return(0);
}

/* Enable hardware interrupt */
int
maskon(unsigned irq)
{
  if (irq < 8)
  {
    clrbit(0x21, 1 << irq);
  }
  else
    if (irq < 16)
    {
      irq -= 8;
      clrbit(0xa1, 1 << irq);
    }
    else
    {
      return(-1);
    }
  return(0);
}

/* Return 1 if specified interrupt is enabled, 0 if not, -1 if invalid */
int
getmask(unsigned irq)
{
  if (irq < 8)
    return((inportb(0x21) & (1 << irq)) ? 0 : 1);
  else
    if (irq < 16)
    {
      irq -= 8;
      return((inportb(0xa1) & (1 << irq)) ? 0 : 1);
    }
    else
      return(-1);
}

/* Set bit(s) in I/O port */
void
setbit(unsigned port, unsigned char bits)
{
  outportb(port, inportb(port) | bits);
}

/* Clear bit(s) in I/O port */
void
clrbit(unsigned port, unsigned char bits)
{
  outportb(port, inportb(port) & ~bits);
}

/* Set or clear selected bits(s) in I/O port */
void
writebit(unsigned port, unsigned char mask, int val)
{
  unsigned char x;

  x = inportb(port);
  if (val)
    x |= mask;
  else
    x &= ~mask;
  outportb(port, x);
}

struct int_tab
{
  __dpmi_paddr old;             /* Previous handler at this vector      */
  _go32_dpmi_seginfo new;       /* Current handler, with wrapper info   */
  void (*func)(int);            /* Function to call on interrupt        */
  int arg;                      /* Arg to pass to interrupt function    */
  int chain;                    /* Is interrupt chained to old handler? */
} Int_tab[16];

/* What a crock. All this inelegance should be replaced with something
 * that figures out what interrupt is being serviced by reading the 8259.
 */
static void
irq0(void)
{
  eoi();
  (*Int_tab[0].func)(Int_tab[0].arg);
}

static void
irq1(void)
{
  eoi();
  (*Int_tab[1].func)(Int_tab[1].arg);
}

static void
irq2(void)
{
  eoi();
  (*Int_tab[2].func)(Int_tab[2].arg);
}

static void
irq3(void)
{
  eoi();
  (*Int_tab[3].func)(Int_tab[3].arg);
}

static void
irq4(void)
{
  eoi();
  (*Int_tab[4].func)(Int_tab[4].arg);
}

static void
irq5(void)
{
  eoi();
  (*Int_tab[5].func)(Int_tab[5].arg);
}

static void
irq6(void)
{
  eoi();
  (*Int_tab[6].func)(Int_tab[6].arg);
}

static void
irq7(void)
{
  eoi();
  (*Int_tab[7].func)(Int_tab[7].arg);
}

static void
irq8(void)
{
  eoi();
  (*Int_tab[8].func)(Int_tab[8].arg);
}

static void
irq9(void)
{
  eoi();
  (*Int_tab[9].func)(Int_tab[9].arg);
}

static void
irq10(void)
{
  eoi();
  (*Int_tab[10].func)(Int_tab[10].arg);
}

static void
irq11(void)
{
  eoi();
  (*Int_tab[11].func)(Int_tab[11].arg);
}

static void
irq12(void)
{
  eoi();
  (*Int_tab[12].func)(Int_tab[12].arg);
}

static void
irq13(void)
{
  eoi();
  (*Int_tab[13].func)(Int_tab[13].arg);
}

static void
irq14(void)
{
  eoi();
  (*Int_tab[14].func)(Int_tab[14].arg);
}

static void
irq15(void)
{
  eoi();
  (*Int_tab[15].func)(Int_tab[15].arg);
}

static void (*Vectab[16])(void) =
{
  irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7,
  irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15
};

int
allocirq(unsigned irq, int chain, void (*func)(int), int arg)
{
  struct int_tab *ip;
  unsigned char   intno;
  int             i;

  if (irq > 15)
    return(-1);      /* IRQ out of legal range */

  ip = &Int_tab[irq];
  if (ip->func != NULL)
    return(-1);      /* Already in use */
/* Convert irq to actual CPU interrupt vector */
  intno = (irq < 8) ? irq + 8 : 0x70 + irq - 8;

  __dpmi_get_protected_mode_interrupt_vector(intno, &ip->old);
  ip->func = func;
  ip->arg = arg;
  ip->new.pm_offset = (int)Vectab[irq];
  ip->new.pm_selector = _go32_my_cs();
  ip->chain = chain;

  if (chain)
    return(_go32_dpmi_chain_protected_mode_interrupt_vector(intno, &ip->new));

  if ((i = _go32_dpmi_allocate_iret_wrapper(&ip->new)) != 0)
    return(i);
  return(_go32_dpmi_set_protected_mode_interrupt_vector(intno, &ip->new));
}

int
freeirq(unsigned irq)
{
  struct int_tab *ip;
  int             i;

  if (irq > 15)
    return(-1);      /* IRQ out of legal range */

  ip = &Int_tab[irq];
  ip->func = NULL;
/* Convert irq to actual CPU interrupt vector */
  irq = (irq < 8) ? irq + 8 : 0x70 + irq - 8;
  if (ip->chain)
    return(_go32_dpmi_unchain_protected_mode_interrupt_vector(irq, &ip->new));
  if((i = __dpmi_set_protected_mode_interrupt_vector(irq, &ip->old)) != 0)
    return(i);
  return(_go32_dpmi_free_iret_wrapper(&ip->new));
}

void
init_timer(void)
{
  unsigned long t1, t2, tx;

  tx = time(NULL);                /* Korrekturfaktor fuer kaputte Boards */
  while (tx == time(NULL));
    t1 = uclock();
  while (tx + 1 == time(NULL));
    t2 = uclock();
  if (t2 - t1 > 2000000)
  {
    xprintf("*** WARNING: Timer XTAL is 2.38Mhz\n");
    tim_factor = 2;
  }
  else
    tim_factor = 1;
  watchdog = 0;                                 /* erstmal auf 0 stellen */
  disable();                         /* einen Soft-Wachhund installieren */
  allocirq(0, 1, watch_dog_reset, 0);        /* wir haengen uns in IRQ 0 */
  oldtimer = getmask(0);                      /* alten IRQ-Vektor merken */
  maskon(0);
  enable();
}

void
exit_timer(void)
{
  disable();
  if (!oldtimer)
    maskoff(0);                     /* alten IRQ-Vektor wieder herstellen */
  freeirq(0);                      /* einmal mit neuer Maske freigeben ?! */
  enable();
}

void
update_timer(void)
{
  static uclock_t last_uclock = 0L;
  static uclock_t uclock_sum = 0L;
  uclock_t        now, diff;

  watchdog = 0;                 /* watchdog zuruecksetzen             */
  now = uclock();               /* aktuelle Uhrzeit merken            */
  diff = (now - last_uclock)    /* Zeit seit letzer Runde             */
              * tim_factor;     /* Korrekturfaktor                    */

  last_uclock = now;            /* den neuen Wert merken            */

  uclock_sum += diff;           /* aktueller Zaehlerstand           */
/* jetzt die vergangenen Tenticks aufaddieren und vom Zaehlerstand  */
/* wegstreichen, Ungenauigkeit etwa 1.2ms in 10 Sekunden. Da es     */
/* sich hier nur um einen Timer fuer LAPB (AX.25) handelt, ist das  */
/* zu verkraften.                                                   */
  tic10 += (unsigned) (uclock_sum / (UCLOCKS_PER_SEC / 100));
  uclock_sum %= (UCLOCKS_PER_SEC / 100);

  fflush(stdout);
}

void
watch_dog_reset(int nix)
{
  nix = 1;      /* Dummy-Variable. Ist beim IRQ-Aufruf so definiert!! */
  if (++watchdog > WATCH_TIME)
  {
    sprintf(debugfile, "%s\\debug.txt", textcmdpath);
    aus = fopen(debugfile, "at");
    fprintf(aus, "Der Wachhund hat zugebissen!!\n");
    fprintf(aus, "Letzter Befehl war : %s\n", clilin);
    zeit = localtime(&sys_time);
    fprintf(aus, "%02d.%02d.%02d %02d:%02d\n\n", zeit->tm_mday,
                                                 zeit->tm_mon+1,
                                                 zeit->tm_year%100,
                                                 zeit->tm_hour,
                                                 zeit->tm_min);
    fclose(aus);
    reboot_system();                                 /* und Schulz is */
  }
}

/* Written to extend gopint.c in djgpp library */
int _go32_dpmi_unchain_protected_mode_interrupt_vector(unsigned irq,_go32_dpmi_seginfo *info)
{
  __dpmi_paddr v;
  char *stack;
  char *wrapper;

  __dpmi_get_protected_mode_interrupt_vector(irq,&v);
  /* Sanity check: does the vector point into our program? A bug in gdb
   * keeps us from hooking the keyboard interrupt when we run under its
   * control. This test catches it.
   */
  if (v.selector != _go32_my_cs())
    return(-1);
  wrapper = (char *)v.offset32;
  /* Extract previous vector from the wrapper chainback area */
  v.offset32 = *(long *)(wrapper + 0x5b);
  v.selector = *(short *)(wrapper + 0x5f);
  /* Extract stack base from address of _call_count variable in wrapper */
  stack = (char *)(*(long *)(wrapper+0x0F) - 8);
#define STACK_WAS_MALLOCED      (1 << 0)

  if (*(long *) stack & STACK_WAS_MALLOCED)
    free(stack);
  free(wrapper);
  __dpmi_set_protected_mode_interrupt_vector(irq, &v);
  return(0);
}

/* Re-arm 8259 interrupt controller(s)
 * Should be called just after taking an interrupt, instead of just
 * before returning. This is because the 8259 inputs are edge triggered, and
 * new interrupts arriving during an interrupt service routine might be missed.
 */
void eoi(void)
{
/* read in-service register from secondary 8259                         */
  outportb(0xa0, 0x0b);
  if(inportb(0xa0))
    outportb(0xa0, 0x20);               /* Send EOI to secondary 8259   */
  outportb(0x20, 0x20);                 /* Send EOI to primary 8259     */
}

/* End of os/go32/go32.c */

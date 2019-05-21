/************************************************************************/
/*                                                                      */
/*                                                                      */
/*    *****                       *****                                 */
/*      *****                   *****                                   */
/*        *****               *****                                     */
/*          *****           *****                                       */
/*  ***************       ***************                               */
/*  *****************   *****************                               */
/*  ***************       ***************                               */
/*          *****           *****           TheNetNode - Tools          */
/*        *****               *****         (PC)                        */
/*      *****                   *****       Public Domain               */
/*    *****                       *****     NORD><LINK                  */
/*                                                                      */
/* This software is public domain ONLY for non commercial use           */
/*                                                                      */
/* output.c             Fernsteuerung ueber Printer Port                */
/*                                                                      */
/* 05-01-91, DF2AU : Urversion                                          */
/* 06-02-94, DB2OS : zusaetzliche Status-Ausgabe                        */
/* 09-30-00, DH6BB : Anpassung fuer Linux. Erfordert ROOT !!            */
/*                   (gcc -O2 -o output output.c                        */
/* 07-21-05, DH6BB : Anpassung von Christoph DK2CRN eingepflegt         */
/*                   (400ms warten nach Portausgabe)                    */
/* 07-21-05, DG9OBU: gewuenschtes Bit auf Gueltigkeit pruefen           */
/************************************************************************/

#include "tnn.h"

#define __NAME__     "\nOutput"
#define __VER__      "2.0"
#define __AUTHOR__   "pG"        /* Monogramm des Autors/Programmierers */

#ifdef __LINUX__
#include <sys/io.h>
#define inportb(adr) inb(adr)
#define outportb(adr, data) outb(data, adr)
#define port_adresse 0x378       /* LPT 1                               */
#else   /* GO32 */
#include <sys/farptr.h>
#define port_adresse _farpeekw(_dos_ds,0x00408)
#endif

char version[] = __NAME__ ", Version " __VER__ __AUTHOR__
                 " ("__DATE__  ", "  __TIME__  ")\r";

/*----------------------------------------------------------------------*/
/* Fehler im Aufruf melden                                              */
/*----------------------------------------------------------------------*/
void usage_error(void)
{
  puts(version);
  printf("Syntax: OUTPUT [<bit> <0|1>]\n");
  printf("        (using io-port 0x%x)\n", port_adresse);
  exit(1);
}

/*----------------------------------------------------------------------*/
/* Port Bit setzen                                                      */
/*----------------------------------------------------------------------*/
void set_port(int io_port)
{
  if (io_port < 8)
  {
    outportb(port_adresse, (inportb(port_adresse) | (0x01 << io_port)));
    /* setzt STROBE-Bit auf LOW-Position (invertiert) */
    outportb(port_adresse + 2, (inportb(port_adresse + 2) | (0x01 << 0)));
    /* 400ms warten */
    usleep(400000);
    /* setzt STROBE-Bit auf HIGH-Position (invertiert) */
    outportb(port_adresse + 2, (inportb(port_adresse + 2) & ~(0x01 << 0)));
  }
  else
    usage_error();
}

/*----------------------------------------------------------------------*/
/* Port Bit loeschen                                                    */
/*----------------------------------------------------------------------*/
void reset_port(int io_port)
{
  if (io_port < 8)
  {
    outportb(port_adresse, (inportb(port_adresse) & ~(0x01 << io_port)));
    /* setzt STROBE-Bit auf LOW-Position (invertiert)  */
    outportb(port_adresse + 2, (inportb(port_adresse + 2) | (0x01 << 0)));
    /* 400ms warten */
    usleep(400000);
    /* setzt STROBE-Bit auf HIGH-Position (invertiert) */
    outportb(port_adresse + 2, (inportb(port_adresse + 2) & ~(0x01 << 0)));
  }
  else
    usage_error();
}

/*----------------------------------------------------------------------*/
/* Port Bit lesen                                                       */
/*----------------------------------------------------------------------*/
int read_port(int io_port)
{
  if (io_port < 8)
    return((inportb(port_adresse) >> io_port) & 1);
  else
  {
    usage_error();
    return(0);
  }
}

/*----------------------------------------------------------------------*/
/* Port Status anzeigen                                                 */
/*----------------------------------------------------------------------*/

void show_port(void)
{
  int ioport;

  printf("\nOutput-Status:\n 7 6 5 4 3 2 1 0\n");
  for (ioport = 7; ioport >= 0; ioport--)
    printf(" %1.1d", read_port(ioport));

  printf("\n");
}

/*----------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
  int bit = -1;

#ifdef __LINUX__
  /* direkten Portzugriff beim Kernel anfordern */
  if (ioperm(port_adresse, 3, 1))
  {
    perror("Can't get permissions for port-access. (Are you \'root\' ?)");
    exit(1);
  }
#endif

  if (argc == 2)  /* Nur Status ausgeben */
  {
    show_port();
    return(0);
  }

  if (argc == 4)  /* Port setzen/loeschen, 4 da TNN Rufzeichen anfuegt */
  {
    switch (*argv[2])
    {
      case '0': bit = atoi(argv[1]);

                if (bit < 0 || bit > 7 )
                {
                  usage_error();
                  exit(1);
                }

                reset_port(bit);
                show_port();
                break;

      case '1': bit = atoi(argv[1]);

                if (bit < 0 || bit > 7 )
                {
                  usage_error();
                  exit(1);
                }

                set_port(bit);
                show_port();
                break;

      default:  usage_error();
                exit(1);
    }
    return(0);
  }
  usage_error();

#ifdef __LINUX__
  /* Port wieder freigeben */
  if (ioperm(port_adresse, 3, 0))
  {
    perror("Can't release io-permissions !");
    exit(1);
  }
#endif

  return(1);
}

/*----------------------------------------------------------------------*/
/*--- Ende von output.c                                              ---*/

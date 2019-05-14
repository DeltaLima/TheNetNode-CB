#include "tnn.h"
#ifdef L1TELNET

static T_INTERFACE *ifpp;                /* Zeiger auf das Aktuelle Interface.*/

/* TELNET-Server Einstellung aendern/setzen. */
void ccptelnet(void)
{
  MBHEAD         *mbp;
  char            ch;
  int             tmp_fd       = EOF;
  int             newloglevel  = 0;
  int             new_tcp_port = 0;
#ifdef DEBUG_MODUS
  lastfunc = "l1telnet(ccptelnet)";
#endif /* DEBUG_MODUS */

  ifpp = &ifp[TEL_ID];               /* Dann Zeiger auf das TELNET-Interface. */

  if (issyso() && skipsp(&clicnt, &clipoi))           /* Sysop will aendern.  */
  {
    clicnt--;
    ch = toupper(*clipoi++);

    switch (ch)
    {
      case 'P':                             /* Sysop will Telnet-Port aéndern */
        if (!skipsp(&clicnt, &clipoi))
        {
          putmsg("Invalid Parameter\r");
          return;
        }

        new_tcp_port = nxtlong(&clicnt, &clipoi);      /* Neuen Port ermitteln */

        if (  (new_tcp_port < 1)
            ||(new_tcp_port > 65535))
        {
          putmsg("TCP-Port not valid, not changed !!!\r");
          return;
        }

        if (ifpp->actively == FALSE)                    /* Telnet ist deaktiv.*/
        {
          putmsg("TCP-Port haven not switched on!\r");
          return;
        }
        /* Wenn NEUER Port und ALTER Port gleich sind, nicht neu Initialisieren.*/
        if (ifpp->tcpport == Htons((unsigned short)new_tcp_port))
        {
          putmsg("TCP-Port successfully changed\r");
          return;
        }

                                           /* Neuen TCP-Port Initialisieren. */
        if ((tmp_fd = SetupTCP(ifpp->name, (unsigned short)new_tcp_port)) != EOF)
        {
#ifdef OS_STACK
          int tmp_fd_OS;

          if ((tmp_fd_OS = SetupOS(ifpp, (unsigned short)new_tcp_port)) != EOF)
          {
            /* alten Socket schliessen */
            close(ifpp->OsSock);
            /* Neuen Socket merken */
            ifpp->OsSock = tmp_fd_OS;
          }
          else
            {
              Close(tmp_fd);
              putmsg("ERROR: Changing UDP-Port failed, Port not changed !!!\r");
              return;
            }
#endif /* OS_STACK */

          Close(ifpp->ISock);                   /* Alten Socket schliessen. */
          ifpp->ISock = tmp_fd ;                     /* Neuen Socket merken */
        }
        else
          {              /* Neuen TCP-Port liess sich nicht Initialisieren. */
            putmsg("ERROR: Changing UDP-Port failed, Port not changed !!!\r");
            return;
          }


        ifpp->tcpport = Htons((unsigned short)new_tcp_port);

        putmsg("TCP-Port successfully changed\r");
        return;

      case 'L':                                /* Sysop will LOGLevel aendern */
       if (!skipsp(&clicnt, &clipoi))
       {
         mbp = putals("Telnet-Server:\r");
         putprintf(mbp, "My LogLevel: %d\r",ifpp->log); /* Loglevel anzeigen. */

         prompt(mbp);
         seteom(mbp);
         return;
       }

       newloglevel = nxtnum(&clicnt, &clipoi);   /* neuen Loglevel ermitteln. */

       if (  (newloglevel < 0)
           ||(newloglevel > 3))                     /* Pruefe neuen Loglevel. */
       {
         mbp = putals("Telnet-Server:\r");
         putprintf(mbp, "Error: Log level worth from 0 to 3!\r");
         prompt(mbp);
         seteom(mbp);
         return;
       }

       ifpp->log = newloglevel;         /* Neuen Loglevel setzen und zeigen */
       mbp = putals("Telnet-Server:\r");
       putprintf(mbp, "My Loglevel: %d\r", ifpp->log);

       prompt(mbp);
       seteom(mbp);
       return;
      break;


      default:                                      /* Ungueltiger Parameter. */
        putmsg("Invalid Parameter\r");
       return;
     }
  }

  mbp = putals("Telnet-Server:\r");         /* Aktuelle Einstellungen zeigen. */

  putprintf(mbp, "My TCP-Port: %u\r", Ntohs(ifpp->tcpport));
  prompt(mbp);
  seteom(mbp);
}

 /* Aktuelle Zeichen auswerten. */
TRILLIAN GetContensTEL(char contens)
{
#ifdef DEBUG_MODUS
  lastfunc = "l1telnet(GetContensTEL)";
#endif /* DEBUG_MODUS */

  if (contens == CR)                                   /* Zeichen ein Return. */
    tcppoi->cr = TRUE;                   /* Markiere, das es ein Return gibt. */

  if (contens == LF)         /* Zeichen ein LF-Return. LF-Return nicht setzen.*/
    return(NO);                                     /* zum naechsten Zeichen. */

  if (contens == (int)NULL)                      /* Alle Zeichen verarbeitet. */
  {
    if (tcppoi->cr)                                      /* Return war dabei. */
    {
      tcppoi->cr = FALSE;                            /* Return zuruecksetzen. */
      return(YES);                                     /* Frame ist komplett. */
    }
    else
      return(ERRORS);                                        /* Return fehlt. */
  }

  if (!tcppoi->login)                      /* User ist noch nicht eingeloggt. */
  {
    if (CheckContens(contens))      /* Login-Zeichen auf Gueltigkeit pruefen. */
      return(NO);                   /* Ungueltige Zeichen werden ignoriert.   */
  }

  tcppoi->cmd[tcppoi->cmdlen++] = contens;        /* Aktuelle Zeichen setzen. */
  return(NO);                                       /* Zum naechsten Zeichen. */
}

#endif /* L1TELNET */

/* End of src/l1telnet.c */

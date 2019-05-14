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
/* File os/linux/6pack.c (maintained by: DG9OBU)                        */
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

#ifdef SIXPACK

#include "6pack.h"

/************************************************************************/
/* Initialisieren einer TNC-Struktur                                    */
/************************************************************************/
void initTNC(UBYTE uTNC)
{
  UWORD uResetCounter;

  /* gewuenschte TNC-Struktur der Liste initialisieren */
  if (uTNC < MAX_TNC)
  {
    /* Reset-Counter konservieren */
    uResetCounter = RING[uTNC].uReset;

    memset(&RING[uTNC], 0, sizeof(TNC));

    RING[uTNC].uReset = uResetCounter;
  }
}

/************************************************************************/
/* Sixpack-Kommunikation mit den angeschlossenen TNC (RX, TX, DCD)      */
/************************************************************************/
void Sixpack_Housekeeping(void)
{
  register UBYTE uTNC;
  UBYTE cCmd[1];

  /* Ring steht, wir koennen nix machen */
  if (uRingStatus == RING_HALT)
    return;

  /* Ring laeuft, Watchdogs checken */
  if (uRingStatus == RING_RUN)
  {
    /* Watchdog fuer haengende TNC */
    for (uTNC = 0; uTNC < MAX_TNC; ++uTNC)
    {
      /* Gibt es diesen TNC ? Falls nicht, dann kann auch keiner mehr kommen */
      if (RING[uTNC].bPresent == FALSE)
        break;

      /* Zu lange nichts mehr gehoert, dann Ring neu starten */
      if ((tic10 - RING[uTNC].tLastHeard) > TNC_TIMEOUT)
      {
        RING[uTNC].uReset++;
        uRingStatus = RING_IDLE;
        LOGL3("TNC %u: TNC seit %ld ms still, reinitialisiere Ring !", uTNC, (tic10 - RING[uTNC].tLastHeard) * 10);
        LOGL3("tic10 ist %ld, LastHeard = %ld", tic10, RING[uTNC].tLastHeard);
        break;
      }

      /* normaler Watchdog fuer TNC-Anwesenheit */
      if ((tic10 - RING[uTNC].tWatchdog) > TNC_WATCHDOG)
      {
        cCmd[0] = cmdLED(uTNC);
        toRing(cCmd, 1);
        RING[uTNC].tWatchdog = tic10;

        /* RXC darf nicht ohne DCD gesetzt sein, ggf. korrigieren */
        if ((RING[uTNC].bDCD == FALSE) && (RING[uTNC].cRXC))
        {
          RING[uTNC].cRXC = 0;
          LOGL3("TNC %u: RXC-Korrektur !", uTNC);
        }
      }

      /* Festhaengende PTT loesen */
      if ((RING[uTNC].tPTTWatchdog != 0) && (tic10 > RING[uTNC].tPTTWatchdog))
      {
        /* sendet dieses TNC momentan ? */
        if (RING[uTNC].cTXC > 0)
        {
          RING[uTNC].cTXC--;
          LOGL3("TNC %u: TXC-Korrektur !", uTNC);

          /* noch Sendeframes im TNC, dann wieder scharf machen */
          if (RING[uTNC].cTXC > 0)
          {
            /* wir nehmen die Aussendung eines Frames maximaler Laenge, */
            /* TX-Delay, TX-Tail und etwas Sicherheit an */
            RING[uTNC].tPTTWatchdog = (  tic10
                                       + (2 * portpar[uMap_TNCtoPort[uTNC]].txdelay)
                                       + (21120 / portpar[uMap_TNCtoPort[uTNC]].speed)
                                      );
          }
          else
          {
            RING[uTNC].tPTTWatchdog = 0; /* Wachhund aus */
          }
        }
        else
        {
          /* TNC sendet nicht, aber Wachhund auch angekettet ? */
          if (RING[uTNC].tPTTWatchdog != 0)
          {
            RING[uTNC].tPTTWatchdog = 0;
            LOGL3("TNC %u: PTT-Watchdog entlaufen, wieder eingefangen !", uTNC);
          }
        }
      }
    }
  }

  /* Ring nicht initialisiert */
  if (uRingStatus == RING_IDLE)
  {
    /* TNC-Strukturen initialisieren */
    for (uTNC = 0; uTNC < MAX_TNC; ++uTNC)
      initTNC(uTNC);

    /* Zaehlung starten */
    startCount();

    /* Zurueck zu anderen Aufgaben */
    return;
  }

  /* Daten vom Ring holen (Daten, Meldungen etc.) */
  Sixpack_RX();

  /* So lange wir auf das Zaehlpaket warten, keine Sendedaten auf */
  /* den Ring legen, aber ggf. noch ein Zaehlpaket senden. */
  if (uRingStatus == RING_INIT)
  {
    /* Falls nach zehn Sekunden kein Zaehlpaket kam, wieder eines senden */
    if ((tic10 - tLastInit) >= PROBE_TIMEOUT)
    {
      uRingStatus = RING_IDLE;
      LOGL2("-RING-: Ring laeuft nicht, reinitialisiere ...");
    }

    return;
  }

  /* Sendedaten zu den TNC bringen */
  Sixpack_TX();
}

/************************************************************************/
/* Liefert den DCD-Status eines TNC                                     */
/* ACHTUNG, die "+" beim Zusammenbau des Status sollen da sein !        */
/* (normalerweise waeren es "|=", aber die funktionieren hier nicht)    */
/************************************************************************/
WORD Sixpack_DCD(UWORD uPort)
{
  WORD uDCD = 0;
  register UBYTE uTNC;   /* Nummer des zustaendigen TNC ermitteln */

  if (uPort <= L2PNUM)
    uTNC = uMap_PorttoTNC[uPort];
  else
    return (0);

  /* Kein TNC fuer diesen Port */
  if (uTNC == 0xFF)
    return (0);

  /* DCD vom TNC gemeldet ? */
  if (RING[uTNC].bDCD == TRUE)
    uDCD |= (WORD)(DCDFLAG);

  /* Sendet der TNC gerade oder Calibrate ? */
  if (   (RING[uTNC].cTXC > 0)
      || (RING[uTNC].cTestCount > 0)
     )
    uDCD |= (WORD)(PTTFLAG);

  /* Empfaengt der TNC momentan ein Frame ? */
/*
  if (RING[uTNC].cRXC > 0)
    uDCD += (WORD)(RXBFLAG);
*/
  /* DCD-Zustand melden */
  return (uDCD);
}

/************************************************************************/
/* Verarbietung von normalen 6PACK-Meldungen                            */
/************************************************************************/
void processSixpackCmd(UBYTE cByte)
{
  UBYTE uTNC = (cByte & MASK_TNCNUM);
  UBYTE cCmdBuf[1];

  /* Puffer holen und Empfangsport eintragen */
  MBHEAD *rxfhd = NULL;

  /* LED-Kommandos, die keinen TNC fanden ignorieren wir */
  if ((cByte & CMD_LED) == CMD_LED)
    return;

  /* Start of Frame, TX-Underrun, RX-Overrun, RX-Buffer Overflow */
  if ((cByte & MASK_NORMCMD) == MASK_NORMCMD)
  {
    /* nur die oberen Bits interessieren uns hier */
    if ((cByte & MASK_CMD) == CMD_SOF)
    {
      /* Start of Frame */
      LOGL4("TNC %u: 'Start of Frame' (%sleitend)", uTNC, ((cReceiveFrom != 0xFF) ? "aus" : "ein"));

      switch (uRXStatus)
      {
        /* Einleitendes SOF */
        case RX_WAIT_SOF:
          /* RX beginnt, erstes SOF am Frameanfang */
          if (cReceiveFrom == 0xFF)
          {
            LOGL2("TNC %u: RX beginnt, RXC ist jetzt %u)", uTNC, RING[uTNC].cRXC);

            if (RING[uTNC].cRXC == 0)
              LOGL3("TNC %u: 'Start of Frame' waehrend RXC noch null !!!", uTNC);

            if (uRXRawBufferSize != 0)
              LOGL3("TNC %u: RX-Rohdatenpuffer schon vorgefuellt !!!", uTNC);

            /* merken, von welchem TNC wir empfangen */
            cReceiveFrom = uTNC;

            /* Puffer ruecksetzen */
/*
            memset(cRXRawBuffer, 0, sizeof(cRXRawBuffer));
            uRXRawBufferSize = 0;
            memset(cRXBuffer, 0, sizeof(cRXBuffer));
            uRXBufferSize = 0;
*/
            /* CON-LED des betreffenden TNC einschalten */
            setLED(uTNC, LED_CON);
            cCmdBuf[0] = cmdLED(uTNC);
            toRing(cCmdBuf, 1);
          }
          else
            LOGL3("TNC %u: RX-TNC bei erstem SOF schon gesetzt !", uTNC);

          /* Empfangszustand merken */
          uRXStatus = RX_WAIT_END;
          break;

        /* Wir empfangen und haben beendendes SOF erhalten */
        case RX_WAIT_END:
          if (cReceiveFrom != uTNC)
            LOGL3("TNC %u: RX-TNC bei Frameende ungleich TNC (%u) bei Frameanfang !", uTNC, cReceiveFrom);

          /* Pruefen, ob Empfangsdaten da sind, wenn nein, dann sind */
          /* wir sehr wahrscheinlich out of sync und empfangen einfach */
          /* mal weiter, da wir jetzt erst wahrscheinlich am Frame- */
          /* anfang sind. */
          if (uRXRawBufferSize == 0)
          {
            /* wie bei normalem RX-Beginn ... */
            /* merken, von welchem TNC wir empfangen */
            cReceiveFrom = uTNC;

            /* Puffer ruecksetzen */
/*
            memset(cRXRawBuffer, 0, sizeof(cRXRawBuffer));
            uRXRawBufferSize = 0;
            memset(cRXBuffer, 0, sizeof(cRXBuffer));
            uRXBufferSize = 0;
*/
            /* CON-LED des betreffenden TNC einschalten */
            setLED(uTNC, LED_CON);
            cCmdBuf[0] = cmdLED(uTNC);
            toRing(cCmdBuf, 1);

            LOGL2("TNC %u: Resync !!!", uTNC);

            return;
          }

          /* Empfangene Daten dekodieren */
          if (DecodeRawBuffer(cReceiveFrom))
          {
            /* Checksumme ist korrekt, Daten an L2 geben    */
            /* Hier koennen auch Daten von TNC ankommen,    */
            /* die keinem Port zugeordnet sind. Deren Daten */
            /* werfen wir weg. */
            if (portenabled(uMap_TNCtoPort[cReceiveFrom]))
            {
              /* Port ist eingeschaltet, dann Daten in den Empfangspuffer */
              register UWORD uByteCounter;

              /* Puffergroesse korrigieren (TX-Delay und Checksumme) */
              uRXBufferSize -= 2;

              /* Puffer holen */
              (rxfhd = (MBHEAD *)allocb(ALLOC_MBHEAD))->l2port = uMap_TNCtoPort[cReceiveFrom];

              /* Frame in Buffer umkopieren */
              for (uByteCounter = 1; uByteCounter <= uRXBufferSize; ++uByteCounter)
                putchr(cRXBuffer[uByteCounter], rxfhd);

              /* Frame zum L2 geben */
              relink((LEHEAD *)rxfhd, (LEHEAD *)rxfl.tail);
            }
          }
          else
          {
            /* Checksumme fehlerhaft */
            LOGL3("TNC %u: Checksumme fehlerhaft !!!", uTNC);
            RING[cReceiveFrom].uChecksumErr++;
          }

          /* CON-LED ausschalten */
          resetLED(cReceiveFrom, LED_CON);
          cCmdBuf[0] = cmdLED(cReceiveFrom);
          toRing(cCmdBuf, 1);

          /* Frameempfang vermerken */
          if (RING[cReceiveFrom].cRXC > 0)
            RING[cReceiveFrom].cRXC--;

          /* Empfangsvariablen neu initialisieren und Puffer ruecksetzen */
          cReceiveFrom = 0xFF;
          uRXStatus = RX_WAIT_SOF;

          memset(cRXRawBuffer, 0, sizeof(cRXRawBuffer));
          uRXRawBufferSize = 0;
          memset(cRXBuffer, 0, sizeof(cRXBuffer));
          uRXBufferSize = 0;

          LOGL3("TNC %u: RX beendet, RXC ist jetzt %u", uTNC, RING[uTNC].cRXC);
          break;

        default: break;
      }
    } /* if (SOF) */

    switch (cByte & MASK_CMD)
    {
      /* TX-Underrun */
      case MSG_TXU  :   /* TX-Underrun waehrend des Sendens des Testsignals ? */
                        if (RING[uTNC].cTestCount > 0)
                        {
                          RING[uTNC].cTestCount = 0;
                          resetLED(uTNC, LED_STA | LED_CON);
                          cCmdBuf[0] = cmdLED(uTNC);
                          toRing(cCmdBuf, 1);
                        }

                        RING[uTNC].uTXU++;
                        RING[uTNC].cTXC = 0;
                        LOGL3("TNC %u: TX-Underrun (neuer Zaehlerstand: %u)", uTNC, RING[uTNC].uTXU);
                        break;

      /* RX-Overrun */
      case MSG_RXO  :   RING[uTNC].uRXO++;
                        LOGL3("TNC %u: RX-Overrun (neuer Zaehlerstand: %u)", uTNC, RING[uTNC].uRXO);
                        break;

      /* RX Buffer Overflow */
      case MSG_RXB  :   RING[uTNC].uRXB++;
                        LOGL3("TNC %u: RX Buffer-Overflow (neuer Zaehlerstand: %u)", uTNC, RING[uTNC].uRXB);
                        break;

      default : break;
    } /* switch */
  } /* if (... NORMCMD) */
}

/************************************************************************/
/* Verarbeitung von 6PACK-Prioritaetskommandos                          */
/************************************************************************/
void processSixpackPrioCmd(UBYTE cByte)
{
  UBYTE uTNC = (cByte & MASK_TNCNUM);

  /* Prioritaetsmeldungen */
  if ((cByte & MASK_CALCNT) != 0)
  {
    LOGL4("TNC %u: Prioritaetskommando \"Zaehlung/Calibrate\" empfangen", uTNC);

    /* Calibrate, Kanalzuweisung */
    if ((cByte & MASK_CHAN) != 0)
    {
      /* Kanalzuweisung / Zaehlung */
      LOGL1("TNC %u: Kanalzuweisung empfangen", uTNC);
    }
    else
    {
      UBYTE cCmdBuf[2];

      /* Calibrate */
      LOGL2("TNC %u: Calibrate-Paket empfangen, Calibrate-Zaehler des TNC ist %u", uTNC, RING[uTNC].cTestCount);

      /* TNC meldet beendetes Calibrate, wenn noch Calibrates uebrig, dann */
      /* wieder ein Kommando schicken und STA-Led einschalten */
      if (RING[uTNC].cTestCount > 1)
      {
        RING[uTNC].cTestCount--;
        setLED(uTNC, LED_STA);
        cCmdBuf[0] = cmdLED(uTNC);
        cCmdBuf[1] = cmdCAL(uTNC);
        toRing(cCmdBuf, 2);
        LOGL2("TNC %u: Calibrate-Paket gesendet, Calibrate-Zaehler des TNC ist jetzt %u", uTNC, RING[uTNC].cTestCount);
      }
      else
      {
        /* letztes Calibrate beendet, dann STA-Led ausschalten */
        RING[uTNC].cTestCount = 0;
        resetLED(uTNC, LED_STA);
        cCmdBuf[0] = cmdLED(uTNC);
        toRing(cCmdBuf, 1);
        LOGL2("TNC %u: Calibrate beendet", uTNC);
      }
    }
  }
  else
  {
    /* DCD, RX-Zaehler, TX-Zaehler */
    /* DCD immer uebernehmen */
    RING[uTNC].bDCD = ((cByte & MASK_DCD) == MASK_DCD ? TRUE : FALSE);

    /* RX-Zaehler + 1 */
    /* Das Kommando ist nur erlaubt, wenn auch DCD gesetzt ist */
    if ((cByte & MASK_RXC) == MASK_RXC)
    {
      if (RING[uTNC].bDCD == TRUE)
      {
        RING[uTNC].cRXC++;
        LOGL2("TNC %u: 'RX-Zaehler + 1', RXC ist jetzt %u", uTNC, RING[uTNC].cRXC);
      }
      else
        LOGL2("TNC %u: 'RX-Zaehler + 1' ohne DCD, ignoriere.", uTNC);
    }

    /* TX-Zaehler + 1 */
    if ((cByte & MASK_TXC) == MASK_TXC)
    {
      if (RING[uTNC].cTXC > 0)
      {
        RING[uTNC].cTXC--;

        /* noch Frames in der Sendung, dann Wachhund scharf machen */
        if (RING[uTNC].cTXC > 0)
        {
          /* wir nehmen die Aussendung eines Frames maximaler Laenge, */
          /* TX-Delay, TX-Tail und etwas Sicherheit an */
          RING[uTNC].tPTTWatchdog = (  tic10
                                     + (2 * portpar[uMap_TNCtoPort[uTNC]].txdelay)
                                     + (21120 / portpar[uMap_TNCtoPort[uTNC]].speed)
                                    );
        }
        else /* Aussendung beendet, Wachhund wieder anketten */
        {
          RING[uTNC].tPTTWatchdog = 0;
        }
      }
      else /* keine Sendung, ist der Hund auch angekettet ? */
      {
        if (RING[uTNC].tPTTWatchdog != 0)
          RING[uTNC].tPTTWatchdog = 0;
      }

      LOGL2("TNC %u: 'TX-Zaehler + 1', TXC ist jetzt %u", uTNC, RING[uTNC].cTXC);
    }

    /* TNC lebt noch */
    RING[uTNC].tLastHeard = tic10;
  }
}

/************************************************************************/
/* 6PACK-Empfang                                                        */
/************************************************************************/
void Sixpack_RX(void)
{
  UBYTE cBuf[520];
  ssize_t       iRet;
  register int  iCounter;
  register int  iSelRet;
  UBYTE cRXByte;
  UBYTE uNumTNC;

  /* ganz wichtig, Linux modifiziert in select() diese Werte !!! */
  tv.tv_sec = 0;
  tv.tv_usec = 0;

  FD_ZERO(&rset);
  FD_SET(iDescriptor, &rset);

  iSelRet = select(iDescriptor + 1, &rset, NULL, NULL, &tv);

  /* Fehler bei select() ? */
  if (iSelRet < 0)
  {
    LOGL4("-RING-: Fehler %n bei select() : %s", errno, strerror(errno));
    Sixpack_l1exit();
    Sixpack_l1init();
    return;
  }

  /* select() hat keinen lesbaren Descriptor gefunden, */
  /* oder es ist nicht unserer */
  if ((iSelRet == 0) || (!FD_ISSET(iDescriptor, &rset)))
    return;

  memset(cBuf, 0, sizeof(cBuf));

  /* Daten holen */
  iRet = read(iDescriptor, cBuf, 512);

  /* Ist was gekommen ? */
  if (iRet > 0)
  {
    /* zeichenweise Verarbeitung */
    for (iCounter = 0; iCounter < iRet; ++iCounter)
    {
      /* Zeichen aus dem Puffer holen */
      cRXByte = cBuf[iCounter];

      /* Wir warten noch auf das Zaehlpaket, so lange kein anderer Empfang */
      if (uRingStatus == RING_INIT)
      {
        /* Zaehlkommando zurueckgekommen ? */
        if ((cRXByte & CMD_INIT) == CMD_INIT)
        {
          /* Kanalzuweisung / Zaehlung */
          uNumTNC = (cRXByte - CMD_INIT);

          LOGL1("-RING-: TNC-Zaehlung beendet, %u TNC gefunden", uNumTNC);

          /* Ring "kurzgeschlossen", noch einmal initialisieren */
          if (uNumTNC == 0)
          {
            uRingStatus = RING_IDLE;
            LOGL3("-RING-: kein TNC im Ring, Ring ist kurzgeschlossen !");
            return;
          }

          /* Ports mit TNC verbinden */
          if (assignPorts(uNumTNC) != uNumTNC)
            LOGL1("-RING-: TNC-Zaehlung weicht von konfigurierter Portanzahl ab !!!");
        }

        /* Falls nach zehn Sekunden kein Zaehlpaket kam, wieder eines senden */
        if ((tic10 - tLastInit) >= PROBE_TIMEOUT)
        {
          uRingStatus = RING_IDLE;
          LOGL2("-RING-: Ring laeuft nicht, reinitialisiere ...");
          return;
        }

        continue;
      }

      /* Meldungen und Daten verarbeiten */
      /* Prioritaetsmeldungen */
      if ((cRXByte & MASK_PRIOCMD) == MASK_PRIOCMD)
      {
        processSixpackPrioCmd(cRXByte);
        continue;
      }

      /* normale Meldungen */
      if ((cRXByte & MASK_NORMCMD) == MASK_NORMCMD)
      {
        processSixpackCmd(cRXByte);
        continue;
      }

      /* Daten */
      if ((cRXByte & MASK_DATA) == 0)
      {
        /* Daten zum falschen Zeitpunkt ? */
        if ((uRXStatus != RX_WAIT_END) || (cReceiveFrom == 0xFF))
        {
          LOGL3("-RING-: Daten ohne SOF (%x) !!! rxsize=%u", cRXByte, uRXRawBufferSize);
/*          continue; */
        }

        /* In den rohen Puffer damit */
        cRXRawBuffer[uRXRawBufferSize++] = cRXByte;

        continue;
      }

      LOGL1("-RING-: Daten ohne Ziel !!! (hex 0x%x, dez %u)", cRXByte, cRXByte);
    } /* for (...) */
  } /* if (iRet > 0) */
  else
  {
    /* wenn nach zehn Sekunden kein Zaehlpaket kam, dann noch eines senden */
    if (   (iRet == 0)
        && (uRingStatus == RING_INIT)
        && ((tic10 - tLastInit) >= PROBE_TIMEOUT)
       )
      uRingStatus = RING_IDLE;

    /* Schnittstellenfehler ! Das sollte NIE passieren ... */
    if ((iRet < 0) && (errno != EAGAIN) && (errno != EWOULDBLOCK))
    {
      /* Oh oh ... alles versuchen neu zu starten ... */
      LOGL2("-RING-: Fehler auf der Schnittstelle, start Ring neu !!!");
      Sixpack_l1exit();
      Sixpack_l1init();
    }
  }
}

/************************************************************************/
/* Dekodiert den gefuellten 6PACK-Rohdatenpuffer in den Nutzdatenpuffer */
/************************************************************************/
BOOLEAN DecodeRawBuffer(UBYTE uTNC)
{
  register UWORD uBufferPos;

  UBYTE cChecksum = uTNC;
  UBYTE cTmpByte = 0;
  UBYTE cBufCorrection = 0;

  /* Puffer auffuellen */
  while ((uRXRawBufferSize % 4) != 0)
  {
    ++uRXRawBufferSize;
    ++cBufCorrection;   /* Merken fuer Korrektur des dekodierten Puffers */
  }

  /* rohen Empfangspuffer decodieren */
  for (uBufferPos = 0; uBufferPos <= uRXRawBufferSize; ++uBufferPos)
  {
    register UBYTE cByte = cRXRawBuffer[uBufferPos];

    switch (uBufferPos % 4)
    {
      /* 1. 6PACK */
      case 0: cTmpByte = cByte;
              break;

      /* 2. 6PACK */
      case 1: cRXBuffer[uRXBufferSize++] = (cTmpByte | ((cByte << 2) & 0xC0));
              cTmpByte = (cByte & 0x0F);
              break;

      /* 3. 6PACK */
      case 2: cRXBuffer[uRXBufferSize++] = (cTmpByte | ((cByte << 2) & 0xF0));
              cTmpByte = (cByte & 0x03);
              break;

      /* 4. 6PACK */
      case 3: cRXBuffer[uRXBufferSize++] = (cTmpByte | (cByte << 2));
              break;

      default: break;
    }
  }

  /* Checksumme ueber den dekodierten Puffer berechnen */
  for (uBufferPos = 0; uBufferPos < uRXBufferSize; ++uBufferPos)
    cChecksum += cRXBuffer[uBufferPos];

  /* dekodierten Puffer in der Groesse korrigieren */
  uRXBufferSize -= cBufCorrection;

  /* Checksumme pruefen und entsprechend melden */
  if (cChecksum == 0xFF)
    return (TRUE);
  else
  {
    LOGL3("TNC %u: Checksummenfehler : %u Bytes roh, %u Byte dekodiert, Checksumme %u",
           uTNC, uRXRawBufferSize, uRXBufferSize, cChecksum);
    return (FALSE);
  }
}

/************************************************************************/
/* Sendet alle fuer einen Port anstehenden Frames zum TNC               */
/************************************************************************/
void sendSixpack(UBYTE uTNC)
{
  UBYTE cCheckSum;
  UBYTE cRawBuffer[512];
  UBYTE cSendBuffer[512];
  UWORD uRawBufferSize;
  UWORD uSendBufferSize;
  register UWORD uCounter;

  MBHEAD *txfhdl;
  LHEAD  *l2flp;

  /* Portnummer fuer den TNC holen */
  UWORD uPort = uMap_TNCtoPort[uTNC];

  /* Sicher ist sicher ... */
  if (kissmode(uPort) != KISS_6PACK)
    return;

  /* Sendeliste des Ports holen */
  l2flp = (LHEAD *)&txl2fl[uPort];

  /* Bei ausgeschalteten Ports die Sendeliste ohne Sendung kopieren */
  if (!portenabled(uPort))
  {
    while (l2flp->head != l2flp)
      relink(ulink((LEHEAD *) l2flp->head), (LEHEAD *)stfl.tail);
    return;
  }

  /* So lange Sendepakete da sind, diese auf den Ring geben */
  /* TNC aber nicht ueberfuellen */
  while (kick[uPort])
  {
    /* Sendelistenzeiger pruefen */
    if (l2flp->head == l2flp)
    {
      /* Zeiger steht auf Anfang, also doch nix zu senden */
      kick[uPort] = FALSE;
      return;
    }

    /* Puffer loeschen fuer neues Frame */
    memset(cSendBuffer, 0, sizeof(cSendBuffer));
    uSendBufferSize = 0;
    memset(cRawBuffer, 0, sizeof(cRawBuffer));
    uRawBufferSize = 0;

    /* STA-LED waehrend der Datenuebertragung zum TNC einschalten */
    setLED(uTNC, LED_STA);
    cSendBuffer[uSendBufferSize++] = cmdLED(uTNC);
    /* "TX-Zaehler + 1" und "Start of Frame" senden */
    cSendBuffer[uSendBufferSize++] = cmdTXC(uTNC);
    cSendBuffer[uSendBufferSize++] = cmdSOF(uTNC);

    /* TX-Delay in den rohen Puffer */
    cRawBuffer[uRawBufferSize++] = (UBYTE)(portpar[uPort].txdelay & 0xFF);

    /* Checksumme mit der TNC-Nummer initialisieren und TX-Delay hinzufuegen */
    cCheckSum = uTNC;
    cCheckSum += (UBYTE)(portpar[uPort].txdelay & 0xFF);

    /* Zeiger auf Sendeliste holen */
    ulink((LEHEAD *)(txfhdl = (MBHEAD *)l2flp->head));

    /* Rohdaten kopieren und Checksumme bilden */
    while (txfhdl->mbgc < txfhdl->mbpc)
    {
      register UBYTE cByte = getchr(txfhdl);
      cRawBuffer[uRawBufferSize++] = cByte;
      cCheckSum += cByte;
    }

    /* Checksumme dem Puffer hinzufuegen */
    cRawBuffer[uRawBufferSize] = (UBYTE)(0xFF - cCheckSum);

    /* Puffer codieren */
    for (uCounter = 0; uCounter <= uRawBufferSize; ++uCounter)
    {
      switch (uCounter % 3)
      {
        /* 1. Byte -> 1. & 2. 6PACK */
        case 0:   cSendBuffer[uSendBufferSize++] = (cRawBuffer[uCounter] & 0x3F);
                  cSendBuffer[uSendBufferSize] = ((cRawBuffer[uCounter] >> 2) & 0x30);
                  break;

        /* 2. Byte -> 2. & 3. 6PACK */
        case 1:   cSendBuffer[uSendBufferSize++] |= (cRawBuffer[uCounter] & 0x0F);
                  cSendBuffer[uSendBufferSize] = ((cRawBuffer[uCounter] >> 2) & 0x3C);
                  break;

        /* 3. Byte -> 3. & 4. 6PACK */
        case 2:   cSendBuffer[uSendBufferSize++] |= (cRawBuffer[uCounter] & 0x03);
                  cSendBuffer[uSendBufferSize++] = (cRawBuffer[uCounter] >> 2);

        default:  break;    /* das sollte _nie_ passieren ... */
      }
    }

    /* Sendepuffer "auffuellen" (terminiert das in Bearbeitung befindliche 6PACK) */
    if ((uRawBufferSize % 3) != 2)
      ++uSendBufferSize;

    /* abschliessendes SOF */
    cSendBuffer[uSendBufferSize++] = cmdSOF(uTNC);

    /* STA-LED wieder ausschalten */
    resetLED(uTNC, LED_STA);
    cSendBuffer[uSendBufferSize++] = cmdLED(uTNC);

    /* Auf den Ring damit */
    toRing(cSendBuffer, uSendBufferSize);

    /* Merken, dass ein Sendeframe unterwegs ist und Wachhunde scharf machen */
    RING[uTNC].cTXC++;
    RING[uTNC].tTXWatchdog = tic10;
    RING[uTNC].tPTTWatchdog = (  tic10
                               + (2 * portpar[uMap_TNCtoPort[uTNC]].txdelay)
                               + (21120 / portpar[uMap_TNCtoPort[uTNC]].speed)
                              );

    /* Frame in die gesendet-Liste umhaengen */
    relink((LEHEAD *)txfhdl, (LEHEAD *)stfl.tail);

    /* ein Frame gesendet, Sendewarteschlange aktualisieren */
    kick[uPort] = ((LHEAD *)l2flp->head != l2flp);

    LOGL3("TNC %u: Frame an TNC gesendet, TXC ist jetzt %u", uTNC, RING[uTNC].cTXC);
  } /* while (...) */
}

/************************************************************************/
/* Sendet ausstehende Frames zu den TNC, Kanalarbitrierung              */
/************************************************************************/
void Sixpack_TX(void)
{
  register UBYTE uTNC;
  register UWORD uPort;
  UBYTE cCmdBuf[1];

  /* Merker, ob wir senden duerfen */
  BOOLEAN bSendOK = FALSE;

  for (uTNC = 0; uTNC < MAX_TNC; ++uTNC)
  {
    /* Nur fuer angeschlossene TNC. Da die TNC-Nummern nur ununterbrochen */
    /* aufsteigend sein koennen, beim ersten nicht vorhandenen TNC raus,  */
    /* es kann keiner mehr danach kommen. */
    if (RING[uTNC].bPresent == FALSE)
      break;

    /* Wenn wir senden und die TX-Watchdogzeit abgelaufen ist,     */
    /* dann den aktuellen LED-Zustand zum TNC senden (TX-Watchdog) */
    if (   (   (RING[uTNC].cTestCount > 0)      /* Testsignal  */
            || (RING[uTNC].cTXC > 0)            /* unbestaetigte Frames */
           )
        && ((RING[uTNC].tTXWatchdog + TXWDTIME) < tic10))
    {
      /* aktuellen LED-Zustand als Watchdogpaket senden */
      cCmdBuf[0] = cmdLED(uTNC);
      toRing(cCmdBuf, 1);

      /* letzten Watchdogzeitpunkt merken */
      RING[uTNC].tTXWatchdog = tic10;
    }

    /* Port holen, der diesem TNC zugeordnet ist */
    uPort = uMap_TNCtoPort[uTNC];

    /* Arbiter fuer Kanalzugriff */
    /* Port hat was zu senden, TNC nicht ueberfuellen */
    if (kick[uPort] /* && RING[uTNC].cTXC < 5 */ )
    {
      LHEAD *l2flp;

      LOGL4("TNC %u: Port %u ist fuer Sendung markiert und hat Platz", uTNC, uPort);

      /* Sendeliste des Ports holen */
      l2flp = (LHEAD *)&txl2fl[uPort];

      /* Sendelistenzeiger pruefen */
      if (l2flp->head == l2flp)
      {
        /* Zeiger steht auf Anfang, doch nix zu senden */
        LOGL4("TNC %u: keine Sendedaten, eventuell Calibrate, keine Arbitrierung", uTNC);
        kick[uPort] = FALSE;
        return;
      }

      /* Arbitrierung notwendig ? */
      if ((fullduplex(uPort)) || (RING[uTNC].cTXC > 0))
      {
        LOGL4("TNC %u: Port ist Vollduplex oder sendet bereits, sende sofort", uTNC);
        bSendOK = TRUE;
      }
      else
      {
        /* DCD beruecksichtigen */
        if (!(Sixpack_DCD(uPort) & (WORD)(DCDFLAG)))
        {
          LOGL4("TNC %u: Kanal ist frei, Slottime ist %u ms", uTNC, portpar[uPort].slottime * 10);

          /* Hatten wir schon mal versucht zu senden ? (Slottime) */
          if (   (RING[uTNC].tNextArbit != 0L)
              && (RING[uTNC].tNextArbit > tic10)
             )
          {
            LOGL4("TNC %u: nicht genug Zeit seit letztem Arbit vergangen", uTNC);
          }
          else
          {
            LOGL4("TNC %u: genug Zeit seit letztem Arbit vergangen", uTNC);

            /* Kanalzugriff auswuerfeln (Persistenz) */
            if ((rand() % 255) <= (dama(uPort) ? 0xFF : (portpar[uPort].persistance & 0xFF)))
            {
              LOGL4("TNC %u: Zufallszahl kleiner, Sendung moeglich", uTNC);
              bSendOK = TRUE;
            }
            else
            {
              LOGL4("TNC %u: Zufallszahl zu gross, keine Sendung", uTNC);
              /* Merken, wann wir es zuletzt versucht hatten zu senden, */
              /* naechster Versuch erst nach Ablauf der Slottime. */
              RING[uTNC].tNextArbit = (tic10 + portpar[uPort].slottime);
            }
          }
        }
        else
        {
          LOGL4("TNC %u: DCD erkannt, kann nicht mehr senden", uTNC);
          /* Waren wir selbst grad in der Arbitrierung und wurden */
          /* durch die RX-DCD unterbrochen, dann Sendeversuch abbrechen. */
          if (RING[uTNC].tNextArbit != 0L)
          {
            RING[uTNC].tNextArbit = 0L;
            LOGL4("TNC %u: laufende Arbitrierung abgebrochen", uTNC);
          }
        }
      }

      if (bSendOK == TRUE)
      {
        /* Kanal frei und Sendung ok */
        LOGL4("TNC %u: Arbitrierung, sende !", uTNC);
        sendSixpack(uTNC);
        /* wir konnten Senden */
        RING[uTNC].tNextArbit = 0L;
      }
      else
      {
        /* Kanal blockiert, nicht senden */
        LOGL4("TNC %u: Arbitrierung nicht erfolgreich", uTNC);
      }
    } /* if ( kick[uPort] ... ) */
  } /* for ( ... ) */
}

/************************************************************************/
/* TNCs im Ring zaehlen / Zaehlpaket senden                             */
/************************************************************************/
void startCount(void)
{
  UBYTE cCmdBuf[1];

  /* Sicherheitscheck, ob wir die Zuordnungen jetzt aendern duerfen */
  if (uRingStatus != RING_IDLE)
    return;

  /* Zaehlkommando zusammenbauen und senden */
  cCmdBuf[0] = cmdINIT(0);
  toRing(cCmdBuf, 1);

  /* Zeitpunkt und Sendung merken */
  tLastInit = tic10;
  ++uNumProbes;

  /* Merken, dass wir auf das Zaehlpaket warten */
  uRingStatus = RING_INIT;
  LOGL4("-RING-: Zaehlpaket gesendet");
}

/************************************************************************/
/* Datenpuffer auf den Ring senden                                      */
/* (basierend auf writen() aus einem Stevens-Buch)                      */
/************************************************************************/
void toRing(UBYTE *pBuffer, UWORD uSize)
{
  UWORD uErrorCounter = 0;

  size_t                tLeft = uSize;
  ssize_t               tWritten;
  const UBYTE   *pBufPtr;

  pBufPtr = pBuffer;

  /* nur wenn der Ring offen ist duerfen wir hier rein */
  if (uRingStatus == RING_HALT)
    return;

  /* solange noch Daten zu senden sind */
  while (tLeft > 0)
  {
    if ((tWritten = write(iDescriptor, pBufPtr, tLeft)) <= 0)
    {
      if (errno == EINTR)
        tWritten = 0;
      else
      {
        if (++uErrorCounter >= 0xFF)
        {
          LOGL3("-RING-: Schnittstellenfehler %u (%s)", errno, strerror(errno));
          Sixpack_l1exit();
          Sixpack_l1init();
          return;
        }
      }
    }

    tLeft   -= tWritten;
    pBufPtr += tWritten;
  }
}

/************************************************************************/
/* Ordnet jedem 6PACK-Port einen freien TNC-Port                        */
/************************************************************************/
UBYTE assignPorts(UBYTE uAvailTNC)
{
  register UBYTE i;
  register UBYTE j = 0;

  UBYTE cCmdBuf[2];

  /* Sicherheitscheck, ob wir die Zuordnungen jetzt aendern duerfen */
  if (uRingStatus != RING_INIT)
    return (0);

  /* zuerst alle TNC als nicht vorhanden markieren */
  for (i = 0; i < MAX_TNC; ++i)
  {
    RING[i].bPresent  = FALSE;
    uMap_TNCtoPort[i] = 0;
  }

  /* und das Gleiche mit den Ports machen */
  for (i = 0; i < L2PNUM; ++i)
    uMap_PorttoTNC[i] = 0xFF;   /* kein TNC */

  /* Alle L2-Ports durchgehen */
  for (i = 0; (i < L2PNUM && j < uAvailTNC); ++i)
  {
    /* andere Ports nicht beachten */
    if (kissmode(i) != KISS_6PACK)
      continue;

    LOGL2("-RING-: Assoziiere TNC %u mit Port %u", j, i);

    /* TNC mit Port verbinden */
    RING[j].bPresent     = TRUE;
    RING[j].tWatchdog    = tic10;
    RING[j].tLastHeard   = tic10;
    RING[j].tPTTWatchdog = 0;
    uMap_PorttoTNC[i]    = j;
    uMap_TNCtoPort[j]    = i;

    /* bei gefundenen TNC die CON-LED einschalten */
    resetLED(j, LED_STA);
    setLED(j, LED_CON);
    cCmdBuf[0] = cmdLED(j);
    resetLED(j, LED_CON);
    toRing(cCmdBuf, 1);
    ++j;
  }

  /* Empfang und initialisieren */
  cReceiveFrom = 0xFF;
  uRXStatus = RX_WAIT_SOF;

  /* Ring freigeben fuer Datenverkehr */
  uRingStatus = RING_RUN;

  /* melden, wie viele Zuordnungen gemacht worden sind */
  return (j);
}

/************************************************************************/
/* L1-Kontrolle (Reset, Portparameter etc.)                             */
/************************************************************************/
void Sixpack_l1ctl(int iReq, UWORD uPort)
{
  UBYTE cCmdBuf[2];
  register UBYTE uTNC = uMap_PorttoTNC[uPort];

  /* nur 6PACK-Ports und davon auch nur die existierenden duerfen hier rein */
  if (   (kissmode(uPort) != KISS_6PACK)
      || (RING[uTNC].bPresent == FALSE)
     )
    return;

  /* Anfragen unterscheiden */
  switch (iReq)
  {
    /* Reset eines TNC. Das geht bei 6PACK nicht direkt, also muss der ganze */
    /* Ring dran glauben ... */
    case L1CRES : RING[uTNC].uReset++;
                  /* den Ring neu initialiseren */
                  uRingStatus = RING_IDLE;
                  LOGL2("TNC %u: TNC-Reset (neuer Zaehlerstand: %u)", uTNC, RING[uTNC].uReset);
                  break;

    /* Testsignal initiieren (4 x 15 Sekunden) */
    case L1CTST : RING[uTNC].cTestCount = 4;
                  setLED(uTNC, LED_STA);
                  cCmdBuf[0] = cmdCAL(uTNC);
                  cCmdBuf[1] = cmdLED(uTNC);
                  toRing(cCmdBuf, 2);
                  LOGL2("TNC %u: Testsignal initiiert", uTNC);
                  break;

    /* Portparameter zum TNC bringen, das braucht 6PACK nicht, */
    /* alles ausser TX-Delay machen wir selbst.                */
    case L1CCMD :
    default     : break;
  }
}

/************************************************************************/
/* Device einrichten                                                    */
/************************************************************************/
void Sixpack_l1init(void)
{
  register UWORD i;
  UWORD uFehler = 0;
  DEVICE *l1pp;
  struct  serial_struct ser_io;
  BOOLEAN bPortFound = FALSE;

  /* Zufallsgenerator initialisieren */
  srand(0);

  /* Erstes 6PACK-Device suchen fuer Lockfile */
  /* Alle L1-Ports durchgehen */
  for (i = 0; i < L1PNUM; ++i)
  {
    l1pp = &l1port[i];

    if (l1pp->kisstype == KISS_6PACK)
    {
      bPortFound = TRUE;
      break;
    }
  }

  /* haben wir einen Port gefunden ? */
  if (bPortFound == FALSE)
    return;

  /* Lock-File schreiben */
  l1pp->lock = -1;
  if (*(l1pp->tnn_lockfile) != '\0')
  {
    if (access(l1pp->tnn_lockfile, F_OK) == 0)
    {
      pid_t alienPID;
      FILE* lockFile;

      printf("6PACK-L1: Warning: Device %s seems to be locked by another by another process.\n", l1pp->device);
      LOGL1("Warning: Device %s seems to be locked by another by another process.", l1pp->device);

      if ((lockFile = fopen(l1pp->tnn_lockfile, "r")) != NULL)
      {
        fscanf(lockFile, "%i", &alienPID);
        fclose(lockFile);
        printf("6PACK-L1: Checking if process with PID %u is still alive ... ", alienPID);
        LOGL1("Checking if process with PID %u is still alive ...", alienPID);

        if ((kill(alienPID, 0) == -1) && (errno == ESRCH))
        {
          printf("No. Deleting stale lockfile.\n");
          LOGL1("No. Deleting stale lockfile.");
          unlink(l1pp->tnn_lockfile);
        }
        else
        {
          printf("Ohoh, active process found.\n6PACK-L1: I won't touch the port !!! Restart with \"-u\" to override.\n");
          LOGL1("Ohoh, active process found. I won't touch the port !!! Restart with \"-u\" to override.");
          uFehler = 1;
        }

      }
      else
      {
        printf("6PACK-L1: Sorry, can't open lockfile for reading locking pocess' PID.\n");
        LOGL1("Sorry, can't open lockfile for reading locking pocess' PID.");
      }

    }

    if ((uFehler == 0) && ((l1pp->lock = open(l1pp->tnn_lockfile, O_CREAT|O_EXCL, 0666)) == -1))
    {
      uFehler = 1;
      printf("Error: Device %s is locked by other user;\n"
             "       unable to create lockfile %s\n", l1pp->device, l1pp->tnn_lockfile);
      printf("       (%s)\n", strerror(errno));
      LOGL1("Error: Device %s is locked by other user;"
            "       unable to create lockfile %s", l1pp->device, l1pp->tnn_lockfile);
      LOGL1("       (%s)", strerror(errno));
    }
    else
    {
      char uBuf[16];
      memset(uBuf, 0, sizeof(uBuf));
      sprintf(uBuf, "%10d\n", getpid());
      write(l1pp->lock, uBuf, strlen(uBuf));
      close(l1pp->lock);
    }
  }

  /* Seriellen Port fuer Ein- und Ausgabe oeffnen */
  if (uFehler == 0)                  /* nur wenn Lock-File geschrieben */
  {                                  /* wurde oder nicht gefordert war */
    if ((iDescriptor = open(l1pp->device, O_RDWR)) == -1)
    {
      uFehler = 2;
      printf("Error: can't open device %s\n", l1pp->device);
      printf("       (%s)\n", strerror(errno));
      LOGL1("Error: can't open device %s", l1pp->device);
      LOGL1("       (%s)", strerror(errno));
    }
  }

  /* Einstellungen der seriellen Schnittstelle merken */
  if (uFehler == 0) /* nur wenn Port geoeffnet worden ist */
  {
    tcgetattr(iDescriptor, &(l1pp->org_termios));
    if (l1pp->speed == B38400)                        /* >= 38400 Bd  */
    {
      if (ioctl(iDescriptor, TIOCGSERIAL, &ser_io) < 0)
      {
        uFehler = 3;
        printf("Error: can't get actual settings for device %s\n", l1pp->device);
        printf("       (%s)\n", strerror(errno));
        LOGL1("Error: can't get actual settings for device %s", l1pp->device);
        LOGL1("       (%s)", strerror(errno));
      }
    }
  }

  /* Neue Einstellungen der seriellen Schnittstelle setzen */
  if (uFehler == 0)
  {
    l1pp->wrk_termios = l1pp->org_termios;
    l1pp->wrk_termios.c_cc[VTIME] = 0;        /* empfangene Daten     */
    l1pp->wrk_termios.c_cc[VMIN] = 0;         /* sofort abliefern     */
    l1pp->wrk_termios.c_iflag = IGNBRK;       /* BREAK ignorieren     */
    l1pp->wrk_termios.c_oflag = 0;            /* keine Delays oder    */
    l1pp->wrk_termios.c_lflag = 0;            /* Sonderbehandlungen   */
    l1pp->wrk_termios.c_cflag |=  (CS8        /* 8 Bit                */
                                  |CREAD      /* RX ein               */
                                  |CLOCAL);   /* kein Handshake       */
    l1pp->wrk_termios.c_cflag &= ~(CSTOPB     /* 1 Stop-Bit           */
                                   |PARENB    /* ohne Paritaet        */
                                   |HUPCL);   /* kein Handshake       */

    /* pty verwenden ? */
    if (l1pp->speed != B0)                    /* B0 -> pty soll ver-  */
    {                                         /* wendet werden        */
      /* Empfangsparameter setzen */
      if (cfsetispeed(&(l1pp->wrk_termios), l1pp->speed) == -1)
      {
        uFehler = 4;
        printf("Error: can't set input baudrate settings on device %s\n", l1pp->device);
        printf("       (%s)\n", strerror(errno));
        LOGL1("Error: can't set input baudrate settings on device %s", l1pp->device);
        LOGL1("       (%s)", strerror(errno));
      }

      /* Empfangsparameter setzen */
      if (cfsetospeed(&(l1pp->wrk_termios), l1pp->speed) == -1)
      {
        uFehler = 4;
        printf("Error: can't set output baudrate settings on device %s\n", l1pp->device);
        printf("       (%s)\n", strerror(errno));
        LOGL1("Error: can't set output baudrate settings on device %s", l1pp->device);
        LOGL1("       (%s)", strerror(errno));
      }

      if (l1pp->speed == B38400)              /* wenn >= 38400 Bd     */
      {
        ser_io.flags &= ~ASYNC_SPD_MASK;      /* Speed-Flag -> 0      */
        ser_io.flags |= l1pp->speedflag;      /* Speed-Flag setzen    */

        if (ioctl(iDescriptor, TIOCSSERIAL, &ser_io) < 0)
        {
          uFehler = 4;
          printf("Error: can't set device settings on device %s\n", l1pp->device);
          printf("       (%s)\n", strerror(errno));
          LOGL1("Error: can't set device settings on device %s", l1pp->device);
          LOGL1("       (%s)", strerror(errno));
        }
      }
    }
  }

  /* Serielle Schnittstelle auf neue Parameter einstellen */
  if (uFehler == 0)
  {
    tcsetattr(iDescriptor, TCSADRAIN, &(l1pp->wrk_termios));
    l1pp->port_active = TRUE;

    /* Ring ist nun offen, aber noch nicht initialisiert */
    uRingStatus = RING_IDLE;

    LOGL1("-RING-: Port erfolgreich geoeffnet");

    return;
  }

  /* Fehlerbehandlung */
  /* Port war schon offen, alte Einstellungen wiederherstellen */
  if (uFehler > 3)
    tcsetattr(iDescriptor, TCSADRAIN, &(l1pp->org_termios));

  /* Port war schon offen, aber noch nicht veraendert, nur schliessen */
  if (uFehler > 2)
  {
    close(iDescriptor);
    iDescriptor = -1;
  }

  /* Port war nicht offen, aber es existiert ein Lockfile, schliessen */
  if (uFehler > 1)
  {
    if (l1pp->lock != -1)
    {
      close(l1pp->lock);
      l1pp->lock = -1;
    }
  }

  /* Lockfile loeschen */
  if ((*(l1pp->tnn_lockfile) != '\0') && (l1pp->lock != -1))
    unlink(l1port[i].tnn_lockfile);

  LOGL1("-RING-: Port %s konnte nicht geoeffnet werden !", l1pp->device);
}

/************************************************************************/
/* Device schliessen                                                    */
/************************************************************************/
void Sixpack_l1exit(void)
{
  register UWORD i;

  /* Erstes 6PACK-Device suchen */
  /* Alle L1-Ports durchgehen */
  for (i = 0; i < L1PNUM; ++i)
  {
    if (l1port[i].kisstype == KISS_6PACK)
      break;
  }

  /* Keinen 6PACK-Port gefunden ? */
  if (i == L1PNUM)
    return;

  /* Ring wird angehalten */
  uRingStatus = RING_HALT;

  /* Serieller Port offen ? Dann schliessen */
  if (iDescriptor != -1)
  {
    /* Alte Porteinstellungen wiederherstellen */
    tcsetattr(iDescriptor, TCSADRAIN, &(l1port[i].org_termios));
    /* Port schliessen */
    close(iDescriptor);
    iDescriptor = -1;
  }
  else
    return;

  /* Lockfile gesetzt ? */
  if (*(l1port[i].tnn_lockfile) != '\0')
  {
    /* Lockfile offen ? Dann schliessen und loeschen */
    if (l1port[i].lock != -1)
    {
      close(l1port[i].lock);
      l1port[i].lock = -1;
      unlink(l1port[i].tnn_lockfile);
    }
  }

  /* Port nicht mehr aktiv */
  l1port[i].port_active = FALSE;

  LOGL1("-RING-: Port geschlossen");
}

/************************************************************************/
/* Anzeige der TNC-Statistiken                                          */
/************************************************************************/
void ccp6pack(void)
{
  MBHEAD *mbp;
  register UBYTE uTNC;
  register UBYTE i;
  register UBYTE uNumPorts = 0;

  ULONG uUpDays = (tic10 - tLastInit) / 100; /* eigentlich sinds hier noch Sekunden ... */
  ULONG uUpHours, uUpMinutes, uUpSeconds;

#define BUFLEN 32
  char cBuf[BUFLEN + 1];

  /* Alle L2-Ports durchgehen */
  for (i = 0; i < L2PNUM; ++i)
    if (kissmode(i) == KISS_6PACK)
      ++uNumPorts;

  /* ueberhaupt ein Port da ? */
  if (uNumPorts == 0)
  {
    mbp = putals("No 6PACK-Ports configured.");
    /* und ab die Post ... */
    putprintf(mbp, "\r");
    prompt(mbp);
    seteom(mbp);
    return;
  }

  /* Sysop will aendern und noch was in der Zeile da ? */
  if (issyso() && skipsp(&clicnt, &clipoi))
  {
    /* Frischer Buffer */
    memset(cBuf, 0, sizeof(cBuf));

    /* Operation lesen (add, delete) */
    for (i = 0; i < BUFLEN; ++i)
    {
      if ((!clicnt) || (*clipoi == ' '))
        break;
      clicnt--;
      cBuf[i] = toupper(*clipoi++);
    }

    /* Hinzufuegen (add oder +) */
    if (   (strcmp(&cBuf[0], "LOGLEVEL") == 0)
        || (strcmp(&cBuf[0], "LOG") == 0)
       )
    {
      WORD uNewLevel;

      /* Noch was da in der Kommandozeile ? */
      if (!skipsp(&clicnt, &clipoi))
      {
        mbp = getmbp();
        putprintf(mbp, "6PACK's actual loglevel is %u\r", uLogLevel);
        /* und ab die Post ...                                            */
        prompt(mbp);
        seteom(mbp);
        return;
      }

      /* neues Loglevel lesen */
      uNewLevel = nxtnum(&clicnt, &clipoi);

      if ((uNewLevel < 0) || (uNewLevel > 4))
      {
        putmsg("error: loglevel out of range (0-4) !!!\r");
        return;
      }

      /* Wert uebernehmen */
      SixpackLog("-RING-: Loglevel %u -> %u", uLogLevel, uNewLevel);
      uLogLevel = (UWORD)uNewLevel;

      /* und ab die Post ... */
      mbp = getmbp();
      prompt(mbp);
      seteom(mbp);
      return;
    }
  }

  mbp = putals("6PACK Error-Statistics:\r");

  putprintf(mbp, "               TX-     RX-   RX-Buffer-\r");
  putprintf(mbp, "-TNC#-Port#-Underrun-Overrun--Overflow--Checksum-Resets-Cal-RXC-TXC-LHeard[ms]\r");

  /* alle TNC durchgehen */
  for (uTNC = 0; uTNC < MAX_TNC; ++uTNC)
  {
    /* nicht vorhandene TNC */
    if (RING[uTNC].bPresent == FALSE)
    {
      putprintf(mbp, "   %u   N/A      -       -        -         -       -     -   -   -      -\r", uTNC);
      continue;
    }

    /* vorhandene TNC */
    putprintf(mbp, "   %u   %2u   %5u   %5u    %5u     %5u   %5u     %1u  %2u  %2u   %5u\r",
              uTNC,                                     /* TNC */
              uMap_TNCtoPort[uTNC],                     /* Port */
              RING[uTNC].uTXU,                          /* TX-Underrun */
              RING[uTNC].uRXO,                          /* RX-Overrun */
              RING[uTNC].uRXB,                          /* RX-Buffer Overflow */
              RING[uTNC].uChecksumErr,                  /* Anz. Checksummenfehler */
              RING[uTNC].uReset,                        /* Anz. Resets */
              RING[uTNC].cTestCount,                    /* Anz. Calibrates */
              RING[uTNC].cRXC,                          /* RX-Zaehler */
              RING[uTNC].cTXC,                          /* TX-Zaehler */
              ((tic10 - RING[uTNC].tLastHeard) * 10));  /* LastHeard */
  } /* for (...) */

  putprintf(mbp, "\rRing is ");

  switch (uRingStatus)
  {
    case RING_HALT  : putprintf(mbp, "halted due to unrecoverable errors ! Please restart.\r");
                      break;

    case RING_IDLE  : putprintf(mbp, "waiting for TNC-initialization.\r");
                      break;

    case RING_INIT  : putprintf(mbp, "awaiting the return of the probe packet.\r");
                      putprintf(mbp, "(%u probes sent so far, last probe sent %u ms ago)\r", uNumProbes, ((tic10 - tLastInit) * 10) );
                      break;

    case RING_RUN   : putprintf(mbp, "up and running for");

                      uUpSeconds = uUpDays % 60L;
                      uUpDays /= 60L;
                      uUpMinutes = uUpDays % 60L;
                      uUpDays /= 60L;
                      uUpHours = uUpDays % 24L;
                      uUpDays /= 24L;

                      if (uUpDays < 1L)
                        putprintf(mbp, " %2lu:%02lu:%02lu\r", uUpHours, uUpMinutes, uUpSeconds); /* hh:mm:ss */
                      else
                      {
                        if (uUpDays < 99L)
                          putprintf(mbp, " %2lu/%02lu:%02lu\r", uUpDays, uUpHours, uUpMinutes);  /* dd/hh:mm */
                        else
                          putprintf(mbp, " %5lu/%02lu\r", uUpDays, uUpHours);                    /* ddddd/hh */
                      }
                      break;

    default: putprintf(mbp, "an undefined state !!! You really shouldn't see this ... Please restart.\r"); break;
  }

  /* und ab die Post ... */
  putprintf(mbp, "\r");
  prompt(mbp);
  seteom(mbp);
}

/************************************************************************/
/* Log-Funktion zum Debugging                                           */
/************************************************************************/
void SixpackLog(const char *format, ...)
{
  FILE *fp;
  va_list arg_ptr;
  struct timeval tv;
  static char str[30];
  char *ptr;

  fp = fopen("6pack.log", "a");

  if (fp != NULL)
  {
    gettimeofday(&tv, NULL);
    ptr = ctime(&tv.tv_sec);
    strcpy(str, &ptr[11]);
    snprintf(str + 8, sizeof(str) - 8, ".%06ld", tv.tv_usec);
    fprintf(fp, "%s:", str);

    va_start(arg_ptr, format);
    vfprintf(fp, format, arg_ptr);
    va_end(arg_ptr);

    fprintf(fp, "\n");
    fclose(fp);
  }
  else
  {
    /* Fehler beim Schreiben in die Logdatei, dann das Logging beenden */
    uLogLevel = 0;
  }
}

/**************************************************************************/

#endif

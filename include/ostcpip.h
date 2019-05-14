#ifdef OS_STACK

#define OS_MODE 0                      /* 1 -> Interner Stack / 0 -> OS-Stack */


extern int     SetupOS(T_INTERFACE *,              /* Aktive Socket's setzen. */
                       unsigned short);

extern int     RecvOS(UWORD, fd_set);  /* Eingehende TCPIP-Packet bearbeiten. */

extern BOOLEAN L1InitOS(UWORD, int, unsigned short);

extern void    L1ExitOS(UWORD);

extern int     PutflushSockOS(void);      /* Frame aus der Sendeliste senden. */

extern BOOLEAN CloseSockOS(BOOLEAN,            /* Schliesse aktuellen Socket. */
                           WORD);

extern BOOLEAN CheckSocketOS(void);       /* Pruefe, ob der Socket aktiv ist. */

extern void    ListenTCP_OS(void);

extern int     AddUserOS(T_INTERFACE *,               /* TCP-Interface.       */
               unsigned,                              /* Neuer Socket.        */
               char *);                               /* IP-Adresse.          */

#endif /* OS_STACK */

/* End of include/ostcpip.h */

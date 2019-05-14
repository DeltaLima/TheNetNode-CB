
typedef struct attprt           /* Attach-Port-Struktur                 */
{
  const char *attstr;           /* Device-Name                          */
  int         attnum;           /* Device-Nummer                        */
} ATTPRT;

typedef struct atttyp           /* Attach-KissType-Struktur             */
{
  const char *attstr;           /* KissType                             */
  int         attnum;           /* Nummer                               */
} ATTTYP;


unsigned short my_udp;
BOOLEAN        tokenflag;

#ifdef VANESSA
extern BOOLEAN van_test(int);
#endif

#ifdef SIXPACK
int iDescriptor;
#endif /* SIXPACK */

void           ccpattach(void);
void           ccpdetach(void);
void           dump_attach(MBHEAD *mbp);
void           tf_set_kiss(DEVICE *);

/* End of include/l1attach.h */

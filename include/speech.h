#define MAXZEILEN 351

#define speech_mode_off 0x0
#define speech_mode_lX  0x1
#define speech_mode_d   0x20
#define speech_mode_ld  0x300
#define speech_mode_f   0x4000
#define speech_mode_s   0x50000
#define speech_mode_u   0x600000
#define speech_mode_lu  0x7000000


#define speech_mode_lX_num  7
#define speech_mode_d_num   6
#define speech_mode_ld_num  5
#define speech_mode_f_num   4
#define speech_mode_s_num   3
#define speech_mode_u_num   2
#define speech_mode_lu_num  1

extern char speech[];
extern char speechpath[];

extern void  speech_init(void);
extern void  ccpspeech(void);
extern void  dump_speech(MBHEAD *);
extern int   speech_load(char *bufspeech);
extern char *speech_message(int msgnum);


/*
	edx/emx (EDitor for X), (C) 2002, Terry Loveall
	released into the public domain.
	Based upon the original work ee.c of Yijun Ding, copyright 1991 which is
	in the public domain.
	This program comes with no warranties or binaries. Use at your own risk.
*/

/* This is the core editor engine */

#define MAXVLINE 450
#define EOL 	'\0'	/* end of line marker */
#define BLNK	' ' 	/* blank */
#define LF		'\n'	/* new line */
#define NLEN	256 	/* input buffer length */
#define LMAX	10000	/* max line length */
#define XINC	20	/* increment for x offset */

#define CHG 0	/* file Modified: =0 file not changed, !=0 file changed */
#define FIL 1	/* Fill */
#define OVR 2	/* character insert=0, Overwrite=1 */
#define CAS 3	/* Case sensitive: =0 no, !=0 yes */
#define TAB 4	/* Tab expand */
#define BLK 5	/* block mark active */

#define IND 6	/* auto indent flag */
#define REC 7	/* recording macro flag */
#define ALL 8	/* replace all flag */
#define EDT 9	/* edit flag */
#define SHW 10	/* update & show entire screen */
#define NEW 11	/* editing a new file */
#define WIN 12	/* window: <0 same win, =0 load new */

/* buffer sizes */

#define AMAX	0x4000 	/* main buffer initial size */
#define BMAX	0x1000 	/* block initial size */
#define UMAX	0x2000	/* undo level */
#define YTOP	0	/* first line */

#define VOID_LINK (unsigned int)-1

#ifdef GREEK
/* Greek letters */
static char *greek_letter[26] = {
"alpha", "!beta", "Chi", "Delta", "!epsilon", "!Phi",
"Gamma", "eta", "iota", "Psi", "kappa", "Lambda", "mu", "nu",
"omega", "Pi", "!Theta", "!rho", "!Sigma", "tau", "Upsilon", NULL,
"Omega", "Xi", "Psi", "zeta"
};
int var_greek = 0;                         /* variant of Greek letters */
#endif

/* character handler types */
static enum {
		MAIN,
		DIALOG,
		OPTIONS
} executive = MAIN; 			/* default character handler */

/* current window struct */
typedef struct {
	char name[NLEN];
	int jump;
} MWIN; 	/* my window structure */

MWIN  ewin;  /* current window */

/* undo record struct */
typedef struct {
	unsigned int link;
	int pos;
	int length;
	char str[0];
} U_REC;

int x=0, screen_width;		/* screen position 1 <= x <= screen_width */
int y=1, screen_height; 	/* screen size 0 <= y <= screen_height */
int eolx;					/* coord of eol */
int update_scr = 1; 		/* do screen updates flag */
int search_mode = 0;		/* was it a replace request ? */
unsigned int amax, bmax, umax;	/* main buffer, block size, undo level */

char *cfdpath;
char Command[256];

char *bstart, *bend; 		/* marked block start, end and char pointer */
#ifndef MINIMAL
char *binding[26];
#endif

char  bbuf[NLEN]; 			  /* temp file name */
char  sbuf[NLEN], rbuf[NLEN]; /* search buffer, replace buffer */
char  *s;					  /* pointer to search string */
int   rlen, slen;			  /* replace and search lengths */

char  *edbuf, *file_end; /* edit buffer, end of file */
char  *bb, *mk, *last_mk = NULL;		  /* block buffer, mark */
unsigned blen;				  /* length of text in block buffer */
char *cur_pos;
char *line_start;				/* current pos, start of current line */
char *screen_start;				/* global screen start */
unsigned int xlo;				/* x offset for left display edge */
char *last_pos;					/* last last position visited */
char *old_pos;					/* last position visited */

int x_offset;					/* offset of xtru from line_start */
int xtru = 0, ytru = 0; 				/* file position */
int ytot = 0;						/* 0 <= ytru <= ytot */

int yl1, yl2; 				  /* 1st, 2nd line of window */
int tabsize=4;				  /* tab size */
int doCtrlC = 0;				/* decode next char from ^C function */
int doCtrlK = 0;				/* decode next char from ^K function */
int doCtrlQ = 0;				/* decode next char from ^Q function */
int doCtrlX = 0;				/* decode next char from ^X function */
int doEscap = 0;				/* decode next char from Esc function */
int help_done = 0;
int literal = 0;

int col;						/* width of dialog preset (if any) */
int diastart;					/* start of dialog input field */
int first;						/* first dialog char flag, 0=true */
int dblen;						/* current dialog buffer size */
char *diabuf;					/* dialog buffer pointer */
void (*dialogCB) ();			/* callback pointer */

static char lbuf[16];			/* goto line number buffer */
static char cbuf[16];			/* goto column number buffer */
static char wbuf[8]="78";		/* reformat block right margin */
static char twbuf[8]="4";		/* tab width buffer */

static char mbuf[256];			/* macro record buffer */
static char *pmbuf=mbuf;		/* record pointer for macro buffer */
struct stat ofstat;				/* edit file stat structure */

FILE  *fi, *fo, *fb;			/* file handles for input, output, block */
static char fbuf[256];			/* current file name buffer */

char	flag[WIN+1]="\0\0\0C\0\0N\0\0E\0";	/* options flag presets */
char	fsym[]="MFOCTBNRAE? ";	/* Modified,Fill,Overwrite,Case,Tab,Block marked,autoiNdent,Rec,All */

/* word delimiter chars */
char wdelims[] = "\t ,;+-*=^&|?:`()[]{}<>\"\'";
char sdelims[] = "\t >]})";
char s1delims[] = "\t ";

int undo_wrap = 0;		/* undo buffer wrap flag */
void * unbuf;			/* undo stack */
U_REC* undop = NULL;		/* active undo/redo pointer */
U_REC* undosp = NULL;		/* top of undo stack pointer */
int in_undo;			/* dont reset undop to undosp if true */
int undone=1; 			/* have undone all */

/* function prototypes */

int get_tru(char* pos);
char* get_cur();
char * realloc_buffer();
void init_undo();
U_REC* new_undo(char *pos, int len);
void u_del(char *pos, int len);
void u_ins(char *pos, int len);
void ytot_ins();
void undo();
void redo();
void sys_exit(int code);
void move_to(int newx, int newy);
void getx();
void cursor_up(), cursor_down(), cursor_left(), cursor_right();
void cursor_pageup();
void cursor_pagedown();
void word_left();
void word_right();
void word_mark();
void drstr(char* disp, int i);
void show_rest(int len, char *s);
void show_scr(int fr, int to);
void scroll_up();
void scroll_down();
void scr_update();
void show_sdn(int line);
void show_flag(int x, int g);
void dialog(int key);
void show_note(char *prp);
int  show_gets(char *prp, char *buf, int blen, void *cb);
void options(int key);
void show_pos();
void show_top(), show_help(), show_mode();
void top();
char *file_new();
void file_read();
char *file_ltab(char *s);
int  file_write(FILE *fp, char *s, char *e);
int  file_fout();
void do_save();
void set_title();
int SYSTEM (char *cmd);
void gorun();
void new_edit(char *nfile);
void do_cd();
void chgdir();
void do_open();
#ifndef MINIMAL
void launch_edit();
void req_edit();
#endif
void file_save(int f_all, int f_win);
void file_resize(char *s, char *d);
void goto_x(int xx), goto_y(int yy);
void goto_ptr(char *s);
void goline();
void goto_last();
void goto_line(), goto_col();
void gocol();
void goto_col();
void switch_marks();
void find_match();
int  str_cmp(char *s);
void gofindforward();
void gofindbackward();
char *goto_find(char *s, int  back);
void get_mk(char* dbuf);
char *goto_search(int	back);
void do_sar();
void ask_replace();
void goto_replace();
void gettab();
void tab_size();
void getmarg();
void window_size();
void mark_off();
void block_put(), block_get(), block_mark();
void remove_block();
void block_remove_update();
void chg_case(int upper);
void transpose();
void block_copy(int delete);
void block_paste(), block_write(), block_line();
void doblockr();
void doblockw();
void block_read();
int  block_fill();
void block_format();
void key_return(), key_deleol(char *s), key_delete();
void key_backspace(), key_delword(int eol);
void key_tab(int tabi);
void key_normal(int key);
void key_macros(int record);
#ifdef GREEK
void key_greek(int key);
#endif
void ctrlc_key(int key);
void ctrlk_key(int key);
void ctrlq_key(int key);
void key_alt(int key);
void key_control(int key);
void key_escape(int key);
void key_func(int key);
void key_binding(int key);
void main_exec(int key);


/* mye.c, from ee.c, Yijun Ding, Copyright 1991
modified April 2002. Copyright (C) 1998,2002 Terry Loveall, <loveall@qwest.net>

Code starts after the Users Manual.

Compile under Linux with termcap lib support
        gcc mye.c -O2 -ltermcap -o mye ; strip mye

Compile under Linux for gdb with termcap lib support
        gcc mye.c. -O2 -g -ltermcap -o mye

Compile with MSVC:      cl -DMSVC -Ox mye.c

------------------------------------------------------------------------------
Users Manual:

Based upon the original work ee.c of Yijun Ding, copyright 1991. His logic
simplicity was elegant. Any problems are mine.

The original work is public domain with source and so is mye.c. I would
appreciate hearing about any bug fixes or improvements.

This is a simple, basic text editor. It requires only three graphic support
routines from the OS: goto X-Y, set character attribute and clear screen. For
input it uses the old WordStar style control key sequences; i.e. ^Kh or ^K^H 
are all equivalant. It will compile on NT with MSVC and lcc and on Linux with 
gcc.

On Linux, it uses termios rather than curses, so will run with minimum library
support (like on a rescue/boot disk). Function keys under Linux probably won't
work depending upon execution context. They might work on the text console and
almost certainly won't on any kind of xterm.

For input, mye uses a version of the old WordStar style control key sequences;
i.e. ^k^h or ^kh, always lower case control chars. ^kc copies marked block to
clipboard, right mouse pastes from clipboard. ^ky deletes marked block to
block buffer, ^kv copies from from block buffer to cursor position.

Basic navigation is on the left of the keyboard:

  Q W E R      ^E    ^W  ^R
   A S D F   ^S  ^D             ^A   ^F
    Z X C      ^X    ^Z  ^C

    ^E - up
    ^X - down
    ^D - right
    ^S - left

    ^W - scroll down 1 line
    ^Z - scroll up 1 line

    ^R - page up
    ^C - page down

    ^F - word right
    ^A - word left

    ^Q^D - goto end of line
    ^Q^S - goto start of line

    ^Q^R - goto start of file
    ^Q^C - goto end of file

    ^Q^F - find
    ^Q^A - find and replace

Once you learn the 'magic diamond' of EXDS with ^Q extensions, function keys
and mouse become irrelevant. Following are the rest of the key commands:

    ^B - word wrap text until the next double newline
    ^G - delete character under cursor
    ^H - delete character left of cursor
    ^I - insert tab char
    ^J - newline
    ^K - prefix for file/block operations
    ^L - repeat last find
    ^M - insert newline
    ^N - insert newline
    ^O - prefix for display and change mode flags
    ^P - inline literal
    ^T - delete from cursor to start of next string
    ^U - undo
    ^V - toggle insert/overwrite mode
    ^Y - delete cursor line

    ^QI - goto line
    ^QL - refresh screen
    ^QM - get new right margin
    ^QY - delete from cursor to end of line

    ^KB - toggle mark block
    ^KC - copy marked block to selection
    ^KD - exit if file not modified
    ^KF - open new file
    ^KH - help
    ^KK - toggle mark block
    ^KM - record macro
    ^KN - get and change directory
    ^KP - play macro
    ^KQ - exit if file not modified
    ^KR - read a file and paste into cursor position
    ^KS - save and continue
    ^KT - get new tab size
    ^KU - redo
    ^KV - paste block buffer into cursor position
    ^KW - write marked block to file
    ^KX - exit if file not modified
    ^KY - cut marked block to block buffer

Under NT the following function keys are operational:

   (key)     (description)                                   (same as)
----------------------------------------------------------------------
    F2      file save (if modified)                             ^KS
    F3      open new file (prompts to save if file modified)    ^KF
    F7      toggle mark block                                   ^KB
    F8      toggle mark block                                   ^KK
    Insert  toggle insert/overwrite                             ^V
    Del     delete character under cursor or marked block       ^G
    Home    move cursor to beginning of line                    ^QS
    End     move cursor to end of line                          ^QD
    PgUp    move up one screen                                  ^R
    PgDn    move down one screen                                ^C

Modes:

Changing modes of operation is performed by ^o followed by one of the
displayed upper/lower case characters MFOCTBNRA. This will toggle the specific
flag. Modes are indicated as being on by displaying their upper case
character. The file modified M flag can be toggled off explicitly. The block
mark B active flag indicates a complex state. Toggle it off with the block
mark key sequences, NOT with ^oB.

M : file modified               set by anything that modifies file.
F : word wrap at text entry     toggle with ^oF
O : overwrite                   toggle with ^oO, ^V or function key Insert
C : search is case sensitive    toggle with ^oC
T : expand/compress tabs        toggle with ^oT
B : block mark active           toggle ^KB, ^KK, F7 or F8 (don't with ^oB)
N : autoiNdent active           toggle ^on
R : macro record                toggle ^KM (don't with ^oB)
A : replace all occurences flag toggle with ^oA

The editor does display tab chars as multiple spaces. Tab (0x09) chars are
displayed as tabsize spaces. Default tab size is 4. To change tab-width to 8
the command line is  'mye -t8'. To change from within the editor use ^KT.

To go to a specified line on initial file opening, the command line is
'mye -+507 somefile'. The minus plus sequence is required. Input a ^J to go to
a line from within e.

Turning on (F)ill mode enables wordwrap during text entry. Block reformat
wraps the text at the right screen edge until a double newline is encounterd.
To reformat a paragraph, place the cursor at the desired point of reformat and
enter a ^b. To change the right margin use ^Q^M.

As noted, undo and redo are available. U for undo, ^KU for redo. A complete
record of the edit session is maintained. Undoing all actions in the undo
buffer will reset the Marked flag.

Find and 'Search and Replace' will pick up any marked blocks, text under
the cursor or user input in that order. Found text is highlighted. Set replace
ALL flag wth ^oA option before running SAR to replace all occurences. 

For general dialog entry if the first character entered is not ^H, ^C, ^L, or 
Enter, the dialog string is discarded. ^L moves the cursor to the end of the 
dialog string.

For a complete understanding of the operation of mye, study the code. It is the
ultimate authority on operation.

Remember, when all else fails READ THE SCREEN.
-----------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>

/* Target dependent include files ********************************* */

#ifdef MSVC

#include <windows.h>
#include <conio.h>
#include <ctype.h>
#include <io.h>
#include <direct.h>

#else /* not MSVCd */

#include <termios.h>    /* use termios.h for termcap output */
#include <unistd.h>

#endif /* MSVC */

#include "version.h"
#include "messages.def"
#include "edit.h"

int funckey = 0;	/* function key flag */
int exitf = 1;		/* exitf = 0 = exit */
#ifdef NOEDIT
int noedit = 0;
#endif

/* code */

/* NT I/O specific code ********************************* */

#ifdef MSVC

/* MSVC specific variables */
COORD outxy ;                   /* output COORD for SetConsoleCursorPosition */
CONSOLE_SCREEN_BUFFER_INFO csbi;/* to get MS screen buffer info */
HANDLE stdIn,stdOut,stdErr;     /* handles to output window/buffer */
BOOL bSuccess;                  /* OS result flag */

/* MSVC specific protos */
void clreol();
void clrscr();
void gotoxy(int horz,int vert);

/* MSVC specific macros */

#define cprintf(x,y)    _cprintf(x,y)

#define ttopen()     stdIn=GetStdHandle(STD_INPUT_HANDLE);\
    stdOut=GetStdHandle(STD_OUTPUT_HANDLE);\
    stdErr=GetStdHandle(STD_ERROR_HANDLE);\
    bSuccess = GetConsoleScreenBufferInfo(stdOut, &csbi);\
    if (!(bSuccess)) return(0);\
    screen_height=((csbi.srWindow.Bottom)-(csbi.srWindow.Top))-2;\
    screen_width=((csbi.srWindow.Right)-(csbi.srWindow.Left))+1

#define highvideo()   SetConsoleTextAttribute(stdOut,FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | BACKGROUND_GREEN)
#define lowvideo()    SetConsoleTextAttribute(stdOut,0x0e)
#define ttclose()     SetConsoleTextAttribute(stdOut,0x0e)

void gotoxy(int horz,int vert)
{
    outxy.X=horz;
    outxy.Y=vert;
    SetConsoleCursorPosition(stdOut,outxy);
}

/* Microsoft get a key */
int get_key()
{

    static  char k1[]="<=ABDHPKMRSGOIQ";    /* input func key second byte */
    static  char k2[]="rfbczEXSDngsdvw";    /* F2,F3,F7,F8,F10,up,dn,le,ri,Ins,Del,home,End,PgUp,PgDn*/
    int key1,key2;
    char *s;

    if(((key1=getch()) == 0xE0) || ((key1 == 0))) {
        key2=getch();
#ifdef DEBUG
    {   /* display up to 4 chars of func key input */
        char tbuf[80];
        show_top();
        sprintf(tbuf, "Line %d/%d, Col %d, char %2x_%2x-%2x+%2x",
            ytru+1, ytot, xtru, -1, -1, key1, key2);
        show_note(tbuf);
    }
#endif /* DEBUG */
        if((s=strchr(k1, key2)) == 0)
            return HLP;
        if((key1 = k2[s-k1]) > 'a')
            funckey=1;
        return key1&0x1F;
    }
    return key1;
}

void clreol()   /* clear from cursor to end of line */
{
  int i,ox=outxy.X,oy=outxy.Y;

  for(i=outxy.X; i<screen_width; ++i) putch(' ');
  gotoxy(ox,oy);
}

/* clrscr(): clear the screen by filling it with blanks, home cursor*/

void clrscr()
{
    COORD coordScreen = { 0, 0 }; /* here's where we'll home the cursor */
    BOOL bSuccess;
    DWORD cCharsWritten;
    CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
    DWORD dwConSize; /* number of character cells in the current buffer */

    /* get the number of character cells in the current buffer */
    bSuccess = GetConsoleScreenBufferInfo(stdOut, &csbi);
    if (!(bSuccess)) return;    /* error return on any failure */

    dwConSize = csbi.dwSize.X * csbi.dwSize.Y; /* figure screen size */

    /* fill the screen with blanks from home cursor position */
    bSuccess = FillConsoleOutputCharacter(stdOut, (TCHAR) ' ',
        dwConSize, coordScreen, &cCharsWritten);
    if (!(bSuccess)) return;

    /* get the current text attribute */
    bSuccess = GetConsoleScreenBufferInfo(stdOut, &csbi);
    if (!(bSuccess)) return;

    /* now set the buffer's attributes accordingly */
    bSuccess = FillConsoleOutputAttribute(stdOut, csbi.wAttributes,
        dwConSize, coordScreen, &cCharsWritten);
    return;
}

#else /* not MSVC */

/******************** Start of termcap I/O *******************/
/*  uses termios.h for termcap I/O on quick and dirty Editor */

/* portable interface defines. */

#define getstate(p)     ((void) tcgetattr(0, (p)))
#define w_setstate(p)   ((void) tcsetattr(0, TCSADRAIN, (p)))

/*
#define oflush()        (void) fflush(stdout)
#define cprintf(x,y)    fprintf(stdout,x,y) 
*/

#define _cputs(s)       fputs (s, stdout)
#define putch(c)        putchar(c)
#define getch()         getchar()
#define ttclose()       sys_endv()
#define clrscr()        erase()

#define highvideo()     tputs(SO, 1, foutch)
#define lowvideo()      tputs(SE, 1, foutch)

/* Termcap-related declarations. */
extern	int	tgetent();
extern	int	tgetnum();
extern	int	tputs();
extern  char    *tgetstr();
extern  char    *tgoto();

typedef struct termios  Termstate;

static Termstate    cooked_state, raw_state;

/* Local termios func declarations. */
void foutch(int c);
void cputs(char *str);
static void fail(char *str);
void ttopen();
void ttyopen();
void sys_endv();
void erase();
void gotoxy(int horz,int vert);
void clreol();

/* termios data declarations. */
unsigned int    COLS = 0;       /* screen dimensions; 0 at start */
unsigned int    LINES = 0;

/* termcap string, num and boolean defs. */
static  char    *CL;            /* erase line/display */
static  char    *CM;            /* cursor motion string */
static  char    *SO, *SE;       /* standout mode start/end */
//static  char    *colours[10];   /* colour caps c0 .. c9 */
//static  int     ncolours;       /* number of colour caps we have */

/* use MS style COORD to track cursor pos */
typedef struct {
    int X;      /* x coordinate */
    int Y;      /* y coordinate */
} COORD;        /* x-y structure */

COORD outxy;    /* cursor coordinates for screen positioning */

/* qde state info */
static enum {
    m_SYS = 0,
    m_QDE
}   termmode;

/* tputs char output callback func */
void foutch(int c)
{
    putchar(c);
}

void cputs(char *prntstr)
{
	int strl = strlen(prntstr);
        _cputs(prntstr);
	outxy.X += strl;
}

/* fail() is called by ttyopen() on terminal open failure */

static void fail(char *str)
{
    /* Assume we are in raw mode already, so go back into cooked. */

    (void) fputs(str, stderr);  /* tell operator why failed */
    putc('\n', stderr);
    sys_endv();

    exit(2);                    /* tell OS we failed */
}

/* Initialise the terminal so that we can do single-character I/O */

void ttopen()
{
    /* Set up tty flags in raw and cooked structures.
     * Do this before any termcap-initialisation stuff
     * so that start sequences can be sent.*/

    getstate(&cooked_state);
    raw_state = cooked_state;

    cfmakeraw(&raw_state);

    /* Now set up the terminal interface. */
    ttyopen();
    clrscr();

    /* Go into raw/cbreak mode, and say so. */
    w_setstate(&raw_state);

    termmode = m_QDE;
}

/* Look up term entry in termcap database, and set up all the strings. */

void ttyopen()
{
    char    tcbuf[4096];        /* buffer for termcap entry */
    char    *termtype;          /* terminal type */
    static  char strings[512];  /* space for storing strings */
    char    *strp = strings;    /* ptr to space left in strings */
    int iv;

    termtype = getenv("TERM");
    if (termtype == NULL) fail("Can't find TERM in environment.");

    switch (tgetent(tcbuf, termtype)) {
    case -1: fail("Can't open termcap.");    /*NOTREACHED*/
    case 0:  fail("Can't find entry for your terminal in termcap.");
    }

    /* Screen dimensions. */
    iv = tgetnum("co");
    /* check that x is there */
    if (iv <= 0) fail("`co' entry in termcap is invalid or missing.");

    COLS = (unsigned) iv;

    iv = tgetnum("li");
    /* check that y is there */
    if (iv <= 0) fail("`li' entry in termcap is invalid or missing.");

    LINES = (unsigned) iv;

    /* set output screen height and width */
    screen_height=LINES;
    screen_width=COLS;

    /* output control */
    CL = tgetstr("cl", &strp);  /* clear erase */
    CM = tgetstr("cm", &strp);  /* move goto */
    SO = tgetstr("so", &strp);  /* standout mode start */
    SE = tgetstr("se", &strp);  /* standout mode end */

    if (CM == NULL) {
        fail("qde can't work without cursor motion.");
    }

}

/* ReSet terminal into the mode it was in when we started. */

void sys_endv()
{
    if (termmode == m_SYS) return;

    (void) fflush(stdout);

    /* Restore terminal modes. */
	w_setstate(&cooked_state);
    termmode = m_SYS;
}

/* Erase display (may optionally home cursor). */

void erase()
{
    tputs(CL, (int) LINES, &foutch);
}

/* Goto the specified location. */

void gotoxy(int horz,int vert)
{
    outxy.X=horz;
    outxy.Y=vert;

    tputs(tgoto(CM, horz, vert), (int) LINES, &foutch);
}

/* Erase from cursor to end of line. */

void clreol()
{
  int i,ox=outxy.X,oy=outxy.Y;

  for(i=outxy.X; i<screen_width; ++i) putch(' ');
  gotoxy(ox,oy);
}

/* get a key */

int get_key()
{
#ifdef EMACS
    static  char k1[]="ABCDFH256"; /* up,dn,rt,le,ins,pgup,pgdn */
    static  char k2[]="PNFBEAnvw";
    static  char k3[]=" ._%<>3bfgjlmnpquvwx";
#else
    static  char k1[]="ABCD256"; /* up,dn,rt,le,ins,pgup,pgdn */
    static  char k2[]="EXDSnvw";
#endif
#define DEL_CHAR 8

    int key1=0,key2=0,key3=0,key4=0;
    char    *s;

    if((key1 = getch()) == -1) {    /* -1 is ERR */
        key1 = getch();
    }
#ifdef EMACS
    if (doCtrlQ)
	return key1;
#endif
    if (key1 == 9 || key1 == 17)
	return key1;
    if(key1 == 13)
    	return 10;

    if(key1 == 127)
	return DEL_CHAR; /* delete cursor char */

    if(key1 == 27) {
        key2 = getch();
#ifdef EMACS
	key2 = tolower(key2);
	if (strchr(k3, key2)) {
        	doEscap = 1;
		return key2;
	}
#endif
  	if (key2=='!') {
		doEscap = 2;
		show_note(ALT_KEY_BINDINGS);
		key3 = getch();
		return tolower(key3); 
	}
        key3 = getch();
        if ((key3 <= 0x40)||(key3 >= 0x5b))
		key4 = getch();
        else 
		key4 = 0x7e;
#ifdef SUPERDEBUG
    {
        char tbuf[80];
        show_top();
	/* sprintf(tbuf, "Line %d/%d, Col %d, char %2x_%2x-%2x+%2x", */
        sprintf(tbuf, "Line %d/%d, Col %d, S/char %d %d %d %d",
            ytru+1, ytot, xtru, key1, key2, key3, key4);
        show_note(tbuf);
	fflush(stdout);
	usleep(2000000);
    }
#endif /* SUPERDEBUG */
        if ((key4 != 0x7e)&(key3 != 0x5b)) { getch();}
	if (key3=='1' || key3=='2') {
	    if (key3=='1') key4 = key4-'0'-(key4>='6');
	    if (key3=='2') key4 = key4-'0'+9;
	    funckey=1;
	    return key4;
	}
	if (key3=='3') return DEL_CHAR;
        if((s=strchr(k1, key3)) != NULL && (key1 = k2[s-k1]) > 'a')
            funckey=1;
        key1=key1&0x1F;
        if (key1==0x1b) key1=0;
    }
#ifdef DEBUG
    {
        char tbuf[80];
        show_top();
	/* sprintf(tbuf, "Line %d/%d, Col %d, char %2x_%2x-%2x+%2x", */
        sprintf(tbuf, "Line %d/%d, Col %d, char %d %d %d %d",
            ytru+1, ytot, xtru, key1, key2, key3, key4);
        show_note(tbuf);
    }
#endif /* DEBUG */
    return key1;
}

/********************** End of termcap input ***********************/

#endif /* not MSVC */

void bell()
{
		putch(7);
}

/* End of I/O specific code****************************** */

#ifdef EMACS
#include "term_bind_em.c"
#endif
#ifdef WORDSTAR
#include "term_bind_ws.c"
#endif

char *RcFile = NULL;

void parse_arg(char *arg1, char *arg2)
{
	if(*arg1=='-') {
		if (!strcasecmp(arg1, "-f"))
			strcpy(ewin.name, arg2);
		else
		if (!strcasecmp(arg1, "-t"))
			tabsize = atoi(arg2);
		else
		if (!strcasecmp(arg1, "-j"))
			ewin.jump = atoi(arg2);
		else
		if (!strcasecmp(arg1, "-rc"))
			RcFile = strdup(arg2);
	}
#ifndef MINIMAL
	if(*arg1=='@') {
		char *ptr;
		int i;
		i = tolower(arg1[1])-'a';
		if (i>=0 && i<26) {
			ptr = arg2;
			while(isspace(*ptr)) ++ptr;
			if (binding[i]) free(binding[i]);
			binding[i] = strdup(ptr);
		}
	}
#endif /* MINIMAL */
}

#ifndef MINIMAL
void parse_rc(FILE *f)
{
	char buf[512], *ptr;
	int i, l;
	while ((ptr=fgets(buf, 510, f))) {
		while (isspace(*ptr)) ++ptr;
	        if (strlen(ptr)<2 || *ptr == '#') continue;
		i = 0;
		while(ptr[i] && !isspace(ptr[i])) ++i;
		if (!ptr[i]) continue;
		ptr[i++] = '\0';
		if (ptr[i]) {
			l = strlen(ptr+i)+i-1;
			if (l>=i && ptr[l]=='\n') ptr[l] = '\0';
			parse_arg(ptr, ptr+i);
		}
	}
}

void read_rc()
{
	FILE *fr = NULL;
	char name[NLEN], *ptr;

	if ((ptr=getenv("HOME"))) {
		if (RcFile) {
			if (ptr && *RcFile == '~' && RcFile[1] == '/')
				sprintf(name, "%s/%s", ptr, RcFile+2);
			else
				strcpy(name, RcFile);
		}
		else
			sprintf(name, "%s/.%s", ptr, DEFAULT_RC);
		fr = fopen(name, "r");
	}

	if (!fr) {
		sprintf(name, "%s/%s", SHAREDIR, DEFAULT_RC);
		fr = fopen(name, "r");
	}

	if (fr) {
		parse_rc(fr);
		fclose(fr);
	}
}
#endif /* MINIMAL */

void init(int argc, char *argv[])
{
	int i;

	/* Default settings */
	*ewin.name = '\0';
	amax = AMAX;
	bmax = BMAX;
	umax = UMAX;

#ifndef MINIMAL
	memset((void *)binding, 0, 26*sizeof(char *));

	for (i=1; i<argc; i++) {
		if (i<argc-1 && !strcasecmp(argv[i], "-rc"))
		RcFile = strdup(argv[++i]);
	}
	read_rc();
#endif /* MINIMAL */

	/* get command line options */
	for (i=1; i<argc; i++) {
		if ((i<argc-1) &&
			(argv[i][0]=='-'
#ifndef MINIMAL
|| argv[i][0]=='@'
#endif
			)) {
				parse_arg(argv[i], argv[i+1]);
				++i;
		} else {
			if (*ewin.name) break;
		        /* case of syntax  'filename:linenumber' */
			{
                            char* p = strchr(argv[i], ':');
                            if (p) {
                                 *p++ = '\0';
                                 ewin.jump = atoi(p);
                            }
			}
			strcpy(ewin.name, argv[i]);
		}
	}

	/* alloc memory for the main buffer and block buffer */
	edbuf = (char *) malloc((size_t)(amax+tabsize+1));
        bb = (char *) malloc((size_t)(bmax+tabsize+1));
	unbuf = (void *) malloc((size_t)(umax+tabsize+1));

	if (!edbuf || !bb || !unbuf) {
		fprintf(stderr,"Memory allocation failed, aborting !!\n");
		exit(1);
        }
}

int main(int argc,char *argv[])
{
	int skey;
/* edit engine init */
	ttopen();
	y1 = YTOP;
	x = 1; y=0;

	/* get command line options */
	init(argc, argv);

	/* set path */
	if (cfdpath == NULL) {
		cfdpath = (char *) malloc(BUFSIZ);
		getcwd(cfdpath, BUFSIZ);
#ifdef MSVC
		if (strcmp(cfdpath, "\\") != 0)
			strcat(cfdpath, "\\");
#else
		if (strcmp(cfdpath, "/") != 0)
			strcat(cfdpath, "/");
#endif /* MSVC */
	}

	do_open();
	scr_update();

/* end of edit engine init */
	/* main event loop, dispatch function calls in response to events */
	while(exitf)  {
		skey = get_key();
		switch(executive) {
			case MAIN:
				main_exec(skey);
				break;
			case DIALOG:
				dialog(skey);
				break;
			case OPTIONS:
				options(skey);
				break;
		}
	}
    return(0);
}

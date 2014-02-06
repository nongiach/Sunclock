
/*
	edx/emx (EDitor for X) (C) 2002, Terry Loveall
	released into the public domain.
	Based upon the original work ee.c of Yijun Ding, copyright 1991 which is
	in the public domain.
	This program comes with no warranties or binaries. Use at your own risk.
*/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <pwd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#define  XK_MISCELLANY
#include <X11/keysymdef.h>
#include <sys/time.h>

#ifdef MINIMAL
#define DEFAULT_HEIGHT 25
#else
#define DEFAULT_HEIGHT 32
#endif

#define DEFAULT_WIDTH 80

/* turns on display of keypressed event values in upper right corner */
/* #define DEBUG */

/*
#define FONTNAME  "-b&h-lucidatypewriter-medium-r-normal-sans-12-*-*-*-m-70-iso8859-1"
#define FONTNAME	"-b&h-lucidatypewriter-bold-r-normal-sans-12-*-*-*-m-70-iso8859-1"
#define FONTNAME "9x15bold"
*/
#ifdef TLL
#define FONTNAME	"-b&h-lucidatypewriter-bold-r-normal-sans-12-*-*-*-m-70-iso8859-1"
#else
#define FONTNAME "8x13"
#endif

#define X11

/* #define cprintf(s,v) { char str[350]; sprintf(str,s,v); cputs(str); } */

#/************************************************************/

/* use MS style COORD to track cursor pos */
typedef struct {
	int X;		/* x coordinate */
	int Y;		/* y coordinate */
} COORD;		/* x-y structure */

COORD outxy;	/* cursor coordinates for screen positioning */

#/************************************************************/

/* X related globals */
Display *dpy;
Window win;
GC gc;
XFontStruct *font;
/*
int Ascent, Descent;
*/	/* font max y excursions */
XEvent event;
XKeyEvent *keve;
Time eve_time;
char *selection_text;	/* selected text for X clipboard */
char *comment;
int selection_length;
int expose;
int vtot, vcur, pos, ftheight; /* global variables for scrollbar */

int Width, Height;
int fwidth, fheight, ycur = 0;	/* font character width, height */

/* Foreground, Background normal and highlight colores */
XColor FgXColor, BgXColor, CrXColor, 
       HiFgXColor, HiBgXColor;

int HiLo;						/* hightlight status, 0=off */
Atom DeleteWindow;				/* Atom of delete window message */

/* char *FgColor="Black", *BgColor="Linen", *HiFgColor="Black", *HiBgColor="Grey75"; */
#ifdef DARK
char	*FgColor="Yellow", 
	*BgColor="Black", 
	*CrColor="White", 
#else
char	*FgColor="Black", 
	*BgColor="White", 
	*CrColor="Red", 
#endif
	*HiFgColor="Black", 
	*HiBgColor="Grey84";
char *FontName=NULL;
char *DisplayName=NULL;
char *AppName;
char *RcFile = NULL;

char *Geometry=NULL;
/* maximum viewable 6x8 chars on a 2000x1600 screen */
#define MAXVLINE 450
char eolbuf[MAXVLINE];	/* holds spaces for clreol(), 450 is 2000x1600 */

/* func prototypes */

void gotoxy(int horz,int vert);
void cursor_draw(unsigned long color);
void draw_cursor();
void undraw_cursor();
void drawstring(char *str, int len);
void cputs(char *prntstr);
int putch(char chr);
void clreol();
void highvideo();
void lowvideo();
void clrscr();
void bell();
void show_vbar();
void update();
void scroll_text();
void sig_handler(int);
void font_height(void);
int paste_primary(int win, int property, int Delete);
int request_selection(int time);
char *mrealloc(char *s, int len);
void set_selection();
void send_selection(XSelectionRequestEvent * rq);
void init(int argc,char *argv[]);
void handle_key(char *astr, int skey, int state);
void moveto();
void do_paste();
void do_select(int delete);
void iconify();
void set_title(char *str);

/* for monolithic whole include engine here */

#include "version.h"
#include "messages.def"
#include "edit.h"

#ifdef EMACS
#include "x11_bind_em.c"
#endif
#ifdef WORDSTAR
#include "x11_bind_ws.c"
#endif

/******************** Start of cursor I/O ********************/

/* Goto the specified location. */

void gotoxy(int horz,int vert)
{
		outxy.X = horz;
		outxy.Y = vert;
}

void cursor_draw(unsigned long color)
{
	XSetForeground(dpy, gc, color);
#ifdef VERTCURS
	XDrawRectangle(dpy, win, gc,
		(outxy.X * fwidth) + 2, (outxy.Y * fheight) + 2,
		0, fheight - 0);
#else
#ifdef BOXCURS
	XDrawRectangle(dpy, win, gc,
		(outxy.X * fwidth) + 1, (outxy.Y * fheight) + 2,
		fwidth, fheight);
#else
	XFillRectangle(dpy, win, gc,
		(outxy.X * fwidth) + 2, ((outxy.Y+1)* fheight) + 2,
		fwidth-1, 2);
#endif
#endif
	XSetForeground(dpy, gc, HiLo ? HiFgXColor.pixel : FgXColor.pixel);
}

void draw_cursor()
{
  /*
        cursor_draw( HiLo ? HiFgXColor.pixel : FgXColor.pixel);
  */
	cursor_draw( CrXColor.pixel);
}

void undraw_cursor()
{
	cursor_draw( HiLo ? HiBgXColor.pixel : BgXColor.pixel);
}

void drawstring(char *str, int len)
{
	if (outxy.X+len>screen_width+1) len = screen_width+1-outxy.X;
	XDrawImageString(dpy, win, gc, (outxy.X * fwidth) + 2, 
            (outxy.Y+1) * fheight, str, len > 0 ? len : 0);
}

int putch(char chr)
{
	static char str[]="\0\0";

	if(chr >= ' '){
		str[0] = chr;
		drawstring(str, 1);
		outxy.X += 1;
	}

	if(!(chr) || ((chr & 0xff) == LF)){
		clreol();
		outxy.X = 0;
		outxy.Y += 1;
	}
	return 0;
}

void cputs(char *prntstr)
{
	int strl = strlen(prntstr);

	drawstring(prntstr, strl);
	outxy.X += strl;
}

/* Erase from cursor to end of line. */

void clreol()
{
		int tmpx = outxy.X;
		int eollen = abs(screen_width+1 - tmpx);
		outxy.X = outxy.X <= screen_width ? outxy.X : screen_width+1;
		drawstring(eolbuf, eollen);
		outxy.X = tmpx;
}

/* display string if visible, bump X */

void highvideo()
{
		XSetBackground(dpy,gc,HiBgXColor.pixel);
		XSetForeground(dpy,gc,HiFgXColor.pixel);
		HiLo = -1;
}

void lowvideo()
{
		XSetBackground(dpy,gc,BgXColor.pixel);
		XSetForeground(dpy,gc,FgXColor.pixel);
		HiLo = 0;
}

void clrscr()
{
		XClearWindow(dpy,win);
}

void bell()
{
		XBell(dpy,100);
}

void show_vbar()
{
	int j;

	/* display buffer screen height in pixels */
	vtot = (screen_height-2)*fheight - font->descent;

	/* font true height as a constant */
	ftheight = fheight+font->descent+1;

	/* thumb height is display buffer percentage of total lines */
	vcur = (vtot*(screen_height-1))/((ytot)?ytot:1);

	/* min thumb size is 1 char */
	if (vcur<fheight) vcur = fheight;

	/* max thumb size is 2/3 vtot */
	j = 2*vtot/3;
	if (vcur>j) vcur = j;

	/* current line number to vertical thumb pos */
	pos = (ytru * (vtot-(vcur-fheight)))/((ytot)? ytot:1);

	/* draw scrollbar through */
	XSetForeground(dpy,gc,HiBgXColor.pixel);
#ifdef THREED	// provide background for line scroll triangles
	XFillRectangle(dpy, win, gc, Width-10, 0, 10, Height);
#else	/* indicate line scroll rectangles */
	XFillRectangle(dpy, win, gc, Width-10, ftheight, 10, Height+2-2*(ftheight));
#endif /* THREED */

	/* draw thumb */
	XSetForeground(dpy,gc,~HiBgXColor.pixel);
	XDrawRectangle(dpy, win, gc, Width-9, ftheight+pos, 8, vcur);

#ifdef THREED
	{
		XPoint opoints[] ={{Width-10,ftheight-1},{10,0},
						   {-5,-9},{-5,9},
						   {0,Height+2-(2*(ftheight))},{5,9},
						   {5,-9},{-10,0}
						  };
		XSegment lpoints[] ={{Width-1, ftheight+pos, Width-9, ftheight+pos},
							 {Width-9, ftheight+pos, Width-9, ftheight+pos+vcur},
							 {Width-9, ftheight-2, Width-6, ftheight-9},
							 {Width-9, Height+2-ftheight, Width-6, Height+3-ftheight+6}
							};

		/* outline trough and end triangles */
		XDrawLines(dpy, win, gc, opoints, 8, CoordModePrevious);

		/* draw in light color for 3D shading */
		XSetForeground(dpy, gc, -1);
		XDrawSegments(dpy, win, gc, lpoints, 4);
	}
#endif /* THREED */
	lowvideo();
}

void scroll_text()
{
	int m, rep;

        if (ycur<0) { y=0 ; scroll_down(); ycur = 0; }
        if (ycur>ytot) ycur = ytot;

        m = (ycur * vtot) / (ytot? ytot : 1);
	rep = abs(ytru-ycur)/screen_height;

	if(m<0) scroll_down();					/* if cursor at top of screen */
	else if(m>vtot+ftheight) scroll_up();	/* if cursor at bottom of screen */
	else if(m<pos) {
	     for (m=0; m<rep; m++)
	         cursor_pageup();		/* if cursor above thumb */
	}
	else if(m>pos+vcur) {
	     for (m=0; m<rep; m++)
	         cursor_pagedown();	/* if cursor below thumb */
	}

	goto_y(ycur);  /* cursor on thumb so track it */

	scr_update(); /* does show_vbar */
}


void update()
{
		flag[SHW] = 1;
		show_top();
		scr_update();
		show_vbar();
		if (executive == DIALOG) {
			undraw_cursor();
			show_gets(comment, diabuf, dblen, dialogCB);
			gotoxy(diastart+col,yl1);
			draw_cursor();
		}
}

void sig_handler(int nothing)
{
		update();
		XFlush(dpy);
}

/*
void font_height(void)
{
		int foo,bar,baz;
		XCharStruct extents;

		XTextExtents(font,"OOO|jW6789",10,&foo,&bar,&baz,&extents);
		Ascent=extents.ascent;
		Descent=extents.descent;
}
*/


int paste_primary(int win, int property, int Delete)
{
	Atom actual_type;
	int actual_format, i;
	long nitem, bytes_after, nread;
	unsigned char *data;
	char indent = flag[IND];	/* stash autoindent state */

	if (property == None)	/* don't paste anything */
		return(0);

	flag[IND] = 0;	/* off autoindent */
	nread = 0;
	/* X-selection paste loop */
	do {
		if (XGetWindowProperty		/* get remaining selection max 1024 chars */
			(dpy, win, property, nread / 4, 1024, Delete,
			 AnyPropertyType, &actual_type, &actual_format, &nitem,
			 &bytes_after, (unsigned char **) &data)
			!= Success)
			return(0);
		update_scr = 0;				/* dont update scr...yet */
		/* paste last batch one char at a time */
		for(i = 0; i < nitem; handle_key(NULL, data[i++],0));
		nread += nitem;
		XFree(data);
	} while (bytes_after > 0);
	update_scr = 1;					/* _now_ update display */
	flag[SHW] = 1;
	scr_update();
	flag[IND] = indent;	/* restore autoindent state */
	return(nread);
}

int request_selection(int time)
{
	Window w;
	Atom property;

	if ((w = XGetSelectionOwner(dpy, XA_PRIMARY)) == None) {
		int tmp = paste_primary(DefaultRootWindow(dpy), XA_CUT_BUFFER0,
				  False);
		return(tmp);
	}
	property = XInternAtom(dpy, "VT_SELECTION", False);
	XConvertSelection(dpy, XA_PRIMARY, XA_STRING, property, win,
			  time);
	return(0);
}

char *mrealloc(char *s, int len)
{
		char *ttt;
		if(!s) ttt = (char *) malloc(len);
		else ttt = (char *) realloc(s, len);
		return ttt;
}

void set_selection()
{
	int i;

	if(!flag[BLK] || mk == cur_pos) return;

		if(cur_pos < mk)
			{ bstart = cur_pos; bend = mk; }
		else
			{ bstart = mk; bend = cur_pos; }

	selection_length = bend - bstart;
	if ((selection_text = (char *) mrealloc(selection_text, selection_length)) == NULL) {
		printf(SELECT_REALLOC"\n");
se_exit:
		bell();
		return;
	}
	for (i = 0; i < selection_length; i++) {
		selection_text[i] = bstart[i] == EOL ? '\n' : bstart[i];
	}
	XSetSelectionOwner(dpy, XA_PRIMARY, win, (Time) eve_time);
	if (XGetSelectionOwner(dpy, XA_PRIMARY) != win) {
		printf(CANT_SELECT"\n");
		goto se_exit;
	}
	XChangeProperty(dpy, DefaultRootWindow(dpy), XA_CUT_BUFFER0,
			XA_STRING, 8, PropModeReplace, selection_text,
			selection_length);
}

void send_selection(XSelectionRequestEvent * rq)
{
	XSelectionEvent notify;

	notify.type = SelectionNotify;
	notify.display = rq->display;
	notify.requestor = rq->requestor;
	notify.selection = rq->selection;
	notify.target = rq->target;
	notify.time = rq->time;
	XChangeProperty(dpy, rq->requestor, rq->property, XA_STRING, 8,
			PropModeReplace, selection_text, selection_length);
	notify.property = rq->property;
	XSendEvent(dpy, rq->requestor, False, 0, (XEvent *) & notify);
}

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
		if (!strcasecmp(arg1, "-w")) {
			screen_width = atoi(arg2);
			if (screen_width<20) screen_width = 20;
			if (screen_width>180) screen_width = 180;
		}
		else
		if (!strcasecmp(arg1, "-h")) {
			screen_height = atoi(arg2);
			if (screen_height<5) screen_height = 5;
			if (screen_height>80) screen_height = 80;
		}
		else
		if (!strcasecmp(arg1, "-fn"))
			FontName = strdup(arg2);
		else
		if (!strcasecmp(arg1, "-rc"))
			RcFile = strdup(arg2);
		else
		if (!strcasecmp(arg1, "-fg"))
	        	FgColor = strdup(arg2);
		else
		if (!strcasecmp(arg1, "-bg"))
			BgColor = strdup(arg2);
		else
		if (!strcasecmp(arg1, "-hifg"))
			HiFgColor = strdup(arg2);
		else
		if (!strcasecmp(arg1, "-hibg"))
			HiBgColor = strdup(arg2);
		else
		if (!strcasecmp(arg1, "-cr"))
			CrColor = strdup(arg2);
		else
		if (!strcasecmp(arg1, "-edit"))
		        flag[EDT] = atoi(arg2);
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

void init(int argc,char *argv[])
{
		int screen;
		XWMHints *wmh;
		XSizeHints *xsh;
		XClassHint *classh;
		XColor tmp;
		int i, x, y;

		/* Default settings */
		*ewin.name = '\0';
		amax = AMAX;
		bmax = BMAX;
		umax = UMAX;

		/* engine screen width, height, font */
		screen_height = DEFAULT_HEIGHT - 1;
		screen_width = DEFAULT_WIDTH;

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
                                    char* p=strchr(argv[i],':');
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

		/* open the display */
		dpy=XOpenDisplay(DisplayName);
		if(dpy==NULL)  {
				fprintf(stderr,"Can't open display: %s\n",DisplayName);
				sys_exit(1);
		}

		/* setup to gracefully respond to exit requests from X */
		screen=DefaultScreen(dpy);
		DeleteWindow = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

		/* establish window manager hints data structure */
		xsh=XAllocSizeHints();
		wmh=XAllocWMHints();
		classh=XAllocClassHint();

		x = 1; y=0;

		/* setup font(s) */
		if(FontName == NULL) { FontName = FONTNAME; }

		font=XLoadQueryFont(dpy, FontName);
		if(font==NULL) {
				fprintf(stderr, FONTS_NOT_FOUND"\n");
				sys_exit(1);
		}

		fheight = font->ascent + font->descent;
		Height = fheight * screen_height + font->descent + 2;

		/* engine screen width */
		fwidth = XTextWidth(font,"8",1);

		Width = (fwidth * screen_width)+13;
		xsh->flags = PSize; xsh->width = Width; xsh->height = Height;

		/* initialize clreol string to all blanks */
		memset(eolbuf, ' ', sizeof(eolbuf));

		/* create the only window */
		win=XCreateSimpleWindow(dpy,RootWindow(dpy,screen), x, y,
                                Width, Height, 0,
				BlackPixel(dpy,screen),WhitePixel(dpy,screen));

		/* setup window hints */
		wmh->initial_state=NormalState;
		wmh->input=True;
		wmh->window_group = win;
		wmh->flags = StateHint | InputHint | WindowGroupHint;

		/* setup window class resource names */
		classh->res_name = (AppName==NULL)?EDIT:AppName;
		classh->res_class = "Xedit";

		/* name that window */
		XmbSetWMProperties(dpy, win, EDIT, EDIT, argv, argc,
										   xsh, wmh, classh);
		/* notify X on how to force emx to exit */
		XSetWMProtocols(dpy, win, &DeleteWindow, 1);

		/* specify accepted XEvent loop events */
		XSelectInput(dpy, win,
						KeyPressMask|\
						FocusChangeMask|\
						StructureNotifyMask|\
						ButtonPressMask|\
						ButtonReleaseMask|\
						ExposureMask|\
						PropertyChangeMask|\
						Button1MotionMask|\
						Button2MotionMask|\
						Button3MotionMask|\
						VisibilityChangeMask
		);
		keve = (XKeyEvent *)&event;

		/* create the Graphic Context for drawing purposes */
		gc=XCreateGC(dpy,win,0,NULL);

		/* allocate required colors */
		XAllocNamedColor(dpy,DefaultColormap(dpy,screen),FgColor,&FgXColor,&tmp);
		XAllocNamedColor(dpy,DefaultColormap(dpy,screen),BgColor,&BgXColor,&tmp);
		XAllocNamedColor(dpy,DefaultColormap(dpy,screen),CrColor,&CrXColor,&tmp);
		XAllocNamedColor(dpy,DefaultColormap(dpy,screen),HiFgColor,&HiFgXColor,&tmp);
		XAllocNamedColor(dpy,DefaultColormap(dpy,screen),HiBgColor,&HiBgXColor,&tmp);

		/* apply colors to window */
		XSetForeground(dpy,gc,FgXColor.pixel);
		XSetWindowBackground(dpy,win,BgXColor.pixel);

		/* set the font */
		XSetFont(dpy,gc,font->fid);

		/* map the window real */
		XMapWindow(dpy, win);
}

void handle_key(char *astr, int skey, int state)
{
		char chstr[256];
		int x=outxy.X, y=outxy.Y, n=0;

		/* display keyboard shift/control/alt status */
		highvideo();
		gotoxy(0,yl1);
		for(n=5;n;chstr[n--]=' ');
		if(state & ShiftMask) chstr[n++] = 'S';
		if(state & Mod1Mask) chstr[n++] = 'A';
		if(skey & 0xff00) chstr[n++] = 'F';
		if(state & ControlMask) chstr[n++] = '^';
		chstr[n] = skey & 0xff;
		chstr[5] = '\0';
		cputs(chstr);
		chstr[0] = '\0';

#ifdef DEBUG
		/* display raw key event data */
		sprintf(chstr,"k=%4x,s=%2x.",skey,state);
#endif /* DEBUG */
		gotoxy(screen_width - 12,0);
		clreol();
		cputs(chstr);
		gotoxy(x,y);
		lowvideo();

		if((skey >= 0xffe1) && (skey <= 0xffee)) return;
		if(skey == NoSymbol) return;

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

void moveto()
{
	move_to(((event.xbutton.x < 0 ? 0 : event.xbutton.x)/fwidth) + 1 + xlo,
			((event.xbutton.y-3)/fheight) - 1);
}

void do_select(int delete)
{
	if(flag[BLK] && mk != cur_pos) {
		set_selection();
		block_copy(delete);
	}
	mark_off();
}

void do_paste()
{
	if(flag[BLK] && executive == MAIN) block_remove_update();
	request_selection(event.xbutton.time);
}

void iconify()
{
	XIconifyWindow(dpy, win, DefaultScreen(dpy));
}

void set_title(char *str)
{
	char b[NLEN];

	sprintf(b, EDIT" : %s", str);
	XStoreName(dpy, win, b);
}

int main(int argc,char *argv[])
{
	Atom WM_PROTOCOLS = 0;
	struct sigaction sig;
	int yp0 = 0, yp1 = 0, yf, yt;

	init(argc,argv);

	/* disconnect from the console */
	/* JPD commented this out. Use instead emx/edx & from shell */
	/*
	switch (fork()) { case 0: case -1: break; default: exit(0); }
	*/

	/* set path */
	if (cfdpath == NULL) {
		cfdpath = (char *) malloc(BUFSIZ);
		getcwd(cfdpath, BUFSIZ);
		if (strcmp(cfdpath, "/") != 0)
			strcat(cfdpath, "/");
	}

	usleep(10000);
	XFlush(dpy);
	do_open();

/* end of edit engine init */

	/* request/create WM_PROTOCOLS atom */
	WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", False);

	/* set up the signal handler response */
	sig.sa_handler=sig_handler;
	sigemptyset(&sig.sa_mask);
	sig.sa_flags=SA_RESTART;
	sigaction(SIGALRM,&sig,NULL);

	/* display the initial cursor */
	clrscr();

#define DBLCLICK 300

	/* main event loop, dispatch function calls in response to events */
	for(;;)  {
			static Time button_release_time;
			static int buttonpressed = 0;
			XNextEvent(dpy,&event);

			switch(event.type)	{
			case Expose:
				while(XCheckTypedEvent(dpy,Expose,&event));
				expose = 1;
				update();
				expose = 0;
				break;
			case MotionNotify:
				if (buttonpressed==-1) {
				        if (executive == MAIN) {
					   ycur = 
				     ((event.xbutton.y - ftheight)*ytot)/vtot;
					   scroll_text();
					}
					break;
				}
#ifndef TWOBUTN
			motion:
				yf = (buttonpressed == Button3)? fheight : 0;
#endif
				if(buttonpressed) {
#ifdef TWOBUTN
					if(!flag[BLK]) block_mark();
				  	undraw_cursor();
					if (event.xbutton.y < fheight + 2) {
#else
					if(!flag[BLK] && ! yf) block_mark();
				  	undraw_cursor();
					if (event.xbutton.y < fheight + yf + 2) {
#endif
						moveto();
						cur_pos = get_cur();
						scroll_down();
						scr_update();
					}
					else
#ifdef TWOBUTN
					if (event.xbutton.y >= Height-6-font->descent) {
#else
					if (event.xbutton.y >= Height-yf-6-font->descent) {
#endif
						moveto();
						cur_pos = get_cur();
						scroll_up();
						scr_update();
					}
					else {
#ifndef TWOBUTN
						if (yf) goto finish;
#endif
						moveto();
						yf = yp0;
						if (y<yf) yf = y;
						if (yp1<yf) yf = yp1;
						yt = yp0;
						if (y>yt) yt = y;
						if (yp1>yt) yt = yp1;
						show_scr(yf, yt);
#ifndef TWOBUTN
					finish:
#endif
						moveto();
						cur_pos = get_cur();
						gotoxy(x-1,y+1);
						draw_cursor();
						yp1 = y;
					}
				}
				break;
			case ButtonPress:
				if (event.xbutton.button>Button3 &&
				    executive==MAIN) {
				   if (event.xbutton.button==Button4) {
				      ycur -= 3;
				      scroll_text();
				   }
				   if (event.xbutton.button==Button5) {
				      ycur += 3;
				      scroll_text();
				   }
				   break;
				}
				help_done = 0;
				undraw_cursor();
			        if (event.xbutton.x>=Width-10) {
					buttonpressed = -1;
				        if (executive == MAIN)
						scroll_text(event.xbutton.y);
					break;
				}
				buttonpressed = event.xbutton.button;
#ifndef TWOBUTN
				if (buttonpressed == Button3) {
					mark_off();
					scr_update();
					goto motion;
				}
#endif
				if (event.xbutton.time - eve_time < DBLCLICK) break;
				eve_time = event.xbutton.time;
				moveto();
				yp0 = y;
				flag[SHW] = 1;
				cur_pos = get_cur();
				if (flag[BLK]) {
					mark_off(); 	/* unmark */
					scr_update();
				} else {
					block_mark();	/* start new mark */
					draw_cursor();
				}
				break;
			case ButtonRelease:
				if (buttonpressed==-1) {
					buttonpressed = 0;
					break;
				}
				buttonpressed = 0;
#ifndef TWOBUTN
				if (event.xbutton.button == Button3) goto motion;
#endif
				switch (event.xbutton.button) {
				case Button1:
					if (event.xbutton.time - button_release_time < DBLCLICK) {
						mark_off(); cursor_right(); word_left(); block_mark();	/* set mark to left end of word */
						word_right(); scr_update();	/* set mark to right end of word */
						break;
					} 
					else
					{

						button_release_time = event.xbutton.time;
						if (mk == cur_pos)
							mark_off();
						else
							set_selection();
						goto setcursor;
					}
					break;
#ifdef TWOBUTN
				case Button2:
				case Button3:
#else
				case Button2:
#endif
					if ((event.xbutton.time - eve_time < DBLCLICK) &&
						(event.xbutton.time - eve_time)) {
							do_paste();
					}
					goto setcursor;
				}
				break;
			setcursor:
				moveto();
				flag[SHW] = 1;
				cur_pos = get_cur();
				if (flag[BLK])
					scr_update();
				else
					draw_cursor();
				break;

			case KeyPress:{
				int count;
				char astr[10];
				KeySym skey;
				eve_time = keve->time;
				astr[0] = 0;
				count = XLookupString(keve, astr,
						  sizeof (astr), &skey, NULL);
				astr[count] = 0;
				handle_key(astr, skey, keve->state);
			}
			break;
		case SelectionClear:
			break;
		case SelectionRequest:
			send_selection((XSelectionRequestEvent *) &
					   event);
			break;
		case SelectionNotify:
			paste_primary(event.xselection.requestor,
					  event.xselection.property, True);
			break;
		case ConfigureNotify:
			Width = event.xconfigure.width;
			screen_width = (Width-16)/fwidth;
			Height = event.xconfigure.height;
			screen_height = (Height/fheight) -1 ;
			update();
			break;
		case ClientMessage:
			if(event.xclient.message_type == WM_PROTOCOLS)	{
			if(event.xclient.data.l[0] == DeleteWindow) {
				XCloseDisplay(dpy);
				flag[CHG] = 0;
				sys_exit(0);
			}
		}
		break;

		case DestroyNotify:
			XCloseDisplay(dpy);
			exit(0);

		}

	}
}

#include <unistd.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <string.h>

#include "sunclock.h"
#include "langdef.h"

extern char *   salloc();
extern void     StringReAlloc();

extern char             **get_dir_list();
extern int              dup_strcmp();
extern void             free_dirlist();

extern char *           num2str();
extern void             processKey();
extern int              readVMF();
extern void             buildMap();
extern int              parseCmdLine();
extern void             correctValues();
extern void             getFonts();

extern void             createGData();
extern void             createGCs();
extern void             clearNightArea();
extern void             drawCities();
extern void             drawSunAndMoon();
extern void             drawDottedRectangle();

extern Display *	dpy;
extern int		scr;
extern Colormap		cmap0, tmp_cmap;

extern int              color_depth;
extern int              runlevel;
extern int              text_input;

extern Atom		wm_delete_window;
extern Window		Root, Menu, Filesel, Zoom, Option, Urban;
extern Pixmap           zoompix, textpix, rootpix;
extern Pixel		black, white;

extern Flags            gflags;
extern ZoomSettings     gzoom;

extern TextEntry        option_entry, urban_entry[5];

extern struct Sundata   *Seed;
extern struct Sundata   *MenuCaller, *FileselCaller, 
                        *ZoomCaller, *OptionCaller, *UrbanCaller, *RootCaller;
extern struct Geometry	ClockGeom, MapGeom;
extern struct Geometry  MenuGeom, FileselGeom, ZoomGeom, OptionGeom, UrbanGeom;

extern char *	        ProgName;
extern char *	        Title;
extern char *           widget_type[7];
extern char *           EditorCommand;

extern char *	        ClassName;
extern char *           ClockClassName;
extern char *           MapClassName;
extern char *           AuxilClassName;

extern char *           ExternAction;
extern char *           CityInit;

extern char *           share_maps_dir;
extern char **          dirtable;
extern char *           image_dir;
extern char *           Help[N_HELP];
extern char *           Label[L_END];
extern char *           FileselBanner[N_FILESEL];
extern char             WeakChars[];

extern char             language[4];

extern char *           Clock_img_file;
extern char *           Map_img_file;
extern char *           Zoom_img_file;

extern int              city_cat;
extern int              *city_spotsizes;
extern int              *city_sizelimits;
extern int		do_menu, do_filesel, do_zoom, do_option, do_urban;
extern int              do_dock, do_sync, do_zoomsync, do_root;

extern int              placement;
extern int              place_shiftx;
extern int              place_shifty;

extern int              verbose;
extern int		button_pressed;
extern int              option_changes;
extern int              root_period;

extern int              horiz_drift, vert_drift;
extern int              label_shift, filesel_shift, zoom_shift;

extern int              num_lines, num_table_entries;

extern int              zoom_mode, zoom_active;

extern char	        menu_lasthint;
extern char	        option_lasthint;
extern char             zoom_lasthint;
extern char             urban_lasthint;

extern KeySym	        menu_newhint;
extern char	        option_newhint;
extern char             zoom_newhint;
extern char             urban_newhint;

extern long             last_time;
extern long             progress_value[6];

extern double           darkness;
extern unsigned int     adjust_dark;

extern int              urban_t[5], urban_x[5], urban_y[5], urban_w[5];

int                     areaw, areah;
int			urbanh;

void shutDown();

/* Is it really more secure than plain tmpnam() ?? */
void 
secure_tmpnam(char *name)
{
    char compl[4];
    static int order = 0;
    compl[0] = 'a' + ((order/676)%26);
    compl[1] = 'a' + ((order/26)%26);
    compl[2] = 'a' + (order%26);
    compl[3] = '\0';
    ++order;
    sprintf(name, "%s/XXXXXX", P_tmpdir);
    close(mkstemp(name));
    unlink(name);
    strcat(name, compl);
}

void 
showManual()
{
int i=0, j, k, l, c, cp;
char cmd[256];
char tmpname[80];
struct stat buf;
char *ptr, *text;
FILE *man;

     secure_tmpnam(tmpname);
     strcat(tmpname, "_sunclock.man");
     sprintf(cmd, "man sunclock > %s", tmpname);
     system(cmd);
     man = fopen(tmpname, "r");
     if (!man) {
        unlink(tmpname);
	return;
     }
     ptr = strdup(tmpname);
     buf.st_mode = 0;
     stat(ptr, &buf);
     free(ptr);
     text = malloc(buf.st_size+2);
     if (!text) {
        fclose(man);
        return;
     }
     while ((c=fgetc(man))!=EOF) {
       iter:
        if (c=='_') {
	   cp = fgetc(man);
	   if (cp==EOF) break;
	   if (cp==8) continue;
	   text[i++] = (char)c;
	   c = cp;
           goto iter;
	} else
	if (c==8) {
	   c = fgetc(man);
	   if (c==EOF) break;
	} else
	   text[i++] = (char)c;
     }
     text[i] = '\0';
     fclose(man);
     man = fopen(tmpname, "w");
     if (!man) {
        free(text);
        return;
     }
     for (j=0; j<i; j++) {
         if (text[j]=='\n' && !strncmp(text+j+1, "Sunclock-", 6)) {
	    k = j;
	    while (isspace(text[k])) k--;
	    while (text[k]!='\n') k++;
	    ptr = strstr(text+j+1, "sunclock(1x)");
	    if (ptr) ptr = strstr(ptr+2, "sunclock(1x)");
	    if (ptr) {
               j = ptr-text+1;
	       while (text[j]!='\n') j++;
	       while (isspace(text[j])) j++;
	       while (text[j]!='\n') j--;
	       for (l=k; l<j; l++) text[l] = '\0';
	    } else
	       for (l=k+3; l<j; l++) text[l] = '\0';
	 }
     }
     for (j=0; j<i; j++) {
         if (text[j]) fputc(text[j], man);
     }
     free(text);
     fflush(man);
     fclose(man);
     chmod(tmpname, 0555);
     sprintf(cmd, "(%s %s ; rm -f %s ) &", 
	     EditorCommand, tmpname, tmpname);
     system(cmd);
}

int 
getState(w)
Window w;
{
XWindowAttributes xwa;
   XGetWindowAttributes(dpy, w, &xwa);
   return xwa.map_state;
}

int
getPlacement(win, x, y, w, h)
Window win;
int *x, *y;
unsigned int *w, *h;
{
   int xp, yp;
   unsigned int b, d, n;
   Window junk, root, parent, *children;

   XFlush(dpy);
   XQueryTree(dpy, win, &root, &parent, &children, &n);
   if (!parent) {
      fprintf(stderr, "Cannot query window tree!\n");
      return 1;
   }
   
   if (parent == root) parent = win;

   XGetGeometry(dpy, parent, &root, x, y, w, h, &b, &d);

   XTranslateCoordinates(dpy, win, parent, 0, 0, x, y, &junk);
   if (2*(*x) < *w) *w -= 2*(*x);
   if ((*x)+(*y) < *h) *h -= (*x) + (*y);
   XTranslateCoordinates(dpy, win, root, 0, 0, x, y, &junk);
   XTranslateCoordinates(dpy, parent, root, 0, 0, &xp, &yp, &junk);

   horiz_drift = *x - xp;
   vert_drift = *y - yp;
/*
   if (verbose) {
      fprintf(stderr, "Window placement (%d,%d)\n", *x, *y);	
      fprintf(stderr, "Window drift (%d,%d)\n", horiz_drift, vert_drift);
   }
*/
   return 0;
}

void
checkGeom(Context, bool)
struct Sundata * Context;
int bool;
{
	int a, b;

        a = DisplayWidth(dpy, scr) - Context->geom.width - horiz_drift - 5;
	b = DisplayHeight(dpy, scr) - Context->geom.height
              - Context->hstrip - vert_drift - 5;
        if (Context->geom.x > a) Context->geom.x = a;
        if (Context->geom.y > b) Context->geom.y = b;
	if (Context->geom.x < 0) Context->geom.x = 5;
	if (Context->geom.y < 0) Context->geom.y = 5;
        if (bool) Context->geom.mask = 
                      XValue | YValue | WidthValue | HeightValue;
}

void
adjustGeom(Context, which)
struct Sundata * Context;
int which;
{
        int x, y, dx=0, dy=0, diff;
        unsigned int w, h;

        if (getPlacement(Context->win, &x, &y, &w, &h)) return;

	if (which) {
	   diff= MapGeom.height - ClockGeom.height +
	         Context->gdata->mapstrip - Context->gdata->clockstrip;
	   switch(placement) {
	      case CENTER:
		 dx = (MapGeom.width - ClockGeom.width)/2;
	         dy = diff/2;
	         break;
	      case NE:
                 dx = MapGeom.width - ClockGeom.width;
	         break;
	      case SE:
                 dx = MapGeom.width - ClockGeom.width;
	      case SW:
	         dy = diff;
	         break;	
	      default:
	         break;
	   }
	}

        if (placement) {
	    dx = dx - place_shiftx;
	    dy = dy - place_shifty;
	    if (Context->wintype) {
	       dx = -dx;
	       dy = -dy;
	    }
	    if (placement >= CENTER) {
	       Context->geom.x = x + dx - horiz_drift;
	       Context->geom.y = y + dy - vert_drift;
	       checkGeom(Context, 1);
	    }
	}
}

/*
 * Set up stuff the window manager will want to know.  Must be done
 * before mapping window, but after creating it.
 */

void
setSizeHints(Context, num)
struct Sundata *                Context;
int				num;
{
	XSizeHints		xsh;
	struct Geometry *       Geom = NULL;
	Window			win = 0;

        if (num<=1) {
	    if (!Context) return;
	    win = Context->win;
	    Geom = &Context->geom;
	    xsh.flags = PSize | PMinSize;
	    if (Geom->mask & (XValue | YValue)) {
		xsh.x = Geom->x;
		xsh.y = Geom->y;
                xsh.flags |= USPosition; 
	    }
	    xsh.width = Geom->width;
	    xsh.height = Geom->height + Context->hstrip;
	    if (do_dock && Context==Seed) {
              xsh.max_width = xsh.min_width = xsh.width;
              xsh.max_height = xsh.min_height = xsh.height;
	      xsh.flags |= PMaxSize | PMinSize;
	    } else {
              xsh.min_width = Geom->w_mini;
              xsh.min_height = Geom->h_mini + Context->hstrip;
	    }
	} else 
	if (num>=2) {
	    xsh.flags = PSize | USPosition | PMinSize;
	    if (num==2) {
	      win = Menu;
	      Geom = &MenuGeom;
	      xsh.flags |= PMaxSize;
	      xsh.min_width = xsh.max_width = Geom->width;
              xsh.min_height = xsh.max_height = Geom->height;
	    }
	    if (num==3) {
	      win = Filesel;
	      Geom = &FileselGeom;
	      xsh.min_width = Geom->w_mini;
              xsh.min_height = Geom->h_mini;
	    }
	    if (num==4) {
	      win = Zoom;
	      Geom = &ZoomGeom;
	      xsh.min_width = Geom->w_mini;
              xsh.min_height = Geom->h_mini;
	    }
	    if (num==5) {
	      win = Option;
	      Geom = &OptionGeom;
	      xsh.flags |= PMaxSize;
	      xsh.min_width = Geom->w_mini;
              xsh.max_width = 2000;
              xsh.min_height = xsh.max_height = Geom->h_mini;
	    }
	    if (num==6) {
	      win = Urban;
	      Geom = &UrbanGeom;
 	      xsh.flags |= PMaxSize;
              xsh.max_width = 2000;
              xsh.min_width = Geom->w_mini;
              xsh.min_height = xsh.max_height = Geom->h_mini;
	    }
	    xsh.x = Geom->x;
	    xsh.y = Geom->y;
	    xsh.width = Geom->width;
	    xsh.height = Geom->height;
	}

	if (!win) return;
	XSetNormalHints(dpy, win, &xsh);
}

void
setClassHints(win, num)
Window win;
int    num;
{
        char *titlename, *iconname;
        XClassHint xch;

	titlename = NULL;
	if (!Title) StringReAlloc(&Title, ProgName);

	if (!ClassName) {
	   StringReAlloc(&ClassName, ProgName);
	   *ClassName = toupper(*ClassName);
	}

	if (ClassName && !ClockClassName) 
           StringReAlloc(&ClockClassName, ClassName);

	if (ClassName && !MapClassName) 
           StringReAlloc(&MapClassName, ClassName);

	if (ClassName && !AuxilClassName) 
           StringReAlloc(&AuxilClassName, ClassName);

	if (num == 0)
	  xch.res_class = ClockClassName;
        else
	if (num == 1)
	  xch.res_class = MapClassName;
	else
	  xch.res_class = AuxilClassName;

        xch.res_name = ProgName;

        XSetClassHint(dpy, win, &xch);

	iconname = (char *)
           salloc((strlen(Title)+strlen(VERSION)+10)*sizeof(char));
        sprintf(iconname, "%s %s", Title, VERSION);
        XSetIconName(dpy, win, iconname);
	free(iconname);

	titlename = (char *)
           salloc((strlen(Title)+20)*sizeof(char));
        sprintf(titlename, "%s / %s", Title, widget_type[num]);
        XStoreName(dpy, win, titlename);
	free(titlename);
}

void
setProtocols(Context, num)
struct Sundata * Context;
int				num;
{
	Window			win = 0;
	long                    mask;

	mask = FocusChangeMask | VisibilityChangeMask | ExposureMask | 
               StructureNotifyMask | 
               ButtonPressMask | ButtonReleaseMask | KeyPressMask;

	if (num>=1)
	       mask |= PointerMotionMask;

        /* use StructureNotifyMask rather than  ResizeRedirectMask 
           to avoid bug in some window mangers... as enlightenment !
           All events would be:
	   for (i=0; i<=24; i++) mask |= 1L<<i;
         */

        switch(num) {

	   case 0:
	   case 1:
                if (!Context) return;
	        mask |= EnterWindowMask | LeaveWindowMask;
		win = Context->win;
		break;

	   case 2:
		win = Menu;
		break;

	   case 3:
	        win = Filesel;
		break;

	   case 4:
	        win = Zoom;
		break;

	   case 5:
	        win = Option;
		mask |= KeyReleaseMask;
		break;

	   case 6:
	        win = Urban;
		mask |= KeyReleaseMask;
		break;

 	   default:
	        break;
	}

	if (!win) return;

       	XSelectInput(dpy, win, mask);
	XSetWMProtocols(dpy, win, &wm_delete_window, 1);
}

Window
newWindow(Context, Geom, num)
struct Sundata * Context;
struct Geometry * Geom;
int num;
{
        Window 		        win = 0;
	int                     strip;

	if (num<=1) {
 	   strip = (num)? Context->gdata->mapstrip :
                          Context->gdata->clockstrip;
	   Context->hstrip = strip;
	   if (Geom->mask & XNegative)
	      Geom->x = DisplayWidth(dpy, scr) - Geom->width + Geom->x;
	   if (Geom->mask & YNegative)
	      Geom->y = DisplayHeight(dpy, scr) - Geom->height + Geom->y-strip;
	} else
	   strip = 0;

	win = XCreateSimpleWindow(dpy, Root,
                      Geom->x, Geom->y, 
                      Geom->width, Geom->height + strip, 0,
		      black, white);
        if (win) setClassHints(win, num);
	return win;
}

void getWinParams(win, Context, keys, nkeys, y, width)
Window win;
struct Sundata **Context;
char **keys;
int *nkeys;
int *y;
int *width;
{
    *Context = NULL;
    if (win==Menu) {
       *Context = MenuCaller;
       *keys = MenuKey;
       *nkeys = N_MENU;
       *y = 0;
       *width = MenuGeom.width;
    }
    if (win==Option) {
       *Context = OptionCaller;
       *keys = OptionKey;
       *nkeys = N_OPTION;
       *y = OptionGeom.height-2*OptionCaller->gdata->menustrip - 1;
       *width = OptionGeom.width;
    }
    if (win==Zoom) {
       *Context = ZoomCaller;
       *keys = ZoomKey;
       *nkeys = N_ZOOM;
       *y = areah+64;
       *width = ZoomGeom.width;
    }
    if (win==Urban) {
       *Context = UrbanCaller;
       *keys = UrbanKey;
       *nkeys = N_URBAN;
       *y = urbanh;
       *width = UrbanGeom.width;
    }
    if (win==Filesel) {
       *Context = FileselCaller;
       *keys = WeakChars;
       *nkeys = 6;
       *y = 0;
       *width = FileselGeom.width;
    }
}

int 
getButton(win, x, y)
Window win;
int x, y;
{
struct Sundata * Context=NULL;
char *keys=NULL;
int nkeys=0;
int y1, w0, n, i, j=0, b, d;
    getWinParams(win, &Context, &keys, &nkeys, &y1, &w0);
    n = -1;
    if (y<=y1 || y>=y1+Context->gdata->menustrip) return n;
    b = 0;
    d = Context->gdata->charspace;
    if (win==Filesel) d = 2*d;
    for (i=0; i<nkeys; i++) {
	j = i*d + b;
	b += 5*(keys[2*i+1]==';');
	if (x>j && x<j+d) {
	    n = i;
	    break;
	}
    }
    if (x>j+d+5) n = nkeys;
    return n;
}

void
drawBox(win, gc, pix, x1, y1, x2, y2, clicked)
Window win;
GC gc;
Pixel *pix;
int x1, y1, x2, y2;
int clicked;
{
    XSetForeground(dpy, gc, pix[BUTTONCOLOR]);
    XFillRectangle(dpy, win, gc, x1, y1, x2-x1, y2-y1);

    XSetForeground(dpy, gc, pix[(clicked)?BUTTONFGCOLOR1:BUTTONFGCOLOR3]);
    XDrawLine(dpy, win, gc, x1, y1, x1, y2);
    XDrawLine(dpy, win, gc, x1, y1, x2, y1);

    XSetForeground(dpy, gc, pix[(clicked)?BUTTONFGCOLOR2:BUTTONFGCOLOR4]);
    XDrawLine(dpy, win, gc, x1+1, y1+1, x1+1, y2-1);
    XDrawLine(dpy, win, gc, x1+1, y1+1, x2-1, y1+1);

    XSetForeground(dpy, gc, pix[(clicked)?BUTTONFGCOLOR4:BUTTONFGCOLOR2]);
    XDrawLine(dpy, win, gc, x1+1, y2-1, x2-1, y2-1);
    XDrawLine(dpy, win, gc, x2-1, y1+1, x2-1, y2-1);

    XSetForeground(dpy, gc, pix[(clicked)?BUTTONFGCOLOR3:BUTTONFGCOLOR1]);
    XDrawLine(dpy, win, gc, x1, y2, x2, y2);
    XDrawLine(dpy, win, gc, x2, y1, x2, y2);
}

void 
drawButton(win, n, clicked)
Window win;
int n;
int clicked;
{
int i, j=0, b, d, x1=0, y1=0, x2=0, y2=0, x, y, w0=0;
struct Sundata * Context=NULL;
char c[2] = { '\0', '\0'};
char *s = c;
char *keys=NULL;
int nkeys=0;
int charspace;

    getWinParams(win, &Context, &keys, &nkeys, &y1, &w0);
    if (!Context) return;
    charspace = Context->gdata->charspace;
    if (win==Filesel) charspace = 2*charspace;

    b = 0;
    for (i=0; i<=n; i++) {
        j = i*charspace + b;
        if (win!=Filesel)
	   b += 5*((i==nkeys-1) || (keys[2*i+1]==';'));
    }
    x1 = j;
    y2 = y1+Context->gdata->menustrip;
    if (n>=0 && n<nkeys) {
        if (win==Filesel)
	   s = FileselBanner[n];
	else {
           c[0] = keys[2*n];
	}
        x2 = j+charspace-1;
    } else {
        if (win==Zoom && n==nkeys+1) {
	   c[0] = (zoom_active)?'+':'-';
           x1 = areah+170, 
           y1 = 21;
           x2 = x1+charspace;
           y2 = y1+18;        
        } else {
           s = Label[L_ESCAPE];
           x2 = w0 - 1;
	}
    }

    if (clicked<=-1) {
       XSetForeground(dpy, Context->gdata->wingc, 
	              Context->gdata->pixel[(clicked==-2)?
                                  BUTTONCOLOR:MENUFGCOLOR]);
       for (y=y1+2; y<=y2-3; y+=y2-y1-5)
           for (x=x1+4; x<=x2-3; x++) 
              if ((x+y)&1) XDrawPoint(dpy, win, 
			              Context->gdata->wingc, x, y);
       for (y=y1+2; y<=y2-3; y++)
           for (x=x1+4; x<=x2-3; x+=x2-x1-7) 
              if ((x+y)&1) XDrawPoint(dpy, win, 
			              Context->gdata->wingc, x, y);
       return;
    }

    drawBox(win, Context->gdata->wingc, 
            Context->gdata->pixel, x1, y1, x2, y2, clicked);

    d = (5*Context->gdata->charspace)/12;
    XSetBackground(dpy, Context->gdata->wingc, 
		   Context->gdata->pixel[BUTTONCOLOR]);

    if (win==Menu && do_dock && (s[0]==c[0]) && index(WeakChars,c[0]))
        XSetForeground(dpy, Context->gdata->wingc, 
                            Context->gdata->pixel[WEAKCOLOR]);
    else
        XSetForeground(dpy, Context->gdata->wingc, 
                            Context->gdata->pixel[MENUFGCOLOR]);

    XDrawImageString(dpy, win, Context->gdata->wingc, 
                     x1+d, 
                     y1+Context->gdata->font[MENUFONT]->max_bounds.ascent + 4, 
                     s, strlen(s));
}

int
getNumCmd(key)
char key;
{
     int i;
     for (i=0; i<N_HELP; i++) 
	 if (key==CommandKey[i]) return i;
     return -1;	
}

void
BasicSettings(Context)
Sundata * Context;
{
        XSetBackground(dpy, Context->gdata->wingc, Context->gdata->pixel[MENUBGCOLOR]);
        XSetForeground(dpy, Context->gdata->wingc, Context->gdata->pixel[MENUFGCOLOR]);
        XSetFont(dpy, Context->gdata->wingc, Context->gdata->font[MENUFONT]->fid);
}

void
helpHint(Context, key, hint)
Sundata * Context;
char key;
char *hint;
{
	int i,j,k,l;
        char more[128], prog_str[60];

	if (key=='\033')
           sprintf(hint, " %s", Label[L_ESCMENU]); 
	else
	if (key==' ')
           sprintf(hint, " %s", Label[L_CONTROLS]); 
	else {
	   i = getNumCmd(key);
	   if (i>=0) 
              sprintf(hint, " %s", Help[i]); 
	   else
	      sprintf(hint, " %s", Label[L_UNKNOWN]);
        }

        more[0] = more[1] = ' ';
        more[2] = '\0';

	if (index("CDS", key))
	   sprintf(more, " (%s)", Label[L_DEGREE]);
	if (index("ABGJ'", key)) {
           switch(Context->flags.progress) {
              case 0: sprintf(prog_str, "1 %s", Label[L_MIN]); break;
	      case 1: sprintf(prog_str, "1 %s", Label[L_HOUR]); break;
	      case 2: sprintf(prog_str, "1 %s", Label[L_DAY]); break;
	      case 3: sprintf(prog_str, "7 %s", Label[L_DAYS]); break;
	      case 4: sprintf(prog_str, "30 %s", Label[L_DAYS]); break;
      	      case 5: sprintf(prog_str, "%ld %s", 
                          progress_value[5], Label[L_SEC]); break;
           }
           sprintf(more, " ( %s %s%s   %s %.3f %s )", 
		  Label[L_PROGRESS], 
                  (key=='A')? "+":((key=='B')? "-":""), prog_str, 
                  Label[L_TIMEJUMP], Context->jump/86400.0,
		  Label[L_DAYS]);
	}
	if (key == 'H') {
	    sscanf(RELEASE, "%d %d %d", &i, &j, &k);
	    sprintf(more+1, "(%s %s, %d %s %d, %s)", 
                      ProgName, VERSION, i, Month_name[j-1], k, COPYRIGHT);
	}
	if (key == 'X') {
	    more[1] = ':';
            more[2] = ' ';
	    if (ExternAction)
	       strncpy(more+3, ExternAction, 123);
            else
	       strcpy(more, "(null)");
            more[126] = '0';
	}

	if (key == '=')
	    sprintf(more+2, "(%c)", (do_sync)? '+' : '-');
	if (key == '[' && do_root <= 0)
	    sprintf(more+2, Label[L_ONCE]);
	if (key == '[' && do_root >= 1)
	    sprintf(more+2, Label[L_PERIODIC], root_period);
	if (key == ']' && do_root <= 0)
	    sprintf(more+2, Label[L_BLANKSCREEN]);
	if (key == ']' && do_root >= 1)
	    sprintf(more+2, Label[L_STARRYSKY]);
        if (more[2])
	   strncat(hint, more, 120 - strlen(hint));
        l = strlen(hint);
	if (l<120)
	    for (i=l; i<120; i++) hint[i] = ' ';
	hint[120] = '\0';
}

void
showMenuHint(but_pos)
int but_pos;
{
        static int move_pos = -1;
        char key;
	char hint[128];

	if (!do_menu) return;
	if (!MenuCaller) return;

        if (menu_newhint == XK_Shift_L || menu_newhint == XK_Shift_R) return;
	key = toupper((char) menu_newhint);

        if (move_pos>=0) drawButton(Menu, move_pos, -2);
        if (but_pos>=0) drawButton(Menu, but_pos, -1);
	move_pos = but_pos;

	if (key == menu_lasthint) return;
	helpHint(MenuCaller, key, hint);

        menu_lasthint = key;
        BasicSettings(MenuCaller);
	XDrawImageString(dpy, Menu, MenuCaller->gdata->wingc, 4, 
              MenuCaller->gdata->font[MENUFONT]->max_bounds.ascent + 
                 MenuCaller->gdata->menustrip + 4, 
              hint, strlen(hint));
}

void
setupMenu(mode)
int mode;
{
	int i, l;
        static int j=-1;
        static char c;

	if (!do_menu) return;

        if (j>=0) WeakChars[j] = c;
	l = strlen(WeakChars);

        if (MenuCaller->wintype) {
	   if (do_dock) j = 1; else j = 0;
	} else {
	   j = -1;
	   if (do_dock) {
 	      if (MenuCaller != Seed) j = l-4;
	   } else j = l-5;
	}
        if (j>=0) {
	   c = WeakChars[j] ;
           WeakChars[j] = '\0';
	}

        BasicSettings(MenuCaller);
        XSetWindowColormap(dpy, Menu, MenuCaller->gdata->cmap);
        XSetWindowBackground(dpy, Menu, MenuCaller->gdata->pixel[MENUBGCOLOR]);
        XClearArea(dpy, Menu,  0, 0, MenuGeom.width, MenuGeom.height, False);

	for (i=0; i<=N_MENU; i++)
            drawButton(Menu, i, 0);
        menu_lasthint = '\0';
	showMenuHint(-1);
}

void
PopMenu(Context)
struct Sundata * Context;
{
	int    w, h, a, b, x=0, y=0;
	
	do_menu = 1 - do_menu;

        if (!do_menu) 
	  {
	  XDestroyWindow(dpy, Menu);
	  Menu = 0;
	  MenuCaller = NULL;
	  return;
	  }

        if (!Menu)
           Menu = newWindow(NULL, &MenuGeom, 2);

	XSelectInput(dpy, Menu, 0);
        MenuCaller = Context;

        MenuGeom.width = MENU_WIDTH * Context->gdata->menustrip - 6;
        MenuGeom.height = 2 * Context->gdata->menustrip;
   
	if (!getPlacement(Context->win, &Context->geom.x, 
                                        &Context->geom.y, &w, &h)) {
	   x = Context->geom.x + MenuGeom.x - horiz_drift - 5;
	   a = Context->geom.y + h + 6; 

           b = Context->geom.y - MenuGeom.height - MenuGeom.y - 2*vert_drift - 28;
           if (b < TOPTITLEBARHEIGHT ) b = TOPTITLEBARHEIGHT;
           if (a > (int) DisplayHeight(dpy,scr) 
                   - 2*MenuGeom.height -vert_drift -20)
              a = b;
	   y = (placement<=NE)? a : b;
	}
        setSizeHints(NULL, 2);
        XMoveWindow(dpy, Menu, x, y);
        XMapWindow(dpy, Menu);
        XMoveWindow(dpy, Menu, x, y);
        XFlush(dpy);
	usleep(2*TIMESTEP);
	menu_lasthint = ' ';
	setupMenu(-1);
        setProtocols(NULL, 2);
}

void 
processMenuAction(Context, x, y, button, evtype)
Sundata * Context;
int x, y, button, evtype;
{
static int click_pos = -1;
int but_pos;
KeySym key;
	   if (evtype == ButtonRelease && click_pos>=0) {
	       drawButton(Menu, click_pos, 0);
               click_pos = -1;
	   }
           but_pos = getButton(Menu, x, y);
           if (evtype == ButtonPress && but_pos>=0 && but_pos<=N_MENU) {
	       drawButton(Menu, but_pos, 1);
	       click_pos = but_pos;
               return;
	   }
           if (evtype == MotionNotify) {
	     if (but_pos >= N_MENU) {
		menu_newhint = XK_Escape;
                showMenuHint(N_MENU);
		return;
	     }
	     if (but_pos <= -1) 
		menu_newhint = ' ';
	     else
	        menu_newhint = MenuKey[2*but_pos];
             showMenuHint(getNumCmd(menu_newhint));
             return;
           }
	   if (but_pos==-1) return;	   
           if (do_menu && but_pos >= N_MENU) {
              PopMenu(MenuCaller);
              return;
           }
           key = MenuKey[2*but_pos];
	   if (button<=2) key = tolower(key);
           processKey(Menu, (KeySym)key);
}

void
clearFileselPartially()
{
    XSetWindowBackground(dpy, Filesel, FileselCaller->gdata->pixel[MENUBGCOLOR]);
    XClearArea(dpy, Filesel, 0, FileselCaller->gdata->menustrip+1, 
        FileselGeom.width-2, FileselGeom.height-FileselCaller->gdata->menustrip-2, False);
}

void
setupFilesel(mode)
int mode;
{
	int i, j, b, d, p, w, h, ht, skip;
	char *s, *sp;

        if (!do_filesel) return;

        XSetWindowColormap(dpy, Filesel, FileselCaller->gdata->cmap);
        XSetWindowBackground(dpy, Filesel, 
           FileselCaller->gdata->pixel[MENUBGCOLOR]);

	d = FileselCaller->gdata->charspace/3;
        h = FileselCaller->gdata->menustrip;

	if (mode < 0) {
          BasicSettings(FileselCaller);
          XClearArea(dpy, Filesel,  0, 0, 
             FileselGeom.width, FileselGeom.height, False);
	  for (i=0; i<=6; i++)
	     drawButton(Filesel, i, 0);
	}
	
        BasicSettings(FileselCaller);

        if (mode <= 0) {
            XClearArea(dpy, Filesel,  0, FileselCaller->gdata->menustrip+1,
                FileselGeom.width - 15, FileselGeom.height, False);
            XClearArea(dpy, Filesel,  
                FileselGeom.width - 15, FileselCaller->gdata->menustrip+1,
                13, FileselCaller->gdata->menustrip, False);
            XSetForeground(dpy,  FileselCaller->gdata->wingc,
                FileselCaller->gdata->pixel[MENUFGCOLOR]);
            XDrawImageString(dpy, Filesel, FileselCaller->gdata->wingc,
                d, FileselCaller->gdata->font[MENUFONT]->max_bounds.ascent + 
                   FileselCaller->gdata->menustrip+4, 
                image_dir, strlen(image_dir));

            h = 2*FileselCaller->gdata->menustrip;
            XDrawLine(dpy, Filesel, FileselCaller->gdata->wingc, 
                0, h, FileselGeom.width, h);

            XDrawLine(dpy, Filesel, FileselCaller->gdata->wingc, 
                FileselGeom.width - 15, 2*FileselCaller->gdata->menustrip, 
                FileselGeom.width - 15, FileselGeom.height);
            /* Drawing small triangular icons */
            w = FileselGeom.width - 7;
            h = 2 * FileselCaller->gdata->menustrip + 1;
            for (i=0; i<=10; i++)
	       XDrawLine(dpy, Filesel, FileselCaller->gdata->wingc, 
                     w-i/2, h+i, w+i/2, h+i);
            h = FileselGeom.height-1;
            for (i=0; i<=10; i++)
               XDrawLine(dpy, Filesel, FileselCaller->gdata->wingc, 
                     w-i/2, h-i, w+i/2, h-i);
	}

	if (mode<=1)
           XClearArea(dpy, Filesel,  0, 2*FileselCaller->gdata->menustrip+1,
               FileselGeom.width - 15, FileselGeom.height, False);

	if (!dirtable) 
	   dirtable = get_dir_list(image_dir, &num_table_entries);
	if (dirtable)
           qsort(dirtable, num_table_entries, sizeof(char *), dup_strcmp);
	else {
	   char error[] = "Directory inexistent or inaccessible !!!";
	   XSetForeground(dpy, FileselCaller->gdata->wingc, 
                               FileselCaller->gdata->pixel[IMAGECOLOR]);
           XDrawImageString(dpy, Filesel, 
                     FileselCaller->gdata->wingc, d, 
                     3*FileselCaller->gdata->menustrip,
		     error, strlen(error));
	   return;
	}

	skip = (3*FileselCaller->gdata->menustrip)/4;
	num_lines = (FileselGeom.height-2*FileselCaller->gdata->menustrip)/skip;
        /* drawing the thumb */
        XSetForeground(dpy, FileselCaller->gdata->wingc,
           FileselCaller->gdata->pixel[ZOOMBGCOLOR]);
        XFillRectangle(dpy, Filesel, FileselCaller->gdata->wingc, 
           FileselGeom.width - 14, 2*FileselCaller->gdata->menustrip+12,
           14, FileselGeom.height-2*FileselCaller->gdata->menustrip-23);

        w = FileselGeom.width - 12;
        p = FileselGeom.height - 2 * FileselCaller->gdata->menustrip - 28;
        ht = p * (num_lines+1) / (num_table_entries+1);
        if (ht<20) ht = 20;
        if (ht>p/2) ht = p/2;
        p = p - ht;
        h = 2 * FileselCaller->gdata->menustrip + 14;
        if (num_table_entries>2)
           h += (filesel_shift * p)/(num_table_entries-2);
	drawBox(Filesel, FileselCaller->gdata->wingc, 
                FileselCaller->gdata->pixel, w, h, w+9, h+ht, 0);

        for (i=0; i<num_table_entries-filesel_shift; i++) 
	if (i<num_lines) {
	  s = dirtable[i+filesel_shift];
	  b = (s[strlen(s)-1]=='/');
          if (b==0) {
	    if (strstr(s,".gif") || strstr(s,".jpg") ||
		strstr(s,".png") || strstr(s,".xpm") ||
		strstr(s,".vmf"))
	       b=2;
	  }
	  j = FileselCaller->gdata->font[MENUFONT]->max_bounds.ascent + 
              2 * FileselCaller->gdata->menustrip + i*skip + 3;
	  sp = (FileselCaller->wintype)? 
	    FileselCaller->map_img_file : FileselCaller->clock_img_file;
	  if (strstr(sp,s)) {
	     if (mode<=3)
                XClearArea(dpy, Filesel, 2, 
	   /* FileselCaller->gdata->font[MENUFONT]->max_bounds.ascent+ */
                      2 * FileselCaller->gdata->menustrip + 1, 3, 
                   FileselGeom.height, False);
	     if (mode==3) {
  	        XSetForeground(dpy, FileselCaller->gdata->wingc, 
                               FileselCaller->gdata->pixel[CHANGECOLOR]);
                XDrawRectangle(dpy, Filesel, FileselCaller->gdata->wingc,
		  d/4, j-FileselCaller->gdata->font[MENUFONT]->max_bounds.ascent/2, 3,4);
	     } else {
  	        XSetForeground(dpy, FileselCaller->gdata->wingc, 
                               FileselCaller->gdata->pixel[CHOICECOLOR]);
                XFillRectangle(dpy, Filesel, FileselCaller->gdata->wingc,
                  d/4, j-FileselCaller->gdata->font[MENUFONT]->max_bounds.ascent/2, 3,4);
	     }
	  }
	  if (mode<=1) {
  	     XSetForeground(dpy, FileselCaller->gdata->wingc,
                (b==0)? FileselCaller->gdata->pixel[MENUFGCOLOR] :
                        FileselCaller->gdata->pixel[DIRCOLOR+b-1]);
             XDrawImageString(dpy, Filesel, FileselCaller->gdata->wingc, 
                d, j, s, strlen(s));
	  }
	}
}

void
PopFilesel(Context)
struct Sundata * Context;
{
        int a, b, w, h, x=0, y=0;

	if (do_filesel)
            do_filesel = 0;
	else
	    do_filesel = 1;

        if (!do_filesel) 
	  {
	  XDestroyWindow(dpy, Filesel);
	  Filesel = 0;
	  FileselCaller = NULL;
	  if (dirtable) free_dirlist(dirtable);
	  dirtable = NULL;
	  return;
	  }

        if (!Filesel)
           Filesel = newWindow(NULL, &FileselGeom, 3);

        XSelectInput(dpy, Filesel, 0);
	FileselCaller = Context;
	filesel_shift = 0;

	if (!getPlacement(Context->win, &Context->geom.x, 
                                        &Context->geom.y, &w, &h)) {
	   x = Context->geom.x + FileselGeom.x - horiz_drift - 5;
	   a = Context->geom.y + h + 6;
           if (do_menu && Context == MenuCaller) 
               a += MenuGeom.height + MenuGeom.y + vert_drift + 2;
           b = Context->geom.y - FileselGeom.height - FileselGeom.y - 2*vert_drift - 28;
           if (b < TOPTITLEBARHEIGHT ) b = TOPTITLEBARHEIGHT;
           if (a > (int) DisplayHeight(dpy,scr) 
                   - FileselGeom.height - vert_drift - 20)
               a = b;
	   y = (placement<=NE)? a : b;
	}

        setSizeHints(NULL, 3);
        XMoveWindow(dpy, Filesel, x, y);
        XMapRaised(dpy, Filesel);
        XMoveWindow(dpy, Filesel, x, y);
	XFlush(dpy);
	usleep(2*TIMESTEP);
	setupFilesel(-1);
        setProtocols(NULL, 3);
}

void 
processFileselAction(Context, x, y, evtype)
struct Sundata * Context;
int x;
int y;
int evtype;
{
	static int pressed3 = 0;
	static int move_pos = -1;
	static int click_pos = -1;
	static int but_pos;
        char newdir[1030];
	char *s, *f, *path;

        if (evtype == MotionNotify && move_pos>=0) {
	   drawButton(Filesel, move_pos, -2);
	   move_pos = -1;
	}
        if (evtype == ButtonRelease && click_pos>=0) {
	   drawButton(Filesel, click_pos, 0);
           click_pos = -1;
	}
  
        but_pos = getButton(Filesel, x, y);

	if (evtype == ButtonPress) {
           if (click_pos>=0) drawButton(Filesel, click_pos, 0);
	   if (but_pos>=0) drawButton(Filesel, but_pos, 1);
	   click_pos = but_pos;
	   pressed3 = 1;
	   return;
	}
	if (evtype == ButtonRelease) {
	   pressed3 = 0;
	}

	if (evtype == MotionNotify) {
	   if (but_pos>=0 && but_pos!=click_pos) {
	      drawButton(Filesel, but_pos, -1);
	      move_pos = but_pos;
	   }
           if (x < FileselGeom.width - 15 ||
               y <= 2 * Context->gdata->menustrip)
	      return;
	}
	
        if (y <= Context->gdata->menustrip) {
	  if (but_pos==0 && getenv("HOME"))
	     sprintf(image_dir, "%s/", getenv("HOME")); 
	  if (but_pos==1)
	     StringReAlloc(&image_dir, share_maps_dir);
	  if (but_pos==2)
	     StringReAlloc(&image_dir, "/"); 
	  if (but_pos==3 && getcwd(NULL,1024)) {
	     sprintf(newdir, "%s/", getcwd(NULL,1024));
	     StringReAlloc(&image_dir, newdir);
	  }
	  if (but_pos>=0 && but_pos<=3) {
	     filesel_shift = 0;
	     if (dirtable) {
	        free(dirtable);
	        dirtable = NULL;
	     }
	  }
	  if (but_pos==4) {	
	      do_filesel = -1;
              processKey(Context->win, XK_w);
	      return;
	  }
	  if (but_pos==5) {	
              processKey(Context->win, XK_exclam);
	      return;
	  }
	  if (but_pos>=6) {
	     XUnmapWindow(dpy, Filesel);
	     do_filesel = 0;
	     return;
	  }
	  setupFilesel(0);
	  return;
	}
        if (y <= 2*Context->gdata->menustrip) {
	  filesel_shift = 0;
	  setupFilesel(0);
	  return;
	}

	if (x > FileselGeom.width - 15) {
	   int old_shift = filesel_shift;
	   if (y <= 2*Context->gdata->menustrip + 10) {
	      if (evtype != ButtonRelease) return;
	      if (filesel_shift == 0) return;
	      filesel_shift -= num_lines/2;
	      if (filesel_shift <0) filesel_shift = 0;
	   } else
	   if (y >= FileselGeom.height - 10) {
	      if (evtype != ButtonRelease) return;
	      if (num_table_entries-filesel_shift<num_lines) return;
	      filesel_shift += num_lines/2;
	   } else {
	      if (evtype == MotionNotify && !pressed3) {
		 return;
	      }
	      filesel_shift = ( num_table_entries *
		 ( y - 2*Context->gdata->menustrip - 10 ))
                 / (FileselGeom.height - 2*Context->gdata->menustrip - 20);
	      if (filesel_shift > num_table_entries - num_lines/2) 
                 filesel_shift = num_table_entries - num_lines/2;
	      if (filesel_shift < 0) filesel_shift = 0;
	   }
	   if (filesel_shift != old_shift)
	      setupFilesel(1);
	   return;
	}

	y = (y-2*Context->gdata->menustrip-4)/(3*Context->gdata->menustrip/4)
            +filesel_shift;
	if (y<num_table_entries) {
	   s = dirtable[y];
	   if (s==NULL || *s=='\0') return;
	   if (x > XTextWidth (Context->gdata->font[MENUFONT], s, 
                   strlen(s))+Context->gdata->charspace/4) return;
	   y = strlen(s)-1;
	   f = (char *) salloc(strlen(image_dir)+y+2);
	   strcpy(f, image_dir);
           if (s[y] == '/') {
	      int l;
	      if (!strcmp(s, "../")) {
	        l=strlen(f)-1;
		if (l==0) return;
                f[l--] = '\0';
	        while (l>=0 && f[l] != '/')
		   f[l--] = '\0';
		s = "";
	      }
              strcat(f, s);
	      l=strlen(f);
              if (f[l-1] != '/') {
                 f[l] = 'l';
                 f[++l] = '\0';
	      }
	      if (dirtable) free_dirlist(dirtable);
	      dirtable = NULL;
	      filesel_shift = 0;
	      num_table_entries=0;
	      StringReAlloc(&image_dir, f);
	      free(f);
              setupFilesel(0);
	      return;
	   } else {
	      path = (Context->wintype)? 
                    Context->map_img_file : Context->clock_img_file;
	      f = (char *)
                salloc((strlen(image_dir)+strlen(s)+2)*sizeof(char));
	      sprintf(f, "%s%s", image_dir, s);
	      if (!path || strcmp(f, path)) {
 		 if (Context->wintype)
                    StringReAlloc(&Context->map_img_file, f);
		 else
		    StringReAlloc(&Context->clock_img_file, f);
		 setupFilesel(3);
		 adjustGeom(Context, 0);
	         shutDown(Context, 0);
	         buildMap(Context, Context->wintype, 0);
	      }
	      free(f);
	   }
	}
}

void 
checkZoomSettings(zoom)
ZoomSettings *zoom;
{
    if (zoom->fx<1.0) zoom->fx = 1.0;
    if (zoom->fx>100.0) zoom->fx = 100.0;

    if (zoom->fy<1.0) zoom->fy = 1.0;
    if (zoom->fy>100.0) zoom->fy = 100.0;

    if (zoom->fdx<0.0) zoom->fdx = 0.0;
    if (zoom->fdx>1.0) zoom->fdx = 1.0;

    if (zoom->fdy<0.01) zoom->fdy = 0.01;
    if (zoom->fdy>0.99) zoom->fdy = 0.99;
}

int setZoomAspect(Context, mode)
struct Sundata * Context;
int mode;
{
   double a, b, f, p;
   int change;

   checkZoomSettings(&Context->newzoom);

   if (memcmp(&Context->newzoom, &Context->zoom, sizeof(ZoomSettings)))
      Context->oldzoom = Context->zoom;

   if (!Context->newzoom.mode) return 0;

   a = Context->newzoom.fx;
   b = Context->newzoom.fy;
   change = 0;

   if (Context->newzoom.mode == 1)
      f = 1.0;
   else
      f = (double)Context->geom.width / 
          ((double)Context->geom.height *2.0 * sin(M_PI*Context->newzoom.fdy));

   if (mode == 3) {
      p = sqrt( Context->newzoom.fx * Context->newzoom.fy / f);
      Context->newzoom.fx = p;
      Context->newzoom.fy = p * f;
      f = 0;
      mode = 1;
   }

   if (mode == 1) {
      if (f == 0) 
          mode = 2;
      else
          Context->newzoom.fx = Context->newzoom.fy / f;
      if (Context->newzoom.fx < 1.0) {
	  Context->newzoom.fy = Context->newzoom.fy / Context->newzoom.fx;
	  Context->newzoom.fx = 1.0;
      }
      if (Context->newzoom.fx > 100.0) {
	  Context->newzoom.fy = 100.0 * Context->newzoom.fy / Context->newzoom.fx;
	  Context->newzoom.fx = 100.0;
      }
   }

   if (mode == 2) {
      if (f!=0) 
          Context->newzoom.fy = Context->newzoom.fx * f;
      if (Context->newzoom.fy < 1.0) {
	  Context->newzoom.fx = Context->newzoom.fx / Context->newzoom.fy;
	  Context->newzoom.fy = 1.0;
      }
      if (Context->newzoom.fy > 100.0) {
	  Context->newzoom.fx = 100.0 * Context->newzoom.fx / Context->newzoom.fy;
	  Context->newzoom.fy = 100.0;
      }
   }

   if (fabs(a-Context->newzoom.fx) > 1E-4) {
      zoom_mode |= 10;
      change = 1;
   } else
      Context->newzoom.fx = a;

   if (fabs(b-Context->newzoom.fy) > 1E-4) {
      zoom_mode |= 12;
      change = 1;
   } else
      Context->newzoom.fy = b;
    
   if (verbose && change)
      fprintf(stderr, "Adjusting zoom area aspect (%.2f , %.2f) --> "
            "(%.2f , %.2f)\n", a, b, Context->newzoom.fx , Context->newzoom.fy);

   return change;
}

void
setZoomDimension(Context)
struct Sundata * Context;
{
    double rx, ry;

    setZoomAspect(Context, 3);

    Context->newzoom.width = (int) 
       ((double) Context->geom.width * Context->newzoom.fx + 0.25);
    Context->newzoom.height = (int)
       ((double) Context->geom.height * Context->newzoom.fy + 0.25);

    rx = 0.5/Context->newzoom.fx;
    ry = 0.5/Context->newzoom.fy;
    Context->newzoom.dx = (int) 
       ((double) Context->newzoom.width * (Context->newzoom.fdx-rx) + 0.25);
    Context->newzoom.dy = (int)
       ((double) Context->newzoom.height * (Context->newzoom.fdy-ry) + 0.25);

    if (Context->newzoom.dx < 0) Context->newzoom.dx = 0;
    if (Context->newzoom.dy < 0) Context->newzoom.dy = 0;
    if (Context->newzoom.dx+Context->geom.width>Context->newzoom.width)
        Context->newzoom.dx = Context->newzoom.width - Context->geom.width;
    if (Context->newzoom.dy+Context->geom.height>Context->newzoom.height)
        Context->newzoom.dy = Context->newzoom.height - Context->geom.height;

    if (verbose && !button_pressed)
       fprintf(stderr, "Zoom (%.2f, %.2f)  centering at (%.2f, %.2f)\n",
	  Context->newzoom.fx, Context->newzoom.fy, 
          Context->newzoom.fdx, Context->newzoom.fdy);
}

void
showZoomHint(but_pos)
int but_pos;
{
        static int move_pos = -1;
	char hint[120];
	int v;

        if (move_pos>=0) drawButton(Zoom, move_pos, -2);
        if (but_pos>=0) drawButton(Zoom, but_pos, -1);
	move_pos = but_pos;

	if (!do_zoom || zoom_lasthint==zoom_newhint) return;

	zoom_lasthint = zoom_newhint;
	v = ZoomGeom.height - ZoomCaller->gdata->menustrip;

	if (zoom_newhint=='\033')
           strcpy(hint, Label[L_ESCMENU]);
	else
	if (zoom_newhint==' ')
           sprintf(hint, 
                "magx = %.3f, magy = %.3f,  lon = %.3f°, lat = %.3f°", 
                ZoomCaller->newzoom.fx, ZoomCaller->newzoom.fy,
	 	360.0 * ZoomCaller->newzoom.fdx - 180.0, 
                90.0 - ZoomCaller->newzoom.fdy*180.0);
        else
	   strcpy(hint, Help[getNumCmd(zoom_newhint)]);

	BasicSettings(ZoomCaller);
        XSetWindowBackground(dpy, Zoom, ZoomCaller->gdata->pixel[MENUBGCOLOR]);
	XClearArea(dpy, Zoom, 0, v+1, ZoomGeom.width, 
             ZoomCaller->gdata->menustrip-1, False);
	XDrawImageString(dpy, Zoom, ZoomCaller->gdata->wingc, 4, 
              v + ZoomCaller->gdata->font[MENUFONT]->max_bounds.ascent + 3,
              hint, strlen(hint));
}

void
PopZoom();

void
setupZoom(mode)
int mode;
{
    int i, j, k;
    int zoomx, zoomy, zoomw, zoomh;
    char *num[] = { "1", "2", "5", "10", "20", "50", "100"};
    char *synchro = Label[L_SYNCHRO];
    char s[80];

    if (!do_zoom) return;

    BasicSettings(ZoomCaller);
    XSetWindowColormap(dpy, Zoom, ZoomCaller->gdata->cmap);
    XSetWindowBackground(dpy, Zoom, ZoomCaller->gdata->pixel[MENUBGCOLOR]);

    areaw = ZoomGeom.width - 74;
    areah = ZoomGeom.height - 2*ZoomCaller->gdata->menustrip - 65;

    if (mode == -1) {
       XClearArea(dpy, Zoom,  0,0, ZoomGeom.width, ZoomGeom.height, False);
       XDrawImageString(dpy, Zoom, ZoomCaller->gdata->wingc, 
          160-XTextWidth(ZoomCaller->gdata->font[MENUFONT], 
          synchro, strlen(synchro))+areah, 
          24+ ZoomCaller->gdata->font[MENUFONT]->max_bounds.ascent, 
          synchro, strlen(synchro));

       for (i=0; i<=N_ZOOM; i++)
	  drawButton(Zoom, i, 0);

       BasicSettings(ZoomCaller);

       for (i=0; i<=6; i++) {
          j = 63 + (int) ( (areah-6)*log(atof(num[i]))/log(100.0));
          k = j - ZoomCaller->gdata->charspace*strlen(num[i])/10;
          XDrawImageString(dpy, Zoom, ZoomCaller->gdata->wingc, k,
             ZoomCaller->gdata->font[MENUFONT]->max_bounds.ascent + 3, 
             num[i], strlen(num[i]));
          k = j + ZoomCaller->gdata->font[MENUFONT]->max_bounds.ascent/2 - 10;
          XDrawImageString(dpy, Zoom, ZoomCaller->gdata->wingc,
                24-ZoomCaller->gdata->charspace*strlen(num[i])/4, k , 
                num[i], strlen(num[i]));
          XDrawLine(dpy, Zoom, ZoomCaller->gdata->wingc, j, 20, j, 24);
          XDrawLine(dpy, Zoom, ZoomCaller->gdata->wingc, 30, j-10, 34, j-10);
       }

       XDrawLine(dpy, Zoom, ZoomCaller->gdata->wingc, 60, 22, areah+60, 22);
       XDrawLine(dpy, Zoom, ZoomCaller->gdata->wingc, 32, 50, 32, areah+50);
    }

    XSetWindowBackground(dpy, Zoom, ZoomCaller->gdata->pixel[ZOOMBGCOLOR]);
    if ((mode & 1) && !ZoomCaller->newzoom.mode)
       XClearArea(dpy, Zoom,  41, 31, 9, 9, False);
    if (mode & 2)
       XClearArea(dpy, Zoom,  61, 31, areah, 9, False);
    if (mode & 4)
       XClearArea(dpy, Zoom,  41, 51, 9, areah, False);

    if (!zoompix) {
       int oldlevel, oldfill, code, num;

       oldlevel = ZoomCaller->flags.colorlevel;
       oldfill = ZoomCaller->flags.fillmode;
       ZoomCaller->flags.colorlevel = MONOCHROME;
       ZoomCaller->flags.fillmode = 1;
       i = ZoomCaller->geom.width;
       j = ZoomCaller->geom.height;
       ZoomCaller->newzoom = ZoomCaller->zoom;
       ZoomCaller->zoom.width = ZoomCaller->geom.width = areaw;
       ZoomCaller->zoom.height = ZoomCaller->geom.height = areah;
       ZoomCaller->zoom.dx = ZoomCaller->zoom.dy = 0;

       code = readVMF(Zoom_img_file, ZoomCaller);
       if (code) {
	  if (ZoomCaller->bits) free(ZoomCaller->bits);
	  ZoomCaller->bits = NULL;
       }
       if (!ZoomCaller->bits) {
          num = ((ZoomCaller->geom.width+7)/8)*
	                       ZoomCaller->geom.height*sizeof(char);
	  ZoomCaller->bits = (char *) salloc(num);
       }
       if (code && ZoomCaller->bits)
	  memset(ZoomCaller->bits, 0xFF, num);
       if (ZoomCaller->bits) {
          zoompix = XCreatePixmapFromBitmapData(dpy, Root,
                     ZoomCaller->bits, ZoomCaller->geom.width,
                     ZoomCaller->geom.height, 0, 1, 1);
          free(ZoomCaller->bits);
       }
       
       ZoomCaller->zoom = ZoomCaller->newzoom;
       ZoomCaller->geom.width = i;
       ZoomCaller->geom.height = j;
       ZoomCaller->flags.colorlevel = oldlevel;
       ZoomCaller->flags.fillmode = oldfill;
    }

    XSetWindowBackground(dpy, Zoom, ZoomCaller->gdata->pixel[CHOICECOLOR]);
    XSetBackground(dpy, ZoomCaller->gdata->wingc, ZoomCaller->gdata->pixel[CHOICECOLOR]);
    XSetForeground(dpy, ZoomCaller->gdata->wingc, ZoomCaller->gdata->pixel[CHANGECOLOR]);

    if (mode & 2) {
       i = (int) ( (double)(areah-6)*log(ZoomCaller->zoom.fx)/log(100.00));
       XClearArea(dpy, Zoom,  61+i, 31, 6, 9, False);
       if (ZoomCaller->newzoom.fx != ZoomCaller->zoom.fx) {
         i = (int) ((double)(areah-6)*log(ZoomCaller->newzoom.fx)/log(100.00));
         XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->wingc, 61+i, 31, 5, 8);
       }
    }
    if (mode & 4) {
       j = (int) ( (double)(areah-6)*log(ZoomCaller->zoom.fy)/log(100.0));
       XClearArea(dpy, Zoom,  41, 51+j, 9, 6, False);
       if (ZoomCaller->newzoom.fy != ZoomCaller->zoom.fy) {
         j = (int) ((double)(areah-6)*log(ZoomCaller->newzoom.fy)/log(100.0));
         XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->wingc, 41, 51+j, 8, 5);
       }
    }

    if (mode & 8) {
       zoomw = (areaw*ZoomCaller->geom.width)/ZoomCaller->zoom.width;
       zoomh = (areah*ZoomCaller->geom.height)/ZoomCaller->zoom.height;
       zoomx = (areaw*ZoomCaller->zoom.dx)/ZoomCaller->zoom.width;
       zoomy = (areah*ZoomCaller->zoom.dy)/ZoomCaller->zoom.height;
       i = areaw-zoomx-zoomw-1;
       j = areah-zoomy-zoomh-1;

       XSetBackground(dpy, ZoomCaller->gdata->wingc, 
                      ZoomCaller->gdata->pixel[ZOOMBGCOLOR]);
       XSetForeground(dpy, ZoomCaller->gdata->wingc, 
                      ZoomCaller->gdata->pixel[ZOOMFGCOLOR]);
       
       if (zoomy)
          XCopyPlane(dpy, zoompix, Zoom, ZoomCaller->gdata->wingc, 
                0, 0, areaw-1, zoomy, 61, 51, 1);
       if (j>0)
          XCopyPlane(dpy, zoompix, Zoom, ZoomCaller->gdata->wingc, 
                0, zoomy+zoomh+1, areaw, j, 61, 52+zoomy+zoomh, 1);
       if (zoomx)
          XCopyPlane(dpy, zoompix, Zoom, ZoomCaller->gdata->wingc, 
                0, 0, zoomx, areah-1, 61, 51, 1);
       if (i>0)
          XCopyPlane(dpy, zoompix, Zoom, ZoomCaller->gdata->wingc, 
                zoomx+zoomw+1, 0, i, areah, 62+zoomx+zoomw, 51, 1);

       if (ZoomCaller->flags.colorlevel == MONOCHROME) {
          XSetBackground(dpy, ZoomCaller->gdata->wingc, 
                      ZoomCaller->gdata->pixel[ZOOMFGCOLOR]);
          XSetForeground(dpy, ZoomCaller->gdata->wingc, 
                      ZoomCaller->gdata->pixel[ZOOMBGCOLOR]);
       } else
          XSetBackground(dpy, ZoomCaller->gdata->wingc, 
                      ZoomCaller->gdata->pixel[CHOICECOLOR]);

       XCopyPlane(dpy, zoompix, Zoom, ZoomCaller->gdata->wingc, 
                  zoomx, zoomy, zoomw+1, zoomh+1, 61+zoomx, 51+zoomy, 1);

       if (ZoomCaller->newzoom.fx!=ZoomCaller->zoom.fx || 
           ZoomCaller->newzoom.fy!=ZoomCaller->zoom.fy || 
           ZoomCaller->newzoom.fdx!=ZoomCaller->zoom.fdx || 
           ZoomCaller->newzoom.fdy!=ZoomCaller->zoom.fdy) {
          zoomw = (areaw*ZoomCaller->geom.width)/ZoomCaller->newzoom.width;
          zoomh = (areah*ZoomCaller->geom.height)/ZoomCaller->newzoom.height;
          zoomx = (areaw*ZoomCaller->newzoom.dx)/ZoomCaller->newzoom.width;
          zoomy = (areah*ZoomCaller->newzoom.dy)/ZoomCaller->newzoom.height;
	  if (ZoomCaller->flags.colorlevel==MONOCHROME)
	     drawDottedRectangle(dpy, Zoom, ZoomCaller->gdata->wingc,
		61+zoomx, 51+zoomy, zoomw, zoomh,
		ZoomCaller->gdata->pixel[ZOOMFGCOLOR],
                ZoomCaller->gdata->pixel[ZOOMBGCOLOR]);
	  else {
             XSetForeground(dpy, ZoomCaller->gdata->wingc,
		   ZoomCaller->gdata->pixel[CHANGECOLOR]);
             XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->wingc,
                61+zoomx, 51+zoomy, zoomw, zoomh);
	  }
       }

       i = 60 + (int) (areaw * ZoomCaller->newzoom.fdx);
       j = 50 + (int) (areah * ZoomCaller->newzoom.fdy);

       XSetForeground(dpy, ZoomCaller->gdata->wingc, ZoomCaller->gdata->pixel[CHANGECOLOR]);
       if (i<60+areaw && j<50+areah)
          XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->wingc, i, j, 1, 1);

       XSetForeground(dpy, ZoomCaller->gdata->wingc, ZoomCaller->gdata->pixel[MENUFGCOLOR]);
       XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->wingc, 60, 50, areaw+1, areah+1);
    }

    XSetWindowBackground(dpy, Zoom, ZoomCaller->gdata->pixel[MENUBGCOLOR]);
    XSetBackground(dpy, ZoomCaller->gdata->wingc, ZoomCaller->gdata->pixel[MENUBGCOLOR]);
    XSetForeground(dpy, ZoomCaller->gdata->wingc, ZoomCaller->gdata->pixel[MENUFGCOLOR]);

    if (mode & 1) {
       XClearArea(dpy, Zoom,  33, 23, 17, 17, False);
       s[0] = '0'+ZoomCaller->newzoom.mode;
       s[1] = '\0';
       XDrawImageString(dpy, Zoom, ZoomCaller->gdata->wingc,
	  39, 25+ZoomCaller->gdata->font[MENUFONT]->max_bounds.ascent, s, 1);
    }

    if (mode & 16) {
       drawButton(Zoom, N_ZOOM+1, 0);
    }

    XSetForeground(dpy, ZoomCaller->gdata->wingc, ZoomCaller->gdata->pixel[MENUFGCOLOR]);

    if (mode == -1) {
       XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->wingc, 60, 30, areah+1, 10);
       XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->wingc, 40, 50, 10, areah+1);
       XDrawRectangle(dpy, Zoom, ZoomCaller->gdata->wingc, 32, 22, 18, 18);
       drawButton(Zoom, N_ZOOM+1, 0);
    }

    zoom_lasthint = '\0';
    showZoomHint(-1);
}

void
PopZoom(Context)
struct Sundata * Context;
{
        int a, b, w, h, x=0, y=0;

	if (do_zoom) {
	    XDestroyWindow(dpy, Zoom);
	    Zoom = 0;
	    ZoomCaller = NULL;
	    zoom_active = 1;
            do_zoom = 0;
	    if (zoompix) {
	       XFreePixmap(dpy, zoompix);
	       zoompix = 0;
	    }
	    return;
	} else
	    do_zoom = 1;

        if (!Zoom)
           Zoom = newWindow(NULL, &ZoomGeom, 4);

        Context->newzoom = Context->zoom;
        XSelectInput(dpy, Zoom, 0);

	ZoomCaller = Context;
	zoom_shift = 0;
	zoom_mode = 0;
	zoom_active = do_zoomsync;
	zoom_newhint = ' ';

	if (!getPlacement(Context->win, &Context->geom.x, &Context->geom.y, &w, &h)) {
	   x = Context->geom.x + ZoomGeom.x - horiz_drift - 5;
	   a = Context->geom.y + h + 6;
           if (do_menu && Context == MenuCaller) 
               a += MenuGeom.height + MenuGeom.y + vert_drift + 2;
           b = Context->geom.y - ZoomGeom.height - ZoomGeom.y - 2*vert_drift - 28;
           if (b < TOPTITLEBARHEIGHT ) b = TOPTITLEBARHEIGHT;
           if (a > (int) DisplayHeight(dpy,scr) 
                   - ZoomGeom.height - vert_drift -20)
              a = b;
	   y = (placement<=NE)? a : b;              
	}

        setSizeHints(NULL, 4);
        XMoveWindow(dpy, Zoom, x, y);
        XMapRaised(dpy, Zoom);
        XMoveWindow(dpy, Zoom, x, y);
        XFlush(dpy);
	usleep(2*TIMESTEP);
	option_lasthint = '\0';
        setupZoom(-1);
        setProtocols(NULL, 4);
}

void
activateZoom(Context, rebuild)
Sundata *Context;
int rebuild;
{ 
    setZoomDimension(Context);
    zoom_newhint = ' ';
    setupZoom(zoom_mode);
    if (rebuild) {
       if (Context->zoom.width!=Context->newzoom.width ||
	   Context->zoom.height!=Context->newzoom.height ||
	   Context->zoom.dx!=Context->newzoom.dx ||
	   Context->zoom.dy!=Context->newzoom.dy) {
          Context->zoom = Context->newzoom;
          shutDown(Context, 0);
          buildMap(Context, Context->wintype, 0);
	  zoom_newhint = ' ';
          setupZoom(zoom_mode);
       } else
          Context->zoom = Context->newzoom;
    }
    if (rebuild>0)
       zoom_mode = 0;
}

void processZoomAction(Context, x, y, button, evtype)
Sundata * Context;
int x;
int y;
int button;
int evtype;
{
           static int click_pos=-1;
           double v1, v2;
	   int but_pos;

           if (evtype == ButtonRelease && click_pos>=0) {
	       drawButton(Zoom, click_pos, 0);
               click_pos = -1;
  	   } 

	   if (y>areah+63 && y<areah+63+Context->gdata->menustrip) {
	      but_pos = getButton(Zoom, x, y);
	      if (but_pos>=N_ZOOM)
		 zoom_newhint = '\033';
	      else
	      if (but_pos>=0)
		 zoom_newhint = ZoomKey[2*but_pos];
	      if (evtype==MotionNotify) {
	         if (but_pos>=0)
		    showZoomHint(but_pos);
	      }
              if (evtype==ButtonPress && but_pos>=0) {
	         drawButton(Zoom, but_pos, 1);
		 click_pos  = but_pos;
	      }
	      if (evtype==ButtonRelease) {
		 if (zoom_newhint == '\033') {
		    PopZoom(Context);
		    return;
		 }
		 if (zoom_newhint == 'W') do_zoom = -1;
		 processKey(Context->win, tolower(zoom_newhint));
		 showZoomHint(but_pos);
		 zoom_mode = 0;
	      }
	      return;
	   }
	   if (button_pressed>=2) return;

           if (evtype==ButtonRelease)
	       drawButton(Zoom, N_ZOOM+1, 0);
           if (evtype==MotionNotify)
	       drawButton(Zoom, N_ZOOM+1, -2);

	   if (x>areah+170 && x<areah+170+Context->gdata->charspace && 
               y>21 && y<39) {
               if (evtype==MotionNotify) {
		  drawButton(Zoom, N_ZOOM+1, -1);
		  zoom_newhint = '"';
		  showZoomHint(getNumCmd(zoom_newhint));
	       }
               if (evtype==ButtonPress) {
		  drawButton(Zoom, N_ZOOM+1, 1);
	       }
               if (evtype==ButtonRelease) {
	          zoom_active = 1 -zoom_active;
	          zoom_mode |= 16;
	       }
	   } else
	   if (x>=60 && x<=areah+60 && y>=30 && y<=40 && button_pressed) {
              v1 = exp((double)(x-66)*log(100.00)/(double)(areah-6));
	      if (v1 != Context->newzoom.fx) {
		 Context->newzoom.fx = v1;
		 zoom_mode |= 10;
                 (void) setZoomAspect(Context, 2);
	      }
	   } else
	   if (x>=40 && x<=50 && y>=50 && y<=areah+50 && button_pressed) {
              v2  = exp((double)(y-53)*log(100.00)/(double)(areah-6));
	      if (v2 != Context->newzoom.fy) {
		 Context->newzoom.fy = v2;
		 zoom_mode |= 12;
                 (void) setZoomAspect(Context, 1);
	      }
	   }
           else
	   if (x>=60 && y>=50 && x<=60+areaw && y<=50+areah && button_pressed){
              v1 = ((double)(x-60))/((double)areaw);
              v2 = ((double)(y-50))/((double)areah);
              if (v1!=Context->newzoom.fdx || v2!=Context->newzoom.fdy) {
		 Context->newzoom.fdx = v1;
		 Context->newzoom.fdy = v2;
	         zoom_mode |= 8;
	      }
	      (void) setZoomAspect(Context, 3);
	   }
	   else {
	     zoom_newhint = ' ';
	     showZoomHint(-1);
	     if (button_pressed) return;
	   }

	   if (zoom_mode>0) {
              setZoomDimension(Context);
	      if (zoom_active && !button_pressed &&
                  memcmp(&Context->newzoom, 
                         &Context->zoom, sizeof(ZoomSettings))) {
		 button_pressed = 0;
		 activateZoom(Context, -1);
	      } else
	         setupZoom(zoom_mode);
 	      if (!button_pressed) zoom_mode = 0;
	   }
}

void
showOptionHint(but_pos)
int but_pos;
{
        static int move_pos = -1;
	char hint[128];
	int l, v;

        if (move_pos>=0) drawButton(Option, move_pos, -2);
        if (but_pos>=0) drawButton(Option, but_pos, -1);
	move_pos = but_pos;

	if (!do_option || option_lasthint==option_newhint) return;

	*hint = '\0';

	option_lasthint = option_newhint;
	v = OptionGeom.height - OptionCaller->gdata->menustrip;

	if (option_newhint=='\033')
           strcpy(hint, Label[L_ESCMENU]);
	else
	if (option_newhint==' ')
	   strcpy(hint, Label[L_OPTIONINTRO]);
	else
	if (option_newhint=='\n')
           strcpy(hint, Label[L_ACTIVATE]);
	else
	if (option_newhint=='?')
           strcpy(hint, Label[L_INCORRECT]);
	else {
	   if (option_newhint == 'G' || option_newhint == 'J') 
              option_lasthint = ' ';
	   helpHint(OptionCaller, option_newhint, hint);
	}

	l = strlen(hint);

	BasicSettings(OptionCaller);
        XSetWindowBackground(dpy, Option, OptionCaller->gdata->pixel[MENUBGCOLOR]);

	XClearArea(dpy, Option, 0, v+1, OptionGeom.width, 
             OptionCaller->gdata->menustrip-1, False);

        XDrawImageString(dpy, Option, OptionCaller->gdata->wingc, 
              4, v + OptionCaller->gdata->font[MENUFONT]->max_bounds.ascent + 3,
              hint, l);
}

void 
showCaret(Context, win, entry, x, y, mode)
Sundata * Context;
Window win;
TextEntry *entry;
int x, y, mode;
{
int i, j;

    i = XTextWidth(Context->gdata->font[MENUFONT], entry->string, entry->caret);
    j = XTextWidth(Context->gdata->font[MENUFONT], "_", 1);

    XSetWindowBackground(dpy, win, 
          Context->gdata->pixel[(mode)? CARETCOLOR : OPTIONBGCOLOR]);
    XClearArea(dpy, win, 
          x+i, y+Context->gdata->font[MENUFONT]->max_bounds.ascent+ 
          Context->gdata->menustrip/3 - 1, j, 2, False);
}

void
setupOption(mode)
int mode;
{
    int i, vskip;
    char s[80];

    if (!do_option) return;

    BasicSettings(OptionCaller);
    XSetWindowColormap(dpy, Option, OptionCaller->gdata->cmap);
    XSetWindowBackground(dpy, Option, OptionCaller->gdata->pixel[MENUBGCOLOR]);

    vskip = 3*OptionCaller->gdata->menustrip/8;
    option_lasthint = '\0';

    if (mode == -1) {
       XClearArea(dpy, Option, 0,0, OptionGeom.width,
                                    OptionGeom.height, False);
       for (i=0; i<=N_OPTION; i++)
	   drawButton(Option, i, 0);

       XSetBackground(dpy, OptionCaller->gdata->wingc, 
                      OptionCaller->gdata->pixel[MENUBGCOLOR]);
       XSetForeground(dpy, OptionCaller->gdata->wingc, 
                      OptionCaller->gdata->pixel[MENUFGCOLOR]);
       strcpy(s, Label[L_OPTION]);
       XDrawImageString(dpy, Option, OptionCaller->gdata->wingc, 
	       8, OptionCaller->gdata->font[MENUFONT]->max_bounds.ascent + vskip + 3,
               s, strlen(s));
       XDrawRectangle(dpy, Option, OptionCaller->gdata->wingc,
                           70, vskip, OptionGeom.width-85, 
                           OptionCaller->gdata->menustrip);
    }  

    XSetWindowBackground(dpy, Option,
       OptionCaller->gdata->pixel[OPTIONBGCOLOR]);
    XClearArea(dpy, Option, 71,vskip+1, OptionGeom.width-86,
           OptionCaller->gdata->menustrip-1, False);
    XSetBackground(dpy, OptionCaller->gdata->wingc, 
                        OptionCaller->gdata->pixel[OPTIONBGCOLOR]);
    XSetForeground(dpy, OptionCaller->gdata->wingc, 
                        OptionCaller->gdata->pixel[OPTIONFGCOLOR]);

    XDrawImageString(dpy, Option, OptionCaller->gdata->wingc, 76,
       OptionCaller->gdata->font[MENUFONT]->max_bounds.ascent + vskip + 3,
       option_entry.string, strlen(option_entry.string));
    if (text_input == OPTION_INPUT) 
       showCaret(OptionCaller, Option, &option_entry, 76, vskip, 1);
    showOptionHint(-1);
}

void
resetStringLength(max, entry)
int max;
TextEntry * entry;
{
        int a;

	entry->maxlength = max;

	a = (entry->string == NULL);
       	entry->string = (char *)
           realloc(entry->string, (max+2)*sizeof(char));
        if (a) {
	   *entry->string = '\0';
	   entry->caret = 0;
	}
	if (entry->caret > max) {
	   entry->string[max] = '\0';
	   entry->caret = max;
	}
}

void
PopOption(Context)
struct Sundata * Context;
{
	int    w, h, a, b, x=0, y=0;
	
	do_option = 1 - do_option;

        if (!do_option) 
	  {
	  XDestroyWindow(dpy, Option);
	  Option = 0;
	  OptionCaller = NULL;
	  return;
	  }

        if (!Option)
           Option = newWindow(NULL, &OptionGeom, 5);

        XSelectInput(dpy, Option, 0);
        OptionCaller = Context;

	w = ((OptionGeom.width-86) / 
              XTextWidth(OptionCaller->gdata->font[MENUFONT], "_", 1)) - 2;
	resetStringLength(w, &option_entry);

	if (runlevel>=RUNTIMEOPTION) 
           option_newhint = '\n';
	else
	   option_newhint = ' ';

	OptionGeom.height = OptionGeom.h_mini
                          = (15 * Context->gdata->menustrip)/4;

	if (!getPlacement(Context->win, &Context->geom.x, &Context->geom.y, &w, &h)) {
	   x = Context->geom.x + OptionGeom.x - horiz_drift - 5;
	   a = Context->geom.y + h + 6;
           if (do_menu && Context == MenuCaller) 
               a += MenuGeom.height + MenuGeom.y + vert_drift + 2;
           b = Context->geom.y - OptionGeom.height - OptionGeom.y - 2*vert_drift - 28;
           if (b < TOPTITLEBARHEIGHT ) b = TOPTITLEBARHEIGHT;
           if (a > (int) DisplayHeight(dpy,scr) 
                   - 2*OptionGeom.height -vert_drift -20)
              a = b;
	   y = (placement<=NE)? a : b;              
	}
        setSizeHints(NULL, 5);
        XMoveWindow(dpy, Option, x, y);
        XMapRaised(dpy, Option);
        XMoveWindow(dpy, Option, x, y);
        XFlush(dpy);
	usleep(2*TIMESTEP);
	option_lasthint = '\0';
	option_newhint = ' ';
	setupOption(-1);
        setProtocols(NULL, 5);
}

void
activateOption()
{
        Sundata *Context;
	Flags oldflags;
        ZoomSettings oldzoom;
        int i, size;
	short *ptr, *oldptr, *newptr;
	double *zptr, *zoldptr, *znewptr;

	Context = OptionCaller;

	if (!do_option || !Context) return;

	oldflags = gflags;
	gflags.animate += 2;
	oldzoom = gzoom;
	runlevel = RUNTIMEOPTION;
	i = parseCmdLine(option_entry.string);
	correctValues();
        if (i>0 || runlevel == FAILEDOPTION) {
	   option_newhint = '?';
	   showOptionHint(-1);
	   return;
	} else
	   option_newhint = '\n';

        showOptionHint(-1);
     /* Set runlevel=IMAGERECYCLE if previous image/pixmap can be recycled */
	if (option_changes<4 && gflags.colorlevel==oldflags.colorlevel && 
            gflags.fillmode==oldflags.fillmode) {
           runlevel = IMAGERECYCLE;
	   tmp_cmap = Context->gdata->cmap;
	   if (gflags.colorlevel<FULLCOLORS) {
              clearNightArea(Context);
	      if (gflags.colorlevel==MONOCHROME) {
	         drawCities(Context);
	         drawSunAndMoon(Context);
	      }
	   } else {
  	      size = Context->xim->bytes_per_line*Context->xim->height;
              memcpy(Context->xim->data, Context->ximdata, size);
	   }
	}
	shutDown(Context, 0);
	memcpy(Context->spotsizes, city_spotsizes, city_cat*sizeof(int));
	memcpy(Context->sizelimits, city_sizelimits, city_cat*sizeof(int));

	if (gflags.animate<2) 
	   Context->flags.animate = gflags.animate;
	else
           gflags.animate -= 2;
	ptr = (short *) &gflags;
	oldptr = (short *) &oldflags;
	newptr = (short *) &Context->flags;
        for (i=0; i<sizeof(Flags)/sizeof(short); i++) 
            if (ptr[i]!=oldptr[i]) newptr[i] = ptr[i];

	zptr = (double *) &gzoom;
	zoldptr = (double *) &oldzoom;
	znewptr = (double *) &Context->zoom;
        for (i=0; i<6; i++) 
            if (zptr[i]!=zoldptr[i]) znewptr[i] = zptr[i];

	if (option_changes & 8)
	    Context->geom = ClockGeom;
	if (option_changes & 16)
	    Context->geom = MapGeom;
	if (option_changes & 32)
            StringReAlloc(&Context->clock_img_file, Clock_img_file);
	if (option_changes & 64)
            StringReAlloc(&Context->map_img_file, Map_img_file);
	buildMap(Context, Context->wintype, 0);
}

void 
processOptionAction(Context, x, y, button, evtype)
Sundata * Context;
int x, y, button, evtype;
{
        static int click_pos = -1;
        int i, opth, vskip, but_pos;
	KeySym key;

        opth = OptionGeom.height - 2 * Context->gdata->menustrip - 1;
	vskip = 3*Context->gdata->menustrip/8;

        if (evtype == ButtonRelease && click_pos>=0) {
	   drawButton(Option, click_pos, 0);
           click_pos = -1;
        }

	if (evtype==ButtonRelease && x>=70 && 
            x<=OptionGeom.width-15 &&
            y>=vskip && y<=Context->gdata->menustrip+vskip) {
	   text_input = OPTION_INPUT;
	   but_pos = (x-76)/XTextWidth(OptionCaller->gdata->font[MENUFONT], "_", 1);
	   i = strlen(option_entry.string);
	   if (but_pos<0) but_pos = 0;
	   if (but_pos>i) but_pos = i;
	   option_entry.caret = but_pos;
	   setupOption(0);
	   return;
	}

        if (evtype==ButtonRelease && text_input==OPTION_INPUT) {
	   text_input = NULL_INPUT;
	   setupOption(0);
	}

	if (y>opth && y<opth+Context->gdata->menustrip) {
           but_pos = getButton(Option, x, y);
	   if (but_pos>=N_OPTION)
	      option_newhint = '\033';
	   else
	   if (but_pos>=0)
	      option_newhint = OptionKey[2*but_pos];
	   if (evtype==MotionNotify) {
	      if (but_pos>=0)
                 showOptionHint(but_pos);
	   }
	   if (evtype==ButtonPress && but_pos>=0) {
	      drawButton(Option, but_pos, 1);
	      click_pos = but_pos;
	   }
	   if (evtype==ButtonRelease) {
	      if (but_pos<0) return;
	      if (but_pos<N_OPTION) {
		 key = (KeySym)tolower(option_newhint);
	         processKey(Option, key);
		 showOptionHint(but_pos);
	      } else
	         PopOption(Context);
	   }
	}
}

void
showUrbanHint(str)
char * str;
{
	char hint[128];
	int l, v;

	if (!do_urban || urban_lasthint==urban_newhint) return;

	*hint = '\0';

	urban_lasthint = urban_newhint;
	v = UrbanGeom.height - UrbanCaller->gdata->menustrip;

	if (urban_newhint=='\033')
           strcpy(hint, Label[L_ESCMENU]);
	else
	if (urban_newhint==' ')
           strcpy(hint, Help[getNumCmd('U')]);
	else {
	   if (str) 
              strncpy(hint, str, 125);
	   else {
	      l = getNumCmd(urban_newhint);
	      if (l>=0 && l<N_HELP) {
		 strcpy(hint, Help[l]);
	      }
	   }
	}

	l = strlen(hint);

        BasicSettings(UrbanCaller);
        XSetWindowBackground(dpy, Urban, UrbanCaller->gdata->pixel[MENUBGCOLOR]);

	XClearArea(dpy, Urban, 0, v+1, UrbanGeom.width, 
             UrbanCaller->gdata->menustrip-1, False);
        XDrawImageString(dpy, Urban, UrbanCaller->gdata->wingc, 
              4, v + UrbanCaller->gdata->font[MENUFONT]->max_bounds.ascent + 3,
              hint, l);
}

void
updateUrbanEntries(Context, city)
Sundata * Context;
City * city;
{
char *ptr;
int w;

    text_input = NULL_INPUT;
    if (city == NULL) return;
    if (city->name && city!=&Context->pos1) {
       w = (urban_w[0]/ 
           XTextWidth(UrbanCaller->gdata->font[MENUFONT],"_",1))-2;
       if (strlen(city->name)<w) w = strlen(city->name);
       strncpy(urban_entry[0].string, city->name, w);
       urban_entry[0].string[w] = '\0';
       urban_entry[0].caret = strlen(urban_entry[0].string);
    } else {
       urban_entry[0].string[0]='\0';
       urban_entry[0].caret = 0;
    }
    if (city->tz) {
       w = (urban_w[1]/ 
           XTextWidth(UrbanCaller->gdata->font[MENUFONT],"_",1))-2;
       if (strlen(city->tz)<w) w = strlen(city->tz);
       strncpy(urban_entry[1].string, city->tz, w);
       urban_entry[1].string[w] = '\0';
       urban_entry[1].caret = strlen(urban_entry[1].string);
    }
    ptr = num2str(city->lat, urban_entry[2].string, Context->flags.dms);
    urban_entry[2].caret = strlen(ptr);
    ptr = num2str(city->lon, urban_entry[3].string, Context->flags.dms);
    urban_entry[3].caret = strlen(ptr);
    sprintf(urban_entry[4].string, "%d", city->size);
    urban_entry[4].caret = 1;
}

void
setupUrban(mode)
int mode;
{
    int i, vskip;
    char s[80];

    if (!do_urban) return;

    if (mode == -2) {
       urban_t[0] = urban_t[2] = 7;
       urban_t[1] = 15 + UrbanGeom.width/2;
       urban_t[3] = (UrbanGeom.width - 120)/2;
       urban_t[4] = UrbanGeom.width - 100;
       urban_x[0] = 100;
       urban_x[1] = 100 + (UrbanGeom.width+1)/2;
       urban_x[2] = 80;
       urban_x[3] = 20 + UrbanGeom.width/2;
       urban_x[4] = UrbanGeom.width-40;
       for (i=0; i<=1; i++)
           urban_y[i] = UrbanCaller->gdata->mapstrip/2 - 1;
       for (i=2; i<=4; i++)
           urban_y[i] = 2*UrbanCaller->gdata->mapstrip - 1;
       urban_w[0] = urban_w[1] = (UrbanGeom.width-210)/2;
       urban_w[2] = urban_w[3] = (UrbanGeom.width-320)/2;
       urban_w[4] = 35;
       return;
    }

    BasicSettings(UrbanCaller);
    XSetWindowColormap(dpy, Urban, UrbanCaller->gdata->cmap);
    XSetWindowBackground(dpy, Urban, UrbanCaller->gdata->pixel[MENUBGCOLOR]);

    urbanh = UrbanGeom.height-2*UrbanCaller->gdata->menustrip - 1;
    vskip = 3*UrbanCaller->gdata->menustrip/8;
    urban_lasthint = '\0';

    if (mode == -1) {
       XClearArea(dpy, Urban, 0,0, UrbanGeom.width,
                                    UrbanGeom.height, False);
       for (i=0; i<=N_URBAN; i++)
	  drawButton(Urban, i, 0);

       BasicSettings(UrbanCaller);
       for (i=0; i<=4; i++) {
           strcpy(s, Label[L_CITYNAME+i]);
           XDrawImageString(dpy, Urban, UrbanCaller->gdata->wingc, 
	       urban_t[i], 
               urban_y[i]+
                  UrbanCaller->gdata->font[MENUFONT]->max_bounds.ascent+4, 
               s, strlen(s));
           XDrawRectangle(dpy, Urban, UrbanCaller->gdata->wingc,
                           urban_x[i], urban_y[i], urban_w[i], 
                           UrbanCaller->gdata->menustrip);
       }
    }  

    for (i=0; i<=4; i++) 
       if (text_input != URBAN_INPUT+i)
          showCaret(UrbanCaller, Urban, &urban_entry[i], 
                                 urban_x[i]+6, urban_y[i], 0);
    for (i=0; i<=4; i++)  
       if (text_input == URBAN_INPUT+i || text_input < URBAN_INPUT) {
       XSetWindowBackground(dpy, Urban, UrbanCaller->gdata->pixel[OPTIONBGCOLOR]);
       XClearArea(dpy, Urban, urban_x[i]+1, urban_y[i]+1, 
              urban_w[i]-1,
              UrbanCaller->gdata->menustrip-1, False);
       XSetBackground(dpy, UrbanCaller->gdata->wingc, 
                      UrbanCaller->gdata->pixel[OPTIONBGCOLOR]);
       XSetForeground(dpy, UrbanCaller->gdata->wingc, 
                      UrbanCaller->gdata->pixel[OPTIONFGCOLOR]);
       XDrawImageString(dpy, Urban, UrbanCaller->gdata->wingc,
          urban_x[i]+6,
          urban_y[i]+ UrbanCaller->gdata->font[MENUFONT]->max_bounds.ascent +4,
          urban_entry[i].string, strlen(urban_entry[i].string));
       if (text_input == URBAN_INPUT+i)
          showCaret(UrbanCaller, Urban, &urban_entry[i], 
                                 urban_x[i]+6, urban_y[i], 1);
    }
    if (urban_newhint == '?')
       urban_newhint = urban_lasthint = '(';
    else
       showUrbanHint(NULL);
}

void
PopUrban(Context)
struct Sundata * Context;
{
	int    i, w, h, a, b, x=0, y=0;
	
	do_urban = 1 - do_urban;

        if (!do_urban) {
	  XDestroyWindow(dpy, Urban);
	  Urban = 0;
	  UrbanCaller = NULL;
	  return;
	}

        if (!Urban)
           Urban = newWindow(NULL, &UrbanGeom, 6);

        XSelectInput(dpy, Urban, 0);
        UrbanCaller = Context;

	UrbanGeom.height = UrbanGeom.h_mini
                          = (22 * Context->gdata->menustrip)/4;
	setupUrban(-2);
        for (i=0; i<=4; i++) {
	   w = (urban_w[i]/ 
              XTextWidth(UrbanCaller->gdata->font[MENUFONT], "_", 1)) - 2;
	   resetStringLength(w, &urban_entry[i]);
	}

	if (!getPlacement(Context->win, &Context->geom.x, &Context->geom.y, &w, &h)) {
	   x = Context->geom.x + UrbanGeom.x - horiz_drift - 5;
	   a = Context->geom.y + h + 6;
           if (do_menu && Context == MenuCaller) 
               a += MenuGeom.height + MenuGeom.y + vert_drift + 2;
           b = Context->geom.y - UrbanGeom.height - UrbanGeom.y - 2*vert_drift -28;
           if (b < TOPTITLEBARHEIGHT ) b = TOPTITLEBARHEIGHT;
           if (a > (int) DisplayHeight(dpy,scr) 
                   - 2*UrbanGeom.height -vert_drift -20)
              a = b;
	   y = (placement<=NE)? a : b;              
	}
        setSizeHints(NULL, 6);
        XMoveWindow(dpy, Urban, x, y);
        XResizeWindow(dpy, Urban, UrbanGeom.width, UrbanGeom.height);
        XMapRaised(dpy, Urban);
        XMoveWindow(dpy, Urban, x, y);
        XFlush(dpy);
	usleep(2*TIMESTEP);
	setupUrban(-1);
        setProtocols(NULL, 6);
}

void
activateUrban()
{
}

void
processUrbanAction(Context, x, y, button, evtype)
Sundata * Context;
int x, y, button, evtype;
{
        static int move_pos;
        static int click_pos;
        int i, j, vskip, but_pos;
	KeySym key;
        
        if (evtype == MotionNotify && move_pos>=0) {
	       drawButton(Urban, move_pos, -2);
	       move_pos = -1;
	}

        if (evtype == ButtonRelease && click_pos>=0) {
	       drawButton(Urban, click_pos, 0);
               click_pos = -1;
	} 

	vskip = 3*Context->gdata->menustrip/8;

	for (i=0; i<=4; i++)
	if (evtype==ButtonRelease && x>=urban_x[i]+1 && 
            x<=urban_x[i]+urban_w[i] &&
            y>=urban_y[i] && y<=urban_y[i]+Context->gdata->menustrip) {
	   text_input = URBAN_INPUT+i;
	   but_pos = 
             (x-urban_x[i]-6)/XTextWidth(UrbanCaller->gdata->font[MENUFONT], "_", 1);
	   j = strlen(urban_entry[i].string);
	   if (but_pos<0) but_pos = 0;
	   if (but_pos>j) but_pos = j;
	   urban_entry[i].caret = but_pos;
	   setupUrban(0);
	   return;
	}

        if (evtype==ButtonRelease) {
	   text_input = NULL_INPUT;
	   setupUrban(0);
	}

	if (y>urbanh && y<urbanh+Context->gdata->menustrip) {
           but_pos = getButton(Urban, x, y);
	   if (but_pos>=N_URBAN)
	      urban_newhint = '\033';
	   else
	   if (but_pos>=0)
	      urban_newhint = UrbanKey[2*but_pos];
	   if (evtype==MotionNotify) {
	      if (but_pos>=0 && but_pos<=N_URBAN) {
                 showUrbanHint(NULL);
		 drawButton(Urban, but_pos, -1);
                 move_pos = but_pos;
	      }
	   }
	   if (evtype==ButtonPress && but_pos>=0) {
	      drawButton(Urban, but_pos, 1);
	      click_pos = but_pos;
	   }
	   if (evtype==ButtonRelease) {
	      if (but_pos<0) return;
	      if (but_pos<N_URBAN) {
		 if (button<=2)
		    key = (KeySym)tolower(urban_newhint);
		 else
		    key = (KeySym)urban_newhint;
		 showUrbanHint(NULL);
	         processKey(Urban, key);
	      } else
	         PopUrban(Context);
	   }
	}
	if (evtype==MotionNotify && 
            (y<=urbanh || y>=urbanh+Context->gdata->menustrip)) {
	   urban_newhint = 'U';
           showUrbanHint(NULL);
	}
}

struct Sundata *
parentContext(Context)
struct Sundata * Context;
{
   struct Sundata * ParentContext;

   ParentContext = Seed;
   while (ParentContext && ParentContext->next != Context) 
     ParentContext = ParentContext->next;
   return ParentContext;
}

void
setAuxilWins(Context, mode)
Sundata * Context;
int mode;
{
#define NUMWIDGETS 5


int * bool_state[NUMWIDGETS] = { 
  &do_menu, &do_filesel, &do_zoom, &do_option, &do_urban };

Window * window[NUMWIDGETS] = { &Menu, &Filesel, &Zoom, &Option, &Urban };

struct Sundata ** caller[NUMWIDGETS] = { 
  &MenuCaller, &FileselCaller, &ZoomCaller, &OptionCaller, &UrbanCaller };

char * char_newhint[NUMWIDGETS] = {
   (char *)&menu_newhint, NULL, 
   &zoom_newhint, &option_newhint, &urban_newhint };

setupCB setup_proc[NUMWIDGETS] = {
   &setupMenu, &setupFilesel, &setupZoom, &setupOption, &setupUrban };

popCB pop_proc[NUMWIDGETS] = {
   &PopMenu, &PopFilesel, &PopZoom, &PopOption, &PopUrban };

int i, announce=1;
  
      if (option_changes && mode==RESET) mode = REMAP;

      for (i=0; i<NUMWIDGETS; i++) if (*bool_state[i]) {
	 if (announce && verbose && mode!=RESET) {
	   fprintf(stderr, 
              "Resetting auxiliary widgets in mode %d...\n", mode);
	   announce = 0;
	 }
	 
	 switch(mode) {

         case RESET:
	    if (Context == *caller[i]) {
  	       *bool_state[i] = 1;
	       *caller[i] = Context;
	       setup_proc[i](-1);
	    }
	    break;

         case REMAP:
	    if (Context != *caller[i]) break;
	 case REATTRIB:
	    *bool_state[i] = 0;
	    if (*window[i]) 
               XUnmapWindow(dpy, *window[i]);
	    if (char_newhint[i])
	       *char_newhint[i] = ' ';
	    pop_proc[i](Context);
	    break;

         case DESTROY:
	    if (Context == *caller[i]) {
	       if (*window[i]) {
		  XDestroyWindow(dpy, *window[i]);
                  *window[i] = 0;
	       }
	       *bool_state[i] = 0;
	       *caller[i] = NULL;
	    }
	    break;

         case ICONIFY:
	    if (Context == *caller[i])
	       XIconifyWindow(dpy, *window[i], scr);
	    break;

         case DEICONIFY:
	    if (Context == *caller[i]) {
	       XMapWindow(dpy, *window[i]);
	       setup_proc[i](-1);
	    }
	    break;

	 }
      }

      XFlush(dpy);
      if (mode == REMAP) usleep(2*TIMESTEP);
}

void RaiseAndFocus(win)
Window win;
{
     XFlush(dpy);
     XRaiseWindow(dpy, win);
     XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
}

/*
 * Free GC's.
 */

void
destroyGCs(Context)
Sundata * Context;
{
int i; 
	 if (Context->gdata->links>0) {
            --Context->gdata->links;
	    return;
	 }

         XFreeGC(dpy, Context->gdata->wingc);
	 Context->gdata->wingc = 0;
         XFreeGC(dpy, Context->gdata->pixgc);
         Context->gdata->pixgc = 0;

         if (runlevel!=IMAGERECYCLE && Context->gdata->cmap!=cmap0)
	    XFreeColormap(dpy, Context->gdata->cmap);

	 for (i=0; i<NUMFONTS; i++)
             XFreeFont(dpy, Context->gdata->font[i]);
	
	 free(Context->gdata);
}

/*
 * Free resources.
 */

void
shutDown(Context, all)
struct Sundata * Context;
int all;
{
        struct Sundata * ParentContext, *NextContext;

	if (all<0)
	   Context = Seed;

      repeat:
	fflush(stderr);

        NextContext = Context->next;
        ParentContext = parentContext(Context);

	if (runlevel!=IMAGERECYCLE) {
  	   if (Context->xim) {
              XDestroyImage(Context->xim); 
              Context->xim = NULL;
	   }
	   if (Context->ximdata) {
	      free(Context->ximdata);
	      Context->ximdata = NULL;
	   }
	   while (Context->label) {
	      struct TextLabel *ptr;
	      ptr = (Context->label)->next;
              free(Context->label->text);
              free(Context->label);
	      Context->label = ptr;
	   }
	   if (Context->mappix) {
              XFreePixmap(dpy, Context->mappix);
	      Context->mappix = 0;
 	   }
           if (Context->daypixel) {
	      free(Context->daypixel);
	      Context->daypixel = NULL;
           }
           if (Context->nightpixel) {
	      free(Context->nightpixel);
	      Context->nightpixel = NULL;
           }
           if (Context->vmfpixels) {
	      free(Context->vmfpixels);
              Context->vmfpixels = NULL;
	   }
           if (Context->tr1) {
              free(Context->tr1);
              Context->tr1 = NULL;
           }
           if (Context->tr2) {
              free(Context->tr2);
              Context->tr2 = NULL;
           }
           if (Context->daywave) {
              free(Context->daywave);
              Context->daywave = NULL;
	   }
	}
        destroyGCs(Context);

	Context->flags.hours_shown = 0;

        if (all) {
	   last_time = 0;

           if (Context->win) {
	      setAuxilWins(Context, DESTROY);
	      if (Context == RootCaller) {
                 RootCaller = NULL;
		 do_root = 0;
	      }
	      XDestroyWindow(dpy, Context->win);
  	      Context->win = 0;
	   }
           if (Context->clock_img_file) {
              free(Context->clock_img_file);
	      Context->clock_img_file = NULL;
	   }
           if (Context->map_img_file) {
              free(Context->map_img_file);
	      Context->map_img_file = NULL;
	   }
	
           free(Context->spotsizes);
           free(Context->sizelimits);
  
	   if (all<0) {
	      free(Context);
	      if (NextContext) {
	         Context = NextContext;
	         goto repeat;
	      }
	      else {
	        endup:
                 if (zoompix) XFreePixmap(dpy, zoompix);
                 if (textpix) XFreePixmap(dpy, textpix);
                 if (rootpix) XFreePixmap(dpy, rootpix);
                 XCloseDisplay(dpy);
         	 if (dirtable) free(dirtable);
         	 exit(0);
 	      }
 	   }
	   if (ParentContext)
	      ParentContext->next = Context->next;
	   else
              Seed = Context->next;
	   free(Context);
           if (Seed == NULL) goto endup;
	}
}

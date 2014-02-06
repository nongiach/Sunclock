/*****************************************************************************
 *
 * Sunclock version 3.xx by Jean-Pierre Demailly
 *
 * Is derived from the previous versions whose notices appear below.
 * See CHANGES for more explanation on the (quite numerous changes and
 * improvements). Version 3.xx is now published under the GPL.
 */

/*****************************************************************************
 *
 * Sun clock.  X11 version by John Mackin.
 *
 * This program was derived from, and is still in part identical with, the
 * Suntools Sun clock program whose author's comment appears immediately
 * below.  Please preserve both notices.
 *
 * The X11R3/4 version of this program was written by John Mackin, at the
 * Basser Department of Computer Science, University of Sydney, Sydney,
 * New South Wales, Australia; <john@cs.su.oz.AU>.  This program, like
 * the one it was derived from, is in the public domain: `Love is the
 * law, love under will.'
 */

/*****************************************************************************

        Sun clock

        Designed and implemented by John Walker in November of 1988.

        Version for the Sun Workstation.

    The algorithm used to calculate the position of the Sun is given in
    Chapter 18 of:

    "Astronomical  Formulae for Calculators" by Jean Meeus, Third Edition,
    Richmond: Willmann-Bell, 1985.  This book can be obtained from:

       Willmann-Bell
       P.O. Box 35025
       Richmond, VA  23235
       USA
       Phone: (804) 320-7016

    This program was written by:

       John Walker
       Autodesk, Inc.
       2320 Marinship Way
       Sausalito, CA  94965
       USA
       Fax:   (415) 389-9418
       Voice: (415) 332-2344 Ext. 2829
       Usenet: {sun,well,uunet}!acad!kelvin
           or: kelvin@acad.uu.net

    modified for interactive maps by

        Stephen Martin
        Fujitsu Systems Business of Canada
        smartin@fujitsu.ca

    This  program is in the public domain: "Do what thou wilt shall be the
    whole of the law".  I'd appreciate  receiving  any  bug  fixes  and/or
    enhancements,  which  I'll  incorporate  in  future  versions  of  the
    program.  Please leave the original attribution information intact  so
    that credit and blame may be properly apportioned.

    Revision history:

        1.0  12/21/89  Initial version.
              8/24/89  Finally got around to submitting.

        1.1   8/31/94  Version with interactive map.
        1.2  10/12/94  Fixes for HP and Solaris, new icon bitmap
        1.3  11/01/94  Timezone now shown in icon

    next minor revisions by Jean-Pierre Demailly (demailly@ujf-grenoble.fr)
        1.4  04/03/99  Change in city management
        1.5  15/03/99  Color support
        1.6  28/03/99  Iconic stuff fixed
        1.7  17/08/99  Calculation of city distances
        1.8  20/08/99  Fixed small bug in mono mode
        1.9  21/01/00  Iconic stuff fixed again (didn't work for all WMHints!)
        2.0  21/04/00  Added XFlush in doTimeout to force update
        2.1  06/06/00  Updated latlong.txt with a better source 

    major rewrite of GUI (Jean-Pierre Demailly, demailly@ujf-grenoble.fr)
       3.00  08/26/00  Many bug corrections, GUI improvements (keyboard and
                       mouse controls), new functions, resulting in a much 
                       more powerful program.
       3.04  09/07/00  Final bug fix release of 3.0x
       3.10  10/03/00  Menu window and a lot of new improvements as well.
       3.13  10/26/00  Final bug fix release of 3.1x
       3.20  11/20/00  Drastic GUI and features improvements (color maps,
                       loadable xpm maps,...), hopefully sunclock now 
                       looks nice!
       3.21  12/04/00  Final bug fix release of 3.2x x<5
       3.25  12/21/00  Dockable, multi-window version
       3.28  01/15/01  Final bug fix release of 3.2x x>=5
       3.30  02/20/01  Sunclock now loads JPG images, and images are resizable
       3.32  03/19/01  Implementation of the zoom widget
       3.34  03/28/01  Vast improvements in the zoom widget
       3.35  04/11/01  Correctly handles bigendian machines + option widget
       3.38  04/17/01  Substantial improvements in option handling
                       still buggy though (depending on WM)
       3.41  05/19/01  The Moon is shown on the map
       3.43  06/14/01  Cities can be managed at runtime and changed with zoom
       3.44  06/22/01  Sunclock can write the map to the root window
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <string.h>
#include <X11/Xatom.h>

#define DEFVAR
#include "sunclock.h"
#include "langdef.h"
#include "bitmaps.h"

/* 
 *  external routines
 */

extern long     time();
#ifdef NEW_CTIME
extern char *   timezone();
#endif

extern double   jtime();
extern double   gmst();
extern long     jdate();
extern void     sunpos();
extern double   phase();

extern char *   salloc();
extern int      readVMF();
extern int      readGIF();
extern int      readJPEG();
extern int      readXPM();
extern int      readPNG();

extern void free_dirlist();

extern char *tildepath();       /* Returns path to ~/<file> */

extern int  getPlacement();
extern int  getState();
extern int getNumCmd();

extern Window newWindow();

extern void showManual();

extern void checkGeom();
extern void adjustGeom();
extern void setSizeHints();
extern void setClassHints();
extern void shutDown();
extern void setProtocols();
extern void destroyGCs();

extern void setupMenu();
extern void PopMenu();
extern void showMenuHint();
extern void processMenuAction();

extern void setupFilesel();
extern void PopFilesel();
extern void processFileselAction();

extern void checkZoomSettings();
extern void setZoomDimension();
extern void setZoomAspect();
extern void showZoomHint();
extern void setupZoom();
extern void PopZoom();
extern void activateZoom();
extern void processZoomAction();

extern void showOptionHint();
extern void resetStringLength();
extern void setupOption();
extern void PopOption();
extern void activateOption();
extern void processOptionAction();

extern void showUrbanHint();
extern void updateUrbanEntries();
extern void setupUrban();
extern void PopUrban();
extern void activateUrban();
extern void processUrbanAction();

extern void setAuxilWins();
extern void RaiseAndFocus();

void doTimeout();

char share_i18n[] = SHAREDIR"/i18n/Sunclock.**";
char app_default[] = SHAREDIR"/Sunclockrc";

char * ProgName;
char * Title = NULL;

char * widget_type[7] =     
           { "clock", "map", "menu", "file selector", 
             "zoom", "option", "urban selector" };

char * ClassName = NULL;
char * ClockClassName = NULL;
char * MapClassName = NULL;
char * AuxilClassName = NULL;

char * Default_img_file = NULL;
char * Clock_img_file = NULL;
char * Map_img_file = NULL;
char * Zoom_img_file = NULL;

char * share_maps_dir = NULL;
char * image_dir = NULL;
char * rc_file = NULL;
char * vmfcolors = NULL;
char * vmfrange = NULL;
char * vmfcoordformat = NULL;

char * ExternAction = NULL;
char * EditorCommand = NULL;
char **DateFormat = NULL;
char * ListFormats = NULL;

char **dirtable = NULL;
char * freq_filter = "";

char oldlanguage[4] = "en";
char language[4] = "";

Pixmap textpix = 0, zoompix = 0, rootpix = 0;
;

struct Sundata *Seed = NULL, 
               *MenuCaller = NULL, 
               *FileselCaller = NULL, 
               *ZoomCaller = NULL, 
               *OptionCaller = NULL,
               *UrbanCaller = NULL,
               *RootCaller = NULL;

char    *Color[NUMPIXELS];

char    *DefaultColor[NUMPIXELS] = {
"White", "White", "Grey92", "Grey92", "Grey92", 
"White", "White", "Grey84", "White", 
"Black", "Black", "Black", 
"Black", "Grey50", "Grey95", "White",
"Black", "Black", 
"Black", "Black", "Red", "Black",
"SkyBlue2", "Brown", "SkyBlue2", "Blue", "Magenta", 
"Red", "Orange", "Red", "Red3", "Pink1", "Pink2",
"Yellow", "Khaki", "Black", "Black", "Black", "Black"};

#define FALLBACKTOWHITE 9

char *colorfield[NUMPIXELS] =
{ "clockbg", "mapbg", "menubg", "clockstripbg", "mapstripbg",
  "zoombg", "optionbg", "buttonbg", "star", 
  "clockfg", "mapfg", "menufg",  
  "buttonfg1", "buttonfg2", "buttonfg3", "buttonfg4",
  "clockstripfg", "mapstripfg", 
  "zoomfg", "optionfg", "weak", "root",
  "caret", "change", "choice", "directory", "image", "cityname",
  "city0", "city1", "city2", "mark1", "mark2", "sun", "moon",
  "line", "meridian", "parallel", "tropic"};

char * SunFont[NUMFONTS];
char * DefaultFont[NUMFONTS]= { 
  "6x10",  "6x13",  "6x13",  "6x10",  "6x10", "6x13"};
char * fontfield[NUMFONTS] = { 
  "clockstrip", "mapstrip", "coord", "city", "label", "menu"};


char *          Display_name = NULL;
char *          CityInit = NULL;
char *          SpotSizes = NULL;
char *          SizeLimits = NULL;

Display *       dpy;
Visual *        visual;
Colormap        cmap0, tmp_cmap;

Flags           gflags;
ZoomSettings    gzoom;

int             scr;
int             bigendian;
int             color_depth;
int             color_pad;
int             bytes_per_pixel;
int             total_colors;
int             text_input = NULL_INPUT;

int             textheight = 0;
int             textwidth = 40;
int             coordvalheight;
int             coordvalwidth;
int             extra_width = 10;

int             focus_in = 0;
int             button_pressed = 0;
int             control_key = 0;
int             precedence = 0;
int             option_changes = 1;
int             auxil_changed = 0;
int             erase_obj = 0;
int             root_period = 30;
int             screen_saver = 0;
int             random_rootpos = 0;

int             zoom_mode = 0;
int             zoom_active = 1;

KeySym          menu_newhint = ' ';
char            zoom_newhint = ' ';
char            option_newhint = ' ';
char            urban_newhint = ' ';

char            menu_lasthint = '\0';
char            zoom_lasthint = '\0';
char            option_lasthint = '\0';
char            urban_lasthint = '\0';

TextEntry       option_entry;
TextEntry       urban_entry[5];

Pixel           black, white;

Atom            wm_delete_window, wm_protocols;

Window          Root, 
                Menu = 0, Filesel = 0, Zoom = 0, Option = 0, Urban = 0;

struct Geometry MapGeom    = { 0, 30,  30, 792, 396, 320, 160 };
struct Geometry ClockGeom  = { 0, 30,  30, 128,  64,  48,  24 };
struct Geometry MenuGeom   = { 0,  0,  30, 792,  40, 792,  40 };
struct Geometry FileselGeom= { 0,  0,  30, 450, 180, 400,  80 };
struct Geometry ZoomGeom   = { 0,  0,  30, 500, 320, 360, 250 };
struct Geometry OptionGeom = { 0,  0,  30, 630,  80, 630,  80 };
struct Geometry UrbanGeom  = { 0,  0,  30, 640, 120, 360, 120 };

int             city_cat = 5;
int             *city_spotsizes;
int             *city_sizelimits;

int             urban_t[5], urban_x[5], urban_y[5], urban_w[5];

int             win_type = 0;
int             placement = -1;
int             place_shiftx = 0;
int             place_shifty = 0;
int             color_alloc_failed = 0;
int             num_formats;
int             runlevel;
int             verbose = 1;
int             reformat = 0;
int		min_zoomwidth = 0;
int		min_zoomheight = 0;
int             citycheck = 0;
int             num_lines;
int             num_table_entries;

int             label_shift = 0;
int             filesel_shift = 0;
int             zoom_shift = 0;

int             horiz_drift = 0;
int             vert_drift =  0;

int             do_menu = 0;
int             do_filesel = 0;
int             do_zoom = 0;
int             do_option = 0;
int             do_urban = 0;
int             do_root = 0;

int             do_hint = 0;
int             do_dock = 0;
int             do_sync = 0;
int             do_zoomsync = 0;

int             time_jump = 0;

long            last_time = 0;
long            progress_value[6] = { 60, 3600, 86400, 604800, 2592000, 0 };

double          darkness = 0.5;
double          rootdx = 0.5;
double          rootdy = 0.5;
double          atm_refraction = ATM_REFRACTION;
double          atm_diffusion = ATM_DIFFUSION;

/* Root and last records for cities */

City position, *cityroot = NULL, *citylast = NULL, *cityinit = NULL;

/* 
 * String copy and reallocation/deallocation routine 
 */

void
StringReAlloc(t, s)
char **t, *s;
{
       if (*t) free(*t);

       if (s) {
          *t = (char *) salloc((strlen(s)+1)*sizeof(char));
          strcpy(*t, s);
       } else
          *t = NULL;
}

/*
 * Read language i18n file
 */

void 
readLanguage()
{
    FILE *rc;           /* File pointer for rc file */
    char buf[1028];      /* Buffer to hold input lines */
    int i, j, k, l, m, p, tot;

    i = strlen(share_i18n);

    for (j=0; j<=1; j++) share_i18n[i+j-2] = tolower(language[j]);

    j = k = l = m = 0;
    tot = 2;
    if ((rc = fopen(share_i18n, "r")) != NULL) {
      while (fgets(buf, 1024, rc)) {
        if (buf[0] != '#') {
                p = strlen(buf) - 1;
                if (p>=0 && buf[p]=='\n') buf[p] = '\0';
                if (j<7) { strcpy(Day_name[j], buf); j++; } else
                if (k<12) { strcpy(Month_name[k], buf); k++; } else
                if (l<L_END) { 
                   StringReAlloc(&Label[l], buf); 
                   l++; 
                } else
                if (m<N_HELP) { 
		   StringReAlloc(&Help[m], buf);
                   m++; 
                } else break;
        }
      }
      strncpy(oldlanguage, language, 2);

      fclose(rc);
    } else {
        fprintf(stderr, 
             "Unable to open language in %s\n", share_i18n);
	strncpy(language, oldlanguage, 2);
    }
}


/* 
 * Usage output 
 */

void
Usage()
{
#define SP		"\t"
     fprintf(stderr, 
     "%s: version %s, %s\n\nUsage:  %s [-options ...]\n\n%s\n\n"
     SP"[-help] [-listmenu] [-version] [-citycheck]\n"
     SP"[-display name] [-sharedir directory] [-citycategories value]\n"
     SP"[-clock] [-map] [-dock] [-undock]\n"
     SP"[-menu] [-nomenu] [-filesel] [-nofilesel]\n"
     SP"[-zoom] [-nozoom] [-option] [-nooption] [-urban] [-nourban]\n"
"**" SP"[-language name] [-rcfile file]\n"
     SP"[-command string] [-helpcommand string]\n"
     SP"[-mapmode * <L,C,S,D,E>] [-dateformat string1|string2|...]\n"
     SP"[-image file] [-clockimage file] [-mapimage file] [-zoomimage file]\n"
     SP"[-clockgeom <geom>] [-mapgeom <geom>]\n"
     SP"[-auxilgeom <geom>] [-menugeom <geom>] [-selgeom <geom>]\n"
     SP"[-zoomgeom <geom>] [-optiongeom <geom>] [-urbangeom <geom>]\n"
     SP"[-title name] [-mapclassname name] [-clockclassname name]\n"
     SP"[-auxilclassname name] [-classname name]\n"
     SP"[-setfont <field>|<fontsetting>{|<languages>}]\twhere\n"
     SP"\t<field> = clockstrip, mapstrip, coord, city, label, menu\n"
     SP"\t<languages> = comma separated list (optional)\n"	     
     SP"[-verbose] [-silent] [-synchro] [-nosynchro] [-zoomsync] [-nozoomsync]\n"
     SP"[-colorlevel level] [-aspect mode]\n"
     SP"[-placement (random, fixed, center, NW, NE, SW, SE)]\n"
     SP"[-placementshift x, y] [-extrawidth value]\n"
     SP"[-decimal] [-dms] [-city name] [-position latitude|longitude]\n"
     SP"[-addcity size|name|lat|lon|tz] [-removecity name (name|lat|lon)]\n"
     SP"[-rootdx value] [-rootdy value] [-fixedrootpos] [-randomrootpos]\n"
     SP"[-screensaver] [-noscreensaver] [-rootperiod value (in seconds)]\n"
     SP"[-animation] [-noanimation] [-animateperiod value (in seconds)]\n"
     SP"[-progress number[s,m,h,d,M,Y]] [-jump number[s,m,h,d,M,Y]]\n"
     SP"[-shading mode=0,1,2,3,4,5] [-diffusion value] [-refraction value]\n"
     SP"[-night] [-terminator] [-twilight] [-luminosity] [-lightgradient]\n"
     SP"[-nonight] [-darkness value<=1.0] [-colorscale number>=1]\n"
     SP"[-coastlines] [-contour] [-landfill] [-fillmode number=0,1,2]\n"
     SP"[-mag value] [-magx value] [-magy value] [-dx value ] [-dy value]\n"
     SP"[-spotsizes s1|s2|s3|... (0<=si<=5, 1<=i<=citycategories)]\n"
     SP"[-sizelimits w1|w2|w3|... (wi = zoom width values)]\n"
     SP"[-citymode mode=0,1,2,3] [-objectmode mode=0,1,2]\n"
     SP"[-sun] [-nosun] [-moon] [-nomoon] [-tropics] [-notropics]\n"
     SP"[-meridianmode mode=0,1,2,3] [-parallelmode mode=0,1,2,3]\n"
     SP"[-meridianspacing value] [-parallelspacing value]\n"
     SP"[-dottedlines] [-plainlines] [-bottomline] [-nobottomline]\n"
     SP"[-reformat] [-vmfcolors color1|color2|color3...]\n"
     SP"[-vmfrange a|b|c|d] [-vmfcoordformat format] [-vmfflags integer]\n"
     SP"[-setcolor field|color]\n\n"
     SP"field = clockbg, clockfg, mapbg, mapfg, menubg, menufg,\n"
     SP"buttonbg, buttonfg1, buttonfg2, buttonfg3, buttonfg4,\n"
     SP"clockstripbg, clockstripfg, mapstripbg, mapstripfg,\n"
     SP"cityname, zoombg, zoomfg, optionbg, optionfg, caret, change, choice,\n"
     SP"directory, image, city0, city1, city2, mark1, mark2, line,\n"
     SP"meridian, parallel, tropic, sun, moon,\n\n"
     SP"color = red, yellow, ... (rgb.txt) / hexadecimal #ijklmn\n\n%s\n\n",
        ProgName, VERSION, COPYRIGHT, ProgName, Label[L_LISTOPTIONS],
        Label[L_CONFIG]);
}

void
ListMenu()
{
        int i;

        fprintf(stderr, "%s\n", Label[L_SHORTHELP]);
        for (i=0; i<N_HELP; i++)
        fprintf(stderr, "%s %c : %s\n", Label[L_KEY], CommandKey[i], Help[i]);
        fprintf(stderr, "\n");
}

void
initValues()
{
        int i;

        gflags.colorlevel = FULLCOLORS;
        gflags.fillmode = 2;
        gflags.vmfflags = -1;
        gflags.dotted = 0;
        gflags.colorscale = 16;

	gflags.animate = 0;
	gflags.animperiod = 0;
        gflags.progress = 0;

        gflags.update = 4;
        gflags.bottom = 0;
        gflags.map_mode = LEGALTIME;
        gflags.clock_mode = 0;
        gflags.hours_shown = 0;
        gflags.dms = 0;
        gflags.shading = 2;
        gflags.citymode = 1;
        gflags.objectmode = 1;
        gflags.objects = 3;
        gflags.meridian = 0;
        gflags.parallel = 0;

        gzoom.mode = 0;
        gzoom.fx = 1.0;
        gzoom.fy = 1.0;
        gzoom.fdx = 0.5;
        gzoom.fdy = 0.5;
        gzoom.meridspacing = 0.0;
        gzoom.paralspacing = 0.0;

        option_entry.string = NULL;
        for (i=0; i<=4; i++) urban_entry[i].string = NULL;

        for (i=0; i<NUMPIXELS; i++)
            Color[i] = strdup(DefaultColor[i]);
        for (i=0; i<NUMFONTS; i++)
            SunFont[i] = strdup(DefaultFont[i]);

        position.lat = 100.0;
        position.tz = NULL;
   
        StringReAlloc(&share_maps_dir, SHAREDIR"/earthmaps/");
        StringReAlloc(&image_dir, share_maps_dir);

        StringReAlloc(&Default_img_file, SHAREDIR"/earthmaps/vmf/timezones.vmf");
        StringReAlloc(&Zoom_img_file, SHAREDIR"/earthmaps/vmf/landwater.vmf");
        StringReAlloc(&Clock_img_file, Default_img_file);
        StringReAlloc(&Map_img_file, Default_img_file);
   
        StringReAlloc(&ListFormats, STDFORMATS);
        StringReAlloc(&EditorCommand, EDITORCOMMAND);
        StringReAlloc(&SpotSizes, "1|2|3|4|5");
        StringReAlloc(&SizeLimits, "0|580|2500|6000|12000");

	for (i=0; i<L_END; i++) Label[i] = strdup(Label[i]);
	for (i=0; i<N_HELP; i++) Help[i] = strdup(Help[i]);
}

/*
 *  Fill line procedure for scanned images
 */
#define RANGE 252
long lr[RANGE], lg[RANGE], lb[RANGE], lnum[RANGE];

void
fill_line(char *scan, char* c, int w, int zw, int wp, int dx)
{
unsigned char r, g, b;
int i, j, k, m;
  k = 0;
  if (color_depth>16) {
    if (bigendian)
       k = bytes_per_pixel - 3;
    for (i = 0; i < w; i++) {
       j = 3 * (((i+dx) * wp)/zw);
       if (bigendian) {
          c[k] = scan[j];
          c[k+1] = scan[j+1];
          c[k+2] = scan[j+2];
       } else {
          c[k] = scan[j+2];
          c[k+1] = scan[j+1];
          c[k+2] = scan[j];
       }
       k +=  bytes_per_pixel;
    }
  } else
  if (color_depth==16)
     for (i = 0; i < w; i++) {
       j = 3 * (((i+dx) * wp)/zw);
       r = scan[j];
       g = scan[j+1];
       b = scan[j+2];
       /* blue  c[k] = 31;  c[k+1] = 0;
          green c[k] = 224  (low weight) c[k+1] = 7 (high weight)
          red   c[k] = 0;   c[k+1] = 248; */
       if (bigendian) {
          c[k+1] = (((b&248)>>3) | ((g&28)<<3));
          c[k] = (((g&224)>>5) | (r&248));
       } else {
          c[k] = (((b&248)>>3) | ((g&28)<<3));
          c[k+1] = (((g&224)>>5) | (r&248));
       }
       k += 2;
     }
  else
  if (color_depth==15)
     for (i = 0; i < w; i++) {
       j = 3 * (((i+dx) * wp)/zw);
       r = scan[j];
       g = scan[j+1];
       b = scan[j+2];
       /* blue  c[k] = 31;  c[k+1] = 0;
          green c[k] = 224  (low weight) c[k+1] = 7 (high weight)
          red   c[k] = 0;   c[k+1] = 248; */
       if (bigendian) {
          c[k+1] = (b&248)>>3 | (g&56)<<2;
          c[k] = (g&192)>>6 | (r&248)>>1;
       } else {
          c[k] = (b&248)>>3 | (g&56)<<2;
          c[k+1] = (g&192)>>6 | (r&248)>>1;
       }
       k += 2;
     }
  else {
     for (i = 0; i < w; i++) {
       j = 3 * (((i+dx) * wp)/zw);
       r = scan[j];
       g = scan[j+1];
       b = scan[j+2];
       c[k] = (unsigned char) 
       (((7*g)/256)*36)+(((6*r)/256)*6)+((6*b)/256);
       m = (unsigned char)c[k];
       lr[m] += r;
       lg[m] += g;
       lb[m] += b;
       lnum[m] += 1;
       k += 1;
     }
  }
}

void
str2numval(s, val, max)
char *s;
int *val;
int max;
{
int i, j, l;
char *ptr;
 
    l = strlen(s);

    j = 0;
    ptr = s;
    for (i=0; i<=l; i++) {
        if (s[i] == '|' || i == l) {
	   s[i] = '\0';
	   if (j>=city_cat) break;
           val[j] = atoi(ptr);
	   if (max>0 && val[j]>max) val[j] = max;
	   ++j;
	   ptr = s+i+1;
           if (i<l) s[i] = '|';
	}
    }

    for (i=j; i<city_cat; i++) val[i] = val[j-1];
}

void
correctValues() 
{
	if (color_depth<=8 && gflags.colorscale>256) 
           gflags.colorscale = 256;

        if ((gflags.colorlevel<FULLCOLORS) && (gflags.shading>=2)) 
           gflags.shading = 1;

        if (color_depth<=16) 
           gflags.darkness = (unsigned short) ((1.0-darkness) * 255.25);
	else
           gflags.darkness = (unsigned short) ((1.0-darkness) * 32767.25);

	if (do_dock) win_type = 0;

        str2numval(SpotSizes, city_spotsizes, CITYBITMAPS);
        str2numval(SizeLimits, city_sizelimits, -1);
}

int
needMore(argc, argv)
int *                  argc;
char **                argv;
{
	-- *argc;
        if (*argc == 0) {
                if (runlevel == PARSECMDLINE) {
		   Usage();
		   exit(1);
		}
		   else 
                if (runlevel == RUNTIMEOPTION) {
                   fprintf(stderr, "Invalid option specification\n");
		   runlevel = FAILEDOPTION;
		} else
                   fprintf(stderr, "Error in config file : \n");

                fprintf(stderr, "%s: option `%s' (with no argument) incorrect\n",
                        ProgName, *argv);

		if (runlevel == PARSECMDLINE) exit(1);
	        return 1;
        }
	return 0;
}

void
getGeom(s, g)
register char *                 s;
register struct Geometry *      g;
{
        register int            mask;

        mask = XParseGeometry(s, &g->x, &g->y, &g->width, &g->height);
        if (mask == 0) {
                fprintf(stderr, "%s: `%s' is a bad geometry specification\n",
                        ProgName, s);
                return;
        }
        if (g->width<g->w_mini) g->width = g->w_mini;
        if (g->height<g->h_mini) g->height = g->h_mini;
        g->mask = mask;
        if ( placement<=RANDOM && (mask & ( XValue | YValue)) ) 
           placement = FIXED;
}

void
parseFormats(char * format)
{
        int i, j, l;
        
        l = strlen(format);

        j = 1;
        while (j>0) {
           j = 0;
           if (format[0]=='|') { ++format; --l; j = 1;}
           if (l>=2 && format[l-1]=='|' && format[l-2]!= '%') 
                               { format[l-1]='\0'; --l; j = 1; }
        }

        num_formats = 1;
        for (i=1; i<l-1; i++) 
            if (format[i]=='|' && format[i-1]!='%') ++num_formats;

        DateFormat = (char **) salloc(num_formats*sizeof(char *));

        DateFormat[0] = format;
        j = 0;

        for (i=1; i<l-1; i++) 
          if (format[i]=='|' && format[i-1]!='%') {
            ++j;
            format[i] = '\0';
            DateFormat[j] = format+i+1;
          }
}

int setWindowAspect(Context)
struct Sundata * Context;
{
   unsigned int b, i, j;
   int change;
   struct Geometry *       Geom;
   double f;

   if (Context->newzoom.mode <= 1)
      f = 1.0;
   else
      f = sin(M_PI*Context->newzoom.fdy);

   change = 0;
   i = Context->geom.width;

   j = (unsigned int) (0.5 + Context->newzoom.fx * 
          (double)Context->geom.width / ( 2.0 * Context->newzoom.fy * f) );
   b = DisplayHeight(dpy,scr) - Context->hstrip - vert_drift - 5;
   if (j<=0) j=1;
   if (j>b) {
      i = (Context->geom.width * b) / j;
      j = b;
   }

   Geom = (Context->wintype)? &MapGeom : &ClockGeom;

   if (j<Geom->h_mini) {
      i = (i * Geom->h_mini)/j;
      j = Geom->h_mini;
   }
  
   if (Context->geom.width != i) {
      change = 1;
      Context->geom.width = Geom->width = i;
   }
   if (Context->geom.height != j) {
      change = 1;
      Context->geom.height = Geom->height = j;
   }

   if (change && verbose)
      fprintf(stderr, "Resizing window to width = %d , height = %d\n", i, j); 

   return change;
}

void
checkRCfile(argc, argv)
register int                    argc;
register char **                argv;
{
        int i;

        for (i=1; i<argc; i++) {
           if (i<argc-1 && !strcasecmp(argv[i], "-rcfile"))
              rc_file = argv[i+1];
           if (i<argc-1 && !strcasecmp(argv[i], "-language")) {
              strncpy(language, argv[i+1], 2);
	      language[2] = '\0';
	   }
           if (strcasecmp(argv[i], "-verbose") == 0)
	      verbose = 1;
	} 

        if (strcmp(language, oldlanguage)) readLanguage();
}

double
dms2decim(string)
char *string;
{
double deg=0.0, min=0.0, sec=0.0;

    if (index(string, '°')) {
       sscanf(string, "%lf°%lf'%lf", &deg, &min, &sec);
       return deg+(min+sec/60.0)/60.0;
    } else
       return atof(string);
}

void
scanPosition(string, city)
char * string;
City * city;
{
int i, l;
char lat[80], lon[80];

     if (!string || !*string) return; 
     
     l = strlen(string);
     for (i=0; i<l; i++) if (string[i] == '|') string[i] = ' ';
     
     if (sscanf(string, "%s %s", lat, lon)<2) {
        city->lat = -100.0;
        return;
     }
     city->lat = dms2decim(lat);
     if (fabs(city->lat)>90.0) city->lat = -100.0;
     city->lon = dms2decim(lon);
     city->lon = fixangle(city->lon + 180.0) - 180.0;
}

City *
searchCityLocation(params)
char *params;
{
     City *c;
     char name[80], lat[80], lon[80];
     double dlat=0.0, dlon=0.0;
     int i, l, complete = 0;

     if (!params || !*params) return NULL;

     if (index(params, '|')) {
        l =strlen(params);
        for (i=0; i<l; i++) {
            if (params[i] == ' ') params[i]= '\037';
            if (params[i] == '|') params[i]= ' ';
	}
        sscanf(params, "%s %s %s", name, lat, lon);
        l = strlen(lat); lat[l-1] = '\0';
        l = strlen(lon); lon[l-1] = '\0';
	dlat = dms2decim(lat);
	dlon = dms2decim(lon);
        l = strlen(name);
	while (l>0 && name[l-1]=='\037') {
	   --l;
	   name[l] = '\0';
	}
        for (i=0; i<l; i++) if (name[i] == '\037') name[i] = ' '; 
        complete = 1;
     } else
        strncpy(name, params, 80);

     c = cityroot;
     while (c) {
        if ((!*name || !strcasecmp(c->name, name)) &&
            (!complete || (fabs(c->lon-dlon)<0.5 && fabs(c->lat-dlat)<0.5)))
	   return c;
        c = c->next;
     }
     return NULL;
}

void
freecity(c)
City * c;
{
     if (cityinit==c) cityinit = NULL;
     free(c->name);
     free(c);
}

void
removeCity(c)
City *c;
{
     City *cp, *cn;

     if (!c) return;  
     cn = c->next;
     freecity(c);

     if (c == cityroot) {
        cityroot = cn;
	return;
     }

     cp = cityroot;
     while (cp) {
        if (cp->next == c) {
	    cp->next = cn;
            if (c == citylast) citylast = cp;
	    return;
	}  
	cp = cp->next;
     }
}

City *
addCity(longparams)
char *longparams;
{
     City *city;
     char name[80], lat[80], lon[80], tz[80];
     char params[256];
     int i, size;

     i = 0;
     if (longparams) {
        while (longparams[i] != '\0') {
           if (longparams[i]==' ') longparams[i] = '\037';
           if (longparams[i]=='|') longparams[i] = ' ';
           ++i;
        }
        if (sscanf(longparams,"%d %s %s %s %s", &size, name, lat, lon, tz)<5) {
           fprintf(stderr, "Incorrect number of parameters in -addcity\n");
	   return NULL;
	}
        i = 0;
        while (name[i] != '\0') {
           if (name[i]=='\037') name[i] = ' ';
           ++i;
        }
     } else {
       if (do_urban) {
          if (!*urban_entry[0].string) {
	     strcpy(urban_entry[0].string, "???");
	     return NULL;
	  }
          strcpy(name, urban_entry[0].string);
          if (!*urban_entry[1].string) {
	     strcpy(urban_entry[1].string, "???");
	     return NULL;
	  }
          strcpy(tz, urban_entry[1].string);
          if (!*urban_entry[2].string) return NULL;
          strcpy(lat, urban_entry[2].string);
          if (!*urban_entry[3].string) return NULL;
          strcpy(lon, urban_entry[3].string);
          if (!*urban_entry[4].string) return NULL;
          if (!sscanf(urban_entry[4].string, "%d", &size)) {
	     strcpy(urban_entry[4].string, "?");
             return NULL;
	  }
       } else
	  return NULL;
     }
     
     if (citycheck || runlevel>READSYSRC) {
        sprintf(params, "%s\037|%s\037|%s\037", name, lat, lon);
	if ((city=searchCityLocation(params))) {
	   sprintf(params, Label[L_CITYWARNING1], name, lat, lon, "\n  ");
	   if (verbose || runlevel != READUSERRC)
	      fprintf(stderr, "%s\n", params);
	   if (do_urban) {
	      urban_newhint = '?';
	      showUrbanHint(params);
	      return NULL;
	   }
	   if (verbose || runlevel != READUSERRC) {
	      sprintf(params, Label[L_CITYWARNING2], name);
	      fprintf(stderr, "%s\n", params);
	   }
           removeCity(city);
	}
     }

     /* Create the record for the city */
     if ((city = (City *) calloc(1, sizeof(City))) == NULL) return NULL;     

     /* Set up the record */

     city->name = strdup(name);
     city->lat = dms2decim(lat);
     city->lon = dms2decim(lon);
     city->size = size;
     city->tz = strdup(tz);

     /* Link it into the list */

     if (!cityroot) 
        cityroot = citylast = city;
     else {
        citylast->next = city;
	citylast = city;
     }
     city ->next = NULL;
     return city;
}

void
deleteMarkedCity()
{
     City *c, *cp = NULL;

     if (!do_urban || !UrbanCaller) return;

     c = cityroot;
     while (c) {
        if (c == UrbanCaller->mark1.city) {
	   UrbanCaller->mark1.city = NULL;
	   if (cp) {
	      cp->next = c->next;
	   } else
	      cityroot = c->next;
	   if (c == citylast) citylast = cp;
	   if (c == UrbanCaller->lastmarked) UrbanCaller->lastmarked = NULL;
           freecity(c);
	   return;
	}
        cp = c;
        c = c->next;
     }
}

int 
readRC();

int
parseColor(s)
char * s;
{
int i, l, done = 0;
char *ptr = NULL;

    l = strlen(s);
    for (i=0; i<l; i++) {
        if (s[i]=='|') {
	   s[i] = '\0';
	   ptr = s+i+1;
	   break;
	}
    }
    l = i;
	
    if (ptr) {
       for (i=0; i<NUMPIXELS; i++)
          if (!strcasecmp(s, colorfield[i])) {
             StringReAlloc(&Color[i], ptr);
             done = 1;
	     break;
          }
       s[l] = '|';
    }

    if (!ptr || !done) {
       fprintf(stderr, "Incorrect -setcolor specification\n");
       return 1;
    }
    return 0;
}

int
parseFont(s)
char * s;
{
int i, l1 = 0, l2 = 0, done;
char *ptr1 = NULL, *ptr2 = NULL;

    done = strlen(s);
    for (i=0; i<done; i++) {
        if (s[i]=='|') {
	   s[i] = '\0';
	   if (!ptr1) {
	      ptr1 = s+i+1;
	      l1 = i;
	   } else {
	      ptr2 = s+i+1;
	      l2 = i;
  	      break;
	   } 
	}
    }
   
    /* Do not take font setting into account if selected language is
       not specified in list of languages */
    if (ptr2 && !strstr(ptr2, language)) return 0;
	
    if (!strcasecmp(s, "menu") && ptr1 && strcmp(ptr1, SunFont[4]))
       option_changes |= 1;
	
    done = 0;
    if (ptr1) {
       for (i=0; i<NUMFONTS; i++)
          if (!strcasecmp(s, fontfield[i])) {
             StringReAlloc(&SunFont[i], ptr1);
             done = 1;
	     break;
          }
       s[l1] = '|';
       if (l2) s[l2] = '|';
    }

    if (!ptr1 || !done) {
       fprintf(stderr, "Incorrect -setfont specification\n");
       return 1;
    }

    return 0;
}

int
parseArgs(argc, argv)
int                    argc;
char **                argv;
{
        int     opt;

        while (--argc > 0) {
                ++argv;
                if (!strcasecmp(*argv, "-verbose"))
                        verbose = 1;
                else if (!strcasecmp(*argv, "-reformat"))
                        reformat = 1;
                else if (!strcasecmp(*argv, "-silent"))
                        verbose = 0;
                else if (!strcasecmp(*argv, "-synchro"))
                        do_sync = 1;
                else if (!strcasecmp(*argv, "-nosynchro"))
                        do_sync = 0;
                else if (!strcasecmp(*argv, "-zoomsync"))
                        do_zoomsync = 1;
                else if (!strcasecmp(*argv, "-nozoomsync"))
                        do_zoomsync = 0;
                else if (!strcasecmp(*argv, "-animation"))
                        gflags.animate = 1;
                else if (!strcasecmp(*argv, "-noanimation"))
                        gflags.animate = 0;
                else if (!strcasecmp(*argv, "-coastlines"))
                        gflags.fillmode = 0;
                else if (!strcasecmp(*argv, "-contour"))
                        gflags.fillmode = 1;
                else if (!strcasecmp(*argv, "-landfill"))
                        gflags.fillmode = 2;
                else if (!strcasecmp(*argv, "-dottedlines"))
                        gflags.dotted = 0;
                else if (!strcasecmp(*argv, "-plainlines"))
                        gflags.dotted = 1;
                else if (!strcasecmp(*argv, "-bottomline"))
                        gflags.bottom = 1;
                else if (!strcasecmp(*argv, "-nobottomline"))
                        gflags.bottom = 0;
                else if (!strcasecmp(*argv, "-decimal"))
                        gflags.dms = 0;
                else if (!strcasecmp(*argv, "-dms"))
                        gflags.dms = gflags.dms = 1;
                else if (!strcasecmp(*argv, "-nonight"))
                        gflags.shading = 0;
                else if (!strcasecmp(*argv, "-night"))
                        gflags.shading = 1;
                else if (!strcasecmp(*argv, "-terminator"))
                        gflags.shading = 2;
                else if (!strcasecmp(*argv, "-twilight"))
                        gflags.shading = 3;
                else if (!strcasecmp(*argv, "-luminosity"))
                        gflags.shading = 4;
                else if (!strcasecmp(*argv, "-lightgradient"))
                        gflags.shading = 5;
                else if (!strcasecmp(*argv, "-tropics"))
                        gflags.parallel |= 8;
                else if (!strcasecmp(*argv, "-notropics"))
                        gflags.parallel &= 3;
                else if (!strcasecmp(*argv, "-sun"))
                        gflags.objects |= 1;
                else if (!strcasecmp(*argv, "-nosun"))
                        gflags.objects &= ~1;
                else if (!strcasecmp(*argv, "-moon"))
                        gflags.objects |= 2;
		else if (!strcasecmp(*argv, "-nomoon"))
                        gflags.objects &= ~2;
                else if (!strcasecmp(*argv, "-dock"))
                        do_dock = 1;
                else if (!strcasecmp(*argv, "-undock"))
                        do_dock = 0;
                else if (runlevel == RUNTIMEOPTION) {
                        if (needMore(&argc, argv)) return(1);
                        goto options_with_parameter;
		}
                else if (!strcasecmp(*argv, "-citycheck"))
                        citycheck = 1;
                else if (!strcasecmp(*argv, "-clock"))
                        win_type = 0;
                else if (!strcasecmp(*argv, "-map"))
                        win_type = 1;
                else if (!strcasecmp(*argv, "-screensaver") &&
			 runlevel <= PARSECMDLINE) {
                        screen_saver = 1;
			random_rootpos = 1;
			win_type = 1;
		}
                else if (!strcasecmp(*argv, "-noscreensaver") &&
			 runlevel <= PARSECMDLINE) {
                        screen_saver = 0;
			random_rootpos = 0;
		}
                else if (!strcasecmp(*argv, "-fixedrootpos"))
                        random_rootpos = 0;
                else if (!strcasecmp(*argv, "-randomrootpos"))
                        random_rootpos = 1;
                else if (!strcasecmp(*argv, "-menu"))
                        do_menu = 1;
                else if (!strcasecmp(*argv, "-nomenu"))
                        do_menu = 0;
                else if (!strcasecmp(*argv, "-filesel"))
                        do_filesel = 1;
                else if (!strcasecmp(*argv, "-nofilesel"))
                        do_filesel = 0;
                else if (!strcasecmp(*argv, "-zoom"))
                        do_zoom = 1;
                else if (!strcasecmp(*argv, "-nozoom"))
                        do_zoom = 0;
                else if (!strcasecmp(*argv, "-option"))
                        do_option = 1;
                else if (!strcasecmp(*argv, "-nooption"))
                        do_option = 0;
                else if (!strcasecmp(*argv, "-urban"))
                        do_urban = 1;
                else if (!strcasecmp(*argv, "-nourban"))
                        do_urban = 0;
                else if (!strcasecmp(*argv, "-help")) {
		        if (runlevel == PARSECMDLINE) {
			   Usage();
			   exit(0);
			}
		}
                else if (!strcasecmp(*argv, "-listmenu")) {
                        if (runlevel == PARSECMDLINE) { 
                           ListMenu();
			   exit(0);
			}
                }
                else if (!strcasecmp(*argv, "-version")) {
                        fprintf(stderr, "%s: version %s, %s\n",
                                ProgName, VERSION, COPYRIGHT);
                        if (runlevel == PARSECMDLINE) 
                          exit(0);
                        else
                          return(0);
		}
             else {
                if (needMore(&argc, argv)) return(1);
                if (!strcasecmp(*argv, "-display"))
                        StringReAlloc(&Display_name, *++argv); 
                else if (!strcasecmp(*argv, "-sharedir")) {
                        StringReAlloc(&share_maps_dir, *++argv);
                        strncpy(image_dir, *argv, 1020);
		}
                else if (!strcasecmp(*argv, "-citycategories")) {
                        city_cat = atoi(*++argv);
			if (city_cat <= 0) city_cat = 1;
			if (city_cat > 100) city_cat = 100;
		}
                else 
	        options_with_parameter :
                     if (!strcasecmp(*argv, "-rcfile")) {
		        if (runlevel == RUNTIMEOPTION) {
			   runlevel = READUSERRC;
                           if (readRC(*++argv)) runlevel = FAILEDOPTION;
                           if (runlevel != FAILEDOPTION) 
                              runlevel = RUNTIMEOPTION;
			}
		}
                else if (!strcasecmp(*argv, "-language")) {
                        strncpy(language, *++argv, 2);
		        language[2] = '\0';
			if (strcmp(language, oldlanguage)) readLanguage();
                } 
	        else if (!strcasecmp(*argv, "-title"))
                        StringReAlloc(&Title, *++argv);
	        else if (!strcasecmp(*argv, "-clockclassname"))
                        StringReAlloc(&ClockClassName, *++argv);
	        else if (!strcasecmp(*argv, "-mapclassname"))
                        StringReAlloc(&MapClassName, *++argv);
	        else if (!strcasecmp(*argv, "-auxilclassname"))
                        StringReAlloc(&AuxilClassName, *++argv);
	        else if (!strcasecmp(*argv, "-classname"))
                        StringReAlloc(&ClassName, *++argv);
                else if (!strcasecmp(*argv, "-colorlevel")) {
                        gflags.colorlevel = atoi(*++argv);
			if (gflags.colorlevel < 0) gflags.colorlevel = 0;
			if (gflags.colorlevel >= FULLCOLORS) {
                           gflags.colorlevel = FULLCOLORS;
			   gflags.fillmode = 2;
			} else
 			   gflags.fillmode = 1;
  	        }
		else if (!strcasecmp(*argv, "-vmfflags")) {
		        gflags.vmfflags = atoi(*++argv);
			option_changes |= 4;			
		}
		else if (!strcasecmp(*argv, "-vmfrange")) {
                        StringReAlloc(&vmfrange, *++argv);
		}
		else if (!strcasecmp(*argv, "-vmfcoordformat")) {
                        StringReAlloc(&vmfcoordformat, *++argv);
		}
		else if (!strcasecmp(*argv, "-vmfcolors")) {
                        StringReAlloc(&vmfcolors, *++argv);
                        if (!strcmp(vmfcolors, "|")) {
			   free(vmfcolors);
			   vmfcolors = NULL;
			}
			option_changes |= 4;
		}
		else if (!strcasecmp(*argv, "-clockgeom")) {
                        getGeom(*++argv, &ClockGeom);
			option_changes |= 8;
                }
                else if (!strcasecmp(*argv, "-mapgeom")) {
                        getGeom(*++argv, &MapGeom);
			option_changes |= 16;
                }
                else if (!strcasecmp(*argv, "-image")) {
                        StringReAlloc(&Clock_img_file, *++argv);
                        StringReAlloc(&Map_img_file, *argv);
			option_changes |= 32|64;
                }
                else if (!strcasecmp(*argv, "-clockimage")) {
                        StringReAlloc(&Clock_img_file, *++argv);
			option_changes |= 32;
                }
                else if (!strcasecmp(*argv, "-mapimage")) {
                        StringReAlloc(&Map_img_file, *++argv);
			option_changes |= 64;
                }
                else if (!strcasecmp(*argv, "-zoomimage")) {
		        StringReAlloc(&Zoom_img_file, *++argv);
		        option_changes |= 2;
		}
                else if (!strcasecmp(*argv, "-auxilgeom")) {
                        getGeom(*++argv, &MenuGeom);
			option_changes |= 2;
			ZoomGeom.x = FileselGeom.x 
                                   = OptionGeom.x 
                                   = UrbanGeom.x 
                                   = MenuGeom.x;
			ZoomGeom.y = FileselGeom.y 
                                   = OptionGeom.y 
                                   = UrbanGeom.y 
                                   = MenuGeom.y;
                }
                else if (!strcasecmp(*argv, "-menugeom")) {
                        getGeom(*++argv, &MenuGeom);
			option_changes |= 2;
                }
                else if (!strcasecmp(*argv, "-selgeom")) {
                        getGeom(*++argv, &FileselGeom);
			option_changes |= 2;
                }
                else if (!strcasecmp(*argv, "-zoomgeom")) {
                        getGeom(*++argv, &ZoomGeom);
			option_changes |= 2;
                }
                else if (!strcasecmp(*argv, "-optiongeom")) {
                        getGeom(*++argv, &OptionGeom);
			option_changes |= 2;
                }
                else if (!strcasecmp(*argv, "-urbangeom")) {
                        getGeom(*++argv, &UrbanGeom);
			option_changes |= 2;
                }
                else if (!strcasecmp(*argv, "-mag")) {
                        gzoom.fx = atof(*++argv);
                        if (gzoom.fx < 1) gzoom.fx = 1.0;
                        if (gzoom.fx > 100.0) gzoom.fx = 100.0;
                        gzoom.fy = gzoom.fx;
			option_changes |= 4;
                }
                else if (!strcasecmp(*argv, "-magx")) {
                        gzoom.fx = atof(*++argv);
                        if (gzoom.fx < 1) gzoom.fx = 1.0;
			option_changes |= 4;
                }
                else if (!strcasecmp(*argv, "-magy")) {
                        gzoom.fy = atof(*++argv);
                        if (gzoom.fy < 1) gzoom.fy = 1.0;
			option_changes |= 4;
                }
                else if (!strcasecmp(*argv, "-dx")) {
                        gzoom.fdx = atof(*++argv)/360.0+0.5;
                        checkZoomSettings(&gzoom);
			option_changes |= 4;
                }
                else if (!strcasecmp(*argv, "-dy")) {
                        gzoom.fdy = 0.5-atof(*++argv)/180.0;
                        checkZoomSettings(&gzoom);
			option_changes |= 4;
                }
                else if (!strcasecmp(*argv, "-rootdx"))
		        rootdx = atof(*++argv);
                else if (!strcasecmp(*argv, "-rootdy"))
		        rootdy = atof(*++argv);
                else if (!strcasecmp(*argv, "-setfont"))
                        parseFont(*++argv); 
                else if (!strcasecmp(*argv, "-mapmode")) {
                        if (!strcasecmp(*++argv, "c")) 
                           gflags.map_mode = COORDINATES;
                        if (!strcasecmp(*argv, "d")) 
                           gflags.map_mode = DISTANCES;
                        if (!strcasecmp(*argv, "e")) 
                           gflags.map_mode = EXTENSION;
                        if (!strcasecmp(*argv, "l")) {
                           StringReAlloc(&CityInit, NULL);
                           gflags.map_mode = LEGALTIME;
                        }
                        if (!strcasecmp(*argv, "s")) 
                           gflags.map_mode = SOLARTIME;
                }
                else if (!strcasecmp(*argv, "-parallelmode")) {
                        opt = atoi(*++argv);
                        if (opt<0) opt = 0;
                        if (opt>3) opt = 3;
			gflags.parallel = opt + (gflags.parallel & 8);
                } 
		else if (!strcasecmp(*argv, "-parallelspacing")) {
                        gzoom.paralspacing = atof(*++argv);
                        if (gzoom.paralspacing<0) gzoom.paralspacing = 0;
                        if (gzoom.paralspacing>30.0) gzoom.paralspacing = 30.0;
                        if (gzoom.paralspacing<0.1) gzoom.paralspacing = 0.1;
                } 
		else if (!strcasecmp(*argv, "-meridianmode")) {
                        gflags.meridian = atoi(*++argv);
                        if (gflags.meridian<0) gflags.meridian = 0;
                        if (gflags.meridian>3) gflags.meridian = 3;
                } 
		else if (!strcasecmp(*argv, "-meridianspacing")) {
                        gzoom.meridspacing = atof(*++argv);
                        if (gzoom.meridspacing<0) gzoom.meridspacing = 0;
                        if (gzoom.meridspacing>30.0) gzoom.meridspacing = 30.0;
                        if (gzoom.meridspacing<0.1) gzoom.meridspacing = 0.1;
                } 
		else if (!strcasecmp(*argv, "-citymode")) {
                        gflags.citymode = atoi(*++argv);
                        if (gflags.citymode<0) gflags.citymode = 0;
                        if (gflags.citymode>3) gflags.citymode = 3;
                } 
		else if (!strcasecmp(*argv, "-objectmode")) {
                        gflags.objectmode = atoi(*++argv);
                        if (gflags.objectmode<0) gflags.objectmode = 0;
                        if (gflags.objectmode>=2) gflags.objectmode = 2;
		}
                else if (!strcasecmp(*argv, "-spotsizes"))
                        StringReAlloc(&SpotSizes, *++argv);
                else if (!strcasecmp(*argv, "-sizelimits"))
                        StringReAlloc(&SizeLimits, *++argv);
                else if (!strcasecmp(*argv, "-fillmode")) {
                        gflags.fillmode = atoi(*++argv);
                        if (gflags.fillmode<0) gflags.fillmode = 0;
                        if (gflags.fillmode>3) gflags.fillmode = 3;
                }
                else if (!strcasecmp(*argv, "-darkness")) {
                        darkness = atof(*++argv);
                        if (darkness<0.0) darkness = 0.0;
                        if (darkness>1.0) darkness = 1.0;
                }
                else if (!strcasecmp(*argv, "-diffusion")) {
                        atm_diffusion = atof(*++argv);
                        if (atm_diffusion<0.0) atm_diffusion = 0.0;
                }
                else if (!strcasecmp(*argv, "-refraction")) {
                        atm_refraction = atof(*++argv);
                        if (atm_refraction<0.0) atm_refraction = 0.0;
                }
                else if (!strcasecmp(*argv, "-colorscale")) {
			opt = atoi(*++argv);
			if (opt<=0) opt = 1;
			if (opt>32767) opt = 32767;
                        gflags.colorscale = opt;
                }
		else if (!strcasecmp(*argv, "-setcolor")) {
		        if (parseColor(*++argv)) return(1);
		}
                else if (!strcasecmp(*argv, "-addcity")) 
		        (void) addCity(*++argv);
		else if (!strcasecmp(*argv, "-removecity")) {
		        City * c = searchCityLocation(*++argv);
		        removeCity(c);
		}
		else if (!strcasecmp(*argv, "-position")) {
                        StringReAlloc(&CityInit, NULL);
                        scanPosition(*++argv, &position);
			if (position.lat < -90.0) {
			  fprintf(stderr, 
                             "Error in -position parameters\n");
			  return(1);
			}
		}
                else if (!strcasecmp(*argv, "-city")) {
                        StringReAlloc(&CityInit, *++argv);
                        position.lat = 100.0;
                        gflags.map_mode = COORDINATES;
                }
                else if (!strcasecmp(*argv, "-placement")) {
		        option_changes |= 2;
                        if (strcasecmp(*++argv, "random")==0)
                           placement = RANDOM;
                        if (strcasecmp(*argv, "fixed")==0) {
                           placement = FIXED;
                           MapGeom.mask = XValue | YValue | 
                                          WidthValue | HeightValue;
                        }
                        if (!strcasecmp(*argv, "center"))
                           placement = CENTER;
                        if (!strcasecmp(*argv, "nw"))
                           placement = NW;
                        if (!strcasecmp(*argv, "ne"))
                           placement = NE;
                        if (!strcasecmp(*argv, "sw"))
                           placement = SW;
                        if (!strcasecmp(*argv, "se"))
                           placement = SE;
                }
                else if (!strcasecmp(*argv, "-extrawidth"))
			extra_width = atol(*++argv);
                else if (!strcasecmp(*argv, "-placementshift")) {
		        option_changes |= 2;
			if (sscanf(*++argv, "%d %d", 
                            &place_shiftx, &place_shifty) < 2) {
			  fprintf(stderr, 
                             "Error in -placementshift parameters\n");
			  return(1);
			}
		}
                else if (!strcasecmp(*argv, "-command"))
                        StringReAlloc(&ExternAction, *++argv);
                else if (!strcasecmp(*argv, "-editorcommand"))
                        StringReAlloc(&EditorCommand, *++argv);
                else if (!strcasecmp(*argv, "-dateformat"))
                        StringReAlloc(&ListFormats, *++argv);
                else if (!strcasecmp(*argv, "-shading")) {
                        gflags.shading = atoi(*++argv);
                        if (gflags.shading < 0) gflags.shading = 0;
                        if (gflags.shading > 5) gflags.shading = 5;
                }
                else if (!strcasecmp(*argv, "-progress") ||
                         !strcasecmp(*argv, "-jump")) {
                        char *str, *invalid, c;
                        long value;
			opt = ((*argv)[5]!='\0');
                        str=*++argv;
                        value = strtol(str, &invalid, 10);
                        if (invalid) 
                           c = *invalid;
			else
			   c = 's';
                        if (c>='0' && c<='9') c='s';
                        switch(c) {
                          case 's': break;
                          case 'm': value *= 60 ; break;
                          case 'h': value *= 3600 ; break;
                          case 'd': value *= 86400 ; break;
                          case 'M': value *= 2592000 ; break;
                          case 'y':
                          case 'Y': value *= 31536000 ; break;
                          default : c = ' '; break;
                        }
                        if (c == ' ') Usage();
                        if (opt) {
                           progress_value[5] = abs(value); 
                           if (value) 
                              gflags.progress = 5;
                           else
                              gflags.progress = 0;
                        } else 
                           time_jump = value;
                }
                else if (!strcasecmp(*argv, "-rootperiod")) {
                        root_period = atoi(*++argv);
                        if (root_period<=0) root_period = 1;
                        if (root_period>120) root_period = 120;
                }
                else if (!strcasecmp(*argv, "-animateperiod")) {
                        gflags.animperiod = atoi(*++argv);
                        if (gflags.animperiod<0) gflags.animperiod = 0;
                        if (gflags.animperiod>5) gflags.animperiod = 5;
                }
                else if (!strcasecmp(*argv, "-aspect")) {
                        gzoom.mode = atoi(*++argv);
                        if (gzoom.mode<0) gzoom.mode = 0;
                        if (gzoom.mode>2) gzoom.mode = 2;
                }
	        else {
                   fprintf(stderr, "%s: unknown option !!\n\n", *argv);
                   if (runlevel == PARSECMDLINE) {
                        Usage();
                        exit(1);
                   } 
		   else if (runlevel == RUNTIMEOPTION) {
                        fprintf(stderr, "Option %s : incorrect or not "
				        "available at runtime !!\n", *argv);
                        runlevel = FAILEDOPTION;
		   }
                   else {
                        fprintf(stderr, "Trying to recover.\n");
                        return(1);
                   }
		}
	     }
	}
        return(0);
}

int
parseCmdLine(buf)
char * buf;
{
    int i, j, l;
    char *dup, *str;
    char ** argv;
    int argc;

    l = strlen(buf);
    dup = (char *) salloc((l+2)*sizeof(char));
    if (dup) {
       if (*buf == '-')
          strcpy(dup, buf);
       else {
	  strcpy(dup+1, buf);
	  *dup = '-';
       }
    } else return 1;

    j = 0;
    for (i=0 ; dup[i] ; ++i)
       if (isspace(dup[i])) ++j;

    argv = (char **) salloc((j+2)*sizeof(char *));

    i = argc = 0;

 rescan:
    while(dup[i] && isspace(dup[i])) {
      dup[i] = '\0';
      ++i;
    }

    if (dup[i]) {
       str = argv[argc] = dup+i;
       while(dup[i] && !isspace(dup[i])) ++i;
       if (str[0]==':' && (!str[1] || isspace(str[1]))) {
	  str[0] = '\0';
	  goto rescan;
       }
       ++argc;
       goto rescan;
    } else
       argv[argc] = NULL;

    for (i=0; i<l+1; i++)
       if (dup[i] && dup[i]<' ') dup[i] = ' ';

    for (i=0; i<argc; i++) {
       str = argv[i];
       l = strlen(str)-1;
       if (*str == '-' && str[l] == ':') str[l] = '\0';
    }

    l = parseArgs(argc+1, argv-1);

    free(dup);
    free(argv);
    return l;
}

/*
 * readRC() - Read a config file (app-default or user .sunclockrc or whatever)
 */

int 
readRC(fname, verbosity)
char *fname;        /* Path to .sunclockrc file */
int verbosity;
{
    /*
     * Local Variables
     */

    FILE *rc;           /* File pointer for rc file */
    char buf[1028];     /* Buffer to hold input lines */
    int  j;

    /* Open the RC file */

    if ((rc = fopen(fname, "r")) == NULL) {
        if (verbosity)
           fprintf(stderr, 
               "Unable to find the config file  %s \n", fname);
        return(1);
    }

    /* Read and parse lines from the file */

    while (fgets(buf, 1024, rc)) {

        /* Look for blank lines or comments */

        j=0;
        while (j<1024 && isspace(buf[j]) && buf[j] != '0') ++j; 
        if ((buf[j] == '#') || (buf[j] == '\0')) continue;

        if (parseCmdLine(buf))
	   fprintf(stderr,"Recheck syntax of config file %s !!\n\n", fname);
           continue;
        }

    if (rc) fclose(rc);
    return(0);
}

struct Sundata *
getContext(win)
Window win;
{
   struct Sundata * Context;
   if (win==Menu) 
     return MenuCaller;
   if (win==Filesel)
     return FileselCaller;
   if (win==Zoom)
     return ZoomCaller;
   if (win==Option)
     return OptionCaller;
   if (win==Urban)
     return UrbanCaller;

   Context = Seed;
   while (Context && Context->win != win) Context = Context->next;
   return Context;
}

XFontStruct *
getFont(num)
int num;
{
XFontStruct * font;

    font = XLoadQueryFont(dpy, SunFont[num]);
    if (font == (XFontStruct *)NULL)
       fprintf(stderr, "%s: can't open font `%s', using `%s'\n",
                ProgName, SunFont[num], FAILFONT);
    else
       return font;
    font = XLoadQueryFont(dpy, FAILFONT);
    if (font == (XFontStruct *)NULL) {
       fprintf(stderr, "%s: can't open font `%s', giving up\n",
               ProgName, FAILFONT);
       return NULL;
    }
    return font;
}

void
getFonts(Context)
Sundata * Context;
{
int i, h, hp;

        for (i=0; i<NUMFONTS; i++)
	    if (!(Context->gdata->font[i] = getFont(i))) exit(1);

        Context->gdata->clockstrip = 
            Context->gdata->font[CLOCKSTRIPFONT]->max_bounds.ascent +
            Context->gdata->font[CLOCKSTRIPFONT]->max_bounds.descent + 4;

        Context->gdata->mapstrip = 
            Context->gdata->font[MAPSTRIPFONT]->max_bounds.ascent + 
            Context->gdata->font[MAPSTRIPFONT]->max_bounds.descent + 8;

        Context->gdata->menustrip = 
            Context->gdata->font[MENUFONT]->max_bounds.ascent + 
            Context->gdata->font[MENUFONT]->max_bounds.descent + 8;

	Context->gdata->charspace = Context->gdata->menustrip+4;

	if (option_changes & 1) {
	    FileselGeom.width = SEL_WIDTH * Context->gdata->menustrip;
            FileselGeom.height = (11+4*SEL_HEIGHT)*Context->gdata->menustrip/5;
	}

	if (Context->flags.colorlevel < FULLCOLORS) return;

        h = Context->gdata->font[CITYFONT]->max_bounds.ascent + 
            Context->gdata->font[CITYFONT]->max_bounds.descent;
        hp = Context->gdata->font[COORDFONT]->max_bounds.ascent + 
             Context->gdata->font[COORDFONT]->max_bounds.descent;
        if (hp>h) h = hp;
	if (h>textheight) {
           textheight = h;
	   if (textpix) {
	      XFreePixmap(dpy, textpix);
	      textpix = 0;
	   }
	}
        if (!textpix)
           textpix = XCreatePixmap(dpy, Root, textwidth, textheight, 1);
}

unsigned long 
getColor(Context, num)
Sundata *        Context;
int              num;
{

        XColor                  c;
        XColor                  e;
        register Status         s;

        ++total_colors;

        if (Context->flags.colorlevel == MONOCHROME) goto monochrome;

        s = XAllocNamedColor(dpy, Context->gdata->cmap, Color[num], &c, &e);

        if (s != (Status)1) {
            fprintf(stderr, "%s: warning: can't allocate color `%s'\n",
                    ProgName, Color[num]);
            color_alloc_failed = 1;
 	 monochrome:
	    s = XAllocNamedColor(dpy, Context->gdata->cmap, 
                (num<FALLBACKTOWHITE)? "White" : "Black", &c, &e);
	} 
         
        return(c.pixel);
}

void
createGData(Context, private)
struct Sundata * Context;
int private;
{
        GraphicData *           graphdata;             
        Sundata *               OtherContext;
        int                     prec, i, j;

        /* Try to use already defined GCs and Pixels in default colormap
           if already defined */

        if (!private && runlevel == RUNNING) {

	   graphdata = NULL;
  	   prec = -1;

           for (OtherContext = Seed; OtherContext; 
                OtherContext = OtherContext->next) {
              if (OtherContext != Context &&
                  OtherContext->flags.colorlevel == gflags.colorlevel &&
                  OtherContext->gdata->cmap == cmap0 && 
                  OtherContext->gdata->precedence > prec) {
	         graphdata = OtherContext->gdata;
	         prec = OtherContext->gdata->precedence;
              }
           } 

	   if (graphdata) {
              Context->gdata = graphdata;
              ++Context->gdata->links;
	      return;
	   }
	}

    newcmap:

        /* Otherwise, define new adhoc graphical data */
        Context->gdata = (GraphicData *)salloc(sizeof(GraphicData));
        Context->gdata->wingc = 0;
        Context->gdata->pixgc = 0;
        Context->gdata->links = 0;
	Context->gdata->precedence = precedence;
	++precedence;
        if (color_depth>8 || Context->flags.colorlevel==MONOCHROME ||
           (Context->flags.colorlevel>MONOCHROME && !private))
           Context->gdata->cmap = cmap0;
        else
           Context->gdata->cmap =
                XCreateColormap(dpy, Root, visual, AllocNone);

        color_alloc_failed = 0;

        total_colors = 0;
        for (i=0; i<NUMPIXELS; i++)
            Context->gdata->pixel[i] = getColor(Context, i);

        if (color_depth<=8 && color_alloc_failed &&
           Context->gdata->cmap==cmap0) {
              private = 1;
              fprintf(stderr, 
                 "Color allocation failed with default colormap.\n"
                 "Trying instead private colormap...\n");
              goto newcmap;
	}

        if (color_alloc_failed) 
           fprintf(stderr, "Color allocation failed !!!\n");
	else
	   if (verbose)
              fprintf(stderr, "Color allocation successful:\n");

        Context->gdata->usedcolors = total_colors;
        for (i=0; i<total_colors; i++) {
           for (j=0; j<i; j++) if (Context->gdata->pixel[i]==Context->gdata->pixel[j]) { 
              --Context->gdata->usedcolors; 
              break;
           }
        }
        if (verbose && !color_alloc_failed)
           fprintf(stderr, 
              "  %d basic colors allocated in %s colormap.\n",
              Context->gdata->usedcolors,
              (Context->gdata->cmap==cmap0)? "default":"private");
}

void
createGCs(Context)
struct Sundata * Context;
{
        XGCValues               gcv;
	int                     mask;

        if (!Context->mappix && (Context->flags.colorlevel<FULLCOLORS))
           Context->mappix = XCreatePixmap(dpy, Root, 
              Context->geom.width, Context->geom.height, 1);

        if (Context->gdata->links==0) {
           if (verbose)
              fprintf(stderr, "Creating new GCs, mode = %d\n", 
                     Context->flags.colorlevel);
           getFonts(Context);
        } else {
           if (verbose)
              fprintf(stderr, "Using previous GC settings (%d links)\n", 
                     Context->gdata->links);
	   return;
        }

        mask = GCForeground | GCBackground | GCFont;
	gcv.background = white;
	gcv.foreground = black;

        if (!Context->gdata->wingc) {
           gcv.font = Context->gdata->font[MENUFONT]->fid;
           Context->gdata->wingc = XCreateGC(dpy, Root, mask, &gcv);
	}

        gcv.font = Context->gdata->font[COORDFONT]->fid;
        if (Context->flags.colorlevel == FULLCOLORS) {
           gcv.background = black;
	   gcv.foreground = white;
           Context->gdata->pixgc = XCreateGC(dpy, textpix, mask, &gcv);
        } else {
           mask |= GCFunction;
	   gcv.function = GXinvert;
           Context->gdata->pixgc = XCreateGC(dpy, Context->mappix, mask, &gcv);
	}
}

void
clearStrip(Context)
struct Sundata * Context;
{
        XSetForeground(dpy, Context->gdata->wingc, 
		    Context->gdata->pixel[CLOCKSTRIPBGCOLOR+Context->wintype]);
        XFillRectangle(dpy, Context->win, Context->gdata->wingc, 
              0, Context->geom.height+(Context->flags.bottom&1)-1, 
              Context->geom.width, 
              Context->hstrip-(Context->flags.bottom&1)+1);
}

/*
 *  Set the timezone of selected location.
 *  This is definitely the most tricky point in the whole sunclock stuff
 *  because of the incompatible Unix implementations !
 */

void
setTZ(cptr)
City    *cptr;
{
#ifndef _OS_LINUX_
        char buf[80];
#endif

        if (cptr && cptr->tz) {
#ifdef _OS_LINUX_
           setenv("TZ", cptr->tz, 1);
#else
           sprintf(buf, "TZ=%s", cptr->tz);
#ifdef _OS_HPUX_
           putenv(strdup(buf));
#else
           putenv(buf);
#endif
#endif
           } 
        else
#ifdef _OS_LINUX_
           unsetenv("TZ");
#else
           {
           /* This is supposed to reset timezone to default localzone */
           strcpy(buf, "TZ");
           /* Another option would be to set LOCALZONE adequately and put:
           strcpy(buf, "TZ="LOCALZONE); */
#ifdef _OS_HPUX_
           putenv(strdup(buf));
#else
           putenv(buf);
#endif
           }
#endif
        tzset();
}

/*
 *   Sets sun position (longitude, declination) and returns
 *   solartime at given city
 *   CAUTION: sunlong is in fact given as longitude+180 in interval 0..360
 */

time_t
sunParams(gtime, sunlong, sundec, city)
time_t gtime;
double *sunlong;
double *sundec;
City *city;
{
        struct tm               ctp, stp;
        time_t                  stime;
        double                  jt, gt;
        double                  sunra, sunrv, junk;
        long                    diff;

        ctp = *gmtime(&gtime);
        jt = jtime(&ctp);
        sunpos(jt, False, &sunra, sundec, &sunrv, &junk);
        gt = gmst(jt);
        *sunlong = fixangle(180.0 + (sunra - (gt * 15))); 
        
        if (city) {
           stime = (long) ((city->lon - *sunlong) * 240.0);
           stp = *gmtime(&stime);
           diff = stp.tm_sec-ctp.tm_sec
                  +60*(stp.tm_min-ctp.tm_min)+3600*(stp.tm_hour-ctp.tm_hour);
           if (city->lon>0.0) while(diff<-40000) diff += 86400;
           if (city->lon<0.0) while(diff>40000) diff -= 86400;
           stime = gtime+diff;
        } else
           stime = 0;
        return(stime);
}

/* get "daylength" at a location of latitude lat, when it is noon at
   GMT time gt at that location */

double
dayLength(gtime, lat)
time_t  gtime;
double  lat;
{
        double                  duration;
        double                  sundec, junk;
        double                  sinsun, sinpos, sinapp, num;

        sinpos = sin(torad(lat));

        /* Get Sun declination */
        (void) sunParams(gtime, &junk, &sundec, NULL);
        sinsun = sin(torad(sundec));

        /* Correct for the sun apparent diameter and atmospheric diffusion */
        sinapp = sin(torad(SUN_APPRADIUS + atm_refraction));

        num = 1 - sinsun*sinsun - sinpos*sinpos - sinapp*sinapp
                - 2*sinsun*sinpos*sinapp;
        if (num<=0) {
           if (sinsun*sinpos>0) 
             duration = 24.0;
           else
             duration = 0.0;
        } else
             duration = 12.0 + 24.0*atan((sinsun*sinpos+sinapp)/sqrt(num))/PI;

        return duration*3600;
}

void
setDayParams(Context)
struct Sundata * Context;
{
        struct tm               gtm, ltm;
        double                  duration, junk;
        time_t                  gtime, stime, sr, ss, dl;

        menu_lasthint = ' ';

        if (!Context->mark1.city) return;

        time(&Context->time);
        gtime = Context->time + Context->jump;

        /* Get local time at given location */
        setTZ(Context->mark1.city);
        ltm = *localtime(&gtime);

        /* Get solar time at given location */
        stime = sunParams(gtime, &junk, &junk, Context->mark1.city);

        /* Go to time when it is noon in solartime at Context->mark1.city */
        gtime += 43200 - (stime % 86400);
        gtm = *gmtime(&gtime);

        if (gtm.tm_mday < ltm.tm_mday) gtime +=86400;
        if (gtm.tm_mday > ltm.tm_mday) gtime -=86400;
       
        /* Iterate, just in case of a day shift */
        stime = sunParams(gtime, &junk, &junk, Context->mark1.city);

        gtime += 43200 - (stime % 86400);

        /* get day length at that time and location*/
        duration = dayLength(gtime, Context->mark1.city->lat);
  
        /* compute sunrise and sunset in legal time */
        sr = gtime - (time_t)(0.5*duration);
        ss = gtime + (time_t)(0.5*duration);
        dl = ss-sr;
        Context->mark1.full = 1;
        if (dl<=0) {dl = 0; Context->mark1.full = 0;}
        if (dl>86380) {dl=86400; Context->mark1.full = 0;}

        Context->mark1.dl = *gmtime(&dl);
        Context->mark1.dl.tm_hour += (Context->mark1.dl.tm_mday-1) * 24;

        setTZ(Context->mark1.city);
        Context->mark1.sr = *localtime(&sr);
        Context->mark1.ss = *localtime(&ss);
}

char *
num2str(value, string, dms)
double value;
char *string;
short dms;
{
        int eps, d, m, s;

        if (dms) {
          if (value<0) {
            value = -value; 
            eps = -1;
          } else
            eps = 1;
	  value = value+1/7200.0;
          d = (int) value;
          value = 60 * (value - d);
          m = (int) value;
          value = 60 * (value - m);       
          s = (int) value;
          sprintf(string, "%s%d°%02d'%02d\"", (eps==1)?"":"-", d, m, s);
        } else
          sprintf(string, "%.3f", value); 
        return string;
}

/*
 *  Produce bottom strip of hours
 */

void
showHours(Context)
struct Sundata * Context;
{
        int i, x;
        char s[128];

        if (Context->flags.hours_shown) return;
        clearStrip(Context);
        for (i=0; i<24; i++) {
            sprintf(s, "%d", i);
            x = ((i*Context->zoom.width)/24 + 2*Context->zoom.width
                - (Context->gdata->mapstrip+5)*strlen(s)/8 
                + (int)(Context->sunlon*Context->zoom.width/360.0))%
                  Context->zoom.width + 1 - Context->zoom.dx;
            if (x>=0 && x<Context->geom.width) {
               XSetBackground(dpy, Context->gdata->wingc, 
                   Context->gdata->pixel[MAPSTRIPBGCOLOR]);
               XSetForeground(dpy, Context->gdata->wingc, 
                   Context->gdata->pixel[MAPSTRIPFGCOLOR]);
               XSetFont(dpy, Context->gdata->wingc, 
                   Context->gdata->font[MAPSTRIPFONT]->fid);
               XDrawImageString(dpy, Context->win, 
                  Context->gdata->wingc, 
                  x, Context->gdata->font[MENUFONT]->max_bounds.ascent + 
                     Context->geom.height + 4, 
                  s, strlen(s));
	    }
        }
        Context->flags.hours_shown = 1;
}


void
drawTextStrip(Context, s, l)
struct Sundata * Context;
char *s;
int l;
{
      XSetBackground(dpy, Context->gdata->wingc, 
          Context->gdata->pixel[CLOCKSTRIPBGCOLOR+Context->wintype]);
      XSetForeground(dpy, Context->gdata->wingc, 
          Context->gdata->pixel[CLOCKSTRIPFGCOLOR+Context->wintype]);
      XSetFont(dpy, Context->gdata->wingc, 
             (Context->wintype)? 
	       Context->gdata->font[MAPSTRIPFONT]->fid : 
	       Context->gdata->font[CLOCKSTRIPFONT]->fid);
      XDrawImageString(dpy, Context->win, Context->gdata->wingc,
           2+2*Context->wintype, Context->geom.height + ((Context->wintype)?
             Context->gdata->font[MAPSTRIPFONT]->max_bounds.ascent + 4 :
             Context->gdata->font[CLOCKSTRIPFONT]->max_bounds.ascent + 3), 
           s+label_shift, l-label_shift);
}

void
writeStrip(Context)
struct Sundata * Context;
{
        register struct tm      ltp;
        register struct tm      gtp;
        register struct tm      stp;
        time_t                  gtime;
        time_t                  stime;
        int                     i, l;
        char            s[128];
        char            slat[20], slon[20], slatp[20], slonp[20];
        double          dist;
#ifdef NEW_CTIME
        struct timeb            tp;

        if (ftime(&tp) == -1) {
                fprintf(stderr, "%s: ftime failed: ", ProgName);
                perror("");
                exit(1);
        }
#endif

	if (!Context->flags.mapped) return;

        time(&Context->time);
        gtime = Context->time + Context->jump;

        if (!Context->wintype) {
                char num[80];
                int hour; 
                char ampm;

                setTZ(NULL);
                ltp = *localtime(&gtime);
                gtp = *gmtime(&gtime);	   
	   
	        hour = ltp.tm_hour;
	        if (hour<12)
	           ampm = 'A';
	        else
	           ampm = 'P';
                if (hour > 12)
                   hour -= 12;
                l = strlen(DateFormat[Context->flags.clock_mode]);
                *s = '\0';
                for (i=0; i<l; i++) {
                   char c = DateFormat[Context->flags.clock_mode][i];
                   if (c != '%') { 
                      num[0] = c;
                      num[1] = '\0'; 
                   }
                   if (c == '%' && i<l-1) {
                      ++i; 
                      c = DateFormat[Context->flags.clock_mode][i];
                      switch(c) {
                        case 'G': sprintf(num, "%02d", gtp.tm_hour); break;			 
                        case 'H': sprintf(num, "%02d", ltp.tm_hour); break;
                        case 'M': sprintf(num, "%02d", ltp.tm_min); break;
                        case 'N': sprintf(num, "%02d", gtp.tm_min); break;			 
                        case 'P': num[0]=ampm; num[1]='\0'; break;
                        case 'S': sprintf(num, "%02d", ltp.tm_sec); break;
#ifdef NEW_CTIME
                        case 'Z': strcpy(num, ltp.tm_zone); break;
#else
                        case 'Z': strcpy(num, tzname[ltp.tm_isdst]); break;
#endif
                        case 'a': strcpy(num, Day_name[ltp.tm_wday]); break;
                        case 'd': sprintf(num, "%02d", ltp.tm_mday); break;
                        case 'h': sprintf(num, "%02d", hour); break;			 
                        case 'j': sprintf(num, "%02d", 1+ltp.tm_yday); break;
                        case 'b': strcpy(num, Month_name[ltp.tm_mon]); break;
                        case 'm': sprintf(num, "%02d", 1+ltp.tm_mon); break;
                        case 't': {
                           int w = ltp.tm_year+1900;
                           if (w % 4==0 && (w % 100!=0 || w % 400 == 0))
                             w = 366;
                           else
                             w = 365;
                           sprintf(num, "%d", w);
                           break;
                           }
                        case 'y': sprintf(num, "%02d", ltp.tm_year%100); break;
                        case 'Y': sprintf(num, "%d", ltp.tm_year+1900); break;
                        case 'U': {
                           struct tm ftm;
                           time_t ftime;
                           int w;
                           /*
                            * define weeknumber
                            * week #1 = week with the first thursday
                            */
                           /* set reference date to 1st of january, 12:00:00 */
                           (void) memset(&ftm, 0, sizeof(struct tm));
                           ftm.tm_isdst = -1;
                           ftm.tm_mon = 0;
                           ftm.tm_mday = 1;
                           ftm.tm_year = ltp.tm_year;
                           ftm.tm_hour = 12;
                           ftm.tm_min = 0;
                           ftm.tm_sec = 0;
                           ftime = mktime(&ftm);
                           ftm = *localtime(&ftime);
                           /* get first sunday (start of week) */
                           if (ftm.tm_wday < 5)
                              w = 1 - ftm.tm_wday;
                           else
                              w = 8 - ftm.tm_wday;
                           /* get weeknumber */
                           sprintf(num, "%02d", 
                                ((ltp.tm_yday+1-ltp.tm_wday-w)/7)+1);
                           break; 
                           }
                        case '_': c = ' ';
                        default: num[0] = c; num[1] = '\0'; break;
                      }
                   }
                   strcat(s, num);
                }
                l = strlen(s);
                if (l<72) {
                  for (i=l; i<72; i++) s[i] = ' ';
                  s[72] = '\0';
                  l = 72;
                }
                drawTextStrip(Context, s, l);
		return;
        }

        switch(Context->flags.map_mode) {

        case LEGALTIME:
           gtp = *gmtime(&gtime);
           setTZ(Context->mark1.city);
           ltp = *localtime(&gtime);
           sprintf(s,
                " %s %02d:%02d:%02d %s %s %02d %s %04d    %s %02d:%02d:%02d UTC %s %02d %s %04d",
                Label[L_LEGALTIME], ltp.tm_hour, ltp.tm_min,
                ltp.tm_sec,
#ifdef NEW_CTIME
                ltp.tm_zone,
#else
                tzname[ltp.tm_isdst],
#endif
                Day_name[ltp.tm_wday], ltp.tm_mday,
                Month_name[ltp.tm_mon], 1900 + ltp.tm_year ,
                Label[L_GMTTIME],
                gtp.tm_hour, gtp.tm_min,
                gtp.tm_sec, Day_name[gtp.tm_wday], gtp.tm_mday,
                Month_name[gtp.tm_mon], 1900 + gtp.tm_year);
           break;

        case COORDINATES:
           setTZ(Context->mark1.city);
           ltp = *localtime(&gtime);
           if (ltp.tm_mday != Context->local_day) 
              setDayParams(Context);
           Context->local_day = ltp.tm_mday;
           if ((Context->mark1.city) && Context->mark1.full)
           sprintf(s,
                " %s (%s,%s)  %02d:%02d:%02d %s %s %02d %s %04d   %s %02d:%02d:%02d   %s %02d:%02d:%02d",
                Context->mark1.city->name, 
                num2str(Context->mark1.city->lat, slat, Context->flags.dms), 
                num2str(Context->mark1.city->lon, slon, Context->flags.dms),
                ltp.tm_hour, ltp.tm_min, ltp.tm_sec,
#ifdef NEW_CTIME
                ltp.tm_zone,
#else
                tzname[ltp.tm_isdst],
#endif
                Day_name[ltp.tm_wday], ltp.tm_mday,
                Month_name[ltp.tm_mon], 1900 + ltp.tm_year ,
                Label[L_SUNRISE],
                Context->mark1.sr.tm_hour, Context->mark1.sr.tm_min, Context->mark1.sr.tm_sec,
                Label[L_SUNSET], 
                Context->mark1.ss.tm_hour, Context->mark1.ss.tm_min, Context->mark1.ss.tm_sec);
                else
           if ((Context->mark1.city) && !Context->mark1.full)
           sprintf(s,
                " %s (%s,%s)  %02d:%02d:%02d %s %s %02d %s %04d   %s %02d:%02d:%02d",
                Context->mark1.city->name, 
                num2str(Context->mark1.city->lat, slat, Context->flags.dms), 
                num2str(Context->mark1.city->lon, slon, Context->flags.dms),
                ltp.tm_hour, ltp.tm_min, ltp.tm_sec,
#ifdef NEW_CTIME
                ltp.tm_zone,
#else
                tzname[ltp.tm_isdst],
#endif
                Day_name[ltp.tm_wday], ltp.tm_mday,
                Month_name[ltp.tm_mon], 1900 + ltp.tm_year ,
                Label[L_DAYLENGTH],
                Context->mark1.dl.tm_hour, Context->mark1.dl.tm_min, Context->mark1.dl.tm_sec);
                else
                  sprintf(s," %s", Label[L_CLICKCITY]);
                break;

        case SOLARTIME:
           if (Context->mark1.city) {
             double junk;
             stime = sunParams(gtime, &junk, &junk, Context->mark1.city);
             stp = *gmtime(&stime);
             if (stp.tm_mday != Context->solar_day) 
                setDayParams(Context);
             Context->solar_day = stp.tm_mday;
             sprintf(s, " %s (%s,%s)  %s %02d:%02d:%02d   %s %02d %s %04d   %s %02d:%02d:%02d", 
                  Context->mark1.city->name, 
                  num2str(Context->mark1.city->lat,slat, Context->flags.dms), 
                  num2str(Context->mark1.city->lon,slon, Context->flags.dms),
                  Label[L_SOLARTIME],
                  stp.tm_hour, stp.tm_min, stp.tm_sec,
                  Day_name[stp.tm_wday], stp.tm_mday,
                  Month_name[stp.tm_mon], 1900 + stp.tm_year,
                  Label[L_DAYLENGTH], 
                  Context->mark1.dl.tm_hour, Context->mark1.dl.tm_min, Context->mark1.dl.tm_sec);
           } else
                  sprintf(s," %s", Label[L_CLICKLOC]);
           break;

        case DISTANCES:
           if(Context->mark1.city && Context->mark2.city) {
             dist = sin(torad(Context->mark1.city->lat)) * sin(torad(Context->mark2.city->lat))
                    + cos(torad(Context->mark1.city->lat)) * cos(torad(Context->mark2.city->lat))
                           * cos(torad(Context->mark1.city->lon-Context->mark2.city->lon));
             if (dist >= 1.0) 
                dist = 0.0;
             else
             if (dist <= -1.0) 
                dist = M_PI;
             else
                dist = acos(dist);
             sprintf(s, " %s (%s,%s) --> %s (%s,%s)     "
                      "%d km  =  %d miles", 
               Context->mark2.city->name, 
               num2str(Context->mark2.city->lat,slatp, Context->flags.dms), 
               num2str(Context->mark2.city->lon, slonp, Context->flags.dms),
               Context->mark1.city->name, 
               num2str(Context->mark1.city->lat, slat, Context->flags.dms), 
               num2str(Context->mark1.city->lon, slon, Context->flags.dms),
               (int)(EARTHRADIUS_KM*dist), (int)(EARTHRADIUS_ML*dist));
           } else
             sprintf(s, " %s", Label[L_CLICK2LOC]);
           break;

        case EXTENSION:
           showHours(Context);
	   return;

        default:
           break;
        }

        l = strlen(s);
        if (l<125) {
          for (i=l; i<125; i++) s[i] = ' ';
          s[125] = '\0';
          l = 125;
        }

      drawTextStrip(Context, s, l);
}

void 
initShading(Context) 
struct Sundata * Context;
{
      int i;

      if (Context->flags.shading <2) 
         Context->shadefactor = 1.0;
      else {
         if (Context->flags.shading == 2)
            Context->shadefactor = 180.0/(M_PI*(SUN_APPRADIUS+atm_refraction));
         else
            Context->shadefactor = 180.0/(M_PI*(SUN_APPRADIUS+atm_diffusion));
      }

      Context->shadescale = 0.5 * ((double)Context->flags.colorscale + 0.5);

      if (Context->flags.shading == 0 || Context->flags.shading >= 4) {
         if (Context->tr1) {
            free(Context->tr1);
            Context->tr1 = NULL;
         }
      }

      if (Context->flags.shading <= 1 && Context->flags.shading >= 4) {
         if (Context->tr2) {
            free(Context->tr2);
            Context->tr2 = NULL;
         }
      }

      if (Context->flags.shading >= 1 && Context->flags.shading <= 3) {
         if (!Context->tr1)  
            Context->tr1 = (short *) 
                     salloc(Context->geom.width*sizeof(short));
         for (i=0; i< (int)Context->geom.width; i++) Context->tr1[i] = 0;
      }
      
      if (Context->flags.shading >= 2 && Context->flags.shading <= 3) {
         if (!Context->tr2)  
            Context->tr2 = (short *) 
                     salloc(Context->geom.width*sizeof(short));
         for (i=0; i< (int)Context->geom.width; i++) 
            Context->tr2[i] = -1; 
      }

      Context->south = -1;
}       

void
makeContext(Context, build)
struct Sundata * Context;
int build;
{
        if (build) {
           Context->win = 0;
	   Context->xim = NULL;
           Context->ximdata = NULL;
	   Context->label = NULL;
	   Context->vmfpixels = NULL;
           Context->mappix = 0;
           if (Context->wintype)
              Context->geom = MapGeom;
           else
              Context->geom = ClockGeom;
           Context->spotsizes = (int *) salloc(city_cat * sizeof(int));
           Context->sizelimits = (int *) salloc(city_cat * sizeof(int));
	   memcpy(Context->spotsizes, city_spotsizes, city_cat*sizeof(int));
 	   memcpy(Context->sizelimits, city_sizelimits, city_cat*sizeof(int));
           Context->zoom = gzoom;
           Context->oldzoom = gzoom;
           Context->flags = gflags;
           Context->jump = time_jump;
           Context->clock_img_file = strdup(Clock_img_file);
           Context->map_img_file = strdup(Map_img_file);
           Context->mark1.city = NULL;
           Context->mark1.flags = 0;
           Context->pos1.tz = NULL;
           Context->mark2.city = NULL;
           Context->mark2.flags = 0;
	   Context->lastmarked = NULL;
           Context->pos2.tz = NULL;
           Context->tr1 = Context->tr2 = NULL;
           if (position.lat<=90.0) {
              Context->pos1 = position;
              Context->mark1.city = &Context->pos1;
           }
        }

        Context->newzoom = Context->zoom;
        setZoomDimension(Context);
        Context->zoom = Context->newzoom;

        Context->local_day = -1;
        Context->solar_day = -1;
	Context->sundec = 100.0;
	Context->sunlon = 0.0;
	Context->moondec = 100.0;
	Context->moonlon = 0.0;

        if (runlevel!= IMAGERECYCLE) {
           if (color_depth<=8) {
              Context->daypixel = (unsigned char *) salloc(256*sizeof(char));
              Context->nightpixel = (unsigned char *) salloc(256*sizeof(char));
           } else {
              Context->daypixel = NULL;
              Context->nightpixel = NULL;
           }
	}

        Context->bits = 0;
        Context->flags.update = 4;
        Context->time = 0L;
        Context->projtime = -1L;
        Context->roottime = -1L;
        Context->animtime = -1L;
        Context->daywave = (double *) salloc( 
              (2*Context->geom.height+Context->geom.width)*sizeof(double));
        Context->cosval = Context->daywave + Context->geom.width;
        Context->sinval = Context->cosval + Context->geom.height;

        initShading(Context);
}

void
DarkenPixel(Context, x, y, t)
struct Sundata * Context;
int x;
int y;
int t;
{
        register int i;
        unsigned int factor;
        unsigned char u, v, w, r, g, b;

        if (Context->flags.colorlevel<FULLCOLORS) {
           XDrawPoint(dpy, Context->mappix,Context->gdata->pixgc, x,y);
           return;
        }
        i = bytes_per_pixel * x + Context->xim->bytes_per_line * y;

        if (color_depth>16) {
           if (bigendian)
              i += bytes_per_pixel - 3;
           u = Context->ximdata[i];
           v = Context->ximdata[i+1];
           w = Context->ximdata[i+2];
           if (t>=0) {
              factor = Context->flags.darkness + (t * (MAXSHORT-
                  Context->flags.darkness))/Context->flags.colorscale;
              u = (u * factor)/MAXSHORT;
              v = (v * factor)/MAXSHORT;
              w = (w * factor)/MAXSHORT;
           }
           Context->xim->data[i] = u;
           Context->xim->data[i+1] = v;
           Context->xim->data[i+2] = w;
        } else 
        if (color_depth==16) {
	   if (bigendian) {
              u = Context->ximdata[i+1];
              v = Context->ximdata[i];
           } else {
              u = Context->ximdata[i];
              v = Context->ximdata[i+1];
           }
           if (t>=0) {
              factor = Context->flags.darkness + (t * (255 -
                 Context->flags.darkness))/Context->flags.colorscale;
              r = v>>3;
              g = ((v&7)<<3) | (u>>5);
              b = u&31;
              r = (r * factor)/31;
              g = (g * factor)/63;
              b = (b * factor)/31;
              u = (b&248)>>3 | (g&28)<<3;
              v = (g&224)>>5 | (r&248);
           }
           if (bigendian) {
              Context->xim->data[i+1] = u;
              Context->xim->data[i] = v;
           } else {
              Context->xim->data[i] = u;
              Context->xim->data[i+1] = v;
	   }
        } else
        if (color_depth==15) {
	   if (bigendian) {
              u = Context->ximdata[i+1];
              v = Context->ximdata[i];
	   } else {
              u = Context->ximdata[i];
              v = Context->ximdata[i+1];
	   }
           if (t>=0) {
              factor = Context->flags.darkness + (t * (255 - 
                 Context->flags.darkness))/Context->flags.colorscale;
              r = v>>2;
              g = (v&3)<<3 | (u>>5);
              b = u&31;
              r = (r * factor)/31;
              g = (g * factor)/31;
              b = (b * factor)/31;
              u = (b&248)>>3 | (g&56)<<2;
              v = (g&192)>>6 | (r&248)>>1;
           }
           if (bigendian) {
              Context->xim->data[i+1] = u;
              Context->xim->data[i] = v;
	   } else {
              Context->xim->data[i] = u;
              Context->xim->data[i+1] = v;
	   }
        } else {
           if (t>=0) {
             if ((277*x+359*y) % Context->flags.colorscale < 
                 Context->flags.colorscale-t)
               Context->xim->data[i] = 
                 Context->nightpixel[(unsigned char)Context->ximdata[i]];
             else
               Context->xim->data[i] = Context->ximdata[i];
           } else
             Context->xim->data[i] = Context->ximdata[i];
        }
}

int
howDark(Context, i, j)
struct Sundata * Context;
int i, j;
{
      double light;
      int k;

      if (Context->flags.shading == 0) {
	 return -1;
      }

      light = Context->daywave[i] * Context->cosval[j] + Context->sinval[j];

      if (Context->flags.shading == 1) {
         if (light >= 0) k = -1; else k = 0;
      } else {
         if (Context->flags.shading<=3 || 
             (Context->flags.shading==4 && light<0))
             light *= Context->shadefactor;
         k = (int) ((1.0+light)*Context->shadescale);
         if (k < 0) k = 0;
         if (k >= Context->flags.colorscale) k = - 1;
      }
      return k;
}

void
SetPixelLight(Context, i, j, pixel)
struct Sundata * Context;
int i, j;
Pixel pixel;
{
      if (i<0 || i>= Context->geom.width) return;
      if (j<0 || j>= Context->geom.height) return;
      if (erase_obj)
         DarkenPixel(Context, i, j, howDark(Context, i, j));
      else
	 XPutPixel(Context->xim, i, j, pixel);
}

void 
XPutStringImage(Context, x, y, s, l, mode)
Sundata *Context;
int x, y;
char *s;
int l, mode;
{
    XImage *xim;
    XFontStruct *font = NULL;
    Pixel pixel = 0;

    int i, j, w, h, dy;
    char u = 0, test;
    
    if (!s || !strlen(s)) return;

    if (mode <= 1) {
       font = Context->gdata->font[COORDFONT];
       pixel = Context->gdata->pixel[PARALLELCOLOR-mode];
    } else
    if (mode == 2) {
       font = Context->gdata->font[CITYFONT];
       pixel = Context->gdata->pixel[CITYNAMECOLOR];
    } else
    if (mode >= 3) {
       font = Context->gdata->font[LABELFONT];
       if (Context->flags.colorlevel <= MANYCOLORS)
          pixel = Context->gdata->pixel[CITYNAMECOLOR];	   
       else
          pixel = Context->vmfpixels[mode-3];
    }

    if (!font) return;
    h = font->max_bounds.ascent + font->max_bounds.descent;
    dy = font->max_bounds.ascent;
    
    w = XTextWidth(font, s, l);
    if (w>textwidth || h>textheight) {
       textwidth = w;
       textheight = h;
       if (textpix) {
	  XFreePixmap(dpy, textpix);
          textpix = 0;
       }
    }

    if (!textpix) {
       textpix = XCreatePixmap(dpy, Context->win, textwidth, textheight, 1);
    }

    XSetForeground(dpy, Context->gdata->pixgc, black);
    XFillRectangle(dpy, textpix, Context->gdata->pixgc, 0, 0, w, h);
    XSetForeground(dpy, Context->gdata->pixgc, white);
    XSetFont(dpy, Context->gdata->pixgc, font->fid);
    if (Context->flags.colorlevel <= MANYCOLORS) {
       XDrawString(dpy, Context->mappix, Context->gdata->pixgc, 
		   x+1, y-dy, s, l);
       return;
    }
    XDrawString(dpy, textpix, Context->gdata->pixgc, 0, dy, s, l);
    xim = XGetImage(dpy, textpix, 0, 0, w, h, 1, XYPixmap);
    if (!xim) return;
    test = (bigendian)? 128 : 1;
    for (j=0; j<h; ++j) {
       if (y-dy+j >= (int)Context->geom.height) break;
       for (i=0; i<w; ++i) {
	  if ((i&7) == 0) u = xim->data[j*xim->bytes_per_line+i/8];
          if (u&test)
	     SetPixelLight(Context, x+i+1, y-dy+j, pixel);
	  u = (bigendian)? u<<1 : u>>1;
       }
    }
    XDestroyImage(xim);
}

int
int_latitude(Context, lat)
Sundata * Context;
double lat;
{
    return
       (int) (Context->zoom.height - (lat+90.0) * (Context->zoom.height/180.0))
             - Context->zoom.dy;
}

int
int_longitude(Context, lon)
Sundata * Context;
double lon;
{
    return
       (int) ((180.0+lon) * (Context->zoom.width/360.0)) - Context->zoom.dx;
}


/*
 * drawObject() - Draw an object (city, mark, sun, moon) on the map.
 */

void
drawObject(Context, lon, lat, mode, color, name)
struct Sundata * Context;
double lon, lat;                /* Latitude and longtitude of the city */
int    mode;
int    color;
char *name;
{
    /*
     * Local Variables
     */
    Pixel pixel;

    int ilon, ilat;             /* Screen coordinates of the city */
    int i, j, dx, dy, u, which, bool=0;
    unsigned short * bits;
    char slat[20], slon[20];

    if (mode == 0) return;
    if ((!Context->wintype && mode > 0) || mode < -SPECIALBITMAPS) return;
    if (mode > city_cat) mode = city_cat;
    if (mode > 0) {
       which = SPECIALBITMAPS + Context->spotsizes[mode-1] - 1;
       if (which < SPECIALBITMAPS) return;
       if (Context->flags.colorlevel>0 && !Context->flags.citymode) return;
       if (Context->zoom.width < Context->sizelimits[mode-1]
           && color==0) return;
       if (color<0) color = 0;
    } else
       which = -mode - 1;

    ilon = int_longitude(Context, lon);
    if (ilon<0 || ilon>Context->geom.width) return;

    ilat = int_latitude(Context, lat);
    if (ilat<0 || ilat>Context->geom.height) return;

    bits = symbol_bits[which];
 
    dx = bits[0]/2;
    dy = bits[1]/2;

    pixel = Context->gdata->pixel[CITYCOLOR0+color];

    if (Context->flags.colorlevel == FULLCOLORS) {
       for (j=0; j<bits[1]; j++) {
	  if (ilat-dy+j >= (int) Context->geom.height) break;
          u = bits[j+2];
          for (i=0; i<bits[0]; i++) {
             if (u&1) SetPixelLight(Context, ilon-dx+i, ilat-dy+j, pixel);
             u = u>>1;
          }
       }
    } else 
    if (Context->flags.colorlevel >= FEWCOLORS) {
       XSetForeground(dpy, Context->gdata->wingc, pixel);
       for (j=0; j<bits[1]; j++) {
	  if (ilat-dy+j >= (int) Context->geom.height) break;
          u = bits[j+2];
          for (i=0; i<bits[0]; i++) {
	     if (u&1) XDrawPoint(dpy, Context->win, Context->gdata->wingc, 
                ilon-dx+i, ilat-dy+j);
	     u = u>>1;
	  }
       }
    } else {
       XSetForeground(dpy, Context->gdata->pixgc, pixel);
       for (j=0; j<bits[1]; j++) {
	  if (ilat-dy+j >= (int) Context->geom.height) break;
          u = bits[j+2];
          for (i=0; i<bits[0]; i++) {
	     if (u&1) XDrawPoint(dpy, Context->mappix, Context->gdata->pixgc, 
                ilon-dx+i, ilat-dy+j);
	     u = u>>1;
	  }
       }
    }

    if (mode<0) ++dx;

    if (Context->flags.citymode==2 && name) {
       if (Context->flags.colorlevel == FULLCOLORS)
          XPutStringImage(Context, ilon+dx, ilat-1, name, strlen(name), 2);
       else
	 if (Context->flags.colorlevel >= FEWCOLORS) {
          XSetForeground(dpy, Context->gdata->wingc, 
			     Context->gdata->pixel[CITYNAMECOLOR]);
          XDrawString(dpy, Context->win, Context->gdata->wingc, 
                      ilon+dx, ilat-1, name, strlen(name));
       } else
          XDrawString(dpy, Context->mappix, Context->gdata->pixgc, 
                      ilon+dx, ilat-1, name, strlen(name));
    }

    if (!Context->wintype) return;
    if ((Context->flags.citymode==3 && mode>0) ||
        (bool=(Context->flags.objectmode==2 && mode<-1))) {
       dy = Context->gdata->mapstrip/2;
       (void) num2str(lat, slat, Context->flags.dms);
       (void) num2str(lon, slon, Context->flags.dms);
       if (Context->flags.colorlevel == FULLCOLORS) {
	  XSetFont(dpy, Context->gdata->pixgc,
		        Context->gdata->font[CITYFONT]->fid);
          XPutStringImage(Context, ilon+dx, ilat-1, slat, strlen(slat), 2);
          XPutStringImage(Context, ilon+dx, ilat-1+dy, slon, strlen(slon), 2);
       } else
       if (Context->flags.colorlevel >= FEWCOLORS ) {
          XSetBackground(dpy, Context->gdata->wingc, Context->gdata->pixel[MAPBGCOLOR]);
          XSetForeground(dpy, Context->gdata->wingc, Context->gdata->pixel[CITYNAMECOLOR]);
          XSetFont(dpy, Context->gdata->wingc, 
                        Context->gdata->font[CITYFONT]->fid);
          XDrawString(dpy, Context->win, Context->gdata->wingc, 
                      ilon+dx, ilat-1, slat, strlen(slat));
          XDrawString(dpy, Context->win, Context->gdata->wingc, 
                      ilon+dx, ilat-1+dy, slon, strlen(slon));
       } else {
	  XSetFont(dpy, Context->gdata->pixgc,
		        Context->gdata->font[CITYFONT]->fid);
          XDrawString(dpy, Context->mappix, Context->gdata->pixgc, 
                      ilon+dx, ilat-1, slat, strlen(slat));
          XDrawString(dpy, Context->mappix, Context->gdata->pixgc, 
                      ilon+dx, ilat-1+dy, slon, strlen(slon));
       }
    }
}

void
drawCities(Context)
struct Sundata * Context;
{
City *c;
        if (!Context->wintype || !Context->flags.citymode) return; 

	if (Context->flags.colorlevel==MONOCHROME ||
            Context->flags.colorlevel==FULLCOLORS)
           XSetFont(dpy, Context->gdata->pixgc, 
                         Context->gdata->font[CITYFONT]->fid);
        else
           XSetFont(dpy, Context->gdata->wingc, 
                         Context->gdata->font[CITYFONT]->fid);

        if (Context->lastmarked && Context->mark1.city != Context->lastmarked
            && Context->flags.colorlevel>MONOCHROME) {
	   erase_obj = 1;
	   c = Context->lastmarked;
           drawObject(Context, c->lon, c->lat, c->size, 1, c->name);
	   erase_obj = 0;
	}

        for (c = cityroot; c; c = c->next) {
	   if (c!=Context->mark1.city && c!=Context->mark2.city)
              drawObject(Context, c->lon, c->lat, c->size, 
                         -(c==cityinit), c->name);
	}

       	c = Context->mark2.city;
	if (c && c!=&Context->pos2)
           drawObject(Context, c->lon, c->lat, c->size, 2, c->name);

       	c = Context->mark1.city;
	if (c && c!=&Context->pos1) {
           drawObject(Context, c->lon, c->lat, c->size, 1, c->name);
	   Context->lastmarked = c;
	}
}

void
drawMarks(Context)
struct Sundata * Context;
{
        if (Context->flags.colorlevel==MONOCHROME || !Context->wintype) return;

        /* code for color mode */
	if (erase_obj==0 || (erase_obj&1))
        if (Context->mark1.city == &Context->pos1)
          drawObject(Context, Context->mark1.city->lon, 
                              Context->mark1.city->lat,
                              -1, 3, NULL);

	if (erase_obj==0 || (erase_obj&2))
        if (Context->mark2.city == &Context->pos2)
          drawObject(Context, Context->mark2.city->lon, 
                              Context->mark2.city->lat,
                              -1, 4, NULL);
}

void
drawLabels(Context)
struct Sundata * Context;
{
   int ilon, ilat, width, dw = 0;
   struct TextLabel * label;
   char *text, *text0, *ptr;

   if(!Context->wintype) return;
   label = Context->label;
   while (label) if (label->text && *label->text) {
      ilon = int_longitude(Context, label->lon);
      ilat = int_latitude(Context, label->lat);
      text = text0 = strdup(label->text);
      while(text) {
	 ptr = index(text,'\n');
	 if (ptr) *ptr = '\0';
         width = XTextWidth(Context->gdata->font[LABELFONT], 
			    text, strlen(text));
         if (label->position==0) dw = width/2;
         else
	 if (label->position==-1) dw = width;
         XPutStringImage(Context, ilon-dw, ilat, text,
   	                 strlen(text), label->color+3);
	 ilat += Context->gdata->font[LABELFONT]->ascent+
	         Context->gdata->font[LABELFONT]->descent+2;
	 if (ptr) text = ptr+1; else text = NULL;
      }
      free(text0);
      label = label->next;
   }
}

double
getSpacing(Context, mode)
Sundata * Context;
int mode;   /* 0=parallel 1=meridian spacing */
{
  double val[12] = 
    { 0.1, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 15.0, 20.0, 30.0, 45.0, 90.0 };
  double quot = 0.0;
  int i = 0;

  i = 11;

  if (mode == 0)
     quot = 4200.0/(double)Context->zoom.height;

  if (mode == 1)
     quot = 10800.0/(double)Context->zoom.width;
  
  if (quot<0.5 && mode) quot = quot*1.2;

  while (i>0 && val[i-1] > quot) --i;
     
  if (mode==1 && i==8) ++i;
  return val[i];  
}


/*
 * drawParallel() - Draw a parallel line
 */

void
drawParallel(Context, pixel, lat, step, thickness, text, numdigits)
struct Sundata * Context;
Pixel pixel;
double lat;
int step;
int thickness;
int text;
int numdigits;
{
        int ilat, i0, i1, i, j, jp, k, min, max, doit;
	char s[10], format[10];

        if (!Context->wintype) return; 

        ilat = int_latitude(Context, lat);

        i = Context->flags.meridian & 3 ;
	min = 0;
	max = Context->geom.height;
	if (i==2) max = max-coordvalheight;
	if (i==3) min = coordvalheight;

        if (ilat<min || ilat>=max) return;

        doit = 1;
	if (text<0) {
	   text = -text;
           if (lat != 0.0)
	      doit = 0;
	}
	
	if (text>=2) {
           sprintf(format, "%%.%df°", numdigits);
	   sprintf(s, format, lat);
           min = XTextWidth(Context->gdata->font[COORDFONT], s, strlen(s))+4;
           max = (int) Context->geom.width;
	   if (text==2)
 	     i0 = 2;
	   else {
             max = max-min-2;
	     i0 = max+4;
	     min = 0;
	   }
           i1 = (coordvalheight-6)/3;
	   if (doit) {
	      if (Context->flags.colorlevel == FULLCOLORS)
                 XPutStringImage(Context, i0, ilat+1, s, strlen(s), 0);
	      else
	      if (Context->flags.colorlevel >= MANYCOLORS)
                 XDrawString(dpy, Context->win, 
                     Context->gdata->wingc, i0, ilat+i1, s, strlen(s));
	      else
                 XDrawString(dpy, Context->mappix, 
                     Context->gdata->pixgc, i0, ilat+i1, s, strlen(s));
	   }
	} else {
	   min = 0;
	   max = (int) Context->geom.width - 1;
	}

        i0 = Context->geom.width/2;
        i1 = 1+i0/step;
        for (i=-i1; i<i1; i+=1) {
          j = i0+step*i-thickness;
          jp = i0+step*i+thickness;
          if (j<0) j = 0;
          if (jp>max) jp = max;
	  if (jp<min) continue;
          if (j>max) continue;
	  if (Context->flags.colorlevel == FULLCOLORS) {
             for (k=j; k<=jp; k++)
                 SetPixelLight(Context, k, ilat, pixel);
	  } else
	  if (Context->flags.colorlevel >= MANYCOLORS)
             XDrawLine(dpy, Context->win, Context->gdata->wingc, 
                       j, ilat, jp,ilat);
	  else
             XDrawLine(dpy, Context->mappix, Context->gdata->pixgc, 
                       j, ilat, jp,ilat);
	}
}

void
drawParallels(Context)
struct Sundata * Context;
{
        Pixel pixel;
        static  double val[5] = { -66.55, -23.45, 0.0, 23.45, 66.55 };
	double  f1, f2, spacing;
        int     i, b1, b2, parmode, numdigits;

        if (!Context->wintype || !Context->flags.parallel) return; 

	parmode = Context->flags.parallel & 3;

	if (Context->zoom.paralspacing)
	   spacing = Context->zoom.paralspacing;
	else
	   spacing = getSpacing(Context, 0);
        
        /* b = (int) (89.9/spacing); */
	f1 = (double)(Context->zoom.dy+Context->geom.height)/
                ((double)Context->zoom.height);
	f2 = (double)Context->zoom.dy/((double)Context->zoom.height);

	b1 = rint(0.7 + (90.0 - f1*180.0)/spacing);
	b2 = rint(-0.7 + (90.0 - f2*180.0)/spacing);

        pixel = Context->gdata->pixel[PARALLELCOLOR];

        if (Context->flags.colorlevel==FULLCOLORS || 
            Context->flags.colorlevel==MONOCHROME) {
           XSetFont(dpy, Context->gdata->pixgc, 
                         Context->gdata->font[COORDFONT]->fid);
        } else {
           XSetFont(dpy, Context->gdata->wingc, 
                         Context->gdata->font[COORDFONT]->fid);
           XSetForeground(dpy, Context->gdata->wingc, pixel);
	}
     
	numdigits = (spacing<1.0);
	if (parmode)
        for (i=b1; i<=b2; i++) if (i!=0 || Context->flags.parallel <=3)
           drawParallel(Context, pixel, i*spacing, 3, Context->flags.dotted, 
                       parmode, numdigits);

        pixel = Context->gdata->pixel[TROPICCOLOR];
        if (Context->flags.colorlevel==MANYCOLORS) 
           XSetForeground(dpy, Context->gdata->wingc, pixel);

	if (Context->flags.parallel & 8) {
           for (i=0; i<5; i++)
              drawParallel(Context, pixel, val[i], 3, 1, -parmode, numdigits);
	}
}

/*
 * drawMeridian() - Draw a meridian line
 */

void
drawMeridian(Context, lon, step, thickness, numdigits)
struct Sundata * Context;
double lon;
int step;
int thickness;
int numdigits;
{
        int ilon, i0, i1, i, j, jp, k, min, max;
	char s[10], format[10];

        ilon = int_longitude(Context, lon);

        i = Context->flags.parallel & 3 ;
	min = 0;
	max = Context->geom.width;
	if (i==2) min = coordvalwidth;
	if (i==3) max = max-coordvalwidth;

        if (ilon<min || ilon>=max) return;

        i0 = Context->geom.height/2;
        i1 = 1+i0/step;
	if (Context->flags.meridian>=2) {
           sprintf(format, "%%.%df°", numdigits);
	   sprintf(s, format, lon);
	   i = 2*XTextWidth(Context->gdata->font[COORDFONT], s, strlen(s))/5;
	   min = Context->gdata->font[COORDFONT]->max_bounds.ascent +
               Context->gdata->font[COORDFONT]->max_bounds.descent + 3;
	   max = (int) Context->geom.height;
	   if (Context->flags.meridian==2) {
	      j = Context->geom.height-3;
	      max = max-min-1;
	      min = 0;
	   } else
	      j = min-4;
	   if (Context->flags.colorlevel==FULLCOLORS)
              XPutStringImage(Context, ilon-i, j, s, strlen(s), 1);
	   else
	   if (Context->flags.colorlevel>=MANYCOLORS)
              XDrawString(dpy, Context->win, Context->gdata->wingc, 
                       ilon-i, j, s, strlen(s));
	   else
              XDrawString(dpy, Context->mappix, Context->gdata->pixgc, 
                       ilon-i, j, s, strlen(s));
	} else {
	   min = 0;
	   max = (int) Context->geom.height - 1 ;
	}

        for (i=-i1; i<i1; i+=1) {
           j = i0+step*i-thickness;
           jp = i0+step*i+thickness;
           if (j<0) j = 0;
           if (jp>max) jp = max;
	   if (jp<min) continue;
	   if (j>max) continue;
	   if (Context->flags.colorlevel==FULLCOLORS) {
	      for (k=j; k<=jp; k++)
	         SetPixelLight(Context, ilon, k,
                               Context->gdata->pixel[MERIDIANCOLOR]);
	   } else
	   if (Context->flags.colorlevel >= MANYCOLORS)
              XDrawLine(dpy, Context->win, Context->gdata->wingc, 
                        ilon, j, ilon, jp);
	   else
	      XDrawLine(dpy, Context->mappix, Context->gdata->pixgc, 
                        ilon, j, ilon, jp);
	}
}

void
drawMeridians(Context)
struct Sundata * Context;
{
        int     i, b1, b2, numdigits;
	double  spacing, f1, f2;

        if (!Context->wintype || !Context->flags.meridian) return; 

	if (Context->zoom.meridspacing)
	   spacing = Context->zoom.meridspacing;
	else
	   spacing = getSpacing(Context, 1);
        
        /* b = (int) (179.9/spacing); */
	f1 = (double)Context->zoom.dx/((double)Context->zoom.width);
	f2 = (double)(Context->zoom.dx+Context->geom.width)/
                ((double)Context->zoom.width);

	b1 = rint(0.7 + (f1*360.0 - 180.0)/spacing);
	b2 = rint(-0.7 + (f2*360.0 - 180.0)/spacing);

        if (Context->flags.colorlevel==FULLCOLORS || 
            Context->flags.colorlevel==MONOCHROME) {
           XSetFont(dpy, Context->gdata->pixgc, 
                         Context->gdata->font[COORDFONT]->fid);
        } else {
           XSetFont(dpy, Context->gdata->wingc, 
                         Context->gdata->font[COORDFONT]->fid);
           XSetForeground(dpy, Context->gdata->wingc, 
                          Context->gdata->pixel[MERIDIANCOLOR]);
	}
 
	numdigits = (spacing<1.0);
        for (i=b1; i<=b2; i++)
          drawMeridian(Context, i*spacing, 3, Context->flags.dotted, numdigits);
}

void
drawLines(Context)
Sundata * Context;
{
        coordvalwidth = 
            XTextWidth(Context->gdata->font[COORDFONT], "-45°", 4) + 6;
        coordvalheight = Context->gdata->font[COORDFONT]->max_bounds.ascent + 
            Context->gdata->font[COORDFONT]->max_bounds.descent + 6;
        drawMeridians(Context);
        drawParallels(Context);
}

/*
 * drawSunAndMoon() - Draw Sun and Moon at position where they stand at zenith
 */

void
drawSunAndMoon(Context)
struct Sundata * Context;
{
    if (Context->flags.objectmode) {
       if (Context->flags.colorlevel>=FEWCOLORS &&
           Context->flags.colorlevel<=MANYCOLORS) {
	 if (Context->flags.objects & 4) return;
       }
       if (Context->flags.objects & 1)
          drawObject(Context, Context->sunlon, Context->sundec, -2, 5, NULL);
       if (Context->flags.objects & 2)
          drawObject(Context, Context->moonlon, Context->moondec, -3, 6, NULL);
       Context->flags.objects |= 4;
    }
}

void
drawBottomline(Context)
struct Sundata * Context;
{
    if (Context->flags.bottom & 2) return;
    if (Context->flags.bottom & 1) {
       XSetForeground(dpy, Context->gdata->wingc, 
          Context->gdata->pixel[CLOCKSTRIPFGCOLOR+Context->wintype]);
       XDrawLine(dpy, Context->win, Context->gdata->wingc, 
                   0, Context->geom.height, 
                   Context->geom.width-1, Context->geom.height);
    }
    Context->flags.bottom |= 2;
}

void
drawAll(Context)
struct Sundata * Context;
{
        if (Context->flags.colorlevel == FULLCOLORS) {
           if (!Context->xim) return;
        } else {
           if (!Context->mappix) return;
        }
        if (Context->flags.colorlevel != FEWCOLORS) drawLines(Context);
        drawBottomline(Context);
        drawCities(Context);
        drawMarks(Context);
        drawLabels(Context);
}

void
showMapImage(Context)
struct Sundata * Context;
{
        if (button_pressed) return;

        if (!Context->flags.mapped) {
	   Context->flags.update = 0;
	   return;
	}

        if (Context->flags.update>=2) {
           if (Context->flags.colorlevel == FULLCOLORS) {
	       drawAll(Context);
               XPutImage(dpy, Context->win, Context->gdata->wingc, 
                    Context->xim, 0, 0, 0, 0,
                    Context->geom.width, Context->geom.height);
           } else {
	       XSetBackground(dpy, Context->gdata->wingc, 
                     Context->gdata->pixel[CLOCKBGCOLOR+Context->wintype]);
	       XSetForeground(dpy, Context->gdata->wingc, 
                     Context->gdata->pixel[CLOCKFGCOLOR+Context->wintype]);
               XCopyPlane(dpy, Context->mappix, Context->win, 
		    Context->gdata->wingc,
                    0, 0, Context->geom.width, Context->geom.height, 0, 0, 1);
	       Context->flags.objects &= 3;
	   }
        }

        if (Context->flags.update) {
           Context->flags.update = 0;
           if (Context->flags.colorlevel >= FEWCOLORS &&
               Context->flags.colorlevel <= MANYCOLORS) drawAll(Context);
        }
}

void
pulseMarks(Context)
struct Sundata * Context;
{
int     done = 0;
        
        if (!Context->wintype) return; 
        if (Context->flags.colorlevel == FULLCOLORS) {
           if (!Context->xim) return;
        } else {
           if (!Context->mappix) return;
        }
        if (Context->mark1.city && Context->mark1.flags<0) {
           if (Context->mark1.pulse) {
             drawObject(Context, Context->mark1.save_lon, 
                   Context->mark1.save_lat, -1, 0, NULL);
             done = 1;
           }
           Context->mark1.save_lat = Context->mark1.city->lat;
           Context->mark1.save_lon = Context->mark1.city->lon;
           if (Context->mark1.city == &Context->pos1) {
              done = 1;
              drawObject(Context, Context->mark1.save_lon, 
                                Context->mark1.save_lat, -1, 0, NULL);
              Context->mark1.pulse = 1;
           } else
              Context->mark1.pulse = 0;
           Context->mark1.flags = 1;
        }
        else
        if (Context->mark1.flags>0) {
           if (Context->mark1.city|| Context->mark1.pulse) {
              drawObject(Context, Context->mark1.save_lon, 
                                Context->mark1.save_lat, -1, 0, NULL);
              Context->mark1.pulse = 1-Context->mark1.pulse;
              done = 1;
           }
           if (Context->mark1.city == NULL) Context->mark1.flags = 0;
        }

        if (Context->mark2.city && Context->mark2.flags<0) {
           if (Context->mark2.pulse) {
             drawObject(Context, Context->mark2.save_lon, 
                               Context->mark2.save_lat, -1, 0, NULL);
             done = 1;
           }
           Context->mark2.save_lat = Context->mark2.city->lat;
           Context->mark2.save_lon = Context->mark2.city->lon;
           if (Context->mark2.city == &Context->pos2) {
              drawObject(Context, Context->mark2.save_lon, 
                       Context->mark2.save_lat, -1, 0, NULL);
              done = 1;
              Context->mark2.pulse = 1;
           } else
              Context->mark2.pulse = 0;
           Context->mark2.flags = 1;
        }
        else
        if (Context->mark2.flags>0) {
           if (Context->mark2.city || Context->mark2.pulse) {
              drawObject(Context, Context->mark2.save_lon, 
                                Context->mark2.save_lat, -1, 0, NULL);
              Context->mark2.pulse = 1 - Context->mark2.pulse;
              done = 1;
           }
           if (Context->mark2.city == NULL) Context->mark2.flags = 0;
        }
        if (done) {
           Context->flags.update = 2;
           showMapImage(Context);
        }
}

/* To be used in case of mono mode only, to clear night area */

void
clearNightArea(Context)
struct Sundata * Context;
{
      int i, j;

      /* if (Context->flags.colorlevel < FULLCOLORS) return; */

      for (i = 0; i < (int)Context->geom.width; i++) {
        if (Context->south==0)
          for (j = Context->tr1[i]; j< (int)Context->geom.height; j++)
             DarkenPixel(Context, i, j, -1);
        else
          for (j = 0; j <Context->tr1[i]; j++)
             DarkenPixel(Context, i, j, -1);
      }
}

/*  moveNightArea  --  Update illuminated portion of the globe.  */

void
moveNightArea(Context)
struct Sundata * Context;
{
      int i, j, k, l, jmin, jmax, j0;
      int midcolor, south, north;
      short tr1, tr2;
      double shift, shiftp, quot, cd, sd;
      double f1, f2, f3; 

      Context->flags.hours_shown = 0;
      if (!Context->flags.shading) return;

      f1 = M_PI/((double) Context->zoom.height);
      f2 = ((double) Context->zoom.height)/M_PI;
      f3 = 1E-8 * f2;

      shift = f1 * (double)Context->zoom.dy;
      shiftp = 0.5*(Context->zoom.height+1) - (double) Context->zoom.dy;
      quot = torad(Context->sundec);
      cd = cos(quot);
      sd = sin(quot);
      if (quot>0) south = 0; else south = -1;
      north = -1-south;

      quot = 2.0*M_PI/(double)Context->zoom.width;
      for (i = 0; i < (int)Context->geom.width; i++)
         Context->daywave[i] = cos(quot *(((double)i)-Context->fnoon));

      for (j = 0; j < (int)Context->geom.height; j++) {
         quot = shift + f1 * (double)j;
         Context->cosval[j] = sin(quot)*cd;
         Context->sinval[j] = cos(quot)*sd;
      }

      /* Shading = 1 uses tr1 as j-integer value of transition day/night */
      /* Context->south means color value near South pole */
      /* which is updated as south, with north = -1-south = opposite color */

      if (Context->flags.shading == 1) {
         for (i = 0; i < (int)Context->geom.width; i++) {
            if (fabs(sd)>f3)
               tr1 = (int) (shiftp+f2*atan(Context->daywave[i]*cd/sd));
            else
               tr1 = 0;
            if (tr1<0) tr1 = 0;     
            if (tr1>(int)Context->geom.height) tr1 = (int)Context->geom.height;
            if (south==Context->south) {
               for (j = tr1; j<(int)Context->tr1[i]; j++)
                     DarkenPixel(Context, i, j, south);
               for (j = (int)Context->tr1[i]; j<tr1; j++)
                     DarkenPixel(Context, i, j, north);
            } else {
               if (tr1 <= (int)Context->tr1[i]) {
                  for (j = 0; j<tr1; j++)
                     DarkenPixel(Context, i, j, north);
                  for (j = (int)Context->tr1[i]; 
                           j<(int)Context->geom.height; j++)
                     DarkenPixel(Context, i, j, south);
               } else {
                  for (j = 0; j<(int)Context->tr1[i]; j++)
                     DarkenPixel(Context, i, j, north);
                  for (j = tr1; j<(int)Context->geom.height; j++)
                     DarkenPixel(Context, i, j, south);
               }
            }
            Context->tr1[i] = tr1;
         }
         Context->south = south;
         return;
      }

      /* Shading = 4,5 are quite straightforward... compute everything! */

      if (Context->flags.shading >= 4) {
         for (i = 0; i < (int)Context->geom.width; i++)
            for (j = 0; j< (int)Context->geom.height; j++) {
               DarkenPixel(Context, i, j, howDark(Context, i, j));
            }
         return;
      }

      /* Shading = 2,3 uses both tr1 and tr2 and is very tricky... */
      /* If both tr1,tr2 >=0 then normal transition 
            day -> shadow -> night  (or night -> shadow -> day)
         Otherwise we have an "exceptional transition"
            shadow near North pole -> (day or night) -> shadow near South pole
         Day or night is encoded midcolor, determined as follow:
              if tr1<0 then midcolor = Context->south
              if tr2<0 then midcolor = -1-Context->south (opposite color)
         Renormalize integres by  tr1=-2-tr1 if <0 tr2=-2-tr2 if <0.
         Then tr1>tr2 are the limits for the interval where color=midcolor */

      for (i = 0; i < (int)Context->geom.width; i++) {
         if (fabs(sd)>f3)
            j0 = (int) (shiftp+f2*atan(Context->daywave[i]*cd/sd));
         else
            j0 = 0;
         if (j0<0) j0 = 0;
         if (j0>(int)Context->geom.height) j0 = (int)Context->geom.height;
         
         tr1 = 0;
         tr2 = (short)(Context->geom.height-1);
         midcolor = -2;
         if (Context->tr1[i] < 0) {
            Context->tr1[i] = -Context->tr1[i]-2;
            midcolor = Context->south;
         }
         if (Context->tr2[i] < 0 && midcolor==-2) {
            Context->tr2[i] = -Context->tr2[i]-2;
            midcolor = -1-Context->south;
         }

         for (j=j0; j<(int)Context->geom.height; j++) {
            k = howDark(Context, i, j);
            if (k!=south) 
               DarkenPixel(Context, i, j, k);
            else {
               tr2 = (short)(j-1);
               jmax = (int)Context->geom.height-1;
               if (j<jmax && howDark(Context, i, jmax) != south) {
                  for (l=jmax; l>tr2 ; l--) {
                     k = howDark(Context, i, l);
                     if (k!=south) 
                        DarkenPixel(Context, i, l, k);
                     else {
                        jmax = l;
                        break;
                     }
                  }
                  tr1 = (short)(-jmax-1);
               }
               if (Context->tr1[i]<=Context->tr2[i]) {
                  if (Context->south == south) {
                     if ((int)Context->tr2[i]<jmax) jmax=(int)Context->tr2[i];
                  } else {
                     if ((int)Context->tr1[i]>j) j=(int)Context->tr1[i];
                  }
               } else {
                  if (midcolor == south) {
                     for (l=j; l<=jmax && l<= (int)Context->tr2[i]; l++)
                         DarkenPixel(Context, i, l, south);
                     for (l=jmax; l>=j && l>= (int)Context->tr1[i]; l--)
                         DarkenPixel(Context, i, l, south);
                     break;
                  }
               }
               for (l=j; l<=jmax; l++) DarkenPixel(Context, i, l, south);
               break;
            }
         }

         for (j=j0-1; j>=0; j--) {
            k = howDark(Context, i, j);
            if (k!=north) 
               DarkenPixel(Context, i, j, k);
            else {
               tr1 = (short) j+1;
               jmin = 0;
               if (j>0 && howDark(Context, i, 0) != north) {
                  for (l=0; l<tr1; l++) {
                     k = howDark(Context, i, l);
                     if (k!=north) 
                        DarkenPixel(Context, i, l, k);
                     else {
                        jmin = l;
                        break;
                     }
                  }
                  tr2 = (short)(-jmin-1);
               }
               if (Context->tr1[i]<=Context->tr2[i]) {
                  if (Context->south == south) {
                     if ((int)Context->tr1[i]>jmin) jmin=(int)Context->tr1[i];
                  } else {
                     if ((int)Context->tr2[i]<j) j=(int)Context->tr2[i];
                  }
               } else {
                  if (midcolor == north) {
                     for (l=jmin; l<=j && l<= (int)Context->tr2[i]; l++)
                         DarkenPixel(Context, i, l, north);
                     for (l=j; l>=jmin && l>= (int)Context->tr1[i]; l--)
                         DarkenPixel(Context, i, l, north);
                     break;
                  }
               }
               for (l=jmin; l<=j; l++) DarkenPixel(Context, i, l, north);
               break;
            }
         }

         Context->tr1[i] = tr1;
         Context->tr2[i] = tr2;
      }

      Context->south = south;
}

void
drawShadedArea (Context)
Sundata * Context;
{
     int size;

     if ((Context->flags.colorlevel<FULLCOLORS) || 
         (Context->flags.colorscale == 1)) {
        if (Context->flags.shading) {
           initShading(Context);
           moveNightArea(Context);
        } else {
           clearNightArea(Context);
           if (Context->tr1) {
              free(Context->tr1);
              Context->tr1 = NULL;
           }
        }
     } else {
        size = Context->xim->bytes_per_line*Context->xim->height;
        memcpy(Context->xim->data, Context->ximdata, size); 
        initShading(Context);
     }
}

City *
markLocation(Context, name)
struct Sundata * Context;
char *  name;
{
City *c;

        c = searchCityLocation(name);
        if (c) {
	   Context->mark1.city = c;
           if (Context->flags.colorlevel==MONOCHROME) 
              Context->mark1.flags = -1;
        }
	return c;
}

void
checkLocation(Context, name)
struct Sundata * Context;
char *  name;
{
        (void) markLocation(Context, name);

        if (Context->mark1.city == &Context->pos1) {
                Context->flags.map_mode = SOLARTIME;
                Context->mark1.city = NULL;
                setTZ(NULL);
                Context->mark1.city = &Context->pos1;
                if (Context->flags.colorlevel==MONOCHROME) pulseMarks(Context);
                Context->pos1.name = Label[L_POINT];
        } else
	    cityinit = Context->mark1.city;
}

/* --- */
/*  UPDIMAGE  --  Update current displayed image.  */

void
updateImage(Context)
struct Sundata * Context;
{
        int                     noon;
        double                  fnoon;
	double junk;

        /* If this is a full repaint of the window, force complete
           recalculation. */

        if (button_pressed) return;

        time(&Context->time);

	erase_obj = 1;
	if (Context->flags.colorlevel == MONOCHROME ||
            Context->flags.colorlevel == FULLCOLORS)
            drawSunAndMoon(Context);
        erase_obj = 0;

        (void) sunParams(Context->time + Context->jump, 
              &Context->sunlon, &Context->sundec, NULL);

        (void) phase(Context->time + Context->jump, 
              &Context->moondec, &Context->moonlon, 
              &junk,  &junk, &junk, &junk, &junk, &junk );
	Context->moonlon = fixangle(Context->moonlon+180.0) - 180.0;

        fnoon = Context->sunlon * (Context->zoom.width / 360.0) 
                         - (double) Context->zoom.dx;
        noon = (int) fnoon;
        Context->sunlon -= 180.0;

        /* Projecting the illumination curve  for the current seasonal
           instant is costly.  If we're running in real time, only  do
           it every PROJINT seconds.
           If the subsolar point has moved at least one pixel, also 
           update the illuminated area on the screen.   */

        if (Context->projtime < 0 || 
            (Context->time - Context->projtime) > PROJINT ||
            Context->noon != noon || Context->flags.update>=4) {
                Context->flags.update = 2;
                Context->projtime = Context->time;
                Context->noon = noon;
                Context->fnoon = fnoon;
                moveNightArea(Context);
		if (Context->flags.colorlevel==FULLCOLORS) {
		   drawAll(Context);
		   drawCities(Context);
		}
        }

        drawSunAndMoon(Context);
}

void 
setPosition1(Context, x, y)
Sundata *Context;
int x, y;
{
    Context->pos1.name = Label[L_POINT];
    Context->pos1.lat = 90.0-((double)(y+Context->zoom.dy)/
                              (double)Context->zoom.height)*180.0 ;
    Context->pos1.lon = ((double)(x+Context->zoom.dx)/
                         (double)Context->zoom.width)*360.0-180.0 ;
    Context->mark1.city = &Context->pos1;
}

void
updateUrban(Context, city)
Sundata *Context;
City *city;
{
    if (!do_urban) {
       if (city!=NULL && city == Context->mark1.city) 
          PopUrban(Context);
    } else {
       XMapWindow(dpy, Urban);
       XMapRaised(dpy, Urban);
    }
    if (do_urban) {
       updateUrbanEntries(Context, city);
       setupUrban(0);
    }
}

/*
 * processPoint() - This is kind of a cheesy way to do it but it works. What happens
 *                  is that when a different city is picked, the TZ environment
 *                  variable is set to the timezone of the city and then tzset().
 *                  is called to reset the system.
 */

void
processPoint(Context, x, y)
struct Sundata * Context;
int x, y;      /* Screen co-ordinates of mouse */
{
    /*
     * Local Variables
     */

    City *city;    /* Used to search for a city */
    int cx, cy;    /* Screen coordinates of the city */

    /* Loop through the cities until on close to the pointer is found */

    for (city = cityroot; city; city = city->next) {

        /* Convert the latitude and longitude of the cities to integer */

        if (city->size == 0) continue;
        if (Context->zoom.width <
               Context->sizelimits[city->size-1] && city!=cityinit) continue;
        cx = int_longitude(Context, city->lon)-x;
	cy = int_latitude(Context, city->lat)-y;

        /* Check to see if we are close enough */

        if (cx*cx+cy*cy <= 13) break;
    }

    if (Context->flags.map_mode == LEGALTIME) {
      if (city)
        Context->flags.map_mode = COORDINATES;
      else
        Context->flags.map_mode = SOLARTIME;
    }

    updateUrban(Context, city);

    switch(Context->flags.map_mode) {

      case COORDINATES:
      case EXTENSION:
        if (city)
           Context->mark1.city = city;
        Context->flags.update = 1;
        break;

      case DISTANCES:
        if (Context->mark2.city) {
	    if (Context->flags.colorlevel==FULLCOLORS) {
	       erase_obj = 2;
	       drawMarks(Context);
               erase_obj = 0;
	    }
	}
        if (Context->mark1.city == &Context->pos1) {
            Context->pos2 = Context->pos1;
            Context->mark2.city = &Context->pos2;
        } else
            Context->mark2.city = Context->mark1.city;
        if (city)
           Context->mark1.city = city;
        else
           setPosition1(Context, x, y);
        Context->flags.update = 2;
        break;

      case SOLARTIME:
        if (Context->mark1.city) {
	   if (Context->flags.colorlevel==FULLCOLORS) {
	      erase_obj = 1;
	      drawMarks(Context);
              erase_obj = 0;
	   }
	}
        if (city)
           Context->mark1.city = city;
        else
           setPosition1(Context, x, y);
        Context->flags.update = 2;
        break;

      default:
        break;
    }

    setDayParams(Context);

    if (Context->flags.colorlevel==MONOCHROME) {
      if (Context->mark1.city) Context->mark1.flags = -1;
      if (Context->mark2.city) Context->mark2.flags = -1;
    } else {
      drawAll(Context);
      showMapImage(Context);
      Context->flags.update = 2;
    }

    if (do_urban && !city) updateUrban(Context, Context->mark1.city);
}

void
report_failure(path, code)
char *path;
int code;
{
  switch(code) {
    case -1: fprintf(stderr, "%s:\nUnknown image format !!\n", path);
             break;

    case 0:  break;

    case 1:  fprintf(stderr, "Cannot read file %s !!\n", path); 
             break;

    case 2:  fprintf(stderr, "File %s has corrupted data!!\n", path);
             break;

    case 3:  fprintf(stderr, "Cannot decode format specification of %s !!\n", 
             path);
             break;

    case 4:  fprintf(stderr, 
                "Image creation failed (memory alloc. problem?) !!\n");
             break;

    case 5:  fprintf(stderr, "Header of file %s corrupted !!\n", path);
             break;

    case 6:  fprintf(stderr, "Color allocation failed !!\n");
             break;

    case 7:  fprintf(stderr, "Trying instead default  %s\n", path);
             break;

    default:
             fprintf(stderr, "Unknown error in  %s !!\n", path);
             break;
  }
}

#define COMPARE 260

void
quantize(Context)
Sundata * Context;
{
     int i, j, k, l, compare, quantum, change;
     unsigned short r[256], g[256], b[256];
     int count[256], done[256];
     char substit[256], value[256], inverse[256], compose[256];
     int d[COMPARE], v1[COMPARE], v2[COMPARE];
     int size, dist;
     XColor xc;

     if (verbose)
        fprintf(stderr, "Number of distinct colors in the map: %d colors\n",
                Context->ncolors);

     xc.flags = DoRed | DoGreen | DoBlue;

     for (i=0; i<256; i++) inverse[i] = '\0';

     for (i=0; i<Context->ncolors; i++) {
         count[i] = 0;
         xc.pixel = Context->daypixel[i];
         XQueryColor(dpy, tmp_cmap, &xc);
         r[i] = xc.red; g[i] = xc.green; b[i] = xc.blue;
	 inverse[(unsigned char)xc.pixel] = i;
     }

     if (tmp_cmap != cmap0)
        XFreeColormap(dpy, tmp_cmap);
     
     size = Context->xim->bytes_per_line * Context->xim->height;

     for (i=0; i<Context->ncolors; i++) {
         substit[i] = (char)i;
         value[i] = (char)i;
     }

     createGData(Context, 0);
     quantum = (256-Context->gdata->usedcolors)/2;     

     if (Context->ncolors<=quantum) goto finish;

     if (verbose)
        fprintf(stderr, "That's too much, quantizing to %d colors...\n", 
                 quantum);

     for (i=0; i<size; i++) ++count[(unsigned char)Context->xim->data[i]];
     
     compare = COMPARE;
     for (i=0; i<compare; i++) d[i] = 2147483647;

     for(i=0; i<Context->ncolors; i++)
        for(j=i+1; j<Context->ncolors; j++) {
            dist = abs((int)(r[i]-r[j]))+abs((int)(g[i]-g[j]))+
                                        +abs((int)(b[i]-b[j]));
            k=compare-1;
            while (k>=0 && dist<d[k]) k--;
            ++k;
            if (k<compare) {
              for (l=compare-1; l>k; l--) {
                  d[l] = d[l-1];
                  v1[l] = v1[l-1];
                  v2[l] = v2[l-1];
              }
              d[k] = dist;
              if (count[i]>count[j] || (count[i]==count[j] && i<j)) {
                 v1[k] = i;
                 v2[k] = j;
              } else {
                 v1[k] = j;
                 v2[k] = i;
              }
            }
        }

     l = 0;
     for (i=0; i<compare; i++) {
         j = v1[i];
         k = v2[i];
         if (substit[k]==(char) k) {
           substit[k] = j;
           ++l;
         }
         if (l>=Context->ncolors-quantum) break;
     }
     if (verbose)
        fprintf(stderr, 
           "%d substitutions from %d pairs of similar colors\n", l, i);

     change = 1;
     while (change) {
        change = 0;
        l = 0;
        for (i=0; i<Context->ncolors; i++) {
           j = (unsigned char) substit[i];
           if (substit[i]==(char)i) l++;
           if (substit[j] != (char)j) {
              substit[i] = substit[j];
              change = 1;
           }
        }
     }

  finish:

     if (verbose) {
        if (Context->gdata->cmap==cmap0)
           fprintf(stderr, "Allocating map colors in default colormap:\n");
        else         
           fprintf(stderr, "Allocating map colors in private colormap:\n");
     }
        
     for (i=0; i<Context->ncolors; i++) done[i] = 0;
     for (i=0; i<256; i++) Context->nightpixel[i] = (unsigned char)i;

     k=0;
     for (i=0; i<Context->ncolors; i++) {
        j = (unsigned char)substit[i];
        if (!done[j]) {
           xc.red = r[j];
           xc.green = g[j];
           xc.blue = b[j];
           if (!XAllocColor(dpy, Context->gdata->cmap, &xc)) {
              color_alloc_failed = 1;
              value[j] = 0;
           } else
              value[j] = (char)xc.pixel;
           xc.red = (unsigned int) (xc.red * Context->flags.darkness) / 255;
           xc.green = (unsigned int) (xc.green * Context->flags.darkness) / 255;
           xc.blue = (unsigned int) (xc.blue * Context->flags.darkness) / 255;
           if (!XAllocColor(dpy, Context->gdata->cmap, &xc)) 
              color_alloc_failed = 1;
           if (value[j]) {
	      Context->daypixel[k] = value[j];
              Context->nightpixel[(unsigned char)value[j]] = (char)xc.pixel;
	   }
           done[j] = 1;
           k++;
        }
     }

     if (Context->gdata->cmap==cmap0 && color_alloc_failed) {
        if (verbose) fprintf(stderr, "Failed !!\n");
        if (Context->gdata->links==0) free(Context->gdata);
        createGData(Context, 1);
        goto finish;
     }

     Context->ncolors = k;

     if (verbose)
        fprintf(stderr, "  2*%d+%d=%d colors allocated in colormap\n", 
	    k, Context->gdata->usedcolors, 2*k+Context->gdata->usedcolors);

     for (i=0; i<256; i++) compose[i] = 
       value[(unsigned char)substit[(unsigned char)inverse[(unsigned char)i]]];

     for (i=0; i<size; i++)
       Context->xim->data[i] = compose[(unsigned char)Context->xim->data[i]];
}

int
createImage(Context)
struct Sundata * Context;
{
   FILE *fd;
   char *file, path[1024]="";
   int code; 

   if (runlevel == IMAGERECYCLE) {
      if (verbose)
         fprintf(stderr, 
           "Recycling image (XID %ld) and changing requested parameters...\n",
	   (gflags.colorlevel==FULLCOLORS)? (long) Context->xim : 
                                       (long) Context->mappix);
      code = 0;
      if (gflags.colorlevel)
	 goto run_direct2;
      else
         goto run_direct1;
   }

   if (color_depth<=8 && Context->flags.colorlevel>0)
     tmp_cmap = XCreateColormap(dpy, Root, visual, AllocNone);
   else
     tmp_cmap = cmap0;

   file = (Context->wintype)? Context->map_img_file : Context->clock_img_file;

 do_path:

   Context->xim = NULL;
   code = -1;

   if (file) {
      strcpy(path, file);
      if (*file != '/' && *file != '.' ) {
         if ((fd=fopen(file, "r"))) {
            fclose(fd);
         } else {
            if (verbose)
              fprintf(stderr, "%s not in current directory ...\n"
                      "Trying to load %s from share directory instead\n", 
                      file, file);
            sprintf(path, "%s%s", share_maps_dir, file);
         }
      }
   }

   if (*path && strcmp(path, Default_img_file)) {
      if ((fd = fopen(path, "r"))) 
         fclose(fd);
      else {
         file = Default_img_file;
         fprintf(stderr, "File %s doesn't seem to exist !!\n"
                         "Trying default  %s\n",
                         path, file);
         goto do_path;
      }
   }

   if (Context->wintype) {
     if (Context->map_img_file && file!=Context->map_img_file)
        StringReAlloc(&Context->map_img_file, file);
   } else {
     if (Context->clock_img_file && file!=Context->clock_img_file) {
        StringReAlloc(&Context->clock_img_file, file);
     }
   }

   if (gflags.colorlevel < FULLCOLORS) {
   retry:
     code = readVMF(path, Context);
     if (code==0 && Context->bits) {
       Context->mappix = XCreatePixmapFromBitmapData(dpy, Root,
          Context->bits, Context->geom.width,
          Context->geom.height, 0, 1, 1);
     run_direct1:
       createGData(Context, 0);
       if (color_alloc_failed) report_failure(path, 6);
       if (Context->bits) free(Context->bits);
       createGCs(Context);
       return 0;
     } else {
       if (strcmp(path, Default_img_file)) {
          report_failure(path, 1);
          strcpy(path, Default_img_file);
          report_failure(path, 7);
          goto retry;
       }
       report_failure(path, 1);
       return 1;
     }
   }

   if (strstr(path, ".gif"))
      code = readGIF(path, Context);
   else   
   if (strstr(path, ".jpg"))
      code = readJPEG(path, Context);
   else
   if (strstr(path, ".png"))
      code = readPNG(path, Context);
   else     
   if (strstr(path, ".vmf"))
      code = readVMF(path, Context);
   else
   if (strstr(path, ".xpm"))
      code = readXPM(path, Context);

   if (code) {
      report_failure(path, code);
      if (strcmp(path, Default_img_file)) {
         file = Default_img_file;
         report_failure(file, 7);
         goto do_path;
      }
   }

 run_direct2:
   if (color_depth<=8)
      quantize(Context);
   
   if (color_alloc_failed) {
         code = 6;
	 if (Context->xim) {
            XDestroyImage(Context->xim);
            Context->xim = 0;
	 }
	 return code;
   }
    
   createGData(Context, 0);
   createGCs(Context);

   return code;
}

void 
createWorkImage(Context)
struct Sundata * Context;
{
   int size;

   if (Context->xim) {
     size = Context->xim->bytes_per_line*Context->xim->height;
     if (verbose)
        fprintf(stderr, "Creating work image data of size "
             "%d x %d x %d bpp = %d bytes\n", 
             Context->xim->width, Context->xim->height, 
             Context->xim->bytes_per_line/Context->xim->width,size);
     if (!Context->ximdata)
        Context->ximdata = (char *)salloc(size);
     memcpy(Context->ximdata, Context->xim->data, size); 
   }

}

Window GetVRoot(dpy)
     Display *dpy;
{
  int          i;
  Window       rootReturn, parentReturn, *children;
  unsigned int numChildren;
  Atom         __SWM_VROOT = None;
  Window       rslt = Root;

  __SWM_VROOT = XInternAtom(dpy, "__SWM_VROOT", False);
  XQueryTree(dpy, Root, &rootReturn, &parentReturn, &children, &numChildren);
  for (i=0; i<numChildren; i++)
    {
    Atom          actual_type;
    int           actual_format;
    unsigned long nitems, bytesafter;
    Window       *newRoot = NULL;

    /* item 148 in the FAQ neglects to mention that there is a race
     * condition here; consider a child of the root window that
     * existed when XQueryTree() was called, but has disappeared
     * before XGetWindowProperty() gets called for that window ...
     */
    if ((XGetWindowProperty(dpy, children[i], __SWM_VROOT, 0, 1,
                            False, XA_WINDOW, &actual_type,
                            &actual_format, &nitems, &bytesafter,
                            (unsigned char **) &newRoot) == Success)
        && newRoot)
      {
      rslt = *newRoot;
      break;
      }
    }

  /* item 148 in the FAQ also neglects to mention that we probably
   * want to free the list of children after we're done with it ...
   */
  XFree((void *) children);

  return rslt;
}

void
drawDottedRectangle(dpy, w, gc, x, y, a, b, pix1, pix2)
Display * dpy;
Drawable w;
GC gc;
int x, y, a, b;
Pixel pix1, pix2;
{
int i, j;

   for (j=y; j<=y+b; j+=b)
   for (i=x; i<=x+a; i++) { 
      XSetForeground(dpy, gc, ((i+j)%2)? pix1 : pix2);
      XDrawPoint(dpy, w, gc, i, j); 
   }

   for (i=x; i<=x+a; i+=a)
   for (j=y+1; j<y+b; j++) {
      XSetForeground(dpy, gc, ((i+j)%2)? pix1 : pix2);
      XDrawPoint(dpy, w, gc, i, j); 
   }
}

void
drawImageToRootWindow(Context, mode)
Sundata * Context;
int mode;
{
Window Vroot = GetVRoot(dpy);
Window win;
int wr = DisplayWidth(dpy,scr);
int hr = DisplayHeight(dpy,scr);

int a, b, c, i, j, k, l, ww, hw, mapped, update = 1;
int dx[5] = { 0, 0, 0, 1, -1};
int dy[5] = { 0, 1, -1, 0, 0};

     if (mode>0) {
        if (do_root < 1) 
	   do_root = 1;
	else
	   do_root = 2;
     }
     if (mode<0) {
        if (do_root > 0)  
           do_root = 0;
	else
           do_root = -1;
     }

     ww = Context->geom.width;
     hw = Context->geom.height;
     if (do_root == 2) hw += Context->hstrip;

     if (abs(Context->time - Context->roottime) >= root_period)
        Context->roottime = Context->time;
     else
        if (do_root == 2 && mode==0 && rootpix) update = 0;

     if (!rootpix)
        rootpix = XCreatePixmap(dpy, Root, wr, hr, DefaultDepth(dpy, scr));

     if (update) {
        XSetForeground(dpy, Context->gdata->wingc, 
                         Context->gdata->pixel[ROOTCOLOR]);
        XFillRectangle(dpy, rootpix, Context->gdata->wingc, 0, 0, wr, hr);
        srandom(Context->time);
        if (random_rootpos) {
           rootdx = (double)(random() % 10001)/10000.0;
           rootdy = (double)(random() % 10001)/10000.0;
        }
     }

     if (ww>=wr-5) a = 0; else a = (wr-ww-5)*rootdx;
     if (hw>=hr-5) b = 0; else b = (hr-hw-5)*rootdy;

     if (do_root >= 0 && update) {
        XSetForeground(dpy, Context->gdata->wingc,
	            Context->gdata->pixel[STARCOLOR]);
        c = random() % 191;
        for (i=0; i<wr; i++)
        for (j=0; j<hr; j++) {
	  if ((i*i+j)%971 == (j*j*j+i)%593 + c) {
	     if (((9*i+j*j)%7) == 0) l=4; else l=0;
	     for (k=0; k<=l; k++)
                XDrawPoint(dpy, rootpix, 
                     Context->gdata->wingc, i+dx[k], j+dy[k]);
	  }
	}
	if (do_root > 0)
        for (i=0; i<=5; i++)
	   drawDottedRectangle(dpy, rootpix, Context->gdata->wingc, 
                               a-i-1, b-i-1, ww+2*i+1, hw+2*i+1,
			       Context->gdata->pixel[MAPBGCOLOR],
			       Context->gdata->pixel[MAPFGCOLOR]);
     }

     if (do_root>=1) {
	win = Context->win;
        Context->win = XCreatePixmap(dpy, Root, ww, hw, DefaultDepth(dpy,scr));
	mapped = Context->flags.mapped;
	if (update) {
	   Context->flags.mapped = 1;
           Context->flags.update = 2;
           updateImage(Context);
           showMapImage(Context);
	}
	if (do_root == 2) {
	   RootCaller = Context;
	   XSetForeground(dpy, Context->gdata->wingc,
                               Context->gdata->pixel[MAPSTRIPBGCOLOR]);
           XFillRectangle(dpy, Context->win, Context->gdata->wingc, 
              0, Context->geom.height, Context->geom.width, Context->hstrip);
           Context->flags.bottom &= 1;
 	   drawBottomline(Context);
	   if (screen_saver) Context->flags.mapped = 1;
           Context->flags.hours_shown = 0;
           writeStrip(Context);
	}
	if (update)
           XCopyArea(dpy, Context->win, rootpix, Context->gdata->wingc,
                     0, 0, ww, hw, a, b);
	else
           XCopyArea(dpy, Context->win, Root, Context->gdata->wingc,
                     0, Context->geom.height, ww, Context->hstrip, 
                     a, b+Context->geom.height);
        XFlush(dpy);
        XFreePixmap(dpy, Context->win);
        Context->win = win;
        Context->flags.mapped = mapped;
     }

     if (update) {
        XSetWindowBackgroundPixmap(dpy, Vroot, rootpix);
        XClearWindow(dpy, Vroot);
     }
     XFlush(dpy);
     if (mode<0) {
        XFreePixmap(dpy, rootpix);
	rootpix = 0;
     }
}

void
warningNew(Context)
struct Sundata * Context;
{
   XFlush(dpy);
   clearStrip(Context);
   XFlush(dpy);
   usleep(TIMESTEP);
   drawTextStrip(Context, Label[L_NEWIMAGE], strlen(Label[L_NEWIMAGE]));
   XFlush(dpy);
}

void 
buildMap(Context, wintype, build)
struct Sundata * Context;
int wintype, build;
{
   Window win;
   int old_w, old_h, old_s, resize;

   if (build < 2) 
      resize = 0;
   else {
      resize = 1;
      build = 0;
   }

   if (build) {
      struct Sundata * NewContext;
      NewContext = (struct Sundata *)salloc(sizeof(struct Sundata));
      NewContext->next = NULL;
      if (Context) {
         if (Context->next) {
            NewContext->next = Context->next;
            Context->next = NewContext;
         } else
            Context->next = NewContext;
      } else
         Seed = NewContext;
      Context = NewContext;
      Context->wintype = wintype;
      if (do_menu<0) {
         do_menu = 1;
         MenuCaller = Context;
      }
      if (do_filesel<0) {
         do_filesel = 1;
         FileselCaller = Context;
      }
      if (do_zoom<0) {
         do_zoom = 1;
         ZoomCaller = Context;
      }
      if (do_option<0) {
         do_option = 1;
         OptionCaller = Context;
      }
   }

   makeContext(Context, build);

   win = Context->win;
   if (win)
      XSelectInput(dpy, Context->win, 0);

   if (createImage(Context)) {
     if (Seed->next) {
         shutDown(Context, 0);
         Context = Seed;
         return;
     } else
         shutDown(Context, -1);
   }
   checkGeom(Context, 0);

   if (win) {
      old_s = Context->hstrip;
      Context->hstrip = (wintype)? 
          Context->gdata->mapstrip : Context->gdata->clockstrip;
      setClassHints(Context->win, wintype);
      setSizeHints(Context, wintype);
      getPlacement(Context->win, &Context->geom.x, &Context->geom.y, 
                   &old_w, &old_h);
      old_h -= Context->hstrip;
      if (resize || Context->hstrip != old_s ||
          Context->geom.width!=old_w || Context->geom.height!=old_h) {
	 XMapRaised(dpy, Context->win);
	 Context->flags.mapped = 1;
	 XFlush(dpy);
         usleep(TIMESTEP);
         setAuxilWins(Context, REMAP);
      } else
         setAuxilWins(Context, RESET);
      if (runlevel!=IMAGERECYCLE || color_depth<=8)
         createWorkImage(Context);
      setProtocols(Context, Context->wintype);
   } else {
      createWorkImage(Context);
      if (screen_saver) {
         Context->flags.mapped = 0;
         do_root = 2;
	 Context->hstrip = Context->gdata->mapstrip;
	 checkLocation(Context, CityInit);
	 drawImageToRootWindow(Context, 0);
	 return;
      }
      Context->win = newWindow(Context, &Context->geom, wintype);
      setSizeHints(Context, wintype);
      XMapWindow(dpy, Context->win);
      Context->flags.mapped = 1;
      XFlush(dpy);
      usleep(TIMESTEP);
      setAuxilWins(Context, REATTRIB);
      setProtocols(Context, wintype);
      Context->prevgeom.width = 0;
   }

   checkLocation(Context, CityInit);
   if (Context->flags.colorlevel == MONOCHROME) drawAll(Context);
   clearStrip(Context);
   if (Context->gdata->cmap!=cmap0)
      XSetWindowColormap(dpy, Context->win, Context->gdata->cmap);
   runlevel = RUNNING;
   option_changes = 0;
   Context->flags.update = 4;
   updateImage(Context);
   showMapImage(Context);
   do_sync |= 2;
}

void
processStringEntry(keysym, entry)
KeySym keysym;
TextEntry *entry;
{
int i, j;
           i = strlen(entry->string);

           switch(keysym) {
             case XK_Left:
               if (entry->caret>0) --entry->caret;
               break;
             case XK_Right:
               if (entry->caret<i) ++entry->caret;
               break;
             case XK_Home:
               entry->caret = 0;
               break;
             case XK_End:
               entry->caret = strlen(entry->string);
               break;
             case XK_BackSpace:
             case XK_Delete:
               if (entry->caret>0) {
                  --entry->caret;
                  for (j=entry->caret; j<i;j++)
                     entry->string[j] = entry->string[j+1];
               }
               break;
             default:
               if (control_key) {
                  if (keysym==XK_space) {
                     keysym = 31;
                     goto specialspace;
                  }
                  if (keysym==XK_a) entry->caret = 0;
                  if (keysym==XK_b && entry->caret>0) --entry->caret;
                  if (keysym==XK_e) entry->caret = i;
                  if (keysym==XK_f && entry->caret<i) ++entry->caret;
                  if (keysym==XK_d) {
                     for (j=entry->caret; j<i;j++)
                        entry->string[j] = entry->string[j+1];
                  }
                  if (keysym==XK_k) {
                     entry->oldcaret = entry->caret;
                     entry->oldlength = i;
                     entry->oldchar = entry->string[entry->caret];
                     entry->string[entry->caret] = '\0';
                  }
                  if (keysym==XK_y && entry->caret==entry->oldcaret) {
                     entry->string[entry->oldcaret] = entry->oldchar;
                     entry->string[entry->oldlength] = '\0';
                     entry->oldcaret = -1;
                  }
                  break;
               }
           specialspace:
               if (keysym<31) break;
	       if (keysym>=XK_KP_Multiply && keysym<=XK_KP_9) 
                   keysym = keysym - XK_KP_0 + '0';
               if (keysym>255) break;
               if (i<entry->maxlength) {
                  for (j=i; j>entry->caret; j--)
                     entry->string[j] = entry->string[j-1];
                  entry->string[entry->caret] = (char) keysym;
                  entry->string[i+1] = '\0';
                  ++entry->caret;
               }
               break;
           }
}

/*
 *  Process key events in eventLoop
 */

void
processKey(win, keysym)
Window  win;
KeySym  keysym;
{
        double v;
        int i, j, old_mode;
        KeySym key;
        struct Sundata * Context = NULL;

        Context = getContext(win);
        if (!Context) return;
        if (Context->flags.colorlevel == FULLCOLORS) {
           if (!Context->xim) return;
        } else {
           if (!Context->mappix) return;
        }

        key = keysym;
        Context->flags.update = 1;
        if (key>=XK_A && key<=XK_Z) key += 32;
        old_mode = Context->flags.map_mode;

        if (win==Filesel) {
           switch(key) {
             case XK_Escape:
                if (do_filesel) 
                  PopFilesel(Context);
                return;
             case XK_Page_Up:
                if (filesel_shift == 0) return;
                filesel_shift -= num_lines/2;
                if (filesel_shift <0) filesel_shift = 0;
                break;
             case XK_Page_Down:
                if (num_table_entries-filesel_shift<num_lines) return;
                filesel_shift += num_lines/2;
                break;
             case XK_Up:
                if (filesel_shift == 0) return;
                filesel_shift -= 1;
                break;
             case XK_Down:
                if (num_table_entries-filesel_shift<num_lines) return;
                filesel_shift += 1;
                break;
             case XK_Home:
                if (filesel_shift == 0) return;
                filesel_shift = 0;
                break;
             case XK_End:
                if (num_table_entries-filesel_shift<num_lines) return;
                filesel_shift = num_table_entries - num_lines+2;
                break;
             case XK_Left:
             case XK_Right:
                return;
             default :
                goto general;
           }
           setupFilesel(1);
           return;
        }

        if (win==Zoom) {
           switch(key) {
             case XK_Escape:
                if (do_zoom) 
                  PopZoom(Context);
                return;
             default:
                goto general;
           }
        }

        if (win==Option) {
	   if (text_input!=OPTION_INPUT) goto general;
           switch(keysym) {
             case XK_Escape:
               if (do_option) 
                  PopOption(Context);
               return;
             case XK_KP_Enter:
             case XK_Return:
                  activateOption();
               return;
	     default:
	       processStringEntry(keysym, &option_entry);
               setupOption(0);
	       return;
	   }
        }

        if (win==Urban) {
	   if (text_input<URBAN_INPUT && keysym!=XK_Escape) goto general;
	   i = text_input-URBAN_INPUT;
           switch(keysym) {
             case XK_Escape:
               if (do_urban) 
                  PopUrban(Context);
               return;
             case XK_KP_Enter:
             case XK_Return:
	       key = keysym = XK_section;
	       goto general;
	       break;
	     default:
	       processStringEntry(keysym, &urban_entry[i]);
               setupUrban(0);
	       return;
	   }
        }

     general:
        switch(key) {
           case XK_Escape:
             if (do_menu) PopMenu(Context);
             return;
	   case XK_percent:
	     if (win==Option && do_option && text_input!=OPTION_INPUT) {
	        option_entry.oldcaret = 0;
	        option_entry.oldlength = strlen(option_entry.string);
	        option_entry.oldchar = *option_entry.string;
	        *option_entry.string = '\0';
	        option_entry.caret = 0;
                setupOption(0);
	     }
	     if (win==Urban && do_urban && text_input<URBAN_INPUT) {
	        for (i=0; i<=4; i++) {
	           urban_entry[i].oldcaret = 0;
	           urban_entry[i].oldlength = strlen(urban_entry[i].string);
	           urban_entry[i].oldchar = *urban_entry[i].string;
	           *urban_entry[i].string = '\0';
	           urban_entry[i].caret = 0;
		}
                setupUrban(0);
		goto erasemarks;
	     }
	     break;
           case XK_degree:
	     erase_obj = 1;
             if (Context->flags.objectmode == 2) drawSunAndMoon(Context);
             if (Context->flags.colorlevel != MANYCOLORS) drawCities(Context);
             Context->flags.dms = 1 -Context->flags.dms;
	     erase_obj = 0;
             if (Context->flags.objectmode == 2) drawSunAndMoon(Context);
             if (Context->flags.colorlevel == MONOCHROME) drawCities(Context);
             if (do_urban) {
	        if (Context->mark1.city) 
                   updateUrban(Context, Context->mark1.city);
                else {
		   for (i=2; i<=3; i++)
                      (void) num2str(dms2decim(urban_entry[i].string),
		         urban_entry[i].string, Context->flags.dms);
                   setupUrban(0);
		}
	     }
	     Context->flags.update = 2;
             return;
	   case XK_section:
	     if (do_urban) {
	        char params[256];
		sprintf(params, "%s\037|%s\037|%s\037",
                    urban_entry[0].string,
                    urban_entry[2].string,
                    urban_entry[3].string);
	        if (!markLocation(Context, params))
                   (void) markLocation(Context, urban_entry[0].string);
                updateUrban(Context, Context->mark1.city);
		Context->flags.update = 2;
	     }
	     break;
	   case XK_asciitilde:
	   case XK_parenright:
	     if (win == Urban && Context->mark1.city &&
                    Context->mark1.city != &Context->pos1) {
                City *c = Context->mark1.city;
		if (c) {
		   erase_obj = 1;
		   drawObject(Context, c->lon, c->lat, c->size, 1, c->name);
		   erase_obj = 0;
		} else return;
	        deleteMarkedCity();
	        Context->flags.update = 2;
	     }
	     if (keysym==XK_parenright)
	        break;
	   case XK_parenleft:
	     if (win == Urban) {
	        City * c = addCity(NULL);
		if (c) {
	           if (Context->mark1.city) {
		      if (Context->mark1.city == &Context->pos1) {
			 erase_obj = 1;
                         drawMarks(Context);
                         erase_obj = 0;
		      }
		   }
                   Context->mark1.city = c;
		   if (Context->flags.colorlevel==MONOCHROME) {
		      drawObject(Context,
                         c->lon, c->lat, c->size, 1, c->name);
                      Context->mark1.flags = -1;
		   }
	           Context->flags.update = 2;
		} else
		   setupUrban(0);
	     }
	     break;
           case XK_less:
             if (Context->prevgeom.width && 
                 (Context->prevgeom.width != Context->geom.width ||
                  Context->prevgeom.height != Context->geom.height)) {
                Context->geom = Context->prevgeom;
                Context->prevgeom.width = 0;
                adjustGeom(Context, 0);
		XResizeWindow(dpy, Context->win,
		      Context->geom.width, 
		      Context->geom.height+Context->gdata->menustrip);
                warningNew(Context);
                shutDown(Context, 0);
                buildMap(Context, Context->wintype, 0);
             }
             break;
           case XK_Home:
             label_shift = 0;
             return;
           case XK_End:
             label_shift = 50;
             clearStrip(Context);
             return;
           case XK_Page_Up: 
             if (label_shift>0)
               --label_shift;
             return;
           case XK_Page_Down: 
             if (label_shift<50)
               ++label_shift;
             return;
           case XK_equal:
	     if (do_sync & 1)
                do_sync = do_sync & 2;
	     else
                do_sync |= 1;
	     menu_lasthint = '\0';
	     option_lasthint = '\0';
	     option_newhint = keysym;
	     showOptionHint(getNumCmd(key));
             break;
           case XK_Delete:
	   case XK_BackSpace:
	   case XK_guillemotleft:
	     if (!memcmp(&Context->newzoom, &Context->oldzoom,
                         sizeof(ZoomSettings))) return;
	     Context->newzoom = Context->oldzoom;
	     setZoomDimension(Context);
             zoom_mode |= 15;
             activateZoom(Context, zoom_active);
             return;
           case XK_Left:
             v = 0.5/Context->newzoom.fx;
             Context->newzoom.fdx -= v;
             if (Context->newzoom.fdx<v) Context->newzoom.fdx = v;
             zoom_mode |= 14;
             activateZoom(Context, zoom_active);
             return;
           case XK_Right:
             v = 0.5/Context->newzoom.fx;
             Context->newzoom.fdx += v;
             if (Context->newzoom.fdx>1.0-v) Context->newzoom.fdx = 1.0-v;
             zoom_mode |= 14;
             activateZoom(Context, zoom_active);
             return;
           case XK_Up:
             v = 0.5/Context->newzoom.fy;
             Context->newzoom.fdy -= v;
             if (Context->newzoom.fdy<v) Context->newzoom.fdy = v;
             zoom_mode |= 14;
             activateZoom(Context, zoom_active);
             return;
           case XK_Down:
             v = 0.5/Context->newzoom.fy;
             Context->newzoom.fdy += v;
             if (Context->newzoom.fdy>1.0-v) Context->newzoom.fdy = 1.0-v;
             zoom_mode |= 14;
             activateZoom(Context, zoom_active);
             return;
           case XK_greater:
             if (do_dock && Context==Seed) break;
             Context->prevgeom = Context->geom;
	     i = DisplayWidth(dpy, scr);
             Context->geom.width = i - extra_width;
	     if (Context->geom.width<i/2) Context->geom.width = i/2;
	     if (Context->geom.width>i) Context->geom.width = i;
           case XK_KP_Divide:
             if (key == XK_KP_Divide) key = XK_slash;
           case XK_colon:
             if (key == XK_colon) key = XK_slash;
           case XK_slash:
             if (do_dock && Context==Seed) break;
             if (key == XK_slash) {
                Context->prevgeom = Context->geom;
                Context->zoom.mode = 2;
                Context->newzoom.mode = Context->zoom.mode;
             }
             if (!do_zoom)
                Context->newzoom = Context->zoom;
             if (setWindowAspect(Context, &Context->zoom)) {
                if (key == XK_greater || key == XK_slash) {
                   adjustGeom(Context, 0);
		   XResizeWindow(dpy, Context->win,
		      Context->geom.width, 
		      Context->geom.height+Context->gdata->menustrip);
                   Context->geom.x = extra_width/2;
                   XMoveWindow(dpy, Context->win, 
                      Context->geom.x, Context->geom.y);
                }
                warningNew(Context);
                shutDown(Context, 0);
                buildMap(Context, Context->wintype, 0);
                MapGeom = Context->geom;
             }
             break;
           case XK_apostrophe:
	     menu_lasthint = ' ';
	     option_lasthint = ' ';
	     Context->flags.animate = 1 - Context->flags.animate;
	     if (Context->flags.animate == 0)
	         Context->jump -= progress_value[Context->flags.progress];
	     Context->flags.update = 4;
	     break;
           case XK_quotedbl:
             if (do_zoom) zoom_active = 1 - zoom_active;
             zoom_mode = 30;
             activateZoom(Context, zoom_active);
             break;
           case XK_KP_Multiply:
           case XK_asterisk:
	     key = XK_asterisk;
             if (!memcmp(&Context->newzoom, 
                         &Context->zoom, sizeof(ZoomSettings))) break;
             activateZoom(Context, 1);
             break;
           case XK_period:
             if (Context->mark1.city) {
                Context->newzoom.fdx = 0.5+Context->mark1.city->lon/360.0;
                Context->newzoom.fdy = 0.5-Context->mark1.city->lat/180.0;
                zoom_mode |= 14;
                zoom_lasthint = ' ';
                activateZoom(Context, zoom_active);
             }
             break;
           case XK_at:
                activateOption();
		return;
           case XK_space:
           case XK_exclam:
	     key = XK_exclam;
             menu_newhint = XK_exclam;
             if (Context==Seed && do_dock) return;
             Context->wintype = 1 - Context->wintype;
             if (Context->wintype) {
                Context->geom.width = MapGeom.width;
                Context->geom.height = MapGeom.height;
             } else {
                Context->geom.width = ClockGeom.width;
                Context->geom.height = ClockGeom.height;
	     }
             adjustGeom(Context, 1);
     	     XSelectInput(dpy, Context->win, 0);
             setSizeHints(Context, Context->wintype);
             setClassHints(Context->win, Context->wintype);
             XMoveResizeWindow(dpy, Context->win, 
                 Context->geom.x, Context->geom.y, 
                 Context->geom.width, 
                 Context->geom.height+((Context->wintype)?
                     Context->gdata->mapstrip:Context->gdata->clockstrip));
             warningNew(Context);
             shutDown(Context, 0);
             buildMap(Context, Context->wintype, 0);
             return;
           case XK_1:
           case XK_KP_1:
	     key = XK_1;
             if (memcmp(&Context->newzoom, 
                        &gzoom, sizeof(ZoomSettings))) {
                Context->newzoom = gzoom;
                zoom_mode |= 15;
                activateZoom(Context, zoom_active);
             }
             break;
           case XK_numbersign:
             if (memcmp(&Context->newzoom, 
                        &Context->zoom, sizeof(ZoomSettings))) {
                Context->newzoom = Context->zoom;
                zoom_mode |= 15;
                activateZoom(Context, zoom_active);
             }
             break;
           case XK_plus:
           case XK_KP_Add:
	     key = XK_plus;
             Context->newzoom.fx *= ZFACT;
             Context->newzoom.fy *= ZFACT;
             setZoomDimension(Context);
             zoom_mode |= 14;
             activateZoom(Context, zoom_active);
             break;
           case XK_minus:
           case XK_KP_Subtract:
	     key = XK_minus;
             Context->newzoom.fx /= ZFACT;
             Context->newzoom.fy /= ZFACT;
             setZoomDimension(Context);  
             zoom_mode |= 14;
             activateZoom(Context, zoom_active);
             break;
           case XK_bracketright:
	     menu_lasthint = ' ';
	     option_lasthint = ' ';
	     drawImageToRootWindow(Context, -1);
	     break;
           case XK_bracketleft:
	     menu_lasthint = ' ';
	     option_lasthint = ' ';
	     drawImageToRootWindow(Context, 1);
	     break;
           case XK_ampersand:
             Context->newzoom.mode = (Context->newzoom.mode+1) %3;
             setZoomDimension(Context);             
             zoom_mode |= 13;
             activateZoom(Context, zoom_active);
             break;
           case XK_a: 
             Context->jump += progress_value[Context->flags.progress];
             Context->flags.update = 4;
             menu_lasthint = ' ';
             break;
           case XK_b: 
             Context->jump -= progress_value[Context->flags.progress];
             Context->flags.update = 4;
             menu_lasthint = ' ';
             break;
           case XK_c: 
             if (!Context->wintype) break;
             if (Context->flags.map_mode != COORDINATES) 
               Context->flags.dms = gflags.dms;
             else
               Context->flags.dms = 1 - Context->flags.dms;
             Context->flags.map_mode = COORDINATES;
             if (Context->mark1.city == &Context->pos1) {
		if (Context->flags.colorlevel==FULLCOLORS) {
		   erase_obj = 1;
		   drawMarks(Context);
                   erase_obj = 0;
		}
                Context->mark1.city = NULL;
	     }
             if (Context->mark1.city)
               setDayParams(Context);
             if (Context->mark2.city) {
		if (Context->flags.colorlevel==FULLCOLORS) {
		   erase_obj = 2;
		   drawMarks(Context);
                   erase_obj = 0;
		}
	     }
             Context->mark2.city = NULL;
             Context->flags.update = 2;
             break;
           case XK_d: 
             if (!Context->wintype) break;
             if (Context->flags.map_mode != DISTANCES) 
               Context->flags.dms = gflags.dms;
             else
               Context->flags.dms = 1 - Context->flags.dms;
             Context->flags.map_mode = DISTANCES;
             break;
           case XK_e: 
             if (!Context->wintype) break;
             Context->flags.map_mode = EXTENSION;
             old_mode = EXTENSION;
             Context->flags.hours_shown = 0;
             showHours(Context);
             break;
           case XK_f:
             if (!do_filesel)
                PopFilesel(Context);
             else {
		if (Filesel) {
                   XMapWindow(dpy, Filesel);
                   XMapRaised(dpy, Filesel);
		}
                if (FileselCaller != Context) {
                   PopFilesel(Context);
                   PopFilesel(Context);
                }
             }
             break;
           case XK_g: 
             if (!do_menu && win != Option)
               PopMenu(Context);
             else {
               menu_lasthint = ' ';
	       option_lasthint = ' ';
               if (keysym==XK_g) {
                  Context->flags.progress = (Context->flags.progress+1) % 6;
                  if (!progress_value[Context->flags.progress])
                     Context->flags.progress = 0;
               }
               if (keysym==XK_G) {
                  Context->flags.progress = (Context->flags.progress+5) % 6;
                  if (!progress_value[Context->flags.progress])
                     Context->flags.progress = 4;
               }
             }
             break;
           case XK_h:
             if (!do_menu) {
                menu_newhint = XK_space;
                PopMenu(Context);
		return;
             } else {
                if (MenuCaller != Context) {
                   PopMenu(Context);
                   PopMenu(Context);
                } else {
		   if (getState(Menu) == IsViewable) {
                      XMapRaised(dpy, Menu);
                      showManual();
		   } else
                      XMapWindow(dpy, Menu);
		}
             }
             break;
           case XK_i: 
             setAuxilWins(Context, ICONIFY);
             XIconifyWindow(dpy, Context->win, scr);
  	     Context->flags.mapped = 0;
             break;
           case XK_j:
             Context->jump = 0;
             Context->flags.update = 4;
             menu_lasthint = ' ';
	     option_lasthint = ' ';
             break;
           case XK_k:
	     if (Context==Seed && do_dock) return;
             if (do_menu) PopMenu(Context);
             if (do_filesel) PopFilesel(Context);
             if (Context==Seed && Seed->next==NULL)
                shutDown(Context, -1);
             else
                shutDown(Context, 1);
             return;
           case XK_l:
             if (!Context->wintype) {
                clearStrip(Context);
                if (!Context->wintype)
                   Context->flags.clock_mode = 
                     (Context->flags.clock_mode+1) % num_formats;
                Context->flags.update = 1;
                break;
             }
             Context->flags.map_mode = LEGALTIME;
	erasemarks:
	     if (Context->flags.colorlevel==FULLCOLORS) {
	        erase_obj = 3;
	        drawMarks(Context);
                erase_obj = 0;
	     }
             Context->mark1.city = NULL;
             Context->mark2.city = NULL;
             Context->flags.update = 2;
             break;
           case XK_m:
             if (!Context->wintype) break;
             if (Context->flags.colorlevel!=MANYCOLORS) {
	        erase_obj = 1;
	        drawLines(Context);
                erase_obj = 0;
	     }
	     if (keysym == XK_M)
                Context->flags.meridian = (Context->flags.meridian + 3) % 4;
	     else
                Context->flags.meridian = (Context->flags.meridian + 1) % 4;
             if (Context->flags.colorlevel<=FEWCOLORS) drawLines(Context);
             Context->flags.update = 2;
             break;
           case XK_n:
             if ((Context->flags.colorlevel<FULLCOLORS) || 
                 (Context->flags.colorscale == 1))
		Context->flags.shading = 1 - Context->flags.shading;
	     else {
                if (keysym==XK_n) 
                   Context->flags.shading = (Context->flags.shading + 1) % 6;
                if (keysym==XK_N) 
                   Context->flags.shading = (Context->flags.shading + 5) % 6;
             }
             drawShadedArea(Context);
             Context->flags.update = 4;
             break;
           case XK_o:
             if (!do_option)
                PopOption(Context);
             else {
                XMapWindow(dpy, Option);
                XMapRaised(dpy, Option);
                if (OptionCaller != Context) {
                   PopOption(Context);
                   PopOption(Context);
                }
             }
             break;
           case XK_p:
             if (!Context->wintype) break;
             if (Context->flags.colorlevel!=MANYCOLORS) {
	        erase_obj = 1;
	        drawLines(Context);
                erase_obj = 0;
	     }
	     i = Context->flags.parallel;
             if (keysym == XK_P) 
                Context->flags.parallel = ((i+3)&3) + (i&8);
	     else
                Context->flags.parallel = ((i+1)&3) + (i&8);
             if (Context->flags.colorlevel<=FEWCOLORS) drawLines(Context);
             Context->flags.update = 2;
             break;
           case XK_q: 
	     if (!do_dock)
                shutDown(Context, -1);
             break;
           case XK_s: 
             if (Context->flags.map_mode != SOLARTIME) 
               Context->flags.dms = gflags.dms;
             else
               Context->flags.dms = 1 - Context->flags.dms;
             Context->flags.map_mode = SOLARTIME;
             if (Context->mark2.city) {
		if (Context->flags.colorlevel==FULLCOLORS) {
		   erase_obj = 2;
		   drawMarks(Context);
                   erase_obj = 0;
		}
	     }
             Context->mark2.city = NULL;
             if (Context->mark1.city)
               setDayParams(Context);
             Context->flags.update = 2;
             break;
           case XK_t:
             if (!Context->wintype) break;
             if (Context->flags.colorlevel!=MANYCOLORS) {
	        erase_obj = 1;
	        drawLines(Context);
                erase_obj = 0;
	     }
             Context->flags.parallel = (Context->flags.parallel + 8) & 15;
             if (Context->flags.colorlevel<=FEWCOLORS) drawLines(Context);
             Context->flags.update = 2;
             break;
           case XK_u:
             if (!Context->wintype) break;
	     if (!do_urban) {
	        PopUrban(Context);
	        updateUrban(Context, Context->mark1.city);
		break;
	     } else {
	        if (getState(Urban)!=IsViewable) {
                   XMapWindow(dpy, Urban);
	           XMapRaised(dpy, Urban);
	           updateUrban(Context, Context->mark1.city);
		   break;
                } else
	           XMapRaised(dpy, Urban);
	     }
             if (Context->flags.colorlevel!=MANYCOLORS) {
	        erase_obj = 1;
	        drawCities(Context);
                erase_obj = 0;
	     }
             if (keysym == XK_U)
                Context->flags.citymode = (Context->flags.citymode + 3) % 4;
	     else
                Context->flags.citymode = (Context->flags.citymode + 1) % 4;
             if (Context->flags.colorlevel==MONOCHROME) drawCities(Context);
             Context->flags.update = 2;
             break;
           case XK_w: 
             if (Context->time<=last_time+2) return;
             if (do_menu) do_menu = -1;
             if (do_filesel) do_filesel = -1;
             if (do_zoom) do_zoom = -1;
             if (do_option) do_option = -1;
             buildMap(Context, 1, 1);
	     keysym = ' ';
             break;
           case XK_r:
             clearStrip(Context);
             Context->flags.update = 4;             
             break;
           case XK_x:
             if (ExternAction) 
                system(ExternAction);
             break;
           case XK_y:
	     erase_obj = 1;
             drawSunAndMoon(Context);
             erase_obj = 0;
             if (keysym==XK_y) 
                Context->flags.objectmode = (Context->flags.objectmode+1) % 3;
             if (keysym==XK_Y) 
                Context->flags.objectmode = (Context->flags.objectmode+2) % 3; 
             drawSunAndMoon(Context);
             Context->flags.update = 2;
             break;
           case XK_z:
             if (!do_zoom)
                PopZoom(Context);
             else {
                XMapWindow(dpy, Zoom);
                XMapRaised(dpy, Zoom);
                if (ZoomCaller != Context) {
                   PopZoom(Context);
                   PopZoom(Context);
                }
             }
             break;
           default:
             if (!Context->wintype) {
               Context->flags.clock_mode = 
                   (1+Context->flags.clock_mode) % num_formats;
               Context->flags.update = 1;
             }
             break ;
        }

        if (old_mode == EXTENSION && Context->flags.map_mode != old_mode) 
           clearStrip(Context);

        if (do_menu) {
	   if (getNumCmd(toupper(key))>=0)
	      menu_newhint = toupper(key);
	   j = -1;
	   for (i=0; i<N_MENU; i++) if (MenuKey[2*i]==toupper(key)) { 
	      j=i; 
	      break; 
	   }
           showMenuHint(j);
	}

        if (do_zoom) {
	   if (getNumCmd(toupper(key))>=0)
	      zoom_newhint = toupper(key);
	   j = -1;
	   for (i=0; i<N_ZOOM; i++) if (ZoomKey[2*i]==toupper(key)) { 
	      j=i; 
	      break; 
	   }
           showZoomHint(j);
	}

        if (do_option) {
	   if (getNumCmd(toupper(key))>=0)
	      option_newhint = toupper(key);
	   j = -1;
	   for (i=0; i<N_OPTION; i++) if (OptionKey[2*i]==toupper(key)) { 
	      j=i; 
	      break; 
	   }
           showOptionHint(j);
	}
}

/*
 *  Process mouse events from eventLoop
 */

void
processMouseEvent(win, x, y, button, evtype)
Window  win;
int     x, y;
int     button;
int     evtype;
{
static int  x0 = -1, y0 = -1, pressed3 = 0;
static int u, v, w = -1, h = -1;
static Pixmap savepix = 0;
struct Sundata * Context = (struct Sundata *) NULL;

        Context = getContext(win);
        if (!Context) return;
        if (Context->flags.colorlevel==FULLCOLORS) {
           if (!Context->xim) return;
        } else {
           if (!Context->mappix) return;
        }

        if (evtype!=MotionNotify) RaiseAndFocus(win);

        if (evtype == ButtonPress) {
	   if (win == Context->win && !Context->wintype) return;
	}

        if (win == Menu) {
	   processMenuAction(MenuCaller, x, y, button, evtype);
	   return;
        }

        if (win == Filesel) {
           processFileselAction(FileselCaller, x, y, evtype);
           return;
        }

        if (win == Zoom) {
           processZoomAction(ZoomCaller, x, y, button, evtype);
           return;
        }

        if (win == Option) {
           processOptionAction(OptionCaller, x, y, button, evtype);
           return;
        }

        if (win == Urban) {
           processUrbanAction(UrbanCaller, x, y, button, evtype);
           return;
        }

        /* Click on bottom strip of window */
        if (y >= Context->geom.height) {
           if (button==1) {
	      if (evtype==ButtonPress) return;
	      if (do_menu && getState(Menu)==IsViewable)
                 processKey(win, XK_o);
	      else
                 processKey(win, XK_h);
              return;
           }
           if (button==2) {
              processKey(win, XK_l);
              return;
           }
           if (button==3) {
              /* Open new window */
              if (evtype==ButtonPress || !focus_in) return;
	      if (pressed3) {
		 y = Context->geom.height-1;
		 goto rect;
	      }
              processKey(win, XK_w);
              return;
           }
        }

        /* Click on the map with button 2*/
        if (button==2) {
           processKey(win, XK_f);
           return;
        }

   	if (!Context->wintype) pressed3 = 0;

        if (evtype == MotionNotify && pressed3 && !do_zoom &&
            x0!=-1 && y0!= -1) {
	   if (w>0 && h>0) {
	      if (savepix) {
                 XCopyArea(dpy, savepix, Context->win, Context->gdata->wingc,
                        0, 0, w+1, h+1, u, v);
		 XFreePixmap(dpy, savepix);
		 savepix = 0;
	      }
	   }
	   if (x0<x) { u = x0; w = x-x0; } else { u = x; w = x0-x; }
	   if (y0<y) { v = y0; h = y-y0; } else { v = y; h = y0-y; }
	   if (w>0 && h>0)
	   savepix = XCreatePixmap(dpy, Root, w+1, h+1, DefaultDepth(dpy,scr));
	   if (savepix)
              XCopyArea(dpy, Context->win, savepix, Context->gdata->wingc,
                        u, v, w+1, h+1, 0, 0);
           XDrawRectangle(dpy, Context->win, Context->gdata->wingc, 
              u, v, w, h);
           Context->flags.update = 4;
	   return;
        }

        /* Click on the map with button 3*/
        if (button==3) {
	   if (!Context->wintype) {
	      if (evtype == ButtonRelease)
                 processKey(win, XK_z);
	      return;
	   }
           if (do_zoom && win==ZoomCaller->win) {
              Context->newzoom.fdx = ((double)(x+Context->zoom.dx))
                         /((double)Context->zoom.width);
              Context->newzoom.fdy = ((double)(y+Context->zoom.dy))
                         /((double)Context->zoom.height);
              setZoomAspect(Context, 3);
              setZoomDimension(Context);
              zoom_mode = 14;
              zoom_lasthint = ' ';
              activateZoom(Context, zoom_active);
	   } else {
	      if (evtype == ButtonPress) {
	         pressed3 = 1;
	         x0 = x;
	         y0 = y;
		 return;
	      }
	      if (x0<x) { u = x0; w = x-x0; } else { u = x; w = x0-x; }
	      if (y0<y) { v = y0; h = y-y0; } else { v = y; h = y0-y; }
	      if (evtype == ButtonRelease) {
		 double fact;
	         pressed3 = 0;
		 if (savepix) {
		    XFreePixmap(dpy, savepix);
		    savepix = 0;
		 }
	         if (x==x0 && y==y0) {
                    /* Open zoom filesel */
                    processKey(win, XK_z);
		    return;
		 }
	       rect:
                 Context->newzoom.fdx += 
                    (((double)(u+w/2)/(double)Context->geom.width)-0.5)/
                       Context->newzoom.fx;
                 Context->newzoom.fdy += 
                    (((double)(v+h/2)/(double)Context->geom.height)-0.5)/
                       Context->newzoom.fy;
                 fact = sqrt( ((double)Context->geom.width)/((double)w) *
                          ((double)Context->geom.height)/((double)h) );
                 Context->newzoom.fx *= fact;
                 Context->newzoom.fy *= fact;
                 setZoomDimension(Context);
                 zoom_mode |= 14;
                 activateZoom(Context, zoom_active);
	         x0 = -1;
	         y0 = -1;
		 return;
	      }
	   }
           return;
        } 
        
        if (evtype == MotionNotify) return;
        /* Click with button 1 on the map */

        /* It's a clock, just execute predefined command */
        if (!Context->wintype) {
           if (ExternAction) 
              system(ExternAction);
	   else
	   if (!do_menu) {
	      menu_lasthint = '\0';
	      menu_newhint = ' ';
	      PopMenu(Context);
	   }
           return;
        }

        /* Otherwise, user wants to get info on a city or a location */
        Context->flags.update = 1;

        /* Set the timezone, marks, etc, on a button press */
        if (evtype==ButtonPress) return;   
        processPoint(Context, x, y);
}

void 
processResize(win)
Window win;
{
           int i, x, y, w, h, num = 0;
           struct Sundata * Context = NULL;
           struct Geometry * Geom = NULL;

           if (win == Menu) return;

           if (win == Filesel) {
	      if (!do_filesel) return;
              Geom = &FileselGeom;
              num = 3;
           }
           if (win == Zoom) {
	      if (!do_zoom) return;
              Geom = &ZoomGeom;
              num = 4;
           }
           if (win == Option) {
	      if (!do_option) return;
              Geom = &OptionGeom;
              num = 5;
           }
           if (win == Urban) {
	      if (!do_urban) return;
              Geom = &UrbanGeom;
              num = 6;
           }

           if (num) {
              if (getPlacement(win, &x, &y, &w, &h)) return;
              if (w==Geom->width && h==Geom->height) return;
              if (w<Geom->w_mini) w = Geom->w_mini;
              if (h<Geom->h_mini) h = Geom->h_mini;
              Geom->width = w;
              Geom->height = h;
              if (verbose)
                 fprintf(stderr, "Resizing %s to %d %d\n", 
                    widget_type[num], w, h);
              XSelectInput(dpy, win, 0);
              setSizeHints(NULL, num);
              setProtocols(NULL, num);
              if (num==3)
                 setupFilesel(-1);
              if (num==4) {
                 if (zoompix) {
                    XFreePixmap(dpy, zoompix);
                    zoompix = 0;
                 }
                 setupZoom(-1);
              }
              if (num==5) {
   	         w = ((OptionGeom.width-86) / 
                   XTextWidth(OptionCaller->gdata->font[MENUFONT], "_", 1))-2;
		 resetStringLength(w, &option_entry);
                 setupOption(-1);
              }
              if (num==6) {
		 text_input = NULL_INPUT;
		 setupUrban(-2);
                 for (i=0; i<=4; i++) {
	            w = (urban_w[i]/ 
                      XTextWidth(UrbanCaller->gdata->font[MENUFONT],"_",1))-2;
	            resetStringLength(w, &urban_entry[i]);
		    urban_entry[i].string[urban_entry[i].maxlength] = '\0';
		 }
		 setupUrban(-1);
	      }
              return;
           }

           Context = getContext(win);
           if(!Context) return;

           if (Context==Seed && !Context->wintype && do_dock) return;

           if (getPlacement(win, &x, &y, &w, &h)) return;
           h -= Context->hstrip;
           if (w==Context->geom.width && h==Context->geom.height) return;
           Context->prevgeom = Context->geom;
           if (w<Context->geom.w_mini) w = Context->geom.w_mini;
           if (h<Context->geom.h_mini) h = Context->geom.h_mini;
	   Context->flags.update=2;
	   showMapImage(Context);
	   clearStrip(Context);
	   writeStrip(Context);
	   XFlush(dpy);
           Context->geom.width = w;
           Context->geom.height = h;
           if (Context->wintype) {
              MapGeom.width = w;
              MapGeom.height = h;
           } else {
              ClockGeom.width = w;
              ClockGeom.height = h;
           }
           adjustGeom(Context, 0);
	   warningNew(Context);
           shutDown(Context, 0);
           setZoomAspect(Context, 3);
           buildMap(Context, Context->wintype, 2);
           XFlush(dpy);
           usleep(2*TIMESTEP);
}

/*
 * Got an expose event for window w.  Do the right thing if it's not
 * currently the one we're displaying.
 */

void
doTimeout(Context)
struct Sundata * Context;
{
        if (!Context) return;

        if (QLength(dpy) && Context->flags.update <= 2)
                return;         /* ensure events processed first */

        if (Context->flags.update)
           Context->count = 0;
        else
           Context->count = (Context->count+1) % TIMECOUNT;

        if (Context->count==0) {
           updateImage(Context);
           showMapImage(Context);
           writeStrip(Context);
           if (Context->flags.colorlevel==MONOCHROME) pulseMarks(Context);
           if (do_root == 2 && Context == RootCaller)
              drawImageToRootWindow(Context, 0);
	   XFlush(dpy);
	   if (Context->flags.animate) {
	      if (abs(Context->time-Context->animtime) >= 
                  Context->flags.animperiod) {
		 Context->animtime = Context->time;
	         Context->jump += progress_value[Context->flags.progress];
                 Context->flags.update = 4;
	      }
	   }
        }
}

void
doExpose(w)
Window w;
{
        struct Sundata * Context;

        Context = getContext(w);
        if (!Context) return;

        if (w == Menu) { setupMenu(-1); return; }
        if (w == Filesel) { setupFilesel(-1); return; }
        if (w == Zoom) { setupZoom(-1); return; }
        if (w == Option) { setupOption(-1); return; }
        if (w == Urban) { setupUrban(-1); return; }

        Context->flags.update = 2;
        showMapImage(Context);

	Context->flags.bottom &= 1;
	drawBottomline(Context);

	clearStrip(Context);
	Context->flags.hours_shown = 0;
        writeStrip(Context);
}

/*
 * Someone is sure to wonder why the event loop is coded this way, without
 * using select().  The answer is that this was developed on a System V
 * kernel, which has select() but the call has bugs; so, I was inspired
 * to make it portable to systems without select().  The slight delay in
 * expose event processing that results from using sleep(1) rather than
 * alarm() is a fine payoff for not having to worry about interrupted
 * system calls.
 *
 * I've got to use XCheckIfEvent with a degenerate predicate rather than
 * XCheckMaskEvent with a mask of -1L because the latter won't collect all
 * types of events, notably ClientMessage and Selection events.  Sigh.
 */

Bool
evpred(d, e, a)
Display *              d;
XEvent *               e;
XPointer               a;
{
        return (True);
}

void
eventLoop()
{
        XEvent                  ev;
        Sundata *               Context;
	Sundata *               Which;
        char                    buffer[1];
        KeySym                  keysym;

        for (;;) {
	   if (XCheckIfEvent(dpy, &ev, evpred, (XPointer)0)) {

	      /*
              fprintf(stderr, "Event %d, Window %d \n"
                   "  (Main %d, Menu %d, Sel %d, Zoom %d, Option %d)\n", 
                   ev.type, ev.xexpose.window, 
                   Seed->win, Menu, Filesel, Zoom, Option);
	      */

              switch(ev.type) {

                 case EnterNotify:
		      focus_in = 1;
		      break;
		 
                 case LeaveNotify:
		      focus_in = 0;
		      break;
		 
                 case FocusOut:
		      if (do_option && text_input == OPTION_INPUT &&
                          ev.xexpose.window == Option) {
			 text_input = NULL_INPUT;
			 setupOption(0);
		      }
		      if (do_urban && text_input >= URBAN_INPUT &&
                          ev.xexpose.window == Urban) {
			 text_input = NULL_INPUT;
			 setupUrban(0);
		      }
		      break;

                 case VisibilityNotify:
                      doExpose(ev.xexpose.window);
                      break;
		      
                 case MapNotify:
                      Context = getContext(ev.xexpose.window);
		      if (!Context) break;
		      if (Context->win!=ev.xexpose.window) break;
                      setAuxilWins(Context, DEICONIFY);
   	              Context->flags.mapped = 1;
                      updateImage(Context);
   	              Context->flags.update = 2;
		      showMapImage(Context);
		      writeStrip(Context);
		      break;

                 case UnmapNotify:
                      Context = getContext(ev.xexpose.window);
		      if (!Context) break;
		      if (Context->win!=ev.xexpose.window) break;
                      setAuxilWins(Context, ICONIFY);
   	              Context->flags.mapped = 0;
		      break;
		     
                 case Expose:
                      if (ev.xexpose.count == 0)
                      doExpose(ev.xexpose.window);
                      break;

                 case ClientMessage:
                      if (ev.xclient.message_type == wm_protocols &&
                          ev.xclient.format == 32 &&
                          ev.xclient.data.l[0] == wm_delete_window) {
                            if (ev.xexpose.window == Menu) PopMenu(MenuCaller);
                               else
                            if (ev.xexpose.window == Filesel) 
                                  PopFilesel(FileselCaller);
                               else
                            if (ev.xexpose.window == Zoom) PopZoom(ZoomCaller);
                               else
                            if (ev.xexpose.window == Option)
                                            PopOption(OptionCaller);
			       else
                            if (ev.xexpose.window == Urban)
                                            PopUrban(UrbanCaller);
			       else {
			         Context = getContext(ev.xexpose.window);
		                 if (!Context) break;
				 if (Context!=Seed || !do_dock)
                                    shutDown(Context, 1);
				 }
		      }
                      break;

                 case KeyPress:
                 case KeyRelease:
                      XLookupString((XKeyEvent *) &ev, buffer, 1, &keysym, NULL);
                      if (keysym==XK_Control_L || keysym==XK_Control_R) {
                         if (ev.type == KeyPress) control_key = 1;
                         if (ev.type == KeyRelease) control_key = 0;
                      } else
                         if (ev.type == KeyPress && keysym != XK_Mode_switch)
                            processKey(ev.xexpose.window, keysym);
                      break;

                 case ButtonPress:
		 case ButtonRelease:
                 case MotionNotify:
                      if (ev.type==ButtonPress)
                            button_pressed = ev.xbutton.button;
                      if (ev.type==ButtonRelease) button_pressed = 0;
                      processMouseEvent(ev.xexpose.window,
                            ev.xbutton.x, ev.xbutton.y,
                            ev.xbutton.button, ev.type);
                      break;

		 /* case ResizeRequest: */
		 case PropertyNotify:
		 case ConfigureNotify:
                      processResize(ev.xexpose.window);
                      break;
			
                 default:
		      break;
	      }
	   } else {
              Which = getContext(ev.xexpose.window);
              usleep(TIMESTEP);
              if (Which == NULL) Which = Seed;
	      if (Which->win==ev.xexpose.window)
	         processResize(Which->win);
              for (Context = Seed; Context; Context = Context->next)
                 if (do_sync || Context == Which || Context == RootCaller ||
                     (do_dock && Context == Seed)) {
		    if (do_sync & 2) Context->flags.update = 2;
                    doTimeout(Context);
		 }
	      do_sync &= 1;
	   }
        }
}

int
main(argc, argv)
int             argc;
char **         argv;
{
        char * p;
        int i;

        ProgName = *argv;
        if ((p = strrchr(ProgName, '/'))) ProgName = ++p;

        /* Set default values */
        initValues();

        /* Set Language */
        if (getenv("LANG")) {
           strncpy(language, getenv("LANG"), 2);
	   language[2] = '\0';
	}
        if (!*language)
           strcpy (language,"en");

        /* Check if options define some new language */
        for (i=1; i<argc-1; i++)
            if (!strcasecmp(argv[i], "-language")) {
	       strncpy(language, argv[++i], 2);
	       language[2] = '\0';
	    }

        /* Read the app-default config file */
        runlevel = READSYSRC;
        if (readRC(app_default, 1)) exit(1);

        /* Check if user has provided another config file */
        runlevel = READUSERRC;
	verbose = 0;
        checkRCfile(argc, argv);

        if (rc_file) p = rc_file;
           else
        if ((p = tildepath("~/.sunclockrc")) == NULL) {
           fprintf(stderr, 
               "Unable to get path to ~/.sunclockrc\n");
	}

	if (p && *p) (void) readRC(p, 0);

        runlevel = PARSECMDLINE;
        (void) parseArgs(argc, argv);
        runlevel = RUNNING;

        /* Open Display */

        dpy = XOpenDisplay(Display_name);
        if (dpy == (Display *)NULL) {
                fprintf(stderr, "%s: can't open display `%s'\n", ProgName,
                        Display_name);
                exit(1);
        }

        scr = DefaultScreen(dpy);
        Root = RootWindow(dpy, scr);

        color_depth = DefaultDepth(dpy, scr);
	bigendian = (ImageByteOrder(dpy) == MSBFirst);
        visual = DefaultVisual(dpy, scr);
        cmap0 = DefaultColormap(dpy, scr);

        black = BlackPixel(dpy, scr);
        white = WhitePixel(dpy, scr);

        if (color_depth == 16 && visual->green_mask==992) color_depth = 15;
        if (color_depth > 16)
            color_pad = 32;
        else if (color_depth > 8)
            color_pad = 16;
        else
            color_pad = 8;

        wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
        wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

        if (verbose) {
           fprintf(stderr, 
              "%s: version %s, %s\nDepth %d    Bits/RGB %d   Bigendian : %s\n"
              "Red mask %ld    Green mask %ld    Blue mask %ld\n", 
                ProgName, VERSION, COPYRIGHT,
                color_depth, visual->bits_per_rgb, (bigendian)? "yes":"no",
                visual->red_mask, visual->green_mask, visual->blue_mask);
        }

        /* Correct some option parameters */
        if (placement<0) placement = NW;
        city_spotsizes = (int *) salloc(city_cat * sizeof(int));
        city_sizelimits = (int *) salloc(city_cat * sizeof(int));
        correctValues();

        parseFormats(ListFormats);
        buildMap(NULL, win_type, 1);

        eventLoop();
        exit(0);
}


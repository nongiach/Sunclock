/*
 * Sun clock definitions.
 */

#define XK_MISCELLANY
#define XK_LATIN1

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>
#include <X11/keysym.h>
#include <X11/xpm.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include "version.h"

#define EDITORCOMMAND EMXBINDIR"/emx -edit 0 -fn 9x15"  /* Default text editor */
#define	FAILFONT	"fixed"

/* num of bitmaps to accomodate 1 mark and 2 spatial objets (Sun, Moon) */
#define SPECIALBITMAPS 3
#define CITYBITMAPS 5

#define MENU_WIDTH 38
#define SEL_WIDTH 20
#define SEL_HEIGHT 10
#define MAXSHORT 32767
#define TOPTITLEBARHEIGHT 40   /* usual height of top title bar for KDE 
				  can put 0 instead if you don't use that! */

#define abs(x) ((x) < 0 ? (-(x)) : x)			  /* Absolute value */
#define sgn(x) (((x) < 0) ? -1 : ((x) > 0 ? 1 : 0))	  /* Extract sign */
#define fixangle(a) ((a) - 360.0 * (floor((a) / 360.0)))  /* Fix angle	  */
#define torad(d) ((d) * (PI / 180.0))                     /* Deg->Rad     */
#define todeg(d) ((d) * (180.0 / PI))                     /* Rad->Deg     */
#define dsin(x) (sin(torad((x))))                         /* Sin from deg */
#define dcos(x) (cos(torad((x))))                         /* Cos from deg */


#define PI 3.14159265358979323846
#define ZFACT 1.2

#define COLORLENGTH 48             /* maximum length of color names */

#define TERMINC  100		   /* Circle segments for terminator */

#define PROJINT  (60 * 10)	   /* Frequency of seasonal recalculation
				      in seconds. */

#define EARTHRADIUS_KM  6378.160
#define EARTHRADIUS_ML  3963.202
#define SUN_APPRADIUS   0.266      /* Sun apparent radius, in degrees */
#define ATM_REFRACTION  0.100      /* Atmospheric refraction, in degrees */
#define ATM_DIFFUSION   3.0        /* Atmospheric diffusion, in degrees */

#define COORDINATES 'c'
#define DISTANCES 'd'
#define EXTENSION 'e'
#define LEGALTIME 'l'
#define SOLARTIME 's'

#define TIMECOUNT 25
#define TIMESTEP  10000

enum {MONOCHROME=0, FEWCOLORS, MANYCOLORS, FULLCOLORS};

enum {RESET=0, REMAP, REATTRIB, DESTROY, ICONIFY, DEICONIFY};

enum {RANDOM=0, FIXED, CENTER, NW, NE, SW, SE};

enum {READSYSRC = 0, READUSERRC, PARSECMDLINE, RUNNING, RUNTIMEOPTION, 
      IMAGERECYCLE, FAILEDOPTION};

enum {NULL_INPUT = 0, OPTION_INPUT, URBAN_INPUT};

enum { 
  CLOCKBGCOLOR=0, MAPBGCOLOR, MENUBGCOLOR, CLOCKSTRIPBGCOLOR, MAPSTRIPBGCOLOR, 
  ZOOMBGCOLOR, OPTIONBGCOLOR, BUTTONCOLOR, STARCOLOR,
  CLOCKFGCOLOR, MAPFGCOLOR, MENUFGCOLOR, 
  BUTTONFGCOLOR1, BUTTONFGCOLOR2, BUTTONFGCOLOR3, BUTTONFGCOLOR4,
  CLOCKSTRIPFGCOLOR, MAPSTRIPFGCOLOR,
  ZOOMFGCOLOR, OPTIONFGCOLOR, WEAKCOLOR, ROOTCOLOR,
  CARETCOLOR, CHANGECOLOR, CHOICECOLOR, DIRCOLOR, IMAGECOLOR, CITYNAMECOLOR, 
  CITYCOLOR0, CITYCOLOR1, CITYCOLOR2, MARKCOLOR1, MARKCOLOR2, 
  SUNCOLOR, MOONCOLOR, LINECOLOR, MERIDIANCOLOR, PARALLELCOLOR, TROPICCOLOR, 
  NUMPIXELS
};

enum {
  CLOCKSTRIPFONT, MAPSTRIPFONT, COORDFONT, CITYFONT, LABELFONT, MENUFONT, NUMFONTS
};

/* Geometry structure */

typedef struct Geometry {
	int	mask;
	int	x;
	int	y;
	unsigned int	width;
	unsigned int	height;
        unsigned int    w_mini;
        unsigned int    h_mini;
} Geometry;

/* Behavioral flags */
typedef struct Flags {
  /* Status values */
        short colorlevel;               /* 0=mono 1=invert1 2=invert2 3=color*/
        short fillmode;                 /* 0=coastlines 1=contour 2=landfill */
        short vmfflags;                 /* flags for VMF format */
        short dotted;                   /* use dotted lines ? */
        short colorscale;               /* number of colors in shading */
        short darkness;                 /* level of darkness in shading */
        short map_mode;                 /* are we in C, D, E, L, S mode? */
        short clock_mode;               /* clock mode */
        short animate;                  /* start/stop animation */
        short animperiod;               /* animation periodicity 0,1..5 sec */
        short progress;                 /* special progress time ?*/
        short shading;                  /* shading mode */
        short dms;                      /* degree, minute, second mode */
        short objectmode;               /* mode for showing Moon/Sun */
        short objects;                  /* are Moon/Sun to be shown ? */
        short citymode;                 /* cities mode */
        short meridian;                 /* meridians mode */
        short parallel;                 /* parallels/tropics mode */
  /* Internal switches */
        short mapped;                   /* is window mapped ? */
        short update;                   /* update image (=-1 full update) */
        short bottom;                   /* bottom strip to be cleaned */
        short hours_shown;              /* hours in extension mode shown? */
} Flags;

/* Zoom settings */

typedef struct ZoomSettings {
        double          fx;             /* zoom factor along width */
        double          fy;             /* zoom factor along height */
        double          fdx;            /* translation factor along width */
        double          fdy;            /* translation factor along height */
        double meridspacing;            /* meridian spacing (degrees) */
        double paralspacing;            /* parallel spacing (degrees) */
        int             mode;           /* zoom behaviour mode=0,1,2 */
        int             width;          /* width of full extent zoomed area */
        int             height;         /* height of full extent zoomed area */
        int             dx;             /* translation along width */
        int             dy;             /* translation along height */
} ZoomSettings;


/* Graphic Data */

typedef struct GraphicData {
        GC              wingc;          /* window gc */
        GC              pixgc;          /* pixmap gc */
        Pixel           pixel[NUMPIXELS];  /* list of pixels used */
        int             precedence;     /* ordinal number of creation */
        int             clockstrip;     /* height of strip for clock */
        int             mapstrip;       /* height of strip for map */
        int             menustrip;      /* height of strip for map */
        int             charspace;      /* menu char spacing */
        short           links;          /* how many other Windows linked ? */
        short           usedcolors;     /* number of colors used */
        Colormap        cmap;           /* window (private?) colormap */  
        XFontStruct *   font[NUMFONTS]; /* font structures */
} GraphicData;

/* Records to hold cities */

typedef struct City {
    char *name;		/* Name of the city */
    double lat, lon;	/* Latitude and longitude of city */
    char *tz;		/* Timezone of city */
    short size;
    struct City *next;	/* Pointer to next record */
} City;

/* Records to hold marks */

typedef struct Mark {
    City *city;
    double save_lat, save_lon;
    int  flags, pulse, full;
    struct tm sr, ss, dl;
} Mark;

/* Records to hold text labels */

typedef struct TextLabel {
    char *text;
    double lat, lon;
    int color;
    int position;
    struct TextLabel *next;
} TextLabel ;

/* Records to hold text entries */

typedef struct TextEntry {
    char * string;
    int maxlength;
    int caret;
    int oldcaret;
    int oldlength;
    char oldchar;
} TextEntry;

/* Sundata structure */
typedef struct Sundata {
        int             wintype;        /* is window map or clock ? */
        Window          win;            /* window id */
        Pixmap          mappix;         /* map pixmap */
        XImage *        xim;            /* ximage of map */ 
        char *          ximdata;        /* ximage data copy */
        int             *spotsizes;     /* city spot sizes, by category */
        int             *sizelimits;    /* city size limits */
        GraphicData *   gdata;          /* associated graphical data */
        Geometry        geom;           /* geometry */
	Geometry        prevgeom;       /* previous geometry */
        ZoomSettings    zoom;           /* Zoom settings of window */
        ZoomSettings    newzoom;        /* New zoom settings */
        ZoomSettings    oldzoom;        /* Old zoom settings */
	Flags		flags;		/* window behavioral settings */
        int             hstrip;         /* height of bottom strip */
        char *          clock_img_file; /* name of clock xpm file */
        char *          map_img_file;   /* name of map xpm file */
	char *		bits;           /* pointer to char * bitmap bits */
        short *         tr1;            /* pointer to day/night transition 1 */
        short *         tr2;            /* pointer to day/night transition 2 */
        int             south;          /* color code (0 / -1) at South pole */
        double *        daywave;        /* pointer to daywave values */
        double *        cosval;         /* pointer to cosine values */
        double *        sinval;         /* pointer to sine values */
        unsigned char * daypixel;       /* pointer to day pixels */
        unsigned char * nightpixel;     /* pointer to night pixels */
        Pixel           *vmfpixels;     /* list of additional vmf pixels */
        int             ncolors;        /* number of colors in day pixels */
	long		time;		/* time - real or fake, see flags */
	long		projtime;	/* last time projected illumination */
	long		roottime;	/* last time written to root */
	long		animtime;	/* last time of animation */
        long            jump;           /* time jump (in sec) */
        double          sundec;         /* Sun declination */
        double          sunlon;         /* Sun longitude */
        double          moondec;        /* Moon declination */
        double          moonlon;        /* Moon longitude */
        double          shadefactor;    /* shading factor */
        double          shadescale;     /* shading rescale factor */
        double          fnoon;          /* position of noon, double float */
	int		noon;		/* position of noon, integer */
        int             local_day;      /* previous local day */
        int             solar_day;      /* previous solar day */
	int		count;	        /* number of time iterations */
        struct City     pos1;           /* first position */
        struct City     pos2;           /* second position */
        struct City     *lastmarked;    /* last marked city */
        struct Mark     mark1;          /* first mark */
        struct Mark     mark2;          /* second mark */
        struct TextLabel *label;        /* label structure if any */
        struct Sundata  *next;          /* pointer to next structure */
} Sundata;

/* setupCallback structure */

typedef void (*setupCB)(int mode);

typedef void (*popCB)(Sundata * Context);

/* Which OS are we using ? */

#if defined(linux) || defined(__linux) || defined(__linux__)
#define _OS_LINUX_
#elif defined(hpux) || defined(__hpux) || defined(__hpux__)
#define _OS_HPUX_
#endif

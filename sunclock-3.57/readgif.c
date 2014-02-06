/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, 1991, 1993 David Koblas.			       | */
/* | Copyright 1996 Torsten Martinsen.				       | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.	       | */
/* +-------------------------------------------------------------------+ */

/* $Id: readGIF.c,v 1.5 1996/06/18 10:04:47 torsten Exp $ */

#include <stdio.h>
#include <string.h>
#include "sunclock.h"

extern Display *	dpy;
extern Visual *		visual;
extern Colormap         tmp_cmap;

extern int		scr;
extern int		bigendian;
extern int              color_depth;
extern int              color_pad;
extern int              bytes_per_pixel;
extern int              color_alloc_failed;
extern int              verbose;

extern char * salloc();
extern void fill_line(char *scan, char* c, int w, int zw, int wp, int dx);

#define	MAXCOLORMAPSIZE		256

#define	TRUE	1
#define	FALSE	0

#define CM_RED		0
#define CM_GREEN	1
#define CM_BLUE		2

#define	MAX_LWZ_BITS		12

#define INTERLACE		0x40
#define LOCALCOLORMAP	0x80
#define BitSet(byte, bit)	(((byte) & (bit)) == (bit))

#define	ReadOK(file,buffer,len)	(fread(buffer, len, 1, file) != 0)

#define LM_to_uint(a,b)			(((b)<<8)|(a))

struct {
    unsigned int Width;
    unsigned int Height;
    unsigned char ColorMap[3][MAXCOLORMAPSIZE];
    unsigned int BitPixel;
    unsigned int ColorResolution;
    unsigned int Background;
    unsigned int AspectRatio;
    int Grayscale;
} GifScreen;

static struct {
    int transparent;
    int delayTime;
    int inputFlag;
    int disposal;
} Gif89 = {
    -1, -1, -1, 0
};

static int ReadColorMap(FILE * fd, int number,
			unsigned char buffer[3][MAXCOLORMAPSIZE], int *flag);
static int DoExtension(FILE * fd, int label);
static int GetDataBlock(FILE * fd, unsigned char *buf);
static int GetCode(FILE * fd, int code_size, int flag);
static int LWZReadByte(FILE * fd, int flag, int input_code_size);
static int ReadImage(FILE * fd, Sundata *Context, 
		        int width, int height, int bitPixel,
			unsigned char cmap[3][MAXCOLORMAPSIZE],
			int gray, int interlace);

int
TestGIF(char *file)
{
    FILE *fd = fopen(file, "r");
    char buf[10];
    int ret = 0;

    if (fd == NULL)
	return 0;

    if (ReadOK(fd, buf, 6)) {
	if ((strncmp(buf, "GIF", 3) == 0) &&
	    ((strcmp(buf + 3, "87a") != 0) ||
	     (strcmp(buf + 3, "89a") != 0)))
	    ret = 1;
    }
    fclose(fd);

    return ret;
}

int
readGIF(path, Context)
char *path;
Sundata * Context;
{
    unsigned char buf[16];
    unsigned char c;
    unsigned char localColorMap[3][MAXCOLORMAPSIZE];
    int output = 0, size;
    int grayScale;
    int useGlobalColormap;
    int bitPixel;
    char version[4];
    FILE *fd = fopen(path, "r");

    if (fd == NULL)
	return 1;

    if (!ReadOK(fd, buf, 6)) {
	fclose(fd);
	return 2;
    }
    if (strncmp((char *) buf, "GIF", 3) != 0) {
	fclose(fd);
	return 3;
    }
    strncpy(version, (char *) buf + 3, 3);
    version[3] = '\0';

    if ((strcmp(version, "87a") != 0) && (strcmp(version, "89a") != 0)) {
	fclose(fd);
	return 3;
    }
    if (!ReadOK(fd, buf, 7)) {
	fclose(fd);
	return 3;
    }
    GifScreen.Width = LM_to_uint(buf[0], buf[1]);
    GifScreen.Height = LM_to_uint(buf[2], buf[3]);
    GifScreen.BitPixel = 2 << (buf[4] & 0x07);
    GifScreen.ColorResolution = (((buf[4] & 0x70) >> 3) + 1);
    GifScreen.Background = buf[5];
    GifScreen.AspectRatio = buf[6];

    if (BitSet(buf[4], LOCALCOLORMAP)) {	/* Global Colormap */
	if (ReadColorMap(fd, GifScreen.BitPixel, GifScreen.ColorMap,
			 &GifScreen.Grayscale)) {
	    fclose(fd);
	    return 2;
	}
    }
 repeat:
    if (!ReadOK(fd, &c, 1)) {
        fclose(fd);
        return 2;
    }
    if (c == '!') {        /* Extension */
        if (!ReadOK(fd, &c, 1)) {
           fclose(fd);
           return 2;
	}
        DoExtension(fd, c);
        goto repeat;
    }
    if (c != ',') {        /* Not a valid start character */
        goto repeat;
    }

    if (!ReadOK(fd, buf, 9)) {
        fclose(fd);
        return 2;
    }
    useGlobalColormap = !BitSet(buf[8], LOCALCOLORMAP);

    bitPixel = 1 << ((buf[8] & 0x07) + 1);

    Context->xim = XCreateImage(dpy, visual, 
              DefaultDepth(dpy, scr), ZPixmap, 0, NULL, 
              Context->geom.width, Context->geom.height, color_pad, 0);
    XFlush(dpy);
    if (!Context->xim)
       return 4;
    bytes_per_pixel = (Context->xim->bits_per_pixel/8);
    size = Context->xim->bytes_per_line * Context->geom.height;
    Context->xim->data = (char *) salloc(size);
    if (!Context->xim->data) {
       XDestroyImage(Context->xim);
       return 4;
    }

    if (!useGlobalColormap) {
        if (ReadColorMap(fd, bitPixel, localColorMap, &grayScale)) {
           XDestroyImage(Context->xim);
           return 2;
        }
        output = ReadImage(fd, Context, 
		  LM_to_uint(buf[4], buf[5]),
                  LM_to_uint(buf[6], buf[7]),
                  bitPixel, localColorMap, grayScale,
                  BitSet(buf[8], INTERLACE));
    } else {
        output = ReadImage(fd, Context, 
		  LM_to_uint(buf[4], buf[5]),
                  LM_to_uint(buf[6], buf[7]),
                  GifScreen.BitPixel, GifScreen.ColorMap,
                  GifScreen.Grayscale, BitSet(buf[8], INTERLACE));
    }

    if (output) XDestroyImage(Context->xim);
    return output;
}

static int
ReadColorMap(FILE *fd, int number, unsigned char buffer[3][MAXCOLORMAPSIZE],
	     int *gray)
{
    int i;
    unsigned char rgb[3];
    int flag;

    flag = TRUE;

    for (i = 0; i < number; ++i) {
	if (!ReadOK(fd, rgb, sizeof(rgb))) {
	    return 1;
	}
	buffer[CM_RED][i] = rgb[0];
	buffer[CM_GREEN][i] = rgb[1];
	buffer[CM_BLUE][i] = rgb[2];
	flag &= (rgb[0] == rgb[1] && rgb[1] == rgb[2]);
    }

#if 0
    if (flag)
	*gray = (number == 2) ? PBM_TYPE : PGM_TYPE;
    else
	*gray = PPM_TYPE;
#else
    *gray = 0;
#endif

    return FALSE;
}

static int
DoExtension(FILE *fd, int label)
{
    static char buf[256];
    char *str;

    switch (label) {
    case 0x01:			/* Plain Text Extension */
	str = "Plain Text Extension";
	break;
    case 0xff:			/* Application Extension */
	str = "Application Extension";
	break;
    case 0xfe:			/* Comment Extension */
	str = "Comment Extension";
	while (GetDataBlock(fd, (unsigned char *) buf) != 0);
	return FALSE;
    case 0xf9:			/* Graphic Control Extension */
	str = "Graphic Control Extension";
	(void) GetDataBlock(fd, (unsigned char *) buf);
	Gif89.disposal = (buf[0] >> 2) & 0x7;
	Gif89.inputFlag = (buf[0] >> 1) & 0x1;
	Gif89.delayTime = LM_to_uint(buf[1], buf[2]);
	if ((buf[0] & 0x1) != 0)
	    Gif89.transparent = buf[3];

	while (GetDataBlock(fd, (unsigned char *) buf) != 0);
	return FALSE;
    default:
	str = buf;
	sprintf(buf, "UNKNOWN (0x%02x)", label);
	break;
    }

    while (GetDataBlock(fd, (unsigned char *) buf) != 0);

    return FALSE;
}

static int ZeroDataBlock = FALSE;

static int
GetDataBlock(FILE *fd, unsigned char *buf)
{
    unsigned char count;

    if (!ReadOK(fd, &count, 1)) {
	/* pm_message("error in getting DataBlock size" ); */
	return -1;
    }
    ZeroDataBlock = count == 0;

    if ((count != 0) && (!ReadOK(fd, buf, count))) {
	/* pm_message("error in reading DataBlock" ); */
	return -1;
    }
    return count;
}

static int
GetCode(FILE *fd, int code_size, int flag)
{
    static unsigned char buf[280];
    static int curbit, lastbit, done, last_byte;
    int i, j, ret;
    unsigned char count;

    if (flag) {
	curbit = 0;
	lastbit = 0;
	done = FALSE;
	return 0;
    }
    if ((curbit + code_size) >= lastbit) {
	if (done) {
	    if (curbit >= lastbit)
		fprintf(stderr, "ran off the end of my bits...\n");
	    return -1;
	}
	buf[0] = buf[last_byte - 2];
	buf[1] = buf[last_byte - 1];

	if ((count = GetDataBlock(fd, &buf[2])) == 0)
	    done = TRUE;

	last_byte = 2 + count;
	curbit = (curbit - lastbit) + 16;
	lastbit = (2 + count) * 8;
    }
    ret = 0;
    for (i = curbit, j = 0; j < code_size; ++i, ++j)
	ret |= ((buf[i / 8] & (1 << (i % 8))) != 0) << j;

    curbit += code_size;

    return ret;
}

static int
LWZReadByte(FILE *fd, int flag, int input_code_size)
{
    static int fresh = FALSE;
    int code, incode;
    static int code_size, set_code_size;
    static int max_code, max_code_size;
    static int firstcode, oldcode;
    static int clear_code, end_code;
    static int table[2][(1 << MAX_LWZ_BITS)];
    static int stack[(1 << (MAX_LWZ_BITS)) * 2], *sp;
    register int i;

    if (flag) {
	set_code_size = input_code_size;
	code_size = set_code_size + 1;
	clear_code = 1 << set_code_size;
	end_code = clear_code + 1;
	max_code_size = 2 * clear_code;
	max_code = clear_code + 2;

	GetCode(fd, 0, TRUE);

	fresh = TRUE;

	for (i = 0; i < clear_code; ++i) {
	    table[0][i] = 0;
	    table[1][i] = i;
	}
	for (; i < (1 << MAX_LWZ_BITS); ++i)
	    table[0][i] = table[1][0] = 0;

	sp = stack;

	return 0;
    } else if (fresh) {
	fresh = FALSE;
	do {
	    firstcode = oldcode = GetCode(fd, code_size, FALSE);
	} while (firstcode == clear_code);
	return firstcode;
    }
    if (sp > stack)
	return *--sp;

    while ((code = GetCode(fd, code_size, FALSE)) >= 0) {
	if (code == clear_code) {
	    for (i = 0; i < clear_code; ++i) {
		table[0][i] = 0;
		table[1][i] = i;
	    }
	    for (; i < (1 << MAX_LWZ_BITS); ++i)
		table[0][i] = table[1][i] = 0;
	    code_size = set_code_size + 1;
	    max_code_size = 2 * clear_code;
	    max_code = clear_code + 2;
	    sp = stack;
	    firstcode = oldcode = GetCode(fd, code_size, FALSE);
	    return firstcode;
	} else if (code == end_code) {
	    int count;
	    unsigned char buf[260];

	    if (ZeroDataBlock)
		return -2;

	    while ((count = GetDataBlock(fd, buf)) > 0);

	    if (count != 0) {
		/*
		 * pm_message("missing EOD in data stream (common occurence)");
		 */
	    }
	    return -2;
	}
	incode = code;

	if (code >= max_code) {
	    *sp++ = firstcode;
	    code = oldcode;
	}
	while (code >= clear_code) {
	    *sp++ = table[1][code];
	    if (code == table[0][code])
		fprintf(stderr, "circular table entry BIG ERROR...\n");
	    code = table[0][code];
	}

	*sp++ = firstcode = table[1][code];

	if ((code = max_code) < (1 << MAX_LWZ_BITS)) {
	    table[0][code] = oldcode;
	    table[1][code] = firstcode;
	    ++max_code;
	    if ((max_code >= max_code_size) &&
		(max_code_size < (1 << MAX_LWZ_BITS))) {
		max_code_size *= 2;
		++code_size;
	    }
	}
	oldcode = incode;

	if (sp > stack)
	    return *--sp;
    }
    return code;
}

static int
ReadImage(FILE * fd, Sundata * Context, int width, int height, int cmapSize,
	  unsigned char cmap[3][MAXCOLORMAPSIZE],
	  int gray, int interlace)
{
    unsigned char c;
    char *out;
    int i, i1, i2, v, ind, y;
    int xpos = 0, ypos = 0, pass = 0;
    int *ys;
    unsigned char *scan;

    /*
    **	Initialize the compression routines
     */
   
    scan = (unsigned char *)salloc(3*width);
    if (!scan) return 4;
   
    ys = (int *)salloc((height+1)*sizeof(int));
    if (!ys) {
	free(scan);
        return 4;
    }
     
    if (!ReadOK(fd, &c, 1)) {
	return 2;
    }
    if (LWZReadByte(fd, TRUE, c) < 0) {
	return 2;
    }

    while ((v = LWZReadByte(fd, FALSE, c)) >= 0) {
        ind = 3*xpos;
        scan[ind] = cmap[CM_RED][v];
        scan[ind+1] = cmap[CM_GREEN][v];
        scan[ind+2] = cmap[CM_BLUE][v];

	++xpos;
	if (xpos == width) {
	    y = ((2*ypos+1) * Context->zoom.height) / (2*height)
                  - Context->zoom.dy;
            ys[ypos] = y;
            if (y>=0 && y<Context->geom.height && ypos<height) { 
               out = Context->xim->data + y*Context->xim->bytes_per_line;
 	       fill_line(scan, out, 
                     Context->geom.width,  Context->zoom.width,
                     width, Context->zoom.dx);    
	    }
	    xpos = 0;
	    if (interlace) {
		switch (pass) {
		case 0:
		case 1:
		    ypos += 8;
		    break;
		case 2:
		    ypos += 4;
		    break;
		case 3:
		    ypos += 2;
		    break;
		}

		if (ypos >= height) {
		    ++pass;
		    switch (pass) {
		    case 1:
			ypos = 4;
			break;
		    case 2:
			ypos = 2;
			break;
		    case 3:
			ypos = 1;
			break;
		    default:
			goto fini;
		    }
		}
	    } else {
		++ypos;
	    }
	}
	if (ypos >= height)
	    break;
    }

  fini:

    if (Context->zoom.height>=height) {
       ys[height] = Context->geom.height;
       for(ypos=0; ypos<height; ++ypos) {
	  if (ys[ypos]>=0 || ys[ypos+1]<Context->geom.height) {
	     if (ys[ypos]<0) {
                out = Context->xim->data + 
                        ys[ypos+1]*Context->xim->bytes_per_line;
                i1 = 0;
                i2 = ys[ypos+1];
	     } else {
                out = Context->xim->data + 
                        ys[ypos]*Context->xim->bytes_per_line;
                i1 = ys[ypos]+1;
                i2 = ys[ypos+1];
                if (i2>Context->geom.height) i2=Context->geom.height;
	     }
             for (i=i1; i<i2; i++) 
                memcpy(Context->xim->data+i*Context->xim->bytes_per_line, out,
                    Context->xim->bytes_per_line);
	  }
       }
    }

    free(scan);
    free(ys);
    return 0;
}

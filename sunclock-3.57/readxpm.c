#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/xpm.h>

#include "sunclock.h"

extern Display *	dpy;
extern Visual * 	visual;
extern Colormap         tmp_cmap;

extern int		scr;
extern int              color_depth;
extern int              color_pad;
extern int              bytes_per_pixel;
extern int              verbose;

extern char *           salloc();

int
readXPM(path, Context)
char * path;
struct Sundata * Context;
{
     XpmAttributes attrib;
     XImage *image = NULL;
     int b, c, i, j, k, ip, jp;

     attrib.colormap = tmp_cmap;
     attrib.valuemask = XpmColormap | XpmReturnPixels;

     if (verbose)
         fprintf(stderr, "Loading XPM image file %s\n", path);
     if (XpmReadFileToImage(dpy, path, &image, NULL, &attrib)) {
         fprintf(stderr, "Cannot read XPM file %s,\nalthough it exists !!\n", 
                         path);
         return 1; 
     }
     if (image)
         bytes_per_pixel = image->bits_per_pixel/8;
     else
         return 4;

     Context->xim = XCreateImage(dpy, visual, 
         DefaultDepth(dpy, scr), ZPixmap,0, NULL, 
         Context->geom.width, Context->geom.height, color_pad,0);
     XFlush(dpy);
     if (!Context->xim) return 4;
     Context->xim->data = (char *) salloc(Context->xim->bytes_per_line 
                                       * Context->xim->height);
     if (!Context->xim->data) return 4;
     Context->ncolors = attrib.npixels;
     if (color_depth<=8) 
	for (j=0; j<attrib.npixels; j++) 
           Context->daypixel[j] = (unsigned char) attrib.pixels[j];
     for (j=0; j<Context->geom.height; j++) {
     jp = ((j+Context->zoom.dy) * image->height)/Context->zoom.height;
     for (i=0; i<Context->geom.width; i++) {
        ip = ((i+Context->zoom.dx) * image->width)/Context->zoom.width;
	b = i*bytes_per_pixel + j*Context->xim->bytes_per_line;
	c = ip*bytes_per_pixel + jp*image->bytes_per_line;
        for (k=0; k<bytes_per_pixel; k++)
            Context->xim->data[b+k] = image->data[c+k];
	}
     }
     XDestroyImage(image);
     return 0;
}




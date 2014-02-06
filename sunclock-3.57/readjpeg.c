/* +-------------------------------------------------------------------+ */
/* | This file is derived from                                         | */
/* | Xpaint's readJPEG routines, copyrighted                           | */
/* | by David Koblas (koblas@netcom.com)	        	       | */
/* |								       | */
/* | Permission to use, copy, modify, and to distribute this software  | */
/* | and its documentation for any purpose is hereby granted without   | */
/* | fee, provided that the above copyright notice appear in all       | */
/* | copies and that both that copyright notice and this permission    | */
/* | notice appear in supporting documentation.	 There is no	       | */
/* | representations about the suitability of this software for	       | */
/* | any purpose.  this software is provided "as is" without express   | */
/* | or implied warranty.					       | */
/* |								       | */
/* +-------------------------------------------------------------------+ */

#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>
#include <jpeglib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

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
#define RANGE 252
extern long lr[RANGE], lg[RANGE], lb[RANGE], lnum[RANGE];

extern char * salloc();
extern void fill_line(char *scan, char* c, int w, int zw, int wp, int dx);

struct error_mgr {
  struct jpeg_error_mgr pub;    /* "public" fields */

  jmp_buf setjmp_buffer;        /* for return to caller */
};

typedef struct error_mgr * error_ptr;

void
error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  error_ptr err = (error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(err->setjmp_buffer, 1);
}

int
readJPEG(path, Context)
char *path;
Sundata * Context;
{
    struct jpeg_decompress_struct cinfo;
    struct error_mgr jerr;
    FILE *input_file;
    double ratio;
    int i, k, l, m, prev, next, size;
    JSAMPROW scanline[1];
    char *scan, *c;
    char pix[RANGE];
    XColor xc;

    if ((input_file = fopen(path, "r")) == NULL) return 1;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
	/* If we get here, the JPEG code has signaled an error.
	 * We need to clean up the JPEG object, close the input file,
	 * and return.
	 */
        jpeg_destroy_decompress(&cinfo);
	fclose(input_file);
        Context->xim = 0;
	return 2;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, input_file);
    jpeg_read_header(&cinfo, TRUE);

    if (cinfo.jpeg_color_space == JCS_GRAYSCALE) return 3;
   
    ratio = ((double) cinfo.image_width/(double) Context->zoom.width + 
             (double) cinfo.image_height/(double) Context->zoom.height )/1.8;
    if (ratio>=8.0) 
      cinfo.scale_denom = 8;
    else
    if (ratio>=4.0) 
      cinfo.scale_denom = 4;
    else
    if (ratio>=2.0) 
      cinfo.scale_denom = 2;
    else
      cinfo.scale_denom = 1;

    jpeg_start_decompress(&cinfo);

    Context->xim = XCreateImage(dpy, visual, 
              DefaultDepth(dpy, scr), ZPixmap, 0, NULL, 
              Context->geom.width, Context->geom.height, color_pad, 0);
    XFlush(dpy);
    if (!Context->xim) return 4;

    bytes_per_pixel = (Context->xim->bits_per_pixel/8);
    size = Context->xim->bytes_per_line * Context->geom.height;
    Context->xim->data = (char *) salloc(size);
    scan = (char *) salloc(3 * cinfo.output_width * sizeof(char));

    if (verbose)
       fprintf(stderr, "Loading %s\n"
	    "Rescaling JPEG data by 1/%d,  "
            "%d %d  -->  %d %d,  %d bytes per pixel\n",
	    path, cinfo.scale_denom,
            cinfo.image_width, cinfo.image_height, 
            Context->geom.width, Context->geom.height, 
            bytes_per_pixel);

    prev = -1;
    scanline[0] = (JSAMPROW) scan;

    if (color_depth<=8) 
      for (l=0; l<RANGE; l++) {
        lr[l] = lg[l] = lb[l] = lnum[l] = 0;
      }

    while (cinfo.output_scanline < cinfo.output_height) {
      (void) jpeg_read_scanlines(&cinfo, scanline, (JDIMENSION) 1);
      next = ((2*cinfo.output_scanline - 1) * Context->zoom.height)/
                  (2*(int)cinfo.output_height) - Context->zoom.dy;
      if (next>=0) {
	if (next>=Context->geom.height) {
	   next = Context->geom.height - 1;
	   /* get loop to stop at next iteration ! */
	   cinfo.output_scanline = cinfo.output_height;
	}
	for (l = prev+1; l<= next; l++) {
	  c = Context->xim->data + l * Context->xim->bytes_per_line;
          fill_line(scan, c, Context->geom.width,  Context->zoom.width,
                    cinfo.output_width, Context->zoom.dx);
	}
        prev = next;
      }
    }

    free(scan);
    jpeg_destroy_decompress(&cinfo);

    fclose(input_file);

    if (jerr.pub.num_warnings > 0) {	
	longjmp(jerr.setjmp_buffer, 1);
    }

    if (color_depth<=8) {
      xc.flags = DoRed | DoGreen | DoBlue;
      k = 0;
      for (m=0; m<RANGE; m++) if (lnum[m]) {
        xc.red = (lr[m]/lnum[m])*257;
        xc.green = (lg[m]/lnum[m])*257;
        xc.blue = (lb[m]/lnum[m])*257;
	if (!XAllocColor(dpy, tmp_cmap, &xc)) 
           color_alloc_failed = 1;
	pix[m] = (char) xc.pixel;
	Context->daypixel[k] = (unsigned char) xc.pixel;
	++k;
      }
      Context->ncolors = k;
      for (i=0; i<size; i++) 
	 Context->xim->data[i] = pix[(unsigned char)Context->xim->data[i]];
    }

    return 0;
}

int
testJPEG(char *file)
{
    unsigned char buf[2];
    FILE *fd = fopen(file, "r");
    int ret = 0;

    if (fd == NULL)
	return 0;

    if (2 == fread(buf, sizeof(char), 2, fd)) {
	if (buf[0] == 0xff && buf[1] == 0xd8)
	    ret = 1;
    }
    fclose(fd);

    return ret;
}

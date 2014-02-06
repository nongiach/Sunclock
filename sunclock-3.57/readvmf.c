/*
 * readvmf.c
 * Jean-Pierre Demailly
 * July 2001
 *
 * Copyright (C) 2001 by Jean-Pierre Demailly <demailly@ujf-grenoble.fr>
 *
 * Permission to use, copy, modify and freely distribute these data for
 * non-commercial and not-for-profit purposes is hereby granted
 * without fee, provided that both the above copyright notice and this
 * permission notice appear in all copies and in supporting
 * documentation.
 *
 * The author makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT
 * OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#ifdef ZLIB
#include <zlib.h>
#include <errno.h>
#endif

#include "sunclock.h"

#define LINELENGTH 256

extern char *ProgName;
extern char *vmfcolors;
extern char *vmfrange;
extern char *vmfcoordformat;

extern Display *	dpy;
extern Colormap         tmp_cmap;

extern int		scr;
extern int		bigendian;
extern int              color_depth;
extern int              color_pad;
extern int              bytes_per_pixel;
extern int              color_alloc_failed;
extern int              verbose;
extern int              reformat;
extern int		min_zoomwidth;
extern int		min_zoomheight;

static struct Sundata *map;
#define pixels map->vmfpixels   /* Pointer to list of colors */

static int mapwidth;
static int mapheight;
static int fillmode;

static char *buffer;  /* Buffer to hold input lines */
static int *grid;     /* Pointer to grid data */
static int *palette;  /* Pointer to list of color codes */

static int uu, cc, vv, vv1, vv2, full;
static int num_palette;
static int default_color;

static double txmin=1E27, txmax=-1E27, tymin=1E27, tymax=-1E27;

char *
salloc(nbytes)
register int			nbytes;
{
	register char *		p;

	p = malloc((unsigned)nbytes);
	if (p == (char *)NULL) {
		fprintf(stderr, "%s: %d bytes required; out of memory\n", 
                      ProgName, nbytes);
		exit(1);
	}
	return (p);
}

void
plotdata(u, v, s)
int u, v, s;
{
  int v1=0, v2=0, c, w, ind;

  c = u;
  if (c<0) c+=map->zoom.width;
  if (c>=map->zoom.width) c-=map->zoom.width;
  c -= map->zoom.dx-1;
  v -= map->zoom.dy;

  if (fillmode==0) {
     if (c>=0 && c<=mapwidth && v>=0 && v<mapheight) {
        ind = c*mapheight+v;
        if (s>0) grid[ind] = s * 65536;
        if (s<0) grid[ind] = -s * 65536;
     }
     return;
  }

  if (c>=0 && c<=mapwidth) {
     ind = c*mapheight;

     if (v>=0 && v<mapheight) {
        if (s>0) grid[ind+v] |= s;
        if (s<0) grid[ind+v] |= -s;
     }

     s *= 65536;
      
     if (v<0) v1=0; else v1=v+1;
     if (v>=mapheight) v2=mapheight-1; else v2=v;

     if (u==uu) {
        if (v>vv)
           for (w=vv1; w<=v2; w++) grid[ind+w] += s;
        if (v<vv)
           for (w=v1; w<=vv2; w++) grid[ind+w] -= s;
     }
     if (u>uu)
        for (w=0; w<=v2; w++) grid[ind+w] += s;
  }

  if (u<uu && cc>=0 && cc<=mapwidth) {
     ind = cc*mapheight;
     for (w=0; w<=vv2; w++) grid[ind+w] -= s;
  }

  uu = u;
  cc = c;
  vv = v;
  vv1 = v1;
  vv2 = v2;
}

int check(i, j, which)
int i, j, which;
{
#define MASK -65536
   int ind, indp, inds;

   which *= 65536;

   ind = i*mapheight+j;
   if (j>0 && (grid[ind-1]&MASK)!=which) return 1;
   if (j<mapheight-1 && (grid[ind+1]&MASK)!=which) return 1;

   indp = (i-1)*mapheight+j;
   if (i>0 && (grid[indp]&MASK)!=which) return 1;

   inds = (i+1)*mapheight+j;
   if (i<mapwidth && (grid[inds]&MASK)!=which) return 1;

   if (full) {
     if (j>0) {
        if (i>0 && (grid[indp-1]&MASK)!=which) return 1;
        if (i<mapwidth && (grid[inds-1]&MASK)!=which) return 1;
     }
     if (j<mapheight-1) {
        if (i>0 && (grid[indp+1]&MASK)!=which) return 1;
        if (i<mapwidth && (grid[inds+1]&MASK)!=which) return 1;
     }
   }
   return 0;
}

void
filterdata()
{
int i, j, t, ind;

    if (fillmode==0) return;

    if (fillmode==1) {

      for (j=0; j<mapheight; j++)
        for (i=0; i<=mapwidth; i++) {
            ind = i*mapheight+j;
	    t = grid[ind];
	    full = 1;
	    if ( (t & 16383) && !check(i,j,0) ) {
	       grid[ind] |= 16384;
	       continue;
	    }
	    full = 0;
	    t = t/65536;
	    if ((t>0) && !check(i,j,t)) grid[ind] |= 32768; 
        }

      for (j=0; j<mapheight; j++)
        for (i=0; i<=mapwidth; i++) {
            ind = i*mapheight+j;
	    t = grid[ind];
	    if (t&16384) 
	      grid[ind] = (t&16383) * 65536;
	    else
	    if (t&32768) 
              grid[ind] = 0;
	    else
	      grid[ind] = t&MASK;
        }
      goto correct;
    }

    /* only remains fillmode = 2 */
    full = 1;

    for (j=0; j<mapheight; j++)
      for (i=0; i<=mapwidth; i++)
	  if (check(i,j,0)) grid[i*mapheight+j] &= MASK;

    for (j=0; j<mapheight; j++)
      for (i=0; i<=mapwidth; i++) {
          ind = i*mapheight+j;
	  t = (grid[ind] & 32767) * 65536;
	  grid[ind] |= t;
      }

 correct:
    if (map->zoom.width > map->geom.width && map->zoom.dx == 0) {
      for (j=0; j<mapheight; j++) grid[j+mapheight] = grid[j+2*mapheight];
    }
}

char *
blacknwhite_image()
{
  int i, j, k, l, u;
  char *bits;

  bits = (char *)
        salloc(((mapwidth+7)/8)*mapheight*sizeof(char));

  k = 0;
  for (j=0; j<mapheight; j++) {
    l = 1;
    u = 0;
    for (i=0; i<mapwidth; i++) {
      if (grid[(i+1)*mapheight+j]<65536) u = u+l;
      l = l+l;
      if (l==256 || i==mapwidth-1) {
        bits[k] = u;
	u = 0;
	l = 1;
        ++k;
      }
    }
  }
  return bits;
}

XImage *
pixmap_image()
{
  int i, j, k, l, w, h;
  char *c;
  XImage *image;
  Visual visual;

  visual = *DefaultVisual(dpy, scr);

  w = mapwidth;
  h = mapheight;

  image = XCreateImage(dpy,&visual, 
      DefaultDepth(dpy, scr), ZPixmap, 0, NULL, w, h, color_pad, 0);
  if (!image) return NULL;

  bytes_per_pixel = (image->bits_per_pixel/8);
  image->data = (char *) salloc(image->bytes_per_line * h);

  if (color_depth>16)
  for (j=0; j<mapheight; j++) {
    c = image->data + j * image->bytes_per_line;
    for (i=0; i<mapwidth; i++) {
       k = bytes_per_pixel * i;
       l = grid[(i+1)*mapheight+j]/65536;
       if (l>=0 && l<num_palette) 
           l = palette[l];
       else
	   l = default_color;
       if (bigendian) {
          c[k+1] = (pixels[l] >> 16) & 255;
          c[k+2] = (pixels[l] >> 8) & 255;
          c[k+3] = pixels[l];
       } else {
          c[k] = pixels[l];
          c[k+1] = (pixels[l] >> 8) & 255;
          c[k+2] = (pixels[l] >> 16) & 255;
       }
    }
  }
  else
  if (color_depth>8)
  for (j=0; j<mapheight; j++) {
    c = image->data + j * mapwidth * bytes_per_pixel;
    for (i=0; i<mapwidth; i++) {
       k = bytes_per_pixel * i;
       l = grid[(i+1)*mapheight+j]/65536;
       if (l>=0 && l<num_palette) 
           l = palette[l];
       else
	 l = default_color;
       if (bigendian) {
          c[k] = pixels[l] / 256;
          c[k+1] = pixels[l] & 255;
       } else {
          c[k] = pixels[l] & 255;
          c[k+1] = pixels[l] / 256;
       }
    }
  }
  else
  for (j=0; j<mapheight; j++) {
    c = image->data + j * mapwidth * bytes_per_pixel;
    for (i=0; i<mapwidth; i++) {
       k = bytes_per_pixel * i;
       l = grid[(i+1)*mapheight+j]/65536;
       if (l>=0 && l<num_palette) 
           l = palette[l];
       else
	   l = default_color;
       c[k] = pixels[l] & 255;
    }
  }

  return image;
}

char *
getdata(fd)
#ifdef ZLIB
gzFile * fd;
#else
FILE *fd;
#endif
{
  int i, j;
  char c;

 repeat:
#ifdef ZLIB
  i = gzgetc(fd);
#else
  i = fgetc(fd);
#endif
  if (i==EOF) return NULL;

  c = (char) i;

  if (c=='%') {
     if (reformat) printf("%c", i);
     while (i!=EOF && (char) i!= '\n') {
#ifdef ZLIB
        i = gzgetc(fd);
#else
        i = fgetc(fd);
#endif
	if (reformat) printf("%c", i);
     }
     if (i==EOF) return NULL;
     goto repeat;
  }

  while(isspace(c)) goto repeat;
  
  j = 0;
  while(!isspace(c) && j<LINELENGTH) {
    buffer[j] = c;
    ++j;
#ifdef ZLIB
    i = gzgetc(fd);
#else
    i = fgetc(fd);
#endif
    if (i==EOF) break;
    c = (char) i;
  }

  buffer[j] = '\0';
  return buffer;
}

Pixel
getPixel(cmap, str)
Colormap cmap;
char *str;
{
Status s;
XColor c, e;

      if (!str) {
	 color_alloc_failed += 2;;
	 return 0;
      }

      s = XAllocNamedColor(dpy, cmap, str, &c, &e);
      if (s != (Status)1) {
	 fprintf(stderr, "%s: warning: can't allocate color `%s'\n",
		 ProgName, buffer);
	 color_alloc_failed += 2;
	 return 0;
      }
      return c.pixel;
}

int
readVMF(path, Context)
char * path;
struct Sundata * Context;
{
  char coordformat[32] = "%7.3f %8.3f";
  int num_colors, correct, maxgrid, run_flag = -1, opencurves = 0, 
      ret_value = -1;
  int color=0, i, j, k, l, num=0, count=0, u=0, v=0, up, vp, flag;
  int position=0;
  int m, min, max, addumin, addvmin, addumax, addvmax, diffu, diffv, sum;
  double fx=0.0, fy=0.0, fx0=0.0, fy0=0.0;
  double fxmin=-180.0, fxmax=180.0, fymin=-90.0, fymax=90.0, 
         fdx=360.0, fdy=180.0;
  double rxmin=-180.0, rxmax, rymin=-90.0, rymax;
  double cx=1.0, cy=1.0, cdx=0.0, cdy=0.0;
  double theta, phi;
  char *str, *ptr;
#ifdef ZLIB
  gzFile * fd;
  int plen;
  char *zpath;
#else
  FILE *fd;
#endif

  fd = NULL;
  buffer = NULL;
  palette = NULL;
  grid = NULL;
  min_zoomwidth = 0;
  min_zoomheight = 0;   

  if (!path) {
     ret_value = 1;
     goto abort;
  }

  map = Context;
  fillmode = Context->flags.fillmode;
  mapwidth = Context->geom.width;
  if (mapwidth <=10) return 4;
  mapheight = Context->geom.height;
  if (mapheight<= 5) return 4;

  if (verbose)
    fprintf(stderr, "Loading vector map %s\n", path);
#ifdef ZLIB
  fd = gzopen(path, "r");
  #define	Z_PATH_EXT	".gz"
  #define	Z_PATH_LEN	( sizeof( Z_PATH_EXT)- 1)
  if ( fd == NULL && errno == ENOENT
       && ( ( plen= strlen( path)) < Z_PATH_LEN
            || strcmp( &path[ plen- Z_PATH_LEN], Z_PATH_EXT) != 0))
    {	/* try to open gzip'd file... (ThMO) */
  #if	__GNUC__ >= 2
      zpath= alloca( plen+ Z_PATH_LEN+ 1);
  #else
      zpath= salloc( plen+ Z_PATH_LEN+ 1);
  #endif
      memcpy( zpath, path, plen);
      memcpy( zpath+ plen, Z_PATH_EXT, Z_PATH_LEN+ 1);
      if ( verbose)
        fprintf( stderr, "Try loading vector map %s\n", zpath);
      fd= gzopen( zpath, "r");
  #if	!( __GNUC__ >= 2)
      free( zpath);
  #endif
    }
  #undef	Z_PATH_EXT
  #undef	Z_PATH_LEN
#else
  fd = fopen(path, "r");
#endif
  if (fd == NULL) {
     ret_value = 1;
     goto abort;
  }

  buffer = (char *) salloc(LINELENGTH+2);
  if (!buffer) goto abort;

#ifdef ZLIB
  str = gzgets(fd, buffer, LINELENGTH);
#else
  str = fgets(buffer, LINELENGTH, fd);
#endif
  if (!str || strncmp(buffer, "%!VMF", 5)) {
     ret_value = 5;
     goto abort;
  }
  if (reformat) printf(str);

  k = 0;
  num_colors = 0;
  while (1) {
    if (map->flags.colorlevel==FULLCOLORS) {
       count = color_alloc_failed;
       if (vmfcolors) {
          if (vmfcolors[k]) {
	     strcpy(buffer, vmfcolors+k);
             str = buffer;
	     l = 0;
             while (str[l]!='|' && str[l] != '\0') ++l;
	     if (str[l]=='|') 
	        k=k+l+1;
	     else
	        k=k+l;
	     str[l] = '\0';
             if (*str) {
	        ++num_colors;
                pixels = 
                  (Pixel *)realloc((void *)pixels, num_colors*sizeof(Pixel));
	        if (!pixels) goto abort;
                pixels[num_colors-1] = getPixel(tmp_cmap, str);
	     } else
	        --count;
          } else
	     --count;
       }
       str = getdata(fd);
       if (!strcmp(str, ";")) break;
       if (!vmfcolors || color_alloc_failed>count) {
	  count = color_alloc_failed;
	  ++num_colors;
          pixels = (Pixel *)realloc((void *)pixels, num_colors*sizeof(Pixel));
	  if (!pixels) goto abort;
	  pixels[num_colors-1] = getPixel(tmp_cmap, str);
       }
       if (color_alloc_failed>count) {
	  color_alloc_failed = 1;
	  ret_value = 6;
	  goto abort;
       } else
	  color_alloc_failed &= 1;
       if (color_depth<=8)
          Context->daypixel[num_colors-1] = 
             (unsigned char) pixels[num_colors-1];
    } else {
       str = getdata(fd);
       ++num_colors;
       if (!strcmp(str, ";")) break;
    }
    if (reformat) printf("%s\t", str);
  }
  if (reformat) printf(";\n\n");
  Context->ncolors = num_colors;

  str = getdata(fd);
  if (!str) goto abort;
  default_color = atoi(str);
  if (default_color<0 || default_color>=num_colors) goto abort;
  if (reformat) printf("%d\n\n", default_color);

  j = 0;
  num_palette = 0;
  while (1) {
    str = getdata(fd);
    if (!str) goto abort;
    if (*str==';') {
       if (reformat) printf("\n;\n\n");
       break;
    }
    if (*str=='c') {
       v = atoi(str+1);
       if (v<0 || v>=num_colors) goto abort;
       if (reformat) {
	  if (j) printf("\n");
	  printf("c%d", v);
       }
    }
    else {
       u = atoi(str);
       if (u<0) goto abort;
       if (u>=num_palette) {
	  palette = (int *) realloc(palette, (u+1)*sizeof(int));
	  if (!palette) goto abort;
	  for (k=num_palette; k<u; ++k) palette[k] = default_color;
	  num_palette = u+1;
       }
       palette[u] = v;
       if (reformat) printf(" %d", u);
    }
    ++j;
  }

  str = getdata(fd);
  if (!str) goto abort;
  if (fillmode)
     correct = atoi(str);
  else
     correct = 0;
  if (reformat) printf("%d\n\n", correct);
  correct *= 65536;

  maxgrid = (mapwidth+1)*mapheight;
  grid = (int *) salloc(maxgrid*sizeof(int));
  for (i=0; i<maxgrid; i++) grid[i] = correct;

  count = 0;

  if (reformat) {
     if (vmfrange) {
        if (vmfcoordformat)
	   strncpy(coordformat, vmfcoordformat, 30);
	else
           strcpy(coordformat, "%g %g");
        for (i=0; i<strlen(vmfrange); i++) 
            if (vmfrange[i]=='|') vmfrange[i] = ' ';
        if (sscanf(vmfrange, "%lg %lg %lg %lg", 
            &rymin, &rymax, &rxmin, &rxmax)<4)
            goto abort;
     } else {
        rxmin = fxmin;
        rxmax = fxmax;
        rymin = fymin;
        rymax = fymax;
     }
     cx =  (rxmax-rxmin)/fdx;
     cy =  (rymax-rymin)/fdy;
     cdx = -fxmin*cx+rxmin;
     cdy = -fymin*cy+rymin;
  }

 iter:
  str = getdata(fd);
  if (!str) goto abort;
  if (*str >= 'A') {
     if (!strcmp(str, "end")) {
        if (reformat) 
           printf("\n%s\n", str);
        goto endcurves;
     } else
     if (!strcmp(str, "flag")) {
        str = getdata(fd);
        if (!str) goto abort;
	run_flag = atoi(str);
        if (reformat) 
           printf("flag %d\n", run_flag);
     } else
     if (!strcmp(str, "opencurves")) {
        if (reformat) 
           printf("%s\n", str);
        opencurves = 1;
     } else
     if (!strcmp(str, "closedcurves")) {
        if (reformat) 
           printf("%s\n", str);
        opencurves = 0;
     } else
     if (!strcmp(str, "zoomwidth")) {
        str = getdata(fd);
	min_zoomwidth = atoi(str);
     } else
     if (!strcmp(str, "zoomheight")) {
        str = getdata(fd);
	min_zoomheight = atoi(str);
     } else
     if (!strcmp(str, "fillmode")) {
        str = getdata(fd);
        if (!str) goto abort;
	i = atoi(str);
        if (reformat) 
           printf("fillmode %d\n", i);	
	if (i>map->flags.fillmode) 
           i = map->flags.fillmode;
	if (i<fillmode) filterdata();
	fillmode = i;
     } else
     if (!strcmp(str, "range")) {
        str = getdata(fd);
        if (!str) goto abort;
        fymin = atof(str);
        str = getdata(fd);
        if (!str) goto abort;
        fymax = atof(str);
        str = getdata(fd);
        if (!str) goto abort;
        fxmin = atof(str);
        str = getdata(fd);
        if (!str) goto abort;
        fxmax = atof(str);
        fdx = fxmax - fxmin;
        fdy = fymax - fymin;
	if (reformat) {
	   if (!vmfrange){
	      rxmin = fxmin;
	      rxmax = fxmax;
	      rymin = fymin;
	      rymax = fymax;
	   }
	   cx = (rxmax-rxmin)/fdx;
	   cy = (rymax-rymin)/fdy;
           cdx = -fxmin*cx+rxmin;
           cdy = -fymin*cy+rymin;
           printf("range %g %g %g %g\n\n", rymin, rymax, rxmin, rxmax);
        }
     } else
     if (!strcmp(str, "label")) {
        /* get rid of count number */
        str = getdata(fd);
        str = getdata(fd);
        if (!str) goto abort;
        color = atoi(str);
        str = getdata(fd);
        if (!str) goto abort;
        position = atoi(str);
        str = getdata(fd);
        if (!str) goto abort;
        fy = atof(str);
        str = getdata(fd);
        if (!str) goto abort;
        fx = atof(str);
        if (reformat) {
           printf("\nlabel %d %d %d ", count, color, position);
           printf(coordformat, fy, fx);
           printf("\n");
        }
        ++count;
        ptr = NULL;
        l = 0;
#ifdef ZLIB
        while (!gzeof(fd)) {
           str = gzgets(fd, buffer, LINELENGTH);
#else
        while (!feof(fd)) {
           str = fgets(buffer, LINELENGTH, fd);
#endif
	   if (!str) break;
           if (reformat) {
	      printf("%s", str);
	   }
	   if (*str == ';') break;
           if (*str == ' ') ++str; /* remove any initial blank space */
           k = strlen(str);
   	   if (k>=2 && str[k-1]=='\n' && str[k-2]=='\\' &&
               (k==2 || (k>=3 && str[k-3]!='\\'))) {
	      k -=2;
              str[k] = '\0';
	   }
           ptr = realloc(ptr, l+k+2);
           strcpy(ptr + l, str);
           l += k;
	}
	if (ptr && *ptr) {
	   l -= 1;
	   if (l>=0 && ptr[l]=='\n') ptr[l] = '\0';
           if (Context->zoom.width<min_zoomwidth) goto iter;
           if (Context->zoom.height<min_zoomheight) goto iter;
           str = (char *)Context->label;
           Context->label = (struct TextLabel *)salloc(sizeof(TextLabel));
           Context->label->text = ptr;
           Context->label->lon = fx;
           Context->label->lat = fy;
           Context->label->color = color;
           Context->label->position = position;
           Context->label->next = (struct TextLabel *)str;
	}
     }
  goto iter;
  } else
  if (*str == '#') {
     num = 0;
     str = getdata(fd);
     if (!str) goto abort;
     color = atoi(str);
     if (reformat)
        printf("\n#%d %d", count, color);
     ++count;
  } else 
  if (*str == ';') {
     fx = fx0;
     fy = fy0;
     if (reformat) printf("\n;\n");
     if (opencurves) goto iter;
  } else {
     fy = atof(str);
     str = getdata(fd);
     if (!str) goto abort;
     fx = atof(str);
     if (verbose) {
        if (fx<txmin) txmin = fx;
        if (fx>txmax) txmax = fx;
        if (fy<tymin) tymin = fy;
        if (fy>tymax) tymax = fy;
     }
     if (num == 0) {
	fx0 = fx;
	fy0 = fy;
     }
     if (reformat) {
        if (num%4 == 0) printf("\n"); else printf("  ");
        printf(coordformat, fy*cy+cdy, fx*cx+cdx);
     }
     ++num;
  }

  flag = run_flag & map->flags.vmfflags;
  if (!flag) goto iter;
  if ((flag&1) && !map->wintype) goto iter;
  if (map->zoom.width<min_zoomwidth) goto iter;

  theta = (fx - fxmin) / fdx;
  phi = (fymax - fy) / fdy;
  cc = uu = up = u;
  if (cc<0) cc+=map->zoom.width;
  if (cc>=map->zoom.width) cc-=map->zoom.width;      
  cc -= map->zoom.dx-1;

  vp = v;
  vv = v - map->zoom.dy;
  vv1 = vv + 1;
  if (vv1<0) vv1 = 0;
  vv2 = vv;
  if (vv2>=mapheight) vv2 = mapheight-1;

  u = (int) (theta * (double) map->zoom.width);
  v = (int) (phi * (double) map->zoom.height);
  if (num>=2) {
     diffu = abs(u-up);
     if (diffu>map->zoom.width/2) {
        if (u>up) 
           u -= map->zoom.width;
	else
	   u += map->zoom.width;
        diffu = abs(u-up);
     }
     diffv = abs(v-vp);
     addumin = (u>up)? 1:-1;
     addvmin = (v>vp)? 1:-1;
     if (diffu>diffv) {
	max = diffu ; 
        min = diffv;
        addumax = addumin;
	addvmax = 0;
     } else {
        max = diffv ; 
        min = diffu;
	addumax = 0;
	addvmax = addvmin;
     }
     sum = max/2-max;
     for (m=0; m<max; m++) {
        sum = sum + min;
        if (sum>=max/2) {
	   sum -= max;
	   up += addumin;
           vp += addvmin;
        } else {
	   up += addumax; 
           vp += addvmax;
	}
        plotdata(up, vp, color);
     }
  }
  goto iter;

 endcurves:
  filterdata();

  if (map->flags.colorlevel==FULLCOLORS)
     Context->xim = pixmap_image();
  else
     Context->bits = blacknwhite_image();
  if ((map->flags.colorlevel<FULLCOLORS && Context->bits==NULL) || 
      (map->flags.colorlevel==FULLCOLORS && Context->xim==NULL)) 
     ret_value = 2;
  if (ret_value < 0) ret_value = 0;

 abort:
  if (ret_value < 0) ret_value = 2;
#ifdef ZLIB
  if (fd) gzclose(fd);
#else
  if (fd) fclose(fd);
#endif
  if (buffer) free(buffer); 
  if (grid) free(grid); 
  if (palette) free(palette);
  if (verbose)
     fprintf(stderr, "ymin = %9.3f ymax = %9.3f   xmin = %9.3f xmax = %9.3f\n", tymin, tymax, txmin, txmax);
  return ret_value;
}

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <string.h>

char buf[128];

char *
getdata(fd)
FILE * fd;
{
  int i, j;
  char c;

 repeat:
  i = fgetc(fd);
  if (i==EOF) return NULL;

  c = (char) i;

  if (c=='%') {
     while (i!=EOF && (char) i!= '\n') {
        i = fgetc(fd);
     }
     if (i==EOF) return NULL;
     goto repeat;
  }

  while(isspace(c)) goto repeat;
  
  j = 0;
  while(!isspace(c) && j<125) {
    buf[j] = c;
    ++j;
    i = fgetc(fd);
    if (i==EOF) break;
    c = (char) i;
  }

  buf[j] = '\0';
  return buf;
}

static double **x = NULL;
static double **y = NULL;
static int *col, *len;

int
main(argc, argv)
int             argc;
char **         argv;
{
FILE * fd;
char *str;
int start=0, num, j, l, j1, j2;

void
reverse(int j)
{
int p;
double z;

  for (p=0; p+p+1<len[j]; p++) {
    z = x[j][p];
    x[j][p] = x[j][len[j]-p-1];
    x[j][len[j]-p-1] = z;
    z = y[j][p];
    y[j][p] = y[j][len[j]-p-1];
    y[j][len[j]-p-1] = z;
  } 
}

void
concat(int j1, int j2)
{
int l, p, t;
  fprintf(stderr, "%d ", j2); 
  l = len[j1]+len[j2]-1; 
  x[j1] = (double *)realloc(x[j1], l*sizeof(double));
  y[j1] = (double *)realloc(y[j1], l*sizeof(double));
  for (p=1; p<len[j2]; p++) {
    t = len[j1]+p-1;
    x[j1][t] = x[j2][p];
    y[j1][t] = y[j2][p];
  }
  free(x[j2]);
  free(y[j2]);
  len[j1] = l; 
  --num;
  for (p=j2; p<num; p++) {
    x[p] = x[p+1];
    y[p] = y[p+1];
    col[p] = col[p+1];
    len[p] = len[p+1];
  }
}

    if (argc<2) {
       fprintf(stderr, "No filename specified !!\n");
       fprintf(stderr, "Usage:  %s file1 > file2 .\n", argv[0]);
       exit(1);
    }
    fd = fopen(argv[1], "r");
    if (!fd) {
       fprintf(stderr, "File %s does not exist !!\n", argv[1]);
       exit(1);
    }
    num = 0;
    while ((str=getdata(fd))) {
       if (*str=='#') {
	 start = 1;
	 ++num;
         j = num-1;
	 x = (double **)realloc(x, num*sizeof(double *));
	 y = (double **)realloc(y, num*sizeof(double *));
	 col = (int *)realloc(col, num*sizeof(int));
	 len = (int *)realloc(len, num*sizeof(int));
	 x[j] = NULL;
	 y[j] = NULL;
	 col[j] = atoi(getdata(fd));
	 len[j] = l = 0;
	 continue;
       } else
       if (*str==';') {
	 start = 0;
	 continue;
       } else {
	 ++l;
         x[j] = (double *)realloc(x[j], l*sizeof(double)); 
         y[j] = (double *)realloc(y[j], l*sizeof(double)); 
	 x[j][l-1] = atof(str);
	 y[j][l-1] = atof(getdata(fd));
	 len[j] = l;
       }
    }
    
    j1=0;
    j2=1;
  
    while(j1<num) {
      fprintf(stderr, "\nDoing %d... ", j1);
      while(j2<num) {
	if (x[j2][0]==x[j1][0] && y[j2][0]==y[j1][0]) {
           reverse(j1);
	   concat(j1,j2);
	   j2=j1+1;
	} else
	if (x[j2][0]==x[j1][len[j1]-1] && y[j2][0]==y[j1][len[j1]-1]) {
	   concat(j1,j2);
	   j2=j1+1;
	} else
	if (x[j2][len[j2]-1]==x[j1][0] && 
            y[j2][len[j2]-1]==y[j1][0]) {	
	   reverse(j1);
	   reverse(j2);
	   concat(j1,j2);
	   j2=j1+1;
	} else
	if (x[j2][len[j2]-1]==x[j1][len[j1]-1] && 
            y[j2][len[j2]-1]==y[j1][len[j1]-1]) {	
	   reverse(j2);
	   concat(j1,j2);
	   j2=j1+1;
	} else
	  ++j2;
      }
      ++j1;
      j2 = j1+1;
    }

    for (j=0; j<num; j++) {
          printf("#%d %d\n", j, col[j]);
          for (l=0; l<len[j]; l++) {
              printf("%7.3f %8.3f", x[j][l], y[j][l]);
	      if (l%4==3 || l==len[j]-1) 
                 printf("\n"); 
              else 
                 printf("  ");
	  }
          printf(";\n\n");
    }
    fclose(fd);
    exit(0);
}


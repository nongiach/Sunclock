
/*
	edx/emx (EDitor for X), (C) 2002, Terry Loveall
	released into the public domain.
	Based upon the original work ee.c of Yijun Ding, copyright 1991 which is
	in the public domain.
	This program comes with no warranties or binaries. Use at your own risk.
*/

/* This is the core editor engine */

/*
void prrec(U_REC* undp, char indicator)
{
		char str[256];

		if(undp == NULL) return;

		memset(str,0,256);
		memcpy(str, undp->str, undp->length >= 0 ?  undp->length: 0 - undp->length);
		printf("%c:p=%x, l=%x, pos=%x, ln=%x, str=%s.\n",
				indicator,
				(unsigned int)undp,
				(unsigned int)undp->link + unbuf,
				(unsigned int)(undp->pos + edbuf),
				undp->length,
				str);
}
*/

/* get true x co-ord for char at 'pos' */
int get_tru(char* pos)
{
	char* tmpp;
	int i = 1;

	tmpp = line_start + strlen(line_start+1)+1;
		if(line_start > pos || tmpp < pos) {
#ifdef DEBUG
			printf("Pos=%x, ls=%x, le=%x.\n",
					(unsigned int)pos,
					(unsigned int)line_start,
					(unsigned int)tmpp);
#else
			printf("get_tru\n");
			bell();
#endif /* DEBUG */
			return 0;
		}
		for(tmpp = line_start+1; tmpp < pos; tmpp++){
			if(*tmpp != '\t') {i++;}
			else {
				i = i + ((tabsize) - ((i-1)%tabsize));
			}
		}
		return(i);
}

char* get_cur()
{
		return(line_start+x_offset);
}

char * realloc_buffer(int mode)
{
	char *ptr;
	int i;
	switch(mode) {

		case 0: {
			i = amax+AMAX+tabsize+1;
			if (i<0) return NULL;
			ptr = realloc(edbuf, (size_t)i);
			if (ptr) {
				amax += AMAX;
				i = ptr - edbuf;
				edbuf += i;
				file_end += i;
				cur_pos += i;
				line_start += i;
				last_pos += i;
				old_pos += i;
				screen_start += i;
				mk += i;
				bstart += i;
				bend += i;
				if (last_mk) last_mk += i;
			}
			return ptr;
		}

		case 1: {
			i = bmax+BMAX+tabsize+1;
			if (i<0) return NULL;
			ptr = realloc(bb, (size_t)i);
			if (ptr) {
				bmax += BMAX;
				bb = ptr;
			}
			return ptr;
		}

		case 2: {
			i = umax+UMAX+1;
			if (i<0) return NULL;
			ptr = realloc(unbuf, (size_t)i);
			if (ptr) {
				umax += UMAX;
				i = (int)ptr - (int)unbuf;
				unbuf = ptr;
				undop = (void*)undop+i;
				undosp = (void*)undosp+i;
			}
			return ptr;
		}
	}
	return NULL;
}

void init_undo()
{
		undosp = undop = unbuf;
		undop->link = VOID_LINK;
		undop->pos = cur_pos - edbuf -1;
		undop->length = 0;
}

U_REC* new_undo(char *pos, int len)
{
	U_REC *tmp;
	int tlen = abs(undosp->length);

 retry:
	/* wrap undo pointer if no space for requested len in undo buffer */
	if ((void *)undosp->str + tlen + sizeof(U_REC) + len < unbuf + umax) {
	      	tmp = (void *) undosp->str + tlen;
	} else {
		if (realloc_buffer(2)) goto retry;
		tmp = (void *) unbuf;
	}

	tmp->link = (unsigned int)undosp - (unsigned int)unbuf;
	tmp->pos = pos - edbuf;
	tmp->length = 0;
	return tmp;
}

void u_del(char *pos, int len)
{
		if(!undosp) init_undo();

		if(!undosp->length) undosp->pos = pos - edbuf;
		/* get new_undo if top rec is u_ins, or different pos or no space */
		if((undosp->length > 0) ||
		   ( undosp->pos + edbuf - undosp->length != pos) ||		/* stack only del not bs */
		   ( (void*)undosp->str - undosp->length + abs(len) >= unbuf + umax )) {
				undosp = new_undo(pos, abs(len));
				if(undosp == unbuf) undo_wrap--;		/* say undo wrapped */
		}

		if(len) memcpy(undosp->str + abs(undosp->length), pos, len);
		undosp->length = undosp->length - len;

		if(!in_undo) { undop = undosp; undone = 0;}
}

void u_ins(char *pos, int len)
{
		int tlen;
		if(!undosp) init_undo();

		if(!undosp->length) undosp->pos = pos - edbuf;
		/* get new_undo if top rec is u_del, or different pos or no space */
		if((undosp->length < 0) ||
		   /* stack only left to right insertions */
		   ( undosp->pos + edbuf + undosp->length != pos ) ||
		   ( (void *)undosp->str + undosp->length + abs(len) >= unbuf + umax )) {
				undosp = new_undo(pos, abs(len));
				if(undosp == unbuf) undo_wrap--;		/* say undo wrapped */
		}
		tlen = abs(undosp->length);
		if(len) memcpy(undosp->str + tlen, pos, len);
		undosp->length = tlen + len;
		if(!in_undo) { undop = undosp; undone = 0;}
}

void ytot_ins()
{
		char* e;
		for(e=undop->pos+edbuf; e<undop->pos+edbuf + abs(undop->length); e++)
				if(*e == EOL) ytot++;
}

void undo()
{
		/* dont undo nothing, but say nothing is unchanged */
		if(undone) { bell(); return; }
		if(undop != undosp) {
			if (undop->link == VOID_LINK)
				undop = unbuf;
			else
				undop = undop->link + unbuf;
		}
		in_undo++;
		goto_ptr(undop->pos + edbuf);
		if(undop->length < 0) { /* is delete so insert */
				/* delete flag is negative length */
				file_resize(undop->pos + edbuf, undop->pos + edbuf - undop->length);
				memcpy(undop->pos + edbuf, undop->str, - undop->length);
				u_ins(undop->pos + edbuf, - undop->length);
				ytot_ins(); 	/* adjust ytot when inserting */
		}
		else {			/* is insert so delete */
				file_resize(undop->pos + edbuf + undop->length, undop->pos + edbuf);
		}
		goto_ptr(undop->pos + edbuf);
		if(undop->link==VOID_LINK) {
				undone = -1;
				flag[CHG] = 0;
				show_top();
		}
		in_undo--;
		flag[SHW] = 1;
}

void redo()
{
		/* dont redo nothing */
		if(undop == undosp) { bell(); return; }
		in_undo++;
		goto_ptr(undop->pos + edbuf);

		if(undop->length > 0) { /* is insert so insert */
				file_resize(undop->pos + edbuf, undop->pos + edbuf + undop->length);
				memcpy(undop->pos + edbuf, undop->str, undop->length);
				u_ins(undop->pos + edbuf, undop->length);
				ytot_ins(); 	/* adjust ytot when inserting */
		}
		else {			/* is delete< so delete */
				file_resize(undop->pos + edbuf - undop->length, undop->pos + edbuf);
		}
		goto_ptr(undop->pos + edbuf);

		undop = (void*)undop->str + abs(undosp->length);
		undone = 0;
		in_undo--;
		flag[SHW] = 1;
}

void sys_exit(int code)
{
	if(!(flag[CHG]) || code==2) {
		/* watch where you're goin', clean up where you been */
#ifdef X11		
		if(selection_text) free(selection_text);
#endif
		if(cfdpath) free(cfdpath);
#ifdef X11
		exit(code&1);
#else
		gotoxy(0,0);
		ttclose();
		exitf = 0;
	        /* printf("\033c"); */           /* reset terminal */
		/* printf("\033[2J"); */              /* clear screen */
		clrscr();
		return;
#endif
	}
	bell();
}

void goto_xpos()
{
	goto_x(x_offset < strlen(line_start+1)+1 ? 
			x_offset : 
			strlen(line_start+1)+1);
}

/* cursor movement */
void cursor_up()
{
	if(flag[BLK]) flag[SHW] = 1;
	if(ytru == 0) return;
	ytru--;
	while(*--line_start != EOL) ;
	y--;
	goto_xpos();
}

void cursor_down()
{
	if(flag[BLK]) flag[SHW] = 1;
	if(ytru == ytot) return;
	ytru++;
	while(*++line_start != EOL) ;
	y++;
	goto_xpos();
}

void move_to(int newx, int newy)
{
	int old;
	if (newy < 0) return; 
	if (file_end==edbuf+1) return;
	if((y == newy && x == newx) || (executive != MAIN)) return;

	while((y < newy) && (ytru < ytot)) cursor_down();
	while((y > newy) && (ytru > 0)) cursor_up();

	if(line_start[x_offset] == EOL) cursor_left();

	/* detect impossible to achieve x pos within tabs */
	old = xtru - newx;
	while((old > 0 ? xtru > newx : xtru < newx) && (*cur_pos != EOL)){
		xtru < newx ? cursor_right() : cursor_left();
	}
}

void getx()
{
	xtru = get_tru(cur_pos);
	x_offset = cur_pos - line_start;
	x = xtru - xlo;
}

/* cursor left & right */
void cursor_left()
{
	if (cur_pos == edbuf+1) return;
	if(xtru == 1) {
		cursor_up();
		goto_x(strlen(line_start+1)+1);
		return;
	}
	if(flag[BLK]) flag[SHW] = 1;
	if( x == 1 && xlo != 0){
		xlo -= XINC;
		flag[SHW] = 1;
	}
	--cur_pos;
	getx();
}

void cursor_right()
{
	if (cur_pos == file_end) return;
	if(flag[BLK]) flag[SHW] = 1;
	if(*cur_pos == EOL || xtru >= LMAX) {
		goto_x(1);
		cursor_down();
	        return;		
	}
	if(x == screen_width-1) {
		xlo += XINC;
		flag[SHW] = 1;
	}
	++cur_pos;
	getx();
}

void cursor_pageup()
{
	int i,ytmp=y;

	for(i=1; i<screen_height; ++i) cursor_up();
	if(ytru < screen_height) ytmp = 0;
	y = ytmp;
	flag[SHW] = 1;
}

void cursor_pagedown()
{
	int i, ytmp=y;

	for(i=1; i<screen_height; ++i) {
	   if (ytru>ytot+y-screen_height) break;
	   cursor_down();
	}
        y = ytmp;
	flag[SHW] = 1;
}

void word_left()
{
	char *d = cur_pos;

	if(d[-1] == EOL) { /* at bol */
		if(ytru == 0) return; /* don't backup past start */
		cursor_up();
		goto_x(strlen(line_start+1)+1); /* goto end of line */
		cursor_right();
		return;
	}

	/* skip white space separators */
	while(strchr(wdelims,d[-1])) { d--; cursor_left(); }
	/* skip real chars */
	while(!strchr(wdelims,d[-1])) { d--; cursor_left(); }
}

void word_right()
{
	char *d = cur_pos;

	if(*d == EOL) { /* at eol */
		if(ytru >= ytot) return;	/* don't go past end */
		cursor_down();
		goto_x(1);					/* goto start of line */
		return;
	}
	
	/* skip first delimitor if any */
        if (strchr(wdelims,*d)) { d++; cursor_right(); }
	/* skip real chars */
	while(!strchr(wdelims,*d)) { d++; cursor_right(); }
	/* skip white space separators 
	   JPD changed the behaviour there - avoiding in particular 
	   a crash when reaching the last word.
	 */
#ifdef TLL
	while(strchr(wdelims,*d)) { d++; cursor_right(); }
#else
	while(isspace(*d)) { d++; cursor_right(); }
#endif
}

void word_mark()
{
	if(cur_pos[0] <= BLNK) return;	/* don't mark nothing */
	mark_off();
	if(!strchr(wdelims,cur_pos[-1])) word_left();
	block_mark();
	while(!strchr(wdelims,*cur_pos)) cursor_right();
#ifdef X11
	keve->state |= ShiftMask;
#endif
}

/* display --------------------------------------------------------*/
/* assuming cursor in correct position: show_rest(screen_width-x,line_start+x_offset) */

void drstr(char* disp, int i)
{
#ifdef X11
	drawstring(disp,i);
#else
	disp[i] = 0;
	cputs(disp);
#endif
	outxy.X += i;
}

void show_rest(int len, char *s)
{
	char *ss;
	char disp[MAXVLINE];
	int i,j,k=0,xlt=xlo;

	ss = s;
	i = 0;
	if(flag[BLK]) {
		if(cur_pos < mk)
			{ bstart = cur_pos; bend = mk; }
		else
			{ bstart = mk; bend = cur_pos; }
		if ((ss >= bstart) && (ss < bend)) highvideo();
	}
	while(*ss != EOL && i < MAXVLINE) {
		if(flag[BLK]) {
			if ((ss == bstart)) { drstr(disp+xlo,i-xlo); k += i; xlo=i=0; highvideo(); }
			if ((ss == bend)) { drstr(disp+xlo,i-xlo); k += i; xlo=i=0; lowvideo(); }
		}
		if(*ss == '\t') {
			for(j = i +(tabsize - (k+i)%tabsize); i<j; disp[i++] = ' ');
			ss++;
		}
		else disp[i++] = *ss++;
	}
	if(i) drstr(disp+xlo,i-xlo);
	if(flag[BLK] && (ss == bend)) lowvideo();
	clreol();
	xlo = xlt;
}

/* line_start and y correct */
void show_scr(int fr, int to)
{
	int len=screen_width-1;
	int i=fr;

	screen_start=line_start;

	/* calculate screen_start */
	for(; i<y; i++) while(screen_start > edbuf && *--screen_start != EOL) ;
	for(; i>y; i--) while(*++screen_start != EOL) ;

	/* first line */
	screen_start++;
	do {
		gotoxy(0,fr+yl2);
		if(screen_start<file_end && strlen(screen_start) > xlo) {
#ifndef X11
			clreol();
#endif
			show_rest(len, screen_start);
		} else {
			if(((flag[BLK]) &&
				(screen_start == bend)) ||
				(screen_start == bend+1)) lowvideo();
			clreol();
		}
		/* skip EOL */
		while(*screen_start++) ;
#ifdef X11
	} while(++fr <= to);
#else
	} while(++fr < to);
#endif
}

void scroll_up()
{
	if(!y) cursor_down();
	y--;
	flag[SHW] = 1;
}

void scroll_down()
{
	if(line_start <= edbuf) return;
	if(ytru <= y)  return;
	if(y >= screen_height-1) cursor_up();
	y++;
	flag[SHW] = 1;
}

/* undraw, redraw status and display */
void scr_update()
{

#ifdef X11
	if(executive != MAIN && !expose) return;
	undraw_cursor();
#else
	if(executive != MAIN) return;
#endif
	/* update text buffer display */
	if( flag[BLK] ) flag[SHW] = 1;
	if(y <= -1 || y >= screen_height) {
		if(y == -1) {
			y++; show_sdn(0);
		}
		else if(y == screen_height) {
			y--; show_sdn(0);
		}
		else {
			y = y < 0 ? 0 : screen_height-1;
			show_scr(0, screen_height-1);
		}
	}
	else if(flag[SHW] ) {
		show_scr(0, screen_height-1);
	}
	flag[SHW] = 0;
#ifdef X11
	if (help_done) return;
#endif
	show_pos();
	lowvideo();
	gotoxy(x-1,y+yl2);
#ifdef X11
        show_vbar();
	draw_cursor();
#endif
}

void show_sdn(int line)
{
	gotoxy(0,yl2+line);
	show_scr(0, screen_height-1);
}

void show_flag(int x, int g)
{
	gotoxy(20+x-1,yl1);
	putch(g? fsym[x]: '.');
	flag[x] = g;
}

/*
  simple minded line input executive for status line dialogs.
  Successful completion, indicated by a Return key, executes the
  callback in dialogCB. Responds to 'Esc' and ^C by terminating
  without executing the callback. Printing first char resets Col.
  Use ^H to overwrite preset string, 'End' displays cursor to EOS.
  All other characters input to diabuf. Set by 'show_gets'.
*/

void dialog(int key)
{
	int skey;

#ifdef X11
	undraw_cursor();
	gotoxy(diastart+col,yl1);
	skey = key & 0xff;
	if(keve->state & ControlMask) skey &= 0x1f;
	/* special controls for search&replace Y/N/Esc/! */
	if (dialogCB == do_sar) {
		switch(tolower(skey)) {
		case 27:  search_mode = 2; break;
		case 'n': search_mode = 3; break;
		case 10:
		case 13:
		case 'y': search_mode = 4; break;
		case '!': search_mode = 5; break;
		default:  search_mode = 6; break;
		}
		do_sar();
		first = key;
		return;
	}
	if(key == XK_End) {
		while((diabuf[col]!=0) && (col<dblen)) putch(diabuf[col++]);
	} else {
		switch(skey){
			case 8: {
				if(col < 0) col = strlen(diabuf);
				if(col) { col--; diabuf[col] = '\0'; outxy.X--; putch(' '); outxy.X--;}
				break;
			}
			case  3:
			case 13:
			case 27: {
				executive = MAIN;
				diabuf[col] = 0;
				if(*diabuf && skey == 13)
					dialogCB();
				flag[SHW] = 1;
				scr_update();
				show_top();
				break;
			}
			default: {
				if(col < 0 || !first) {
					col = 0;
					gotoxy(diastart+col,yl1);
					clreol();
				}
				if (col < dblen) {
					diabuf[col++] = key;
					diabuf[col] = '\0';
					putch(key);
				} else
					bell();
			}
		}
	}
	first = key;
	draw_cursor();
#else
	skey = key & 0xff;

	gotoxy(diastart+col,yl1);

	/* special control for search&replace Y/N/Esc/! */
	if (dialogCB == do_sar) {
		switch(tolower(skey)) {
		case 27:  search_mode = 2; break;
		case 'n': search_mode = 3; break;
		case 10:
		case 13:
		case 'y': search_mode = 4; break;
		case '!': search_mode = 5; break;
		default:  search_mode = 6; break;
		}
		do_sar();
		first = key;
		return;
	}
	if(key == 0x0c) {	/* ^L */
		while(diabuf[col] != 0) putch(diabuf[col++]);
	} else {
		switch(skey){
			case 8: {
				if(col < 0) col = strlen(diabuf);
				if(col) { col--; gotoxy(--outxy.X,outxy.Y); putch(' '); gotoxy(outxy.X,outxy.Y);}
				break;
			}
			case  3:
			case 10:
			case 13:
			case 27: {
				executive = MAIN;
				diabuf[col] = 0;
				if(*diabuf && (skey == 10 || skey == 13))
					dialogCB();
				flag[SHW] = 1;
				scr_update();
				show_top();
				break;
			}
			default: {
				if(col < 0 || !first) {
					col = 0;
					gotoxy(diastart+col,yl1);
					clreol();
				}
				diabuf[col++] = key;
				putch(key);
			}
		}
	}
	first = key;
#endif
}

/* display a dialog property string at fixed location in the status bar */

void show_note(char *prp)
{
	gotoxy(32,yl1);
	clreol();
	cputs(prp);
	diastart = outxy.X+2;
}

/* setup status line interactive dialog and input data processing callback */

int show_gets(char *prp, char *buf, int blen, void *cb)
{
#ifdef X11
	comment = prp;
#endif
	diabuf = buf;		/* point at the input buffer */
	dialogCB = cb;		/* setup the dialog callback */
	first = *diabuf;	/* first = 0 = True; */
	dblen = blen;

	lowvideo(); 		/* make dialog stand out in status line */
	show_note(prp); 	/* show function */
	cputs(": ");
	cputs(diabuf);		/* show preset, if any */
	col=strlen(diabuf);	/* point cursor at _end_ of preset string. */
	executive = DIALOG;	/* tell handle_hey() to pass keys to dialog */
#ifdef X11
	draw_cursor();
#endif
	return 0;
}

	/* update line and col */
void show_pos()
{
	char tbuf[NLEN];

	if(executive != MAIN) return;
	highvideo();
	gotoxy(5,yl1);
	sprintf(tbuf,"%d %d     ", ytru+1, xtru);
	cputs(tbuf);
}

/* redraw the status line only in executive == MAIN mode */

void show_top()
{
	int i;
	char tbuf[NLEN];

#ifdef X11
	if(executive != MAIN && !expose) return;
#else
	if(executive != MAIN || !exitf) return;
#endif
	gotoxy(0,yl1);
	highvideo();
	clreol();
	show_pos();
	for(i=0; i<=EDT; i++)
		show_flag(i, flag[i]);
	sprintf(tbuf,"   "HELP"=F1   %s", cfdpath);
	cputs(tbuf);
	lowvideo();
	gotoxy(x-1,y+yl2);
}

/* chdir to working directory and fire up another copy of the editor */
void new_edit(char *nfile)
{
	char b[1024];
	sprintf(b,"cd %s; "EDIT" %s &",
		cfdpath ? cfdpath : "./", nfile ? nfile : "" );
	SYSTEM(b);
}

/* display help/about page */

void show_help(int mode)
{
#ifdef EXTHELP
	char name[NLEN];
#ifdef EMACS
	sprintf(name, "%s/%s", DOCDIR, "MANUAL.emx");
#else
	sprintf(name, "%s/%s", DOCDIR, "MANUAL.wordstar");
#endif
	new_edit(name);
#else

	char* prntstr;
	int i;

        flag[SHW] = 0;
	flag[BLK] = 0;
	last_mk = mk;
	mk = (void*) -1;
	clrscr();
	show_top();
	gotoxy(0,yl2);

#ifdef X11

#ifdef MINIMAL
	prntstr = HELP_STR;
	/* char at a time to process EOL char */
	for(i=0; i < sizeof(HELP_STR); putch(prntstr[i++]));
#else
	if (mode == 0) {
		prntstr = HELP_STR;
		/* char at a time to process EOL char */
		for(i=0; i < sizeof(HELP_STR); putch(prntstr[i++]));
	}
	if (mode == 1) {
                int j;
		prntstr = ALT_KEY_BINDINGS;
		for (j=0; j<strlen(prntstr); j++) putch(prntstr[j]);
		putch(EOL); putch(EOL);
		for(i=0; i<26; i++) if ((prntstr=binding[i])) {
			putch('a'+i); putch(':'); putch(' ');
			for (j=0; j<strlen(prntstr); j++) putch(prntstr[j]);
			putch(EOL);
		}
	}

#endif /* MINIMAL */

#else /* TERMCAP */

#ifdef MINIMAL
	/* line at a time to process EOL char */
	i=0;
	prntstr = HELP_STR;
	while(prntstr < HELP_STR + sizeof(HELP_STR)) {
		puts(prntstr);
		prntstr += strlen(prntstr)+1;
		gotoxy(0,++i);
	}
#else
	if (mode == 0) {
		/* line at a time to process EOL char */
		i=0;
		prntstr = HELP_STR;
		while (prntstr < HELP_STR + sizeof(HELP_STR)) {
			puts(prntstr);
			prntstr += strlen(prntstr)+1;
			gotoxy(0,++i);
		}
	}
	if (mode == 1) {
                int j=2;
		prntstr = ALT_KEY_BINDINGS;
		puts(prntstr);
		gotoxy(0, j);
		for(i=0; i<26; i++) if ((prntstr=binding[i])) {
			putchar('a'+i); putchar(':'); putchar(' ');
			puts(prntstr);
			gotoxy(0, ++j);
		}
	}
#endif /* MINIMAL */
#endif /* TERMCAP */
	help_done = -1;
	scr_update();
	mk = last_mk;
#endif /* EXTHELP */
}

/*
  set editor options executive.
  Accept a single UPPER or lower case char only from the string "MFOCTBAE":
	Modified, Fill, Overwrite, Case, Tab, Block marked, replace All, Edit.
*/

void options(int key)
{
	char *d;
	int k;

	if ((key&255) < 0x20)
	   goto end_options;

	k = toupper(key) & ~0x20;
	if((d=strchr(fsym, k)) != 0) {
		k = d-fsym;
		show_flag(k, !flag[k]);
	}
    end_options:
	executive = MAIN;
	show_top();
	scr_update();
}

/* setup 'executive = OPTIONS' mode */

void show_mode()
{
	highvideo();
	show_note(fsym);
	lowvideo();
	putch(BLNK);
	executive = OPTIONS;
}

/* file operation ---*/

void file_read()
{
	int c, d;
	char *col = file_new();

	if(fi == 0) return;

	/* read complete line */
	do {
		c = fgetc(fi);
		if(c == EOF) {
			fclose(fi);
			fi = 0; /* no more read */
			break;
		}
		if(c == '\t' && flag[TAB]) { /* conditionally convert tabs on input */
			do (*file_end++ = BLNK);
			while( ((file_end-col) % tabsize) != 0);
		}
		else
			if(c == LF) {
			*file_end++ = EOL;
			col = file_end;
			ytot++;
		}
		else *file_end++ = c;
		if (file_end >= edbuf+amax) {
			d = file_end - col;
			realloc_buffer(0);
			col = file_end - d;
		}
	} while(file_end < edbuf+amax);
	for(; ewin.jump>1; ewin.jump--) cursor_down();
}

/* compress one line from end */

char *file_ltab(char *s)
{
	char *d, *e;

	e = d = strchr(s--, EOL);
	while(*--e == BLNK) ;	/* trailing blank */
	while(e > s) {
		if(e[0] == BLNK && (e-s)%tabsize == 0 && e[-1] == BLNK) {
			*--d = 9;
			while(*--e == BLNK && (e-s)%tabsize != 0) ;
		}
		else *--d = *e--;
	}
	return d;
}

/* routine used for write block file also, this makes it more complicated */

int file_write(FILE *fp, char *s, char *e)
{
	if(fp == 0) return 1; /* no write */
	do {
		if(flag[TAB] && *s != EOL)
			s = file_ltab(s);
		if(*s && fputs(s, fp) < 0)
			return 1;
		fputc(LF, fp);
		while(*s++ != EOL) ;
	} while(s < e);
	return 0;
}

int file_fout()
{
	if(fo == 0) {
                strcpy(bbuf, ewin.name);
		strcat(bbuf, "XXXXXX");
		close(mkstemp(bbuf));
		fo=fopen(bbuf, "w");
	}
	if (!flag[NEW]) {
		fchmod(fileno(fo), ofstat.st_mode & 07777);
	}
	return file_write(fo, edbuf+1, file_end);
}

void do_save()
{
	char bakfile[NLEN+1];

	/* prompt on empty filename */
	if((ewin.name[0] == 0)) { 
		file_save(-1,0); 
		return;
	}

	executive=MAIN;
	if(file_fout() ) {
		return; /* exit if can't write output */
	}

	fclose(fo);
	fo = 0;	/* set fo to known condition */
	/* if editing old file then delete old file */
	if (flag[NEW] == 0) {
		strcpy(bakfile, ewin.name);
		strcat(bakfile, "~");
		unlink(bakfile);
		rename(ewin.name, bakfile);
#ifdef MINIMAL
		unlink(bakfile);
#endif
	}

	rename(bbuf, ewin.name);	/* ren new temp to old name */
#ifdef X11
	set_title(ewin.name);
#endif
	flag[CHG] = 0;
	show_top();
}

/* go to top of file and reset to known condition */
void top()
{
	yl1 = YTOP;
	yl2 = yl1+1;
	line_start = edbuf;
	x_offset = 1;
	xtru = x = 1;
	ytru = y = 0;
	flag[SHW] = 1;
}

/* new file init */
char *file_new()
{
	clrscr();
	top();
	edbuf[0] = EOL;
	last_pos = edbuf;
	file_end = edbuf+1;
	ytot = 0;
	flag[NEW] = 0;
	mark_off();
	last_mk = NULL;
	if((fi = fopen(ewin.name, "r")) == 0) flag[NEW]++;
	else fstat(fileno(fi), &ofstat);
	return(file_end);
}

/* exec the forking cmd */
int SYSTEM (char *cmd)
{
		#define SHELL	"/bin/sh"
		pid_t pid;
		switch(pid=fork()) {
			case 0:
				execl(SHELL,"sh","-c",cmd,(char *) 0);
				printf(EXECL_FAILED"\n");
				_exit(127);
			case -1: printf(FORK_ERROR"\n");
			default: return(0);
		}
}

void gorun()
{
	SYSTEM(Command);
}

#ifdef MINIMAL
/* chdir to working directory and fire up rxvt */

void mterm()
{
	char b[1024];
	sprintf(b,"cd %s;rxvt -font 8x16 -ls -sl 500 -sr +st -geometry 96x28 &",
			cfdpath ? cfdpath : "./");
	SYSTEM(b);
}
#else
void launch_edit()
{
	if (*diabuf)
		new_edit(diabuf);
	free(diabuf);
}

void req_edit()
{
        diabuf = malloc(NLEN+1);
		*diabuf = '\0';
		show_gets(OPEN, diabuf, NLEN, &launch_edit);
		return;
}
#endif /* MINIMAL */

void do_cd()
{
	chdir(cfdpath);
}

void chgdir()
{
	show_gets("cd", cfdpath, BUFSIZ, do_cd);
}

void do_open()
{
	file_read();
#ifdef X11
	set_title(ewin.name);
#endif
	executive=MAIN;
}

void file_save(int f_all, int f_win)
{
        if (!flag[EDT]) return;
	if(((flag[CHG]) | f_all) || f_win==-1) {
		show_gets(SAVE_AS, ewin.name, sizeof(ewin.name), &do_save);
		return;
	}
	/* optionally make new empty file */
	if(f_win==1) {
		show_gets(OPEN, ewin.name, sizeof(ewin.name), &do_open);
	}
}

/* main editor workhorse. Inserts/deletes specified source/destination */
void file_resize(char *s, char *d)
{
	char	*e;
	unsigned int i;
	int s_rel = s - edbuf;
	int d_rel = d - edbuf;

	/* immediate problem only when block buffer on disk too big */

        i = file_end - s;
	file_end += d-s;
 retry:
	if(file_end>= edbuf+amax) {
		if (realloc_buffer(0)) { 
			s = edbuf + s_rel;
			d = edbuf + d_rel;
			goto retry; 
		}
		show_note(MAIN_BUFFER_FULL);
		bell();
		return;
	}

	e = s+i;
	if(s < d) { /* expand */
		d += e - s;
		s = e;
		while(i-- > 0) *--d = *--s;
	}
	else {
		u_del(d, s - d);		/* save undo delete string */
		/* adjust ytot when shrink */
		for(e=d; e<s; e++) if(*e == EOL) ytot--;
		while(i-- > 0) *d++ = *s++;
	}
	*file_end = EOL;	/* last line may not complete */
	if(!flag[CHG] ) {
		show_flag(CHG, 1);
		gotoxy(x-1,y+yl2);
		clreol();
	}
}

/* search and goto */

/* xx >= 1, yy >= 0 */

void goto_x(int xx)
{
	cur_pos = line_start + xx;
	if (cur_pos>file_end) cur_pos=file_end;
	xtru = get_tru(cur_pos);
	if(!xtru) cur_pos--;
	x_offset = cur_pos - line_start;

	xx = xlo;
	if(xtru == 1) xlo = 0;
	if(xtru-xlo >= screen_width)
		xlo = ((xtru + XINC - screen_width)/XINC) * XINC;
	x = xtru - xlo;
	if (xlo!= xx)
		flag[SHW]=1;
}

void goto_y(int yy)
{
	int i, n;

	n = ytru;

	for(i=n; i<yy; i++) cursor_down();
	for(i=yy; i<n; i++) cursor_up();
	if(y > screen_height || y < 0) {
		flag[SHW] = 1;
		y = screen_height/4;
	}
}

void goto_ptr(char *s)
{
	/* find line_start <= s */
	char *s1 = s;
	int yo = y;

	if(s < edbuf || s >= edbuf + amax) {
		bell();
		return;
	}
	top();
	while(*--s1 != EOL && s1 > edbuf) ; 				/* find start of target line */
	while(line_start+1 <= s1 && y < ytot) cursor_down();/* move to target line */
	while(line_start+1 > s && y > 0) cursor_up();		/* move back before target */
	goto_x(s-line_start);								/* get x from line start */
	if(y > screen_height || xlo != 0) {
		flag[SHW] = 1;
		y = yo;
	}
}

void goline()
{
	goto_y(atoi(lbuf)-1);
}

void goto_last()
{
	goto_ptr(last_pos); cursor_down();
}

void goto_line()
{
	show_gets(GOTO_LINE, lbuf, sizeof(lbuf), goline);
}

void switch_marks()
{
	char *c;
	if (!last_mk || last_mk>=file_end) return;
	c = cur_pos;
	goto_ptr(last_mk);
	last_mk = c;
	if (mk) mk = last_mk;
}

void run()
{
	show_gets(COMMAND, Command, sizeof(Command), gorun);
}

void gocol()
{
	goto_x(atoi(cbuf) );
}

void goto_col()
{
	show_gets(GOTO_COLUMN, cbuf, sizeof(cbuf), gocol);
}

/* find and set cursor to matching bracket '(,{,[,],},)' */

void find_match()
{
	char *s = cur_pos;
	int dire, cnt;
	char ch, mch;

	if (!s) return;

	ch = *s;
	switch (ch) {
	case '(':
		mch = ')'; dire = 0; break;
	case ')':
		mch = '('; dire = 1; break;
	case '[':
		mch = ']'; dire = 0; break;
	case ']':
		mch = '['; dire = 1; break;
	case '{':
		mch = '}'; dire = 0; break;
	case '}':
		mch = '{'; dire = 1; break;
	default:
		return;
	}
	cnt = 0;
	if (dire) {
		while (--s >= edbuf) {
			if (*s == ch) cnt++;
			else if (*s == mch) {
				if (!cnt) goto match;
				cnt--;
			}
		}
	} else {
		while (++s < file_end) {
			if (*s == ch) cnt++;
			else if (*s == mch) {
				if (!cnt) goto match;
				cnt--;
			}
		}
	}
	return;
match:
	goto_ptr(s);
	flag[SHW]=1;
}

/* compare to sbuf. used by search */
int str_cmp(char *s)
{
	char	*d = sbuf;

	if(flag[CAS] )
	{
		while(*d ) if(*s++ != *d++ ) return 1;
	}
	else
	{
		while(*d ) if(tolower(*s++) != tolower(*d++)) return 1;
	}
	return 0;
}

/* back/forward search */
char *goto_find(char *s, int back)
{
	mark_off();
	do
	{
		if(back) {
			if(--s <= edbuf) {
				bell();
				return 0;
			}
		}
		else {
			if(++s >= file_end) {
				bell();
				return 0;
			}
		}
	} while(str_cmp(s) );
	goto_ptr(s);
	block_mark();
	mk += strlen(sbuf);
	return s;
}

void gofindforward()
{
	goto_find(cur_pos, 0);
}

void gofindbackward()
{
	goto_find(cur_pos, 1);
}

void get_mk(char* dbuf)
{
	if(strchr(sdelims,*cur_pos) && !flag[BLK]) return;
	memset(dbuf,0,NLEN * sizeof(char));
	if(!flag[BLK]) word_mark();
	if(flag[BLK] && (mk != cur_pos) && (abs(cur_pos - mk) < NLEN)){
		memcpy(dbuf, mk < cur_pos ? mk : cur_pos, abs(cur_pos - mk)); 
	}
}

char *goto_search(int back)
{
	if (back) --cur_pos;
	get_mk(sbuf);
	if (back) ++cur_pos;
	search_mode = back;
   	show_gets(SEARCH_FOR, sbuf, sizeof(sbuf),
                  (back)? gofindbackward:gofindforward);
	return NULL;
}

void do_sar()
{
	int i;
	char *sb;

	if (search_mode<=2) {
		executive = MAIN;
		flag[SHW] = 1;
#ifdef X11
		free(comment);
#endif
		scr_update();
		show_top();
		return;
	}

    repl_again:
	if (search_mode==4 || search_mode==5) {
		rlen = strlen(rbuf);
		do {
			file_resize(s+slen, s); 		/* delete srch string&save undo data */
			file_resize(s, s+rlen); 		/* insert space for replace string */
			memcpy(s, rbuf, rlen);
			u_ins(s, rlen);				/* add replace string to undo buf */
			s = s+rlen;
		}
		while(flag[ALL] && (s=goto_find(s, 0)) != 0) ;
		flag[SHW] = 1;
	}
	if (search_mode==6) return;
	if((s =(char*) goto_find(cur_pos,0))) {
		slen = strlen(sbuf);
		show_scr(0, screen_height-1);
		gotoxy(x-1,y+yl2);
		highvideo();
		i = slen;
		sb = s;
		while(i-- && !putch(*sb++));
		lowvideo();
		if (search_mode==5) goto repl_again;
	} else {
		search_mode = 2;
		do_sar();
	}
}

void ask_replace()
{
#ifdef X11
	comment = malloc(2*NLEN);
#else
	char comment[2*NLEN];
#endif
	search_mode = 3;
	word_left();
	--cur_pos;
	do_sar();
	if (search_mode == 2) return;
	sprintf(comment, ASK_REPLACE, rbuf);
	show_gets(comment, "", 2*NLEN-1, do_sar);
}

void replace_with()
{
	show_gets(REPLACE_WITH, rbuf, sizeof(rbuf), ask_replace);
}

void goto_replace()
{
	get_mk(sbuf);
	show_gets(SAR_FOR, sbuf, sizeof(sbuf), replace_with);
}

void gettab()
{
		tabsize = atoi(twbuf);
}

void tab_size()
{
	show_gets(TAB_SIZE,twbuf, sizeof(twbuf), gettab);
}

void getmarg()
{
	screen_width = atoi(wbuf);
	flag[SHW]++;
}

void window_size()
{
	show_gets(WRAP_COL, wbuf, sizeof(wbuf), getmarg);
}

/* block and format ---*/
/* use blen, mk, bb */

void mark_off()
{
        mk = NULL;
	flag[BLK] = 0;
	flag[SHW] = 1;
	show_top();
}

void reset_mark()
{
	if (!last_mk) return;
	flag[BLK] = 1; mk = last_mk; 
}

void block_mark()
{
	if( mk == NULL ) {
		mk = cur_pos;
		last_mk = mk;
		flag[BLK] = 1;
		show_top();
	}
	else
		mark_off();
}

void block_put()
{
	while (blen >= bmax && realloc_buffer(1));

	if (blen < bmax)
		memcpy(bb, mk, blen);
	else {
		if(fb == 0 && (fb = tmpfile()) == 0) return;
		fseek(fb, 0L, 0);
		fwrite(mk, 1, blen, fb);
	}
}

void block_get()
{
	int i;

	while (mk+blen >= edbuf+amax && realloc_buffer(0));
	if (mk+blen < edbuf+amax) 
		memcpy(mk, bb, blen);
	else {
		if(fb == 0) return;
		fseek(fb, 0L, 0);		/* 0L offset, 0=beginning of file */
		fread(mk, 1, blen, fb);
	}
	/* calculate ytot */
	for(i=0; i<blen; i++) {
		if((mk[i] == EOL) || (mk[i] == LF)) {
			ytot++;
			mk[i] = 0;
		}
	}
}

void block_line()
{
	if(ytru == ytot) return;
	goto_x(1 );
	for(blen = 0; line_start[++blen] != EOL; ) ;
	mk = line_start+1;
	block_put();

	/* delete line and save to undo buf */
	file_resize(line_start+blen, line_start);
	show_sdn(y);
	mark_off();
}

void remove_block()
{
	char *s;

	if(!flag[BLK]) return; 		/* exit if no mark */
	if(cur_pos < mk) {		/* ensure cur_pos >= mk */
		s = cur_pos ;		/* swap cur_pos & mk */
		cur_pos = mk ;
		mk = s ;
	}
	s = cur_pos;
	goto_ptr(mk);

	/* delete block and save to undo buf */
	file_resize(s, mk);
	cur_pos = mk;
}

void block_remove_update()
{
	remove_block();
	mark_off();
}

void chg_case(int upper)
{
	char *s;
	int i, j, change=0;

	if(!flag[BLK]) return; 	/* exit if no mark */
	if(cur_pos < mk) {		/* ensure cur_pos >= mk */
		s = cur_pos ;		/* swap cur_pos & mk */
		cur_pos = mk ;
		mk = s ;
		goto_ptr(cur_pos);
	}
	i = j = cur_pos+1 - mk;		/* get length */

	/* detect whether there is any change */
	while(j--) 
	  if (mk[j] != (!upper)? tolower(mk[j]) : toupper(mk[j])) {
	     change = 1;
	     break;
	  }
	if (!change) return;
	u_del(mk, i-1);			/* save undo data enmasse */
	/* printf("mk=%x, i=%d.\n",mk,i); */
	while(i--) mk[i] = !upper ? tolower(mk[i]) : toupper(mk[i]);
	u_ins(mk, cur_pos - mk);
        flag[CHG] = 1;
	flag[SHW] = 1;
}

void transpose()
{
	char c;
	if (cur_pos && cur_pos>edbuf+1 && cur_pos<file_end) {
           u_del(cur_pos-1, 2);
	   c = *cur_pos;
	   *cur_pos = *(cur_pos-1);
	   *(cur_pos-1) = c;
	   u_ins(cur_pos-1, 2);
           flag[CHG] = 1;
           flag[SHW] = 1;
        }
}

void block_copy(int delete)
{
	char *s;

	if(!flag[BLK]) return; 		/* exit if no mark */
	if(cur_pos < mk) {		/* ensure cur_pos >= mk */
		s = cur_pos ;		/* swap cur_pos & mk */
		cur_pos = mk ;
		mk = s ;
	}
	blen = cur_pos - mk;	/* get length */
	block_put();			/* copy block to buffer */
	if(delete) {
		block_remove_update();
		return;
	}
	mark_off();
}

void block_paste()
{
	if(!blen) return;			/* dont paste nothing */
	if(flag[BLK]) block_remove_update();		/* delete marked block */

	file_resize(cur_pos, cur_pos+blen);
	mk = cur_pos;
	block_get();

	/* save to undo buf */
	u_ins(mk - 1, blen);
	goto_ptr(mk + blen);
	mark_off();
}

/* read file into cursor position */
void doblockr()
{
	char *bb_end;
	int c, d;
	char *col;

	if(!(fb = fopen(fbuf, "r"))) { bell(); return; };

	fseek(fb, 0L, 2);		/* seek to 0L offset, 2=end of file */
	blen=ftell(fb); 		/* get file length */
	fseek(fb, 0L, 0);		/* seek back to file start */
	bb_end = col = bb;

	/* read complete file */
	do {
		c = fgetc(fb);
		if(c == EOF) {
			fclose(fb);
			fb = 0; /* no more read */
			break;
		}
		if(c == '\t' && flag[TAB]) {
			do (*bb_end++ = BLNK);
			while( ((bb_end-col) % tabsize) != 0);
		}
		else
			if(c == LF) {
				*bb_end++ = EOL;
				col = bb_end;
			}
		else
			*bb_end++ = c;
		if (bb_end >= bb+bmax) {
			d = (int)bb;
			realloc_buffer(1);
			d = (int)bb - d;
			col += d;
			bb_end += d;
		}
	} while(bb_end < bb+bmax);

	blen = bb_end - bb;
	block_paste();
}

void block_read()
{
	show_gets(READ_FILE, fbuf, sizeof(fbuf), doblockr);
}

/* copy block to file, not to block buffer */
void doblockw()
{
	FILE	*fp;
	show_flag(TAB, 0);
	fp = fopen(fbuf, "w");
	file_write(fp, bstart, bend);
	fclose(fp);
	show_top();
}

void block_write()
{
        if (!flag[EDT]) return;
	if(!flag[BLK]) return;
	if(cur_pos < mk)
		{ bstart = cur_pos; bend = mk; }
	else
		{ bstart = mk; bend = cur_pos; }

	show_gets(WRITE_FILE, fbuf, sizeof(fbuf), (void*) doblockw);
}

/* fill current line; does not change cursor or file len */
int block_fill()
{
	int i=screen_width;

	while(line_start[--i] > BLNK) ;
	if(i == 0) i = screen_width-1;
	u_del(&line_start[i],1);
	line_start[i] = EOL;
	u_ins(&line_start[i],1);
	ytot++;
	cursor_down();
	goto_x(1);
	goto_x(strlen(line_start+1)+1);
	flag[CHG] = 1;
	return i;
}

#ifdef JUSTIFY

/* right justify a line : enlarge from oldlen to newlen by adding blank chars
   at suitable places, preferably after punctuation signs */
void justify_line(char *newline, int newlen, char *oldline, int oldlen)
{
	int added, i, j, k, l, num=0, npunct=0, quot, rem, suppl;

	added = newlen - oldlen;
	if (added <= 0) {
		strncpy(newline, oldline, newlen);
		return;
	}

	/* Count the number of punctuation signs. Justify routine is smart:
	   it attempts to increase spaces after punctuation marks rather
	   than in random places */
	for (i=0; i < oldlen; i++) {
		if (isspace(oldline[i])) {
			num++;
			if (i && ispunct(oldline[i-1])) ++npunct;
		}
	}

	if (!num) return;

	quot = added / num;
	rem = added % num;
	if (rem >= npunct) {
		rem = rem - npunct;
		suppl = 1;
	} else
		suppl = 0;

	j = 0;
	for (i=0; i < oldlen; i++) {
		newline[j] = oldline[i]; 
		j++;
		k = 0;
		if (isspace(oldline[i])) {
			k = quot;
			if ( i > 0 && ispunct(oldline[i-1])) {
				if (suppl || rem>0) {
			 		++k;
			 		if (!suppl) --rem;
				}
			} else
			if (suppl && rem>0) {
		       		++k;
			 	--rem;
			}
		}
		for (l=0; l<k; l++) {
			newline[j] = BLNK;
			j++;
		}
	}
}

/* format paragraph : mode==0 don't fill, mode==1 fill lines */
void block_format(int mode)
{
	char *s, *sp, *line = NULL, *newline, *newpar;
	int j, k, l, xtmp, length, maxlen, parlen, pb, nret;

	xtmp = x-1;
	if (xtmp<0) xtmp = 0;
	length = screen_width - 4 - xtmp;
	if (length <= 5) return;

	goto_x(1);
	s = cur_pos;
	nret = 0;

	while(s<file_end && (*s==EOL || isspace(*s))) {
		if (*s==EOL) ++nret;
		++s;
	}
	if (s==file_end) return;
	if (nret) {
		goto_ptr(s);
		goto_x(xtmp+1);
		return;
	}
	mk = last_mk = cur_pos;

        j = l = 0;
	pb = -1;
	maxlen = length+2;
	parlen = maxlen+xtmp+2;

	newline = malloc(maxlen);
	newpar = malloc(parlen);

 iter:
	if (j > maxlen-2 || !line) {
		maxlen = j+2;
		line = realloc(line, maxlen);
	}
	nret = 2;
	if (*s==EOL || isspace(*s)) {
		sp = s;
		while (nret && sp<file_end && (*sp==EOL || isspace(*sp))) {
			if (*sp==EOL) --nret;
			++sp;
			if (sp==file_end) nret = 0;
		}
		line[j] = BLNK;
		if (j==length) pb = j;
		if (j>=length || !nret) {
			if (l+j+length+xtmp>parlen-2) {
				parlen = l+j+length+xtmp+2;
				newpar = realloc(newpar, parlen); 
			}
			for (k=0; k<xtmp; k++)
				newpar[l++] = BLNK;
			if (pb<0 || (!nret && j<=length)) {
				for (k=0; k<j; k++)
					newpar[l++] = line[k];
			} else {
				if (mode) {
					justify_line(newline,length, line,pb);
					for (k=0; k<length; k++)
						newpar[l++] = newline[k];
				} else {
					for (k=0; k<pb; k++)
						newpar[l++] = line[k];
				}
				if (j>length) {
					--s;
					while(!isspace(*s)&& *s!=EOL) --s;
					sp = s+1;
					nret = 2;
				}
			}
			newpar[l++] = EOL;
			j = 0;
			pb = -1;
		} else {
			pb = j;
			++j;
		}
		if (!nret) goto finish;
		s = sp;
	} else {
		line[j] = *s;
		++j;
		++s;
	}
	goto iter;

 finish:
	--l;
	while (*s==EOL || isspace(*s)) --s;
	++s;

	/* store data only if a change occurred */
	if (l!=s-last_mk || memcmp(last_mk, newpar, l)) {
		k = flag[OVR];
		cur_pos = s;
		flag[BLK] = 1; 
		flag[OVR] = 0;
		remove_block();
		goto_ptr(last_mk);
		for (j=0; j<l; j++) {
			if (newpar[j] == EOL) ++ytot;
			key_normal(newpar[j]);
		}
		s = cur_pos;
		flag[OVR] = k;
	}
	while (s<file_end && (*s==EOL || isspace(*s))) ++s;
	goto_ptr(s);
	goto_x(xtmp+1);
	flag[CHG] = 1;
	flag[SHW] = 1;
	free(line);
	free(newline);
	free(newpar);
}
#else

/* format paragraph */
void block_format(int mode)
{
	char	*s=line_start;
	int ytmp = y;

	goto_x(1);
	/* remove newlines to end of para */
	while(s = strchr(++s, EOL), ytru < ytot && s[1] != EOL) {
		u_del(s,1);
		*s = BLNK;
		u_ins(s,1);
		ytot--;
		flag[CHG] = 1;
	}
	/* truncate line <= screen_width */
	while(goto_x(strlen(line_start+1)+1), x_offset >= screen_width) block_fill();
	/* goto end of line */
	while(line_start[x_offset] ) cursor_right();
	show_top();
	if( flag[SHW] == 0) show_scr(ytmp, screen_height);
}
#endif

/* key actions ---*/

void key_deleol(char *s)
{
	if(ytru == ytot) return;
	if(flag[BLK]) {
		block_remove_update();		/* delete marked block */
		return;
	}
	goto_ptr(s);

	/* delete eol and save to undo buf */
	file_resize(s+1, s);
	flag[SHW]=1;
}

/* delete char under cursor */
void key_delete()
{
	char	*s=cur_pos;

	if(flag[BLK]) {
		block_remove_update();
		return;
	}
	else if( *s == EOL) {
		key_deleol(s);
		return;
	}
	/* delete cursor char and save to undo buf */
	file_resize(s+1, s);
	show_rest(screen_width-x, s);
}

/* delete char before cursor */
void key_backspace()
{
	char *s=cur_pos;

	if(flag[BLK]) {
		block_remove_update();
		return;
	}
	if(*--s == EOL) { /* delete EOL */
		if(ytru == 0) return;
		cursor_up();
		goto_x(strlen(line_start+1)+1);
		key_deleol(s);
		return;
	}
	cursor_left();
	key_delete();
	flag[SHW]=1;
}

/* delete non-white string + white-space string */
void key_delword(int eol)
{
	char	*d=cur_pos;

	if(flag[BLK]) {
		block_remove_update();
		return;
	}
	if(*d == EOL) {
	        blen = 1;
	        mk = d;
	        block_put();
		key_deleol(d);
		return;
	}
	if(eol) while(*d != EOL) d++;
	else {
		while(!strchr(sdelims,*d) && *d != EOL) d++;
		while(strchr(s1delims,*d) && *d != EOL) d++;
	}
	mk = cur_pos;
	blen = d-mk;
	block_put();
	/* delete word and save to undo buf */
	file_resize(d, cur_pos);
	flag[SHW]=1;
}

/* insert tab char */
void key_tab(int tabi)
{
	if(flag[BLK]) {
		key_delete();
	}

	if(flag[TAB]){
		do key_normal(BLNK);
		while((xtru%tabsize) != 1 && *cur_pos != EOL);
	} else {
		key_normal('\t');
	}
}

/* insert newline, increase line count */
void key_return()
{
	char	*s = cur_pos;

	/* reset marked block on char entry */
	/* JPD: disable this behaviour !
	if(flag[BLK]) {
		key_delete();
		s = cur_pos;
	}
	else 
	*/
        if(flag[OVR] ) {
		cursor_down();
		goto_x(1);
		return;
	}
	file_resize(s, s+1);
	goto_x(1);
	*s = EOL;
	/* save newly inserted EOL to undo buf */
	u_ins(s, 1);
	ytot++;
	/* get prev line_start */
	s = line_start+1;
	cursor_down();
	goto_x(1);	/* ensure visible cursor */
	if(flag[IND]) {
		int i=0;
		/* count spaces and tabs */
		while(s[i] == BLNK || s[i] == '\t') i++;
		if(i){ /* and only if */
			file_resize(line_start+1, line_start+1+i);
			memcpy(line_start+1,s, sizeof(char) * i);
			/* save undo info */
			u_ins(line_start+1, i);
			x_offset += i;
			goto_x(i+1);
		}
	}
	flag[SHW]=1;
}

/* everything ASCII else */
void key_normal(int key)
{
	char *s=line_start+x_offset;
	int xtmp;

	/* reset marked block on char entry */
	if(flag[BLK]) { key_delete(); s = cur_pos; }

	if(flag[OVR] && *s != EOL) {
		/* save old overwrite char to undo buf */
		u_del(s, 1);
		putch(*s = key);
		flag[CHG] = 1;
	}
	else {
		file_resize(s, s+1);
		*s = key;
		flag[SHW] =1 ;
	}
	cursor_right();
	/* save new overwrite/insert char to undo buf */
	u_ins(s, 1);

	if(!flag[FIL] || xtru < screen_width) return;

	xtmp = block_fill();	/* cursor_down */
}

/* simple, aint it? */
void rec_macro(int k)
{

	if(k & 0xff00) bell();	/* beep&discard non ASCII, ie func keys */
	else					/* else record key */
#ifdef X11
		*pmbuf++ = keve->state & ControlMask ? k & 0x1f: k;
#else
		*pmbuf++ = k;
#endif
}

/* turn on macro recording or play back macro */
void key_macros(int record)
{
	char *s=mbuf;
	int k;

	if(!record) {	/* play macro back */
		if(*s == 0) { bell(); return; };	/* ding user on empty mbuf */
		doCtrlX = 0;		/* turn off control key state from ^K */
#ifdef X11
		keve->state = 0;	/* turn off control key state from ^P */
#endif
		while(*s != 0) {
			k = 255 & *s++;
			switch(executive) {
				case MAIN:
					main_exec(k);
					break;
				case DIALOG:
					dialog(k);
					break;
				case OPTIONS:
					options(k);
					break;
			}
		}
		flag[SHW] = 1;
		return;
	}
	/* else toggle recording on/off */
	flag[REC] ^= 1;
	show_top();
	if(!flag[REC]) {	/* terminal condition */
		pmbuf[-2] = 0;	/* backup over ^K^M & terminate macro with a 0x0 */
		pmbuf = mbuf;	/* reset record pointer to start of mbuf */
	}
}

#ifdef GREEK
void key_greek(int key)
{
unsigned char c;
char *seq = NULL;
	int i, l;
	c = key | 0x40;
	if (c>='a' && c<='z')
		seq = greek_letter[c-'a'];
	if (c>='A' && c<='Z') 
	  	seq = greek_letter[c-'A'];
	if (seq) {
	  	key_normal('\\');
		if (*seq=='!') {
			++seq;
			if (var_greek && c>='a') {
				key_normal('v');			
				key_normal('a');			
				key_normal('r');
			}
  		}
		l = strlen(seq);
		if (c>='a')
			for (i=0; i<l; i++) key_normal(tolower(seq[i]));
		else
			for (i=0; i<l; i++) key_normal(seq[i]);
	}
	var_greek = (c == 'v' || c=='V');
}
#endif

#ifndef MINIMAL
void key_binding(int key)
{
	char b[1024];
	if (key=='!') {
	   run();
	   return;
	}
	if (key=='?') {
		show_help(1);
		return;
	}
	if (key>=0x20) key = (key|0x60)-'a';
	if (key>=0 && key<26) if (binding[key]) {
		sprintf(b, binding[key], (cfdpath? cfdpath : "./"), ewin.name);
		SYSTEM(b);
	}
}
#endif



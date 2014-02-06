
/*
	edx (EDitor for X), (C) 2002, Terry Loveall
	released into the public domain.
	Based upon the original work ee.c of Yijun Ding, copyright 1991 which is
	in the public domain.
	This program comes with no warranties or binaries. Use at your own risk.
*/

#define EDIT "xterm -cr green -ms red -fn 9x15 -geometry 80x32 -e edt"

#ifndef MINIMAL
#define DEFAULT_RC "edtrc"
#endif

/* include the editor engine */
#include "edit.c"

/* Wordstar bindings */

#ifdef GREEK
/* similar to Ctrl-C under emacs... */
void ctrlc_key(int key)
{
	key_greek(key);
	doCtrlC = var_greek;
}
#endif /* GREEK */

#ifdef MINIMAL
void key_binding(int key)
{
	switch(key) {
		case 'c': SYSTEM("xcalc"); break;				/* Alt-C xcalc*/
		case 'd': new_edit("/c/text/phone.dir"); break;	/* Alt-D phone Dir */
		case 'l': SYSTEM("xcalendar"); break;			/* Alt-L xcalendar */
		case 'u': redo(); break;						/* Alt-U redo */
	}
}
#endif

/* ctrl K keys */
void ctrlk_key(int key)
{
	switch(key | 0x60) {
		case 'b': block_mark(); break;		/* ^K^B set mark block on */
#ifdef X11
		case 'c': set_selection();			/* ^K^C block copy to X clipboard */
#endif
				  block_copy(0); break; 	/*	and to bbuffer */
		case 'd': flag[CHG]=0; break;		/* ^K^D say file not changed */
		case 'f': file_save(0,-1); break;	/* ^K^F new file */
#ifdef GREEK
		case 'g': doCtrlC = 1; break;		/* ^K^G Greek char */
#endif /* GREEK */
		case 'h': show_help(0); break;		/* ^K^H show help menu */
		case 'k': block_mark(); break;		/* ^K^K set mark block on */
		case 'm': key_macros(1); break; 	/* ^K^M record macro */
		case 'p': key_macros(0); break; 	/* ^K^P play macro */
		case 'q': sys_exit(0); break;		/* ^K^Q exit only if file saved */
		case 'r': block_read(); break;		/* ^K^R read file into cursor pos */
		case 's': do_save(); break; 		/* ^K^S save file and resume */
		case 't': tab_size(); break;		/* ^K^T get tab size */
		case 'u': redo(); break;	 	/* ^K^U redo */
		case 'v': block_paste(); break; 	/* ^K^V copy buffer to cursor */
		case 'w': block_write(); break; 	/* ^K^W write block to disk */
		case 'x': sys_exit(0); break;		/* ^K^X exit only if file saved */
		case 'y': block_copy(1); break; 	/* ^K^Y cut block to buffer */
#ifdef MINIMAL
		case 'z': mterm(); break;		/* ^K^Z open an rxvt term */
#endif

	}
	doCtrlK = 0;
	show_top();
}

/* ctrl Q keys */
void ctrlq_key(int key)
{
	switch(key | 0x60) {
		case 'a': goto_replace(); break;	/* ^Q^A replace */
#ifdef JUSTIFY
		case 'b': block_format(1); break; 	/* ^Q^B right justify */
#endif
		case 'c': y = screen_height-1; goto_ptr(file_end); break;	/* ^Q^C eof */
		case 'd': goto_x(strlen(line_start+1)+1); break;	/* ^Q^D eol */
		case 'f': goto_search(0); break;	/* ^Q^F find */
		case 'i': goto_line(); break;		/* ^Q^I goto line */
		case 'l': chg_case(0); break; 		/* ^Q^L lower case block */
		case 'm': window_size(); break; 	/* ^Q^M get right margin */
		case 'p': goto_last(); break;		/* ^Q^P goto last pos */
		case 'r': top();					/* ^Q^R bof */
		case 's': goto_x(1); break; 		/* ^Q^S bol */
		case 'u': chg_case(1); break; 		/* ^Q^L upper case block */
		case 'y': key_delword(1); break;	/* ^Q^Y delete to end of line */
	}
	doCtrlQ = 0;
	show_top();
}

/* Control keys */
void key_control(int key)
{
	switch(key|0x60) {
		case 'a': word_left(); break;			/* word left */
		case 'b': block_format(0); break;		/* justify block */
		case 'c': cursor_pagedown(); break; 	/* pgdn */
		case 'd': cursor_right(); break;		/* right */
		case 'e': cursor_up(); break;			/* up */
		case 'f': word_right(); break;			/* word right */
		case 'g': key_delete(); break;			/* delete cursor char */
		case 'h': key_backspace(); break;		/* destructive backspace */
		case 'i': key_tab(0); break;			/* insert tab char */
		case 'j': key_return(); break;		/* newline */
		case 'k': doCtrlK = key; break; 		/* ^K key */
		case 'l': goto_find(cur_pos, 0); break; /* find again */
		case 'm': 
		case 'n': key_return(); break;			/* newline at cursor */
		case 'o': show_mode(); break;			/* change modes */
		case 'p': literal = 1; break;			/* get inline literal */
		case 'q': doCtrlQ = key; break; 		/* ^Q key */
		case 'r': cursor_pageup(); break;		/* pgup */
		case 's': cursor_left(); break; 		/* left */
		case 't': key_delword(0); break;		/* delete word/to word */
		case 'u': undo(); break;				/* undo */
		case 'v': show_flag(OVR,!flag[OVR]);
				  break;						/* toggle Insert mode */
		case 'w': scroll_down(); break; 		/* scroll down */
		case 'x': cursor_down(); break; 		/* down */
		case 'y': block_line(); break;			/* delete line */
		case 'z': scroll_up(); break;			/* scroll up */
	}
	show_top();
}

/* main function */
void main_meta(int key)
{
    if (key>=1 && key<=10)
    switch(key) {
		case 1: show_help(0); break;			/* F1 show help */
		case 2: do_save(); break; 			/* F2 save file and resume */
		case 3: file_save(0,1); break;			/* F3 open new file */
		case 4: find_match(); break; 			/* F4 find {} () [] match  */
		case 5: run(); break; 				/* F5 get and run cmd */
		case 6: chgdir(); break; 			/* F6 get & change to dir */
		case 7: block_mark(); break;			/* F7 set mark block on */
		case 8: block_mark(); break;			/* F8 set mark block on */
#ifndef MINIMAL
		case 9: req_edit(); break;			/* F9 open new file in new window */
		case 10: key_binding('z'); break;			/* F10 open rxvt in cur.dir */
#endif
	}
    else
    switch(key | 0x60) {
        case 'b': block_mark(); break;          /* F7 set mark block on */
        case 'c': block_mark(); break;          /* F8 set mark block on */
        case 'd': goto_x(strlen(line_start+1)+1); break;/* End eol */
        case 'f': file_save(0,-1); break;       /* F3 open new file */
        case 'g': key_delete(); break;          /* Del delete cursor char */
        case 'n': show_flag(OVR, !flag[OVR]); break;/* Ins toggle insert mode */
        case 'r': do_save(); break;             /* F2 save file and resume */
        case 's': goto_x(1); break;             /* Home bol */
        case 'v': cursor_pageup(); break;       /* PgUp */
        case 'w': cursor_pagedown(); break;     /* PgDn */
//		case 'z': key_shell(0); break;          /* F10 execute a shell */
    }
    funckey=0;
    show_top();
}

/* single char interpreter */
void main_exec(int key)
{
	cur_pos = get_cur();
	if(update_scr) {
		if(flag[REC]) rec_macro(key);
	}
	if(help_done){
		help_done = 0;
		flag[SHW] = 1;
	} else if (literal) {
		key_normal(key);
		literal = 0;
	} else {
		if (funckey) main_meta(key); else
		if (doEscap==2) key_binding(key); else
		if (doCtrlK) ctrlk_key(key); else
		if (doCtrlQ) ctrlq_key(key); else {
			if(key >= BLNK)  key_normal(key);
			else key_control(key);
		}
	}
	if(!doCtrlK && !doCtrlQ && old_pos != line_start) {
		last_pos = old_pos;
		old_pos = line_start;
	}
	cur_pos = get_cur();

	if(update_scr && exitf && executive == MAIN) scr_update();
}

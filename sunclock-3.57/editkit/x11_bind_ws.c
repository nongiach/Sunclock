
/*
	edx (EDitor for X), (C) 2002, Terry Loveall
	released into the public domain.
	Based upon the original work ee.c of Yijun Ding, copyright 1991 which is
	in the public domain.
	This program comes with no warranties or binaries. Use at your own risk.
*/

#define EDIT "edx"

#ifndef MINIMAL
#define DEFAULT_RC "edxrc"
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
		case 'c': set_selection();			/* ^K^C block copy to X clipboard */
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
		case 't': transpose(); break;		/* ^Q^T transpose chars */
		case 'u': chg_case(1); break; 		/* ^Q^L upper case block */
		case 'x': switch_marks(); break;	/* ^Q^X switch marks */

		case 'y': key_delword(1); break;	/* ^Q^Y delete to end of line */
	}
	doCtrlQ = 0;
	show_top();
}

/* Shift, control and bare Function keys */
void key_func(int key)
{
	if(keve->state & ShiftMask)
		switch(key) {
		case XK_Insert	: do_paste(); return; 			/* Shift Ins paste */
		case XK_Delete	: do_select(1); return;			/* Shift Del cut */
		}
	if((keve->state & ShiftMask) && !flag[BLK] &&
		key >= XK_Home && key <= XK_End) block_mark();	/* enable marked block with Shift */
	if(keve->state & ControlMask) {
		switch(key) {
		case XK_F1		: word_mark(); break;			/* ^F1 mark cursor word */
		case XK_F3		: new_edit(""); break;			/* ^F3 open new edx */
		case XK_Home	: top(); break; 				/* ^Home bof */
		case XK_End 	: goto_y(ytot); break;			/* ^End eof */
		case XK_Left	: word_left(); break;			/* ^Left word left */
		case XK_Right	: word_right(); break;			/* ^Right word right */
		case XK_Insert	: do_select(0); break;			/* ^Ins copy */
		}
	} else
		switch(key) {
		case XK_F1		: show_help(0); break;			/* F1 show help */
		case XK_F2		: do_save(); break; 			/* F2 save file and resume */
		case XK_F3		: file_save(0,1); break;		/* F3 open new file */
		case XK_F4		: find_match(); break; 			/* F4 find {} () [] match  */
		case XK_F5		: run(); break; 				/* F5 get and run cmd */
		case XK_F6		: chgdir(); break; 				/* F6 get & change to dir */
		case XK_F7		: block_mark(); break;			/* F7 set mark block on */
		case XK_F8		: block_mark(); break;			/* F8 set mark block on */
#ifdef MINIMAL
		case XK_F10 	: mterm(); break;				/* F10 open rxvt in cur.dir */
#else
		case XK_F9		: req_edit(); break;		/* F9 open new file in new window */
		case XK_F10 	: key_binding('z'); break;				/* F10 open rxvt in cur.dir */
#endif
		case XK_F12 	: show_flag(OVR, !flag[OVR]); break;/* Ins toggle insert mode */
		case XK_Return	: key_return(); break;			/* Enter newline at cursor */
		case XK_Tab 	: key_tab(0); break;			/* Tab insert tab char at cursor */
		case XK_BackSpace: key_backspace(); break;		/* BS delete prev char */
		case XK_Insert	: show_flag(OVR, !flag[OVR]); break;/* Ins toggle insert mode */
		case XK_Delete	: key_delete(); break;			/* Del delete cursor char */
		case XK_Page_Up : cursor_pageup(); break;		/* PgUp */
		case XK_Page_Down: cursor_pagedown(); break;	/* PgDn */
		case XK_End 	: goto_x(strlen(line_start+1)+1); break;/* End eol */
		case XK_Home	: goto_x(1); break; 			/* Home bol */
		case XK_Up		: cursor_up(); break;			/* up */
		case XK_Down	: cursor_down(); break; 		/* down */
		case XK_Right	: cursor_right(); break;		/* right */
		case XK_Left	: cursor_left(); break; 		/* left */
	}
	if(!(keve->state & ShiftMask) && flag[BLK]) mark_off();	 // turn off marked block
}

/* Control keys */
void key_control(int key)
{
	if(!(keve->state & ShiftMask) && flag[BLK]) mark_off();
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
		case 'j': keve->state & ControlMask ?
					goto_line() :				/* goto line# */
					key_return(); break;		/* newline */
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
		case 'u': undo(0); break;				/* undo */
		case 'v': show_flag(OVR,!flag[OVR]);
				  break;						/* toggle Insert mode */
		case 'w': scroll_down(); break; 		/* scroll down */
		case 'x': cursor_down(); break; 		/* down */
		case 'y': block_line(); break;			/* delete line */
		case 'z': scroll_up(); break;			/* scroll up */
	}
	show_top();
}

/* single char interpreter */
void main_exec(int key)
{
	cur_pos = get_cur();
	if(update_scr) {
		undraw_cursor();
		if(flag[REC]) rec_macro(key);
	}
	if(help_done){
		help_done = 0;
		flag[SHW] = 1;
	} else if(literal) {
		key_normal(keve->state & ControlMask ? key & 0x1f : key);
		literal = 0;
	} else {
		if(key & 0xFF00) key_func(key); else
		if(keve->state & Mod1Mask) key_binding(key);
		else {
#ifdef GREEK
			/* Ctrl-C is enabled here by ^K^G */
			if(doCtrlC) ctrlc_key(key); else
#endif /* GREEK */
			if(doCtrlK) ctrlk_key(key); else
			if(doCtrlQ) ctrlq_key(key); else {
				if(keve->state & ControlMask) key &= 0x1f;
				if(key >= BLNK)  key_normal(key);
				else key_control(key);
			}
		}
	}
	if(!doCtrlK && !doCtrlQ && old_pos != line_start) {
		last_pos = old_pos;
		old_pos = line_start;
	}
	cur_pos = get_cur();

	if(update_scr) scr_update();
}

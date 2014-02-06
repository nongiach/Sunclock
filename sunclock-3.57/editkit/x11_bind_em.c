
/*
	emx (small EMacs clone for X)
	(C) 2002, Jean-Pierre Demailly
	released in the public domain

	derived from edx ver. 0.56, (C) 2002, Terry Loveall
	Based upon the original work ee.c of Yijun Ding, (C) 1991 which is
	in the public domain.

	This program comes with no warranties. Use at your own risk.
*/

#define EDIT "emx"

#ifndef MINIMAL
#define DEFAULT_RC "emxrc"
#endif

/* Help screen -- rewrite to suit your own tastes */

/* include the editor engine */
#include "edit.c"

/* All this stuff consists of the emacs-like bindings */

#ifdef GREEK
void ctrlc_key(int key)
{
	key_greek(key);
	doCtrlC = var_greek;
}
#endif

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

/* ctrl X keys */
void ctrlx_key(int key)
{
	switch(key | 0x60) {
		case 'c': sys_exit(0); break;		/* ^XC exit only if file saved */
		case 'd': key_delword(0); break;	/* ^XD delete word */
		case 'f': file_save(0,1); break;	/* ^XF open new file */
		case 'h': show_help(0); break;		/* ^XH show help menu */
		case 'i': block_read(); break;		/* ^XI read file into cursor pos */
		case 'k': block_line(); break;		/* ^XK delete line */
		case 'm': key_macros(1); break; 	/* ^XM record macro */
		case 'p': key_macros(0); break; 	/* ^XP play macro */
		case 'q': sys_exit(2); break;		/* ^XQ exit even if file unsaved */
		case 's': do_save(); break; 		/* ^XS write block to disk */
		case 't': tab_size(); break;		/* ^XT get tab size */
		case 'v': block_write(); break; 	/* ^XV write block to disk */
		case 'w': file_save(0,-1); break;	/* ^XW save file as */
		case 'x': goto_last(); break;		/* ^XX goto last pos */
	}
	doCtrlX = 0;
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
		case XK_Tab		: word_mark(); break;			/* ^Tab mark cursor word */
#ifndef MINIMAL
		case XK_F1		: new_edit(DOCDIR"/MANUAL.emx"); break;			/* ^F1 open emx Manual */
#endif
		case XK_F9		: new_edit(""); break;			/* ^F3 open new emx */
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
#ifndef MINIMAL
		case XK_F9		: req_edit(); break;		/* F9 open new file in new window */
#endif
		case XK_F10 	: key_binding('z'); break;				/* F10 open rxvt in cur.dir */
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
		case XK_Up	: cursor_up(); break;			/* up */
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
		case 'a': goto_x(1); break; 		/* bol */
		case 'b': cursor_left(); break; 	/* left */
#ifdef GREEK
		case 'c': doCtrlC = 1; break; 		/* ^C switch */
#endif
		case 'd': key_delete(); break;		/* delete cursor char */
		case 'e': goto_x(strlen(line_start+1)+1); break;	/* goto eol */
		case 'f': cursor_right(); break;	/* right */
		case 'h': key_backspace(); break;	/* destructive backspace */
		case 'j': key_return(); break;		/* newline at cursor */
		case 'k': key_delword(1); break;	/* delete to eol */
		case 'l': expose=1; scr_update(); expose=0; break;		/* refresh screen */
		case 'n': cursor_down(); break; 	/* down */
		case 'o': show_mode(); break; 		/* change modes */
		case 'p': cursor_up(); break;		/* up */
		case 'q': literal = 1; break;		/* literal */
		case 'r': goto_search(1); break;	/* ^R find backward */
		case 's': goto_search(0); break;	/* ^S find foreword */
		case 't': transpose(); break;		/* ^T transpose */
		case 'u': if (search_mode<=1)		/* repeat search */
				goto_find(cur_pos, search_mode); 
			  else				/* repeat replace */
				ask_replace();
			  break; /* search or replace again */
		case 'v': cursor_pagedown(); break; 	/* ^V pgdn */
		case 'w': reset_mark();
                          block_copy(1); break; 	/* ^W cut block at buffer */
		case 'x': doCtrlX = 1; break;		/* ^X switch */
		case 'y': block_paste(); break; 	/* ^Y copy buffer at cursor */
		case 'z': iconify(); break;		/* ^Z iconify window */
		case 0x7f: undo(0); break;		/* ^_ undo */
							/* user typed Ctrl-_ */
	}
	show_top();
}

/* Escape keys */
void key_escape(int key)
{
	switch(tolower(key)) {
		case ' ':
		case '.': block_mark(); break;		/* Esc-BLANK set mark block on */
#ifndef MINIMAL
		case '!': doEscap = 2; show_note(ALT_KEY_BINDINGS); return;	/* Esc-! = Alt-key bindings */
#endif
		case '_': redo(); break;		/* Esc-_ redo */
		case '%': goto_replace(); break;	/* Esc-% replace */

		case '<': top(); break;			/* Esc-< bof */
		case '>': y = screen_height-1; goto_ptr(file_end); break; 	/* Esc-> eof */
		case 'b': word_left(); break;		/* Esc-B word left */
		case 'f': word_right(); break;		/* Esc-F word right */
		case 'g': keve->state & ControlMask ?
					goto_line() :	/* goto line# */
					key_return(); break;	/* newline */
#ifdef JUSTIFY
		case 'j': block_format(1); break;	/* Esc-J reformat & fill block */
#endif
		case 'l': chg_case(0); break; 		/* Esc-L lower case block */
		case 'm': window_size(); break; 	/* Esc-M get right margin */
                case 'n': scroll_up(); break;		/* Esc-N scroll down (!) */
		case 'p': scroll_down(); break;		/* Esc-P scroll up (!) */
		case 'q': block_format(0); break;	/* Esc-Q reformat block */
		case 'u': chg_case(1); break; 		/* Esc-U upper case block */
		case 'v': cursor_pageup(); break;	/* Esc-V pgup */

		case 'w': reset_mark(); block_copy(0); 
                          reset_mark(); break; 		/* Esc-W copy block to buffer */
		case 'x': switch_marks(); break;	/* Esc-X switch between block marks */
	}
	doEscap = 0;
	show_top();
}

/* single char interpreter */
void main_exec(int key)
{
	cur_pos = get_cur();
	if (update_scr) {
		undraw_cursor();
		if(flag[REC]) rec_macro(key);
	}
	if (help_done){
		help_done = 0;
		flag[SHW] = 1;
	} else if (literal) {
		key_normal(keve->state & ControlMask ? key & 0x1f : key);
		literal = 0;
	} else {
		if (key==XK_Escape) doEscap = 1; else
		if (key & 0xFF00) key_func(key); else
		if (doEscap==2 || (keve->state & Mod1Mask)) key_binding(key);
		else {
			if (doEscap==1) key_escape(key); else
#ifdef GREEK
			if (doCtrlC) ctrlc_key(key); else
#endif
			if (doCtrlX) ctrlx_key(key); else 
			{
				if (keve->state & ControlMask) key &= 0x1f;
				if (key >= BLNK)  {
					flag[BLK] = 0;
					key_normal(key);
				}
				else key_control(key);
			}
		}
	}
	if(!doCtrlX && old_pos != line_start) {
		last_pos = old_pos;
		old_pos = line_start;
	}
	cur_pos = get_cur();

	if(update_scr) scr_update();
}


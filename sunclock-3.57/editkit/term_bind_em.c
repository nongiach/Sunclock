
/*
	emx (small EMacs clone for X)
	(C) 2002, Jean-Pierre Demailly
	released in the public domain

	derived from edx ver. 0.56, (C) 2002, Terry Loveall
	Based upon the original work ee.c of Yijun Ding, (C) 1991 which is
	in the public domain.

	This program comes with no warranties. Use at your own risk.
*/

#define EDIT "xterm -cr green -ms red -fn 9x15 -geometry 80x32 -e emt"

#ifndef MINIMAL
#define DEFAULT_RC "emtrc"
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

void main_meta(int key)
{
    if (key>=1 && key<=10)
    switch(key) {
		case 1: show_help(0); break;			/* F1 show help */
		case 2: do_save(); break; 			/* F2 save file and resume */
		case 3: file_save(0,1); break;			/* F3 open new file */
		case 4: find_match(); break; 			/* F4 find {} () [] match  */
		case 5: run(); break;	 			/* F5 get and run cmd */
		case 6: chgdir(); break; 			/* F6 get & change to dir */
		case 7: block_mark(); break;			/* F7 set mark block on */
		case 8: block_mark(); break;			/* F8 set mark block on */
#ifndef MINIMAL
		case 9: req_edit(); break;			/* F9 open new file in new window */
#endif
		case 10: key_binding('z'); break;			/* F10 open rxvt in cur.dir */
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

/* Control keys */
void key_control(int key)
{
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
		case 'l': scr_update(); break;		/* refresh screen */
		case 'n': cursor_down(); break; 	/* down */
		case 'o': show_mode(); break; 		/* change modes */
		case 'p': cursor_up(); break;		/* up */
		case 'q': literal = 1; break;		/* literal */
		case 'r': goto_search(1); break;	/* ^R find backward */
		case 's': goto_search(0); break;	/* ^S find */
		case 't': transpose(); break;		/* ^T transpose */
		/*
		case 'u': block_line(); break; */	/* ^U delete line */
		case 'u': if (search_mode<=1)		/* repeat search */
				goto_find(cur_pos, search_mode); 
			  else				/* repeat replace */
				ask_replace();
			  break; /* search or replace again */
		case 'v': cursor_pagedown(); break; 	/* ^V pgdn */
		case 'x': doCtrlX = 1; break;		/* ^X switch */
		case 'y': block_paste(); break; 	/* ^Y copy buffer at cursor */
		case 'w': reset_mark();
                          block_copy(1); 
			  mark_off(); break;	 	/* ^W cut block at buffer */
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
		case '_': redo(); break;		/* Esc-_ redo */
		case '%': goto_replace(); break;	/* Esc-% replace */

		case '<': top(); break;			/* Esc-< bof */
		case '>': y = screen_height-1; goto_ptr(file_end); break; 	/* Esc-> eof */
		case 'b': word_left(); break;		/* Esc-B word left */
		case 'f': word_right(); break;		/* Esc-F word right */
		case 'g': goto_line(); break;		/* goto line# */
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
                          mark_off(); break; 		/* Esc-W copy block to buffer */
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
		if(flag[REC]) rec_macro(key);
	}
	if (help_done){
 		clrscr();
		show_top();
		help_done = 0;
		flag[SHW] = 1;
	} else if (literal) {
		key_normal(key);
		literal = 0;
	} else {
		if (funckey) main_meta(key); 
		else {
			if (doEscap==1) key_escape(key); else
			if (doEscap==2) key_binding(key); else
#ifdef GREEK
			if (doCtrlC) ctrlc_key(key); else
#endif
			if (doCtrlX) ctrlx_key(key); else 
			{
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

	if(update_scr && exitf && executive == MAIN) scr_update();
}


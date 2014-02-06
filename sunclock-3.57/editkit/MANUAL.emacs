------------------------------------------------------------------------------
Emx User Manual:

Copyright (C) 2002, Jean-Pierre Demailly <demailly@ujf-grenoble.fr>

Based upon the original package edx-0.56 by Terry Loveall, itself derived
from ee.c by Yijun Ding, copyright 1991.

The original work is source in the public domain and so is emx. 

------------------------------------------------------------------------------
Emx's main goal is to provide an emacs-like editor for the X Window system,
with a very small memory footprint. It uses only the libc and the basic
X routines from the X11 library. Its simple internal structure should make
it easy to adapt the code for any use. Emx version 0.60+ automatically
reallocs memory for the buffer it uses, hence the size of files and blocks
it can handle are limited only by the available RAM and swap (there's 
also a theoretical limit of about 1GByte on 32bit architectures, but this
has not been checked...).

Ems doesn't have menus, just an internal help window available through
the F1 key or the ^XH control sequence (Ctrl-X H)

All sequences Alt-? (Alt-A to Alt-Z) are free and available for private 
key bindings. They should be defined either in the system wide emxrc file 
or in the private ~/.emxrc file (check rc.example to see how this works).
The -rc option allows to load an arbitrary rcfile instead.

Command line options
====================

Usage: emx [-fn font] [-jump line#] [-tab skip] [-w width] [-h height]
           [-bg color] [-fg color] [-hibg color] [-hifg color] [-cr color]
           [-rc rcfile] [file]

Function key commands
=====================
F1 show help      F2  save          F3 open new file  F5 exec command
F6 Chgdir         F9  open file and fire new editor   F10 rxvt          


Special key commands
====================
Shift-Del cut     Shift-Ins paste   ^Ins copy block to X clipboard

Control key commands
====================

^A  goto bol      ^E  goto eol      ^G  goto line     ^O? switch a flag
^B  left          ^F  right         ^N  down          ^P  up
^D  del char      ^H  del prev char ^J  insert line   ^K  del to eol
^Q? literal char  ^S ^R find string ^U  repeat SAR    ^T  transpose
^V  page down     ^W  cut block     ^Y  paste block   ^_  undo

^L is used to force a screen update (redisplay)

Escape-? key commands
=====================

E B word left     E F word right    E N scroll down   E P scroll up
E < start of file E > end of file   E _ redo          E % find & replace
E W copy block    E V page up       E . set mark      E X switch marks
E L lowcase block E U upcase block  E J fill paragr   E Q format paragr

Ctrl-X  key commands
====================

^XC (^XQ) exit    ^XH show help     ^XS save          ^XF open new file
^XI insert file   ^XV write block   ^XW save as       ^XD del word
^XM togl rec mac  ^XP play macro    ^XT get tab size  ^XK delete line


Function keys
=============

   (key)     (description)                                   (same as)
----------------------------------------------------------------------
    F1      help                                                ^XH
    F2      file save (if modified)                             ^XS
    F3      open new file (prompts to save if file modified)    ^XF
    F4      search for matching parentheses (){}[]
    F5      prompt for and execute user command line
    F6      get and change directory
    F7      toggle mark block                                   Esc-.
    F8      toggle mark block                                   Esc-.
    F9      fire up a new editor with a given file name
    F10     open an rxvt terminal in the current directory      Alt-Z
    Ins     toggle insert/overwrite
shf-Del     cut marked block to X clipboard
shf-Ins     paste from X clipboard
ctl-Ins     copy marked block to X clipboard
    Del     delete character under cursor or marked block       ^D
    Home    move cursor to beginning of line                    ^A
    End     move cursor to end of line                          ^E
    PgUp    move up one screen                                  Esc-V
    PgDn    move down one screen                                ^V

Navigation keys (arrows, Home, End, PgUp and PgDn) are operational.

Shift navigation marks text. 

^Home and ^End go to BOF and EOF, respectively.

Control left arrow and right arrow move by word.

Left mouse button click: set text cursor.
Right mouse button click: set text cursor or scroll text.

Left mouse button double click: select word under cursor.

Left mouse button click and drag: 
     marks and copies a text block to X clipboard.

Middle mouse button click: paste from X clipboard.

Middle mouse button drag: marks a text block.


Modes and flags:
================

Changing modes of operation is performed by ^O followed by one of the
displayed upper/lower case characters MFOCTBA. This will toggle the specific
flag. Modes are indicated as being on by displaying their upper case
character. The file modified M flag can be toggled off explicitly. The block
mark B active flag indicates a complex state. Toggle it off with the block
mark key sequences, NOT with ^OB.

M : file modified               set by anything that modifies file.
F : word wrap at text entry     toggle with ^OF
O : overwrite                   toggle with ^OO
C : search is case sensitive    toggle with ^OC
T : expand/compress tabs        toggle with ^OT
B : block mark active           toggle with ^OB
A : replace all occurences flag toggle with ^OA

The editor does display tab chars as multiple spaces. Tab (0x09) chars are
displayed as tabsize spaces. Default tab size is 4. To change tab-width to 8
the command line is  'emx -tab 8'. To change from within the editor use ^XT.

To go to a specified line on initial file opening, the command line is
'emx -jump 507 somefile'. The minus plus sequence is required. Input a 
^G to go to a line from within emx.

Turning on (F)ill mode enables wordwrap during text entry. Block reformat
wraps the text at the right screen edge until a double newline is encounterd.
To reformat a paragraph, place the cursor at the desired point of reformat and
enter a Esc-Q. To change the right margin use Esc-M.

As noted, undo and redo are available. Ctrl-_ for undo, Esc-_ for redo. 
A complete record of the edit session is maintained. Undoing all actions 
in the undo buffer will reset the Marked flag.

X clipboard is somewhat integrated. Cut, copy and paste with respect to 
the X clipboard are obtained by Shift-Del, Control-Insert and Shift-Insert, 
respectively (Control-Insert copies the marked block to the X clipboard 
so that the middle mouse button can then be used to paste it to other 
applications which support pasting through this procedure). ^W deletes the
marked block (and copies the deleted marked block to the block buffer), while
^Y copies the deleted block from the block buffer to the point of the text
cursor. 

Find and 'Search and Replace' will pick up any marked blocks, text under
the cursor or user input in that order. Found text is highlighted. Set replace
ALL flag wth ^OA option before running SAR to replace all occurences. 

For general dialog entry if the first character entered is not ^H, ^C, End,
Esc or Enter, the dialog string is discarded. End moves the cursor to the end
of the dialog string.

For a complete understanding of the operation of emx, study the code. It is the
ultimate authority on operation.

Remember, when all else fails READ THE SCREEN.
------------------------------------------------------------------------------

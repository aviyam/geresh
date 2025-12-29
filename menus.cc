// Copyright (C) 2003 Mooffie <mooffie@typo.co.il>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111, USA.

#include <config.h>

#if HAVE_DIRENT_H
# include <dirent.h> // for population the 'color scheme' sub menu.
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <algorithm> // sort

#include "geresh_io.h"     // get_cfg_filename
#include "pathnames.h"
#include "menus.h"

#define STT_AUTOINDENT	    1001
#define STT_AUTOJUSTIFY	    1002
#define STT_ALTKBD	    1003
#define STT_SMRT	    1004
#define STT_ARABICSHAPING   1005
#define STT_FORMATMARKS	    1006
#define STT_CURSORREPORT    1007
#define STT_SPELLERLOADED   1008
#define STT_READONLY	    1009
#define STT_BIGCURSOR	    1010
#define STT_GRAPHBOXES	    1011
#define STT_KEYFORKEYUNDO   1012
#define STT_BIDI	    1013
#define STT_UNDERLINE	    1014
#define STT_SYNAUTO	    1015

#define STT_EOPUNIX	    5001
#define STT_EOPDOS	    5002
#define STT_EOPMAC	    5003
#define STT_EOPUNICODE	    5004

#define STT_RTLNSMASCII	    5010
#define STT_RTLNSMASIS	    5011
#define STT_RTLNSMHIDE	    5012

#define STT_MQFASCII	    5020
#define STT_MQFHIGHLIGHT    5021
#define STT_MQFASIS	    5022

#define STT_WRAPWSPACE	    5030
#define STT_WRAPBREAK	    5031
#define STT_WRAPOFF	    5032

#define STT_DIRALGOUNICODE  5040
#define STT_DIRALGOCTXSTRNG 5041
#define STT_DIRALGOCTXRTL   5042
#define STT_DIRALGOFLTR	    5043
#define STT_DIRALGOFRTL	    5044

#define STT_SCRLBRNONE	    5060
#define STT_SCRLBRLEFT	    5061
#define STT_SCRLBRRIGHT	    5062

#define STT_SYNNONE	    5070
#define STT_SYNHTML	    5071
#define STT_SYNEMAIL	    5072

#define STT_CRSVIS	    5080
#define STT_CRSLOG	    5081

#define STT_THEME_START	    6000
#define STT_THEME_END	    6999

#define CMD_CHR		    100
#define CMD_CRSVIS	    101
#define CMD_CRSLOG	    102
#define CMD_SETTHEME	    103
#define CMD_SETFLAG	    104
#define CMD_ENC		    105
#define CMD_INTERACTIVEENC  106

#define CMD_HELP	    120

#define FLAG_FILE_ENCODING	1
#define FLAG_DEFAULT_ENCODING	2

#define HELP_ITEM   N_("Get a detailed explanation for the above")

#define HELP_TOPIC_ALGO_STR	"\xD7\x90\xD7\x9C\xD7\x92\xD7\x95\xD7\xA8\xD7\x99\xD7\xAA\xD7\x9D\x20\xD7\x9C\xD7\xA7\xD7\x91\xD7\x99\xD7\xA2\xD7"
#define HELP_TOPIC_POINTS_STR	"\xD7\xA0\xD7\x99\xD7\xA7\xD7\x95\xD7\x93\x20\xD7\xA2\xD7\x91\xD7\xA8\xD7\x99"
#define HELP_TOPIC_MAQAF_STR	"\xD7\x9E\xD7\xA7\xD7\xA3"
#define HELP_TOPIC_EOL_STR	"\xD7\xA1\xD7\x95\xD7\xA4\xD7\x99\x20\xD7\xA9\xD7\x95\xD7\xA8\xD7\x95\xD7\xAA"
#define HELP_TOPIC_CRSLV_STR	"\xD7\xAA\xD7\xA0\xD7\x95\xD7\xA2\xD7\xAA\x20\xD7\xA1\xD7\x9E\xD7\x9F"
#define HELP_TOPIC_SYNHLT_STR	"\xD7\xA6\xD7\x91\xD7\x99\xD7\xA2\xD7\x94\x20\xD7\xAA\xD7\x97\xD7\x91\xD7\x99\xD7\xA8\xD7\x99\xD7\xAA"
#define HELP_TOPIC_WRAP_STR	"\xD7\xA7\xD7\x99\xD7\xA4\xD7\x95\xD7\x9C\x20\xD7\xA4\xD7\xA1\xD7\xA7\xD7\x90\xD7\x95\xD7\xAA"
#define HELP_TOPIC_COLORS_STR	"\xD7\xA2\xD7\xA8\xD7\x9B\xD7\x95\xD7\xAA\x20\xD7\xA6\xD7\x91\xD7\xA2\xD7\x99\xD7\x9D"
#define HELP_TOPIC_ENCODING_STR "\xD7\xA7\xD7\x99\xD7\x93\xD7\x95\xD7\x93\x20\xD7\x94\xD7\xA7\xD7\x91\xD7\xA6\xD7\x99\xD7\x9D"
#define HELP_TOPIC_EXTED_STR	"\xD7\x94\xD7\xA4\xD7\xA2\xD7\x9C\xD7\xAA\x20\xD7\xA2\xD7\x95\xD7\xA8\xD7\x9A\x20\xD7\x97\xD7\x99\xD7\xA6\xD7\x95\xD7\xA0\xD7\x99"

PulldownMenu FileMenu = {
    { "load_file", N_("~Open...") },
    { "insert_file",  N_("~Insert file...") },
    { "-----------" },
    { "save_file", N_("~Save") },
    { "save_file_as", N_("Save ~As...") },
    { "write_selection_to_file", N_("~Write selection to...") },
    { "-----------" },
    { "external_edit_prompt",    N_("Launch external ~editor...") },
    { "external_edit_no_prompt", N_("Launch external editor") },
    { "xxx", HELP_ITEM, 0, 0, CMD_HELP, 0, HELP_TOPIC_EXTED_STR },
    { "-----------" },
    { "change_directory", N_("Ch~dir...") },
    { "quit", N_("~Quit") },
    { NULL }
};

PulldownMenu MovementMenu = {
    { "move_beginning_of_buffer", N_("Move to the ~beginning of the buffer") },
    { "move_end_of_buffer", N_("Move to the ~end of the buffer") },
    { "go_to_line", N_("Go to a specific ~line") },
    { "move_last_modification", N_("Jump to the last ~modification point") },
    { "-----------" },
    { "key_left", N_("Move a character left") },
    { "key_right", N_("Move a character right") },
    { "move_previous_line", N_("Move to the previous line") },
    { "move_next_line", N_("Move to the next line") },
    { "move_backward_page", N_("Move to the previous page") },
    { "move_forward_page", N_("Move to the next page") },
    { "move_backward_char", N_("Move back a character") },
    { "move_forward_char", N_("Move forward a character") },
    //{ "move_beginning_of_line", N_("Move to the beginning of the current line") },
    { "key_home", N_("Move to the beginning of the current line") },
    { "move_end_of_line", N_("Move to the end of the current line") },
    { NULL }
};

PulldownMenu DeletionMenu = {
    { "delete_backward_char", N_("Delete the previous character") },
    { "delete_forward_char", N_("Delete the character the cursor is on") },
    { "delete_backward_word", N_("Delete to the start of the current or previous word") },
    { "delete_forward_word", N_("Delete to the end of the current or next word") },
    { "delete_paragraph", N_("Delete the current paragraph") },
    { "cut_end_of_paragraph", N_("Cut [to] end of paragraph") },
    { NULL }
};

PulldownMenu EditMenu = {
    { "undo", N_("~Undo") },
    { "redo", N_("~Redo") },
    { "toggle_key_for_key_undo", N_("Toggle ~key-for-key undo"), STT_KEYFORKEYUNDO },
    { "-----------" },
    { "copy", N_("~Copy") },
    { "cut",  N_("Cu~t") },
    { "paste",N_("~Paste") },
    { "toggle_primary_mark", N_("Start/cancel selection") },
    { "-----------" },
    { "", N_("~Movement"), 0, MovementMenu },
    { "", N_("~Deletion"), 0, DeletionMenu },
    { "toggle_read_only", N_("Toggle read-only status of buffer"), STT_READONLY },
    { "-----------" },
    { "search_forward", N_("~Search...") },
    { "search_forward_next", N_("Search ~next") },
    { "-----------" },
    { "toggle_auto_justify", N_("Toggle auto-~justify"), STT_AUTOJUSTIFY },
    { "justify", N_("Justify the current or next paragraph") },
    { "change_justification_column", N_("Change justify column...") },
    { "-----------" },
    { "toggle_auto_indent", N_("Toggle auto-~indent"), STT_AUTOINDENT },
    { "change_tab_width", N_("Change the TAB character size...") },
    { NULL }
};

PulldownMenu EolMenu = {
    { "set_eops_unix",	    N_("~Unix (LF)"),		    STT_EOPUNIX },
    { "set_eops_dos",	    N_("~DOS/Windows (CR,LF)"),	    STT_EOPDOS },
    { "set_eops_mac",	    N_("~Macintosh (CR)"),	    STT_EOPMAC },
    { "set_eops_unicode",   N_("U~nicode PS (U+2029)"),	    STT_EOPUNICODE },
    { "-----------" },
    { "xxx", HELP_ITEM, 0, 0, CMD_HELP, 0, HELP_TOPIC_EOL_STR },
    { NULL }
};

#define ICONVREQ N_("(requires iconv)")

PulldownMenu CyrillicEncodingsMenu = {
    { "xxx", N_("(Cyrillic) ~Windows-1251"),	0, 0, CMD_ENC, 0, "CP1251", ICONVREQ },
    { "xxx", N_("(Cyrillic) ~ISO-8859-5"),	0, 0, CMD_ENC, 0, "ISO-8859-5", ICONVREQ },
    { "xxx", N_("(Cyrillic) KOI8-~R"),		0, 0, CMD_ENC, 0, "KOI8-R", ICONVREQ },
    { "xxx", N_("(Cyrillic) KOI8-~U"),		0, 0, CMD_ENC, 0, "KOI8-U", ICONVREQ },
    { "xxx", N_("(Cyrillic) IBM-866"),		0, 0, CMD_ENC, 0, "CP866", ICONVREQ },
    { "xxx", N_("(Cyrillic) IBM-855"),		0, 0, CMD_ENC, 0, "CP855", ICONVREQ },
    { "xxx", N_("(Cyrillic) ISO-IR-111"),	0, 0, CMD_ENC, 0, "ISO-IR-111", ICONVREQ },
    { "xxx", N_("(Cyrillic) ~MacCyrillic"),	0, 0, CMD_ENC, 0, "MACCYRILLIC", ICONVREQ },
    { NULL }
};

PulldownMenu UnicodeEncodingsMenu = {
    { "xxx", N_("UTF-1~6"),		    0, 0, CMD_ENC, 0, "UTF-16", ICONVREQ },
    { "xxx", N_("UTF-16 Big Endian"),	    0, 0, CMD_ENC, 0, "UTF-16BE", ICONVREQ },
    { "xxx", N_("UTF-16 Little Endian"),    0, 0, CMD_ENC, 0, "UTF-16LE", ICONVREQ },
    { "xxx", N_("UTF-~32"),		    0, 0, CMD_ENC, 0, "UTF-32", ICONVREQ },
    { "xxx", N_("UTF-32 Big Endian"),	    0, 0, CMD_ENC, 0, "UTF-32BE", ICONVREQ },
    { "xxx", N_("UTF-32 Little Endian"),    0, 0, CMD_ENC, 0, "UTF-32LE", ICONVREQ },
    { "xxx", N_("UTF-~7"),		    0, 0, CMD_ENC, 0, "UTF-7", ICONVREQ },
    { "-----------" },
    { "xxx", N_("UCS-2"),		    0, 0, CMD_ENC, 0, "UCS-2", ICONVREQ },
    { "xxx", N_("UCS-2 Big Endian"),	    0, 0, CMD_ENC, 0, "UCS-2BE", ICONVREQ },
    { "xxx", N_("UCS-2 Little Endian"),	    0, 0, CMD_ENC, 0, "UCS-2BE", ICONVREQ },
    { "xxx", N_("UCS-4"),		    0, 0, CMD_ENC, 0, "UCS-4", ICONVREQ },
    { "xxx", N_("UCS-4 Big Endian"),	    0, 0, CMD_ENC, 0, "UCS-4BE", ICONVREQ },
    { "xxx", N_("UCS-4 Little Endian"),	    0, 0, CMD_ENC, 0, "UCS-4LE", ICONVREQ },
    { NULL }
};

PulldownMenu EncodingsMenu = {
    { "xxx", N_("~UTF-8"),		    0, 0, CMD_ENC, 0, "UTF-8" },
    { "-----------" },
    { "xxx", N_("(Hebrew) ~ISO-8859-8"),    0, 0, CMD_ENC, 0, "ISO-8859-8" },
    { "xxx", N_("(Hebrew) ~Windows-1255"),  0, 0, CMD_ENC, 0, "CP1255", ICONVREQ },
    { "xxx", N_("(Hebrew) MacHebrew"),	    0, 0, CMD_ENC, 0, "MACHEBREW", ICONVREQ },
    { "xxx", N_("(Hebrew) IBM-862 (~Dos)"), 0, 0, CMD_ENC, 0, "CP862", ICONVREQ },
    { "-----------" },
    { "xxx", N_("(Arabic) ISO-8859-6"),	    0, 0, CMD_ENC, 0, "ISO-8859-6", ICONVREQ },
    { "xxx", N_("(~Arabic) Windows-1256"),  0, 0, CMD_ENC, 0, "CP1256", ICONVREQ },
    { "xxx", N_("(Arabic) MacArabic"),	    0, 0, CMD_ENC, 0, "MACARABIC", ICONVREQ },
    { "xxx", N_("(Arabic) IBM-864 (Dos)"),  0, 0, CMD_ENC, 0, "CP864", ICONVREQ },
    { "xxx", N_("(Farsi)  MacFarsi"),	    0, 0, CMD_ENC, 0, "MACFARSI", ICONVREQ },
    { "xxx", N_("~Cyrillic"), 0, CyrillicEncodingsMenu },
    { "xxx", N_("~More Unicodes"), 0, UnicodeEncodingsMenu },
    { "-----------" },
    { "xxx", N_("Other..."), 0, 0, CMD_INTERACTIVEENC },
    { NULL }
};

PulldownMenu CantillationsCharMenu = {
    { "xxx", N_("ETNAHTA         (U+0591)"), 0, 0, CMD_CHR, 0x0591 },
    { "xxx", N_("SEGOL           (U+0592)"), 0, 0, CMD_CHR, 0x0592 },
    { "xxx", N_("SHALSHELET      (U+0593)"), 0, 0, CMD_CHR, 0x0593 },
    { "xxx", N_("ZAQEF QATAN     (U+0594)"), 0, 0, CMD_CHR, 0x0594 },
    { "xxx", N_("ZAQEF GADOL     (U+0595)"), 0, 0, CMD_CHR, 0x0595 },
    { "xxx", N_("TIPEHA          (U+0596)"), 0, 0, CMD_CHR, 0x0596 },
    { "xxx", N_("REVIA           (U+0597)"), 0, 0, CMD_CHR, 0x0597 },
    { "xxx", N_("ZARQA           (U+0598)"), 0, 0, CMD_CHR, 0x0598 },
    { "xxx", N_("PASHTA          (U+0599)"), 0, 0, CMD_CHR, 0x0599 },
    { "xxx", N_("YETIV           (U+059A)"), 0, 0, CMD_CHR, 0x059A },
    { "xxx", N_("TEVIR           (U+059B)"), 0, 0, CMD_CHR, 0x059B },
    { "xxx", N_("GERESH          (U+059C)"), 0, 0, CMD_CHR, 0x059C },
    { "xxx", N_("GERESH MUQDAM   (U+059D)"), 0, 0, CMD_CHR, 0x059D },
    { "xxx", N_("GERSHAYIM       (U+059E)"), 0, 0, CMD_CHR, 0x059E },
    { "xxx", N_("QARNEY PARA     (U+059F)"), 0, 0, CMD_CHR, 0x059F },
    { "xxx", N_("TELISHA GEDOLA  (U+05A0)"), 0, 0, CMD_CHR, 0x05A0 },
    { "xxx", N_("PAZER           (U+05A1)"), 0, 0, CMD_CHR, 0x05A1 },
    { "xxx", N_("MUNAH           (U+05A3)"), 0, 0, CMD_CHR, 0x05A3 },
    { "xxx", N_("MAHAPAKH        (U+05A4)"), 0, 0, CMD_CHR, 0x05A4 },
    { "xxx", N_("MERKHA          (U+05A5)"), 0, 0, CMD_CHR, 0x05A5 },
    { "xxx", N_("MERKHA KEFULA   (U+05A6)"), 0, 0, CMD_CHR, 0x05A6 },
    { "xxx", N_("DARGA           (U+05A7)"), 0, 0, CMD_CHR, 0x05A7 },
    { "xxx", N_("QADMA           (U+05A8)"), 0, 0, CMD_CHR, 0x05A8 },
    { "xxx", N_("TELISHA QETANA  (U+05A9)"), 0, 0, CMD_CHR, 0x05A9 },
    { "xxx", N_("YERAH BEN YOMO  (U+05AA)"), 0, 0, CMD_CHR, 0x05AA },
    { "xxx", N_("OLE             (U+05AB)"), 0, 0, CMD_CHR, 0x05AB },
    { "xxx", N_("ILUY            (U+05AC)"), 0, 0, CMD_CHR, 0x05AC },
    { "xxx", N_("DEHI            (U+05AD)"), 0, 0, CMD_CHR, 0x05AD },
    { "xxx", N_("ZINOR           (U+05AE)"), 0, 0, CMD_CHR, 0x05AE },
    { "xxx", N_("MASORA CIRCLE   (U+05AF)"), 0, 0, CMD_CHR, 0x05AF },
    { "-----------" },
    { "xxx", N_("METEG           (U+05BD)"), 0, 0, CMD_CHR, UNI_HEB_METEG },
    { "xxx", N_("PASEQ           (U+05C0)"), 0, 0, CMD_CHR, UNI_HEB_PASEQ },
    { "xxx", N_("SOF PASUQ       (U+05C3)"), 0, 0, CMD_CHR, UNI_HEB_SOF_PASUQ },
    { NULL }
};

PulldownMenu ArabicCharMenu = {
    { "xxx", N_("~FATHATAN         (U+064B)"), 0, 0, CMD_CHR, 0x064B },
    { "xxx", N_("DAMMATAN         (U+064C)"), 0, 0, CMD_CHR, 0x064C },
    { "xxx", N_("~KASRATAN         (U+064D)"), 0, 0, CMD_CHR, 0x064D },
    { "xxx", N_("FATHA            (U+064E)"), 0, 0, CMD_CHR, 0x064E },
    { "xxx", N_("~DAMMA            (U+064F)"), 0, 0, CMD_CHR, 0x064F },
    { "xxx", N_("KASRA            (U+0650)"), 0, 0, CMD_CHR, 0x0650 },
    { "xxx", N_("~SHADDA           (U+0651)"), 0, 0, CMD_CHR, 0x0651 },
    { "xxx", N_("SUKUN            (U+0652)"), 0, 0, CMD_CHR, 0x0652 },
    { "xxx", N_("SUPERSCRIPT ALEF (U+0670)"), 0, 0, CMD_CHR, 0x0670 },
    { "-----------" },
    { "xxx", N_("ZWJ,  ZERO WIDTH JOINER     (U+200D)"), 0, 0, CMD_CHR, 0x200D },
    { "xxx", N_("ZWNJ, ZERO WIDTH NON-JOINER (U+200C)"), 0, 0, CMD_CHR, 0x200C },
    { NULL }
};

PulldownMenu InsertCharMenu = {
    { "insert_maqaf", N_("Hebrew ~maqaf (U+05BE)") },
    { "-----------" },
    { "xxx", N_("~LRM, LEFT-TO-RIGHT MARK         (U+200E)"), 0, 0, CMD_CHR, UNI_LRM },
    { "xxx", N_("~RLM, RIGHT-TO-LEFT MARK         (U+200F)"), 0, 0, CMD_CHR, UNI_RLM },
    { "xxx", N_("LRE, LEFT-TO-RIGHT EMBEDDING    (U+202A)"), 0, 0, CMD_CHR, UNI_LRE },
    { "xxx", N_("RLE, RIGHT-TO-LEFT EMBEDDING    (U+202B)"), 0, 0, CMD_CHR, UNI_RLE },
    { "xxx", N_("PDF, POP DIRECTIONAL FORMATTING (U+202C)"), 0, 0, CMD_CHR, UNI_PDF },
    { "xxx", N_("LRO, LEFT-TO-RIGHT OVERRIDE     (U+202D)"), 0, 0, CMD_CHR, UNI_LRO },
    { "xxx", N_("RLO, RIGHT-TO-LEFT OVERRIDE     (U+202E)"), 0, 0, CMD_CHR, UNI_RLO },
    { "-----------" },
    { "xxx", N_("~SHEVA           (U+05B0)"), 0, 0, CMD_CHR, UNI_HEB_SHEVA },
    { "xxx", N_("HATAF SEGOL     (U+05B~1)"), 0, 0, CMD_CHR, UNI_HEB_HATAF_SEGOL },
    { "xxx", N_("HATAF PATAH     (U+05B~2)"), 0, 0, CMD_CHR, UNI_HEB_HATAF_PATAH },
    { "xxx", N_("HATAF QAMATS    (U+05B~3)"), 0, 0, CMD_CHR, UNI_HEB_HATAF_QAMATS },
    { "xxx", N_("~HIRIQ           (U+05B4)"), 0, 0, CMD_CHR, UNI_HEB_HIRIQ },
    { "xxx", N_("TS~ERE           (U+05B5)"), 0, 0, CMD_CHR, UNI_HEB_TSERE },
    { "xxx", N_("SE~GOL           (U+05B6)"), 0, 0, CMD_CHR, UNI_HEB_SEGOL },
    { "xxx", N_("~PATAH           (U+05B7)"), 0, 0, CMD_CHR, UNI_HEB_PATAH },
    { "xxx", N_("~QAMATS          (U+05B8)"), 0, 0, CMD_CHR, UNI_HEB_QAMATS },
    { "xxx", N_("H~OLAM           (U+05B9)"), 0, 0, CMD_CHR, UNI_HEB_HOLAM },
    { "xxx", N_("Q~UBUTS          (U+05BB)"), 0, 0, CMD_CHR, UNI_HEB_QUBUTS },
    { "xxx", N_("~DAGESH OR MAPIQ (U+05BC)"), 0, 0, CMD_CHR, UNI_HEB_DAGESH_OR_MAPIQ },
    { "xxx", N_("~RAFE            (U+05BF)"), 0, 0, CMD_CHR, UNI_HEB_RAFE },
    { "xxx", N_("SHIN DOT        (U+05C1)"), 0, 0, CMD_CHR, UNI_HEB_SHIN_DOT },
    { "xxx", N_("SIN DOT         (U+05C2)"), 0, 0, CMD_CHR, UNI_HEB_SIN_DOT },
    { "-----------" },
    { "xxx", N_("GERESH          (U+05F3)"), 0, 0, CMD_CHR, UNI_HEB_GERESH },
    { "xxx", N_("GERSHAYIM       (U+05F4)"), 0, 0, CMD_CHR, UNI_HEB_GERSHAYIM },
    { "xxx", N_("NO-BREAK SPACE  (U+00A0)"), 0, 0, CMD_CHR, 0x00A0 },
    { "xxx", N_("ZERO WIDTH SPACE(U+200B)"), 0, 0, CMD_CHR, 0x200B },
    { "-----------" },
    { "xxx", N_("~Cantillations"), 0, CantillationsCharMenu },
    { "xxx", N_("~Arabic"), 0, ArabicCharMenu },
};

PulldownMenu CharactersUtilsMenu = {
    { "show_character_code", N_("Print ~Unicode and UTF-8 value") },
    { "show_character_info", N_("Print the corresponding Unicode~Data.txt line") },
    { "xxx", N_("~Insert a Unicode character from the list"), 0, InsertCharMenu },
    { "insert_unicode_char", N_("Insert a Unicode character using its ~code") },
    { "-----------" },
    { "xxx", N_("Set the ~encoding used for saving this file"), 0, EncodingsMenu, CMD_SETFLAG, FLAG_FILE_ENCODING },
    { "xxx", N_("Set the de~fault encoding"), 0, EncodingsMenu, CMD_SETFLAG, FLAG_DEFAULT_ENCODING, 0,
	N_("The \"default encoding\" is used when loading files and when creating new files.") },
    { "xxx", HELP_ITEM, 0, 0, CMD_HELP, 0, HELP_TOPIC_ENCODING_STR },
    { "-----------" },
    { "toggle_alt_kbd", N_("Toogle ~Hebrew keyboard emulation"), STT_ALTKBD },
    { "toggle_smart_typing", N_("Toggle ~smart-typing mode"), STT_SMRT },
    { "set_translate_next_char", N_("~Translate next character") },
    { "-----------" },
    { "toggle_eops", N_("Change end-of-~line type"), 0, EolMenu },
    { NULL }
};

PulldownMenu DirAlgoMenu = {
    { "set_dir_algo_unicode",		N_("~Unicode's TR #9"),	    STT_DIRALGOUNICODE },
    { "set_dir_algo_context_strong",	N_("Contextual-~strong"),   STT_DIRALGOCTXSTRNG },
    { "set_dir_algo_context_rtl",	N_("~Contextual-rtl"),	    STT_DIRALGOCTXRTL },
    { "set_dir_algo_force_ltr",		N_("Force ~LTR"),	    STT_DIRALGOFLTR },
    { "set_dir_algo_force_rtl",		N_("Force ~RTL"),	    STT_DIRALGOFRTL },
    { "-----------" },
    { "xxx", HELP_ITEM, 0, 0, CMD_HELP, 0, HELP_TOPIC_ALGO_STR },
    { NULL }
};

PulldownMenu CursorMovementMenu = {
    { "toggle_visual_cursor_movement",	N_("~Logical"),	STT_CRSLOG, 0, CMD_CRSLOG, 0,
		N_("Logical movement: cursor follows the buffer order") },
    { "toggle_visual_cursor_movement",	N_("~Visual"),	STT_CRSVIS, 0, CMD_CRSVIS, 0,
		N_("Visual movement: cursor follows what you see on screen") },
    { "-----------" },
    { "xxx", HELP_ITEM, 0, 0, CMD_HELP, 0, HELP_TOPIC_CRSLV_STR },
    { NULL }
};

PulldownMenu BiDiMenu = {
    { "toggle_bidi", N_("Toggle the ~BiDi algorithm"), STT_BIDI },
    { "toggle_dir_algo", N_("Select base-directionality ~algorithm"), 0, DirAlgoMenu },
    { "toggle_visual_cursor_movement", N_("~Cursor movement"), 0, CursorMovementMenu },
    { NULL }
};

PulldownMenu SpellerMenu = {
    { "spell_check_all", N_("Spell check all ~document") },
    { "spell_check_forward", N_("Spell check ~from cursor onward") },
    { "spell_check_word", N_("Spell check ~word under cursor") },
    { "-----------" },
    { "load_unload_speller", N_("Explicitly ~load/unload the speller process..."), STT_SPELLERLOADED },
    { NULL }
};

PulldownMenu WrapMenu = {
    { "set_wrap_type_at_white_space",
	    N_("Wrap lines, do not break ~words"),   STT_WRAPWSPACE },
    { "set_wrap_type_anywhere",
	    N_("Wrap lines, ~break words"),	    STT_WRAPBREAK },
    { "set_wrap_type_off",
	    N_("Do ~not wrap lines"),		    STT_WRAPOFF },
    { "-----------" },
    { "xxx", HELP_ITEM, 0, 0, CMD_HELP, 0, HELP_TOPIC_WRAP_STR },
    { NULL }
};

PulldownMenu rtlnsmMenu = {
    { "set_rtl_nsm_transliterated",
	    N_("Display Hebrew/Arabic points as highlighted ~ASCII characters"), STT_RTLNSMASCII },
    { "set_rtl_nsm_asis",
	    N_("Display Hebrew/Arabic points as-~is (for capable terminals)"), STT_RTLNSMASIS },
    { "set_rtl_nsm_off",
	    N_("~Hide Hebrew/Arabic points"), STT_RTLNSMHIDE },
    { "-----------" },
    { "xxx", HELP_ITEM, 0, 0, CMD_HELP, 0, HELP_TOPIC_POINTS_STR },
    { NULL }
};

PulldownMenu MaqafMenu = {
    { "set_maqaf_display_transliterated",
	    N_("Display the maqaf as ~ASCII dash"),	STT_MQFASCII },
    { "set_maqaf_display_asis",
	    N_("Display the maqaf as-~is (for capable terminals)"),
						STT_MQFASIS },
    { "set_maqaf_display_highlighted",
	    N_("~Highlight the maqaf"),		STT_MQFHIGHLIGHT },
    { "-----------" },
    { "xxx", HELP_ITEM, 0, 0, CMD_HELP, 0, HELP_TOPIC_MAQAF_STR },
    { NULL }
};

PulldownMenu ScrollbarMenu = {
    { "menu_set_scrollbar_none",    N_("~Off"),		STT_SCRLBRNONE },
    { "menu_set_scrollbar_left",    N_("~Left"),	STT_SCRLBRLEFT },
    { "menu_set_scrollbar_right",   N_("~Right"),	STT_SCRLBRRIGHT },
    { NULL }
};

PulldownMenu HighlightMenu = {
    { "toggle_syntax_auto_detection",	N_("Syntax ~Auto-Detection"),	STT_SYNAUTO },
    { "-----------" },
    { "menu_set_syn_hlt_none",	N_("~None"),	STT_SYNNONE },
    { "menu_set_syn_hlt_html",	N_("~HTML"),	STT_SYNHTML },
    { "menu_set_syn_hlt_email",	N_("~Email"),	STT_SYNEMAIL },
    { "-----------" },
    { "toggle_underline",	N_("Highlight *~text* and _text_"), STT_UNDERLINE },
    { "-----------" },
    { "xxx", HELP_ITEM, 0, 0, CMD_HELP, 0, HELP_TOPIC_SYNHLT_STR },
    { NULL }
};

#define MAX_COLOR_SCHEMES 100
MenuItem ColorScheme[MAX_COLOR_SCHEMES] = {
    { "set_default_theme", N_("~Default (transparent)"), STT_THEME_START, 0, 0, 0, "default.thm" },
    { "-----------" },
};

PulldownMenu DisplayMenu = {
    { "toggle_formatting_marks", N_("Toggle display of ~formatting marks"), STT_FORMATMARKS },
    { "toggle_arabic_shaping",   N_("Toggle ~Arabic shaping"), STT_ARABICSHAPING },
    { "xxx_toggle_rtl_nsm", N_("Change display of Hebrew/Arabic ~points"), 0, rtlnsmMenu },
    { "xxx_toggle_maqaf", N_("Change display of the Hebrew ~maqaf"), 0, MaqafMenu },
    { "-----------" },
    { "toggle_wrap", N_("Change ~wrap style"), 0, WrapMenu },
    { "xxx", N_("~Scrollbar"), 0, ScrollbarMenu },
    { "toggle_cursor_position_report", N_("Toggle display of cursor p~osition"), STT_CURSORREPORT },
    { "change_scroll_step", N_("Change the scroll step...") },
    { "-----------" },
    { "xxx", N_("~Color scheme"), 0, ColorScheme },
    { "-----------" },
    { "xxx", N_("Syntax ~Highlighting"), 0, HighlightMenu },
    { "-----------" },
    { "refresh_and_center", N_("~Repaint the screen and center the line") },
    { "toggle_graphical_boxes", N_("Toggle use of ~graphical characters for frames"), STT_GRAPHBOXES },
    { "toggle_big_cursor",	N_("Toggle ~big cursor"), STT_BIGCURSOR },
    { NULL }
};

PulldownMenu HelpMenu = {
    { "help",		N_("User ~Manual") },
    { "describe_key",	N_("~Describe key...") },
    { NULL }
};

MenubarMenu mainMenu = {
    { "File", FileMenu },
    { "Edit", EditMenu },
    { "Characters", CharactersUtilsMenu },
    { "Display", DisplayMenu },
    { "BiDi", BiDiMenu },
    { "Speller", SpellerMenu },
    { "Help", HelpMenu },
    { NULL }
};

////////////////////////////// MMPopupMenu /////////////////////////////////

class MMPopupMenu : public PopupMenu {
    Editor  *editor;
    EditBox *editbox;
public:
    MMPopupMenu(Editor *aEditor, EditBox *aEditbox,	    
		PopupMenu *aParent, PulldownMenu aMnu);
protected:
    virtual void show_hint(const char *hint);
    virtual void clear_other_popups();
    virtual Dispatcher *get_primary_target();
    virtual Dispatcher *get_secondary_target();
    virtual PopupMenu *create_popupmenu(PopupMenu *aParent, PulldownMenu mnu);
    virtual bool get_item_state(int id);
    virtual void do_command(unsigned long parameter1, unsigned long parameter2,
			    const char *parameter3);
};

MMPopupMenu::MMPopupMenu(Editor *aEditor, EditBox *aEditbox,	
			 PopupMenu *aParent, PulldownMenu aMnu)
    : PopupMenu(aParent, NULL)
{
    editor  = aEditor;
    editbox = aEditbox;
    init(aMnu);
}

void MMPopupMenu::show_hint(const char *hint)
{
    editor->show_hint(hint);
}

void MMPopupMenu::clear_other_popups()
{
    editor->refresh(true);
}

Dispatcher *MMPopupMenu::get_primary_target()
{
    return editor;
}

Dispatcher *MMPopupMenu::get_secondary_target()
{
    return editbox;
}

PopupMenu *MMPopupMenu::create_popupmenu(PopupMenu *aParent, PulldownMenu mnu)
{
    return new MMPopupMenu(editor, editbox, aParent, mnu);
}

bool MMPopupMenu::get_item_state(int id)
{
    switch (id) {
    case STT_AUTOINDENT:    return editbox->is_auto_indent();
    case STT_AUTOJUSTIFY:   return editbox->is_auto_justify();
    case STT_ALTKBD:	    return editbox->get_alt_kbd();
    case STT_SMRT:	    return editbox->is_smart_typing();
    case STT_ARABICSHAPING: return terminal::do_arabic_shaping;
    case STT_FORMATMARKS:   return editbox->has_formatting_marks();
    case STT_CURSORREPORT:  return editor->is_cursor_position_report();
    case STT_READONLY:	    return editbox->is_read_only();
#ifdef HAVE_CURS_SET
    case STT_BIGCURSOR:	    return editor->is_big_cursor();
#endif
    case STT_GRAPHBOXES:    return terminal::graphical_boxes;
    case STT_KEYFORKEYUNDO: return editbox->is_key_for_key_undo();
    case STT_BIDI:	    return editbox->is_bidi_enabled();

    case STT_EOPUNIX:	    return editbox->get_dominant_eop() == eopUnix;
    case STT_EOPDOS:	    return editbox->get_dominant_eop() == eopDOS;
    case STT_EOPMAC:	    return editbox->get_dominant_eop() == eopMac;
    case STT_EOPUNICODE:    return editbox->get_dominant_eop() == eopUnicode;

    case STT_RTLNSMASCII:  return editbox->get_rtl_nsm_display() == EditBox::rtlnsmTransliterated;
    case STT_RTLNSMASIS:   return editbox->get_rtl_nsm_display() == EditBox::rtlnsmAsis;
    case STT_RTLNSMHIDE:   return editbox->get_rtl_nsm_display() == EditBox::rtlnsmOff;

    case STT_MQFASCII:	    return editbox->get_maqaf_display() == EditBox::mqfTransliterated;
    case STT_MQFHIGHLIGHT:  return editbox->get_maqaf_display() == EditBox::mqfHighlighted;
    case STT_MQFASIS:	    return editbox->get_maqaf_display() == EditBox::mqfAsis;

    case STT_WRAPOFF:	    return editbox->get_wrap_type() == EditBox::wrpOff;
    case STT_WRAPBREAK:	    return editbox->get_wrap_type() == EditBox::wrpAnywhere;
    case STT_WRAPWSPACE:    return editbox->get_wrap_type() == EditBox::wrpAtWhiteSpace;

    case STT_DIRALGOUNICODE:    return editbox->get_dir_algo() == algoUnicode;
    case STT_DIRALGOCTXSTRNG:	return editbox->get_dir_algo() == algoContextStrong;
    case STT_DIRALGOCTXRTL:	return editbox->get_dir_algo() == algoContextRTL;
    case STT_DIRALGOFLTR:	return editbox->get_dir_algo() == algoForceLTR;
    case STT_DIRALGOFRTL:	return editbox->get_dir_algo() == algoForceRTL;

    case STT_SPELLERLOADED:	return editor->is_speller_loaded();

    case STT_SCRLBRNONE:    return editor->get_scrollbar_pos() == Editor::scrlbrNone;
    case STT_SCRLBRLEFT:    return editor->get_scrollbar_pos() == Editor::scrlbrLeft;
    case STT_SCRLBRRIGHT:   return editor->get_scrollbar_pos() == Editor::scrlbrRight;

    case STT_SYNNONE:	    return editbox->get_syn_hlt() == EditBox::synhltOff;
    case STT_SYNHTML:	    return editbox->get_syn_hlt() == EditBox::synhltHTML;
    case STT_SYNEMAIL:	    return editbox->get_syn_hlt() == EditBox::synhltEmail;

    case STT_UNDERLINE:	    return editbox->get_underline();

    case STT_SYNAUTO:	    return editor->get_syntax_auto_detection();

    case STT_CRSVIS:	    return editbox->get_visual_cursor_movement();
    case STT_CRSLOG:	    return !editbox->get_visual_cursor_movement();

    default:
	if (id >= STT_THEME_START && id <= STT_THEME_END) {
	    int i = 0;
	    while (ColorScheme[i].action) {
		if (ColorScheme[i].state_id == id)
		    return STREQ(ColorScheme[i].command_parameter3, editor->get_theme_name());
		i++;
	    }
	    return false;
	}
	else {
	    return false;
	}
    }
}

void MMPopupMenu::do_command(unsigned long parameter1, unsigned long parameter2,
			     const char *parameter3)
{
    static int flag;
    switch (parameter1) {
    case CMD_SETTHEME:
	editor->set_theme(parameter3);
	break;
    case CMD_CHR:
	editbox->insert_char((unichar)parameter2);
	break;
    case CMD_CRSVIS:
	editbox->set_visual_cursor_movement(true);
	break;
    case CMD_CRSLOG:
	editbox->set_visual_cursor_movement(false);
	break;
    case CMD_HELP:
	editor->show_help_topic(parameter3);
	break;
    case CMD_ENC:
	if (flag == FLAG_DEFAULT_ENCODING)
	    editor->set_default_encoding(parameter3);
	else
	    editor->set_encoding(parameter3);
	break;
    case CMD_INTERACTIVEENC:
	editor->menu_set_encoding(flag == FLAG_DEFAULT_ENCODING);
	break;
    case CMD_SETFLAG:
	flag = parameter2;
	break;
    }
}

/////////////////////////////// MMMenubar //////////////////////////////////

class MMMenubar : public Menubar {
    Editor  *editor;
    EditBox *editbox;
    void populate_color_scheme_menu();
public:
    MMMenubar(Editor *, EditBox *);
    virtual void refresh_screen();
    virtual PopupMenu *create_popupmenu(PulldownMenu mnu);
};

MMMenubar::MMMenubar(Editor *aEditor, EditBox *aEditbox)
    : Menubar()
{
    editor  = aEditor;
    editbox = aEditbox;
    populate_color_scheme_menu();
    init(mainMenu);
}

void MMMenubar::refresh_screen()
{
    editor->refresh(true);
}

PopupMenu *MMMenubar::create_popupmenu(PulldownMenu mnu)
{
    return new MMPopupMenu(editor, editbox, NULL, mnu);
}

//////////////////// Populate the ColorScheme Menu /////////////////////////

bool operator<(const MenuItem &a, const MenuItem &b)
{
    u8string as = a.label, bs = b.label;
    return strcmp(as.erase_char('~').toupper_ascii().c_str(),
		  bs.erase_char('~').toupper_ascii().c_str()) < 0;
}

static u8string get_color_scheme_title(const char *filename)
{
#define MAX_LINE_LEN 1024
    FILE *fp;
    if ((fp = fopen(filename, "r")) != NULL) {
	char line[MAX_LINE_LEN];
	int  line_no = 0;
	while (fgets(line, MAX_LINE_LEN, fp) && line_no++ < 5) {
	    const char *pos;
	    if ((pos = strstr(line, "Title:")) != NULL) {
		fclose(fp);
		return u8string(pos + strlen("Title:")).trim();
	    }
	}
	fclose(fp);
    }
    return "";
#undef MAX_LINE_LEN
}

static bool theme_already_listed(const char *theme_name)
{
    int i = 0;
    while (ColorScheme[i].action) {
	if (ColorScheme[i].command_parameter3 &&
		STREQ(ColorScheme[i].command_parameter3, theme_name))
	    return true;
	i++;
    }
    return false;
}

void MMMenubar::populate_color_scheme_menu()
{
    int menu_pos = 0;
    while (ColorScheme[menu_pos].action) menu_pos++;
    int menu_start = menu_pos;

    for (int i = 0; i < 2; i++) {
	DIR	*dir;
	dirent	*ent;
	const char *dirstr = get_cfg_filename((i == 0) ? USER_THEMES_DIR : SYSTEM_THEMES_DIR);
	if ((dir = opendir(dirstr)) == NULL)
	    continue;
	while ((ent = readdir(dir)) != NULL) {
	    const char *d_name = ent->d_name;
	    u8string desc;
	    desc.cformat("File: %s%s", dirstr, d_name);

	    if (STREQ(d_name, "default.thm") && !ColorScheme[0].desc)
		ColorScheme[0].desc = strdup(desc.c_str());

	    if (strstr(d_name, ".thm") && !strchr(d_name, '~')
		    && !theme_already_listed(d_name)) {
		ColorScheme[menu_pos].action = "xxx";
		ColorScheme[menu_pos].command_parameter1 = CMD_SETTHEME;
		ColorScheme[menu_pos].command_parameter3 = strdup(d_name);
		ColorScheme[menu_pos].state_id = STT_THEME_START + menu_pos;
		ColorScheme[menu_pos].desc = strdup(desc.c_str());
		
		u8string filepath = dirstr; filepath += d_name;
		u8string title = get_color_scheme_title(filepath.c_str());
		if (title.empty()) {
		    char *label = strdup(d_name);
		    // cut the file extension
		    if (strchr(label, '.'))
			*strchr(label, '.') = '\0';
		    ColorScheme[menu_pos].label = label;
		} else {
		    ColorScheme[menu_pos].label = strdup(title.c_str());
		}
		
		menu_pos++;
	    }
	}
	closedir(dir);
    }
    std::sort(&ColorScheme[menu_start], &ColorScheme[menu_pos]);

    ColorScheme[menu_pos++].action = "---";
    ColorScheme[menu_pos].action = "xxx";
    ColorScheme[menu_pos].label  = HELP_ITEM;
    ColorScheme[menu_pos].command_parameter1 = CMD_HELP;
    ColorScheme[menu_pos].command_parameter3 = HELP_TOPIC_COLORS_STR;
}

////////////////////////////////////////////////////////////////////////////

Menubar *create_main_menubar(Editor *aEdtr, EditBox *aEdtbx)
{
    return new MMMenubar(aEdtr, aEdtbx);
}


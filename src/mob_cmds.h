/***************************************************************************
 *  Dark Cloud copyright (C) 1998-2019                                     *
 ***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik Strfeldt, Tom Madsen, and Katja Nyboe.    *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  ROM 2.4 improvements copyright (C) 1993-1998 Russ Taylor, Gabrielle    *
 *  Taylor and Brian Moore                                                 *
 *                                                                         *
 *  In order to use any part of this Diku Mud, you must comply with        *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt' as well as the ROM license.  In particular,   *
 *  you may not remove these copyright notices.                            *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *  Based on MERC 2.2 MOBprograms by N'Atas-ha.                            *
 *  Written and adapted to ROM 2.4 by                                      *
 *          Markku Nylander (markku.nylander@uta.fi)                       *
 *                                                                         *
 ***************************************************************************/

struct	mob_cmd_type
{
    char * const	name;
    DO_FUN *		do_fun;
};

/* the command table itself */
extern	const	struct	mob_cmd_type	mob_cmd_table[];

/*
 * Command functions.
 * Defined in mob_cmds.c
 */
DECLARE_DO_FUN(do_mpasound);
DECLARE_DO_FUN(do_mpgecho);
DECLARE_DO_FUN(do_mpzecho);
DECLARE_DO_FUN(do_mpkill);
DECLARE_DO_FUN(do_mpassist);
DECLARE_DO_FUN(do_mpjunk);
DECLARE_DO_FUN(do_mpechoaround);
DECLARE_DO_FUN(do_mpecho);
DECLARE_DO_FUN(do_mpechoat);
DECLARE_DO_FUN(do_mpmload);
DECLARE_DO_FUN(do_mpoload);
DECLARE_DO_FUN(do_mppurge);
DECLARE_DO_FUN(do_mpgoto);
DECLARE_DO_FUN(do_mpat);
DECLARE_DO_FUN(do_mptransfer);
DECLARE_DO_FUN(do_mpgtransfer);
DECLARE_DO_FUN(do_mpforce);
DECLARE_DO_FUN(do_mpgforce);
DECLARE_DO_FUN(do_mpvforce);
DECLARE_DO_FUN(do_mpcast);
DECLARE_DO_FUN(do_mpdamage);
DECLARE_DO_FUN(do_mpremember);
DECLARE_DO_FUN(do_mpforget);
DECLARE_DO_FUN(do_mpdelay);
DECLARE_DO_FUN(do_mpcancel);
DECLARE_DO_FUN(do_mpcall);
DECLARE_DO_FUN(do_mpflee);
DECLARE_DO_FUN(do_mpotransfer);
DECLARE_DO_FUN(do_mpremove);
DECLARE_DO_FUN(do_mprot);


/***************************************************************************
 *  Dark Cloud copyright (C) 1998-2018                                     *
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
*  File: olc.h                                                            *
*                                                                         *
*  This code was freely distributed with the The Isles 1.1 source code,   *
*  and has been used here for OLC - OLC would not be what it is without   *
*  all the previous coders who released their source code.                *
*                                                                         *
***************************************************************************/

/*
 * This is a header file for all the OLC files.  Feel free to copy it into
 * merc.h if you wish.  Many of these routines may be handy elsewhere in
 * the code.  -Jason Dinkel
*/

/*
 * The version info.  Please use this info when reporting bugs.
 * It is displayed in the game by typing 'version' while editing.
 * Do not remove these from the code - by request of Jason Dinkel
 */
#define OLC_VERSION	"ILAB Online Creation [Beta 1.0, ROM 2.3 modified]\r\n" \
        "     Port a ROM 2.4 v1.8\r\n"
#define AUTHOR	"     By Jason(jdinkel@mines.colorado.edu)\r\n" \
                "     Modified for use with ROM 2.3\r\n"        \
                "     By Hans Birkeland (hansbi@ifi.uio.no)\r\n" \
                "     Modificado para uso en ROM 2.4b6\r\n"	\
                "     Por Ivan Toledo (itoledo@ctcreuna.cl)\r\n"
#define DATE	"     (Apr. 7, 1995 - ROM mod, Apr 16, 1995)\r\n" \
        "     (Port a ROM 2.4 - Nov 2, 1996)\r\n" \
        "     Version actual : 1.8 - Sep 8, 1998\r\n"
#define CREDITS "     Original by Surreality(cxw197@psu.edu) and Locke(locke@lm.com)"

/*
 * New typedefs.
 */
typedef	bool OLC_FUN		(CHAR_DATA *ch, char *argument);
#define DECLARE_OLC_FUN(fun) OLC_FUN fun

/* Command procedures needed ROM OLC */
DECLARE_DO_FUN(do_help);
DECLARE_SPELL_FUN(spell_null);

/*
 * Connected states for editor.
 */
#define ED_NONE     0
#define ED_AREA     1
#define ED_ROOM	    2
#define ED_OBJECT   3
#define ED_MOBILE   4
#define ED_MPCODE   5
#define ED_HELP     6
#define ED_GROUP    7
#define ED_CLASS    8
#define ED_SKILL    9

/*
 * Interpreter Prototypes
 */
void    aedit          (CHAR_DATA *ch, char *argument);
void    redit          (CHAR_DATA *ch, char *argument);
void    medit          (CHAR_DATA *ch, char *argument);
void    oedit          (CHAR_DATA *ch, char *argument);
void	mpedit         (CHAR_DATA *ch, char *argument);
void	hedit          (CHAR_DATA *, char *);
void    gedit          (CHAR_DATA *ch, char *argument);
void    cedit          (CHAR_DATA *ch, char *argument);
void    sedit          (CHAR_DATA *ch, char *argument);

/*
 * OLC Constants
 */
#define MAX_MOB	1		/* Default maximum number for resetting mobs */

/*
 * Structure for an OLC editor command.
 */
struct olc_cmd_type
{
    char * const	name;
    OLC_FUN *		olc_fun;
};

/*
 * Structure for an OLC editor startup command.
 */
struct	editor_cmd_type
{
    char * const	name;
    DO_FUN *		do_fun;
};

/*
 * Utils.
 */
AREA_DATA *get_vnum_area    (int vnum);
AREA_DATA *get_area_data	(int vnum);
int flag_value			    (const struct flag_type *flag_table, char *argument);
char *flag_string		    (const struct flag_type *flag_table, int bits);
void add_reset              (ROOM_INDEX_DATA *room, RESET_DATA *pReset, int index);
int flag_find               (const char *name, const struct flag_type *flag_table);
void balance_mob            (MOB_INDEX_DATA *pMob);

/*
 * Interpreter Table Prototypes
 */
extern const struct olc_cmd_type	aedit_table[];
extern const struct olc_cmd_type	redit_table[];
extern const struct olc_cmd_type	oedit_table[];
extern const struct olc_cmd_type	medit_table[];
extern const struct olc_cmd_type	mpedit_table[];
extern const struct olc_cmd_type	hedit_table[];
extern const struct olc_cmd_type    gedit_table[];
extern const struct olc_cmd_type    cedit_table[];
extern const struct olc_cmd_type    sedit_table[];

/*
 * Editor Commands.
 */
int continent_lookup(const char *name);

DECLARE_DO_FUN(do_aedit);
DECLARE_DO_FUN(do_redit);
DECLARE_DO_FUN(do_oedit);
DECLARE_DO_FUN(do_medit);
DECLARE_DO_FUN(do_mpedit);
DECLARE_DO_FUN(do_hedit);
DECLARE_DO_FUN(do_gedit);
DECLARE_DO_FUN(do_cedit);
DECLARE_DO_FUN(do_sedit);

/*
* General Functions
*/
bool show_commands		(CHAR_DATA *ch, char *argument);
bool show_help			(CHAR_DATA *ch, char *argument);
bool edit_done			(CHAR_DATA *ch);
bool show_version		(CHAR_DATA *ch, char *argument);

/*
* Area Editor Prototypes
*/
DECLARE_OLC_FUN(aedit_show);
DECLARE_OLC_FUN(aedit_create);
DECLARE_OLC_FUN(aedit_name);
DECLARE_OLC_FUN(aedit_file);
DECLARE_OLC_FUN(aedit_age);
/* DECLARE_OLC_FUN( aedit_recall	);       ROM OLC */
DECLARE_OLC_FUN(aedit_reset);
DECLARE_OLC_FUN(aedit_security);
DECLARE_OLC_FUN(aedit_builder);
DECLARE_OLC_FUN(aedit_vnum);
DECLARE_OLC_FUN(aedit_lvnum);
DECLARE_OLC_FUN(aedit_uvnum);
DECLARE_OLC_FUN(aedit_credits);
DECLARE_OLC_FUN(aedit_levelrange);
DECLARE_OLC_FUN(aedit_continent);

/*
* Room Editor Prototypes
*/
DECLARE_OLC_FUN(redit_show);
DECLARE_OLC_FUN(redit_create);
DECLARE_OLC_FUN(redit_name);
DECLARE_OLC_FUN(redit_desc);
DECLARE_OLC_FUN(redit_ed);
DECLARE_OLC_FUN(redit_format);
DECLARE_OLC_FUN(redit_north);
DECLARE_OLC_FUN(redit_south);
DECLARE_OLC_FUN(redit_east);
DECLARE_OLC_FUN(redit_west);
DECLARE_OLC_FUN(redit_up);
DECLARE_OLC_FUN(redit_down);
DECLARE_OLC_FUN(redit_northeast);
DECLARE_OLC_FUN(redit_northwest);
DECLARE_OLC_FUN(redit_southeast);
DECLARE_OLC_FUN(redit_southwest);
DECLARE_OLC_FUN(redit_mreset);
DECLARE_OLC_FUN(redit_oreset);
DECLARE_OLC_FUN(redit_mlist);
DECLARE_OLC_FUN(redit_rlist);
DECLARE_OLC_FUN(redit_olist);
DECLARE_OLC_FUN(redit_mshow);
DECLARE_OLC_FUN(redit_oshow);
DECLARE_OLC_FUN(redit_heal);
DECLARE_OLC_FUN(redit_mana);
DECLARE_OLC_FUN(redit_clan);
DECLARE_OLC_FUN(redit_owner);
DECLARE_OLC_FUN(redit_room);
DECLARE_OLC_FUN(redit_sector);
DECLARE_OLC_FUN(redit_copy);
DECLARE_OLC_FUN(redit_delete);

/*
* Object Editor Prototypes
*/
DECLARE_OLC_FUN(oedit_show);
DECLARE_OLC_FUN(oedit_create);
DECLARE_OLC_FUN(oedit_name);
DECLARE_OLC_FUN(oedit_short);
DECLARE_OLC_FUN(oedit_long);
DECLARE_OLC_FUN(oedit_addaffect);
DECLARE_OLC_FUN(oedit_addapply);
DECLARE_OLC_FUN(oedit_delaffect);
DECLARE_OLC_FUN(oedit_value0);
DECLARE_OLC_FUN(oedit_value1);
DECLARE_OLC_FUN(oedit_value2);
DECLARE_OLC_FUN(oedit_value3);
DECLARE_OLC_FUN(oedit_value4);  /* ROM */
DECLARE_OLC_FUN(oedit_weight);
DECLARE_OLC_FUN(oedit_cost);
DECLARE_OLC_FUN(oedit_ed);
DECLARE_OLC_FUN(oedit_extra);  /* ROM */
DECLARE_OLC_FUN(oedit_wear);  /* ROM */
DECLARE_OLC_FUN(oedit_type);  /* ROM */
DECLARE_OLC_FUN(oedit_affect);  /* ROM */
DECLARE_OLC_FUN(oedit_material);  /* ROM */
DECLARE_OLC_FUN(oedit_level);  /* ROM */
DECLARE_OLC_FUN(oedit_condition);  /* ROM */
DECLARE_OLC_FUN(oedit_copy);
DECLARE_OLC_FUN(oedit_delete);

/*
* Mobile Editor Prototypes
*/
DECLARE_OLC_FUN(medit_show);
DECLARE_OLC_FUN(medit_create);
DECLARE_OLC_FUN(medit_name);
DECLARE_OLC_FUN(medit_short);
DECLARE_OLC_FUN(medit_long);
DECLARE_OLC_FUN(medit_shop);
DECLARE_OLC_FUN(medit_desc);
DECLARE_OLC_FUN(medit_level);
DECLARE_OLC_FUN(medit_align);
DECLARE_OLC_FUN(medit_spec);
DECLARE_OLC_FUN(medit_delete);
DECLARE_OLC_FUN(medit_sex);  /* ROM */
DECLARE_OLC_FUN(medit_act);  /* ROM */
DECLARE_OLC_FUN(medit_affect);  /* ROM */
DECLARE_OLC_FUN(medit_ac);  /* ROM */
DECLARE_OLC_FUN(medit_form);  /* ROM */
DECLARE_OLC_FUN(medit_part);  /* ROM */
DECLARE_OLC_FUN(medit_imm);  /* ROM */
DECLARE_OLC_FUN(medit_res);  /* ROM */
DECLARE_OLC_FUN(medit_vuln);  /* ROM */
DECLARE_OLC_FUN(medit_material);  /* ROM */
DECLARE_OLC_FUN(medit_off);  /* ROM */
DECLARE_OLC_FUN(medit_size);  /* ROM */
DECLARE_OLC_FUN(medit_hitdice);  /* ROM */
DECLARE_OLC_FUN(medit_manadice);  /* ROM */
DECLARE_OLC_FUN(medit_damdice);  /* ROM */
DECLARE_OLC_FUN(medit_race);  /* ROM */
DECLARE_OLC_FUN(medit_position);  /* ROM */
DECLARE_OLC_FUN(medit_gold);  /* ROM */
DECLARE_OLC_FUN(medit_hitroll);  /* ROM */
DECLARE_OLC_FUN(medit_damtype);  /* ROM */
DECLARE_OLC_FUN(medit_group);  /* ROM */
DECLARE_OLC_FUN(medit_addmprog);  /* ROM */
DECLARE_OLC_FUN(medit_delmprog);  /* ROM */
DECLARE_OLC_FUN(medit_copy);
DECLARE_OLC_FUN(medit_balance);

/* Mobprog editor */
DECLARE_OLC_FUN(mpedit_create);
DECLARE_OLC_FUN(mpedit_code);
DECLARE_OLC_FUN(mpedit_show);
DECLARE_OLC_FUN(mpedit_list);
DECLARE_OLC_FUN(mpedit_name);

/* Editor de helps */
DECLARE_OLC_FUN(hedit_keyword);
DECLARE_OLC_FUN(hedit_text);
DECLARE_OLC_FUN(hedit_new);
DECLARE_OLC_FUN(hedit_level);
DECLARE_OLC_FUN(hedit_delete);
DECLARE_OLC_FUN(hedit_show);
DECLARE_OLC_FUN(hedit_list);

/* Group Editor */
DECLARE_OLC_FUN(gedit_name);
DECLARE_OLC_FUN(gedit_add);
DECLARE_OLC_FUN(gedit_del);
DECLARE_OLC_FUN(gedit_rating);
DECLARE_OLC_FUN(gedit_show);
DECLARE_OLC_FUN(gedit_create);
DECLARE_OLC_FUN(gedit_list);

/* Skill Editor */
DECLARE_OLC_FUN(sedit_name);
DECLARE_OLC_FUN(sedit_spellfun);
DECLARE_OLC_FUN(sedit_target);
DECLARE_OLC_FUN(sedit_minpos);
DECLARE_OLC_FUN(sedit_minmana);
DECLARE_OLC_FUN(sedit_beats);
DECLARE_OLC_FUN(sedit_msgoff);
DECLARE_OLC_FUN(sedit_msgobj);
DECLARE_OLC_FUN(sedit_create);
DECLARE_OLC_FUN(sedit_show);
DECLARE_OLC_FUN(sedit_level);
DECLARE_OLC_FUN(sedit_rating);
DECLARE_OLC_FUN(sedit_race);
DECLARE_OLC_FUN(sedit_damage);
DECLARE_OLC_FUN(sedit_ranged);

/* Class Editor */
DECLARE_OLC_FUN(cedit_name);
DECLARE_OLC_FUN(cedit_whoname);
DECLARE_OLC_FUN(cedit_attrprime);
DECLARE_OLC_FUN(cedit_attrsecond);
DECLARE_OLC_FUN(cedit_weapon);
DECLARE_OLC_FUN(cedit_skilladept);
DECLARE_OLC_FUN(cedit_thac0);
DECLARE_OLC_FUN(cedit_thac32);
DECLARE_OLC_FUN(cedit_hpmin);
DECLARE_OLC_FUN(cedit_hpmax);
DECLARE_OLC_FUN(cedit_mana);
DECLARE_OLC_FUN(cedit_moon);
DECLARE_OLC_FUN(cedit_basegroup);
DECLARE_OLC_FUN(cedit_defgroup);
DECLARE_OLC_FUN(cedit_create);
DECLARE_OLC_FUN(cedit_show);
DECLARE_OLC_FUN(cedit_skills);
DECLARE_OLC_FUN(cedit_spells);
DECLARE_OLC_FUN(cedit_groups);
DECLARE_OLC_FUN(cedit_isreclass);
DECLARE_OLC_FUN(cedit_isenabled);
DECLARE_OLC_FUN(cedit_clan);
DECLARE_OLC_FUN(cedit_guild);

/*
* Macros
*/
#define TOGGLE_BIT(var, bit)    ((var) ^= (bit))

/* Return pointers to what is being edited. */
#define EDIT_MOB(Ch, Mob)	( Mob = (MOB_INDEX_DATA *)Ch->desc->pEdit )
#define EDIT_OBJ(Ch, Obj)	( Obj = (OBJ_INDEX_DATA *)Ch->desc->pEdit )
#define EDIT_ROOM(Ch, Room)	( Room = Ch->in_room )
#define EDIT_AREA(Ch, Area)	( Area = (AREA_DATA *)Ch->desc->pEdit )
#define EDIT_MPCODE(Ch, Code)   ( Code = (MPROG_CODE*)Ch->desc->pEdit )
#define EDIT_GROUP(Ch, Group)   ( Group = (GROUPTYPE *)Ch->desc->pEdit )
#define EDIT_CLASS(Ch, Class)   ( Class = (CLASSTYPE *)Ch->desc->pEdit )
#define EDIT_SKILL(Ch, Skill)   ( Skill = (SKILLTYPE *)Ch->desc->pEdit )

/*
* Prototypes
*/
/* mem.c - memory prototypes. */
#define ED	EXTRA_DESCR_DATA
RESET_DATA	*new_reset_data		(void);
void		free_reset_data		(RESET_DATA *pReset);
AREA_DATA	*new_area		(void);
void		free_area		(AREA_DATA *pArea);
EXIT_DATA	*new_exit		(void);
void		free_exit		(EXIT_DATA *pExit);
ED 		*new_extra_descr	(void);
void		free_extra_descr	(ED *pExtra);
ROOM_INDEX_DATA *new_room_index		(void);
void		free_room_index		(ROOM_INDEX_DATA *pRoom);
AFFECT_DATA	*new_affect		(void);
void		free_affect		(AFFECT_DATA* pAf);
SHOP_DATA	*new_shop		(void);
void		free_shop		(SHOP_DATA *pShop);
OBJ_INDEX_DATA	*new_obj_index		(void);
void		free_obj_index		(OBJ_INDEX_DATA *pObj);
MOB_INDEX_DATA	*new_mob_index		(void);
void		free_mob_index		(MOB_INDEX_DATA *pMob);
#undef	ED

void		show_liqlist		(CHAR_DATA *ch);
void		show_damlist		(CHAR_DATA *ch);

char * mprog_type_to_name(int type);
MPROG_LIST *new_mprog(void);
void free_mprog(MPROG_LIST *mp);
MPROG_CODE *new_mpcode(void);
void free_mpcode(MPROG_CODE *pMcode);
void unlink_reset(ROOM_INDEX_DATA *pRoom, RESET_DATA *pReset);
void unlink_obj_index(OBJ_INDEX_DATA *pObj);
void unlink_room_index(ROOM_INDEX_DATA *pRoom);
void unlink_mob_index(MOB_INDEX_DATA *pMob);

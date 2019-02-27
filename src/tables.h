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

struct flag_type
{
    char *name;
    int bit;
    bool settable;
};

struct clan_type
{
    char 	*name;                     // Keyword name, no spaces
    char 	*who_name;                 // The name that should appear in the who list
    char    *friendly_name;            // Friendly unformatted name without color and with spacing
    int 	hall;                      // Death Transfer Room
    int     recall_vnum;               // Default recall vnum for the clan
    bool	independent;               // True for loners/renegades, false for part of a real clan
    bool    enabled;                   // Whether or not this clan is enabled for use
    char    *guild_msg;                // The message that is gecho'd when a player is guilded
    char    *regen_room_directions;    // Directions to the clan's regen room from their recall
};

struct position_type
{
    char *name;
    char *short_name;
};

struct sex_type
{
    char *name;
};

struct size_type
{
    char *name;
};

struct	bit_type
{
    const	struct	flag_type *	table;
    char *				help;
};

/*
 * This is used for lookups on stat types to allow "wis", "str", etc. to be stored
 * instead of the int value (basically for readability).
 */
struct stat_type
{
    int stat;
    char *name;
};

/*
 * The portal shop type holds information that portal shops can use to transfer
 * customers around the world.  Define the location via the vnum, give it a keyword
 * name (the description will be read from the vnum) and the base cost (the actual
 * command can then manipulate the cost based off of factors like if it's transfering
 * a user across continents, etc).
 */
struct portal_shop_type
{
    char *name;
    int to_vnum;
    int cost;
};

/*
 * Deity table for the gods and goddess of the world.
 */
struct deity_type
{
    char *name;          // The actual name of the deity
    char *description;   // God/Goddesses description
    int align;           // Alignment (or all)
    bool enabled;        // Whether or not they can be selected at creation
};

/* game tables */
extern	const	struct	clan_type	     clan_table[MAX_CLAN];
extern	const	struct	position_type	 position_table[];
extern	const	struct	sex_type	     sex_table[];
extern	const	struct	size_type	     size_table[];
extern  const   struct  continent_type   continent_table[];
extern  const   struct  portal_shop_type portal_shop_table[];
extern  const   struct  deity_type       deity_table[];
extern  const   struct  merit_type       merit_table[];

/* flag tables */
extern	const	struct	flag_type	act_flags[];
extern	const	struct	flag_type	plr_flags[];
extern	const	struct	flag_type	affect_flags[];
extern	const	struct	flag_type	off_flags[];
extern	const	struct	flag_type	imm_flags[];
extern	const	struct	flag_type	form_flags[];
extern	const	struct	flag_type	part_flags[];
extern	const	struct	flag_type	comm_flags[];
extern	const	struct	flag_type	extra_flags[];
extern	const	struct	flag_type	wear_flags[];
extern	const	struct	flag_type	weapon_flags[];
extern	const	struct	flag_type	container_flags[];
extern	const	struct	flag_type	portal_flags[];
extern	const	struct	flag_type	room_flags[];
extern	const	struct	flag_type	exit_flags[];
extern 	const	struct  flag_type	mprog_flags[];
extern	const	struct	flag_type	area_flags[];
extern	const	struct	flag_type	sector_flags[];
extern	const	struct	flag_type	door_resets[];
extern	const	struct	flag_type	wear_loc_strings[];
extern	const	struct	flag_type	wear_loc_flags[];
extern	const	struct	flag_type	res_flags[];
extern	const	struct	flag_type	imm_flags[];
extern	const	struct	flag_type	vuln_flags[];
extern	const	struct	flag_type	type_flags[];
extern	const	struct	flag_type	apply_flags[];
extern	const	struct	flag_type	sex_flags[];
extern	const	struct	flag_type	furniture_flags[];
extern	const	struct	flag_type	weapon_class[];
extern	const	struct	flag_type	apply_types[];
extern	const	struct	flag_type	weapon_type2[];
extern	const	struct	flag_type	apply_types[];
extern	const	struct	flag_type	size_flags[];
extern	const	struct	flag_type	position_flags[];
extern	const	struct	flag_type	ac_type[];
extern	const	struct	bit_type	bitvector_type[];
extern  const   struct  flag_type   continent_flags[];
extern  const   struct  flag_type   stat_flags[];
extern  const   struct  flag_type   target_flags[];
extern  const   struct  flag_type   pc_clan_flags[];  // Player clan flags

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
 *  File: olc_bit.c                                                        *
 *                                                                         *
 *  This code was written by Jason Dinkel and inspired by Russ Taylor,     *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/
/*
 The code below uses a table lookup system that is based on suggestions
 from Russ Taylor.  There are many routines in handler.c that would benefit
 with the use of tables.  You may consider simplifying your code base by
 implementing a system like below with such functions. -Jason Dinkel
 */

// System Specific Includes
#if defined(_WIN32)
#include <sys/types.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#endif

// General Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "tables.h"
#include "lookup.h"

int flag_find(const char *name, const struct flag_type *flag_table);

struct flag_stat_type {
    const struct flag_type *structure;
    bool stat;
};

/*****************************************************************************
 Name:        flag_stat_table
 Purpose:    This table catagorizes the tables following the lookup
         functions below into stats and flags.  Flags can be toggled
         but stats can only be assigned.  Update this table when a
         new set of flags is installed.
 ****************************************************************************/
const struct flag_stat_type flag_stat_table[] = {
/*  {    structure        stat    }, */
    {area_flags, FALSE},
    {sex_flags, TRUE},
    {exit_flags, FALSE},
    {door_resets, TRUE},
    {room_flags, FALSE},
    {sector_flags, TRUE},
    {type_flags, TRUE},
    {extra_flags, FALSE},
    {wear_flags, FALSE},
    {act_flags, FALSE},
    {affect_flags, FALSE},
    {apply_flags, TRUE},
    {wear_loc_flags, TRUE},
    {wear_loc_strings, TRUE},
    {container_flags, FALSE},
    {continent_flags, TRUE},
    {target_flags, TRUE},

/* ROM specific flags: */

    {form_flags, FALSE},
    {part_flags, FALSE},
    {ac_type, TRUE},
    {size_flags, TRUE},
    {position_flags, TRUE},
    {off_flags, FALSE},
    {imm_flags, FALSE},
    {res_flags, FALSE},
    {vuln_flags, FALSE},
    {weapon_class, TRUE},
    {weapon_type2, FALSE},
    {apply_types, TRUE},
    {stat_flags, TRUE},
    {0, 0}
};

const struct flag_type stat_flags[] =
{
   {    "str",            STAT_STR,             TRUE    },
   {    "int",            STAT_INT,             TRUE    },
   {    "wis",            STAT_WIS,             TRUE    },
   {    "dex",            STAT_DEX,             TRUE    },
   {    "con",            STAT_CON,             TRUE    },
   {    NULL,             0,                    FALSE   }
};


/*****************************************************************************
 Name:        is_stat( table )
 Purpose:    Returns TRUE if the table is a stat table and FALSE if flag.
 Called by:    flag_value and flag_string.
 Note:        This function is local and used only in bit.c.
 ****************************************************************************/
bool is_stat(const struct flag_type *flag_table)
{
    int flag;

    for (flag = 0; flag_stat_table[flag].structure; flag++)
    {
        if (flag_stat_table[flag].structure == flag_table
            && flag_stat_table[flag].stat)
            return TRUE;
    }
    return FALSE;
}

/*****************************************************************************
 Name:        flag_value( table, flag )
 Purpose:    Returns the value of the flags entered.  Multi-flags accepted.
 Called by:    olc.c and olc_act.c.
 ****************************************************************************/
int flag_value(const struct flag_type *flag_table, char *argument)
{
    char word[MAX_INPUT_LENGTH];
    register int  bit;
    register int  marked = 0;
    bool found = FALSE;

    if (is_stat(flag_table))
    {
        one_argument(argument, word);

        if ((bit = flag_find(word, flag_table)) != NO_FLAG)
            return bit;
        else
            return NO_FLAG;
    }

    /*
     * Accept multiple flags.
     */
    for (; ;)
    {
        argument = one_argument(argument, word);

        if (word[0] == '\0')
            break;

        if ((bit = flag_find(word, flag_table)) != NO_FLAG)
        {
            SET_BIT(marked, bit);
            found = TRUE;
        }
    }

    if (found)
        return marked;
    else
        return NO_FLAG;
}


/*****************************************************************************
 Name:        flag_string( table, flags/stat )
 Purpose:    Returns string with name(s) of the flags or stat entered.
 Called by:    act_olc.c, olc.c, and olc_save.c.
 ****************************************************************************/
char *flag_string(const struct flag_type *flag_table, register int bits)
{
    static char buf[512];
    register int  flag;

    buf[0] = '\0';

    for (flag = 0; flag_table[flag].name != NULL; flag++)
    {
        if (!is_stat(flag_table) && IS_SET(bits, flag_table[flag].bit))
        {
            strcat(buf, " ");
            strcat(buf, flag_table[flag].name);
        }
        else
            if (flag_table[flag].bit == bits)
            {
                strcat(buf, " ");
                strcat(buf, flag_table[flag].name);
                break;
            }
    }
    return (buf[0] != '\0') ? buf + 1 : "none";
}

/*****************************************************************************
 Name:          flag_find( flag, table )
 Purpose:       Returns the value of a single, settable flag from the table.
 Called by:     flag_value and flag_string.
 Note:          This function is local and used only in bit.c.
 ****************************************************************************/
int flag_find(const char *name, const struct flag_type *flag_table)
{
    register int flag;

    for (flag = 0; flag_table[flag].name != NULL; flag++)
    {
        if (!str_cmp(name, flag_table[flag].name)
            && flag_table[flag].settable)
            return flag_table[flag].bit;
    }

    return NO_FLAG;
}

/*
 * This is the table to hold our friendly neighborhood continents.
 */
const struct flag_type continent_flags[] = {
    { "limbo",    CONTINENT_ETHEREAL_PLANE,    TRUE },
    { "oceans",   CONTINENT_OCEANS,   TRUE },
    { "midgaard", CONTINENT_MIDGAARD, TRUE },
    { "arcanis",  CONTINENT_ARCANIS,  TRUE },
    { NULL, 0, 0 }
};

/*
 * Target flags for spells, this will allow us to save friendly names in the
 * skills.dat file and then look them up.
 */
const struct flag_type target_flags[] =
{
    {   "ignore",               TAR_IGNORE,             TRUE    },
    {   "char_offensive",       TAR_CHAR_OFFENSIVE,     TRUE    },
    {   "char_defensive",       TAR_CHAR_DEFENSIVE,     TRUE    },
    {   "char_self",            TAR_CHAR_SELF,          TRUE    },
    {   "obj_inventory",        TAR_OBJ_INV,            TRUE    },
    {   "obj_char_defensive",   TAR_OBJ_CHAR_DEF,       TRUE    },
    {   "obj_char_offensive",   TAR_OBJ_CHAR_OFF,       TRUE    },
    {   NULL,                   0,                      FALSE   }
};

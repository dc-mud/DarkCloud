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
*  File: olc_act.c                                                        *
*                                                                         *
*  This code was freely distributed with the The Isles 1.1 source code,   *
*  and has been used here for OLC - OLC would not be what it is without   *
*  all the previous coders who released their source code.                *
*                                                                         *
***************************************************************************/

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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"
#include "recycle.h"
#include "lookup.h"
#include "interp.h"

char *mprog_type_to_name(int type);

#define ALT_FLAGVALUE_SET( _blargh, _table, _arg )        \
    {                            \
        int blah = flag_value( _table, _arg );        \
        _blargh = (blah == NO_FLAG) ? 0 : blah;        \
    }

#define ALT_FLAGVALUE_TOGGLE( _blargh, _table, _arg )        \
    {                            \
        int blah = flag_value( _table, _arg );        \
        _blargh ^= (blah == NO_FLAG) ? 0 : blah;    \
    }

char      *spell_name_lookup     (SPELL_FUN *spell);
SPELL_FUN *spell_function_lookup (char *name);

/* Return TRUE if area changed, FALSE if not. */
#define REDIT( fun )        bool fun( CHAR_DATA *ch, char *argument )
#define OEDIT( fun )        bool fun( CHAR_DATA *ch, char *argument )
#define MEDIT( fun )        bool fun( CHAR_DATA *ch, char *argument )
#define AEDIT( fun )        bool fun( CHAR_DATA *ch, char *argument )
#define HEDIT( fun )        bool fun( CHAR_DATA *ch, char *argument )
#define MPEDIT( fun )       bool fun( CHAR_DATA *ch, char *argument )
#define GEDIT( fun )        bool fun( CHAR_DATA *ch, char *argument )
#define CEDIT( fun )        bool fun( CHAR_DATA *ch, char *argument )
#define SEDIT( fun )        bool fun( CHAR_DATA *ch, char *argument )
#define EDIT_HELP(ch, help) ( help = (HELP_DATA *) ch->desc->pEdit )

struct olc_help_type {
    char *command;
    const void *structure;
    char *desc;
};

bool show_version(CHAR_DATA * ch, char *argument)
{
    send_to_char(OLC_VERSION, ch);
    send_to_char("\r\n", ch);
    send_to_char(AUTHOR, ch);
    send_to_char("\r\n", ch);
    send_to_char(DATE, ch);
    send_to_char("\r\n", ch);
    send_to_char(CREDITS, ch);
    send_to_char("\r\n", ch);

    return FALSE;
}

/*
 * This table contains help commands and a brief description of each.
 * ------------------------------------------------------------------
 */
const struct olc_help_type help_table[] = {
    { "area", area_flags, "Area attributes." },
    { "room", room_flags, "Room attributes." },
    { "sector", sector_flags, "Sector types, terrain." },
    { "exit", exit_flags, "Exit types." },
    { "type", type_flags, "Types of objects." },
    { "extra", extra_flags, "Object attributes." },
    { "wear", wear_flags, "Where to wear object." },
    { "spec", spec_table, "Available special programs." },
    { "sex", sex_flags, "Sexes." },
    { "act", act_flags, "Mobile attributes." },
    { "affect", affect_flags, "Mobile affects." },
    { "wear-loc", wear_loc_flags, "Where mobile wears object." },
    { "spells", skill_table, "Names of current spells." },
    { "container", container_flags, "Container status." },
    { "continent", continent_flags, "Continent names." },
    { "minpos", position_flags, "Position names." },
    { "target", target_flags, "Target names" },

    /* ROM specific bits: */

    { "armor", ac_type, "Ac for different attacks." },
    { "apply", apply_flags, "Apply flags" },
    { "form", form_flags, "Mobile body form." },
    { "part", part_flags, "Mobile body parts." },
    { "imm", imm_flags, "Mobile immunity." },
    { "res", res_flags, "Mobile resistance." },
    { "vuln", vuln_flags, "Mobile vulnerability." },
    { "off", off_flags, "Mobile offensive behaviour." },
    { "size", size_flags, "Mobile size." },
    { "position", position_flags, "Mobile positions." },
    { "wclass", weapon_class, "Weapon class." },
    { "wtype", weapon_type2, "Special weapon type." },
    { "portal", portal_flags, "Portal types." },
    { "furniture", furniture_flags, "Furniture types." },
    { "liquid", liquid_table, "Liquid types." },
    { "apptype", apply_types, "Apply types." },
    { "weapon", attack_table, "Weapon types." },
    { "mprog", mprog_flags, "MobProgram flags." },
    { NULL, NULL, NULL }
};

/*****************************************************************************
Name:       show_flag_cmds
Purpose:    Displays settable flags and stats.
Called by:  show_help(olc_act.c).
****************************************************************************/
void show_flag_cmds(CHAR_DATA * ch, const struct flag_type *flag_table)
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    int flag;
    int col;

    buf1[0] = '\0';
    col = 0;
    for (flag = 0; flag_table[flag].name != NULL; flag++)
    {
        if (flag_table[flag].settable)
        {
            sprintf(buf, "%-19.18s", flag_table[flag].name);
            strcat(buf1, buf);
            if (++col % 4 == 0)
                strcat(buf1, "\r\n");
        }
    }

    if (col % 4 != 0)
        strcat(buf1, "\r\n");

    send_to_char(buf1, ch);
    return;
}

/*****************************************************************************
Name:       show_skill_cmds
Purpose:    Displays all skill functions.
            Does remove those damn immortal commands from the list.
            Could be improved by:
            (1) Adding a check for a particular class.
            (2) Adding a check for a level range.
Called by:  show_help(olc_act.c).
****************************************************************************/
void show_skill_cmds(CHAR_DATA * ch, int tar)
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH * 2];
    int sn;
    int col;

    buf1[0] = '\0';
    col = 0;
    for (sn = 0; sn < top_sn; sn++)
    {
        if (!skill_table[sn]->name)
            break;

        if (!str_cmp(skill_table[sn]->name, "reserved")
            || skill_table[sn]->spell_fun == spell_null)
            continue;

        if (tar == -1 || skill_table[sn]->target == tar)
        {
            sprintf(buf, "%-19.18s", skill_table[sn]->name);
            strcat(buf1, buf);
            if (++col % 4 == 0)
                strcat(buf1, "\r\n");
        }
    }

    if (col % 4 != 0)
        strcat(buf1, "\r\n");

    send_to_char(buf1, ch);
    return;
}

/*****************************************************************************
Name:       show_spec_cmds
Purpose:    Displays settable special functions.
Called by:  show_help(olc_act.c).
****************************************************************************/
void show_spec_cmds(CHAR_DATA * ch)
{
    char buf[MAX_STRING_LENGTH];
    char buf1[MAX_STRING_LENGTH];
    int spec;
    int col;

    buf1[0] = '\0';
    col = 0;
    send_to_char("Preceed special functions with 'spec_'\r\n\r\n", ch);
    for (spec = 0; spec_table[spec].function != NULL; spec++)
    {
        sprintf(buf, "%-19.18s", &spec_table[spec].name[5]);
        strcat(buf1, buf);
        if (++col % 4 == 0)
            strcat(buf1, "\r\n");
    }

    if (col % 4 != 0)
        strcat(buf1, "\r\n");

    send_to_char(buf1, ch);
    return;
}

/*****************************************************************************
Name:       show_help
Purpose:    Displays help for many tables used in OLC.
Called by:  olc interpreters.
****************************************************************************/
bool show_help(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char spell[MAX_INPUT_LENGTH];
    int cnt;

    argument = one_argument(argument, arg);
    one_argument(argument, spell);

    /*
    * Display syntax.
    */
    if (arg[0] == '\0')
    {
        send_to_char("Syntax:  ? [command]\r\n\r\n", ch);
        send_to_char("[command]  [description]\r\n", ch);
        for (cnt = 0; help_table[cnt].command != NULL; cnt++)
        {
            sprintf(buf, "%-10.10s -%s\r\n",
                capitalize(help_table[cnt].command),
                help_table[cnt].desc);
            send_to_char(buf, ch);
        }
        return FALSE;
    }

    /*
    * Find the command, show changeable data.
    * ---------------------------------------
    */
    for (cnt = 0; help_table[cnt].command != NULL; cnt++)
    {
        if (arg[0] == help_table[cnt].command[0]
            && !str_prefix(arg, help_table[cnt].command))
        {
            if (help_table[cnt].structure == spec_table)
            {
                show_spec_cmds(ch);
                return FALSE;
            }
            else if (help_table[cnt].structure == liquid_table)
            {
                show_liqlist(ch);
                return FALSE;
            }
            else if (help_table[cnt].structure == attack_table)
            {
                show_damlist(ch);
                return FALSE;
            }
            else if (help_table[cnt].structure == skill_table)
            {

                if (spell[0] == '\0')
                {
                    send_to_char("Syntax:  ? spells "
                        "[ignore/attack/defend/self/object/all]\r\n",
                        ch);
                    return FALSE;
                }

                if (!str_prefix(spell, "all"))
                    show_skill_cmds(ch, -1);
                else if (!str_prefix(spell, "ignore"))
                    show_skill_cmds(ch, TAR_IGNORE);
                else if (!str_prefix(spell, "attack"))
                    show_skill_cmds(ch, TAR_CHAR_OFFENSIVE);
                else if (!str_prefix(spell, "defend"))
                    show_skill_cmds(ch, TAR_CHAR_DEFENSIVE);
                else if (!str_prefix(spell, "self"))
                    show_skill_cmds(ch, TAR_CHAR_SELF);
                else if (!str_prefix(spell, "object"))
                    show_skill_cmds(ch, TAR_OBJ_INV);
                else
                    send_to_char("Syntax:  ? spell "
                        "[ignore/attack/defend/self/object/all]\r\n",
                        ch);

                return FALSE;
            }
            else
            {
                show_flag_cmds(ch, help_table[cnt].structure);
                return FALSE;
            }
        }
    }

    show_help(ch, "");
    return FALSE;
}

REDIT(redit_rlist)
{
    ROOM_INDEX_DATA *pRoomIndex;
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];
    BUFFER *buf1;
    char arg[MAX_INPUT_LENGTH];
    bool found;
    int vnum;
    int col = 0;

    one_argument(argument, arg);

    pArea = ch->in_room->area;
    buf1 = new_buf();
    /*    buf1[0] = '\0'; */
    found = FALSE;

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++)
    {
        if ((pRoomIndex = get_room_index(vnum)))
        {
            found = TRUE;
            sprintf(buf, "[%5d] %-17.16s",
                vnum, capitalize(pRoomIndex->name));
            add_buf(buf1, buf);
            if (++col % 3 == 0)
                add_buf(buf1, "\r\n");
        }
    }

    if (!found)
    {
        send_to_char("Room(s) not found in this area.\r\n", ch);
        return FALSE;
    }

    if (col % 3 != 0)
        add_buf(buf1, "\r\n");

    page_to_char(buf_string(buf1), ch);
    free_buf(buf1);
    return FALSE;
}

REDIT(redit_mlist)
{
    MOB_INDEX_DATA *pMobIndex;
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];
    BUFFER *buf1;
    char arg[MAX_INPUT_LENGTH];
    bool fAll, found;
    int vnum;
    int col = 0;

    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char("Syntax:  mlist <all/name>\r\n", ch);
        return FALSE;
    }

    buf1 = new_buf();
    pArea = ch->in_room->area;
    /*    buf1[0] = '\0'; */
    fAll = !str_cmp(arg, "all");
    found = FALSE;

    send_to_char("[Lv  Vnum] [Mob Name]                   [Lv  Vnum] [Mob Name]\r\n", ch);

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++)
    {
        if ((pMobIndex = get_mob_index(vnum)) != NULL)
        {
            if (fAll || is_name(arg, pMobIndex->player_name))
            {
                found = TRUE;
                sprintf(buf, "[%s%2d{x %5d] %-29.28s",
                    pMobIndex->level < 0 ? "{R" : "",
                    pMobIndex->level,
                    pMobIndex->vnum,
                    strip_color(capitalize(pMobIndex->short_descr)));
                add_buf(buf1, buf);
                if (++col % 2 == 0)
                    add_buf(buf1, "\r\n");
            }
        }
    }

    if (!found)
    {
        send_to_char("Mobile(s) not found in this area.\r\n", ch);
        return FALSE;
    }

    if (col % 2 != 0)
        add_buf(buf1, "\r\n");

    page_to_char(buf_string(buf1), ch);
    free_buf(buf1);
    return FALSE;
}

REDIT(redit_olist)
{
    OBJ_INDEX_DATA *pObjIndex;
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];
    BUFFER *buf1;
    char arg[MAX_INPUT_LENGTH];
    bool fAll, found;
    int vnum;
    int col = 0;

    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char("Syntax:  olist <all/name/item_type>\r\n", ch);
        return FALSE;
    }

    pArea = ch->in_room->area;
    buf1 = new_buf();
    /*    buf1[0] = '\0'; */
    fAll = !str_cmp(arg, "all");
    found = FALSE;

    send_to_char("[Lv  Vnum] [Object Name]                [Lv  Vnum] [Object Name]\r\n", ch);

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++)
    {
        if ((pObjIndex = get_obj_index(vnum)))
        {
            if (fAll || is_name(arg, pObjIndex->name)
                || flag_value(type_flags, arg) == pObjIndex->item_type)
            {
                found = TRUE;
                sprintf(buf, "[%s%2d{x %5d] %-29.28s",
                    pObjIndex->level < 0 ? "{R" : "",
                    pObjIndex->level,
                    pObjIndex->vnum,
                    strip_color(capitalize(pObjIndex->short_descr)));
                add_buf(buf1, buf);
                if (++col % 2 == 0)
                    add_buf(buf1, "\r\n");
            }
        }
    }

    if (!found)
    {
        send_to_char("Object(s) not found in this area.\r\n", ch);
        return FALSE;
    }

    if (col % 2 != 0)
        add_buf(buf1, "\r\n");

    page_to_char(buf_string(buf1), ch);
    free_buf(buf1);
    return FALSE;
}

REDIT(redit_mshow)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  mshow <vnum>\r\n", ch);
        return FALSE;
    }

    if (!is_number(argument))
    {
        send_to_char("REdit: Must be a number.\r\n", ch);
        return FALSE;
    }

    if (is_number(argument))
    {
        value = atoi(argument);
        if (!(pMob = get_mob_index(value)))
        {
            send_to_char("REdit:  That mobile does not exist.\r\n", ch);
            return FALSE;
        }

        ch->desc->pEdit = (void *)pMob;
    }

    medit_show(ch, argument);
    ch->desc->pEdit = (void *)ch->in_room;
    return FALSE;
}

REDIT(redit_oshow)
{
    OBJ_INDEX_DATA *pObj;
    int value;

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  oshow <vnum>\r\n", ch);
        return FALSE;
    }

    if (!is_number(argument))
    {
        send_to_char("REdit: Must be a number.\r\n", ch);
        return FALSE;
    }

    if (is_number(argument))
    {
        value = atoi(argument);
        if (!(pObj = get_obj_index(value)))
        {
            send_to_char("REdit:  That object does not exist.\r\n", ch);
            return FALSE;
        }

        ch->desc->pEdit = (void *)pObj;
    }

    oedit_show(ch, argument);
    ch->desc->pEdit = (void *)ch->in_room;
    return FALSE;
}

/*****************************************************************************
Name:       check_range( lower vnum, upper vnum )
Purpose:    Ensures the range spans only one area.
Called by:  aedit_vnum(olc_act.c).
****************************************************************************/
bool check_range(int lower, int upper)
{
    AREA_DATA *pArea;
    int cnt = 0;

    for (pArea = area_first; pArea; pArea = pArea->next)
    {
        /*
        * lower < area < upper
        */
        if ((lower <= pArea->min_vnum && pArea->min_vnum <= upper)
            || (lower <= pArea->max_vnum && pArea->max_vnum <= upper))
            ++cnt;

        if (cnt > 1)
            return FALSE;
    }
    return TRUE;
}

AREA_DATA *get_vnum_area(int vnum)
{
    AREA_DATA *pArea;

    for (pArea = area_first; pArea; pArea = pArea->next)
    {
        if (vnum >= pArea->min_vnum && vnum <= pArea->max_vnum)
            return pArea;
    }

    return 0;
}

/*
 * Area Editor Functions.
 */
AEDIT(aedit_show)
{
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];

    EDIT_AREA(ch, pArea);

    sprintf(buf, "Name:       [%5d] %s\r\n", pArea->vnum, pArea->name);
    send_to_char(buf, ch);

#if 0                            /* ROM OLC */
    sprintf(buf, "Recall:     [%5d] %s\r\n", pArea->recall,
        get_room_index(pArea->recall)
        ? get_room_index(pArea->recall)->name : "none");
    send_to_char(buf, ch);
#endif /* ROM */

    sprintf(buf, "File:       %s\r\n", pArea->file_name);
    send_to_char(buf, ch);

    sprintf(buf, "Vnums:      [%d-%d]\r\n", pArea->min_vnum, pArea->max_vnum);
    send_to_char(buf, ch);

    sprintf(buf, "LevelRange: [%d-%d]\r\n", pArea->min_level, pArea->max_level);
    send_to_char(buf, ch);

    sprintf(buf, "Age:        [%d]\r\n", pArea->age);
    send_to_char(buf, ch);

    sprintf(buf, "Players:    [%d]\r\n", pArea->nplayer);
    send_to_char(buf, ch);

    sprintf(buf, "Security:   [%d]\r\n", pArea->security);
    send_to_char(buf, ch);

    sprintf(buf, "Builders:   [%s]\r\n", pArea->builders);
    send_to_char(buf, ch);

    sprintf(buf, "Credits :   [%s]\r\n", pArea->credits);
    send_to_char(buf, ch);

    sprintf(buf, "Area Flags: [%s]\r\n", flag_string(area_flags, pArea->area_flags));
    send_to_char(buf, ch);

    sprintf(buf, "Continent:  [%s]\r\n", continent_table[pArea->continent].name);
    send_to_char(buf, ch);

    return FALSE;
}

AEDIT(aedit_reset)
{
    AREA_DATA *pArea;

    EDIT_AREA(ch, pArea);

    reset_area(pArea);
    send_to_char("Area reset.\r\n", ch);

    return FALSE;
}

AEDIT(aedit_create)
{
    AREA_DATA *pArea;

    pArea = new_area();
    area_last->next = pArea;
    area_last = pArea;            /* Thanks, Walker. */
    ch->desc->pEdit = (void *)pArea;

    SET_BIT(pArea->area_flags, AREA_ADDED);
    send_to_char("Area Created.\r\n", ch);
    return FALSE;
}

AEDIT(aedit_name)
{
    AREA_DATA *pArea;

    EDIT_AREA(ch, pArea);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:   name [$name]\r\n", ch);
        return FALSE;
    }

    free_string(pArea->name);
    pArea->name = str_dup(argument);

    send_to_char("Name set.\r\n", ch);
    return TRUE;
}

AEDIT(aedit_credits)
{
    AREA_DATA *pArea;

    EDIT_AREA(ch, pArea);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:   credits [$credits]\r\n", ch);
        return FALSE;
    }

    free_string(pArea->credits);
    pArea->credits = str_dup(argument);

    send_to_char("Credits set.\r\n", ch);
    return TRUE;
}

AEDIT(aedit_file)
{
    AREA_DATA *pArea;
    char file[MAX_STRING_LENGTH];
    int i, length;

    EDIT_AREA(ch, pArea);

    one_argument(argument, file);    /* Forces Lowercase */

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  filename [$file]\r\n", ch);
        return FALSE;
    }

    /*
    * Simple Syntax Check.
    */
    length = strlen(argument);

    /*
    * Allow only letters and numbers.
    */
    for (i = 0; i < length; i++)
    {
        if (!isalnum(file[i]))
        {
            send_to_char("Only letters and numbers are valid.\r\n", ch);
            return FALSE;
        }
    }

    free_string(pArea->file_name);
    strcat(file, ".are");
    pArea->file_name = str_dup(file);

    send_to_char("Filename set.\r\n", ch);
    return TRUE;
}

AEDIT(aedit_age)
{
    AREA_DATA *pArea;
    char age[MAX_STRING_LENGTH];

    EDIT_AREA(ch, pArea);

    one_argument(argument, age);

    if (!is_number(age) || age[0] == '\0')
    {
        send_to_char("Syntax:  age [#xage]\r\n", ch);
        return FALSE;
    }

    pArea->age = atoi(age);

    send_to_char("Age set.\r\n", ch);
    return TRUE;
}

#if 0                            /* ROM OLC */
AEDIT(aedit_recall)
{
    AREA_DATA *pArea;
    char room[MAX_STRING_LENGTH];
    int value;

    EDIT_AREA(ch, pArea);

    one_argument(argument, room);

    if (!is_number(argument) || argument[0] == '\0')
    {
        send_to_char("Syntax:  recall [#xrvnum]\r\n", ch);
        return FALSE;
    }

    value = atoi(room);

    if (!get_room_index(value))
    {
        send_to_char("AEdit:  Room vnum does not exist.\r\n", ch);
        return FALSE;
    }

    pArea->recall = value;

    send_to_char("Recall set.\r\n", ch);
    return TRUE;
}
#endif /* ROM OLC */

AEDIT(aedit_security)
{
    AREA_DATA *pArea;
    char sec[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int value;

    EDIT_AREA(ch, pArea);

    one_argument(argument, sec);

    if (!is_number(sec) || sec[0] == '\0')
    {
        send_to_char("Syntax:  security [#xlevel]\r\n", ch);
        return FALSE;
    }

    value = atoi(sec);

    if (value > ch->pcdata->security || value < 0)
    {
        if (ch->pcdata->security != 0)
        {
            sprintf(buf, "Security is 0-%d.\r\n", ch->pcdata->security);
            send_to_char(buf, ch);
        }
        else
            send_to_char("Security is 0 only.\r\n", ch);
        return FALSE;
    }

    pArea->security = value;

    send_to_char("Security set.\r\n", ch);
    return TRUE;
}

AEDIT(aedit_builder)
{
    AREA_DATA *pArea;
    char name[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    EDIT_AREA(ch, pArea);

    one_argument(argument, name);

    if (name[0] == '\0')
    {
        send_to_char("Syntax:  builder [$name]  -toggles builder\r\n", ch);
        send_to_char("Syntax:  builder All      -allows everyone\r\n", ch);
        return FALSE;
    }

    name[0] = UPPER(name[0]);

    if (strstr(pArea->builders, name) != NULL)
    {
        pArea->builders = string_replace(pArea->builders, name, "\0");
        pArea->builders = string_unpad(pArea->builders);

        if (pArea->builders[0] == '\0')
        {
            free_string(pArea->builders);
            pArea->builders = str_dup("None");
        }

        send_to_char("Builder removed.\r\n", ch);
        return TRUE;
    }
    else
    {
        buf[0] = '\0';
        if (strstr(pArea->builders, "None") != NULL)
        {
            pArea->builders = string_replace(pArea->builders, "None", "\0");
            pArea->builders = string_unpad(pArea->builders);
        }

        if (pArea->builders[0] != '\0')
        {
            strcat(buf, pArea->builders);
            strcat(buf, " ");
        }
        strcat(buf, name);
        free_string(pArea->builders);
        pArea->builders = string_proper(str_dup(buf));

        sendf(ch, "Builder added: [%s]\r\n", pArea->builders);
        return TRUE;
    }

    return FALSE;
}

AEDIT(aedit_vnum)
{
    AREA_DATA *pArea;
    char lower[MAX_STRING_LENGTH];
    char upper[MAX_STRING_LENGTH];
    int ilower;
    int iupper;

    EDIT_AREA(ch, pArea);

    argument = one_argument(argument, lower);
    one_argument(argument, upper);

    if (!is_number(lower) || lower[0] == '\0'
        || !is_number(upper) || upper[0] == '\0')
    {
        send_to_char("Syntax:  vnum [#xlower] [#xupper]\r\n", ch);
        return FALSE;
    }

    if ((ilower = atoi(lower)) > (iupper = atoi(upper)))
    {
        send_to_char("AEdit:  Upper must be larger then lower.\r\n", ch);
        return FALSE;
    }

    if (!check_range(atoi(lower), atoi(upper)))
    {
        send_to_char("AEdit:  Range must include only this area.\r\n", ch);
        return FALSE;
    }

    if (get_vnum_area(ilower) && get_vnum_area(ilower) != pArea)
    {
        send_to_char("AEdit:  Lower vnum already assigned.\r\n", ch);
        return FALSE;
    }

    pArea->min_vnum = ilower;
    send_to_char("Lower vnum set.\r\n", ch);

    if (get_vnum_area(iupper) && get_vnum_area(iupper) != pArea)
    {
        send_to_char("AEdit:  Upper vnum already assigned.\r\n", ch);
        return TRUE;            /* The lower value has been set. */
    }

    pArea->max_vnum = iupper;
    send_to_char("Upper vnum set.\r\n", ch);

    return TRUE;
}

AEDIT(aedit_lvnum)
{
    AREA_DATA *pArea;
    char lower[MAX_STRING_LENGTH];
    int ilower;
    int iupper;

    EDIT_AREA(ch, pArea);

    one_argument(argument, lower);

    if (!is_number(lower) || lower[0] == '\0')
    {
        send_to_char("Syntax:  min_vnum [#xlower]\r\n", ch);
        return FALSE;
    }

    if ((ilower = atoi(lower)) > (iupper = pArea->max_vnum))
    {
        send_to_char("AEdit:  Value must be less than the max_vnum.\r\n",
            ch);
        return FALSE;
    }

    if (!check_range(ilower, iupper))
    {
        send_to_char("AEdit:  Range must include only this area.\r\n", ch);
        return FALSE;
    }

    if (get_vnum_area(ilower) && get_vnum_area(ilower) != pArea)
    {
        send_to_char("AEdit:  Lower vnum already assigned.\r\n", ch);
        return FALSE;
    }

    pArea->min_vnum = ilower;
    send_to_char("Lower vnum set.\r\n", ch);
    return TRUE;
}

AEDIT(aedit_uvnum)
{
    AREA_DATA *pArea;
    char upper[MAX_STRING_LENGTH];
    int ilower;
    int iupper;

    EDIT_AREA(ch, pArea);

    one_argument(argument, upper);

    if (!is_number(upper) || upper[0] == '\0')
    {
        send_to_char("Syntax:  max_vnum [#xupper]\r\n", ch);
        return FALSE;
    }

    if ((ilower = pArea->min_vnum) > (iupper = atoi(upper)))
    {
        send_to_char("AEdit:  Upper must be larger then lower.\r\n", ch);
        return FALSE;
    }

    if (!check_range(ilower, iupper))
    {
        send_to_char("AEdit:  Range must include only this area.\r\n", ch);
        return FALSE;
    }

    if (get_vnum_area(iupper) && get_vnum_area(iupper) != pArea)
    {
        send_to_char("AEdit:  Upper vnum already assigned.\r\n", ch);
        return FALSE;
    }

    pArea->max_vnum = iupper;
    send_to_char("Upper vnum set.\r\n", ch);

    return TRUE;
}

AEDIT(aedit_levelrange)
{
    AREA_DATA *pArea;
    char lower[MAX_STRING_LENGTH];
    char upper[MAX_STRING_LENGTH];
    int ilower;
    int iupper;

    EDIT_AREA(ch, pArea);

    argument = one_argument(argument, lower);
    one_argument(argument, upper);

    if (!is_number(lower) || lower[0] == '\0'
        || !is_number(upper) || upper[0] == '\0')
    {
        send_to_char("Syntax:  levelrange [#xlower] [#xupper]\r\n", ch);
        return FALSE;
    }

    if ((ilower = atoi(lower)) > (iupper = atoi(upper)))
    {
        send_to_char("AEdit:  Upper must be larger then lower.\r\n", ch);
        return FALSE;
    }

    pArea->min_level = ilower;
    pArea->max_level = iupper;
    send_to_char("Level range set.\r\n", ch);

    return TRUE;

} // end aedit_levelrange

/*
 * Room Editor Functions.
 */
REDIT(redit_show)
{
    ROOM_INDEX_DATA *pRoom;
    char buf[MAX_STRING_LENGTH];
    char buf1[2 * MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    CHAR_DATA *rch;
    int door;
    bool fcnt;

    EDIT_ROOM(ch, pRoom);

    buf1[0] = '\0';

    sprintf(buf, "Description:\r\n%s", pRoom->description);
    strcat(buf1, buf);

    sprintf(buf, "Name:       [%s]\r\nArea:       [%5d] %s\r\n",
        pRoom->name, pRoom->area->vnum, pRoom->area->name);
    strcat(buf1, buf);

    sprintf(buf, "Vnum:       [%5d]\r\nSector:     [%s]\r\n",
        pRoom->vnum, flag_string(sector_flags, pRoom->sector_type));
    strcat(buf1, buf);

    sprintf(buf, "Room flags: [%s]\r\n",
        flag_string(room_flags, pRoom->room_flags));
    strcat(buf1, buf);

    sprintf(buf, "Health rec: [%d]\r\nMana rec  : [%d]\r\n",
        pRoom->heal_rate, pRoom->mana_rate);
    strcat(buf1, buf);

    if (pRoom->clan > 0)
    {
        sprintf(buf, "Clan      : [%d] (%s)\r\n", pRoom->clan, clan_table[pRoom->clan].friendly_name);
        strcat(buf1, buf);
    }
    else
    {
        strcat(buf1, "Clan      : [0] (None)\r\n");
    }

    if (!IS_NULLSTR(pRoom->owner))
    {
        sprintf(buf, "Owner     : [%s]\r\n", pRoom->owner);
        strcat(buf1, buf);
    }

    if (pRoom->extra_descr)
    {
        EXTRA_DESCR_DATA *ed;

        strcat(buf1, "Desc Kwds:  [");
        for (ed = pRoom->extra_descr; ed; ed = ed->next)
        {
            strcat(buf1, ed->keyword);
            if (ed->next)
                strcat(buf1, " ");
        }
        strcat(buf1, "]\r\n");
    }

    strcat(buf1, "Characters: [");
    fcnt = FALSE;
    for (rch = pRoom->people; rch; rch = rch->next_in_room)
    {
        one_argument(rch->name, buf);
        strcat(buf1, buf);
        strcat(buf1, " ");
        fcnt = TRUE;
    }

    if (fcnt)
    {
        int end;

        end = strlen(buf1) - 1;
        buf1[end] = ']';
        strcat(buf1, "\r\n");
    }
    else
        strcat(buf1, "none]\r\n");

    strcat(buf1, "Objects:    [");
    fcnt = FALSE;
    for (obj = pRoom->contents; obj; obj = obj->next_content)
    {
        one_argument(obj->name, buf);
        strcat(buf1, buf);
        strcat(buf1, " ");
        fcnt = TRUE;
    }

    if (fcnt)
    {
        int end;

        end = strlen(buf1) - 1;
        buf1[end] = ']';
        strcat(buf1, "\r\n");
    }
    else
        strcat(buf1, "none]\r\n");

    for (door = 0; door < MAX_DIR; door++)
    {
        EXIT_DATA *pexit;

        if ((pexit = pRoom->exit[door]))
        {
            char word[MAX_INPUT_LENGTH];
            char reset_state[MAX_STRING_LENGTH];
            char *state;
            int i, length;

            sprintf(buf, "-%-5s to [%5d] Key: [%5d] ",
                capitalize(dir_name[door]),
                pexit->u1.to_room ? pexit->u1.to_room->vnum : 0,    /* ROM OLC */
                pexit->key);
            strcat(buf1, buf);

            /*
            * Format up the exit info.
            * Capitalize all flags that are not part of the reset info.
            */
            strcpy(reset_state, flag_string(exit_flags, pexit->rs_flags));
            state = flag_string(exit_flags, pexit->exit_info);
            strcat(buf1, " Exit flags: [");
            for (;;)
            {
                state = one_argument(state, word);

                if (word[0] == '\0')
                {
                    int end;

                    end = strlen(buf1) - 1;
                    buf1[end] = ']';
                    strcat(buf1, "\r\n");
                    break;
                }

                if (str_infix(word, reset_state))
                {
                    length = strlen(word);
                    for (i = 0; i < length; i++)
                        word[i] = UPPER(word[i]);
                }
                strcat(buf1, word);
                strcat(buf1, " ");
            }

            if (pexit->keyword && pexit->keyword[0] != '\0')
            {
                sprintf(buf, "Kwds: [%s]\r\n", pexit->keyword);
                strcat(buf1, buf);
            }
            if (pexit->description && pexit->description[0] != '\0')
            {
                sprintf(buf, "%s", pexit->description);
                strcat(buf1, buf);
            }
        }
    }

    send_to_char(buf1, ch);
    return FALSE;
}

/* Local function. */
bool change_exit(CHAR_DATA * ch, char *argument, int door)
{
    ROOM_INDEX_DATA *pRoom;
    char command[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    int value;

    EDIT_ROOM(ch, pRoom);

    /*
    * Set the exit flags, needs full argument.
    * ----------------------------------------
    */
    if ((value = flag_value(exit_flags, argument)) != NO_FLAG)
    {
        ROOM_INDEX_DATA *pToRoom;
        int rev;                /* ROM OLC */

        if (!pRoom->exit[door])
        {
            send_to_char("Exit doesn't exist.\r\n", ch);
            return FALSE;
        }

        /*
        * This room.
        */
        TOGGLE_BIT(pRoom->exit[door]->rs_flags, value);
        /* Don't toggle exit_info because it can be changed by players. */
        pRoom->exit[door]->exit_info = pRoom->exit[door]->rs_flags;

        /*
        * Connected room.
        */
        pToRoom = pRoom->exit[door]->u1.to_room;    /* ROM OLC */
        rev = rev_dir[door];

        if (pToRoom->exit[rev] != NULL)
        {
            pToRoom->exit[rev]->rs_flags = pRoom->exit[door]->rs_flags;
            pToRoom->exit[rev]->exit_info = pRoom->exit[door]->exit_info;
        }

        send_to_char("Exit flag toggled.\r\n", ch);
        return TRUE;
    }

    /*
    * Now parse the arguments.
    */
    argument = one_argument(argument, command);
    one_argument(argument, arg);

    if (command[0] == '\0' && argument[0] == '\0')
    {                            /* Move command. */
        move_char(ch, door, TRUE);    /* ROM OLC */
        return FALSE;
    }

    if (command[0] == '?')
    {
        do_help(ch, "EXIT");
        return FALSE;
    }

    if (!str_cmp(command, "delete"))
    {
        ROOM_INDEX_DATA *pToRoom;
        int rev;                /* ROM OLC */

        if (!pRoom->exit[door])
        {
            send_to_char("REdit:  Cannot delete a null exit.\r\n", ch);
            return FALSE;
        }

        /*
        * Remove ToRoom Exit.
        */
        rev = rev_dir[door];
        pToRoom = pRoom->exit[door]->u1.to_room;    /* ROM OLC */

        if (pToRoom->exit[rev])
        {
            free_exit(pToRoom->exit[rev]);
            pToRoom->exit[rev] = NULL;
        }

        /*
        * Remove this exit.
        */
        free_exit(pRoom->exit[door]);
        pRoom->exit[door] = NULL;

        send_to_char("Exit unlinked.\r\n", ch);
        return TRUE;
    }

    if (!str_cmp(command, "link") || !str_cmp(command, "twoway"))
    {
        EXIT_DATA *pExit;
        ROOM_INDEX_DATA *toRoom;

        if (arg[0] == '\0' || !is_number(arg))
        {
            send_to_char("Syntax:  [direction] link [vnum]\r\n", ch);
            return FALSE;
        }

        value = atoi(arg);

        if (!(toRoom = get_room_index(value)))
        {
            send_to_char("REdit:  Cannot link to non-existant room.\r\n",
                ch);
            return FALSE;
        }

        if (!IS_BUILDER(ch, toRoom->area))
        {
            send_to_char("REdit:  Cannot link to that area.\r\n", ch);
            return FALSE;
        }

        if (toRoom->exit[rev_dir[door]])
        {
            send_to_char("REdit:  Remote side's exit already exists.\r\n",
                ch);
            return FALSE;
        }

        if (!pRoom->exit[door])
            pRoom->exit[door] = new_exit();

        pRoom->exit[door]->u1.to_room = toRoom;
        pRoom->exit[door]->orig_door = door;

        door = rev_dir[door];
        pExit = new_exit();
        pExit->u1.to_room = pRoom;
        pExit->orig_door = door;
        toRoom->exit[door] = pExit;

        send_to_char("Two-way link established.\r\n", ch);
        return TRUE;
    }

    if (!str_cmp(command, "dig"))
    {
        char buf[MAX_STRING_LENGTH];

        if (arg[0] == '\0' || !is_number(arg))
        {
            send_to_char("Syntax: [direction] dig <vnum>\r\n", ch);
            return FALSE;
        }

        redit_create(ch, arg);
        sprintf(buf, "link %s", arg);
        change_exit(ch, buf, door);
        return TRUE;
    }

    if (!str_cmp(command, "room") || !str_cmp(command, "oneway"))
    {
        ROOM_INDEX_DATA *toRoom;

        if (arg[0] == '\0' || !is_number(arg))
        {
            send_to_char("Syntax:  [direction] room [vnum]\r\n", ch);
            return FALSE;
        }

        value = atoi(arg);

        if (!(toRoom = get_room_index(value)))
        {
            send_to_char("REdit:  Cannot link to non-existant room.\r\n",
                ch);
            return FALSE;
        }

        if (!pRoom->exit[door])
            pRoom->exit[door] = new_exit();

        pRoom->exit[door]->u1.to_room = toRoom;    /* ROM OLC */
        pRoom->exit[door]->orig_door = door;

        send_to_char("One-way link established.\r\n", ch);
        return TRUE;
    }

    if (!str_cmp(command, "key"))
    {
        OBJ_INDEX_DATA *key;

        if (arg[0] == '\0' || !is_number(arg))
        {
            send_to_char("Syntax:  [direction] key [vnum]\r\n", ch);
            return FALSE;
        }

        if (!pRoom->exit[door])
        {
            send_to_char("Exit doesn't exist.\r\n", ch);
            return FALSE;
        }

        value = atoi(arg);

        if (!(key = get_obj_index(value)))
        {
            send_to_char("REdit:  Key doesn't exist.\r\n", ch);
            return FALSE;
        }

        if (key->item_type != ITEM_KEY)
        {
            send_to_char("REdit:  Object is not a key.\r\n", ch);
            return FALSE;
        }

        pRoom->exit[door]->key = value;

        send_to_char("Exit key set.\r\n", ch);
        return TRUE;
    }

    if (!str_cmp(command, "name"))
    {
        if (arg[0] == '\0')
        {
            send_to_char("Syntax:  [direction] name [string]\r\n", ch);
            send_to_char("         [direction] name none\r\n", ch);
            return FALSE;
        }

        if (!pRoom->exit[door])
        {
            send_to_char("Exit doesn't exist.\r\n", ch);
            return FALSE;
        }

        free_string(pRoom->exit[door]->keyword);

        if (str_cmp(arg, "none"))
            pRoom->exit[door]->keyword = str_dup(arg);
        else
            pRoom->exit[door]->keyword = str_dup("");

        send_to_char("Exit name set.\r\n", ch);
        return TRUE;
    }

    if (!str_prefix(command, "description"))
    {
        if (arg[0] == '\0')
        {
            if (!pRoom->exit[door])
            {
                send_to_char("Exit doesn't exist.\r\n", ch);
                return FALSE;
            }

            string_append(ch, &pRoom->exit[door]->description);
            return TRUE;
        }

        send_to_char("Syntax:  [direction] desc\r\n", ch);
        return FALSE;
    }

    return FALSE;
}

REDIT(redit_north)
{
    if (change_exit(ch, argument, DIR_NORTH))
        return TRUE;

    return FALSE;
}

REDIT(redit_south)
{
    if (change_exit(ch, argument, DIR_SOUTH))
        return TRUE;

    return FALSE;
}

REDIT(redit_east)
{
    if (change_exit(ch, argument, DIR_EAST))
        return TRUE;

    return FALSE;
}

REDIT(redit_west)
{
    if (change_exit(ch, argument, DIR_WEST))
        return TRUE;

    return FALSE;
}

REDIT(redit_up)
{
    if (change_exit(ch, argument, DIR_UP))
        return TRUE;

    return FALSE;
}

REDIT(redit_down)
{
    if (change_exit(ch, argument, DIR_DOWN))
        return TRUE;

    return FALSE;
}

REDIT(redit_northeast)
{
    if (change_exit(ch, argument, DIR_NORTHEAST))
        return TRUE;

    return FALSE;
}

REDIT(redit_northwest)
{
    if (change_exit(ch, argument, DIR_NORTHWEST))
        return TRUE;

    return FALSE;
}

REDIT(redit_southeast)
{
    if (change_exit(ch, argument, DIR_SOUTHEAST))
        return TRUE;

    return FALSE;
}

REDIT(redit_southwest)
{
    if (change_exit(ch, argument, DIR_SOUTHWEST))
        return TRUE;

    return FALSE;
}

REDIT(redit_ed)
{
    ROOM_INDEX_DATA *pRoom;
    EXTRA_DESCR_DATA *ed;
    char command[MAX_INPUT_LENGTH];
    char keyword[MAX_INPUT_LENGTH];

    EDIT_ROOM(ch, pRoom);

    argument = one_argument(argument, command);
    one_argument(argument, keyword);

    if (command[0] == '\0' || keyword[0] == '\0')
    {
        send_to_char("Syntax:  ed add [keyword]\r\n", ch);
        send_to_char("         ed edit [keyword]\r\n", ch);
        send_to_char("         ed delete [keyword]\r\n", ch);
        send_to_char("         ed format [keyword]\r\n", ch);
        return FALSE;
    }

    if (!str_cmp(command, "add"))
    {
        if (keyword[0] == '\0')
        {
            send_to_char("Syntax:  ed add [keyword]\r\n", ch);
            return FALSE;
        }

        ed = new_extra_descr();
        ed->keyword = str_dup(keyword);
        ed->description = str_dup("");
        ed->next = pRoom->extra_descr;
        pRoom->extra_descr = ed;

        string_append(ch, &ed->description);

        return TRUE;
    }


    if (!str_cmp(command, "edit"))
    {
        if (keyword[0] == '\0')
        {
            send_to_char("Syntax:  ed edit [keyword]\r\n", ch);
            return FALSE;
        }

        for (ed = pRoom->extra_descr; ed; ed = ed->next)
        {
            if (is_name(keyword, ed->keyword))
                break;
        }

        if (!ed)
        {
            send_to_char("REdit:  Extra description keyword not found.\r\n",
                ch);
            return FALSE;
        }

        string_append(ch, &ed->description);

        return TRUE;
    }


    if (!str_cmp(command, "delete"))
    {
        EXTRA_DESCR_DATA *ped = NULL;

        if (keyword[0] == '\0')
        {
            send_to_char("Syntax:  ed delete [keyword]\r\n", ch);
            return FALSE;
        }

        for (ed = pRoom->extra_descr; ed; ed = ed->next)
        {
            if (is_name(keyword, ed->keyword))
                break;
            ped = ed;
        }

        if (!ed)
        {
            send_to_char("REdit:  Extra description keyword not found.\r\n",
                ch);
            return FALSE;
        }

        if (!ped)
            pRoom->extra_descr = ed->next;
        else
            ped->next = ed->next;

        free_extra_descr(ed);

        send_to_char("Extra description deleted.\r\n", ch);
        return TRUE;
    }


    if (!str_cmp(command, "format"))
    {
        if (keyword[0] == '\0')
        {
            send_to_char("Syntax:  ed format [keyword]\r\n", ch);
            return FALSE;
        }

        for (ed = pRoom->extra_descr; ed; ed = ed->next)
        {
            if (is_name(keyword, ed->keyword))
                break;
        }

        if (!ed)
        {
            send_to_char("REdit:  Extra description keyword not found.\r\n",
                ch);
            return FALSE;
        }

        ed->description = format_string(ed->description);

        send_to_char("Extra description formatted.\r\n", ch);
        return TRUE;
    }

    redit_ed(ch, "");
    return FALSE;
}

REDIT(redit_create)
{
    AREA_DATA *pArea;
    ROOM_INDEX_DATA *pRoom;
    int value;
    int iHash;

    EDIT_ROOM(ch, pRoom);

    value = atoi(argument);

    if (argument[0] == '\0' || value <= 0)
    {
        send_to_char("Syntax:  create [vnum > 0]\r\n", ch);
        return FALSE;
    }

    pArea = get_vnum_area(value);
    if (!pArea)
    {
        send_to_char("REdit:  That vnum is not assigned an area.\r\n", ch);
        return FALSE;
    }

    if (!IS_BUILDER(ch, pArea))
    {
        send_to_char("REdit:  Vnum in an area you cannot build in.\r\n", ch);
        return FALSE;
    }

    if (get_room_index(value))
    {
        send_to_char("REdit:  Room vnum already exists.\r\n", ch);
        return FALSE;
    }

    pRoom = new_room_index();
    pRoom->area = pArea;
    pRoom->vnum = value;

    if (value > top_vnum_room)
        top_vnum_room = value;

    iHash = value % MAX_KEY_HASH;
    pRoom->next = room_index_hash[iHash];
    room_index_hash[iHash] = pRoom;
    ch->desc->pEdit = (void *)pRoom;

    send_to_char("Room created.\r\n", ch);
    return TRUE;
}

REDIT(redit_name)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  name [name]\r\n", ch);
        return FALSE;
    }

    free_string(pRoom->name);
    pRoom->name = str_dup(argument);

    send_to_char("Name set.\r\n", ch);
    return TRUE;
}

REDIT(redit_desc)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    if (argument[0] == '\0')
    {
        string_append(ch, &pRoom->description);
        return TRUE;
    }

    send_to_char("Syntax:  desc\r\n", ch);
    return FALSE;
}

REDIT(redit_heal)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    if (is_number(argument))
    {
        pRoom->heal_rate = atoi(argument);
        send_to_char("Heal rate set.\r\n", ch);
        return TRUE;
    }

    send_to_char("Syntax: heal <#xnumber>\r\n", ch);
    return FALSE;
}

REDIT(redit_mana)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    if (is_number(argument))
    {
        pRoom->mana_rate = atoi(argument);
        send_to_char("Mana rate set.\r\n", ch);
        return TRUE;
    }

    send_to_char("Syntax: mana <#xnumber>\r\n", ch);
    return FALSE;
}

REDIT(redit_clan)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    pRoom->clan = clan_lookup(argument);

    send_to_char("Clan set.\r\n", ch);
    return TRUE;
}

REDIT(redit_format)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    pRoom->description = format_string(pRoom->description);

    send_to_char("String formatted.\r\n", ch);
    return TRUE;
}

REDIT(redit_mreset)
{
    ROOM_INDEX_DATA *pRoom;
    MOB_INDEX_DATA *pMobIndex;
    CHAR_DATA *newmob;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    RESET_DATA *pReset;
    char output[MAX_STRING_LENGTH];

    EDIT_ROOM(ch, pRoom);

    argument = one_argument(argument, arg);
    argument = one_argument(argument, arg2);

    if (arg[0] == '\0' || !is_number(arg))
    {
        send_to_char("Syntax:  mreset <vnum> <max #x> <mix #x>\r\n", ch);
        return FALSE;
    }

    if (!(pMobIndex = get_mob_index(atoi(arg))))
    {
        send_to_char("REdit: No mobile has that vnum.\r\n", ch);
        return FALSE;
    }

    if (pMobIndex->area != pRoom->area)
    {
        send_to_char("REdit: No such mobile in this area.\r\n", ch);
        return FALSE;
    }

    /*
     * Create the mobile reset.
     */
    pReset = new_reset_data();
    pReset->command = 'M';
    pReset->arg1 = pMobIndex->vnum;
    pReset->arg2 = is_number(arg2) ? atoi(arg2) : MAX_MOB;
    pReset->arg3 = pRoom->vnum;
    pReset->arg4 = is_number(argument) ? atoi(argument) : 1;
    add_reset(pRoom, pReset, 0 /* Last slot */);

    /*
     * Create the mobile.
     */
    newmob = create_mobile(pMobIndex);
    char_to_room(newmob, pRoom);

    sprintf(output, "%s (%d) has been loaded and added to resets.\r\n"
        "There will be a maximum of %d loaded to this room.\r\n",
        capitalize(pMobIndex->short_descr),
        pMobIndex->vnum, pReset->arg2);
    send_to_char(output, ch);
    act("$n has created $N!", ch, NULL, newmob, TO_ROOM);
    return TRUE;
}

struct wear_type {
    int wear_loc;
    int wear_bit;
};

const struct wear_type wear_table[] = {
    { WEAR_NONE, ITEM_TAKE },
    { WEAR_LIGHT, ITEM_LIGHT },
    { WEAR_FINGER_L, ITEM_WEAR_FINGER },
    { WEAR_FINGER_R, ITEM_WEAR_FINGER },
    { WEAR_NECK_1, ITEM_WEAR_NECK },
    { WEAR_NECK_2, ITEM_WEAR_NECK },
    { WEAR_BODY, ITEM_WEAR_BODY },
    { WEAR_HEAD, ITEM_WEAR_HEAD },
    { WEAR_LEGS, ITEM_WEAR_LEGS },
    { WEAR_FEET, ITEM_WEAR_FEET },
    { WEAR_HANDS, ITEM_WEAR_HANDS },
    { WEAR_ARMS, ITEM_WEAR_ARMS },
    { WEAR_SHIELD, ITEM_WEAR_SHIELD },
    { WEAR_ABOUT, ITEM_WEAR_ABOUT },
    { WEAR_WAIST, ITEM_WEAR_WAIST },
    { WEAR_WRIST_L, ITEM_WEAR_WRIST },
    { WEAR_WRIST_R, ITEM_WEAR_WRIST },
    { WEAR_WIELD, ITEM_WIELD },
    { WEAR_HOLD, ITEM_HOLD },
    { NO_FLAG, NO_FLAG }
};

/*****************************************************************************
Name:       wear_loc
Purpose:    Returns the location of the bit that matches the count.
            1 = first match, 2 = second match etc.
Called by:  oedit_reset(olc_act.c).
****************************************************************************/
int wear_loc(int bits, int count)
{
    int flag;

    for (flag = 0; wear_table[flag].wear_bit != NO_FLAG; flag++)
    {
        if (IS_SET(bits, wear_table[flag].wear_bit) && --count < 1)
            return wear_table[flag].wear_loc;
    }

    return NO_FLAG;
}

/*****************************************************************************
Name:       wear_bit
Purpose:    Converts a wear_loc into a bit.
Called by:  redit_oreset(olc_act.c).
****************************************************************************/
int wear_bit(int loc)
{
    int flag;

    for (flag = 0; wear_table[flag].wear_loc != NO_FLAG; flag++)
    {
        if (loc == wear_table[flag].wear_loc)
            return wear_table[flag].wear_bit;
    }

    return 0;
}

REDIT(redit_oreset)
{
    ROOM_INDEX_DATA *pRoom;
    OBJ_INDEX_DATA *pObjIndex;
    OBJ_DATA *newobj;
    OBJ_DATA *to_obj;
    CHAR_DATA *to_mob;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int olevel = 0;

    RESET_DATA *pReset;
    char output[MAX_STRING_LENGTH];

    EDIT_ROOM(ch, pRoom);

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || !is_number(arg1))
    {
        send_to_char("Syntax:  oreset <vnum> <args>\r\n", ch);
        send_to_char("        -no_args               = into room\r\n", ch);
        send_to_char("        -<obj_name>            = into obj\r\n", ch);
        send_to_char("        -<mob_name> <wear_loc> = into mob\r\n", ch);
        return FALSE;
    }

    if (!(pObjIndex = get_obj_index(atoi(arg1))))
    {
        send_to_char("REdit: No object has that vnum.\r\n", ch);
        return FALSE;
    }

    if (pObjIndex->area != pRoom->area)
    {
        send_to_char("REdit: No such object in this area.\r\n", ch);
        return FALSE;
    }

    /*
    * Load into room.
    */
    if (arg2[0] == '\0')
    {
        pReset = new_reset_data();
        pReset->command = 'O';
        pReset->arg1 = pObjIndex->vnum;
        pReset->arg2 = 0;
        pReset->arg3 = pRoom->vnum;
        pReset->arg4 = 0;
        add_reset(pRoom, pReset, 0 /* Last slot */);

        newobj = create_object(pObjIndex);
        obj_to_room(newobj, pRoom);

        sprintf(output, "%s (%d) has been loaded and added to resets.\r\n",
            capitalize(pObjIndex->short_descr), pObjIndex->vnum);
        send_to_char(output, ch);
    }
    else
        /*
        * Load into object's inventory.
        */
        if (argument[0] == '\0'
            && ((to_obj = get_obj_list(ch, arg2, pRoom->contents)) != NULL))
        {
            pReset = new_reset_data();
            pReset->command = 'P';
            pReset->arg1 = pObjIndex->vnum;
            pReset->arg2 = 0;
            pReset->arg3 = to_obj->pIndexData->vnum;
            pReset->arg4 = 1;
            add_reset(pRoom, pReset, 0 /* Last slot */);

            newobj = create_object(pObjIndex);
            newobj->cost = 0;
            obj_to_obj(newobj, to_obj);

            sprintf(output, "%s (%d) has been loaded into "
                "%s (%d) and added to resets.\r\n",
                capitalize(newobj->short_descr),
                newobj->pIndexData->vnum,
                to_obj->short_descr, to_obj->pIndexData->vnum);
            send_to_char(output, ch);
        }
        else
            /*
            * Load into mobile's inventory.
            */
            if ((to_mob = get_char_room(ch, arg2)) != NULL)
            {
                int wear_loc;

                /*
                * Make sure the location on mobile is valid.
                */
                if ((wear_loc = flag_value(wear_loc_flags, argument)) == NO_FLAG)
                {
                    send_to_char("REdit: Invalid wear_loc.  '? wear-loc'\r\n", ch);
                    return FALSE;
                }

                /*
                * Disallow loading a sword(WEAR_WIELD) into WEAR_HEAD.
                */
                if (!IS_SET(pObjIndex->wear_flags, wear_bit(wear_loc)))
                {
                    sprintf(output,
                        "%s (%d) has wear flags: [%s]\r\n",
                        capitalize(pObjIndex->short_descr),
                        pObjIndex->vnum,
                        flag_string(wear_flags, pObjIndex->wear_flags));
                    send_to_char(output, ch);
                    return FALSE;
                }

                /*
                * Can't load into same position.
                */
                if (get_eq_char(to_mob, wear_loc))
                {
                    send_to_char("REdit:  Object already equipped.\r\n", ch);
                    return FALSE;
                }

                pReset = new_reset_data();
                pReset->arg1 = pObjIndex->vnum;
                pReset->arg2 = wear_loc;
                if (pReset->arg2 == WEAR_NONE)
                    pReset->command = 'G';
                else
                    pReset->command = 'E';
                pReset->arg3 = wear_loc;

                add_reset(pRoom, pReset, 0 /* Last slot */);

                olevel = URANGE(0, to_mob->level - 2, LEVEL_HERO);
                newobj = create_object(pObjIndex);

                if (to_mob->pIndexData->pShop)
                {                        /* Shop-keeper? */
                    switch (pObjIndex->item_type)
                    {
                        default:
                            olevel = 0;
                            break;
                        case ITEM_PILL:
                            olevel = number_range(0, 10);
                            break;
                        case ITEM_POTION:
                            olevel = number_range(0, 10);
                            break;
                        case ITEM_SCROLL:
                            olevel = number_range(5, 15);
                            break;
                        case ITEM_WAND:
                            olevel = number_range(10, 20);
                            break;
                        case ITEM_STAFF:
                            olevel = number_range(15, 25);
                            break;
                        case ITEM_ARMOR:
                            olevel = number_range(5, 15);
                            break;
                        case ITEM_WEAPON:
                            if (pReset->command == 'G')
                                olevel = number_range(5, 15);
                            else
                                olevel = number_fuzzy(olevel);
                            break;
                    }

                    newobj = create_object(pObjIndex);
                    if (pReset->arg2 == WEAR_NONE)
                        SET_BIT(newobj->extra_flags, ITEM_INVENTORY);
                }
                else
                    newobj = create_object(pObjIndex);

                obj_to_char(newobj, to_mob);
                if (pReset->command == 'E')
                    equip_char(to_mob, newobj, pReset->arg3);

                sprintf(output, "%s (%d) has been loaded "
                    "%s of %s (%d) and added to resets.\r\n",
                    capitalize(pObjIndex->short_descr),
                    pObjIndex->vnum,
                    flag_string(wear_loc_strings, pReset->arg3),
                    to_mob->short_descr, to_mob->pIndexData->vnum);
                send_to_char(output, ch);
            }
            else
            {                            /* Display Syntax */

                send_to_char("REdit:  That mobile isn't here.\r\n", ch);
                return FALSE;
            }

    act("$n has created $p!", ch, newobj, NULL, TO_ROOM);
    return TRUE;
}

/*
 * Object Editor Functions.
 */
void show_obj_values(CHAR_DATA * ch, OBJ_INDEX_DATA * obj)
{
    char buf[MAX_STRING_LENGTH];

    switch (obj->item_type)
    {
        default:                /* No values. */
            break;

        case ITEM_LIGHT:
            if (obj->value[2] == -1 || obj->value[2] == 999)    /* ROM OLC */
                sprintf(buf, "[v2] Light:  Infinite[-1]\r\n");
            else
                sprintf(buf, "[v2] Light:  [%d]\r\n", obj->value[2]);
            send_to_char(buf, ch);
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            sprintf(buf,
                "[v0] Level:          [%d]\r\n"
                "[v1] Charges Total:  [%d]\r\n"
                "[v2] Charges Left:   [%d]\r\n"
                "[v3] Spell:          %s\r\n",
                obj->value[0],
                obj->value[1],
                obj->value[2],
                obj->value[3] != -1 ? skill_table[obj->value[3]]->name
                : "none");
            send_to_char(buf, ch);
            break;

        case ITEM_PORTAL:
            sprintf(buf,
                "[v0] Charges:        [%d]\r\n"
                "[v1] Exit Flags:     %s\r\n"
                "[v2] Portal Flags:   %s\r\n"
                "[v3] Goes to (vnum): [%d]\r\n",
                obj->value[0],
                flag_string(exit_flags, obj->value[1]),
                flag_string(portal_flags, obj->value[2]),
                obj->value[3]);
            send_to_char(buf, ch);
            break;

        case ITEM_FURNITURE:
            sprintf(buf,
                "[v0] Max people:      [%d]\r\n"
                "[v1] Max weight:      [%d]\r\n"
                "[v2] Furniture Flags: %s\r\n"
                "[v3] Heal bonus:      [%d]\r\n"
                "[v4] Mana bonus:      [%d]\r\n",
                obj->value[0],
                obj->value[1],
                flag_string(furniture_flags, obj->value[2]),
                obj->value[3], obj->value[4]);
            send_to_char(buf, ch);
            break;

        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            sprintf(buf,
                "[v0] Level:  [%d]\r\n"
                "[v1] Spell:  %s\r\n"
                "[v2] Spell:  %s\r\n"
                "[v3] Spell:  %s\r\n"
                "[v4] Spell:  %s\r\n",
                obj->value[0],
                obj->value[1] != -1 ? skill_table[obj->value[1]]->name
                : "none",
                obj->value[2] != -1 ? skill_table[obj->value[2]]->name
                : "none",
                obj->value[3] != -1 ? skill_table[obj->value[3]]->name
                : "none",
                obj->value[4] != -1 ? skill_table[obj->value[4]]->name
                : "none");
            send_to_char(buf, ch);
            break;

            /* ARMOR for ROM */

        case ITEM_ARMOR:
            sprintf(buf,
                "[v0] Ac pierce       [%d]\r\n"
                "[v1] Ac bash         [%d]\r\n"
                "[v2] Ac slash        [%d]\r\n"
                "[v3] Ac exotic       [%d]\r\n",
                obj->value[0], obj->value[1], obj->value[2],
                obj->value[3]);
            send_to_char(buf, ch);
            break;

            /* WEAPON changed in ROM: */
            /* I had to split the output here, I have no idea why, but it helped -- Hugin */
            /* It somehow fixed a bug in showing scroll/pill/potions too ?! */
        case ITEM_WEAPON:
            sprintf(buf, "[v0] Weapon class:   %s\r\n",
                flag_string(weapon_class, obj->value[0]));
            send_to_char(buf, ch);
            sprintf(buf, "[v1] Number of dice: [%d]\r\n", obj->value[1]);
            send_to_char(buf, ch);
            sprintf(buf, "[v2] Type of dice:   [%d]\r\n", obj->value[2]);
            send_to_char(buf, ch);
            sprintf(buf, "[v3] Type:           %s\r\n",
                attack_table[obj->value[3]].name);
            send_to_char(buf, ch);
            sprintf(buf, "[v4] Special type:   %s\r\n",
                flag_string(weapon_type2, obj->value[4]));
            send_to_char(buf, ch);
            break;
        case ITEM_FOG:
            sprintf(buf, "[v0] Fog Density:    [%d]\r\n", obj->value[0]);
            send_to_char(buf, ch);
            break;
        case ITEM_CONTAINER:
            sprintf(buf,
                "[v0] Weight:     [%d kg]\r\n"
                "[v1] Flags:      [%s]\r\n"
                "[v2] Key:        [%d] %s\r\n"
                "[v3] Capacity    [%d]\r\n"
                "[v4] Weight Mult [%d]\r\n",
                obj->value[0],
                flag_string(container_flags, obj->value[1]),
                obj->value[2],
                get_obj_index(obj->value[2]) ? get_obj_index(obj->value[2])->short_descr : "none",
                obj->value[3], obj->value[4]);
            send_to_char(buf, ch);
            break;

        case ITEM_DRINK_CON:
            sprintf(buf,
                "[v0] Liquid Total: [%d]\r\n"
                "[v1] Liquid Left:  [%d]\r\n"
                "[v2] Liquid:       %s\r\n"
                "[v3] Poisoned:     %s\r\n",
                obj->value[0],
                obj->value[1],
                liquid_table[obj->value[2]].liq_name,
                obj->value[3] != 0 ? "Yes" : "No");
            send_to_char(buf, ch);
            break;

        case ITEM_SEED:
            sprintf(buf,
                "[v0] Vnum to Grow: [%d]\r\n"
                "[v1] Percent Bonus: [%d]\r\n",
                obj->value[0],
                obj->value[1]);
            send_to_char(buf, ch);
            break;

        case ITEM_FOUNTAIN:
            sprintf(buf,
                "[v0] Liquid Total: [%d]\r\n"
                "[v1] Liquid Left:  [%d]\r\n"
                "[v2] Liquid:        %s\r\n",
                obj->value[0],
                obj->value[1], liquid_table[obj->value[2]].liq_name);
            send_to_char(buf, ch);
            break;

        case ITEM_FOOD:
            sprintf(buf,
                "[v0] Food hours: [%d]\r\n"
                "[v1] Full hours: [%d]\r\n"
                "[v3] Poisoned:   %s\r\n",
                obj->value[0],
                obj->value[1], obj->value[3] != 0 ? "Yes" : "No");
            send_to_char(buf, ch);
            break;

        case ITEM_MONEY:
            sprintf(buf, "[v0] Silver: [%d]\r\n", obj->value[0]);
            send_to_char(buf, ch);
            sprintf(buf, "[v1] Gold    [%d]\r\n", obj->value[1]);
            send_to_char(buf, ch);
            break;
    }

    return;
}

bool set_obj_values(CHAR_DATA * ch, OBJ_INDEX_DATA * pObj, int value_num, char *argument)
{
    switch (pObj->item_type)
    {
        default:
            break;

        case ITEM_LIGHT:
            switch (value_num)
            {
                default:
                    do_help(ch, "ITEM_LIGHT");
                    return FALSE;
                case 2:
                    send_to_char("HOURS OF LIGHT SET.\r\n\r\n", ch);
                    pObj->value[2] = atoi(argument);
                    break;
            }
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            switch (value_num)
            {
                default:
                    do_help(ch, "ITEM_STAFF_WAND");
                    return FALSE;
                case 0:
                    send_to_char("SPELL LEVEL SET.\r\n\r\n", ch);
                    pObj->value[0] = atoi(argument);
                    break;
                case 1:
                    send_to_char("TOTAL NUMBER OF CHARGES SET.\r\n\r\n", ch);
                    pObj->value[1] = atoi(argument);
                    break;
                case 2:
                    send_to_char("CURRENT NUMBER OF CHARGES SET.\r\n\r\n",
                        ch);
                    pObj->value[2] = atoi(argument);
                    break;
                case 3:
                    send_to_char("SPELL TYPE SET.\r\n", ch);
                    pObj->value[3] = skill_lookup(argument);
                    break;
            }
            break;

        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            switch (value_num)
            {
                default:
                    do_help(ch, "ITEM_SCROLL_POTION_PILL");
                    return FALSE;
                case 0:
                    send_to_char("SPELL LEVEL SET.\r\n\r\n", ch);
                    pObj->value[0] = atoi(argument);
                    break;
                case 1:
                    send_to_char("SPELL TYPE 1 SET.\r\n\r\n", ch);
                    pObj->value[1] = skill_lookup(argument);
                    break;
                case 2:
                    send_to_char("SPELL TYPE 2 SET.\r\n\r\n", ch);
                    pObj->value[2] = skill_lookup(argument);
                    break;
                case 3:
                    send_to_char("SPELL TYPE 3 SET.\r\n\r\n", ch);
                    pObj->value[3] = skill_lookup(argument);
                    break;
                case 4:
                    send_to_char("SPELL TYPE 4 SET.\r\n\r\n", ch);
                    pObj->value[4] = skill_lookup(argument);
                    break;
            }
            break;

            /* ARMOR for ROM: */

        case ITEM_ARMOR:
            switch (value_num)
            {
                default:
                    do_help(ch, "ITEM_ARMOR");
                    return FALSE;
                case 0:
                    send_to_char("AC PIERCE SET.\r\n\r\n", ch);
                    pObj->value[0] = atoi(argument);
                    break;
                case 1:
                    send_to_char("AC BASH SET.\r\n\r\n", ch);
                    pObj->value[1] = atoi(argument);
                    break;
                case 2:
                    send_to_char("AC SLASH SET.\r\n\r\n", ch);
                    pObj->value[2] = atoi(argument);
                    break;
                case 3:
                    send_to_char("AC EXOTIC SET.\r\n\r\n", ch);
                    pObj->value[3] = atoi(argument);
                    break;
            }
            break;

            /* WEAPONS changed in ROM */

        case ITEM_WEAPON:
            switch (value_num)
            {
                default:
                    do_help(ch, "ITEM_WEAPON");
                    return FALSE;
                case 0:
                    send_to_char("WEAPON CLASS SET.\r\n\r\n", ch);
                    ALT_FLAGVALUE_SET(pObj->value[0], weapon_class,
                        argument);
                    break;
                case 1:
                    send_to_char("NUMBER OF DICE SET.\r\n\r\n", ch);
                    pObj->value[1] = atoi(argument);
                    break;
                case 2:
                    send_to_char("TYPE OF DICE SET.\r\n\r\n", ch);
                    pObj->value[2] = atoi(argument);
                    break;
                case 3:
                    send_to_char("WEAPON TYPE SET.\r\n\r\n", ch);
                    pObj->value[3] = attack_lookup(argument);
                    break;
                case 4:
                    send_to_char("SPECIAL WEAPON TYPE TOGGLED.\r\n\r\n", ch);
                    ALT_FLAGVALUE_TOGGLE(pObj->value[4], weapon_type2,
                        argument);
                    break;
            }
            break;
        case ITEM_FOG:
            if (atoi(argument) < 0 || atoi(argument) > 100)
            {
                send_to_char("Fog density should be 0 to 100.\r\n\r\n", ch);
                return FALSE;
            }

            switch (value_num)
            {
                default:
                    send_to_char("Fog Density (v0) should be 0 to 100.\r\n\r\n", ch);
                    return FALSE;
                case 0:
                    send_to_char("Fog density set.\r\n\r\n", ch);
                    pObj->value[0] = atoi(argument);
                    break;
            }
            break;
        case ITEM_SEED:
            switch (value_num)
            {
                default:
                    send_to_char("Nature System:  Grow Seed.\r\n", ch);
                    return FALSE;
                case 0:
                    send_to_char( "Vnum of object to grow SET.\r\n\r\n", ch);
                    pObj->value[0] = atoi(argument);
                    break;
                case 1:
                    send_to_char( "Bonus for growing SET.\r\n\r\n", ch);
                    pObj->value[1] = atoi(argument);
                    break;
            }

            break;
        case ITEM_PORTAL:
            switch (value_num)
            {
                default:
                    do_help(ch, "ITEM_PORTAL");
                    return FALSE;

                case 0:
                    send_to_char("CHARGES SET.\r\n\r\n", ch);
                    pObj->value[0] = atoi(argument);
                    break;
                case 1:
                    send_to_char("EXIT FLAGS SET.\r\n\r\n", ch);
                    ALT_FLAGVALUE_SET(pObj->value[1], exit_flags, argument);
                    break;
                case 2:
                    send_to_char("PORTAL FLAGS SET.\r\n\r\n", ch);
                    ALT_FLAGVALUE_SET(pObj->value[2], portal_flags,
                        argument);
                    break;
                case 3:
                    send_to_char("EXIT VNUM SET.\r\n\r\n", ch);
                    pObj->value[3] = atoi(argument);
                    break;
            }
            break;

        case ITEM_FURNITURE:
            switch (value_num)
            {
                default:
                    do_help(ch, "ITEM_FURNITURE");
                    return FALSE;

                case 0:
                    send_to_char("NUMBER OF PEOPLE SET.\r\n\r\n", ch);
                    pObj->value[0] = atoi(argument);
                    break;
                case 1:
                    send_to_char("MAX WEIGHT SET.\r\n\r\n", ch);
                    pObj->value[1] = atoi(argument);
                    break;
                case 2:
                    send_to_char("FURNITURE FLAGS TOGGLED.\r\n\r\n", ch);
                    ALT_FLAGVALUE_TOGGLE(pObj->value[2], furniture_flags,
                        argument);
                    break;
                case 3:
                    send_to_char("HEAL BONUS SET.\r\n\r\n", ch);
                    pObj->value[3] = atoi(argument);
                    break;
                case 4:
                    send_to_char("MANA BONUS SET.\r\n\r\n", ch);
                    pObj->value[4] = atoi(argument);
                    break;
            }
            break;

        case ITEM_CONTAINER:
            switch (value_num)
            {
                int value;

                default:
                    do_help(ch, "ITEM_CONTAINER");
                    return FALSE;
                case 0:
                    send_to_char("WEIGHT CAPACITY SET.\r\n\r\n", ch);
                    pObj->value[0] = atoi(argument);
                    break;
                case 1:
                    if ((value = flag_value(container_flags, argument)) !=
                        NO_FLAG)
                        TOGGLE_BIT(pObj->value[1], value);
                    else
                    {
                        do_help(ch, "ITEM_CONTAINER");
                        return FALSE;
                    }
                    send_to_char("CONTAINER TYPE SET.\r\n\r\n", ch);
                    break;
                case 2:
                    if (atoi(argument) != 0)
                    {
                        if (!get_obj_index(atoi(argument)))
                        {
                            send_to_char("THERE IS NO SUCH ITEM.\r\n\r\n",
                                ch);
                            return FALSE;
                        }

                        if (get_obj_index(atoi(argument))->item_type !=
                            ITEM_KEY)
                        {
                            send_to_char("THAT ITEM IS NOT A KEY.\r\n\r\n",
                                ch);
                            return FALSE;
                        }
                    }
                    send_to_char("CONTAINER KEY SET.\r\n\r\n", ch);
                    pObj->value[2] = atoi(argument);
                    break;
                case 3:
                    send_to_char("CONTAINER MAX WEIGHT SET.\r\n", ch);
                    pObj->value[3] = atoi(argument);
                    break;
                case 4:
                    send_to_char("WEIGHT MULTIPLIER SET.\r\n\r\n", ch);
                    pObj->value[4] = atoi(argument);
                    break;
            }
            break;

        case ITEM_DRINK_CON:
            switch (value_num)
            {
                default:
                    do_help(ch, "ITEM_DRINK");
                    /* OLC            do_help( ch, "liquids" );    */
                    return FALSE;
                case 0:
                    send_to_char
                        ("MAXIMUM AMOUT OF LIQUID HOURS SET.\r\n\r\n", ch);
                    pObj->value[0] = atoi(argument);
                    break;
                case 1:
                    send_to_char
                        ("CURRENT AMOUNT OF LIQUID HOURS SET.\r\n\r\n", ch);
                    pObj->value[1] = atoi(argument);
                    break;
                case 2:
                    send_to_char("LIQUID TYPE SET.\r\n\r\n", ch);
                    pObj->value[2] = (liq_lookup(argument) != -1 ?
                        liq_lookup(argument) : 0);
                    break;
                case 3:
                    send_to_char("POISON VALUE TOGGLED.\r\n\r\n", ch);
                    pObj->value[3] = (pObj->value[3] == 0) ? 1 : 0;
                    break;
            }
            break;

        case ITEM_FOUNTAIN:
            switch (value_num)
            {
                default:
                    do_help(ch, "ITEM_FOUNTAIN");
                    /* OLC            do_help( ch, "liquids" );    */
                    return FALSE;
                case 0:
                    send_to_char
                        ("MAXIMUM AMOUT OF LIQUID HOURS SET.\r\n\r\n", ch);
                    pObj->value[0] = atoi(argument);
                    break;
                case 1:
                    send_to_char
                        ("CURRENT AMOUNT OF LIQUID HOURS SET.\r\n\r\n", ch);
                    pObj->value[1] = atoi(argument);
                    break;
                case 2:
                    send_to_char("LIQUID TYPE SET.\r\n\r\n", ch);
                    pObj->value[2] = (liq_lookup(argument) != -1 ?
                        liq_lookup(argument) : 0);
                    break;
            }
            break;

        case ITEM_FOOD:
            switch (value_num)
            {
                default:
                    do_help(ch, "ITEM_FOOD");
                    return FALSE;
                case 0:
                    send_to_char("HOURS OF FOOD SET.\r\n\r\n", ch);
                    pObj->value[0] = atoi(argument);
                    break;
                case 1:
                    send_to_char("HOURS OF FULL SET.\r\n\r\n", ch);
                    pObj->value[1] = atoi(argument);
                    break;
                case 3:
                    send_to_char("POISON VALUE TOGGLED.\r\n\r\n", ch);
                    pObj->value[3] = (pObj->value[3] == 0) ? 1 : 0;
                    break;
            }
            break;

        case ITEM_MONEY:
            switch (value_num)
            {
                default:
                    do_help(ch, "ITEM_MONEY");
                    return FALSE;
                case 0:
                    send_to_char("SILVER AMOUNT SET.\r\n\r\n", ch);
                    pObj->value[0] = atoi(argument);
                    break;
                case 1:
                    send_to_char("GOLD AMOUNT SET.\r\n\r\n", ch);
                    pObj->value[1] = atoi(argument);
                    break;
            }
            break;
    }

    show_obj_values(ch, pObj);

    return TRUE;
}

OEDIT(oedit_show)
{
    OBJ_INDEX_DATA *pObj;
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA *paf;
    int cnt;

    EDIT_OBJ(ch, pObj);

    sprintf(buf, "Name:        [%s]\r\nArea:        [%5d] %s\r\n",
        pObj->name,
        !pObj->area ? -1 : pObj->area->vnum,
        !pObj->area ? "No Area" : pObj->area->name);
    send_to_char(buf, ch);


    sprintf(buf, "Vnum:        [%5d]\r\nType:        [%s]\r\n",
        pObj->vnum, flag_string(type_flags, pObj->item_type));
    send_to_char(buf, ch);

    sprintf(buf, "Level:       [%5d]\r\n", pObj->level);
    send_to_char(buf, ch);

    sprintf(buf, "Wear flags:  [%s]\r\n",
        flag_string(wear_flags, pObj->wear_flags));
    send_to_char(buf, ch);

    sprintf(buf, "Extra flags: [%s]\r\n",
        flag_string(extra_flags, pObj->extra_flags));
    send_to_char(buf, ch);

    sprintf(buf, "Material:    [%s]\r\n",    /* ROM */
        pObj->material);
    send_to_char(buf, ch);

    sprintf(buf, "Condition:   [%5d]\r\n",    /* ROM */
        pObj->condition);
    send_to_char(buf, ch);

    sprintf(buf, "Weight:      [%5d]\r\nCost:        [%5d]\r\n",
        pObj->weight, pObj->cost);
    send_to_char(buf, ch);

    if (pObj->extra_descr)
    {
        EXTRA_DESCR_DATA *ed;

        send_to_char("Ex desc kwd: ", ch);

        for (ed = pObj->extra_descr; ed; ed = ed->next)
        {
            send_to_char("[", ch);
            send_to_char(ed->keyword, ch);
            send_to_char("]", ch);
        }

        send_to_char("\r\n", ch);
    }

    sprintf(buf, "Short desc:  %s\r\nLong desc:\r\n     %s\r\n",
        pObj->short_descr, pObj->description);
    send_to_char(buf, ch);

    for (cnt = 0, paf = pObj->affected; paf; paf = paf->next)
    {
        if (cnt == 0)
        {
            send_to_char("Number Modifier Affects\r\n", ch);
            send_to_char("------ -------- -------\r\n", ch);
        }
        sprintf(buf, "[%4d] %-8d %s\r\n", cnt,
            paf->modifier, flag_string(apply_flags, paf->location));
        send_to_char(buf, ch);
        cnt++;
    }

    show_obj_values(ch, pObj);

    return FALSE;
}

/*
 * Need to issue warning if flag isn't valid. -- does so now -- Hugin.
 */
OEDIT(oedit_addaffect)
{
    int value;
    OBJ_INDEX_DATA *pObj;
    AFFECT_DATA *pAf;
    char loc[MAX_STRING_LENGTH];
    char mod[MAX_STRING_LENGTH];

    EDIT_OBJ(ch, pObj);

    argument = one_argument(argument, loc);
    one_argument(argument, mod);

    if (loc[0] == '\0' || mod[0] == '\0' || !is_number(mod))
    {
        send_to_char("Syntax:  addaffect [location] [#xmod]\r\n", ch);
        return FALSE;
    }

    if ((value = flag_value(apply_flags, loc)) == NO_FLAG)
    {                            /* Hugin */
        send_to_char("Valid affects are:\r\n", ch);
        show_help(ch, "apply");
        return FALSE;
    }

    pAf = new_affect();
    pAf->location = value;
    pAf->modifier = atoi(mod);
    pAf->where = TO_OBJECT;
    pAf->type = -1;
    pAf->duration = -1;
    pAf->bitvector = 0;
    pAf->level = pObj->level;
    pAf->next = pObj->affected;
    pObj->affected = pAf;

    send_to_char("Affect added.\r\n", ch);
    return TRUE;
}

OEDIT(oedit_addapply)
{
    int value, bv, typ;
    OBJ_INDEX_DATA *pObj;
    AFFECT_DATA *pAf;
    char loc[MAX_STRING_LENGTH];
    char mod[MAX_STRING_LENGTH];
    char type[MAX_STRING_LENGTH];
    char bvector[MAX_STRING_LENGTH];

    EDIT_OBJ(ch, pObj);

    argument = one_argument(argument, type);
    argument = one_argument(argument, loc);
    argument = one_argument(argument, mod);
    one_argument(argument, bvector);

    if (type[0] == '\0' || (typ = flag_value(apply_types, type)) == NO_FLAG)
    {
        send_to_char("Invalid apply type. Valid apply types are:\r\n", ch);
        show_help(ch, "apptype");
        return FALSE;
    }

    if (loc[0] == '\0' || (value = flag_value(apply_flags, loc)) == NO_FLAG)
    {
        send_to_char("Valid applys are:\r\n", ch);
        show_help(ch, "apply");
        return FALSE;
    }

    if (bvector[0] == '\0'
        || (bv = flag_value(bitvector_type[typ].table, bvector)) == NO_FLAG)
    {
        send_to_char("Invalid bitvector type.\r\n", ch);
        send_to_char("Valid bitvector types are:\r\n", ch);
        show_help(ch, bitvector_type[typ].help);
        return FALSE;
    }

    if (mod[0] == '\0' || !is_number(mod))
    {
        send_to_char
            ("Syntax:  addapply [type] [location] [#xmod] [bitvector]\r\n",
                ch);
        return FALSE;
    }

    pAf = new_affect();
    pAf->location = value;
    pAf->modifier = atoi(mod);
    pAf->where = apply_types[typ].bit;
    pAf->type = -1;
    pAf->duration = -1;
    pAf->bitvector = bv;
    pAf->level = pObj->level;
    pAf->next = pObj->affected;
    pObj->affected = pAf;

    send_to_char("Apply added.\r\n", ch);
    return TRUE;
}

/*
* My thanks to Hans Hvidsten Birkeland and Noam Krendel(Walker)
* for really teaching me how to manipulate pointers.
*/
OEDIT(oedit_delaffect)
{
    OBJ_INDEX_DATA *pObj;
    AFFECT_DATA *pAf;
    AFFECT_DATA *pAf_next;
    char affect[MAX_STRING_LENGTH];
    int value;
    int cnt = 0;

    EDIT_OBJ(ch, pObj);

    one_argument(argument, affect);

    if (!is_number(affect) || affect[0] == '\0')
    {
        send_to_char("Syntax:  delaffect [#xaffect]\r\n", ch);
        return FALSE;
    }

    value = atoi(affect);

    if (value < 0)
    {
        send_to_char("Only non-negative affect-numbers allowed.\r\n", ch);
        return FALSE;
    }

    if (!(pAf = pObj->affected))
    {
        send_to_char("OEdit:  Non-existant affect.\r\n", ch);
        return FALSE;
    }

    if (value == 0)
    {                            /* First case: Remove first affect */
        pAf = pObj->affected;
        pObj->affected = pAf->next;
        free_affect(pAf);
    }
    else
    {                            /* Affect to remove is not the first */

        while ((pAf_next = pAf->next) && (++cnt < value))
            pAf = pAf_next;

        if (pAf_next)
        {                        /* See if it's the next affect */
            pAf->next = pAf_next->next;
            free_affect(pAf_next);
        }
        else
        {                        /* Doesn't exist */

            send_to_char("No such affect.\r\n", ch);
            return FALSE;
        }
    }

    send_to_char("Affect removed.\r\n", ch);
    return TRUE;
}



OEDIT(oedit_name)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  name [string]\r\n", ch);
        return FALSE;
    }

    free_string(pObj->name);
    pObj->name = str_dup(argument);

    send_to_char("Name set.\r\n", ch);
    return TRUE;
}



OEDIT(oedit_short)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  short [string]\r\n", ch);
        return FALSE;
    }

    free_string(pObj->short_descr);
    pObj->short_descr = str_dup(argument);
    pObj->short_descr[0] = LOWER(pObj->short_descr[0]);

    send_to_char("Short description set.\r\n", ch);
    return TRUE;
}



OEDIT(oedit_long)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  long [string]\r\n", ch);
        return FALSE;
    }

    free_string(pObj->description);
    pObj->description = str_dup(argument);
    pObj->description[0] = UPPER(pObj->description[0]);

    send_to_char("Long description set.\r\n", ch);
    return TRUE;
}



bool set_value(CHAR_DATA * ch, OBJ_INDEX_DATA * pObj, char *argument,
    int value)
{
    if (argument[0] == '\0')
    {
        set_obj_values(ch, pObj, -1, "");    /* '\0' changed to "" -- Hugin */
        return FALSE;
    }

    if (set_obj_values(ch, pObj, value, argument))
        return TRUE;

    return FALSE;
}



/*****************************************************************************
Name:        oedit_values
Purpose:    Finds the object and sets its value.
Called by:    The four valueX functions below. (now five -- Hugin )
****************************************************************************/
bool oedit_values(CHAR_DATA * ch, char *argument, int value)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if (set_value(ch, pObj, argument, value))
        return TRUE;

    return FALSE;
}


OEDIT(oedit_value0)
{
    if (oedit_values(ch, argument, 0))
        return TRUE;

    return FALSE;
}



OEDIT(oedit_value1)
{
    if (oedit_values(ch, argument, 1))
        return TRUE;

    return FALSE;
}



OEDIT(oedit_value2)
{
    if (oedit_values(ch, argument, 2))
        return TRUE;

    return FALSE;
}



OEDIT(oedit_value3)
{
    if (oedit_values(ch, argument, 3))
        return TRUE;

    return FALSE;
}



OEDIT(oedit_value4)
{
    if (oedit_values(ch, argument, 4))
        return TRUE;

    return FALSE;
}



OEDIT(oedit_weight)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if (argument[0] == '\0' || !is_number(argument))
    {
        send_to_char("Syntax:  weight [number]\r\n", ch);
        return FALSE;
    }

    pObj->weight = atoi(argument);

    send_to_char("Weight set.\r\n", ch);
    return TRUE;
}

OEDIT(oedit_cost)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if (argument[0] == '\0' || !is_number(argument))
    {
        send_to_char("Syntax:  cost [number]\r\n", ch);
        return FALSE;
    }

    pObj->cost = atoi(argument);

    send_to_char("Cost set.\r\n", ch);
    return TRUE;
}



OEDIT(oedit_create)
{
    OBJ_INDEX_DATA *pObj;
    AREA_DATA *pArea;
    int value;
    int iHash;

    value = atoi(argument);
    if (argument[0] == '\0' || value == 0)
    {
        send_to_char("Syntax:  oedit create [vnum]\r\n", ch);
        return FALSE;
    }

    pArea = get_vnum_area(value);
    if (!pArea)
    {
        send_to_char("OEdit:  That vnum is not assigned an area.\r\n", ch);
        return FALSE;
    }

    if (!IS_BUILDER(ch, pArea))
    {
        send_to_char("OEdit:  Vnum in an area you cannot build in.\r\n", ch);
        return FALSE;
    }

    if (get_obj_index(value))
    {
        send_to_char("OEdit:  Object vnum already exists.\r\n", ch);
        return FALSE;
    }

    pObj = new_obj_index();
    pObj->vnum = value;
    pObj->area = pArea;

    if (value > top_vnum_obj)
        top_vnum_obj = value;

    iHash = value % MAX_KEY_HASH;
    pObj->next = obj_index_hash[iHash];
    obj_index_hash[iHash] = pObj;
    ch->desc->pEdit = (void *)pObj;

    send_to_char("Object Created.\r\n", ch);
    return TRUE;
}



OEDIT(oedit_ed)
{
    OBJ_INDEX_DATA *pObj;
    EXTRA_DESCR_DATA *ed;
    char command[MAX_INPUT_LENGTH];
    char keyword[MAX_INPUT_LENGTH];

    EDIT_OBJ(ch, pObj);

    argument = one_argument(argument, command);
    one_argument(argument, keyword);

    if (command[0] == '\0')
    {
        send_to_char("Syntax:  ed add [keyword]\r\n", ch);
        send_to_char("         ed delete [keyword]\r\n", ch);
        send_to_char("         ed edit [keyword]\r\n", ch);
        send_to_char("         ed format [keyword]\r\n", ch);
        return FALSE;
    }

    if (!str_cmp(command, "add"))
    {
        if (keyword[0] == '\0')
        {
            send_to_char("Syntax:  ed add [keyword]\r\n", ch);
            return FALSE;
        }

        ed = new_extra_descr();
        ed->keyword = str_dup(keyword);
        ed->next = pObj->extra_descr;
        pObj->extra_descr = ed;

        string_append(ch, &ed->description);

        return TRUE;
    }

    if (!str_cmp(command, "edit"))
    {
        if (keyword[0] == '\0')
        {
            send_to_char("Syntax:  ed edit [keyword]\r\n", ch);
            return FALSE;
        }

        for (ed = pObj->extra_descr; ed; ed = ed->next)
        {
            if (is_name(keyword, ed->keyword))
                break;
        }

        if (!ed)
        {
            send_to_char("OEdit:  Extra description keyword not found.\r\n",
                ch);
            return FALSE;
        }

        string_append(ch, &ed->description);

        return TRUE;
    }

    if (!str_cmp(command, "delete"))
    {
        EXTRA_DESCR_DATA *ped = NULL;

        if (keyword[0] == '\0')
        {
            send_to_char("Syntax:  ed delete [keyword]\r\n", ch);
            return FALSE;
        }

        for (ed = pObj->extra_descr; ed; ed = ed->next)
        {
            if (is_name(keyword, ed->keyword))
                break;
            ped = ed;
        }

        if (!ed)
        {
            send_to_char("OEdit:  Extra description keyword not found.\r\n",
                ch);
            return FALSE;
        }

        if (!ped)
            pObj->extra_descr = ed->next;
        else
            ped->next = ed->next;

        free_extra_descr(ed);

        send_to_char("Extra description deleted.\r\n", ch);
        return TRUE;
    }

    // Removed the ped variable like above because it wasn't being used which may or may not be, not
    // sure on the intent.  Will test.
    if (!str_cmp(command, "format"))
    {
        if (keyword[0] == '\0')
        {
            send_to_char("Syntax:  ed format [keyword]\r\n", ch);
            return FALSE;
        }

        for (ed = pObj->extra_descr; ed; ed = ed->next)
        {
            if (is_name(keyword, ed->keyword))
                break;
        }

        if (!ed)
        {
            send_to_char("OEdit:  Extra description keyword not found.\r\n",
                ch);
            return FALSE;
        }

        ed->description = format_string(ed->description);

        send_to_char("Extra description formatted.\r\n", ch);
        return TRUE;
    }

    oedit_ed(ch, "");
    return FALSE;
}





/* ROM object functions : */

OEDIT(oedit_extra)
{                                /* Moved out of oedit() due to naming conflicts -- Hugin */
    OBJ_INDEX_DATA *pObj;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_OBJ(ch, pObj);

        if ((value = flag_value(extra_flags, argument)) != NO_FLAG)
        {
            TOGGLE_BIT(pObj->extra_flags, value);

            send_to_char("Extra flag toggled.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char("Syntax:  extra [flag]\r\n"
        "Type '? extra' for a list of flags.\r\n", ch);
    return FALSE;
}


OEDIT(oedit_wear)
{                                /* Moved out of oedit() due to naming conflicts -- Hugin */
    OBJ_INDEX_DATA *pObj;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_OBJ(ch, pObj);

        if ((value = flag_value(wear_flags, argument)) != NO_FLAG)
        {
            TOGGLE_BIT(pObj->wear_flags, value);

            send_to_char("Wear flag toggled.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char("Syntax:  wear [flag]\r\n"
        "Type '? wear' for a list of flags.\r\n", ch);
    return FALSE;
}


OEDIT(oedit_type)
{                                /* Moved out of oedit() due to naming conflicts -- Hugin */
    OBJ_INDEX_DATA *pObj;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_OBJ(ch, pObj);

        if ((value = flag_value(type_flags, argument)) != NO_FLAG)
        {
            pObj->item_type = value;

            send_to_char("Type set.\r\n", ch);

            /*
            * Clear the values.
            */
            pObj->value[0] = 0;
            pObj->value[1] = 0;
            pObj->value[2] = 0;
            pObj->value[3] = 0;
            pObj->value[4] = 0;    /* ROM */

            return TRUE;
        }
    }

    send_to_char("Syntax:  type [flag]\r\n"
        "Type '? type' for a list of flags.\r\n", ch);
    return FALSE;
}

OEDIT(oedit_material)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  material [string]\r\n", ch);
        return FALSE;
    }

    free_string(pObj->material);
    pObj->material = str_dup(argument);

    send_to_char("Material set.\r\n", ch);
    return TRUE;
}

OEDIT(oedit_level)
{
    OBJ_INDEX_DATA *pObj;

    EDIT_OBJ(ch, pObj);

    if (argument[0] == '\0' || !is_number(argument))
    {
        send_to_char("Syntax:  level [number]\r\n", ch);
        return FALSE;
    }

    pObj->level = atoi(argument);

    send_to_char("Level set.\r\n", ch);
    return TRUE;
}



OEDIT(oedit_condition)
{
    OBJ_INDEX_DATA *pObj;
    int value;

    if (argument[0] != '\0'
        && (value = atoi(argument)) >= 0 && (value <= 100))
    {
        EDIT_OBJ(ch, pObj);

        pObj->condition = value;
        send_to_char("Condition set.\r\n", ch);

        return TRUE;
    }

    send_to_char("Syntax:  condition [number]\r\n"
        "Where number can range from 0 (ruined) to 100 (perfect).\r\n",
        ch);
    return FALSE;
}





/*
* Mobile Editor Functions.
*/
MEDIT(medit_show)
{
    MOB_INDEX_DATA *pMob;
    char buf[MAX_STRING_LENGTH];
    MPROG_LIST *list;

    EDIT_MOB(ch, pMob);

    sprintf(buf, "Name:        [%s]\r\nArea:        [%5d] %s\r\n",
        pMob->player_name,
        !pMob->area ? -1 : pMob->area->vnum,
        !pMob->area ? "No Area" : pMob->area->name);
    send_to_char(buf, ch);

    sprintf(buf, "Act:         [%s]\r\n",
        flag_string(act_flags, pMob->act));
    send_to_char(buf, ch);

    sprintf(buf, "Vnum:        [%5d] Sex:   [%s]   Race: [%s]\r\n",
        pMob->vnum,
        pMob->sex == SEX_MALE ? "male   " :
        pMob->sex == SEX_FEMALE ? "female " :
        pMob->sex == 3 ? "random " : "neutral",
        race_table[pMob->race].name);
    send_to_char(buf, ch);

    sprintf(buf,
        "Level:       [%2d]    Align: [%4d]      Hitroll: [%2d] Dam Type:    [%s]\r\n",
        pMob->level, pMob->alignment,
        pMob->hitroll, attack_table[pMob->dam_type].name);
    send_to_char(buf, ch);

    if (pMob->group)
    {
        sprintf(buf, "Group:       [%5d]\r\n", pMob->group);
        send_to_char(buf, ch);
    }

    sprintf(buf, "Hit dice:    [%2dd%-3d+%4d] ",
        pMob->hit[DICE_NUMBER],
        pMob->hit[DICE_TYPE], pMob->hit[DICE_BONUS]);
    send_to_char(buf, ch);

    sprintf(buf, "Damage dice: [%2dd%-3d+%4d] ",
        pMob->damage[DICE_NUMBER],
        pMob->damage[DICE_TYPE], pMob->damage[DICE_BONUS]);
    send_to_char(buf, ch);

    sprintf(buf, "Mana dice:   [%2dd%-3d+%4d]\r\n",
        pMob->mana[DICE_NUMBER],
        pMob->mana[DICE_TYPE], pMob->mana[DICE_BONUS]);
    send_to_char(buf, ch);

    /* ROM values end */

    sprintf(buf, "Affected by: [%s]\r\n",
        flag_string(affect_flags, pMob->affected_by));
    send_to_char(buf, ch);

    /* ROM values: */

    sprintf(buf,
        "Armor:       [pierce: %d  bash: %d  slash: %d  magic: %d]\r\n",
        pMob->ac[AC_PIERCE], pMob->ac[AC_BASH], pMob->ac[AC_SLASH],
        pMob->ac[AC_EXOTIC]);
    send_to_char(buf, ch);

    sprintf(buf, "Form:        [%s]\r\n",
        flag_string(form_flags, pMob->form));
    send_to_char(buf, ch);

    sprintf(buf, "Parts:       [%s]\r\n",
        flag_string(part_flags, pMob->parts));
    send_to_char(buf, ch);

    sprintf(buf, "Imm:         [%s]\r\n",
        flag_string(imm_flags, pMob->imm_flags));
    send_to_char(buf, ch);

    sprintf(buf, "Res:         [%s]\r\n",
        flag_string(res_flags, pMob->res_flags));
    send_to_char(buf, ch);

    sprintf(buf, "Vuln:        [%s]\r\n",
        flag_string(vuln_flags, pMob->vuln_flags));
    send_to_char(buf, ch);

    sprintf(buf, "Off:         [%s]\r\n",
        flag_string(off_flags, pMob->off_flags));
    send_to_char(buf, ch);

    sprintf(buf, "Size:        [%s]\r\n",
        flag_string(size_flags, pMob->size));
    send_to_char(buf, ch);

    sprintf(buf, "Material:    [%s]\r\n", pMob->material);
    send_to_char(buf, ch);

    sprintf(buf, "Start pos.   [%s]\r\n",
        flag_string(position_flags, pMob->start_pos));
    send_to_char(buf, ch);

    sprintf(buf, "Default pos  [%s]\r\n",
        flag_string(position_flags, pMob->default_pos));
    send_to_char(buf, ch);

    sprintf(buf, "Wealth:      [%5ld]\r\n", pMob->wealth);
    send_to_char(buf, ch);

    /* ROM values end */

    if (pMob->spec_fun)
    {
        sprintf(buf, "Spec fun:    [%s]\r\n", spec_name(pMob->spec_fun));
        send_to_char(buf, ch);
    }

    sprintf(buf, "Short descr: %s\r\nLong descr:\r\n%s",
        pMob->short_descr, pMob->long_descr);
    send_to_char(buf, ch);

    sprintf(buf, "Description:\r\n%s", pMob->description);
    send_to_char(buf, ch);

    if (pMob->pShop)
    {
        SHOP_DATA *pShop;
        int iTrade;

        pShop = pMob->pShop;

        sprintf(buf,
            "Shop data for [%5d]:\r\n"
            "  Markup for purchaser: %d%%\r\n"
            "  Markdown for seller:  %d%%\r\n",
            pShop->keeper, pShop->profit_buy, pShop->profit_sell);
        send_to_char(buf, ch);
        sprintf(buf, "  Hours: %d to %d.\r\n",
            pShop->open_hour, pShop->close_hour);
        send_to_char(buf, ch);

        for (iTrade = 0; iTrade < MAX_TRADE; iTrade++)
        {
            if (pShop->buy_type[iTrade] != 0)
            {
                if (iTrade == 0)
                {
                    send_to_char("  Number Trades Type\r\n", ch);
                    send_to_char("  ------ -----------\r\n", ch);
                }
                sprintf(buf, "  [%4d] %s\r\n", iTrade,
                    flag_string(type_flags, pShop->buy_type[iTrade]));
                send_to_char(buf, ch);
            }
        }
    }

    if (pMob->mprogs)
    {
        int cnt;

        sprintf(buf, "\r\nMOBPrograms for [%5d]:\r\n", pMob->vnum);
        send_to_char(buf, ch);

        for (cnt = 0, list = pMob->mprogs; list; list = list->next)
        {
            if (cnt == 0)
            {
                send_to_char(" Number Vnum Trigger Phrase\r\n", ch);
                send_to_char(" ------ ---- ------- ------\r\n", ch);
            }

            sprintf(buf, "[%5d] %4d %7s %s\r\n", cnt,
                list->vnum, mprog_type_to_name(list->trig_type),
                list->trig_phrase);
            send_to_char(buf, ch);
            cnt++;
        }
    }

    return FALSE;
}



MEDIT(medit_create)
{
    MOB_INDEX_DATA *pMob;
    AREA_DATA *pArea;
    int value;
    int iHash;

    value = atoi(argument);
    if (argument[0] == '\0' || value == 0)
    {
        send_to_char("Syntax:  medit create [vnum]\r\n", ch);
        return FALSE;
    }

    pArea = get_vnum_area(value);

    if (!pArea)
    {
        send_to_char("MEdit:  That vnum is not assigned an area.\r\n", ch);
        return FALSE;
    }

    if (!IS_BUILDER(ch, pArea))
    {
        send_to_char("MEdit:  Vnum in an area you cannot build in.\r\n", ch);
        return FALSE;
    }

    if (get_mob_index(value))
    {
        send_to_char("MEdit:  Mobile vnum already exists.\r\n", ch);
        return FALSE;
    }

    pMob = new_mob_index();
    pMob->vnum = value;
    pMob->area = pArea;

    if (value > top_vnum_mob)
        top_vnum_mob = value;

    pMob->act = ACT_IS_NPC;
    iHash = value % MAX_KEY_HASH;
    pMob->next = mob_index_hash[iHash];
    mob_index_hash[iHash] = pMob;
    ch->desc->pEdit = (void *)pMob;

    send_to_char("Mobile Created.\r\n", ch);
    return TRUE;
}



MEDIT(medit_spec)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  spec [special function]\r\n", ch);
        return FALSE;
    }


    if (!str_cmp(argument, "none"))
    {
        pMob->spec_fun = NULL;

        send_to_char("Spec removed.\r\n", ch);
        return TRUE;
    }

    if (spec_lookup(argument))
    {
        pMob->spec_fun = spec_lookup(argument);
        send_to_char("Spec set.\r\n", ch);
        return TRUE;
    }

    send_to_char("MEdit: No such special function.\r\n", ch);
    return FALSE;
}

MEDIT(medit_damtype)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  damtype [damage message]\r\n", ch);
        send_to_char
            ("For a list of damage types, type '? weapon'.\r\n",
                ch);
        return FALSE;
    }

    pMob->dam_type = attack_lookup(argument);
    send_to_char("Damage type set.\r\n", ch);
    return TRUE;
}


MEDIT(medit_align)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0' || !is_number(argument))
    {
        send_to_char("Syntax:  alignment [number]\r\n", ch);
        send_to_char("         Evil=1, Neutral=2, Good=3\r\n", ch);
        return FALSE;
    }

    if (atoi(argument) < 1 || atoi(argument) > 3)
    {
        send_to_char("Syntax:  alignment [number]\r\n", ch);
        send_to_char("         Evil=1, Neutral=2, Good=3\r\n", ch);
        return FALSE;
    }

    pMob->alignment = atoi(argument);

    send_to_char("Alignment set.\r\n", ch);
    return TRUE;
}



MEDIT(medit_level)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0' || !is_number(argument))
    {
        send_to_char("Syntax:  level [number]\r\n", ch);
        return FALSE;
    }

    pMob->level = atoi(argument);

    send_to_char("Level set.\r\n", ch);
    return TRUE;
}



MEDIT(medit_desc)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
        string_append(ch, &pMob->description);
        return TRUE;
    }

    send_to_char("Syntax:  desc    - line edit\r\n", ch);
    return FALSE;
}




MEDIT(medit_long)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  long [string]\r\n", ch);
        return FALSE;
    }

    free_string(pMob->long_descr);
    strcat(argument, "\r\n");
    pMob->long_descr = str_dup(argument);
    pMob->long_descr[0] = UPPER(pMob->long_descr[0]);

    send_to_char("Long description set.\r\n", ch);
    return TRUE;
}



MEDIT(medit_short)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  short [string]\r\n", ch);
        return FALSE;
    }

    free_string(pMob->short_descr);
    pMob->short_descr = str_dup(argument);

    send_to_char("Short description set.\r\n", ch);
    return TRUE;
}



MEDIT(medit_name)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  name [string]\r\n", ch);
        return FALSE;
    }

    free_string(pMob->player_name);
    pMob->player_name = str_dup(argument);

    send_to_char("Name set.\r\n", ch);
    return TRUE;
}

MEDIT(medit_shop)
{
    MOB_INDEX_DATA *pMob;
    char command[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];

    argument = one_argument(argument, command);
    argument = one_argument(argument, arg1);

    EDIT_MOB(ch, pMob);

    if (command[0] == '\0')
    {
        send_to_char("Syntax:  shop hours [#xopening] [#xclosing]\r\n", ch);
        send_to_char("         shop profit [#xbuying%] [#xselling%]\r\n",
            ch);
        send_to_char("         shop type [#x0-4] [item type]\r\n", ch);
        send_to_char("         shop assign\r\n", ch);
        send_to_char("         shop remove\r\n", ch);
        return FALSE;
    }


    if (!str_cmp(command, "hours"))
    {
        if (arg1[0] == '\0' || !is_number(arg1)
            || argument[0] == '\0' || !is_number(argument))
        {
            send_to_char("Syntax:  shop hours [#xopening] [#xclosing]\r\n",
                ch);
            return FALSE;
        }

        if (!pMob->pShop)
        {
            send_to_char
                ("MEdit:  You must create the shop first (shop assign).\r\n",
                    ch);
            return FALSE;
        }

        pMob->pShop->open_hour = atoi(arg1);
        pMob->pShop->close_hour = atoi(argument);

        send_to_char("Shop hours set.\r\n", ch);
        return TRUE;
    }


    if (!str_cmp(command, "profit"))
    {
        if (arg1[0] == '\0' || !is_number(arg1)
            || argument[0] == '\0' || !is_number(argument))
        {
            send_to_char("Syntax:  shop profit [#xbuying%] [#xselling%]\r\n",
                ch);
            return FALSE;
        }

        if (!pMob->pShop)
        {
            send_to_char
                ("MEdit:  You must create the shop first (shop assign).\r\n",
                    ch);
            return FALSE;
        }

        pMob->pShop->profit_buy = atoi(arg1);
        pMob->pShop->profit_sell = atoi(argument);

        send_to_char("Shop profit set.\r\n", ch);
        return TRUE;
    }


    if (!str_cmp(command, "type"))
    {
        char buf[MAX_INPUT_LENGTH];
        int value;

        if (arg1[0] == '\0' || !is_number(arg1) || argument[0] == '\0')
        {
            send_to_char("Syntax:  shop type [#x0-4] [item type]\r\n", ch);
            return FALSE;
        }

        if (atoi(arg1) >= MAX_TRADE)
        {
            sprintf(buf, "MEdit:  May sell %d items max.\r\n", MAX_TRADE);
            send_to_char(buf, ch);
            return FALSE;
        }

        if (!pMob->pShop)
        {
            send_to_char
                ("MEdit:  You must create the shop first (shop assign).\r\n",
                    ch);
            return FALSE;
        }

        if ((value = flag_value(type_flags, argument)) == NO_FLAG)
        {
            send_to_char("MEdit:  That type of item is not known.\r\n", ch);
            return FALSE;
        }

        pMob->pShop->buy_type[atoi(arg1)] = value;

        send_to_char("Shop type set.\r\n", ch);
        return TRUE;
    }

    /* shop assign && shop delete by Phoenix */

    if (!str_prefix(command, "assign"))
    {
        if (pMob->pShop)
        {
            send_to_char("Mob already has a shop assigned to it.\r\n", ch);
            return FALSE;
        }

        pMob->pShop = new_shop();
        if (!shop_first)
            shop_first = pMob->pShop;
        if (shop_last)
            shop_last->next = pMob->pShop;
        shop_last = pMob->pShop;

        pMob->pShop->keeper = pMob->vnum;

        send_to_char("New shop assigned to mobile.\r\n", ch);
        return TRUE;
    }

    if (!str_prefix(command, "remove"))
    {
        SHOP_DATA *pShop;

        pShop = pMob->pShop;
        pMob->pShop = NULL;

        if (pShop == shop_first)
        {
            if (!pShop->next)
            {
                shop_first = NULL;
                shop_last = NULL;
            }
            else
                shop_first = pShop->next;
        }
        else
        {
            SHOP_DATA *ipShop;

            for (ipShop = shop_first; ipShop; ipShop = ipShop->next)
            {
                if (ipShop->next == pShop)
                {
                    if (!pShop->next)
                    {
                        shop_last = ipShop;
                        shop_last->next = NULL;
                    }
                    else
                        ipShop->next = pShop->next;
                }
            }
        }

        free_shop(pShop);

        send_to_char("Mobile is no longer a shopkeeper.\r\n", ch);
        return TRUE;
    }

    medit_shop(ch, "");
    return FALSE;
}


/* ROM medit functions: */


MEDIT(medit_sex)
{                                /* Moved out of medit() due to naming conflicts -- Hugin */
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB(ch, pMob);

        if ((value = flag_value(sex_flags, argument)) != NO_FLAG)
        {
            pMob->sex = value;

            send_to_char("Sex set.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char("Syntax: sex [sex]\r\n"
        "Type '? sex' for a list of flags.\r\n", ch);
    return FALSE;
}


MEDIT(medit_act)
{                                /* Moved out of medit() due to naming conflicts -- Hugin */
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB(ch, pMob);

        if ((value = flag_value(act_flags, argument)) != NO_FLAG)
        {
            pMob->act ^= value;
            SET_BIT(pMob->act, ACT_IS_NPC);

            send_to_char("Act flag toggled.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char("Syntax: act [flag]\r\n"
        "Type '? act' for a list of flags.\r\n", ch);
    return FALSE;
}


MEDIT(medit_affect)
{                                /* Moved out of medit() due to naming conflicts -- Hugin */
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB(ch, pMob);

        if ((value = flag_value(affect_flags, argument)) != NO_FLAG)
        {
            pMob->affected_by ^= value;

            send_to_char("Affect flag toggled.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char("Syntax: affect [flag]\r\n"
        "Type '? affect' for a list of flags.\r\n", ch);
    return FALSE;
}



MEDIT(medit_ac)
{
    MOB_INDEX_DATA *pMob;
    char arg[MAX_INPUT_LENGTH];
    int pierce, bash, slash, exotic;

    do
    {                            /* So that I can use break and send the syntax in one place */
        if (argument[0] == '\0')
            break;

        EDIT_MOB(ch, pMob);
        argument = one_argument(argument, arg);

        if (!is_number(arg))
            break;
        pierce = atoi(arg);
        argument = one_argument(argument, arg);

        if (arg[0] != '\0')
        {
            if (!is_number(arg))
                break;
            bash = atoi(arg);
            argument = one_argument(argument, arg);
        }
        else
            bash = pMob->ac[AC_BASH];

        if (arg[0] != '\0')
        {
            if (!is_number(arg))
                break;
            slash = atoi(arg);
            argument = one_argument(argument, arg);
        }
        else
            slash = pMob->ac[AC_SLASH];

        if (arg[0] != '\0')
        {
            if (!is_number(arg))
                break;
            exotic = atoi(arg);
        }
        else
            exotic = pMob->ac[AC_EXOTIC];

        pMob->ac[AC_PIERCE] = pierce;
        pMob->ac[AC_BASH] = bash;
        pMob->ac[AC_SLASH] = slash;
        pMob->ac[AC_EXOTIC] = exotic;

        send_to_char("Ac set.\r\n", ch);
        return TRUE;
    } while (FALSE);                /* Just do it once.. */

    send_to_char
        ("Syntax:  ac [ac-pierce [ac-bash [ac-slash [ac-exotic]]]]\r\n"
            "help MOB_AC  gives a list of reasonable ac-values.\r\n", ch);
    return FALSE;
}

MEDIT(medit_form)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB(ch, pMob);

        if ((value = flag_value(form_flags, argument)) != NO_FLAG)
        {
            pMob->form ^= value;
            send_to_char("Form toggled.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char("Syntax: form [flags]\r\n"
        "Type '? form' for a list of flags.\r\n", ch);
    return FALSE;
}

MEDIT(medit_part)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB(ch, pMob);

        if ((value = flag_value(part_flags, argument)) != NO_FLAG)
        {
            pMob->parts ^= value;
            send_to_char("Parts toggled.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char("Syntax: part [flags]\r\n"
        "Type '? part' for a list of flags.\r\n", ch);
    return FALSE;
}

MEDIT(medit_imm)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB(ch, pMob);

        if ((value = flag_value(imm_flags, argument)) != NO_FLAG)
        {
            pMob->imm_flags ^= value;
            send_to_char("Immunity toggled.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char("Syntax: imm [flags]\r\n"
        "Type '? imm' for a list of flags.\r\n", ch);
    return FALSE;
}

MEDIT(medit_res)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB(ch, pMob);

        if ((value = flag_value(res_flags, argument)) != NO_FLAG)
        {
            pMob->res_flags ^= value;
            send_to_char("Resistance toggled.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char("Syntax: res [flags]\r\n"
        "Type '? res' for a list of flags.\r\n", ch);
    return FALSE;
}

MEDIT(medit_vuln)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB(ch, pMob);

        if ((value = flag_value(vuln_flags, argument)) != NO_FLAG)
        {
            pMob->vuln_flags ^= value;
            send_to_char("Vulnerability toggled.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char("Syntax: vuln [flags]\r\n"
        "Type '? vuln' for a list of flags.\r\n", ch);
    return FALSE;
}

MEDIT(medit_material)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  material [string]\r\n", ch);
        return FALSE;
    }

    free_string(pMob->material);
    pMob->material = str_dup(argument);

    send_to_char("Material set.\r\n", ch);
    return TRUE;
}

MEDIT(medit_off)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB(ch, pMob);

        if ((value = flag_value(off_flags, argument)) != NO_FLAG)
        {
            pMob->off_flags ^= value;
            send_to_char("Offensive behaviour toggled.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char("Syntax: off [flags]\r\n"
        "Type '? off' for a list of flags.\r\n", ch);
    return FALSE;
}

MEDIT(medit_size)
{
    MOB_INDEX_DATA *pMob;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_MOB(ch, pMob);

        if ((value = flag_value(size_flags, argument)) != NO_FLAG)
        {
            pMob->size = value;
            send_to_char("Size set.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char("Syntax: size [size]\r\n"
        "Type '? size' for a list of sizes.\r\n", ch);
    return FALSE;
}

MEDIT(medit_hitdice)
{
    static char syntax[] = "Syntax:  hitdice <number> d <type> + <bonus>\r\n";
    char *num, *type, *bonus, *cp;
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
        send_to_char(syntax, ch);
        return FALSE;
    }

    num = cp = argument;

    while (isdigit(*cp))
        ++cp;
    while (*cp != '\0' && !isdigit(*cp))
        *(cp++) = '\0';

    type = cp;

    while (isdigit(*cp))
        ++cp;
    while (*cp != '\0' && !isdigit(*cp))
        *(cp++) = '\0';

    bonus = cp;

    while (isdigit(*cp))
        ++cp;
    if (*cp != '\0')
        *cp = '\0';

    if ((!is_number(num) || atoi(num) < 1)
        || (!is_number(type) || atoi(type) < 1)
        || (!is_number(bonus) || atoi(bonus) < 0))
    {
        send_to_char(syntax, ch);
        return FALSE;
    }

    pMob->hit[DICE_NUMBER] = atoi(num);
    pMob->hit[DICE_TYPE] = atoi(type);
    pMob->hit[DICE_BONUS] = atoi(bonus);

    send_to_char("Hitdice set.\r\n", ch);
    return TRUE;
}

MEDIT(medit_manadice)
{
    static char syntax[] =
        "Syntax:  manadice <number> d <type> + <bonus>\r\n";
    char *num, *type, *bonus, *cp;
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
        send_to_char(syntax, ch);
        return FALSE;
    }

    num = cp = argument;

    while (isdigit(*cp))
        ++cp;
    while (*cp != '\0' && !isdigit(*cp))
        *(cp++) = '\0';

    type = cp;

    while (isdigit(*cp))
        ++cp;
    while (*cp != '\0' && !isdigit(*cp))
        *(cp++) = '\0';

    bonus = cp;

    while (isdigit(*cp))
        ++cp;
    if (*cp != '\0')
        *cp = '\0';

    if (!(is_number(num) && is_number(type) && is_number(bonus)))
    {
        send_to_char(syntax, ch);
        return FALSE;
    }

    if ((!is_number(num) || atoi(num) < 1)
        || (!is_number(type) || atoi(type) < 1)
        || (!is_number(bonus) || atoi(bonus) < 0))
    {
        send_to_char(syntax, ch);
        return FALSE;
    }

    pMob->mana[DICE_NUMBER] = atoi(num);
    pMob->mana[DICE_TYPE] = atoi(type);
    pMob->mana[DICE_BONUS] = atoi(bonus);

    send_to_char("Manadice set.\r\n", ch);
    return TRUE;
}

MEDIT(medit_damdice)
{
    static char syntax[] = "Syntax:  damdice <number> d <type> + <bonus>\r\n";
    char *num, *type, *bonus, *cp;
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
        send_to_char(syntax, ch);
        return FALSE;
    }

    num = cp = argument;

    while (isdigit(*cp))
        ++cp;
    while (*cp != '\0' && !isdigit(*cp))
        *(cp++) = '\0';

    type = cp;

    while (isdigit(*cp))
        ++cp;
    while (*cp != '\0' && !isdigit(*cp))
        *(cp++) = '\0';

    bonus = cp;

    while (isdigit(*cp))
        ++cp;
    if (*cp != '\0')
        *cp = '\0';

    if (!(is_number(num) && is_number(type) && is_number(bonus)))
    {
        send_to_char(syntax, ch);
        return FALSE;
    }

    if ((!is_number(num) || atoi(num) < 1)
        || (!is_number(type) || atoi(type) < 1)
        || (!is_number(bonus) || atoi(bonus) < 0))
    {
        send_to_char(syntax, ch);
        return FALSE;
    }

    pMob->damage[DICE_NUMBER] = atoi(num);
    pMob->damage[DICE_TYPE] = atoi(type);
    pMob->damage[DICE_BONUS] = atoi(bonus);

    send_to_char("Damdice set.\r\n", ch);
    return TRUE;
}


MEDIT(medit_race)
{
    MOB_INDEX_DATA *pMob;
    int race;

    if (argument[0] != '\0' && (race = race_lookup(argument)) != 0)
    {
        EDIT_MOB(ch, pMob);

        pMob->race = race;
        pMob->act |= race_table[race].act;
        pMob->affected_by |= race_table[race].aff;
        pMob->off_flags |= race_table[race].off;
        pMob->imm_flags |= race_table[race].imm;
        pMob->res_flags |= race_table[race].res;
        pMob->vuln_flags |= race_table[race].vuln;
        pMob->form |= race_table[race].form;
        pMob->parts |= race_table[race].parts;

        send_to_char("Race set.\r\n", ch);
        return TRUE;
    }

    if (argument[0] == '?')
    {
        char buf[MAX_STRING_LENGTH];

        send_to_char("Available races are:", ch);

        for (race = 0; race_table[race].name != NULL; race++)
        {
            if ((race % 3) == 0)
                send_to_char("\r\n", ch);
            sprintf(buf, " %-15s", race_table[race].name);
            send_to_char(buf, ch);
        }

        send_to_char("\r\n", ch);
        return FALSE;
    }

    send_to_char("Syntax:  race [race]\r\n"
        "Type 'race ?' for a list of races.\r\n", ch);
    return FALSE;
}


MEDIT(medit_position)
{
    MOB_INDEX_DATA *pMob;
    char arg[MAX_INPUT_LENGTH];
    int value;

    argument = one_argument(argument, arg);

    switch (arg[0])
    {
        default:
            break;

        case 'S':
        case 's':
            if (str_prefix(arg, "start"))
                break;

            if ((value = flag_value(position_flags, argument)) == NO_FLAG)
                break;

            EDIT_MOB(ch, pMob);

            pMob->start_pos = value;
            send_to_char("Start position set.\r\n", ch);
            return TRUE;

        case 'D':
        case 'd':
            if (str_prefix(arg, "default"))
                break;

            if ((value = flag_value(position_flags, argument)) == NO_FLAG)
                break;

            EDIT_MOB(ch, pMob);

            pMob->default_pos = value;
            send_to_char("Default position set.\r\n", ch);
            return TRUE;
    }

    send_to_char("Syntax:  position [start/default] [position]\r\n"
        "Type '? position' for a list of positions.\r\n", ch);
    return FALSE;
}


MEDIT(medit_gold)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0' || !is_number(argument))
    {
        send_to_char("Syntax:  wealth [number]\r\n", ch);
        return FALSE;
    }

    pMob->wealth = atoi(argument);

    send_to_char("Wealth set.\r\n", ch);
    return TRUE;
}

MEDIT(medit_hitroll)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0' || !is_number(argument))
    {
        send_to_char("Syntax:  hitroll [number]\r\n", ch);
        return FALSE;
    }

    pMob->hitroll = atoi(argument);

    send_to_char("Hitroll set.\r\n", ch);
    return TRUE;
}

void show_liqlist(CHAR_DATA * ch)
{
    int liq;
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH];

    buffer = new_buf();

    for (liq = 0; liquid_table[liq].liq_name != NULL; liq++)
    {
        if ((liq % 21) == 0)
            add_buf(buffer,
                "Name                 Color          Proof Full Thirst Food Ssize\r\n");

        sprintf(buf, "%-20s %-14s %5d %4d %6d %4d %5d\r\n",
            liquid_table[liq].liq_name, liquid_table[liq].liq_color,
            liquid_table[liq].liq_affect[0], liquid_table[liq].liq_affect[1],
            liquid_table[liq].liq_affect[2], liquid_table[liq].liq_affect[3],
            liquid_table[liq].liq_affect[4]);
        add_buf(buffer, buf);
    }

    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);

    return;
}

void show_damlist(CHAR_DATA * ch)
{
    int att;
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH];

    buffer = new_buf();

    for (att = 0; attack_table[att].name != NULL; att++)
    {
        if ((att % 21) == 0)
            add_buf(buffer, "Name                 Noun\r\n");

        sprintf(buf, "%-20s %-20s\r\n",
            attack_table[att].name, attack_table[att].noun);
        add_buf(buffer, buf);
    }

    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);

    return;
}

MEDIT(medit_group)
{
    MOB_INDEX_DATA *pMob;
    MOB_INDEX_DATA *pMTemp;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int temp;
    BUFFER *buffer;
    bool found = FALSE;

    EDIT_MOB(ch, pMob);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax: group [number]\r\n", ch);
        send_to_char("        group show [number]\r\n", ch);
        return FALSE;
    }

    if (is_number(argument))
    {
        pMob->group = atoi(argument);
        send_to_char("Group set.\r\n", ch);
        return TRUE;
    }

    argument = one_argument(argument, arg);

    if (!strcmp(arg, "show") && is_number(argument))
    {
        if (atoi(argument) == 0)
        {
            send_to_char("Are you crazy?\r\n", ch);
            return FALSE;
        }

        buffer = new_buf();

        for (temp = 0; temp < 65536; temp++)
        {
            pMTemp = get_mob_index(temp);
            if (pMTemp && (pMTemp->group == atoi(argument)))
            {
                found = TRUE;
                sprintf(buf, "[%5d] %s\r\n", pMTemp->vnum,
                    pMTemp->player_name);
                add_buf(buffer, buf);
            }
        }

        if (found)
            page_to_char(buf_string(buffer), ch);
        else
            send_to_char("No mobs in that group.\r\n", ch);

        free_buf(buffer);
        return FALSE;
    }

    return FALSE;
}

REDIT(redit_owner)
{
    ROOM_INDEX_DATA *pRoom;

    EDIT_ROOM(ch, pRoom);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  owner [owner]\r\n", ch);
        send_to_char("         owner none\r\n", ch);
        return FALSE;
    }

    free_string(pRoom->owner);
    if (!str_cmp(argument, "none"))
        pRoom->owner = str_dup("");
    else
        pRoom->owner = str_dup(argument);

    send_to_char("Owner set.\r\n", ch);
    return TRUE;
}

MEDIT(medit_addmprog)
{
    int value;
    MOB_INDEX_DATA *pMob;
    MPROG_LIST *list;
    MPROG_CODE *code;
    char trigger[MAX_STRING_LENGTH];
    char phrase[MAX_STRING_LENGTH];
    char num[MAX_STRING_LENGTH];

    EDIT_MOB(ch, pMob);
    argument = one_argument(argument, num);
    argument = one_argument(argument, trigger);
    argument = one_argument(argument, phrase);

    if (!is_number(num) || trigger[0] == '\0' || phrase[0] == '\0')
    {
        send_to_char("Syntax:   addmprog [vnum] [trigger] [phrase]\r\n", ch);
        return FALSE;
    }

    if ((value = flag_value(mprog_flags, trigger)) == NO_FLAG)
    {
        send_to_char("Valid flags are:\r\n", ch);
        show_help(ch, "mprog");
        return FALSE;
    }

    if ((code = get_mprog_index(atoi(num))) == NULL)
    {
        send_to_char("No such MOBProgram.\r\n", ch);
        return FALSE;
    }

    list = new_mprog();
    list->vnum = atoi(num);
    list->trig_type = value;
    list->trig_phrase = str_dup(phrase);
    list->code = code->code;
    SET_BIT(pMob->mprog_flags, value);
    list->next = pMob->mprogs;
    pMob->mprogs = list;

    send_to_char("Mprog Added.\r\n", ch);
    return TRUE;
}

MEDIT(medit_delmprog)
{
    MOB_INDEX_DATA *pMob;
    MPROG_LIST *list;
    MPROG_LIST *list_next;
    char mprog[MAX_STRING_LENGTH];
    int value;
    int cnt = 0;

    EDIT_MOB(ch, pMob);

    one_argument(argument, mprog);
    if (!is_number(mprog) || mprog[0] == '\0')
    {
        send_to_char("Syntax:  delmprog [#mprog]\r\n", ch);
        return FALSE;
    }

    value = atoi(mprog);

    if (value < 0)
    {
        send_to_char("Only non-negative mprog-numbers allowed.\r\n", ch);
        return FALSE;
    }

    if (!(list = pMob->mprogs))
    {
        send_to_char("MEdit:  Non existant mprog.\r\n", ch);
        return FALSE;
    }

    if (value == 0)
    {
        REMOVE_BIT(pMob->mprog_flags, pMob->mprogs->trig_type);
        list = pMob->mprogs;
        pMob->mprogs = list->next;
        free_mprog(list);
    }
    else
    {
        while ((list_next = list->next) && (++cnt < value))
            list = list_next;

        if (list_next)
        {
            REMOVE_BIT(pMob->mprog_flags, list_next->trig_type);
            list->next = list_next->next;
            free_mprog(list_next);
        }
        else
        {
            send_to_char("No such mprog.\r\n", ch);
            return FALSE;
        }
    }

    send_to_char("Mprog removed.\r\n", ch);
    return TRUE;
}

/*
 * Balances a mobs hit, damage and mana based on level.
 */
MEDIT(medit_balance)
{
    MOB_INDEX_DATA *pMob;

    EDIT_MOB(ch, pMob);

    if (pMob->level > 60)
    {
        send_to_char("This mobs level is too high to balance.\r\n", ch);
        return FALSE;
    }

    balance_mob(pMob);

    sendf(ch, "Standard mob values set for a level %d mob.\r\n", pMob->level);

    return TRUE;
}

/*
 * Balances a mob by settings it's armor, hit, damage and mana to the standard
 * values.  An asave would need to be done after using this to persist those
 * values.
 */
void balance_mob(MOB_INDEX_DATA *pMob)
{
    if (pMob->level > MAX_LEVEL)
    {
        bugf("Mob %d greater than MAX_LEVEL (%d).", pMob->vnum, pMob->level);
        return;
    }

    pMob->ac[AC_PIERCE] = standard_mob_values[pMob->level].ac_pierce;
    pMob->ac[AC_BASH] = standard_mob_values[pMob->level].ac_bash;
    pMob->ac[AC_SLASH] = standard_mob_values[pMob->level].ac_slash;
    pMob->ac[AC_EXOTIC] = standard_mob_values[pMob->level].ac_exotic;

    pMob->hit[DICE_NUMBER] = standard_mob_values[pMob->level].hit_number;
    pMob->hit[DICE_TYPE] = standard_mob_values[pMob->level].hit_type;
    pMob->hit[DICE_BONUS] = standard_mob_values[pMob->level].hit_bonus;

    pMob->damage[DICE_NUMBER] = standard_mob_values[pMob->level].dam_number;
    pMob->damage[DICE_TYPE] = standard_mob_values[pMob->level].dam_type;
    pMob->damage[DICE_BONUS] = standard_mob_values[pMob->level].dam_bonus;

    pMob->mana[DICE_NUMBER] = standard_mob_values[pMob->level].mana_number;
    pMob->mana[DICE_TYPE] = standard_mob_values[pMob->level].mana_type;
    pMob->mana[DICE_BONUS] = standard_mob_values[pMob->level].mana_bonus;
}

REDIT(redit_room)
{
    ROOM_INDEX_DATA *room;
    int value;

    EDIT_ROOM(ch, room);

    if ((value = flag_value(room_flags, argument)) == NO_FLAG)
    {
        send_to_char("Syntax: room [flags]\r\n", ch);
        return FALSE;
    }

    TOGGLE_BIT(room->room_flags, value);
    send_to_char("Room flags toggled.\r\n", ch);
    return TRUE;
}

REDIT(redit_sector)
{
    ROOM_INDEX_DATA *room;
    int value;

    EDIT_ROOM(ch, room);

    if ((value = flag_value(sector_flags, argument)) == NO_FLAG)
    {
        send_to_char("Syntax: sector [type]\r\n", ch);
        return FALSE;
    }

    room->sector_type = value;
    send_to_char("Sector type set.\r\n", ch);

    return TRUE;
}

REDIT(redit_copy) {
    ROOM_INDEX_DATA *pRoom;
    ROOM_INDEX_DATA *pRoom2; /* Room to copy */
    int vnum;

    if (argument[0] == '\0')
    {
        send_to_char("Syntax: copy  \r\n", ch);
        return FALSE;
    }
    if (!is_number(argument))
    {
        send_to_char("REdit: You must enter a number (vnum).\r\n", ch);
        return FALSE;
    }
    else
        /* argument is a number */
    {
        vnum = atoi(argument);

        if (!(pRoom2 = get_room_index(vnum)))
        {
            send_to_char("REdit: That room does not exist.\r\n", ch);
            return FALSE;
        }
    }

    EDIT_ROOM(ch, pRoom);

    free_string(pRoom->description);
    pRoom->description = str_dup(pRoom2->description);
    free_string(pRoom->name);
    pRoom->name = str_dup(pRoom2->name);
    pRoom->sector_type = pRoom2->sector_type;
    pRoom->room_flags = pRoom2->room_flags;
    pRoom->heal_rate = pRoom2->heal_rate;
    pRoom->mana_rate = pRoom2->mana_rate;
    pRoom->clan = pRoom2->clan;
    free_string(pRoom->owner);
    pRoom->owner = str_dup(pRoom2->owner);
    pRoom->extra_descr = pRoom2->extra_descr;
    send_to_char("Room info copied.\r\n", ch);
    return TRUE;
}

/* * oedit_copy function thanks to Zanthras of Mystical Realities MUD. */
OEDIT(oedit_copy)
{
    OBJ_INDEX_DATA *pObj;
    OBJ_INDEX_DATA *pObj2;

    /* The object to copy */
    int vnum, i;

    if (argument[0] == '\0')
    {
        send_to_char("Syntax: copy  \r\n", ch);
        return FALSE;
    }

    if (!is_number(argument))
    {
        send_to_char("OEdit: You must enter a number (vnum).\r\n", ch);
        return FALSE;
    }
    else /* argument is a number */
    {
        vnum = atoi(argument);

        if (!(pObj2 = get_obj_index(vnum)))
        {
            send_to_char("OEdit: That object does not exist.\r\n", ch);
            return FALSE;
        }

    }

    EDIT_OBJ(ch, pObj);

    free_string(pObj->name);
    pObj->name = str_dup(pObj2->name);
    pObj->item_type = pObj2->item_type;
    pObj->level = pObj2->level;
    pObj->wear_flags = pObj2->wear_flags;
    pObj->extra_flags = pObj2->extra_flags;
    free_string(pObj->material);
    pObj->material = str_dup(pObj2->material);
    pObj->condition = pObj2->condition;
    pObj->weight = pObj2->weight;
    pObj->cost = pObj2->cost;
    pObj->extra_descr = pObj2->extra_descr;
    free_string(pObj->short_descr);
    pObj->short_descr = str_dup(pObj2->short_descr);
    free_string(pObj->description);
    pObj->description = str_dup(pObj2->description);
    pObj->affected = pObj2->affected;

    for (i = 0; i < 5; i++)
    {
        pObj->value[i] = pObj2->value[i];
    }

    send_to_char("Object info copied.\r\n", ch);
    return TRUE;
}

/* * medit_copy function thanks to Zanthras of Mystical Realities MUD. * Thanks to Ivan for what there is of the incomplete mobprog part. * Hopefully it will be finished in a later release of this snippet. */
MEDIT(medit_copy)
{
    MOB_INDEX_DATA *pMob;
    MOB_INDEX_DATA *pMob2;

    /* The mob to copy */
    int vnum;

    /* MPROG_LIST *list; */ /* Used for the mob prog for loop */
    if (argument[0] == '\0')
    {
        send_to_char("Syntax: copy  \r\n", ch);
        return FALSE;
    }

    if (!is_number(argument))
    {
        send_to_char("MEdit: You must enter a number (vnum).\r\n", ch);
        return FALSE;
    }
    else /* argument is a number */
    {
        vnum = atoi(argument);
        if (!(pMob2 = get_mob_index(vnum)))
        {
            send_to_char("MEdit: That mob does not exist.\r\n", ch);
            return FALSE;
        }
    }

    EDIT_MOB(ch, pMob);
    free_string(pMob->player_name);
    pMob->player_name = str_dup(pMob2->player_name);
    pMob->act = pMob2->act;
    pMob->sex = pMob2->sex;
    pMob->race = pMob2->race;
    pMob->level = pMob2->level;
    pMob->alignment = pMob2->alignment;
    pMob->hitroll = pMob2->hitroll;
    pMob->dam_type = pMob2->dam_type;
    pMob->group = pMob2->group;
    pMob->hit[DICE_NUMBER] = pMob2->hit[DICE_NUMBER];
    pMob->hit[DICE_TYPE] = pMob2->hit[DICE_TYPE];
    pMob->hit[DICE_BONUS] = pMob2->hit[DICE_BONUS];
    pMob->damage[DICE_NUMBER] = pMob2->damage[DICE_NUMBER];
    pMob->damage[DICE_TYPE] = pMob2->damage[DICE_TYPE];
    pMob->damage[DICE_BONUS] = pMob2->damage[DICE_BONUS];
    pMob->mana[DICE_NUMBER] = pMob2->mana[DICE_NUMBER];
    pMob->mana[DICE_TYPE] = pMob2->mana[DICE_TYPE];
    pMob->mana[DICE_BONUS] = pMob2->mana[DICE_BONUS];
    pMob->affected_by = pMob2->affected_by;
    pMob->ac[AC_PIERCE] = pMob2->ac[AC_PIERCE];
    pMob->ac[AC_BASH] = pMob2->ac[AC_BASH];
    pMob->ac[AC_SLASH] = pMob2->ac[AC_SLASH];
    pMob->ac[AC_EXOTIC] = pMob2->ac[AC_EXOTIC];
    pMob->form = pMob2->form;
    pMob->parts = pMob2->parts;
    pMob->imm_flags = pMob2->imm_flags;
    pMob->res_flags = pMob2->res_flags;
    pMob->vuln_flags = pMob2->vuln_flags;
    pMob->off_flags = pMob2->off_flags;
    pMob->size = pMob2->size;
    free_string(pMob->material);
    pMob->material = str_dup(pMob2->material);
    pMob->start_pos = pMob2->start_pos;
    pMob->default_pos = pMob2->default_pos;
    pMob->wealth = pMob2->wealth;
    pMob->spec_fun = pMob2->spec_fun;
    free_string(pMob->short_descr);
    pMob->short_descr = str_dup(pMob2->short_descr);
    free_string(pMob->long_descr);
    pMob->long_descr = str_dup(pMob2->long_descr);
    free_string(pMob->description);
    pMob->description = str_dup(pMob2->description);

    /* Hopefully get the shop data to copy later * This is the fields here if you get it copying send me * a copy and I'll add it to the snippet with credit to * you of course :) same with the mobprogs for loop :) */ /* if ( pMob->pShop ) { SHOP_DATA *pShop, *pShop2; pShop = pMob->pShop; pShop2 = pMob2->pShop; pShop->profit_buy = pShop2->profit_buy; pShop->profit_sell = pShop2->profit_sell; pShop->open_hour = pShop2->open_hour; pShop->close_hour = pShop2->close_hour; pShop->buy_type[iTrade] = pShop2->buy_type[iTrade]; } */ /* for loop thanks to Ivan, still needs work though for (list = pMob->mprogs; list; list = list->next ) { MPROG_LIST *newp = new_mprog(); newp->trig_type = list->trig_type; free_string( newp->trig_phrase ); newp->trig_phrase = str_dup( list->trig_phrase ); newp->vnum = list->vnum; free_string( newp->code ) newp->code = str_dup( list->code ); pMob->mprogs = newp; } */
    send_to_char("Mob info copied.\r\n", ch);
    return FALSE;
}

/*****************************************************************
   Help OLC (merged from hedit.c)
******************************************************************/
extern HELP_AREA *had_list;

const struct olc_cmd_type hedit_table[] = {
    /*    {    command        function    }, */

    { "keyword", hedit_keyword },
    { "text", hedit_text },
    { "new", hedit_new },
    { "level", hedit_level },
    { "commands", show_commands },
    { "delete", hedit_delete },
    { "list", hedit_list },
    { "show", hedit_show },
    { "?", show_help },

    { NULL, 0 }
};

HELP_AREA *get_help_area(HELP_DATA * help)
{
    HELP_AREA *temp;
    HELP_DATA *thelp;

    for (temp = had_list; temp; temp = temp->next)
        for (thelp = temp->first; thelp; thelp = thelp->next_area)
            if (thelp == help)
                return temp;

    return NULL;
}

HEDIT(hedit_show)
{
    HELP_DATA *help;
    char buf[MSL * 2];

    EDIT_HELP(ch, help);

    sprintf(buf, "Keyword : [%s]\r\n"
        "Level   : [%d]\r\n"
        "Text    :\r\n"
        "%s-END-\r\n", help->keyword, help->level, help->text);
    send_to_char(buf, ch);

    return FALSE;
}

HEDIT(hedit_level)
{
    HELP_DATA *help;
    int lev;

    EDIT_HELP(ch, help);

    if (IS_NULLSTR(argument) || !is_number(argument))
    {
        send_to_char("Syntax: level [-1..MAX_LEVEL]\r\n", ch);
        return FALSE;
    }

    lev = atoi(argument);

    if (lev < -1 || lev > MAX_LEVEL)
    {
        sendf(ch, "HEdit : levels are between -1 and %d inclusive.\r\n",
            MAX_LEVEL);
        return FALSE;
    }

    help->level = lev;
    send_to_char("Ok.\r\n", ch);
    return TRUE;
}

HEDIT(hedit_keyword)
{
    HELP_DATA *help;

    EDIT_HELP(ch, help);

    if (IS_NULLSTR(argument))
    {
        send_to_char("Syntax: keyword [keywords]\r\n", ch);
        return FALSE;
    }

    free_string(help->keyword);
    help->keyword = str_dup(argument);

    send_to_char("Ok.\r\n", ch);
    return TRUE;
}

HEDIT(hedit_new)
{
    char arg[MIL], fullarg[MIL];
    HELP_AREA *had;
    HELP_DATA *help;
    extern HELP_DATA *help_last;

    if (IS_NULLSTR(argument))
    {
        send_to_char("Syntax: new [name]\r\n", ch);
        send_to_char("         new [area] [name]\r\n", ch);
        return FALSE;
    }

    strcpy(fullarg, argument);
    argument = one_argument(argument, arg);

    if (!(had = had_lookup(arg)))
    {
        had = ch->in_room->area->helps;
        argument = fullarg;
    }

    if (help_lookup(argument))
    {
        send_to_char("HEdit : help exists.\r\n", ch);
        return FALSE;
    }

    if (!had)
    {                            /* the area has no helps */
        had = new_had();
        had->filename = str_dup(ch->in_room->area->file_name);
        had->area = ch->in_room->area;
        had->first = NULL;
        had->last = NULL;
        had->changed = TRUE;
        had->next = had_list;
        had_list = had;
        ch->in_room->area->helps = had;
        SET_BIT(ch->in_room->area->area_flags, AREA_CHANGED);
    }

    help = new_help();
    help->level = 0;
    help->keyword = str_dup(argument);
    help->text = str_dup("");

    if (help_last)
        help_last->next = help;

    if (help_first == NULL)
        help_first = help;

    help_last = help;
    help->next = NULL;

    if (!had->first)
        had->first = help;
    if (!had->last)
        had->last = help;

    had->last->next_area = help;
    had->last = help;
    help->next_area = NULL;

    ch->desc->pEdit = (HELP_DATA *)help;
    ch->desc->editor = ED_HELP;

    send_to_char("Ok.\r\n", ch);
    return FALSE;
}

HEDIT(hedit_text)
{
    HELP_DATA *help;

    EDIT_HELP(ch, help);

    if (!IS_NULLSTR(argument))
    {
        send_to_char("Syntax: text\r\n", ch);
        return FALSE;
    }

    string_append(ch, &help->text);

    return TRUE;
}

void hedit(CHAR_DATA * ch, char *argument)
{
    HELP_DATA *pHelp;
    HELP_AREA *had;
    char arg[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int cmd;

    smash_tilde(argument);
    strcpy(arg, argument);
    argument = one_argument(argument, command);

    EDIT_HELP(ch, pHelp);

    had = get_help_area(pHelp);

    if (had == NULL)
    {
        bugf("hedit : had for help %s NULL", pHelp->keyword);
        edit_done(ch);
        return;
    }

    if (ch->pcdata->security < 9)
    {
        send_to_char("HEdit: Insufficient security to edit helps.\r\n",
            ch);
        edit_done(ch);
        return;
    }

    if (command[0] == '\0')
    {
        hedit_show(ch, argument);
        return;
    }

    if (!str_cmp(command, "done"))
    {
        edit_done(ch);
        return;
    }

    for (cmd = 0; hedit_table[cmd].name != NULL; cmd++)
    {
        if (!str_prefix(command, hedit_table[cmd].name))
        {
            if ((*hedit_table[cmd].olc_fun) (ch, argument))
                had->changed = TRUE;
            return;
        }
    }

    interpret(ch, arg);
    return;
}

/*
void do_hedit (CHAR_DATA * ch, char *argument)
{
HELP_DATA *pHelp;

if (IS_NPC (ch))
return;


if ((pHelp = help_lookup (argument)) == NULL)
{
send_to_char ("HEdit : Help doesn't exist.\r\n", ch);
return;
}

ch->desc->pEdit = (void *) pHelp;
ch->desc->editor = ED_HELP;

return;
}
*/

void do_hedit(CHAR_DATA *ch, char *argument)
{
    HELP_DATA *pHelp;
    char arg1[MIL];
    char argall[MAX_INPUT_LENGTH], argone[MAX_INPUT_LENGTH];
    bool found = FALSE;

    strcpy(arg1, argument);

    if (argument[0] != '\0')
    {
        /* Taken from do_help */
        argall[0] = '\0';
        while (argument[0] != '\0')
        {
            argument = one_argument(argument, argone);
            if (argall[0] != '\0')
                strcat(argall, " ");
            strcat(argall, argone);
        }
        for (pHelp = help_first; pHelp != NULL; pHelp = pHelp->next)
        {
            if (is_name(argall, pHelp->keyword))
            {
                ch->desc->pEdit = (void *)pHelp;
                ch->desc->editor = ED_HELP;
                found = TRUE;
                return;
            }
        }
    }
    if (!found)
    {
        argument = one_argument(arg1, arg1);

        if (!str_cmp(arg1, "new"))
        {
            if (argument[0] == '\0')
            {
                send_to_char("Syntax: edit help new [topic]\r\n", ch);
                return;
            }
            if (hedit_new(ch, argument))
                ch->desc->editor = ED_HELP;
            return;
        }
    }
    send_to_char("HEdit:  There is no default help to edit.\r\n", ch);
    return;
}


HEDIT(hedit_delete)
{
    HELP_DATA *pHelp, *temp;
    HELP_AREA *had;
    DESCRIPTOR_DATA *d;
    bool found = FALSE;

    EDIT_HELP(ch, pHelp);

    for (d = descriptor_list; d; d = d->next)
        if (d->editor == ED_HELP && pHelp == (HELP_DATA *)d->pEdit)
            edit_done(d->character);

    if (help_first == pHelp)
        help_first = help_first->next;
    else
    {
        for (temp = help_first; temp; temp = temp->next)
            if (temp->next == pHelp)
                break;

        if (!temp)
        {
            bugf("hedit_delete : help %s not found in help_first",
                pHelp->keyword);
            return FALSE;
        }

        temp->next = pHelp->next;
    }

    for (had = had_list; had; had = had->next)
        if (pHelp == had->first)
        {
            found = TRUE;
            had->first = had->first->next_area;
        }
        else
        {
            for (temp = had->first; temp; temp = temp->next_area)
                if (temp->next_area == pHelp)
                    break;

            if (temp)
            {
                temp->next_area = pHelp->next_area;
                found = TRUE;
                break;
            }
        }

    if (!found)
    {
        bugf("hedit_delete : help %s not found in had_list",
            pHelp->keyword);
        return FALSE;
    }

    free_help(pHelp);

    send_to_char("Ok.\r\n", ch);
    return TRUE;
}

HEDIT(hedit_list)
{
    char buf[MIL];
    int cnt = 0;
    HELP_DATA *pHelp;
    BUFFER *buffer;

    EDIT_HELP(ch, pHelp);

    if (!str_cmp(argument, "all"))
    {
        buffer = new_buf();

        for (pHelp = help_first; pHelp; pHelp = pHelp->next)
        {
            sprintf(buf, "%3d. %-14.14s%s", cnt, pHelp->keyword,
                cnt % 4 == 3 ? "\r\n" : " ");
            add_buf(buffer, buf);
            cnt++;
        }

        if (cnt % 4)
            add_buf(buffer, "\r\n");

        page_to_char(buf_string(buffer), ch);
        return FALSE;
    }

    if (!str_cmp(argument, "area"))
    {
        if (ch->in_room->area->helps == NULL)
        {
            send_to_char("No helps in this area.\r\n", ch);
            return FALSE;
        }

        buffer = new_buf();

        for (pHelp = ch->in_room->area->helps->first; pHelp;
        pHelp = pHelp->next_area)
        {
            sprintf(buf, "%3d. %-14.14s%s", cnt, pHelp->keyword,
                cnt % 4 == 3 ? "\r\n" : " ");
            add_buf(buffer, buf);
            cnt++;
        }

        if (cnt % 4)
            add_buf(buffer, "\r\n");

        page_to_char(buf_string(buffer), ch);
        return FALSE;
    }

    if (IS_NULLSTR(argument))
    {
        send_to_char("Syntax: list all\r\n", ch);
        send_to_char("        list area\r\n", ch);
        return FALSE;
    }

    return FALSE;
}

/*****************************************************************
   Mob Prog OLC (merged from olc_mpcode.c)
******************************************************************/

const struct olc_cmd_type mpedit_table[] = {
    /*    {    command        function    }, */

    { "commands", show_commands },
    { "create", mpedit_create },
    { "code", mpedit_code },
    { "name", mpedit_name },
    { "show", mpedit_show },
    { "list", mpedit_list },
    { "?", show_help },

    { NULL, 0 }
};

void mpedit(CHAR_DATA * ch, char *argument)
{
    MPROG_CODE *pMcode;
    char arg[MAX_INPUT_LENGTH];
    char command[MAX_INPUT_LENGTH];
    int cmd;
    AREA_DATA *ad;

    smash_tilde(argument);
    strcpy(arg, argument);
    argument = one_argument(argument, command);

    EDIT_MPCODE(ch, pMcode);

    if (pMcode)
    {
        ad = get_vnum_area(pMcode->vnum);

        if (ad == NULL)
        {                        /* ??? */
            edit_done(ch);
            return;
        }

        if (!IS_BUILDER(ch, ad))
        {
            send_to_char("MPEdit: Insufficient security to modify code.\r\n",
                ch);
            edit_done(ch);
            return;
        }
    }

    if (command[0] == '\0')
    {
        mpedit_show(ch, argument);
        return;
    }

    if (!str_cmp(command, "done"))
    {
        edit_done(ch);
        return;
    }

    for (cmd = 0; mpedit_table[cmd].name != NULL; cmd++)
    {
        if (!str_prefix(command, mpedit_table[cmd].name))
        {
            if ((*mpedit_table[cmd].olc_fun) (ch, argument) && pMcode)
                if ((ad = get_vnum_area(pMcode->vnum)) != NULL)
                    SET_BIT(ad->area_flags, AREA_CHANGED);
            return;
        }
    }

    interpret(ch, arg);

    return;
}

void do_mpedit(CHAR_DATA * ch, char *argument)
{
    MPROG_CODE *pMcode;
    char command[MAX_INPUT_LENGTH];

    argument = one_argument(argument, command);

    if (is_number(command))
    {
        int vnum = atoi(command);
        AREA_DATA *ad;

        if ((pMcode = get_mprog_index(vnum)) == NULL)
        {
            send_to_char("MPEdit : That vnum does not exist.\r\n", ch);
            return;
        }

        ad = get_vnum_area(vnum);

        if (ad == NULL)
        {
            send_to_char("MPEdit : VNUM not assigned to any area.\r\n", ch);
            return;
        }

        if (!IS_BUILDER(ch, ad))
        {
            send_to_char
                ("MPEdit : Insufficient security to edit area.\r\n", ch);
            return;
        }

        ch->desc->pEdit = (void *)pMcode;
        ch->desc->editor = ED_MPCODE;

        return;
    }

    if (!str_cmp(command, "create"))
    {
        if (argument[0] == '\0')
        {
            send_to_char("Syntax : mpedit create [vnum]\r\n", ch);
            return;
        }

        mpedit_create(ch, argument);
        return;
    }

    send_to_char("Syntax : mpedit [vnum]\r\n", ch);
    send_to_char("           mpedit create [vnum]\r\n", ch);

    return;
}

MPEDIT(mpedit_create)
{
    MPROG_CODE *pMcode;
    int value = atoi(argument);
    AREA_DATA *ad;

    if (IS_NULLSTR(argument) || value < 1)
    {
        send_to_char("Syntax : mpedit create [vnum]\r\n", ch);
        return FALSE;
    }

    ad = get_vnum_area(value);

    if (ad == NULL)
    {
        send_to_char("MPEdit : VNUM not assigned to an area.\r\n", ch);
        return FALSE;
    }

    if (!IS_BUILDER(ch, ad))
    {
        send_to_char
            ("MPEdit : Insufficient seucrity to edit MobProgs.\r\n", ch);
        return FALSE;
    }

    if (get_mprog_index(value))
    {
        send_to_char("MPEdit: Code vnum already exists.\r\n", ch);
        return FALSE;
    }

    pMcode = new_mpcode();
    pMcode->vnum = value;
    pMcode->next = mprog_list;
    mprog_list = pMcode;
    ch->desc->pEdit = (void *)pMcode;
    ch->desc->editor = ED_MPCODE;

    send_to_char("MobProgram Code Created.\r\n", ch);

    return TRUE;
}

MPEDIT(mpedit_show)
{
    MPROG_CODE *pMcode;
    char buf[MAX_STRING_LENGTH];

    EDIT_MPCODE(ch, pMcode);

    sprintf(buf,
        "Vnum:  [%d]\r\n"
        "Name:  [%s]\r\n"
        "Code:\r\n%s\r\n", pMcode->vnum, pMcode->name, pMcode->code);

    send_to_char(buf, ch);

    return FALSE;
}

MPEDIT(mpedit_code)
{
    MPROG_CODE *pMcode;
    EDIT_MPCODE(ch, pMcode);

    if (argument[0] == '\0')
    {
        string_append(ch, &pMcode->code);
        return TRUE;
    }

    send_to_char("Syntax: code\r\n", ch);
    return FALSE;
}

MPEDIT(mpedit_name)
{
    MPROG_CODE *pMcode;
    EDIT_MPCODE(ch, pMcode);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax: name <name>\r\n", ch);
        return FALSE;
    }

    free_string(pMcode->name);
    pMcode->name = str_dup(argument);

    return TRUE;
}

MPEDIT(mpedit_list)
{
    int count = 1;
    MPROG_CODE *mprg;
    char buf[MAX_STRING_LENGTH];
    BUFFER *buffer;
    bool fAll = !str_cmp(argument, "all");
    AREA_DATA *ad;

    buffer = new_buf();

    for (mprg = mprog_list; mprg != NULL; mprg = mprg->next)
    {
        if (fAll
            || (ch->in_room->area->min_vnum <= mprg->vnum && ch->in_room->area->max_vnum >= mprg->vnum))
        {
            ad = get_vnum_area(mprg->vnum);

            if (ad != NULL)
            {
                sprintf(buf, "[ {G%5d{x ] {W%s:{x {C%s{x\r\n", mprg->vnum, ad->name, mprg->name);
                add_buf(buffer, buf);
            }

            count++;
        }
    }

    if (count == 1)
    {
        if (fAll)
        {
            add_buf(buffer, "MobPrograms do not exist!\r\n");
        }
        else
        {
            add_buf(buffer, "MobPrograms do not exist in this area.\r\n");
        }
    }

    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);

    return FALSE;
}

REDIT(aedit_continent) {
    AREA_DATA *pArea;
    char buf[MAX_INPUT_LENGTH];

    EDIT_AREA(ch, pArea);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax: continent [name]", ch);
        return FALSE;
    }

    pArea->continent = continent_lookup(argument);
    sprintf(buf, "Continent set to %s.\r\n", continent_table[pArea->continent].name);
    send_to_char(buf, ch);
    return TRUE;
} // end REDIT(aedit_continent)

bool gedit_show(CHAR_DATA *ch, char *argument)
{
    GROUPTYPE *group;
    char buf[MAX_STRING_LENGTH];
    int class, x, fact;
    fact = (MAX_IN_GROUP / 3);

    EDIT_GROUP(ch, group);

    sprintf(buf, "\r\nName:         [%s]\r\n\r\n", group->name);
    send_to_char(buf, ch);

    for (x = 0; x <= fact; x++)
    {
        sprintf(buf, "%2d) [%-15.15s] ", x, group->spells[x] ? group->spells[x] : "None");
        send_to_char(buf, ch);
        sprintf(buf, "%2d) [%-15.15s] ", x + fact + 1, group->spells[x + fact + 1] ? group->spells[x + fact + 1] : "None");
        send_to_char(buf, ch);
        if (x + ((fact + 1) * 2) < MAX_IN_GROUP)
        {
            sprintf(buf, "%2d) [%-15.15s]\r\n", x + ((fact + 1) * 2), group->spells[x + ((fact + 1) * 2)] ? group->spells[x + ((fact + 1) * 2)] : "None");
            send_to_char(buf, ch);
        }
        else
        {
            send_to_char("\r\n", ch);
        }
    }


    sprintf(buf, "\r\nClass         Rating  Class         Rating\r\n");
    send_to_char(buf, ch);

    sprintf(buf, "--------------------------------------------------------\r\n");
    send_to_char(buf, ch);

    for (class = 0; class < top_class; class++)
    {
        sprintf(buf, "%-13s [%2d]", class_table[class]->name, group->rating[class]);
        if ((class % 2) == 0)
        {
            strcat(buf, "    ");
        }
        else
        {
            strcat(buf, "\r\n");
        }
        send_to_char(buf, ch);
    }
    send_to_char("\r\n", ch);

    return TRUE;
}

GEDIT(gedit_create)
{
    GROUPTYPE *group;
    int gn, i;

    gn = group_lookup(argument);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  edit group create group_name\r\n", ch);
        return FALSE;
    }

    if (gn != -1)
    {
        send_to_char("GEdit:  group already exists.\r\n", ch);
        return FALSE;
    }

    if (top_group + 1 >= MAX_GROUP)
    {
        send_to_char("MAX_GROUP Exceeded, please increae the MAX_GROUP in merc.h\r\n", ch);
        return FALSE;
    }

    group = alloc_perm(sizeof(*group));
    group->name = str_dup(argument);
    for (i = 0; i < top_class; i++)
    {
        group->rating[i] = -1;
    }
    group_table[top_group] = group;
    ch->desc->pEdit = (void *)group;
    top_group++;
    send_to_char("Group Created.\r\n", ch);
    return TRUE;
}

GEDIT(gedit_list)
{
    do_function(ch, &do_groups, "all");
    return TRUE;
}

GEDIT(gedit_add)
{
    GROUPTYPE *group;
    char arg1[MAX_STRING_LENGTH];
    int num, sn;
    char buf[MAX_STRING_LENGTH];

    EDIT_GROUP(ch, group);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  add [#] group/skill\r\n", ch);
        return FALSE;
    }

    argument = one_argument(argument, arg1);
    num = atoi(arg1);

    if (!is_number(arg1) || !argument || argument[0] == '\0')
    {
        send_to_char("Syntax:  add [#] group/skill\r\n", ch);
        return FALSE;
    }

    if ((sn = group_lookup(argument)) != -1)
    {
        free_string(group->spells[num]);
        group->spells[num] = str_dup(group_table[sn]->name);
    }
    else if ((sn = skill_lookup(argument)) != -1)
    {
        free_string(group->spells[num]);
        group->spells[num] = str_dup(skill_table[sn]->name);
    }
    else
    {
        sprintf(buf, "%s doesn't exist.\r\n", argument);
        send_to_char(buf, ch);
        return FALSE;
    }
    send_to_char("Ok.\r\n", ch);
    return TRUE;
}

GEDIT(gedit_del)
{
    GROUPTYPE *group;
    char arg1[MAX_STRING_LENGTH];
    int num;

    EDIT_GROUP(ch, group);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  del [#]\r\n", ch);
        return FALSE;
    }

    argument = one_argument(argument, arg1);
    num = atoi(arg1);

    if (!is_number(arg1))
    {
        send_to_char("Syntax:  del [#]\r\n", ch);
        return FALSE;
    }

    free_string(group->spells[num]);
    group->spells[num] = '\0';
    send_to_char("Ok.\r\n", ch);

    return TRUE;
}

GEDIT(gedit_name)
{
    GROUPTYPE *group;

    EDIT_GROUP(ch, group);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  name [string]\r\n", ch);
        return FALSE;
    }

    free_string(group->name);
    group->name = str_dup(argument);
    group->name[0] = LOWER(group->name[0]);

    send_to_char("Name set.\r\n", ch);
    return TRUE;
}

GEDIT(gedit_rating)
{
    GROUPTYPE *group;
    int class_no, rating;
    char class_name[MAX_INPUT_LENGTH];
    char arg1[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    EDIT_GROUP(ch, group);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  rating class [#]\r\n", ch);
        return FALSE;
    }

    argument = one_argument(argument, class_name);
    argument = one_argument(argument, arg1);
    rating = atoi(arg1);

    if (!is_number(arg1))
    {
        send_to_char("Syntax:  rating class [#]\r\n", ch);
        return FALSE;
    }

    for (class_no = 0; class_no < top_class; class_no++)
        if (!str_cmp(class_name, class_table[class_no]->name))
            break;

    if (!str_cmp(class_name, "all"))
    {
        for (class_no = 0; class_no < top_class; class_no++)
        {
            group->rating[class_no] = rating;
        }

        sprintf(buf, "OK, Cost set at %d for all classes.\r\n", rating);
        send_to_char(buf, ch);
    }
    else
    {

        for (class_no = 0; class_no < top_class; class_no++)
            if (!str_prefix(class_name, class_table[class_no]->name))
                break;

        if (class_no >= top_class)
        {
            sprintf(buf, "No class named '%s' exists.\r\n", class_name);
            send_to_char(buf, ch);
            return FALSE;
        }
        group->rating[class_no] = rating;
        sprintf(buf, "OK, %s will now cost %d for %s.\r\n", group->name, rating, class_table[class_no]->name);
        send_to_char(buf, ch);
    }

    return TRUE;
}

CEDIT(cedit_name)
{
    CLASSTYPE *class;

    EDIT_CLASS(ch, class);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  name [string]\r\n", ch);
        return FALSE;
    }

    free_string(class->name);
    class->name = str_dup(argument);
    class->name[0] = LOWER(class->name[0]);

    send_to_char("Name set.\r\n", ch);
    return TRUE;
}

CEDIT(cedit_clan)
{
    CLASSTYPE *class;

    EDIT_CLASS(ch, class);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  clan [clan name]\r\n", ch);
        return FALSE;
    }

    class->clan = clan_lookup(argument);

    if (class->clan == 0)
    {
        send_to_char("Clan cleared.\r\n", ch);
    }
    else
    {
        send_to_char("Clan set.\r\n", ch);
    }

    return TRUE;
}


CEDIT(cedit_whoname)
{
    CLASSTYPE *class;

    EDIT_CLASS(ch, class);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  whoname [string]\r\n", ch);
        return FALSE;
    }

    if (strlen(argument) > 4)
    {
        send_to_char("Who name must be 3 characters or less.\r\n", ch);
        return FALSE;
    }

    class->who_name[0] = '\0';
    strcpy(class->who_name, argument);

    send_to_char("Name set.\r\n", ch);
    return TRUE;
}


CEDIT(cedit_attrprime)
{
    CLASSTYPE *class;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_CLASS(ch, class);

        if ((value = flag_value(stat_flags, argument)) != NO_FLAG)
        {
            class->attr_prime = value;

            send_to_char("Prime Attribute set.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char("Syntax:  attrprime [attr]\r\n", ch);
    return FALSE;
}

CEDIT(cedit_attrsecond)
{
    CLASSTYPE *class;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_CLASS(ch, class);

        if ((value = flag_value(stat_flags, argument)) != NO_FLAG)
        {
            class->attr_second = value;

            send_to_char("Secondary attribute set.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char("Syntax:  attrsecond [attr]\r\n", ch);
    return FALSE;
}

CEDIT(cedit_weapon)
{
    CLASSTYPE *class;

    EDIT_CLASS(ch, class);

    if (argument[0] == '\0' || !is_number(argument))
    {
        send_to_char("Syntax:  weapon [vnum]\r\n", ch);
        return FALSE;
    }

    class->weapon = atoi(argument);

    send_to_char("Weapon vnum set.\r\n", ch);
    return TRUE;
}


CEDIT(cedit_skilladept)
{
    CLASSTYPE *class;

    EDIT_CLASS(ch, class);

    if (argument[0] == '\0' || !is_number(argument))
    {
        send_to_char("Syntax:  skilladept [number]\r\n", ch);
        return FALSE;
    }

    class->skill_adept = atoi(argument);

    send_to_char("Skill Adept set.\r\n", ch);
    return TRUE;
}


CEDIT(cedit_thac0)
{
    CLASSTYPE *class;

    EDIT_CLASS(ch, class);

    if (argument[0] == '\0' || !is_number(argument))
    {
        send_to_char("Syntax:  thac0 [number]\r\n", ch);
        return FALSE;
    }

    class->thac0_00 = atoi(argument);

    send_to_char("Thac0 set.\r\n", ch);
    return TRUE;
}


CEDIT(cedit_thac32)
{
    CLASSTYPE *class;

    EDIT_CLASS(ch, class);

    if (argument[0] == '\0' || !is_number(argument))
    {
        send_to_char("Syntax:  thac32 [number]\r\n", ch);
        return FALSE;
    }

    class->thac0_32 = atoi(argument);

    send_to_char("Thac32 set.\r\n", ch);
    return TRUE;
}


CEDIT(cedit_hpmin)
{
    CLASSTYPE *class;

    EDIT_CLASS(ch, class);

    if (argument[0] == '\0' || !is_number(argument))
    {
        send_to_char("Syntax:  hpmin [number]\r\n", ch);
        return FALSE;
    }

    class->hp_min = atoi(argument);

    send_to_char("HP Min set.\r\n", ch);
    return TRUE;
}


CEDIT(cedit_hpmax)
{
    CLASSTYPE *class;

    EDIT_CLASS(ch, class);

    if (argument[0] == '\0' || !is_number(argument))
    {
        send_to_char("Syntax:  hpmax [number]\r\n", ch);
        return FALSE;
    }

    class->hp_max = atoi(argument);

    send_to_char("HP Max set.\r\n", ch);
    return TRUE;
}


CEDIT(cedit_mana)
{
    CLASSTYPE *class;

    EDIT_CLASS(ch, class);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  mana [TRUE/FALSE]\r\n", ch);
        return FALSE;
    }

    if (!str_prefix(argument, "true"))
    {
        class->mana = TRUE;
    }
    else if (!str_prefix(argument, "false"))
    {
        class->mana = FALSE;
    }
    else
    {
        send_to_char("Syntax:  mana [TRUE/FALSE]\r\n", ch);
        return FALSE;
    }

    send_to_char("Mana set.\r\n", ch);
    return TRUE;
}

CEDIT(cedit_isreclass)
{
    CLASSTYPE *class;

    EDIT_CLASS(ch, class);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  isreclass [TRUE/FALSE]\r\n", ch);
        return FALSE;
    }

    if (!str_prefix(argument, "true"))
    {
        class->is_reclass = TRUE;
    }
    else if (!str_prefix(argument, "false"))
    {
        class->is_reclass = FALSE;
    }
    else
    {
        send_to_char("Syntax:  isreclass [TRUE/FALSE]\r\n", ch);
        return FALSE;
    }

    send_to_char("Reclass flag set.\r\n", ch);
    return TRUE;

}

CEDIT(cedit_isenabled)
{
    CLASSTYPE *class;

    EDIT_CLASS(ch, class);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  isenabled [TRUE/FALSE]\r\n", ch);
        return FALSE;
    }

    if (!str_prefix(argument, "true"))
    {
        class->is_enabled = TRUE;
    }
    else if (!str_prefix(argument, "false"))
    {
        class->is_enabled = FALSE;
    }
    else
    {
        send_to_char("Syntax:  isenabled [TRUE/FALSE]\r\n", ch);
        return FALSE;
    }

    send_to_char("Enabled flag set.\r\n", ch);
    return TRUE;

}

// We don't support moons yet but we will and it will look like this, just
// like the mana flag
/*CEDIT( cedit_moon )
{
    CLASSTYPE *class;

    EDIT_CLASS(ch, class);

    if ( argument[0] == '\0' )
    {
        send_to_char( "Syntax:  mana [TRUE/FALSE]\r\n", ch );
        return FALSE;
    }

    if (!str_prefix(argument,"true"))
    {
    class->fMoon = TRUE;
    }
    else if (!str_prefix(argument,"false"))
    {
    class->fMoon = FALSE;
    }
    else
    {
        send_to_char( "Syntax:  moon [TRUE/FALSE]\r\n", ch );
        return FALSE;
    }

    send_to_char( "Moon set.\r\n", ch);
    return TRUE;
}*/


CEDIT(cedit_basegroup)
{
    CLASSTYPE *class;
    char buf[MAX_STRING_LENGTH];

    EDIT_CLASS(ch, class);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  basegroup group\r\n", ch);
        return FALSE;
    }

    if (group_lookup(argument) == -1)
    {
        sprintf(buf, "%s doesn't exist.\r\n", argument);
        send_to_char(buf, ch);
        return FALSE;
    }

    free_string(class->base_group);
    class->base_group = str_dup(argument);

    return TRUE;
}


CEDIT(cedit_defgroup)
{
    CLASSTYPE *class;
    char buf[MAX_STRING_LENGTH];

    EDIT_CLASS(ch, class);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  defaultgroup group\r\n", ch);
        return FALSE;
    }

    if (group_lookup(argument) == -1)
    {
        sprintf(buf, "%s doesn't exist.\r\n", argument);
        send_to_char(buf, ch);
        return FALSE;
    }

    free_string(class->default_group);
    class->default_group = str_dup(argument);

    return TRUE;
}


CEDIT(cedit_create)
{
    CLASSTYPE *class;
    int cl, x;

    cl = class_lookup(argument);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  edit class create group_name\r\n", ch);
        return FALSE;
    }

    if (cl != -1)
    {
        send_to_char("CEdit:  group already exists.\r\n", ch);
        return FALSE;
    }

    if (top_class + 1 >= MAX_CLASS)
    {
        send_to_char("MAX_CLASS Exceeded, please increae the MAX_CLASS in merc.h\r\n", ch);
        return FALSE;
    }

    class = alloc_perm(sizeof(*class));
    class->name = str_dup(argument);
    class_table[top_class] = class;
    ch->desc->pEdit = (void *)class;
    for (x = 0; x < top_sn; x++)
    {
        if (!skill_table[x]->name)
            break;

        skill_table[x]->skill_level[top_class] = LEVEL_IMMORTAL;
        skill_table[x]->rating[top_class] = 1;
    }

    for (x = 0; x < top_group; x++)
    {
        if (!group_table[x]) break;
        group_table[x]->rating[top_class] = -1;
    }

    for (x = 0; x < MAX_PC_RACE; x++)
    {
        if (pc_race_table[x].name == NULL || pc_race_table[x].name[0] == '\0')
            break;

        pc_race_table[x].class_mult[top_class] = 100;
    }
    top_class++;
    send_to_char("Class Created.\r\n", ch);
    return TRUE;
}


CEDIT(cedit_show)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    CLASSTYPE *class;
    int x;

    EDIT_CLASS(ch, class);

    sprintf(buf, "\r\nName:          [%s]\r\n", class->name);
    send_to_char(buf, ch);
    sprintf(buf, "WhoName:       [%s]\r\n", class->who_name);
    send_to_char(buf, ch);
    sprintf(buf2, "Attr Prime:    [%s]\r\n", flag_string(stat_flags, class->attr_prime));
    send_to_char(buf2, ch);
    sprintf(buf, "Attr Second:   [%s]\r\n", flag_string(stat_flags, class->attr_second));
    send_to_char(buf, ch);
    sprintf(buf, "Weapon Vnum:   [%d]\r\n", class->weapon);
    send_to_char(buf, ch);
    sprintf(buf, "Skill Adept:   [%d]\r\n", class->skill_adept);
    send_to_char(buf, ch);
    sprintf(buf, "Thac0_00:      [%d]\r\n", class->thac0_00);
    send_to_char(buf, ch);
    sprintf(buf, "Thac0_32:      [%d]\r\n", class->thac0_32);
    send_to_char(buf, ch);
    sprintf(buf, "HP Min:        [%d]\r\n", class->hp_min);
    send_to_char(buf, ch);
    sprintf(buf, "HP Max:        [%d]\r\n", class->hp_max);
    send_to_char(buf, ch);
    sprintf(buf, "Mana:          [%s]\r\n", class->mana ? "True" : "False");
    send_to_char(buf, ch);
    //sprintf(buf, "Moon:          [%s]\r\n",class->fMoon ? "True" : "False");
    sprintf(buf, "Is Reclass:    [%s]\r\n", class->is_reclass ? "True" : "False");
    send_to_char(buf, ch);
    sprintf(buf, "Is Enabled:    [%s]\r\n", class->is_enabled ? "True" : "False");
    send_to_char(buf, ch);

    if (class->clan > 0)
    {
        sprintf(buf, "Clan Specific: [%s]\r\n", clan_table[class->clan].friendly_name);
        send_to_char(buf, ch);
    }
    else
    {
        send_to_char("Clan Specific: [None]\r\n", ch);
    }

    sprintf(buf, "Base Group:    [%s]\r\n", class->base_group);
    send_to_char(buf, ch);
    sprintf(buf, "Default Group: [%s]\r\n", class->default_group);
    send_to_char(buf, ch);

    for (x = 0; x < MAX_GUILD; x++)
    {
        sprintf(buf, "%d) Guild:      [%d]\r\n", x, class->guild[x]);
        send_to_char(buf, ch);
    }

    sprintf(buf, "\r\nUse 'skills' and 'spells' to view spells for the %s class.\r\n", class->name);
    send_to_char(buf, ch);

    return FALSE;
}

// This is a Smaug thing, we're leaving it out for now
/*CEDIT( cedit_attrsecond )
{
    CLASSTYPE *class;
    int value;

    if ( argument[0] != '\0' )
    {
        EDIT_CLASS(ch, class);

        if ( ( value = flag_value( stat_flags, argument ) ) != NO_FLAG )
        {
            class->attr_second = value;

            send_to_char( "Second Attribute set.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char( "Syntax:  attrsecond [attr]\r\n", ch);
    return FALSE;
}*/

CEDIT(cedit_guild)
{
    CLASSTYPE *class;
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    int num, vnum;
    ROOM_INDEX_DATA *room;

    EDIT_CLASS(ch, class);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  guild [#] Room_Vnum\r\n", ch);
        return FALSE;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    num = atoi(arg1);
    vnum = atoi(arg2);

    if (!is_number(arg1) || !is_number(arg2))
    {
        send_to_char("Syntax:  guild [#] Room_Vnum\r\n", ch);
        return FALSE;
    }

    if (num > MAX_GUILD)
    {
        send_to_char("MAX_GUILD exceded, please increase MAX_GUILD in merc.h\r\n", ch);
        return FALSE;
    }

    if (vnum > top_vnum_room)
    {
        send_to_char("That is a greater vnum than any existing room.\r\n", ch);
        return FALSE;
    }


    if ((room = get_room_index(vnum)) == NULL)
    {
        send_to_char("That is an invalid room.\r\n", ch);
        return FALSE;
    }

    class->guild[num] = vnum;

    sendf(ch, "Guild slot %d set to {c%s{x [Room %d].\r\n", num, room->name, vnum);

    return TRUE;
}

CEDIT(cedit_skills)
{
    BUFFER *buffer;
    char arg[MAX_INPUT_LENGTH];
    char skill_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
    char skill_columns[LEVEL_HERO + 1];
    int sn, level, min_lev = 1, max_lev = LEVEL_HERO, class_no;
    //bool fAll = FALSE;
    bool found = FALSE;
    char buf[MAX_STRING_LENGTH];
    CLASSTYPE *class;

    EDIT_CLASS(ch, class);

    class_no = class_lookup(class->name);

    if (argument[0] != '\0')
    {
        //fAll = TRUE;

        if (str_prefix(argument, "all"))
        {
            argument = one_argument(argument, arg);
            if (!is_number(arg))
            {
                send_to_char("Arguments must be numerical or all.\r\n", ch);
                return FALSE;
            }
            max_lev = atoi(arg);

            if (max_lev < 1 || max_lev > LEVEL_HERO)
            {
                sprintf(buf, "Levels must be between 1 and %d.\r\n", LEVEL_HERO);
                send_to_char(buf, ch);
                return FALSE;
            }

            if (argument[0] != '\0')
            {
                argument = one_argument(argument, arg);
                if (!is_number(arg))
                {
                    send_to_char("Arguments must be numerical or all.\r\n", ch);
                    return FALSE;
                }
                min_lev = max_lev;
                max_lev = atoi(arg);

                if (max_lev < 1 || max_lev > LEVEL_HERO)
                {
                    sprintf(buf,
                        "Levels must be between 1 and %d.\r\n", LEVEL_HERO);
                    send_to_char(buf, ch);
                    return FALSE;
                }

                if (min_lev > max_lev)
                {
                    send_to_char("That would be silly.\r\n", ch);
                    return FALSE;
                }
            }
        }
    }

    /* initialize data */
    for (level = 0; level < LEVEL_HERO + 1; level++)
    {
        skill_columns[level] = 0;
        skill_list[level][0] = '\0';
    }

    for (sn = 0; sn < top_sn; sn++)
    {
        if (skill_table[sn]->name == NULL)
            break;

        if ((level = skill_table[sn]->skill_level[class_no]) < LEVEL_HERO + 1
            && level >= min_lev && level <= max_lev
            &&  skill_table[sn]->spell_fun == spell_null)
        {
            found = TRUE;
            level = skill_table[sn]->skill_level[class_no];
            sprintf(buf, "%-18s  [%2d]   ", skill_table[sn]->name, skill_table[sn]->rating[class_no]);

            if (skill_list[level][0] == '\0')
                sprintf(skill_list[level], "\r\nLevel %2d: %s", level, buf);
            else /* append */
            {
                if (++skill_columns[level] % 2 == 0)
                    strcat(skill_list[level], "\r\n          ");
                strcat(skill_list[level], buf);
            }
        }
    }

    /* return results */

    if (!found)
    {
        send_to_char("No skills found.\r\n", ch);
        return FALSE;
    }

    buffer = new_buf();
    for (level = 0; level < LEVEL_HERO + 1; level++)
        if (skill_list[level][0] != '\0')
            add_buf(buffer, skill_list[level]);
    add_buf(buffer, "\r\n");
    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
    return FALSE;
}

CEDIT(cedit_spells)
{
    BUFFER *buffer;
    char arg[MAX_INPUT_LENGTH];
    char skill_list[LEVEL_HERO + 1][MAX_STRING_LENGTH];
    char skill_columns[LEVEL_HERO + 1];
    int sn, level, min_lev = 1, max_lev = LEVEL_HERO, class_no;
    //bool fAll = FALSE, 
    bool found = FALSE;
    char buf[MAX_STRING_LENGTH];
    CLASSTYPE *class;

    EDIT_CLASS(ch, class);

    class_no = class_lookup(class->name);

    if (argument[0] != '\0')
    {
        //fAll = TRUE;

        if (str_prefix(argument, "all"))
        {
            argument = one_argument(argument, arg);
            if (!is_number(arg))
            {
                send_to_char("Arguments must be numerical or all.\r\n", ch);
                return FALSE;
            }
            max_lev = atoi(arg);

            if (max_lev < 1 || max_lev > LEVEL_HERO)
            {
                sprintf(buf, "Levels must be between 1 and %d.\r\n", LEVEL_HERO);
                send_to_char(buf, ch);
                return FALSE;
            }

            if (argument[0] != '\0')
            {
                argument = one_argument(argument, arg);
                if (!is_number(arg))
                {
                    send_to_char("Arguments must be numerical or all.\r\n", ch);
                    return FALSE;
                }
                min_lev = max_lev;
                max_lev = atoi(arg);

                if (max_lev < 1 || max_lev > LEVEL_HERO)
                {
                    sprintf(buf,
                        "Levels must be between 1 and %d.\r\n", LEVEL_HERO);
                    send_to_char(buf, ch);
                    return FALSE;
                }

                if (min_lev > max_lev)
                {
                    send_to_char("That would be silly.\r\n", ch);
                    return FALSE;
                }
            }
        }
    }


    /* initialize data */
    for (level = 0; level < LEVEL_HERO + 1; level++)
    {
        skill_columns[level] = 0;
        skill_list[level][0] = '\0';
    }

    for (sn = 0; sn < top_sn; sn++)
    {
        if (skill_table[sn]->name == NULL)
            break;

        if ((level = skill_table[sn]->skill_level[class_no]) < LEVEL_HERO + 1
            && level >= min_lev && level <= max_lev
            &&  skill_table[sn]->spell_fun != spell_null)
        {
            found = TRUE;
            level = skill_table[sn]->skill_level[class_no];
            sprintf(buf, "%-18s  [%2d]   ", skill_table[sn]->name, skill_table[sn]->rating[class_no]);

            if (skill_list[level][0] == '\0')
                sprintf(skill_list[level], "\r\nLevel %2d: %s", level, buf);
            else /* append */
            {
                if (++skill_columns[level] % 2 == 0)
                    strcat(skill_list[level], "\r\n          ");
                strcat(skill_list[level], buf);
            }
        }
    }

    /* return results */

    if (!found)
    {
        send_to_char("No skills found.\r\n", ch);
        return FALSE;
    }

    buffer = new_buf();
    for (level = 0; level < LEVEL_HERO + 1; level++)
        if (skill_list[level][0] != '\0')
            add_buf(buffer, skill_list[level]);
    add_buf(buffer, "\r\n");
    page_to_char(buf_string(buffer), ch);
    free_buf(buffer);
    return FALSE;
}

/* shows all groups, or the sub-members of a group */
CEDIT(cedit_groups)
{
    CLASSTYPE *class;
    char buf[100];
    int gn, sn, col, class_no;

    EDIT_CLASS(ch, class);

    class_no = class_lookup(class->name);

    col = 0;

    if (argument[0] == '\0')
    {   /* show all groups */

        for (gn = 0; gn < top_group; gn++)
        {
            if (group_table[gn]->name == NULL)
                break;

            if (group_table[gn]->rating[class_no] >= 0)
            {
                sprintf(buf, "%-20s ", group_table[gn]->name);
                send_to_char(buf, ch);
                if (++col % 3 == 0)
                    send_to_char("\r\n", ch);
            }
        }
        if (col % 3 != 0)
            send_to_char("\r\n", ch);
        return TRUE;
    }

    /* show the sub-members of a group */
    gn = group_lookup(argument);

    if (gn == -1)
    {
        send_to_char("No group of that name exist.\r\n", ch);
        send_to_char("Type 'groups all' or 'info all' for a full listing.\r\n", ch);
        return FALSE;
    }

    if (group_table[gn]->rating[class_no] < 0)
    {
        gn = -1;
    }

    if (gn == -1)
    {
        send_to_char("That group doesn't exist for this class.\r\n", ch);
        return FALSE;
    }

    for (sn = 0; sn < MAX_IN_GROUP; sn++)
    {
        int x;
        if (group_table[gn]->spells[sn] == NULL)
            break;
        x = skill_lookup(group_table[gn]->spells[sn]);

        if (skill_table[x]->skill_level[class_no] > LEVEL_HERO ||
            skill_table[x]->rating[class_no] < 0) continue;

        sprintf(buf, "%-20s ", group_table[gn]->spells[sn]);
        send_to_char(buf, ch);
        if (++col % 3 == 0)
            send_to_char("\r\n", ch);
    }
    if (col % 3 != 0)
        send_to_char("\r\n", ch);
    return TRUE;
}

/*
 * Creates a new skill or spell.  Something with a skill number that can be learned
 * and used by a player.
 */
SEDIT(sedit_create)
{
    SKILLTYPE *skill;
    int sn;
    int x;

    sn = skill_lookup(argument);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  edit skill create skill_name\r\n", ch);
        return FALSE;
    }

    if (sn != -1)
    {
        send_to_char("SEdit:  Skill already exists.\r\n", ch);
        return FALSE;
    }

    if (top_sn + 1 >= MAX_SKILL)
    {
        send_to_char("MAX_SKILL Exceeded, please increae the MAX_SKILL in merc.h\r\n", ch);
        return FALSE;
    }

    skill = alloc_perm(sizeof(*skill));
    skill->name = str_dup(argument);
    skill->noun_damage = str_dup("");
    skill->msg_off = str_dup("");
    skill->msg_obj = str_dup("");
    skill->spell_fun = spell_null;
    skill->race = 0;

    // Set the default level for all of the classes to the level of an immortal
    // so we will only have to set the levels of those classes receiving the skill.
    for (x = 0; x < top_class; x++)
    {
        skill->skill_level[x] = LEVEL_IMMORTAL;
        skill->rating[x] = -1;
    }

    // Put the n ew skill in the table then incriment the top_sn
    skill_table[top_sn] = skill;

    ch->desc->pEdit = (void *)skill;
    top_sn++;
    send_to_char("Skill Created.\r\n", ch);
    return TRUE;
}

SEDIT(sedit_show)
{
    SKILLTYPE *skill;
    char buf[MAX_STRING_LENGTH];
    int class;

    EDIT_SKILL(ch, skill);

    sprintf(buf, "\r\nName:         [%s]\r\n", skill->name);
    send_to_char(buf, ch);

    sprintf(buf, "SpellFun:     [%s]\r\n", spell_name_lookup(skill->spell_fun));
    send_to_char(buf, ch);

    sprintf(buf, "Target:       [%s]\r\n", flag_string(target_flags, skill->target));
    send_to_char(buf, ch);

    sprintf(buf, "MinPos:       [%s]\r\n", flag_string(position_flags, skill->minimum_position));
    send_to_char(buf, ch);

    sprintf(buf, "MinMana:      [%d]\r\n", skill->min_mana);
    send_to_char(buf, ch);

    sprintf(buf, "Beats:        [%d]\r\n", skill->beats);
    send_to_char(buf, ch);

    sprintf(buf, "Damage:       [%s]\r\n", skill->noun_damage);
    send_to_char(buf, ch);

    sprintf(buf, "MsgOff:       [%s]\r\n", skill->msg_off);
    send_to_char(buf, ch);

    sprintf(buf, "MsgObj:       [%s]\r\n", skill->msg_obj);
    send_to_char(buf, ch);

    if (skill->race == 0)
    {
        sprintf(buf, "Race:         [none]\r\n");
        send_to_char(buf, ch);
    }
    else
    {
        sprintf(buf, "Race:         [%s]\r\n", pc_race_table[skill->race].name);
        send_to_char(buf, ch);
    }

    sprintf(buf, "Ranged:       [%s]\n\r", skill->ranged ? "True" : "False");
    send_to_char(buf, ch);

    send_to_char("\r\nClass         Level  Rating  Class         Level  Rating\r\n", ch);
    send_to_char("--------------------------------------------------------\r\n", ch);

    for (class = 0; class < top_class; class++)
    {
        sprintf(buf, "%-13s [%2d]   [%2d]", class_table[class]->name, skill->skill_level[class], skill->rating[class]);
        if ((class % 2) == 0)
        {
            strcat(buf, "    ");
        }
        else
        {
            strcat(buf, "\r\n");
        }
        send_to_char(buf, ch);
    }
    send_to_char("\r\n", ch);

    return TRUE;
}

SEDIT(sedit_name)
{
    SKILLTYPE *skill;

    EDIT_SKILL(ch, skill);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  name [string]\r\n", ch);
        return FALSE;
    }

    free_string(skill->name);
    skill->name = str_dup(argument);

    send_to_char("Name set.\r\n", ch);
    return TRUE;
}

SEDIT(sedit_damage)
{
    SKILLTYPE *skill;

    EDIT_SKILL(ch, skill);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  damage [string]\r\n", ch);
        return FALSE;
    }

    free_string(skill->noun_damage);
    skill->noun_damage = str_dup(argument);

    send_to_char("Damage Noun set.\r\n", ch);
    return TRUE;
}

SEDIT(sedit_spellfun)
{
    SKILLTYPE *skill;

    EDIT_SKILL(ch, skill);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  spellfun [string]\r\n", ch);
        return FALSE;
    }

    skill->spell_fun = spell_function_lookup(argument);

    send_to_char("Spell function set.\r\n", ch);
    return TRUE;
}

SEDIT(sedit_target)
{
    SKILLTYPE *skill;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_SKILL(ch, skill);

        if ((value = flag_value(target_flags, argument)) != NO_FLAG)
        {
            skill->target = value;

            send_to_char("Target set.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char("Syntax:  target [flag]\r\n", ch);
    return FALSE;
}

SEDIT(sedit_minpos)
{
    SKILLTYPE *skill;
    int value;

    if (argument[0] != '\0')
    {
        EDIT_SKILL(ch, skill);

        if ((value = flag_value(position_flags, argument)) != NO_FLAG)
        {
            skill->minimum_position = value;

            send_to_char("Minpos set.\r\n", ch);
            return TRUE;
        }
    }

    send_to_char("Syntax:  minpos [flag]\r\n", ch);
    return FALSE;
}

SEDIT(sedit_minmana)
{
    SKILLTYPE *skill;

    EDIT_SKILL(ch, skill);

    if (argument[0] == '\0' || !is_number(argument))
    {
        send_to_char("Syntax:  minmana [number]\r\n", ch);
        return FALSE;
    }

    skill->min_mana = atoi(argument);

    send_to_char("Min Mana set.\r\n", ch);
    return TRUE;
}

SEDIT(sedit_beats)
{
    SKILLTYPE *skill;

    EDIT_SKILL(ch, skill);

    if (argument[0] == '\0' || !is_number(argument))
    {
        send_to_char("Syntax:  beats [number]\r\n", ch);
        return FALSE;
    }

    skill->beats = atoi(argument);

    send_to_char("Beats set.\r\n", ch);
    return TRUE;
}

SEDIT(sedit_msgoff)
{
    SKILLTYPE *skill;

    EDIT_SKILL(ch, skill);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  msgoff [string/clear]\r\n", ch);
        return FALSE;
    }

    if (skill->msg_off)
    {
        free_string(skill->msg_off);
        skill->msg_off = '\0';
    }
    if (str_cmp(argument, "clear"))
        skill->msg_off = str_dup(argument);

    send_to_char("Msgoff set.\r\n", ch);
    return TRUE;
}

SEDIT(sedit_msgobj)
{
    SKILLTYPE *skill;

    EDIT_SKILL(ch, skill);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  msgobj [string]\r\n", ch);
        return FALSE;
    }

    if (skill->msg_obj)
    {
        skill->msg_obj = '\0';
        free_string(skill->msg_obj);
    }
    if (str_cmp(argument, "clear"))
        skill->msg_obj = str_dup(argument);

    send_to_char("Msgobj set.\r\n", ch);
    return TRUE;
}

SEDIT(sedit_level)
{
    SKILLTYPE *skill;
    int class_no, level, rating;
    char class_name[MAX_INPUT_LENGTH];
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    EDIT_SKILL(ch, skill);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  level class [#]\r\n", ch);
        return FALSE;
    }

    argument = one_argument(argument, class_name);
    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);
    level = atoi(arg1);
    rating = atoi(arg2);

    if (!is_number(arg1) || level < 0 || level > MAX_LEVEL)
    {
        sprintf(buf, "Level range is from 0 to %d.\r\n", MAX_LEVEL);
        send_to_char(buf, ch);
        return FALSE;
    }

    if (!str_cmp(class_name, "all"))
    {
        for (class_no = 0; class_no < top_class; class_no++)
        {
            skill->skill_level[class_no] = level;
            skill->rating[class_no] = rating;
        }

        sprintf(buf, "OK, all classes will now gain %s at level %d.\r\n", skill->name, level);
        send_to_char(buf, ch);
    }
    else
    {

        for (class_no = 0; class_no < top_class; class_no++)
            if (!str_prefix(class_name, class_table[class_no]->name))
                break;

        if (!is_number(arg2))
            rating = skill->rating[class_no];

        if (class_no >= top_class)
        {
            sprintf(buf, "No class named '%s' exists.\r\n", class_name);
            send_to_char(buf, ch);
            return FALSE;
        }

        skill->skill_level[class_no] = level;
        skill->rating[class_no] = rating;

        sprintf(buf, "OK, %s will now gain %s at level %d.\r\n", class_table[class_no]->name, skill->name, level);
        send_to_char(buf, ch);

    }

    return TRUE;
}

SEDIT(sedit_rating)
{
    SKILLTYPE *skill;
    int class_no, rating;
    char class_name[MAX_INPUT_LENGTH];
    char arg1[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];

    EDIT_SKILL(ch, skill);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  rating class [#]\r\n", ch);
        return FALSE;
    }

    argument = one_argument(argument, class_name);
    argument = one_argument(argument, arg1);
    rating = atoi(arg1);

    if (!is_number(arg1))
    {
        send_to_char("Syntax:  rating class [#]\r\n", ch);
        return FALSE;
    }

    for (class_no = 0; class_no < top_class; class_no++)
        if (!str_prefix(class_name, class_table[class_no]->name))
            break;

    if (class_no >= top_class)
    {
        sprintf(buf, "No class named '%s' exists.\r\n", class_name);
        send_to_char(buf, ch);
        return FALSE;
    }

    skill->rating[class_no] = rating;
    sprintf(buf, "OK, %s will now cost %d for %s.\r\n", class_table[class_no]->name, rating, skill->name);
    send_to_char(buf, ch);
    return TRUE;
}

SEDIT(sedit_race)
{
    SKILLTYPE *skill;

    EDIT_SKILL(ch, skill);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  race [string]\r\n", ch);
        return FALSE;
    }

    skill->race = race_lookup(argument);

    send_to_char("Race set.\r\n", ch);
    return TRUE;
}

SEDIT(sedit_ranged)
{
    SKILLTYPE *skill;
    EDIT_SKILL(ch, skill);

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  ranged [true/false]\r\n", ch);
        return FALSE;
    }

    if (!str_prefix(argument,"true"))
    {
        skill->ranged = TRUE;
    }
    else if (!str_prefix(argument,"false"))
    {
        skill->ranged = FALSE;
    }
    else
    {
        send_to_char( "Syntax:  ranged [true/false]\r\n", ch );
        return FALSE;
    }

    return TRUE;
}

OEDIT(oedit_delete)
{
    OBJ_DATA *obj, *obj_next;
    OBJ_INDEX_DATA *pObj;
    RESET_DATA *pReset, *wReset;
    ROOM_INDEX_DATA *pRoom;
    char arg[MIL];
    char buf[MSL];
    int index, rcount, ocount, i, iHash;

    if (argument[0] == '\0')
    {
        send_to_char( "Syntax:  oedit delete [vnum]\r\n", ch );
        return FALSE;
    }

    one_argument(argument, arg);

    if (is_number(arg))
    {
        index = atoi(arg);
        pObj = get_obj_index(index);
    }
    else
    {
        send_to_char("That is not a number.\r\n", ch);
        return FALSE;
    }

    if (!pObj)
    {
        send_to_char("No such object.\r\n", ch);
        return FALSE;
    }

    SET_BIT(pObj->area->area_flags, AREA_CHANGED);

    if (top_vnum_obj == index)
    {
        for (i = 1; i < index; i++)
        {
            if(get_obj_index(i))
            {
                top_vnum_obj = i;
            }
        }
    }

    /* remove objects */
    ocount = 0;

    for (obj = object_list; obj; obj = obj_next)
    {
        obj_next = obj->next;

        if (obj->pIndexData == pObj)
        {
            extract_obj(obj);
            ocount++;
        }
    }

    /* crush resets */
    rcount = 0;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
        for (pRoom = room_index_hash[iHash]; pRoom; pRoom = pRoom->next)
        {
            for (pReset = pRoom->reset_first; pReset; pReset = wReset)
            {
                wReset = pReset->next;
                switch(pReset->command)
                {
                case 'O':
                case 'E':
                case 'P':
                case 'G':
                    if ((pReset->arg1 == index) ||
                        ((pReset->command == 'P') && (pReset->arg3 == index)))
                    {
                        unlink_reset(pRoom, pReset);
                        free_reset_data(pReset);

                        rcount++;
                        SET_BIT(pRoom->area->area_flags, AREA_CHANGED);
                    }
                }
            }
        }
    }

    unlink_obj_index(pObj);

    pObj->area = NULL;
    pObj->vnum = 0;

    free_obj_index(pObj);

    sprintf(buf, "Removed object vnum %d and %d resets.\r\n", index, rcount);
    send_to_char(buf, ch);

    sprintf(buf, "%d occurences of the object %d were extracted from the mud.\r\n", ocount, index);
    send_to_char(buf, ch);

    log_f("Deleted object vnum %d.", index);

    edit_done(ch);

    return TRUE;
}

MEDIT(medit_delete)
{
    CHAR_DATA *wch, *wnext;
    MOB_INDEX_DATA *pMob;
    RESET_DATA *pReset, *wReset;
    ROOM_INDEX_DATA *pRoom;
    char arg[MIL];
    int index, mcount, rcount, iHash, i;
    bool foundmob = FALSE;
    bool foundobj = FALSE;

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  medit delete [vnum]\r\n", ch);
        return FALSE;
    }

    one_argument(argument, arg);

    if (is_number(arg))
    {
        index = atoi(arg);
        pMob = get_mob_index(index);
    }
    else
    {
        send_to_char("That is not a number.\r\n", ch);
        return FALSE;
    }

    if (!pMob)
    {
        send_to_char("No such mobile.\r\n", ch);
        return FALSE;
    }

    SET_BIT(pMob->area->area_flags, AREA_CHANGED);

    if (top_vnum_mob == index)
    {
        for (i = 1; i < index; i++)
        {
            if (get_mob_index(i))
            {
                top_vnum_mob = i;
            }
        }
    }

    /* Now crush all resets and take out mobs while were at it */
    rcount = 0;
    mcount = 0;

    for(iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
        for (pRoom = room_index_hash[iHash]; pRoom; pRoom = pRoom->next)
        {
            for (wch = pRoom->people; wch; wch = wnext)
            {
                wnext = wch->next_in_room;

                if (wch->pIndexData == pMob)
                {
                    extract_char(wch, TRUE);
                    mcount++;
                }
            }

            for (pReset = pRoom->reset_first; pReset; pReset = wReset)
            {
                wReset = pReset->next;

                switch( pReset->command )
                {
                case 'M':
                    if( pReset->arg1 == index )
                    {
                        foundmob = TRUE;

                        unlink_reset( pRoom, pReset );
                        free_reset_data( pReset );

                        rcount++;
                        SET_BIT( pRoom->area->area_flags,
                        AREA_CHANGED );

                    }
                    else
                    {
                        foundmob = FALSE;
                    }

                    break;
                case 'E':
                case 'G':
                    if(foundmob)
                    {
                        foundobj = TRUE;

                        unlink_reset( pRoom, pReset );
                        free_reset_data( pReset );

                        rcount++;
                        SET_BIT( pRoom->area->area_flags, AREA_CHANGED );
                    }
                    else
                    {
                        foundobj = FALSE;
                    }

                    break;
                case '0':
                    foundobj = FALSE;
                    break;
                case 'P':
                    if(foundobj && foundmob)
                    {
                        unlink_reset(pRoom, pReset);
                        free_reset_data(pReset);

                        rcount++;
                        SET_BIT( pRoom->area->area_flags, AREA_CHANGED );
                    }
                }
            }
        }
    }

    SHOP_DATA *pShop;
    pShop = pMob->pShop;

    if (pMob->pShop != NULL)
    {
        pMob->pShop = NULL;

        if (pShop == shop_first)
        {
            if (!pShop->next)
            {
                shop_first = NULL;
                shop_last = NULL;
            }
            else
            {
                shop_first = pShop->next;
            }
        }
    }
    else
    {
        SHOP_DATA *ipShop;

        for (ipShop = shop_first; ipShop; ipShop = ipShop->next)
        {
            if (ipShop->next == pShop)
            {
                if (!pShop->next)
                {
                    shop_last = ipShop;
                    shop_last->next = NULL;
                }
                else
                {
                    ipShop->next = pShop->next;
                }
            }
        }
    }

    free_shop(pShop);

    unlink_mob_index(pMob);

    pMob->area = NULL;
    pMob->vnum = 0;

    free_mob_index(pMob);

    sendf(ch, "Removed mobile vnum %d and %d resets.\r\n", index, rcount);
    sendf(ch, "%d mobiles were extracted from the mud.\r\n", mcount);

    log_f("Deleted mobile vnum %d.", index);

    edit_done(ch);

    return TRUE;
}

REDIT(redit_delete)
{
    ROOM_INDEX_DATA *pRoom, *pRoom2;
    RESET_DATA *pReset;
    EXIT_DATA *ex;
    OBJ_DATA *Obj, *obj_next;
    CHAR_DATA *wch, *wnext;
    EXTRA_DESCR_DATA *pExtra;
    char arg[MIL];
    char buf[MSL];
    int index, i, iHash, rcount, ecount, mcount, ocount, edcount;

    if (argument[0] == '\0')
    {
        send_to_char("Syntax:  redit delete [vnum]\r\n", ch);
        return FALSE;
    }

    one_argument(argument, arg);

    if (is_number(arg))
    {
        index = atoi(arg);
        pRoom = get_room_index(index);
    }
    else
    {
        send_to_char("That is not a number.\r\n", ch);
        return FALSE;
    }

    if (!pRoom)
    {
        send_to_char("No such room.\r\n", ch);
        return FALSE;
    }

    /* Move the player out of the room. */
    if (ch->in_room->vnum == index)
    {
        send_to_char( "Moving you out of the room you are deleting.\r\n", ch);

        if (ch->fighting != NULL)
        {
            stop_fighting(ch, TRUE);
        }

        char_from_room(ch);
        char_to_room(ch, get_room_index(1200));
        ch->was_in_room = ch->in_room;
    }

    SET_BIT(pRoom->area->area_flags, AREA_CHANGED);

    /* Count resets. They are freed by free_room_index. */
    rcount = 0;

    for (pReset = pRoom->reset_first; pReset; pReset = pReset->next)
    {
        rcount++;
    }

    /* Now contents */
    ocount = 0;

    for (Obj = pRoom->contents; Obj; Obj = obj_next)
    {
        obj_next = Obj->next_content;

        extract_obj(Obj);
        ocount++;
    }

    /* Now PCs and Mobs */
    mcount = 0;

    for (wch = pRoom->people; wch; wch = wnext)
    {
        wnext = wch->next_in_room;

        if (IS_NPC(wch))
        {
            extract_char(wch, TRUE);
            mcount++;
        }
        else
        {
            send_to_char("This room is being deleted. Moving you somewhere safe.\r\n", ch);

            if (wch->fighting != NULL)
            {
                stop_fighting( wch, TRUE );
            }

            char_from_room(wch);
            char_to_room(wch, get_room_index(1200));
            wch->was_in_room = wch->in_room;
        }
    }

    /* unlink all exits to the room. */
    ecount = 0;
    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
        for (pRoom2 = room_index_hash[iHash]; pRoom2; pRoom2 = pRoom2->next)
        {
            for (i = 0; i <= MAX_DIR; i++)
            {
                if (!(ex = pRoom2->exit[i]))
                {
                    continue;
                }

                if (pRoom2 == pRoom)
                {
                    /* these are freed by free_room_index */
                    ecount++;
                    continue;
                }

                if (ex->u1.to_room == pRoom)
                {
                    free_exit(pRoom2->exit[i]);
                    pRoom2->exit[i] = NULL;
                    SET_BIT(pRoom2->area->area_flags, AREA_CHANGED);
                    ecount++;
                }
            }
        }
    }

    /* count extra descs. they are freed by free_room_index */
    edcount = 0;
    for (pExtra = pRoom->extra_descr; pExtra; pExtra = pExtra->next)
    {
        edcount++;
    }

    if (top_vnum_room == index)
    {
        for (i = 1; i < index; i++)
        {
            if (get_room_index(i))
            {
                top_vnum_room = i;
            }
        }
    }

    unlink_room_index(pRoom);

    pRoom->area = NULL;
    pRoom->vnum = 0;

    free_room_index(pRoom);

    /* Na na na na! Hey Hey Hey, Good Bye! */

    sprintf(buf, "Removed room vnum %d, %d resets, %d extra descriptions and %d exits.\r\n", index, rcount, edcount, ecount);
    send_to_char(buf, ch);
    sprintf(buf, "%d objects and %d mobiles were extracted from the room.\r\n", ocount, mcount );
    send_to_char(buf, ch);

    log_f("Deleted room vnum %d", index);

    return TRUE;
}


/*
 * Unlink a given reset from a given room
 */
void unlink_reset(ROOM_INDEX_DATA *pRoom, RESET_DATA *pReset)
{
    RESET_DATA *prev, *wReset;

    prev = pRoom->reset_first;

    for (wReset = pRoom->reset_first; wReset; wReset = wReset->next)
    {
        if (wReset == pReset)
        {
            if (pRoom->reset_first == pReset)
            {
                pRoom->reset_first = pReset->next;

                if (!pRoom->reset_first)
                {
                    pRoom->reset_last = NULL;
                }
            }
            else if (pRoom->reset_last == pReset)
            {
                pRoom->reset_last = prev;
                prev->next = NULL;
            }
            else
            {
                prev->next = prev->next->next;
            }
        }

        prev = wReset;
    }
}

void unlink_obj_index(OBJ_INDEX_DATA *pObj)
{
    int iHash;
    OBJ_INDEX_DATA *iObj, *sObj;

    iHash = pObj->vnum % MAX_KEY_HASH;

    sObj = obj_index_hash[iHash];

    if (sObj->next == NULL) /* only entry */
    {
        obj_index_hash[iHash] = NULL;
    }
    else if (sObj == pObj) /* first entry */
    {
        obj_index_hash[iHash] = pObj->next;
    }
    else /* everything else */
    {
        for (iObj = sObj; iObj != NULL; iObj = iObj->next)
        {
            if (iObj == pObj)
            {
                sObj->next = pObj->next;
                break;
            }

            sObj = iObj;
        }
    }
}


void unlink_room_index(ROOM_INDEX_DATA *pRoom)
{
    int iHash;
    ROOM_INDEX_DATA *iRoom, *sRoom;

    iHash = pRoom->vnum % MAX_KEY_HASH;

    sRoom = room_index_hash[iHash];

    if (sRoom->next == NULL) /* only entry */
    {
        room_index_hash[iHash] = NULL;
    }
    else if (sRoom == pRoom) /* first entry */
    {
        room_index_hash[iHash] = pRoom->next;
    }
    else /* everything else */
    {
        for (iRoom = sRoom; iRoom != NULL; iRoom = iRoom->next)
        {
            if (iRoom == pRoom)
            {
                sRoom->next = pRoom->next;
                break;
            }

            sRoom = iRoom;
        }
    }
}

void unlink_mob_index(MOB_INDEX_DATA *pMob)
{
    int iHash;
    MOB_INDEX_DATA *iMob, *sMob;

    iHash = pMob->vnum % MAX_KEY_HASH;

    sMob = mob_index_hash[iHash];

    if( sMob->next == NULL ) /* only entry */
    {
        mob_index_hash[iHash] = NULL;
    }
    else if( sMob == pMob ) /* first entry */
    {
        mob_index_hash[iHash] = pMob->next;
    }
    else /* everything else */
    {
        for (iMob = sMob; iMob != NULL; iMob = iMob->next)
        {
            if (iMob == pMob)
            {
                sMob->next = pMob->next;
                break;
            }

            sMob = iMob;
        }
    }
}

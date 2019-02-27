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
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "sha256.h"
#include <ctype.h> /* for isalpha() and isspace() -- JR */

char *reclass_list(CHAR_DATA *ch);
bool is_valid_reclass(CHAR_DATA *ch, int class, bool silent);

/*
 * Determines whether a requested reclass is valid for the character in
 * question.  If the call is not silent then the user will get an echo
 * as to why the class is not valid.
 */
bool is_valid_reclass(CHAR_DATA *ch, int class, bool silent)
{
    if (ch == NULL)
    {
        return FALSE;
    }

    // Initial checks
    if (class < 0
        || class > MAX_CLASS
        || IS_NPC(ch))
    {
        if (!silent)
        {
            send_to_char("That is not a valid class.\r\n", ch);
            send_to_char(reclass_list(ch), ch);
        }

        return FALSE;
    }

    // If the player wants to reclass into the same class they are (to try
    // to level and get better stats, add more skills, let them.
    /*
    if (ch->class == class)
    {
        return TRUE;
    }
    */

    // We will go through the checks and see if the char in question meets
    // the qualifications for the requested reclass.
    if (class_table[class]->is_reclass == FALSE)
    {
        if (!silent)
        {
            send_to_char("That is a base class, you must choose a reclass.\r\n", ch);
            send_to_char(reclass_list(ch), ch);
        }

        return FALSE;
    }
    else if (class_table[class]->is_enabled == FALSE)
    {
        if (!silent)
        {
            send_to_char("That reclass is currently disabled.\r\n", ch);
        }

        return FALSE;
    }
    else if (class_table[class]->clan > 0 && ch->clan != class_table[class]->clan)
    {
        if (!silent)
        {
            sendf(ch, "That is a clan specific reclass only available to %s.\r\n", clan_table[class_table[class]->clan].friendly_name);
        }

        return FALSE;
    }
    else if (class == ENCHANTER_CLASS && ch->class != MAGE_CLASS)
    {
        if (!silent)
        {
            send_to_char("Only mages can reclass into enchanters.\r\n", ch);
        }

        return FALSE;
    }
    else if (class == PSIONICIST_CLASS && ch->class != MAGE_CLASS)
    {
        if (!silent)
        {
            send_to_char("Only mages can reclass into psionicists.\r\n", ch);
        }

        return FALSE;
    }
    else if (class == HEALER_CLASS && ch->class != CLERIC_CLASS)
    {
        if (!silent)
        {
            send_to_char("Only clerics can reclass into healers.\r\n", ch);
        }

        return FALSE;
    }
    else if (class == DIRGE_CLASS
        && (ch->race != ELF_RACE && ch->race != HALF_ELF_RACE && ch->race != WILD_ELF_RACE && ch->race != SEA_ELF_RACE))
    {
        if (!silent)
        {
            send_to_char("Only elves can be dirges (excluding dark elves).\r\n", ch);
        }

        return FALSE;
    }
    else if (class == RANGER_CLASS &&
                (ch->class != WARRIOR_CLASS && ch->class != THIEF_CLASS))
    {
        if (!silent)
        {
            send_to_char("Only warriors or thieves can reclass into a ranger.\r\n", ch);
        }

        return FALSE;
    }
    else if (class == ROGUE_CLASS && ch->class != THIEF_CLASS)
    {
        if (!silent)
        {
            send_to_char("Only thieves can reclass into a rogue.\r\n", ch);
        }

        return FALSE;
    }
    else if (class == PRIEST_CLASS && ch->class != CLERIC_CLASS)
    {
        if (!silent)
        {
            send_to_char("Only clerics can reclass into a priest.\r\n", ch);
        }

        return FALSE;
    }
    else if (class == PRIEST_CLASS && ch->pcdata->deity == 0)
    {
        // Must follow a god or goddess in order to reclass into a priest.
        if (!silent)
        {
            send_to_char("Athiest's may not reclass into a priest.\r\n", ch);
        }

        return FALSE;
    }
    else if (class == BARBARIAN_CLASS && ch->class != WARRIOR_CLASS)
    {
        if (!silent)
        {
            send_to_char("Only warrior's can be barbarians.\r\n", ch);
        }

        return FALSE;
    }
    else if (class == BARBARIAN_CLASS && ch->race == KENDER_RACE)
    {
        if (!silent)
        {
            send_to_char("Kender cannot be barbarians.\r\n", ch);
        }

        return FALSE;
    }

    // If we got here we should be good.
    return TRUE;
}

/*
 * Reclasses the player into a new specialized class. (5/21/2015)
 */
void do_reclass(CHAR_DATA * ch, char *argument)
{
    extern int top_group;

    // Players must be at least level 10 to reclass.
    if (ch->level < 10)
    {
        send_to_char("You must be at least level 10 to reclass.\r\n", ch);
        return;
    }

    // Immortals can't reclass.. they must set.
    if (IS_NPC(ch) || IS_IMMORTAL(ch))
    {
        send_to_char("Immortals cannot reclass, use set to change your class instead.\r\n", ch);
        return;
    }

    if (class_table[ch->class]->is_reclass == TRUE)
    {
        send_to_char("You have already reclassed.\r\n", ch);
        return;
    }

    // Do not allow someone who is fighting or stunned to reclass.
    if (ch->position == POS_FIGHTING)
    {
        send_to_char("No way! You are fighting.\r\n", ch);
        return;
    }

    if (ch->position < POS_STUNNED)
    {
        send_to_char("You're not DEAD yet.\r\n", ch);
        return;
    }

    if (IS_NULLSTR(argument))
    {
        send_to_char("Syntax: reclass <reclass name>\r\n", ch);
        sendf(ch, "\r\n%s", reclass_list(ch));
        return;
    }

    // Declare the things we need now that we're for sure reclassing.
    AFFECT_DATA *af, *af_next;
    int class = 0;
    class = class_lookup(argument);
    int i = 0;
    long merit = 0;

    // Check if it's a valid reclass for the character.
    if (!is_valid_reclass(ch, class, FALSE))
    {
        return;
    }

    char buf[MSL];
    int oldLevel = 0;

    sprintf(buf, "$N is reclassing to %s", class_table[class]->name);
    wiznet(buf, ch, NULL, WIZ_GENERAL, 0, 0);

    // This is a temporary flag to identify the player as reclassing, do NOT save this
    // flag, we want it to reset when they leave
    ch->pcdata->is_reclassing = TRUE;

    // Remove the player from a group if they are in a group
    if (ch->master != NULL)
    {
        stop_follower(ch);
    }

    // Set the new class which is a reclass
    ch->class = class;

    // Level resets to 1, hit, mana and move also reset
    oldLevel = ch->level;
    ch->level = 1;

    ch->pcdata->perm_hit = 20;
    ch->max_hit = 20;
    ch->hit = 20;

    ch->pcdata->perm_mana = 100;
    ch->max_mana = 100;
    ch->mana = 100;

    ch->pcdata->perm_move = 100;
    ch->max_move = 100;
    ch->move = 100;

    // Reset the skill that they might have been focused on improving
    ch->pcdata->improve_focus_gsn = 0;

    // Reclassing will give a bonus so that the higher the level you were before you reclassed the more
    // initial trains/practices you get to start with.  This should encourage players to level up their
    // initial class.
    ch->train = 5;
    ch->train += oldLevel / 5;

    ch->practice = 5;
    ch->practice += oldLevel / 5;

    // Save the merits they had, we will use this to re-construct the merits at a later step.
    merit = ch->pcdata->merit;

    // Remove all merits the user had, we will re-add them later so any stats are correctly
    // reapplied.
    for (i = 0; merit_table[i].name != NULL; i++)
    {
        if (IS_SET(ch->pcdata->merit, merit_table[i].merit))
        {
            remove_merit(ch, merit_table[i].merit);
        }
    }

    // Reset the users stats back to default (they will have more trains now to up them quicker).
    for (i = 0; i < MAX_STATS; i++)
    {
        ch->perm_stat[i] = pc_race_table[ch->race].stats[i];
    }

    // Get rid of any pets in case they have a high level one that we don't them to have when their level
    // is halved.
    if (ch->pet != NULL)
    {
        nuke_pets(ch);
        ch->pet = NULL;
    }

    // Remove all of the players gear, we don't want someone to reclass and still be wearing
    // their high level gear.
    remove_all_obj(ch);

    // Remove any spells or affects so the player can't come out spelled up with
    // much higher levels spells.
    for (af = ch->affected; af != NULL; af = af_next)
    {
        af_next = af->next;
        affect_remove(ch, af);
    }

    // Remove the players wanted flag, we don't want them to get smoked right out of the gate, they
    // get a clean slate in this respect upon reclassing.
    REMOVE_BIT(ch->act, PLR_WANTED);

    // Call here for safety, this is also called on login.
    reset_char(ch);

    // Clear all previously known skills
    for (i = 0; i < top_sn; i++)
    {
        ch->pcdata->learned[i] = 0;
    }

    // Clear all previously known groups
    for (i = 0; i < top_group; i++)
    {
        if (group_table[i]->name == NULL)
            break;

        ch->pcdata->group_known[i] = 0;
    }

    // Add back Race skills
    for (i = 0; i < 5; i++)
    {
        if (pc_race_table[ch->race].skills[i] == NULL)
            break;

        group_add(ch, pc_race_table[ch->race].skills[i], FALSE);
    }

    // Reset points
    ch->pcdata->points = pc_race_table[ch->race].points;

    // Reset the merits, we saved the long value which is the state of all merits, we
    // will now reset it, then remove/re-add so their stat changes take affect.
    for (i = 0; merit_table[i].name != NULL; i++)
    {
        if (IS_SET(merit, merit_table[i].merit))
        {
            add_merit(ch, merit_table[i].merit);
        }
    }

    // Add back in the base groups
    group_add(ch, "rom basics", FALSE);
    group_add(ch, class_table[ch->class]->base_group, FALSE);
    ch->pcdata->learned[gsn_recall] = 50;

    sprintf(buf, "\r\n{YCongratulations{x, you are preparing to reclass as a %s.\r\n", class_table[ch->class]->name);
    send_to_char(buf, ch);

    send_to_char("\r\nDo you wish to customize this character?\r\n", ch);
    send_to_char("Customization takes time, but allows a wider range of skills and abilities.\r\n", ch);
    send_to_char("Customize (Y/N)? ", ch);

    // Move character to LIMBO so they can't be attacked or messed with while this
    // process is happening.  If they disconnect from reclass they will end up at
    // their last save point.  Reclassing players don't save.
    char_from_room(ch);
    char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));

    // Put them back in creation
    ch->desc->connected = CON_DEFAULT_CHOICE;

}

/*
 * Returns a list of all reclasses.
 */
char *reclass_list(CHAR_DATA *ch)
{
    int i;
    static char buf[MAX_STRING_LENGTH];

    buf[0] = '\0';

    // Get the list of valid reclasses so we can show them if we need to.
    // The reclasses start at position 4, that's not changing so we'll
    // start the loop there.  We will check each reclass to make sure
    // it's valid for the class of the calling character.
    sprintf(buf, "Valid reclasses for you are [");

    for (i = 4; i < top_class; i++)
    {
        if (class_table[i]->name == NULL)
        {
            log_string("BUG: null class");
            continue;
        }

        // Show only base classes, not reclasses.
        if (class_table[i]->is_reclass == TRUE
            && is_valid_reclass(ch, i, TRUE))
        {
            strcat(buf, " ");
            strcat(buf, class_table[i]->name);
        }
    }
    strcat(buf, " ]\r\n");

    return buf;
}

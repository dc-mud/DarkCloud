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
 *  Merits add abilities to a character, they cost creation points when a  *
 *  character is created.  They can also be earned from in game mechansims.*
 *  They are added and removed through add_merit and remove_merit which    *
 *  will set the bits necessary and then setup any conditions that need to *
 *  happen (like lower saves, etc).  Anything like this will also need to  *
 *  be accounted for in the reset_char function in handler.c if it affects *
 *  something like allow a stat above the maximum for that race/class.     *
 *  Merits that don't toggle stats like that (e.g. magic affinity which    *
 *  raises casting level) don't need to worry about that in either         *
 *  location (or can use a permanent affect added to apply_merit_affects). *
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
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "tables.h"
#include "interp.h"

/*
 * Merits that can increase a players ability in given areas.
 */
const struct merit_type merit_table[] = {
    { MERIT_DAMAGE_REDUCTION, "Damage Reduction", TRUE },     // -5% damage
    { MERIT_FAST_LEARNER, "Fast Learner", TRUE },             // Double experience, bonus on raising % on skills when learning, additional practice per level.
    { MERIT_HEALTHY, "Healthy", TRUE },                       // Increased regen, additional resistance to plague/poison
    { MERIT_CONSTITUTION, "Higher Constitution", TRUE },      // +1 constitution, +2 hp per level.
    { MERIT_DEXTERITY, "Higher Dexterity", TRUE },            // +1 Dexterity
    { MERIT_INTELLIGENCE, "Higher Intelligence", TRUE },      // +1 intelligence, +1 mana per level.
    { MERIT_STRENGTH, "Higher Strength", TRUE },              // +1 Strength
    { MERIT_WISDOM, "Higher Wisdom", TRUE },                  // +1 wisdom, +1 mana per level.
    { MERIT_LIGHT_FOOTED, "Light Footed", TRUE },             // Chance of reduced movement cost, access to sneak skill
    { MERIT_MAGIC_AFFINITY, "Magic Affinity", TRUE },         // +1 Casting Level
    { MERIT_MAGIC_PROTECTION, "Magic Protection", TRUE },     // -4 Saves
    { MERIT_PERCEPTION, "Perception", TRUE },                 // Permanent affects for -> detect hidden, detect invis, detect good, detect evil, detect magic
    { MERIT_PRECISION_STRIKING, "Precision Striking", TRUE }, // +5 hit roll
    { MERIT_INCREASED_DAMAGE, "Increased Damage", TRUE},      // +4 damage roll
    { 0, NULL, FALSE}
};

/*
 * Returns a list of the names of the merits a person has.
 */
char *merit_list(CHAR_DATA *ch)
{
    static char buf[1024];
    int i = 0;
    int found = 0;
    bool new_line = FALSE;

    if (IS_NPC(ch))
    {
        return "{CNone{x";
    }

    // Since this is static it's a must must must that it's reset, otherwise
    // it would concat onto the previous run and eventually ya know, stuff goes bad.
    buf[0] = '\0';

    for (i = 0; merit_table[i].name != NULL; i++)
    {
        if (IS_SET(ch->pcdata->merit, merit_table[i].merit))
        {
            if (found % 3 == 0 && found != 0)
            {
                strcat(buf, " \n          ");
                new_line = TRUE;
            }

            found++;

            if (!new_line)
            {
                strcat(buf, ", ");
            }

            new_line = FALSE;
            strcat(buf, "{C");
            strcat(buf, merit_table[i].name);
            strcat(buf, "{x");
        }
    }

    return (buf[0] != '\0') ? buf + 2 : "{CNone{x";
}

/*
 * Adds a merit to a character and initially puts the affect in place if it's
 * a merit that needs an attribute to change on a char.  remove_merit must be
 * called to properly remove the merit.
 */
void add_merit(CHAR_DATA *ch, long merit)
{
    // Not on NPC's and don't process the change (that reset_char would also process) if
    // the player already has the merit.
    if (IS_NPC(ch) || IS_SET(ch->pcdata->merit, merit))
    {
        return;
    }

    SET_BIT(ch->pcdata->merit, merit);

    // Every case here must have a corresponding removal case in remove_merit
    switch (merit)
    {
        case MERIT_MAGIC_PROTECTION:
        case MERIT_PERCEPTION:
        case MERIT_PRECISION_STRIKING:
        case MERIT_INCREASED_DAMAGE:
            apply_merit_affects(ch);
            break;
        case MERIT_CONSTITUTION:
            ch->perm_stat[STAT_CON] += 1;
            break;
        case MERIT_WISDOM:
            ch->perm_stat[STAT_WIS] += 1;
            break;
        case MERIT_INTELLIGENCE:
            ch->perm_stat[STAT_INT] += 1;
            break;
        case MERIT_STRENGTH:
            ch->perm_stat[STAT_STR] += 1;
            break;
        case MERIT_DEXTERITY:
            ch->perm_stat[STAT_DEX] += 1;
            break;
    }
}

/*
 * Removes a merit from a character and removes the affects (these affects would have
 * been removed on the next login or whenever reset_char was called).
 */
void remove_merit(CHAR_DATA *ch, long merit)
{
    // Not on NPC's and don't process the change (that reset_char would also process) if
    // the player already has the merit.
    if (IS_NPC(ch) || !IS_SET(ch->pcdata->merit, merit))
    {
        return;
    }

    REMOVE_BIT(ch->pcdata->merit, merit);

    // Every case here must have a corresponding removal case in remove_merit
    switch (merit)
    {
        case MERIT_MAGIC_PROTECTION:
            affect_strip(ch, gsn_magic_protection);
            break;
        case MERIT_PERCEPTION:
            affect_strip(ch, gsn_detect_hidden);
            affect_strip(ch, gsn_detect_invis);
            affect_strip(ch, gsn_detect_magic);
            affect_strip(ch, gsn_detect_good);
            affect_strip(ch, gsn_detect_evil);
            affect_strip(ch, gsn_dark_vision);
            break;
        case MERIT_PRECISION_STRIKING:
            affect_strip(ch, gsn_precision_striking);
            break;
        case MERIT_INCREASED_DAMAGE:
            affect_strip(ch, gsn_increased_damage);
            break;
        case MERIT_CONSTITUTION:
            ch->perm_stat[STAT_CON] -= 1;
            break;
        case MERIT_WISDOM:
            ch->perm_stat[STAT_WIS] -= 1;
            break;
        case MERIT_INTELLIGENCE:
            ch->perm_stat[STAT_INT] -= 1;
            break;
        case MERIT_STRENGTH:
            ch->perm_stat[STAT_STR] -= 1;
            break;
        case MERIT_DEXTERITY:
            ch->perm_stat[STAT_DEX] -= 1;
            break;
    }

}

/*
 * Applies any offical affects to a player for their given merits.  A merit affect can and should
 * still be cancelled off or dispelled if it's a normal spell, but will re-take affect on tick.
 */
void apply_merit_affects(CHAR_DATA *ch)
{
    AFFECT_DATA af;

    if (ch == NULL || IS_NPC(ch))
    {
        return;
    }

    // Perception
    if (IS_SET(ch->pcdata->merit, MERIT_PERCEPTION))
    {
        // Base affect settings
        af.where = TO_AFFECTS;
        af.level = ch->level;
        af.duration = -1;
        af.location = APPLY_NONE;
        af.modifier = 0;

        // Detect Hidden
        if (!is_affected(ch, gsn_detect_hidden))
        {
            af.type = gsn_detect_hidden;
            af.bitvector = AFF_DETECT_HIDDEN;
            affect_to_char(ch, &af);
        }

        // Detect Invisible
        if (!is_affected(ch, gsn_detect_invis))
        {
            af.type = gsn_detect_invis;
            af.bitvector = AFF_DETECT_INVIS;
            affect_to_char(ch, &af);
        }

        // Detect Magic
        if (!is_affected(ch, gsn_detect_magic))
        {
            af.type = gsn_detect_magic;
            af.bitvector = AFF_DETECT_MAGIC;
            affect_to_char(ch, &af);
        }

        // Detect Good
        if (!is_affected(ch, gsn_detect_good))
        {
            af.type = gsn_detect_good;
            af.bitvector = AFF_DETECT_GOOD;
            affect_to_char(ch, &af);
        }

        // Detect Evil
        if (!is_affected(ch, gsn_detect_evil))
        {
            af.type = gsn_detect_evil;
            af.bitvector = AFF_DETECT_EVIL;
            affect_to_char(ch, &af);
        }

        // Dark vision
        if (!is_affected(ch, gsn_dark_vision))
        {
            af.type = gsn_dark_vision;
            af.bitvector = AFF_DARK_VISION;
            affect_to_char(ch, &af);
        }
    }

    // Magic Resistance
    if (IS_SET(ch->pcdata->merit, MERIT_MAGIC_PROTECTION))
    {
        if (!is_affected(ch, gsn_magic_protection))
        {
            af.where = TO_AFFECTS;
            af.level = ch->level;
            af.duration = -1;
            af.location = APPLY_SAVES;
            af.modifier = -4;
            af.type = gsn_magic_protection;
            af.bitvector = 0;
            affect_to_char(ch, &af);
        }
    }

    // Precision Striking
    if (IS_SET(ch->pcdata->merit, MERIT_PRECISION_STRIKING))
    {
        if (!is_affected(ch, gsn_precision_striking))
        {
            af.where = TO_AFFECTS;
            af.level = ch->level;
            af.duration = -1;
            af.location = APPLY_HITROLL;
            af.modifier = 5;
            af.type = gsn_precision_striking;
            af.bitvector = 0;
            affect_to_char(ch, &af);
        }
    }

    // Increased Damage
    if (IS_SET(ch->pcdata->merit, MERIT_INCREASED_DAMAGE))
    {
        if (!is_affected(ch, gsn_increased_damage))
        {
            af.where = TO_AFFECTS;
            af.level = ch->level;
            af.duration = -1;
            af.location = APPLY_DAMROLL;
            af.modifier = 4;
            af.type = gsn_increased_damage;
            af.bitvector = 0;
            affect_to_char(ch, &af);
        }
    }


}

/*
 * Shows a list of all merits in the game and whether the player any of them.
 */
void do_meritlist(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;

    if (IS_NPC(ch))
    {
        send_to_char("This command is not usable by NPC's\r\n", ch);
        return;
    }

    if (!IS_IMMORTAL(ch))
    {
        // Mortals
        victim = ch;
    }
    else
    {
        // Immortals
        if (IS_NULLSTR(argument))
        {
            victim = ch;
        }
        else
        {
            if ((victim = get_char_world(ch, argument)) == NULL)
            {
                send_to_char("They aren't here.\r\n", ch);
                return;
            }

            if (IS_NPC(victim))
            {
                send_to_char("You cannot perform that on an NPC.\r\n", ch);
                return;
            }

            if (get_trust(victim) >= get_trust(ch))
            {
                send_to_char("You do not have trust to see their merits.\r\n", ch);
                return;
            }
        }
    }

    int i;

    send_to_char("--------------------------------------------------------------------\r\n", ch);

    if (victim == ch)
    {
        sendf(ch, "%-29s%-17s%-15s\r\n", "{WMerit{x", "{WYou Have{x", "{WPlayer Chooseable{x");
    }
    else
    {
        sendf(ch, "%-29s{W%s Has{x\r\n", "{WMerit{x", victim->name);
    }

    send_to_char("--------------------------------------------------------------------\r\n", ch);

    for (i = 0; merit_table[i].name != NULL; i++)
    {
        if (victim == ch)
        {
            sendf(ch, "%-25s%-17s%-13s\r\n",
                merit_table[i].name,
                IS_SET(victim->pcdata->merit, merit_table[i].merit) ? "{GTrue{x" : "{RFalse{x",
                merit_table[i].chooseable == TRUE ? "{GTrue{x" : "{RFalse{x");
        }
        else
        {
            sendf(ch, "%-25s%-17s\r\n",
                merit_table[i].name,
                IS_SET(victim->pcdata->merit, merit_table[i].merit) ? "{GTrue{x" : "{RFalse{x");
        }
    }

}

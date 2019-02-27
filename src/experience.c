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
 *  Experience                                                             *
 ***************************************************************************
 *                                                                         *
 *  This will house code related to calculating and distributing           *
 *  experience to players in order to gain levels.  This will also include *
 *  the logic to actually gain the levels.  Initially this will include    *
 *  the traditional Diku/Rom style experience but will and should also     *
 *  in the future include other ways to gain experience or different types *
 *  of experience.                                                         *
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
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "interp.h"
#include "tables.h"

/*
 * Compute xp for a kill.
 * Also adjust alignment of killer.
 * Edit this function to change xp computations.
 */
int xp_compute(CHAR_DATA * gch, CHAR_DATA * victim, int total_levels)
{
    int xp, base_exp;
    int level_range;
    int time_per_level;

    // No experience if the person is charmed or they are in the arena.
    if (IS_SET(gch->affected_by, AFF_CHARM)
        || IS_SET(gch->in_room->room_flags, ROOM_ARENA))
    {
        return 0;
    }

    xp = 0;
    level_range = victim->level - gch->level;

    /* compute the base exp */
    switch (level_range)
    {
        default:
            base_exp = 0;
            break;
        case -9:
            base_exp = 1;
            break;
        case -8:
            base_exp = 2;
            break;
        case -7:
            base_exp = 5;
            break;
        case -6:
            base_exp = 9;
            break;
        case -5:
            base_exp = 11;
            break;
        case -4:
            base_exp = 22;
            break;
        case -3:
            base_exp = 33;
            break;
        case -2:
            base_exp = 50;
            break;
        case -1:
            base_exp = 66;
            break;
        case 0:
            base_exp = 83;
            break;
        case 1:
            base_exp = 99;
            break;
        case 2:
            base_exp = 121;
            break;
        case 3:
            base_exp = 143;
            break;
        case 4:
            base_exp = 165;
            break;
    }

    // If level range is greater than 4 between player and victim then
    // give them a sliding bonus
    if (level_range > 4)
    {
        base_exp = 160 + 20 * (level_range - 4);
    }

    // Instead adding up the base, going to create slight increase here
    // that can be tweaked as needed.
    base_exp = base_exp * 4 / 3;

    /* do alignment computations */
    // XP was getting wiped out for certain setups here, put cases in for
    // everything.
    switch (gch->alignment)
    {
        case ALIGN_GOOD:
            switch (victim->alignment)
            {
                case ALIGN_GOOD: xp += base_exp * 2 / 3; break;
                case ALIGN_NEUTRAL: xp = base_exp; break;
                case ALIGN_EVIL: xp += base_exp * 3 / 2; break;
                default: break;
            }
            break;
        case ALIGN_NEUTRAL:
            switch (victim->alignment)
            {
                case ALIGN_EVIL: xp = base_exp; break;
                case ALIGN_GOOD: xp += base_exp * 5 / 4; break;
                case ALIGN_NEUTRAL: xp += base_exp * 4 / 5; break;
                default: break;
            }
            break;
        case ALIGN_EVIL:
            switch (victim->alignment)
            {
                case ALIGN_EVIL: xp += base_exp * 2 / 3; break;
                case ALIGN_NEUTRAL: xp = base_exp; break;
                case ALIGN_GOOD: xp += base_exp * 3 / 2; break;
                default: break;
            }
            break;
        default: break;
    }

    /* more exp at the low levels */
    if (gch->level < 6)
    {
        xp = 10 * xp / (gch->level + 4);
    }

    /* less at high */
    if (gch->level > 43)
    {
        xp = 15 * xp / (gch->level - 25);
    }

    /* reduce for playing time if the game setting for this is toggled on */
    if (settings.hours_affect_exp)
    {
        /* compute quarter-hours per level */
        time_per_level = 4 * hours_played(gch) / gch->level;
        time_per_level = URANGE(2, time_per_level, 12);

        /* make it a curve */
        if (gch->level < 15)
        {
            time_per_level = UMAX(time_per_level, (15 - gch->level));
        }

        xp = xp * time_per_level / 12;
    }

    // Any class or race specific bonuses, we'll put these together so they don't stack and
    // give a common small bonus
    if ((gch->class == RANGER_CLASS && gch->in_room->sector_type == SECT_FOREST)
        || (gch->race == ELF_RACE && gch->in_room->sector_type == SECT_FOREST)
        || (gch->race == DWARF_RACE && gch->in_room->sector_type == SECT_MOUNTAIN))
    {
        xp = xp * 20 / 19;
    }

    /* randomize the rewards */
    xp = number_range(xp * 3 / 4, xp * 5 / 4);

    /* adjust for grouping */
    xp = xp * gch->level / (UMAX(1, total_levels - 1));

    /* bonus for intelligence */
    xp = (xp * (100 + (get_curr_stat(gch, STAT_INT) * 4))) / 100;

    // Double experience bonus (this must be the last check to truly double
    // the calculation.  Also, people with the fast learner merit get this all
    // the time (they don't get double the double experience however)
    if (settings.double_exp || (!IS_NPC(gch) && IS_SET(gch->pcdata->merit, MERIT_FAST_LEARNER)))
    {
        xp *= 2;
    }

    return xp;
}

/*
 * Gain experience points in a group
 */
void group_gain(CHAR_DATA * ch, CHAR_DATA * victim)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *gch;
    CHAR_DATA *lch;
    int xp;
    int members;
    int group_levels;

    /*
     * Monsters don't get kill xp's or alignment changes.
     * P-killing doesn't help either.
     * Dying of mortal wounds or poison doesn't give xp to anyone!
     */
    if (victim == ch)
        return;

    members = 0;
    group_levels = 0;
    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
    {
        if (is_same_group(gch, ch))
        {
            members++;
            group_levels += IS_NPC(gch) ? gch->level / 2 : gch->level;
        }
    }

    if (members == 0)
    {
        bug("Group_gain: members.", members);
        members = 1;
        group_levels = ch->level;
    }

    lch = (ch->leader != NULL) ? ch->leader : ch;

    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
    {
        OBJ_DATA *obj;
        OBJ_DATA *obj_next;

        // Questing
        if (!IS_NPC(ch) && IS_SET(ch->act, PLR_QUESTOR) && IS_NPC(victim))
        {
            if (ch->pcdata->quest_mob == victim->pIndexData->vnum)
            {
                send_to_char("{GCongratulations.{x You have almost completed your QUEST!\r\n", ch);
                send_to_char("Return to the questmaster before your time runs out!\r\n", ch);
                ch->pcdata->quest_mob = -1;
            }
        }

        if (!is_same_group(gch, ch) || IS_NPC(gch))
            continue;

        if (gch->level - lch->level >= 8)
        {
            send_to_char("You are too high for this group.\r\n", gch);
            continue;
        }

        if (gch->level - lch->level <= -8)
        {
            send_to_char("You are too low for this group.\r\n", gch);
            continue;
        }

        xp = xp_compute(gch, victim, group_levels);

        sprintf(buf, "You receive %d experience points.\r\n", xp);
        send_to_char(buf, gch);
        gain_exp(gch, xp);

        for (obj = ch->carrying; obj != NULL; obj = obj_next)
        {
            obj_next = obj->next_content;
            if (obj->wear_loc == WEAR_NONE)
                continue;

            if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch))
                || (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch))
                || (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch)))
            {
                act("You are zapped by $p.", ch, obj, NULL, TO_CHAR);
                act("$n is zapped by $p.", ch, obj, NULL, TO_ROOM);
                obj_from_char(obj);
                obj_to_room(obj, ch->in_room);
            }
        }
    }

    return;
}

/*
 * Adds any computed experience gains onto a specific player.
 */
void gain_exp(CHAR_DATA * ch, int gain)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch) || ch->level >= LEVEL_HERO)
        return;

    ch->exp = UMAX(exp_per_level(ch, ch->pcdata->points), ch->exp + gain);
    while (ch->level < LEVEL_HERO && ch->exp >=
        exp_per_level(ch, ch->pcdata->points) * (ch->level + 1))
    {
        send_to_char("{GYou raise a level!!{x\r\n", ch);
        ch->level += 1;
        sprintf(buf, "%s gained level %d", ch->name, ch->level);
        log_string(buf);
        sprintf(buf, "$N has attained level %d!", ch->level);
        wiznet(buf, ch, NULL, WIZ_LEVELS, 0, 0);
        advance_level(ch, FALSE);
        save_char_obj(ch);
    }

    return;
}

/*
 * Advancement stuff.
 */
void advance_level(CHAR_DATA * ch, bool hide)
{
    char buf[MAX_STRING_LENGTH];
    int add_hp;
    int add_mana;
    int add_move;
    int add_prac;
    int add_train;

    ch->pcdata->last_level = hours_played(ch);

    add_hp = con_app[get_curr_stat(ch, STAT_CON)].hitp +
        number_range(class_table[ch->class]->hp_min, class_table[ch->class]->hp_max);

    add_mana = number_range(2, (2 * get_curr_stat(ch, STAT_INT)
        + get_curr_stat(ch, STAT_WIS)) / 5);

    // Not a magic type class (e.g. mage or cleric or their tree of reclasses)
    if (!class_table[ch->class]->mana)
    {
        add_mana /= 2;
    }

    add_move = number_range(1, (get_curr_stat(ch, STAT_CON) + get_curr_stat(ch, STAT_DEX)) / 6);

    // Practices are based off of wisdom
    add_prac = wis_app[get_curr_stat(ch, STAT_WIS)].practice;

    // Merit - Fast Learner (1 additional practice per level).
    if (IS_SET(ch->pcdata->merit, MERIT_FAST_LEARNER))
    {
        add_prac += 1;
    }

    add_hp = add_hp * 9 / 10;
    add_mana = add_mana * 9 / 10;
    add_move = add_move * 9 / 10;

    add_hp = UMAX(2, add_hp);
    add_mana = UMAX(2, add_mana);
    add_move = UMAX(6, add_move);

    // Bonuses by class
    switch (ch->class)
    {
        case HEALER_CLASS:
        case ENCHANTER_CLASS:
            // These classes are heavy on the mana usage, they'll get a slight bonus per level.
            add_mana += 2;
            break;
        case BARBARIAN_CLASS:
            // Barbarians have no spells and have to walk everywhere, give them a fairly large
            // movement bonus on level.
            add_move += 4;
            break;
    }

    // Merit - Constitution
    if (IS_SET(ch->pcdata->merit, MERIT_CONSTITUTION))
    {
        add_hp += 2;
    }

    // The merits for wisdom and intelligence are seperate checks so if someone has both
    // they will get 2 mana instead of 1.
    // Merit - Intelligence
    if (IS_SET(ch->pcdata->merit, MERIT_INTELLIGENCE))
    {
        add_mana += 1;
    }

    // Merit - Wisdom
    if (IS_SET(ch->pcdata->merit, MERIT_WISDOM))
    {
        add_mana += 1;
    }

    add_train = 1;

    ch->max_hit += add_hp;
    ch->max_mana += add_mana;
    ch->max_move += add_move;
    ch->practice += add_prac;
    ch->train += add_train;

    ch->pcdata->perm_hit += add_hp;
    ch->pcdata->perm_mana += add_mana;
    ch->pcdata->perm_move += add_move;

    if (!hide)
    {
        sprintf(buf,
            "You gain %d hit point%s, %d mana, %d move, %d practice%s and %d train%s.\r\n",
            add_hp, add_hp == 1 ? "" : "s", add_mana, add_move,
            add_prac, add_prac == 1 ? "" : "s",
            add_train, add_train == 1 ? "" : "s");
        send_to_char(buf, ch);
    }

    return;
}

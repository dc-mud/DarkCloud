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
*  Thief Class                                                             *
*                                                                          *
*  Thief is a base class for which many reclasses and/or other classes may *
*  share skills with.                                                      *
*                                                                          *
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
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"

/*
 * Evenenom - For poisoning weapons and food/drink
 */
void do_envenom(CHAR_DATA * ch, char *argument)
{
    OBJ_DATA *obj;
    AFFECT_DATA af;
    int percent, skill;

    /* find out what */
    if (argument[0] == '\0')
    {
        send_to_char("Envenom what item?\r\n", ch);
        return;
    }

    obj = get_obj_list(ch, argument, ch->carrying);

    if (obj == NULL)
    {
        send_to_char("You don't have that item.\r\n", ch);
        return;
    }

    if ((skill = get_skill(ch, gsn_envenom)) < 1)
    {
        send_to_char("Are you crazy? You'd poison yourself!\r\n", ch);
        return;
    }

    if (obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON)
    {
        if (IS_OBJ_STAT(obj, ITEM_BLESS)
            || IS_OBJ_STAT(obj, ITEM_BURN_PROOF))
        {
            act("You fail to poison $p.", ch, obj, NULL, TO_CHAR);
            return;
        }

        if (number_percent() < skill)
        {                        /* success! */
            act("$n treats $p with deadly poison.", ch, obj, NULL, TO_ROOM);
            act("You treat $p with deadly poison.", ch, obj, NULL, TO_CHAR);
            if (!obj->value[3])
            {
                obj->value[3] = 1;
                check_improve(ch, gsn_envenom, TRUE, 4);
            }
            WAIT_STATE(ch, skill_table[gsn_envenom]->beats);
            return;
        }

        act("You fail to poison $p.", ch, obj, NULL, TO_CHAR);
        if (!obj->value[3])
            check_improve(ch, gsn_envenom, FALSE, 4);
        WAIT_STATE(ch, skill_table[gsn_envenom]->beats);
        return;
    }

    if (obj->item_type == ITEM_WEAPON)
    {
        if (IS_WEAPON_STAT(obj, WEAPON_FLAMING)
            || IS_WEAPON_STAT(obj, WEAPON_FROST)
            || IS_WEAPON_STAT(obj, WEAPON_VAMPIRIC)
            || IS_WEAPON_STAT(obj, WEAPON_SHARP)
            || IS_WEAPON_STAT(obj, WEAPON_VORPAL)
            || IS_WEAPON_STAT(obj, WEAPON_SHOCKING)
            || IS_WEAPON_STAT(obj, WEAPON_LEECH)
            || IS_OBJ_STAT(obj, ITEM_BLESS)
            || IS_OBJ_STAT(obj, ITEM_BURN_PROOF))
        {
            act("You can't seem to envenom $p.", ch, obj, NULL, TO_CHAR);
            return;
        }

        if (obj->value[3] < 0
            || attack_table[obj->value[3]].damage == DAM_BASH)
        {
            send_to_char("You can only envenom edged weapons.\r\n", ch);
            return;
        }

        if (IS_WEAPON_STAT(obj, WEAPON_POISON))
        {
            act("$p is already envenomed.", ch, obj, NULL, TO_CHAR);
            return;
        }

        percent = number_percent();
        if (percent < skill)
        {
            separate_obj(obj);
            af.where = TO_WEAPON;
            af.type = gsn_poison;
            af.level = ch->level * percent / 100;
            af.duration = ch->level / 2 * percent / 100;
            af.location = 0;
            af.modifier = 0;
            af.bitvector = WEAPON_POISON;
            affect_to_obj(obj, &af);

            act("$n coats $p with deadly venom.", ch, obj, NULL, TO_ROOM);
            act("You coat $p with venom.", ch, obj, NULL, TO_CHAR);
            check_improve(ch, gsn_envenom, TRUE, 3);
            WAIT_STATE(ch, skill_table[gsn_envenom]->beats);
            return;
        }
        else
        {
            act("You fail to envenom $p.", ch, obj, NULL, TO_CHAR);
            check_improve(ch, gsn_envenom, FALSE, 3);
            WAIT_STATE(ch, skill_table[gsn_envenom]->beats);
            return;
        }
    }

    act("You can't poison $p.", ch, obj, NULL, TO_CHAR);
    return;
}

/*
 * Backstab skill
 */
void do_backstab(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Backstab whom?\r\n", ch);
        return;
    }

    if (ch->fighting != NULL)
    {
        send_to_char("You're facing the wrong end.\r\n", ch);
        return;
    }

    else if ((victim = get_char_room(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\r\n", ch);
        return;
    }

    if (victim == ch)
    {
        send_to_char("How can you sneak up on yourself?\r\n", ch);
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (IS_NPC(victim) &&
        victim->fighting != NULL && !is_same_group(ch, victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\r\n", ch);
        return;
    }

    if ((obj = get_eq_char(ch, WEAR_WIELD)) == NULL)
    {
        send_to_char("You need to wield a weapon to backstab.\r\n", ch);
        return;
    }

    if (victim->hit < victim->max_hit / 3)
    {
        act("$N is hurt and suspicious ... you can't sneak up.", ch, NULL, victim, TO_CHAR);
        return;
    }

    check_wanted(ch, victim);
    WAIT_STATE(ch, skill_table[gsn_backstab]->beats);
    if (number_percent() < get_skill(ch, gsn_backstab)
        || (get_skill(ch, gsn_backstab) >= 2 && !IS_AWAKE(victim)))
    {
        check_improve(ch, gsn_backstab, TRUE, 1);
        multi_hit(ch, victim, gsn_backstab);
    }
    else
    {
        check_improve(ch, gsn_backstab, FALSE, 1);
        damage(ch, victim, 0, gsn_backstab, DAM_NONE, TRUE);
    }

    return;

} // end do_backstab

/*
 * Allows a player to steal an item from another.  If the player is a clanner they will
 * receive a WANTED flag it it fails.  Kender are acute at stealing or "finding misplaced
 * items).
 */
void do_steal(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int percent;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
        send_to_char("Steal what from whom?\r\n", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg2)) == NULL)
    {
        send_to_char("They aren't here.\r\n", ch);
        return;
    }

    if (victim == ch)
    {
        send_to_char("That's pointless.\r\n", ch);
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (IS_NPC(victim) && victim->position == POS_FIGHTING)
    {
        send_to_char("Kill stealing is not permitted.\r\n"
            "You'd better not -- you might get hit.\r\n", ch);
        return;
    }

    if (victim->in_room != NULL && IS_SET(victim->in_room->room_flags, ROOM_ARENA))
    {
        send_to_char("You cannot steal in an arena.\r\n", ch);
        return;
    }

    WAIT_STATE(ch, skill_table[gsn_steal]->beats);
    percent = number_percent();

    if (!IS_AWAKE(victim))
        percent += 5;

    if (!can_see(victim, ch))
        percent += 5;

    if (ch->race == KENDER_RACE)
        percent += 20;

    if (((ch->level + 7 < victim->level || ch->level - 7 > victim->level)
        && !IS_NPC(victim) && !IS_NPC(ch))
        || (!IS_NPC(ch) && percent > get_skill(ch, gsn_steal))
        || (!IS_NPC(ch) && !is_clan(ch)))
    {
        /*
        * Failure.
        */
        send_to_char("Oops.\r\n", ch);
        affect_strip(ch, gsn_sneak);
        REMOVE_BIT(ch->affected_by, AFF_SNEAK);

        act("$n tried to steal from you.\r\n", ch, NULL, victim, TO_VICT);
        act("$n tried to steal from $N.\r\n", ch, NULL, victim, TO_NOTVICT);
        switch (number_range(0, 3))
        {
            case 0:
                sprintf(buf, "%s is a lousy thief!", ch->name);
                break;
            case 1:
                sprintf(buf, "%s couldn't rob %s way out of a paper bag!",
                    ch->name, (ch->sex == 2) ? "her" : "his");
                break;
            case 2:
                sprintf(buf, "%s tried to rob me!", ch->name);
                break;
            case 3:
                sprintf(buf, "Keep your hands out of there, %s!", ch->name);
                break;
        }
        if (!IS_AWAKE(victim))
            do_function(victim, &do_wake, "");
        if (IS_AWAKE(victim))
            do_function(victim, &do_yell, buf);
        if (!IS_NPC(ch))
        {
            if (IS_NPC(victim))
            {
                check_improve(ch, gsn_steal, FALSE, 2);
                multi_hit(victim, ch, TYPE_UNDEFINED);
            }
            else
            {
                sprintf(buf, "$N tried to steal from %s.", victim->name);
                wiznet(buf, ch, NULL, WIZ_FLAGS, 0, 0);
                if (!IS_SET(ch->act, PLR_WANTED))
                {
                    SET_BIT(ch->act, PLR_WANTED);
                    send_to_char("*** You are now ({RWANTED{x)!! ***\r\n", ch);
                    save_char_obj(ch);
                }
            }
        }

        return;
    }

    if (!str_cmp(arg1, "coin")
        || !str_cmp(arg1, "coins")
        || !str_cmp(arg1, "gold") || !str_cmp(arg1, "silver"))
    {
        int gold, silver;

        gold = victim->gold * number_range(1, ch->level) / MAX_LEVEL;
        silver = victim->silver * number_range(1, ch->level) / MAX_LEVEL;
        if (gold <= 0 && silver <= 0)
        {
            send_to_char("You couldn't get any coins.\r\n", ch);
            return;
        }

        ch->gold += gold;
        ch->silver += silver;
        victim->silver -= silver;
        victim->gold -= gold;
        if (silver <= 0)
            sprintf(buf, "Bingo!  You got %d gold coins.\r\n", gold);
        else if (gold <= 0)
            sprintf(buf, "Bingo!  You got %d silver coins.\r\n", silver);
        else
            sprintf(buf, "Bingo!  You got %d silver and %d gold coins.\r\n",
                silver, gold);

        send_to_char(buf, ch);
        check_improve(ch, gsn_steal, TRUE, 2);
        return;
    }

    if ((obj = get_obj_carry(victim, arg1, ch)) == NULL)
    {
        send_to_char("You can't find it.\r\n", ch);
        return;
    }

    if (!can_drop_obj(ch, obj)
        || IS_SET(obj->extra_flags, ITEM_INVENTORY)
        || obj->level > ch->level)
    {
        send_to_char("You can't pry it away.\r\n", ch);
        return;
    }

    if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch))
    {
        send_to_char("You have your hands full.\r\n", ch);
        return;
    }

    if (ch->carry_weight + get_obj_weight(obj) > can_carry_w(ch))
    {
        send_to_char("You can't carry that much weight.\r\n", ch);
        return;
    }

    separate_obj(obj);
    obj_from_char(obj);
    obj_to_char(obj, ch);
    act("You pocket $p.", ch, obj, NULL, TO_CHAR);
    check_improve(ch, gsn_steal, TRUE, 2);
    send_to_char("Got it!\r\n", ch);
    return;
}

/*
 * Dirt kicking skill to temporarly blind an opponent.  This is a shared skill between
 * multiple classes.
 */
void do_dirt(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument, arg);

    if ((chance = get_skill(ch, gsn_dirt)) == 0
        || (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_KICK_DIRT))
        || (!IS_NPC(ch)
            && ch->level < skill_table[gsn_dirt]->skill_level[ch->class]))
    {
        send_to_char("You get your feet dirty.\r\n", ch);
        return;
    }

    if (arg[0] == '\0')
    {
        victim = ch->fighting;
        if (victim == NULL)
        {
            send_to_char("But you aren't in combat!\r\n", ch);
            return;
        }
    }

    else if ((victim = get_char_room(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\r\n", ch);
        return;
    }

    if (IS_AFFECTED(victim, AFF_BLIND))
    {
        act("$E's already been blinded.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (victim == ch)
    {
        send_to_char("Very funny.\r\n", ch);
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (IS_NPC(victim) &&
        victim->fighting != NULL && !is_same_group(ch, victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\r\n", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
    {
        act("But $N is such a good friend!", ch, NULL, victim, TO_CHAR);
        return;
    }

    /* modifiers */

    /* dexterity */
    chance += get_curr_stat(ch, STAT_DEX);
    chance -= 2 * get_curr_stat(victim, STAT_DEX);

    /* speed  */
    if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags, OFF_FAST)
        || IS_AFFECTED(victim, AFF_HASTE))
        chance -= 25;

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* sloppy hack to prevent false zeroes */
    if (chance % 5 == 0)
        chance += 1;

    /* terrain */

    switch (ch->in_room->sector_type)
    {
        case (SECT_INSIDE):
            chance -= 20;
            break;
        case (SECT_CITY):
            chance -= 10;
            break;
        case (SECT_FIELD):
            chance += 5;
            break;
        case (SECT_FOREST):
            break;
        case (SECT_HILLS):
            break;
        case (SECT_MOUNTAIN):
            chance -= 10;
            break;
        case (SECT_WATER_SWIM):
            chance = 0;
            break;
        case (SECT_WATER_NOSWIM):
            chance = 0;
            break;
        case (SECT_OCEAN):
            chance = 0;
            break;
        case (SECT_UNDERWATER):
            chance = 0;
            break;
        case (SECT_AIR):
            chance = 0;
            break;
        case (SECT_DESERT):
            chance += 15;
            break;
        case (SECT_BEACH):
            chance += 15;
            break;
    }

    if (chance == 0)
    {
        send_to_char("There isn't any dirt to kick.\r\n", ch);
        return;
    }

    if (IS_TESTER(ch))
    {
        sendf(ch, "[Dirt Kick Chance {W%d%%{x]\r\n", chance);
    }

    /* now the attack */
    if (number_percent() < chance)
    {
        AFFECT_DATA af;
        act("$n is blinded by the dirt in $s eyes!", victim, NULL, NULL, TO_ROOM);
        act("$n kicks dirt in your eyes!", ch, NULL, victim, TO_VICT);
        damage(ch, victim, number_range(2, 5), gsn_dirt, DAM_NONE, FALSE);
        send_to_char("You can't see a thing!\r\n", victim);
        check_improve(ch, gsn_dirt, TRUE, 2);
        WAIT_STATE(ch, skill_table[gsn_dirt]->beats);

        af.where = TO_AFFECTS;
        af.type = gsn_dirt;
        af.level = ch->level;
        af.duration = 0;
        af.location = APPLY_HITROLL;
        af.modifier = -4;
        af.bitvector = AFF_BLIND;

        affect_to_char(victim, &af);
    }
    else
    {
        damage(ch, victim, 0, gsn_dirt, DAM_NONE, TRUE);
        check_improve(ch, gsn_dirt, FALSE, 2);
        WAIT_STATE(ch, skill_table[gsn_dirt]->beats);
    }
    check_wanted(ch, victim);
} // end do_dirt

  /*
   * Trip skill, if successful will daze the victim and knock them down.  This is a shared
   * skill between multiple classes.
   */
void do_trip(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument, arg);

    if ((chance = get_skill(ch, gsn_trip)) == 0
        || (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_TRIP))
        || (!IS_NPC(ch)
            && ch->level < skill_table[gsn_trip]->skill_level[ch->class]))
    {
        send_to_char("Tripping?  What's that?\r\n", ch);
        return;
    }


    if (arg[0] == '\0')
    {
        victim = ch->fighting;
        if (victim == NULL)
        {
            send_to_char("But you aren't fighting anyone!\r\n", ch);
            return;
        }
    }

    else if ((victim = get_char_room(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\r\n", ch);
        return;
    }

    if (is_safe(ch, victim))
        return;

    if (IS_NPC(victim) &&
        victim->fighting != NULL && !is_same_group(ch, victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\r\n", ch);
        return;
    }

    if (IS_AFFECTED(victim, AFF_FLYING))
    {
        act("$S feet aren't on the ground.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (victim->position < POS_FIGHTING)
    {
        act("$N is already down.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (victim == ch)
    {
        send_to_char("You fall flat on your face!\r\n", ch);
        WAIT_STATE(ch, 2 * skill_table[gsn_trip]->beats);
        act("$n trips over $s own feet!", ch, NULL, NULL, TO_ROOM);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
    {
        act("$N is your beloved master.", ch, NULL, victim, TO_CHAR);
        return;
    }

    /* modifiers */

    /* size */
    if (ch->size < victim->size)
        chance += (ch->size - victim->size) * 10;    /* bigger = harder to trip */

                                                     /* dex */
    chance += get_curr_stat(ch, STAT_DEX);
    chance -= get_curr_stat(victim, STAT_DEX) * 3 / 2;

    /* speed */
    if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags, OFF_FAST)
        || IS_AFFECTED(victim, AFF_HASTE))
        chance -= 20;

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* now the attack */
    if (number_percent() < chance)
    {
        act("$n trips you and you go down!", ch, NULL, victim, TO_VICT);
        act("You trip $N and $N goes down!", ch, NULL, victim, TO_CHAR);
        act("$n trips $N, sending $M to the ground.", ch, NULL, victim,
            TO_NOTVICT);
        check_improve(ch, gsn_trip, TRUE, 1);

        DAZE_STATE(victim, 2 * PULSE_VIOLENCE);
        WAIT_STATE(ch, skill_table[gsn_trip]->beats);
        victim->position = POS_RESTING;
        damage(ch, victim, number_range(2, 2 + 2 * victim->size), gsn_trip,
            DAM_BASH, TRUE);
    }
    else
    {
        damage(ch, victim, 0, gsn_trip, DAM_BASH, TRUE);
        WAIT_STATE(ch, skill_table[gsn_trip]->beats * 2 / 3);
        check_improve(ch, gsn_trip, FALSE, 1);
    }
    check_wanted(ch, victim);
} // end do_trip

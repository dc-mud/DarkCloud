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
 *  Warrior Class                                                           *
 *                                                                          *
 *  Warrior is a base class for which many reclasses and/or other classes   *
 *  may share skills with.                                                  *
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
* Berserk skill, get mad, crush things harder. Rawr.
*/
void do_berserk(CHAR_DATA * ch, char *argument)
{
    int chance, hp_percent;

    if ((chance = get_skill(ch, gsn_berserk)) == 0
        || (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_BERSERK))
        || (!IS_NPC(ch)
            && ch->level < skill_table[gsn_berserk]->skill_level[ch->class]))
    {
        send_to_char("You turn red in the face, but nothing happens.\r\n", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_BERSERK) || is_affected(ch, gsn_berserk)
        || is_affected(ch, gsn_frenzy))
    {
        send_to_char("You get a little madder.\r\n", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CALM))
    {
        send_to_char("You're feeling to mellow to berserk.\r\n", ch);
        return;
    }

    if (ch->mana < 50)
    {
        send_to_char("You can't get up enough energy.\r\n", ch);
        return;
    }

    /* modifiers */

    /* fighting */
    if (ch->position == POS_FIGHTING)
        chance += 10;

    /* damage -- below 50% of hp helps, above hurts */
    hp_percent = percent_health(ch);
    chance += 25 - hp_percent / 2;

    if (number_percent() < chance)
    {
        AFFECT_DATA af;

        WAIT_STATE(ch, PULSE_VIOLENCE);
        ch->mana -= 50;
        ch->move /= 2;

        /* heal a little damage */
        ch->hit += ch->level * 2;
        ch->hit = UMIN(ch->hit, ch->max_hit);

        send_to_char("Your pulse races as you are consumed by rage!\r\n",
            ch);
        act("$n gets a wild look in $s eyes.", ch, NULL, NULL, TO_ROOM);
        check_improve(ch, gsn_berserk, TRUE, 2);

        af.where = TO_AFFECTS;
        af.type = gsn_berserk;
        af.level = ch->level;
        af.duration = number_fuzzy(ch->level / 8);
        af.modifier = UMAX(1, ch->level / 5);
        af.bitvector = AFF_BERSERK;

        af.location = APPLY_HITROLL;
        affect_to_char(ch, &af);

        af.location = APPLY_DAMROLL;
        affect_to_char(ch, &af);

        af.modifier = UMAX(10, 10 * (ch->level / 5));
        af.location = APPLY_AC;
        affect_to_char(ch, &af);
    }

    else
    {
        WAIT_STATE(ch, 3 * PULSE_VIOLENCE);
        ch->mana -= 25;
        ch->move /= 2;

        send_to_char("Your pulse speeds up, but nothing happens.\r\n", ch);
        check_improve(ch, gsn_berserk, FALSE, 2);
    }
} // end do_berserk

  /*
  * Bash, one character slams into another and if succesful dazes them.
  */
void do_bash(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument, arg);

    if ((chance = get_skill(ch, gsn_bash)) == 0
        || (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_BASH))
        || (!IS_NPC(ch)
            && ch->level < skill_table[gsn_bash]->skill_level[ch->class]))
    {
        send_to_char("Bashing? What's that?\r\n", ch);
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

    if (victim->position < POS_FIGHTING)
    {
        act("You'll have to let $M get back up first.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (victim == ch)
    {
        send_to_char("You try to bash your brains out, but fail.\r\n", ch);
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
        act("But $N is your friend!", ch, NULL, victim, TO_CHAR);
        return;
    }

    /* modifiers */

    /* size  and weight */
    chance += ch->carry_weight / 250;
    chance -= victim->carry_weight / 200;

    if (ch->size < victim->size)
        chance += (ch->size - victim->size) * 15;
    else
        chance += (ch->size - victim->size) * 10;


    /* stats */
    chance += get_curr_stat(ch, STAT_STR);
    chance -= (get_curr_stat(victim, STAT_DEX) * 4) / 3;
    chance -= GET_AC(victim, AC_BASH) / 25;

    /* speed */
    if (IS_SET(ch->off_flags, OFF_FAST) || IS_AFFECTED(ch, AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags, OFF_FAST) || IS_AFFECTED(victim, AFF_HASTE))
        chance -= 30;

    /* level */
    chance += (ch->level - victim->level);

    // Kender are very hard to bash
    if (victim->race == KENDER_RACE)
        chance -= 20;

    if (!IS_NPC(victim) && chance < get_skill(victim, gsn_dodge))
    {
        chance -= 3 * (get_skill(victim, gsn_dodge) - chance);
    }

    /* now the attack */
    if (number_percent() < chance)
    {

        act("$n sends you sprawling with a powerful bash!", ch, NULL, victim, TO_VICT);
        act("You slam into $N, and send $M flying!", ch, NULL, victim, TO_CHAR);
        act("$n sends $N sprawling with a powerful bash.", ch, NULL, victim, TO_NOTVICT);
        check_improve(ch, gsn_bash, TRUE, 1);

        DAZE_STATE(victim, 3 * PULSE_VIOLENCE);
        WAIT_STATE(ch, skill_table[gsn_bash]->beats);
        victim->position = POS_RESTING;
        damage(ch, victim, number_range(2, 2 + 2 * ch->size + chance / 20), gsn_bash, DAM_BASH, FALSE);
    }
    else
    {
        damage(ch, victim, 0, gsn_bash, DAM_BASH, FALSE);
        act("You fall flat on your face!", ch, NULL, victim, TO_CHAR);
        act("$n falls flat on $s face.", ch, NULL, victim, TO_NOTVICT);
        act("You evade $n's bash, causing $m to fall flat on $s face.", ch, NULL, victim, TO_VICT);
        check_improve(ch, gsn_bash, FALSE, 1);
        ch->position = POS_RESTING;
        WAIT_STATE(ch, skill_table[gsn_bash]->beats * 3 / 2);
    }
    check_wanted(ch, victim);
} // end do_bash

/*
 * Disarm a creature.
 * Caller must check for successful attack.
 */
void disarm(CHAR_DATA * ch, CHAR_DATA * victim)
{
    OBJ_DATA *obj;
    OBJ_DATA *vobj;

    if ((obj = get_eq_char(victim, WEAR_WIELD)) == NULL)
        return;

    if (IS_OBJ_STAT(obj, ITEM_NOREMOVE))
    {
        act("$S weapon won't budge!", ch, NULL, victim, TO_CHAR);
        act("$n tries to disarm you, but your weapon won't budge!", ch, NULL, victim, TO_VICT);
        act("$n tries to disarm $N, but fails.", ch, NULL, victim, TO_NOTVICT);
        return;
    }

    act("$n DISARMS you and sends your weapon flying!", ch, NULL, victim, TO_VICT);
    act("You disarm $N!", ch, NULL, victim, TO_CHAR);
    act("$n disarms $N!", ch, NULL, victim, TO_NOTVICT);

    obj_from_char(obj);

    if (IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_INVENTORY))
    {
        obj_to_char(obj, victim);
    }
    else
    {
        obj_to_room(obj, victim->in_room);

        if (IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim, obj))
        {
            separate_obj(obj);
            get_obj(victim, obj, NULL);
        }
    }

    // If they were wielding a secondary weapon, move it to their primary arm.
    if ((vobj = get_eq_char(victim, WEAR_SECONDARY_WIELD)) != NULL)
    {
        obj_from_char(vobj);
        obj_to_char(vobj, victim);
        equip_char(victim, vobj, WEAR_WIELD);
        act("You switch $p to your swordarm.", victim, vobj, NULL, TO_CHAR);
        act("$n switches $p to $s swordarm.", victim, vobj, NULL, TO_ROOM);
    }

    return;
} // end void disarm


  /*
  * Disarm - attempt to knock your opponents weapon out of their hands.
  */
void do_disarm(CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int chance, hth, ch_weapon, vict_weapon, ch_vict_weapon;

    hth = 0;

    if ((chance = get_skill(ch, gsn_disarm)) == 0)
    {
        send_to_char("You don't know how to disarm opponents.\r\n", ch);
        return;
    }

    if (get_eq_char(ch, WEAR_WIELD) == NULL
        && ((hth = get_skill(ch, gsn_hand_to_hand)) == 0
            || (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_DISARM))))
    {
        send_to_char("You must wield a weapon to disarm.\r\n", ch);
        return;
    }

    if ((victim = ch->fighting) == NULL)
    {
        send_to_char("You aren't fighting anyone.\r\n", ch);
        return;
    }

    if ((obj = get_eq_char(victim, WEAR_WIELD)) == NULL)
    {
        send_to_char("Your opponent is not wielding a weapon.\r\n", ch);
        return;
    }

    /* find weapon skills */
    ch_weapon = get_weapon_skill(ch, get_weapon_sn(ch, FALSE));
    vict_weapon = get_weapon_skill(victim, get_weapon_sn(victim, FALSE));
    ch_vict_weapon = get_weapon_skill(ch, get_weapon_sn(victim, FALSE));

    /* modifiers */

    /* skill */
    if (get_eq_char(ch, WEAR_WIELD) == NULL)
        chance = chance * hth / 150;
    else
        chance = chance * ch_weapon / 100;

    chance += (ch_vict_weapon / 2 - vict_weapon) / 2;

    /* dex vs. strength */
    chance += get_curr_stat(ch, STAT_DEX);
    chance -= 2 * get_curr_stat(victim, STAT_STR);

    /* level */
    chance += (ch->level - victim->level) * 2;

    // Dirges get a disarm bonus
    if (is_affected(ch, gsn_dirge))
    {
        chance += 4;
    }

    /* and now the attack */
    if (number_percent() < chance)
    {
        WAIT_STATE(ch, skill_table[gsn_disarm]->beats);
        disarm(ch, victim);
        check_improve(ch, gsn_disarm, TRUE, 1);
    }
    else
    {
        WAIT_STATE(ch, skill_table[gsn_disarm]->beats);
        act("You fail to disarm $N.", ch, NULL, victim, TO_CHAR);
        act("$n tries to disarm you, but fails.", ch, NULL, victim, TO_VICT);
        act("$n tries to disarm $N, but fails.", ch, NULL, victim, TO_NOTVICT);
        check_improve(ch, gsn_disarm, FALSE, 1);
    }
    check_wanted(ch, victim);
    return;
} // end do_disarm

/*
 * Kick, does a little extra damage.
 */
void do_kick(CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *victim;

    if (!IS_NPC(ch)
        && ch->level < skill_table[gsn_kick]->skill_level[ch->class])
    {
        send_to_char("You better leave the martial arts to fighters.\r\n", ch);
        return;
    }

    if (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_KICK))
    {
        return;
    }

    if (IS_NULLSTR(argument))
    {
        if ((victim = ch->fighting) == NULL)
        {
            send_to_char("You aren't fighting anyone.\r\n", ch);
            return;
        }
    }
    else if ((victim = get_char_room(ch, argument)) == NULL)
    {
        send_to_char("They aren't here.\r\n", ch);
        return;
    }

    if (ch == victim)
    {
        send_to_char("You kick yourself.\r\n", ch);
        return;
    }

    if (is_safe(ch, victim ))
    {
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
    {
        act("But $N is your friend!",ch, NULL, victim, TO_CHAR);
        return;
    }

    WAIT_STATE(ch, skill_table[gsn_kick]->beats);
    check_wanted(ch, victim);

    if (get_skill(ch, gsn_kick) > number_percent())
    {
        damage(ch, victim, number_range(1, ch->level) + get_curr_stat(ch, STAT_STR), gsn_kick, DAM_BASH, TRUE);
        check_improve(ch, gsn_kick, TRUE, 1);
    }
    else
    {
        damage(ch, victim, 0, gsn_kick, DAM_BASH, TRUE);
        check_improve(ch, gsn_kick, FALSE, 1);
    }

    return;
}

/*
 * Skill to put yourself in front of another player being attacked to make
 * yourself the primary target.
 */
void do_rescue(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *fch;

    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char("Rescue whom?\r\n", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\r\n", ch);
        return;
    }

    if (victim == ch)
    {
        send_to_char("What about fleeing instead?\r\n", ch);
        return;
    }

    if (!IS_NPC(ch) && IS_NPC(victim))
    {
        send_to_char("Doesn't need your help!\r\n", ch);
        return;
    }

    if (ch->fighting == victim)
    {
        send_to_char("Too late.\r\n", ch);
        return;
    }

    if ((fch = victim->fighting) == NULL)
    {
        send_to_char("That person is not fighting right now.\r\n", ch);
        return;
    }

    if (IS_NPC(fch) && !is_same_group(ch, victim))
    {
        send_to_char("Kill stealing is not permitted.\r\n", ch);
        return;
    }

    WAIT_STATE(ch, skill_table[gsn_rescue]->beats);

    if (number_percent() > get_skill(ch, gsn_rescue))
    {
        send_to_char("You fail the rescue.\r\n", ch);
        check_improve(ch, gsn_rescue, FALSE, 1);
        return;
    }

    act("You rescue $N!", ch, NULL, victim, TO_CHAR);
    act("$n rescues you!", ch, NULL, victim, TO_VICT);
    act("$n rescues $N!", ch, NULL, victim, TO_NOTVICT);
    check_improve(ch, gsn_rescue, TRUE, 1);

    // Stop the fighting
    stop_fighting(fch, FALSE);
    stop_fighting(victim, FALSE);
    stop_fighting (ch, FALSE); // Gets rid of the extra attack round technique/bug

    // Check wanted, reset fighting.
    check_wanted(ch, fch);
    set_fighting(ch, fch);
    set_fighting(fch, ch);
    return;
} // end do_rescue

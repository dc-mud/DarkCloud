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
 *  Rogues                                                                 *
 *                                                                         *
 *  Rogue are sly individuals who prey on their victims vulnerabilities    *
 *  and borrow skills from various devious professions relating to the     *
 *  thieving trade.                                                        *
 *                                                                         *
 ***************************************************************************/

 /*
    TODO list
    Set Trap
 */

// General Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"

/*
 * A skill that allows a Rogue to prick a victim with a poison needle which
 * will lower their strength and add a poison effect although less so than
 * a poison spell.  This will land with a high efficiency and thus will have
 * a smaller effect than a poison spell which goes through saves.
 */
void do_poisonprick(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
    int chance = 0;
    char buf[MAX_STRING_LENGTH];

    // No NPC's can use this skill.
    if (IS_NPC(ch))
    {
        return;
    }

    // Must be over the level to use this skill.
    if (ch->level < skill_table[gsn_poison_prick]->skill_level[ch->class]
        || (chance = get_skill(ch, gsn_poison_prick)) == 0)
    {
        send_to_char("That is not something you are skilled at.\r\n", ch);
        return;
    }

    if (IS_NULLSTR(argument))
    {
        send_to_char("Who do you want to prick with a poison needle?\r\n", ch);
        return;
    }

    if ((victim = get_char_room(ch, argument)) == NULL)
    {
        send_to_char("They aren't here.\r\n", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
    {
        act("But $N is your friend!", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (is_affected(victim, gsn_poison_prick))
    {
        send_to_char("They are already poisoned.\r\n", ch);
        return;
    }

    if (is_safe(ch, victim))
    {
        return;
    }

    // See if a WANTED flag is warranted
    check_wanted(ch, victim);

    // The chance is going to start high with it being the users skill (assuming
    // daze state doesn't lower that.  We'll add/subtract based off of that.
    chance += (ch->level - victim->level) * 2;

    // Raise or lower based off of dex difference
    chance += ((get_curr_stat(ch, STAT_DEX) - get_curr_stat(victim, STAT_DEX)) * 2);

    if (IS_NPC(victim))
    {
        chance += 10;
    }

    // There is always some chance to succeed or fail (this will almost always be a
    // high chance skill).
    chance = URANGE( 5, chance, 95 );

    if (IS_TESTER(ch))
    {
        sprintf(buf, "[Poison Prick Chance {W%d%%{x]\r\n", chance);
        send_to_char(buf, ch);
    }

    if (!CHANCE(chance))
    {
        act("$N evades your poisoning attempt!", ch, NULL, victim, TO_CHAR);
        act("$n attempts to poison $N but fails.", ch, NULL, victim, TO_NOTVICT);
        act("You evade $n's attempt to poison you.", ch, NULL, victim, TO_VICT);

        check_improve(ch, gsn_poison_prick, FALSE, 5);

        // The time that must be waited after this command
        WAIT_STATE(ch, skill_table[gsn_poison_prick]->beats);

        return;
    }

    af.where = TO_AFFECTS;
    af.type = gsn_poison_prick;
    af.level = ch->level / 2;
    af.duration = 1;
    af.location = APPLY_STR;

    // More strength adjustment to mobs than players.
    if (IS_NPC(victim))
    {
        af.modifier = -2;
    }
    else
    {
        af.modifier = -1;
    }

    af.bitvector = AFF_POISON;
    affect_to_char(victim, &af);

    act("$n poisons you, you begin to feel very ill.", ch, NULL, victim, TO_VICT);
    act("You poison $N, then begin to look very ill.", ch, NULL, victim, TO_CHAR);
    act("$n poisons $N. $N begins to look very ill.", ch, NULL, victim, TO_NOTVICT);

    check_improve(ch, gsn_poison_prick, TRUE, 5);

    // The time that must be waited after this command
    WAIT_STATE(ch, skill_table[gsn_poison_prick]->beats);

}

/*
 * Shiv skill.  This is a stabblinb skill for Rogues.  This will be similiar to backstab but with
 * less damage but can be used beyond the % requirement for hp on the victim.
 */
void do_shiv(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim = NULL;
    OBJ_DATA *obj;
    int chance = 0;
    int dam = 0;

    // Must be over the level to use this skill.
    if (ch->level < skill_table[gsn_shiv]->skill_level[ch->class] || (chance = get_skill(ch, gsn_shiv)) == 0)
    {
        send_to_char("That is not something you are skilled at.\r\n", ch);
        return;
    }

    if (IS_NULLSTR(argument))
    {
        victim = ch->fighting;

        if (victim == NULL)
        {
            send_to_char("But you aren't fighting anyone!\r\n", ch);
            return;
        }
    }

    if (victim == NULL)
    {
        if ((victim = get_char_room(ch, argument)) == NULL)
        {
            send_to_char("They aren't here.\r\n", ch);
            return;
        }
    }

    if (victim == ch)
    {
        send_to_char("You cannot shiv yourself.\r\n", ch);
        return;
    }

    if (is_safe(ch, victim))
    {
        return;
    }

    if (IS_NPC(victim) &&
        victim->fighting != NULL && !is_same_group(ch, victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\r\n", ch);
        return;
    }

    if ((obj = get_eq_char(ch, WEAR_WIELD)) == NULL)
    {
        send_to_char("You need to wield a primary weapon to shiv someone.\r\n", ch);
        return;
    }

    check_wanted(ch, victim);

    chance = (chance * 2) / 3; // Base starting point
    chance += ((get_curr_stat(ch, STAT_DEX) * 4) - (get_curr_stat(victim, STAT_DEX) * 4)); // Dex vs. Dex

    // Haste vs. Haste vs. Slow (dex is already factor'd but the spells will add or subtract more)
    if (IS_AFFECTED(ch, AFF_HASTE))
    {
        chance += 10;
    }

    if (IS_AFFECTED(victim, AFF_HASTE))
    {
        chance -= 10;
    }

    if (IS_AFFECTED(ch, AFF_SLOW))
    {
        chance -= 20;
    }

    if (IS_AFFECTED(victim, AFF_SLOW))
    {
        chance += 20;
    }

    chance += ((ch->level - victim->level) * 2); // Level difference factors greatly
    chance += number_range(0, 10); // Randomness

    // Great chance against mobs
    if (IS_NPC(victim))
    {
        chance += 10;
    }

    // Reduce for AC vs. Pierce
    chance += GET_AC(victim, AC_PIERCE) / 25;

    // Lag for the command
    WAIT_STATE(ch, skill_table[gsn_shiv]->beats);

    // Show the percent to testers here
    if (IS_TESTER(ch))
    {
        sprintf(buf, "[Shiv Chance {W%d%%{x]\r\n", chance);
        send_to_char(buf, ch);
    }

    // Moment of truth
    if (!CHANCE(chance))
    {
        // Fail!  *sad trombone*
        act("$n attempts to shiv you but misses.", ch, NULL, victim, TO_VICT);
        act("You attempt to shiv $N but miss.", ch, NULL, victim, TO_CHAR);
        act("$n attempts to shiv $N and misses.", ch, NULL, victim, TO_NOTVICT);
        damage(ch, victim, 0, gsn_shiv, DAM_PIERCE, TRUE);
        check_improve(ch, gsn_shiv, FALSE, 2);
        return;
    }

    // Stab, calculate the damage
    dam = dice(obj->value[1], obj->value[2]) * (obj->level / 10);

    // More against NPC's or if the attacker is of a much greater level
    if (IS_NPC(victim) || (ch->level - victim->level) > 6)
    {
        dam += 30;
    }

    // Slight boost based on luck
    if (number_range(1, 3) == 3)
    {
        dam += 20;
    }

    // Do the damage
    damage(ch, victim, dam, gsn_shiv, DAM_PIERCE, TRUE);
    check_improve(ch, gsn_shiv, TRUE, 1);

    return;
}

/*
 * Escape command to get yourself out of battle with a high chance of
 * success.  This can only be used once every few ticks, it will require
 * a cool down period.  This will be similiar to flee except will work at
 * least once ignoring daze state (which is not something we will do for
 * other skills, this is unique and is likely a one time thing for a fight, if
 * a rogue flees and then is re-attacked it will no longer work until the cool
 * down period is over (if the cooldown is in effect, this will default to flee).
 * Closed doors will also be fleeable exits *IF* the character has pass door up.
 */
void do_escape(CHAR_DATA * ch, char *argument)
{
    ROOM_INDEX_DATA *was_in;
    ROOM_INDEX_DATA *now_in;
    CHAR_DATA *victim;
    int attempt;
    AFFECT_DATA af;
    int skill = 0;

    if (IS_NPC(ch))
    {
        send_to_char("Huh?\r\n", ch);
        return;
    }

    if (is_affected(ch, gsn_escape))
    {
        send_to_char("You cannot escape again so quickly.\r\n", ch);
        return;
    }

    if ((victim = ch->fighting) == NULL)
    {
        if (ch->position == POS_FIGHTING)
        {
            ch->position = POS_STANDING;
        }

        send_to_char("You aren't fighting anyone.\r\n", ch);
        return;
    }

    // We're going to check the skill this way here so it skips the daze
    // state.. this is a rare specialized skill that will avoid failing
    // because of stun... although it can still fail because you're not
    // good at it yet.
    if (ch->level < skill_table[gsn_escape]->skill_level[ch->class])
    {
        skill = 0;
    }
    else
    {
        skill = ch->pcdata->learned[gsn_escape];
    }


    if (!CHANCE(skill))
    {
        send_to_char("You stumble trying to make your escape!\r\n", ch);
        check_improve(ch, gsn_escape, FALSE, 8);
        return;
    }

    was_in = ch->in_room;
    for (attempt = 0; attempt < MAX_DIR; attempt++)
    {
        EXIT_DATA *pexit;
        int door;

        door = number_door();
        if ((pexit = was_in->exit[door]) == 0
            || pexit->u1.to_room == NULL
            || (IS_SET(pexit->exit_info, EX_CLOSED) && !IS_AFFECTED(ch, AFF_PASS_DOOR)))
        {
            continue;
        }

        move_char(ch, door, FALSE);

        if ((now_in = ch->in_room) == was_in)
            continue;

        ch->in_room = was_in;
        act("$n has escaped!", ch, NULL, NULL, TO_ROOM);
        ch->in_room = now_in;

        send_to_char("You have escaped from combat!\r\n", ch);

        stop_fighting(ch, TRUE);

        af.where = TO_AFFECTS;
        af.type = gsn_escape;
        af.level = 0;
        af.duration = 3;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = 0;
        affect_to_char(ch, &af);

        check_improve(ch, gsn_escape, TRUE, 9);

        return;
    }

    send_to_char("PANIC! You couldn't escape!\r\n", ch);
    return;

}

/*
 * Skill that will allow the Rogue to peer around the room with high
 * accuracy the number of other people in the room.  Even if they can't
 * see who is there they can determine if someone might be hiding.
 */
void do_peer(CHAR_DATA * ch, char *argument)
{
    int chance = 0;
    int count = 0;
    CHAR_DATA *victim;

    if (IS_NPC(ch))
    {
        send_to_char("Huh?\r\n", ch);
        return;
    }

    if (ch->fighting != NULL)
    {
        send_to_char("You're too busy fighting\r\n", ch);
        return;
    }

    // Must be over the level to use this skill.
    if (ch->level < skill_table[gsn_peer]->skill_level[ch->class]
        || (chance = get_skill(ch, gsn_peer)) == 0)
    {
        send_to_char("That is not something you are skilled at.\r\n", ch);
        return;
    }

    // Move have some movement left
    if (ch->move < 5)
    {
        send_to_char("You are too exhausted to peer.\r\n", ch);
        return;
    }

    // Slight movement cost
    ch->move -= 5;

    // There is always some chance to succeed or fail (this will almost always be a
    // high chance skill).
    chance = URANGE( 5, chance, 95 );

    if (IS_TESTER(ch))
    {
        sendf(ch, "[Peer Chance {W%d%%{x]\r\n", chance);
    }

    act("$N peers around the immediate area.", ch, NULL, NULL, TO_ROOM);

    // The time that must be waited after this command
    WAIT_STATE(ch, skill_table[gsn_peer]->beats);

    if (!CHANCE(chance))
    {
        check_improve(ch, gsn_peer, FALSE, 3);
        send_to_char("You do not see traces of anyone as you peer around the area.\r\n", ch);
        return;
    }

    // Get the count of eligble people in the room, which is everyone but immortals.
    for (victim = ch->in_room->people; victim != NULL; victim = victim->next_in_room)
    {
        if (!IS_IMMORTAL(victim) && victim != ch)
        {
            count++;
        }
    }

    if (count > 0)
    {
        sendf(ch, "You see signs of %d other individual%s here.\r\n", count, count > 1 ? "s" : "");
        check_improve(ch, gsn_peer, TRUE, 3);
    }
    else
    {
        check_improve(ch, gsn_peer, TRUE, 3);
        send_to_char("You do not see traces of anyone as you peer around the area.\r\n", ch);
    }

    return;
}

/*
 * Skill that will allow a rogue to use a mace and bludgeon the head of a victim, this will
 * cause a temporary lowering of int and/or wisdom for the victim and also a possible short
 * stun (if the victim has already been bludgeoned we'll not allow a second).
 */
void do_bludgeon(CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
    OBJ_DATA *weapon;
    int chance = 0;

    if (IS_NPC(ch))
    {
        send_to_char("Huh?\r\n", ch);
        return;
    }

    if ((victim = ch->fighting) == NULL)
    {
        if (ch->position == POS_FIGHTING)
        {
            ch->position = POS_STANDING;
        }

        send_to_char("You aren't fighting anyone.\r\n", ch);
        return;
    }

    // Must be over the level to use this skill.
    if (ch->level < skill_table[gsn_bludgeon]->skill_level[ch->class] || (chance = get_skill(ch, gsn_bludgeon)) == 0)
    {
        send_to_char("That is not something you are skilled at.\r\n", ch);
        return;
    }

    // The rogue must be wearing a mace in order to bludgeon someone effectively.
    if ((weapon = get_eq_char(ch, WEAR_WIELD)) == NULL)
    {
        send_to_char("You must be wielding a mace in order to bludgeon someone else effectively.\r\n", ch);
        return;
    }

    // They're wearing a weapon, now make sure that it is a mace specifically.
    switch (weapon->value[0])
    {
        case(WEAPON_MACE) :
            break;
        default:
            send_to_char("You must be wielding a mace in order to bludgeon someone else effectively.\r\n", ch);
            return;
    }

    if (is_affected(victim, gsn_bludgeon))
    {
        send_to_char("They have already been bludgeoned.\r\n", ch);
        return;
    }

    // Base, 75% of skill's %
    chance = (chance * 3) / 4;

    // Size difference will make it easier or smaller.
    chance += (ch->size - victim->size) * 2;

    // Take the characters dexterity into account on the hit, higher the dex, the less
    // that will be removed from the chance.
    if (get_curr_stat(ch, STAT_DEX) >= 0)
    {
        chance -= 25 - get_curr_stat(ch, STAT_DEX);
    }
    else
    {
        chance -= 25;
    }

    // Add or lower for level difference between the user and the victim
    chance += (ch->level - victim->level);

    // Higher success rate on mobs
    if (IS_NPC(victim))
    {
        chance += 15;
    }

    // Haste vs. Haste vs. Slow (dex is already factor'd but the spells will add or subtract more)
    if (IS_AFFECTED(ch, AFF_HASTE))
    {
        chance += 10;
    }

    if (IS_AFFECTED(victim, AFF_HASTE))
    {
        chance -= 10;
    }

    if (IS_AFFECTED(ch, AFF_SLOW))
    {
        chance -= 40;
    }

    if (IS_AFFECTED(victim, AFF_SLOW))
    {
        chance += 40;
    }

    // Always at least some chance for success or failure
    chance = URANGE(5, chance, 90);

    if (IS_TESTER(ch))
    {
        sendf(ch, "[Bludgeon Chance {W%d%%{x]\r\n", chance);
    }

    // The time that must be waited after this command
    WAIT_STATE(ch, skill_table[gsn_bludgeon]->beats);

    if (!CHANCE(chance))
    {
        act("$N evades your bludgeon!", ch, NULL, victim, TO_CHAR);
        act("$n attempts to bludgeon $N but misses.", ch, NULL, victim, TO_NOTVICT);
        act("You evade $n's attempt to bludgeon you.", ch, NULL, victim, TO_VICT);

        check_improve(ch, gsn_bludgeon, FALSE, 5);

        return;
    }

    // Success!
    af.where = TO_AFFECTS;
    af.type = gsn_bludgeon;
    af.level = ch->level;
    af.duration = 3;
    af.location = APPLY_INT;
    af.modifier = -1;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    af.location = APPLY_WIS;
    affect_to_char(victim, &af);

    act("You bludgeon $N across the head!", ch, NULL, victim, TO_CHAR);
    act("$n bludgeons $N across the head!", ch, NULL, victim, TO_NOTVICT);
    act("$n bludgeons you across the head!", ch, NULL, victim, TO_VICT);

    // Small stunning affect
    DAZE_STATE(victim, 3 * PULSE_VIOLENCE);

    check_improve(ch, gsn_poison_prick, FALSE, 5);
}

/*
 * Revolt will give the Rogue a chance to turn a charmy against it's master.  This is more of a
 * PVP skill.  Perhaps consider a different behavior against a NPC who isn't charmed.
 */
void do_revolt(CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *victim;
    CHAR_DATA *master;
    int chance = 0;
    int roll = 0;

    if (IS_NPC(ch))
    {
        send_to_char("Huh?\r\n", ch);
        return;
    }

    // Must be over the level to use this skill.
    if (((chance = get_skill(ch, gsn_revolt)) == 0) || ch->level < skill_table[gsn_revolt]->skill_level[ch->class])
    {
        send_to_char("Creating a revolt is not a skill you are good at.\r\n", ch);
        return;
    }

    // Get the victim from the room
    if ((victim = get_char_room(ch, argument)) == NULL)
    {
        send_to_char("They aren't here.\r\n", ch);
        return;
    }

    // Not on immortal characters
    if (IS_IMMORTAL(victim))
    {
        send_to_char("Their mind is too powerful conqueror.\r\n", ch);
        return;
    }

    // Against yourself?  Na.
    if (ch == victim)
    {
        send_to_char("You want to incite a revolt against yourself?  Ok.\r\n", ch);
        return;
    }

    // Is the victim safe from the character in question
    if (is_safe(ch, victim))
    {
        return;
    }

    // Does the victim follow a master?  If they do not, get out (and we won't lag).
    if (victim->master == NULL)
    {
        send_to_char("They do not currently have a master.\r\n", ch);
        return;
    }
    else
    {
        // Hold onto this for a second, we'll break they group first which will clear the victim->master, we
        // will then use this to add a round of hit on failure to start a fight.
        master = victim->master;
    }

    // Is the master safe?  We want to treat them like the victim.. we check at this point because we
    // know master has a value.
    if (is_safe(ch, master))
    {
        return;
    }

    // Let's calculate and then make a roll
    roll = chance + ((ch->level - victim->level) * 2);
    roll += (get_curr_stat(ch, STAT_INT) * 2) - (get_curr_stat(victim, STAT_INT) * 2);
    roll += (get_curr_stat(ch, STAT_WIS) * 2) - (get_curr_stat(victim, STAT_WIS) * 2);

    // Less of a chance on an actual player
    if (!IS_NPC(victim))
    {
        roll = (roll * 9) / 10;
    }

    // Always at least some chance for success or failure
    roll = URANGE(5, roll, 90);

    if (IS_TESTER(ch))
    {
        sendf(ch, "[Revolt Chance {W%d%%{x]\r\n", roll);
    }

    if (CHANCE(roll))
    {
        act("You incite $N to turn against their master!", ch, NULL, victim, TO_CHAR);
        act("$n incites $N to turn against their master!", ch, NULL, victim, TO_NOTVICT);
        act("$n incites you to revolt against your master!", ch, NULL, victim, TO_VICT);
        stop_follower(victim);
        check_improve(ch, gsn_revolt, TRUE, 8);

        // Let's start a fight!
        if (master != NULL)
        {
            one_hit(victim, master, TYPE_UNDEFINED, FALSE);
        }
    }
    else
    {
        act("You attempt to incite $N to turn against their master but fail.", ch, NULL, victim, TO_CHAR);
        act("$n attempts to incite $N but fails.", ch, NULL, victim, TO_NOTVICT);
        act("$n attempts to incite you to revolt against your master but fails.", ch, NULL, victim, TO_VICT);
        check_improve(ch, gsn_revolt, FALSE, 8);
    }

    WAIT_STATE(ch, skill_table[gsn_revolt]->beats);
}

/*
 * Smoke bomb, allows a Rogue to toss a smoke bomb down in the room creating a temporary smoke screen
 * that's hard to see through (e.g. fog restrung).
 */
void do_smokebomb(CHAR_DATA * ch, char *argument)
{
    OBJ_DATA *fog;
    OBJ_DATA *obj;
    int density = 0;
    int duration = 0;

    if (!CHANCE_SKILL(ch, gsn_smokebomb))
    {
        act("$n throws down a smoke bomb down that appears to fizzle out.", ch, NULL, NULL, TO_ROOM);
        act("You throw down a smoke bomb that appears to fizzle out.", ch, NULL, NULL, TO_CHAR);
        check_improve(ch, gsn_smokebomb, FALSE, 8);
        return;
    }

    for (obj = ch->in_room->contents; obj; obj = obj->next_content)
    {
        if (obj->item_type == ITEM_FOG)
        {
            send_to_char("There is already fog or smoke in the room.\r\n", ch);
            return;
        }
    }

    // Quality of the fog
    density = number_range(ch->level / 2, 100);
    duration = number_range(1, 3);

    // Much lower density in the city
    if (ch->in_room->sector_type == SECT_CITY)
    {
        density -= 30;
    }

    density = URANGE(5, density, 100);

    // Immortals cast impenetrable fog
    if (IS_IMMORTAL(ch))
    {
        density = 100;
    }

    fog = create_object(get_obj_index(OBJ_VNUM_FOG));
    fog->value[0] = density; // thickness of the fog
    fog->timer = duration; // duration of the fog

    // Restring the fog object.
    free_string(fog->short_descr);
    fog->short_descr = str_dup("A wall of smoke");

    free_string(fog->description);
    fog->description = str_dup("A wall of smoke hangs in the air.");

    obj_to_room(fog, ch->in_room);

    act("$n throws down a smoke bomb as smoke begins to engulf the area.", ch, NULL, NULL, TO_ROOM);
    act("You throw down a smoke bomb as smoke begins to engulf the area.", ch, NULL, NULL, TO_CHAR);

    check_improve(ch, gsn_smokebomb, TRUE, 7);

    // Lag for the command
    WAIT_STATE(ch, skill_table[gsn_smokebomb]->beats);

    return;
}

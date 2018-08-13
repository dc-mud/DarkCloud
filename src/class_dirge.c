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
 *                                                                         *
 *  Dirge                                                                  *
 *                                                                         *
 *  The code for dirge originated (slightly tweaked) from Moosehead        *
 *  Sled which has been made open source on GitHub.  The rest is my        *
 *  creation.  Dirge's combine magical hymns with a weapon masters attack  *
 *  ability                                                                *
 *                                                                         *
 ***************************************************************************/

// General Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"

/*
 * Dirge is the underpinning skill of all Dirge's.  They must be
 * under it's affects for any of their other skills to work.
 */
void do_dirge(CHAR_DATA *ch, char *arg)
{
    AFFECT_DATA af;
    int modifier = 0;

    // No NPC's
    if (IS_NPC(ch))
    {
        send_to_char("You cannot sing the mystic dirge.\r\n", ch);
        return;
    }

    if (ch->class != DIRGE_CLASS)
    {
        send_to_char("Huh?\r\n", ch);
        return;
    }

    if (is_affected(ch, gsn_dirge))
    {
        send_to_char("You are already affected by the mystic dirge.\r\n", ch);
        return;
    }

    if (ch->level < skill_table[gsn_dirge]->skill_level[ch->class])
    {
        send_to_char("You are not yet skilled enough.\r\n", ch);
        return;
    }

    // Success!
    act("You hum a haunting dirge.", ch, NULL, NULL, TO_CHAR);
    act("$n hums a haunting dirge.", ch, NULL, NULL, TO_ROOM);

    modifier = UMAX(1, ch->level / 10);

    af.where = TO_AFFECTS;
    af.type = gsn_dirge;
    af.level = ch->level;
    af.duration = ch->level / 5;
    af.modifier = modifier;
    af.location = APPLY_HITROLL;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    af.location = APPLY_DAMROLL;
    affect_to_char(ch, &af);

    af.modifier = modifier * -10;
    af.location = APPLY_AC;
    affect_to_char(ch, &af);

    check_improve(ch, gsn_dirge, TRUE, 5);

    // Lag for the command
    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

}

/*
 * A dirge spell that will offer all grouped members a small AC bonus.  This spell can be
 * cast both in and out of battle and does not require the dirge to be in affect.  It
 * can be cast over and over but the affect will only land once (it will refresh the affect
 * if cast again).
 */
void spell_song_of_protection(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *gch;
    AFFECT_DATA af;
    //bool found = FALSE;

    act("$n hums a melodic tune of protection.", ch, NULL, NULL, TO_ROOM);
    send_to_char("You hum a melodic tune of protection.\r\n", ch);

    // Will set these once here, then add them in the loop
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = ch->level;
    af.modifier = -10;
    af.location = APPLY_AC;
    af.bitvector = 0;

    // Find group members who aren't NPC's
    for (gch = char_list; gch != NULL; gch = gch->next)
    {
        if (!IS_NPC(gch) && is_same_group(gch, ch))
        {
            if (!is_affected(gch, sn))
            {
                // Strip the affect and re-add it
                if (is_affected(gch, sn))
                {
                    affect_strip(gch, sn);
                }

                act("You are now influenced by $n's song of protection.", ch, NULL, gch, TO_VICT);
                affect_to_char(gch, &af);
            }
        }
    }

    send_to_char("Your group is now influenced by the song of protection.\r\n", ch);

}

/*
 * A dirge that will allow the dirge to disorient (once) and add a few ticks
 * of a victim being deaf.  Awful singing FTW!
 */
void spell_song_of_dissonance(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *gch;
    AFFECT_DATA af;

    // Must be affected by the mystic dirge in order to use.
    if (!is_affected(ch, gsn_dirge))
    {
        send_to_char("You cannot use the song of dissonance unless you are affected by the mystic dirge.\r\n", ch);
        return;
    }

    act("$n hums a piercing dissonant tune.", ch, NULL, NULL, TO_ROOM);
    send_to_char("You hum a piercing dissonant tune.\r\n", ch);

    // Will set these once here, then add them in the loop
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 1;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_DEAFEN;

    // Find non group members, in the same room, who aren't NPC's
    for (gch = char_list; gch != NULL; gch = gch->next)
    {
        if (gch->in_room == NULL)
            continue;

        if (gch->in_room != ch->in_room)
            continue;

        if (!IS_NPC(gch)
            && !is_same_group(gch, ch)
            && gch != ch
            && !is_safe_spell(ch, gch, TRUE))
        {
            // They were a good target but they're already affected
            if (!is_affected(gch, sn))
            {
                // Very small stunning affect, this is due to no saves check
                DAZE_STATE(gch, PULSE_VIOLENCE);
                act("$N's looks pained.", ch, NULL, gch, TO_CHAR);
                send_to_char("You feeling a piercing pain in your ears as your hearing becomes muffled", gch);
                affect_to_char(gch, &af);
            }
        }
    }

} // end spell_song_of_dissonance

void do_disorient(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
    int modifier = 0;
    int chance = 0;

    // No NPC's can use this skill.
    if (IS_NPC(ch))
    {
        return;
    }

    // Must be over the level to use this skill.
    if (ch->level < skill_table[gsn_disorientation]->skill_level[ch->class])
    {
        send_to_char("You are not skilled enough to use the disorient technique.\r\n", ch);
        return;
    }

    // Must be affected by the mystic dirge
    if (!is_affected(ch, gsn_dirge))
    {
        send_to_char("You must be affected by the mystic dirge first.\r\n", ch);
        return;
    }

    // Must be fighting someone already, can't initiate with this skill.
    if (ch->fighting == NULL)
    {
        send_to_char("You're not fighting anybody to disorient.\r\n", ch);
        return;
    }
    else
    {
        // Set our victim
        victim = ch->fighting;
    }

    // You can't use this on someone who has charmed you.
    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
    {
        act("But $N is your friend!", ch, NULL, victim, TO_CHAR);
        return;
    }

    // See if the victim is already disoriented.
    if (is_affected(victim, gsn_disorientation))
    {
        send_to_char("Your opponent is already disoriented.\r\n", ch);
        WAIT_STATE(ch, skill_table[gsn_disorientation]->beats);
        check_improve(ch, gsn_disorientation, TRUE, 1);
        return;
    }

    // This is the skill check to see if they can pull off even executing it, we'll have the
    // check with other factors later.
    if (!CHANCE_SKILL(ch, gsn_disorientation))
    {
        act("$n attemps to disorient you but fails!", ch, NULL, victim, TO_VICT);
        act("You attempt to disorient $N but fail.", ch, NULL, victim, TO_CHAR);
        act("$n attempts to disorient $N but fails.", ch, NULL, victim, TO_NOTVICT);
        check_improve(ch, gsn_disorientation, FALSE, 2);
        WAIT_STATE(ch, skill_table[gsn_disorientation]->beats);
        return;
    }

    // Base
    chance = ((get_curr_stat(ch, STAT_DEX) - get_curr_stat(victim, STAT_DEX)) * 4) + number_range(20, 45);

    // If the character is affected by haste and the victim isn't they get a bonus, conversely, if the
    // victim is affected by haste and the player isn't, they get a penalty.
    if (IS_AFFECTED(ch, AFF_HASTE) && !IS_AFFECTED(victim, AFF_HASTE))
    {
        chance += 10;
    }
    else if (!IS_AFFECTED(ch, AFF_HASTE) && IS_AFFECTED(victim, AFF_HASTE))
    {
        chance -= 10;
    }

    // Elves get a bonus.  If we make this a race only reclass this check really won't matter.
    if (ch->race == ELF_RACE)
    {
        chance += 15;
    }
    else
    {
        chance -= 5;
    }

    // If the victim is blind they get a penalty UNLESS they have blind fighting and meet
    // the skill check, then they're good.
    if (IS_AFFECTED(victim, AFF_BLIND))
    {
        if (get_skill(victim, gsn_blind_fighting) < number_percent())
        {
            check_improve(victim, gsn_blind_fighting, TRUE, 2);
            chance += 10;
        }
    }

    // The attacker gets a bonus if they are a higher level, then get none of it's equal
    // and they lose some if they are of a less level
    if (victim->level < ch->level)
    {
        chance += ((ch->level - victim->level) * 2);
    }
    else if (victim->level > ch->level)
    {
        chance -= 5;
    }

    // Failure?
    if (chance < number_range(1, 100))
    {
        act("$n attempts to disorient you but fails!", ch, NULL, victim, TO_VICT);
        act("You attempt to disorient $N who is not affected.", ch, NULL, victim, TO_CHAR);
        act("$n attempts to disorient $N to no avail.", ch, NULL, victim, TO_NOTVICT);
        check_improve(ch, gsn_disorientation, FALSE, 1);
        WAIT_STATE(ch, skill_table[gsn_disorientation]->beats);
        return;
    }

    // Success!

    // Elves get a bonus
    if (ch->race == ELF_RACE)
    {
        modifier = 8;
    }
    else
    {
        modifier = 6;
    }

    af.where = TO_AFFECTS;
    af.type = gsn_disorientation;
    af.level = ch->level;
    af.duration = modifier;
    af.modifier = (modifier / 2) * -1;  // -3 or -4 dex
    af.location = APPLY_DEX;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    af.modifier = modifier * -1;
    af.location = APPLY_HITROLL;
    affect_to_char(victim, &af);

    // Disorientation will tire the victim a bit.
    victim->move -= (victim->move / 4);

    // Small stunning affect
    DAZE_STATE(victim, 3 * PULSE_VIOLENCE);

    // The normal wait state for the character
    WAIT_STATE(ch, skill_table[gsn_disorientation]->beats);

    // Show everyone what's going on.
    act("$n disorients you with their quick movements and haunting dirge!", ch, NULL, victim, TO_VICT);
    act("You disorient $N with your quick movements and haunting dirge!", ch, NULL, victim, TO_CHAR);
    act("$n disorients $N with their quick movements and haunting dirge.", ch, NULL, victim, TO_NOTVICT);

    // Check improvement
    check_improve(ch, gsn_disorientation, TRUE, 1);

} // end do_circle

/*
 * Stab is a skill that will allow a dirge to strike for an extra attack
 * if the victim is already disoriented.  It can only be used on their primary target.
 */
void do_stab(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;

    // Must be a character who has the skill.
    if (!IS_NPC(ch) && ch->level < skill_table[gsn_stab]->skill_level[ch->class])
    {
        send_to_char("You don't know how to the dirge's stab technique.\r\n", ch);
        return;
    }

    if (!is_affected(ch, gsn_dirge))
    {
        send_to_char("You must be affected by the mystic dirge first.\r\n", ch);
        return;
    }

    // The character can only use this in battle.
    if ((victim = ch->fighting) == NULL)
    {
        send_to_char("You aren't fighting anyone.\r\n", ch);
        return;
    }

    // You can't use this on someone who has charmed you.
    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
    {
        act("But $N is your friend!", ch, NULL, victim, TO_CHAR);
        return;
    }

    // See if the victim is already disoriented.
    if (!is_affected(victim, gsn_disorientation))
    {
        send_to_char("You victim must first be disoriented.\r\n", ch);
        WAIT_STATE(ch, skill_table[gsn_stab]->beats);
        check_improve(ch, gsn_stab, TRUE, 1);
        return;
    }

    // The time it takes to execute the command.
    WAIT_STATE(ch, skill_table[gsn_stab]->beats);

    if (CHANCE_SKILL(ch, gsn_stab))
    {
        // Success!
        act("$n stabs at you!", ch, NULL, victim, TO_VICT);
        act("You stab at $N!", ch, NULL, victim, TO_CHAR);
        act("$n stabs at $N!", ch, NULL, victim, TO_NOTVICT);

        // Chance for hits
        one_hit(ch, victim, TYPE_UNDEFINED, FALSE);
        one_hit(ch, victim, TYPE_UNDEFINED, FALSE);

        // Lower chance for a third
        if (CHANCE(33))
        {
            one_hit(ch, victim, TYPE_UNDEFINED, FALSE);
        }

        // Chance for stun (50% base at 51 if the player isn't stunned)
        int chance = ch->level / 2;
        chance += get_skill(ch, gsn_stab) / 4;

        if (IS_AFFECTED(ch, AFF_HASTE) && !IS_AFFECTED(victim, AFF_HASTE))
        {
            // Gain 25% if the character is hasted but the victim isn't
            chance += 25;
        }
        else if (!IS_AFFECTED(ch, AFF_HASTE) && IS_AFFECTED(victim, AFF_HASTE))
        {
            // Lose 25% if the character isn't hasted but the victim is
            chance -= 25;
        }

        // Harder to stun them while blind
        if (IS_AFFECTED(ch, AFF_BLIND) && !CHANCE_SKILL(ch, gsn_blind_fighting))
        {
            chance -= 30;
        }
        else if (IS_AFFECTED(victim, AFF_BLIND) && !CHANCE_SKILL(victim, gsn_blind_fighting))
        {
            chance += 15;
        }

        // Bonus or negative for level difference
        if (ch->level > victim->level)
        {
            chance += ch->level - victim->level;
        }
        else if (victim->level > ch->level)
        {
            chance -= victim->level - ch->level;
        }

        // Easier on mobs than on characters.
        if (IS_NPC(victim))
        {
            chance += 5;
        }
        else
        {
            chance -= 5;
        }

        if (IS_TESTER(ch))
        {
            sendf(ch, "[Stab Stun Chance {W%d%%{x]\r\n", chance);
        }

        if (CHANCE(chance))
        {
            // Success for stun!
            DAZE_STATE(victim, 3 * PULSE_VIOLENCE);
            act("$n strikes you across the head with the hilt of $m weapon.", ch, NULL, victim, TO_VICT);
            act("You strike $N across the head with the hilt of your weapon.", ch, NULL, victim, TO_CHAR);
            act("$n strikes $N across the head with the hilt of $m weapon.", ch, NULL, victim, TO_NOTVICT);
        }

        // Check to see if you get better at it
        check_improve(ch, gsn_stab, TRUE, 1);
    }
    else
    {
        // Failure
        act("$n attemps to circle stab you but stumbles in mid stride!", ch, NULL, victim, TO_VICT);
        act("You circle stab at $N but stumble in mid stride.", ch, NULL, victim, TO_CHAR);
        act("$n circle stabs at $N but stumbles in mid stride.", ch, NULL, victim, TO_NOTVICT);

        // Check to see if you get better at it
        check_improve(ch, gsn_stab, FALSE, 2);
    }

    check_wanted(ch, victim);
    return;

}

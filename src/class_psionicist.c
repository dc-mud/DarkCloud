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
 *  Psionicist Class                                                        *
 *                                                                          *
 *  Psionicst's are those with magics that come deep within the mind who    *
 *  can relay them onto physical entities in the world.                     *
 *                                                                          *
 *    - psionic blast                                                       *
 *    - healing dream                                                       *
 *    - mental weight                                                       *
 *    - forget                                                              *
 *    - psionic focus                                                       *
 *    - clairvoyance                                                        *
 *    - psionic shield                                                      *
 *    - boost                                                               *
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
#include "magic.h"
#include "recycle.h"

extern char *target_name;

/*
 * Psionic's Spell - Psionics Blast
 */
void spell_psionic_blast(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int dam;
    int hours = hours_played(ch);

    dam = dice(level, 12);

    // As the psionicist goes up in hours, they get a damage boost up to 1000 hours.
    if (hours >= 250 && hours < 500)
    {
        dam += 5;
    }
    else if (hours >= 500 && hours < 1000)
    {
        dam += 7;
    }
    else if (hours >= 1000)
    {
        dam += 10;
    }

    // First saves check to see if the damage is halved
    if (saves_spell(level, victim, DAM_MENTAL))
    {
        dam /= 2;
    }

    damage(ch, victim, dam, sn, DAM_MENTAL, TRUE);

    // Second saves check to see if there is an additional stun, short duration
    if (!saves_spell(level, victim, DAM_MENTAL))
    {
        send_to_char("You are knocked back by the psionic blast!\r\n", victim);
        act("$n has been knocked back by the psionic blast!", victim, NULL, NULL, TO_ROOM);

        if (number_range(1, 10) != 10)
        {
            // 90% of hitting this after the saves check
            DAZE_STATE(victim, PULSE_VIOLENCE);
        }
        else
        {
            // 10% of hitting this after the saves check
            DAZE_STATE(victim, PULSE_VIOLENCE);
        }

        return;
    }

    return;
}

/*
 * This is a spell that can be cast on others if they are sleeping.  It
 * cannot be cast on one's self but when a psioncist goes to sleep it will
 * automatically be added to their affects.
 */
void spell_healing_dream(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    // Must be awake and must not be an NPC
    if (IS_AWAKE(victim) || IS_NPC(victim))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    if (is_affected(victim, sn))
    {
        act("$N is already affected by a healing dream.", ch, NULL, victim, TO_CHAR);
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = -1;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(victim, &af);
    send_to_char("You fall into a deep and restful sleep.\r\n", victim);

    if (ch != victim)
    {
        act("Your magic lulls $N into a deep and restful sleep.", ch, NULL, victim, TO_CHAR);
    }

    return;
}

/*
 * Forget spell, will cause the victim to have a lower chanct to succeed on all skills/spells (lowers
 * both by 33%).
 */
void spell_forget(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;
    int duration = 2;

    if (is_affected(victim, gsn_forget))
    {
        send_to_char("They are already forgetful.\r\n", ch);
        return;
    }

    if (saves_spell(level, victim, DAM_MENTAL))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    // Duration is increased by one tick if the psion is older/stronger.
    if (hours_played(ch) > 1000)
    {
        duration++;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.duration = duration;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("Your mind feels clouded.\r\n", victim);
    act("$n looks as if their mind is clouded.", victim, NULL, NULL, TO_ROOM);
    return;
}

/*
 * Spell that will cause the victim to have 2x mana cost to cast a spell.
 */
void spell_mental_weight(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
        send_to_char("Their mind is already burdened.\r\n", ch);
        return;
    }

    if (saves_spell(level, victim, DAM_MENTAL))
    {
        send_to_char("The spell failed.\r\n", ch);
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 10;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("Your mind feels burdened and heavy.\r\n", victim);
    act("$n looks as if they are burdened.", victim, NULL, NULL, TO_ROOM);
    return;
}

/*
 * Psionic Focus - Adds a focus which will increase hit (and possibly give a few
 * other bonuses in skills checks (TODO)
 */
void spell_psionic_focus(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N's mind is already focused.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level / 2;
    af.location = APPLY_HITROLL;
    af.modifier = 3;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("Your mind becomes acutely focused.\r\n", victim);
    act("$n looks as if their mind is acutely focused.", victim, NULL, NULL, TO_ROOM);
    return;
}

/*
 * The clairvoyance spell will allow a psionicist to focus their mind on the current
 * room they are in (and then peek back in on it later using the clairvoyance command).
 */
void spell_clairvoyance(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    AFFECT_DATA af;

    // No NPC's ever can use this as it utilitzes pc_data
    if (IS_NPC(ch)
        || ch == NULL
        || ch->in_room == NULL)
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    // Remove the affect if it's already there
    affect_strip(ch, sn);

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 30;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    ch->pcdata->vnum_clairvoyance = ch->in_room->vnum;

    send_to_char("You focus your mind on the immediate surroundings.\r\n", ch);
    act("$n focuses their mind on the immediate surroundings.", ch, NULL, NULL, TO_ROOM);
    return;
}

/*
 * The clairvoyance command works in tandem with the spell.  The spell sets the location
 * that the psionicist wants to look at remotely, the command is the actual act of looking.
 * In order to cast the spell the psionicist has to be able to get to the room first which
 * will protect things like enemy clan rooms, etc.
 */
void do_clairvoyance(CHAR_DATA * ch, char *argument)
{
    if (ch == NULL
        || IS_NPC(ch))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    if (ch->class != PSIONICIST_CLASS)
    {
        send_to_char("Only psionicists are capable of clairvoyance.\r\n", ch);
        return;
    }

    if (!is_affected(ch, gsn_clairvoyance))
    {
        send_to_char("You are not currently affected by clairvoyance.\r\n", ch);
        return;
    }

    // Shouldn't happen, but this immortal area rooms are protected.
    if (ch->pcdata->vnum_clairvoyance < 100)
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    switch (ch->substate)
    {
        default:
            add_timer(ch, TIMER_DO_FUN, 24, do_clairvoyance, 1, NULL);
            send_to_char("Your eyes turn pale white as you begin your focus your clairvoyance...\r\n", ch);
            act("$n's eyes turn pale white.", ch, NULL, NULL, TO_ROOM);
            return;
        case 1:
            // Continue onward with the clairvoyance.
            break;
        case SUB_TIMER_DO_ABORT:
            ch->substate = SUB_NONE;
            send_to_char("Your eyes return to normal as you break your clairvoyant focus.\r\n", ch);
            act("$n's eyes return to normal.", ch, NULL, NULL, TO_ROOM);
            return;
    }

    ch->substate = SUB_NONE;

    ROOM_INDEX_DATA *original_location;
    ROOM_INDEX_DATA *remote_location;

    remote_location = get_room_index(ch->pcdata->vnum_clairvoyance);

    // Make the check to see if they can see to the other side or not
    if (remote_location == NULL
        || remote_location == ch->in_room
        || !can_see_room(ch, remote_location)
        || ch->fighting != NULL
        || (room_is_private(remote_location) && !is_room_owner(ch, remote_location)))
    {
        send_to_char("You see swirling chaos...\r\n", ch);
        return;
    }

    // Show the other side to the player, this will move them, look and then move them back.
    original_location = ch->in_room;
    char_from_room(ch);
    char_to_room(ch, remote_location);
    do_look(ch, "auto");
    char_from_room(ch);
    char_to_room(ch, original_location);

    send_to_char("\r\nYour eyes return to normal as you break your clairvoyant focus.\r\n", ch);
    act("$n's eyes return to normal.", ch, NULL, NULL, TO_ROOM);
}

/*
 * Spell that gives psionicists a small amount of armor class an additional save.
 */
void spell_psionic_shield(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N's is already surrounded by a psionic shield.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level / 2;
    af.location = APPLY_AC;
    af.modifier = -10;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    af.location = APPLY_SAVES;
    af.modifier = -1;;
    affect_to_char(victim, &af);

    send_to_char("You are enveloped by a psionic shield.\r\n", victim);
    act("$n is enveloped by a psionic shield.", victim, NULL, NULL, TO_ROOM);

    return;
}

/*
 * A spell that will allow a psionicist to boost a single stat of their choice.
 * c 'boost' blake strength.  This spell will need to be setup with a target of
 * ignore so that we can parse the target_name variable ourselves.  This spell
 * requires more of the argument to know what to do making it out of the norm for
 * spell setup.
 */
void spell_boost(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
    char arg[MAX_STRING_LENGTH];
    char victim_name[MAX_STRING_LENGTH];

    // Pluck the victim's name and the argument off the "target_name".
    target_name = one_argument(target_name, victim_name);
    target_name = one_argument(target_name, arg);

    if (IS_NULLSTR(victim_name) || IS_NULLSTR(arg))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    if ((victim = get_char_room(ch, victim_name)) == NULL)
    {
        send_to_char("They aren't here.\r\n", ch);
        return;
    }

    // Strip it on both the victim regardless whether it's there or not
    affect_strip(victim, gsn_boost);

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level / 2;
    af.bitvector = 0;

    // If the psionicist is affected by psionic focus then there is a chance
    // the modifier can jump from 1 to 2.  Otherwise, it's always 1.
    if (is_affected(ch, gsn_psionic_focus))
    {
        af.modifier = number_range(1, 2);
    }
    else
    {
        af.modifier = 1;
    }

    // Now, find out what the psionicist wants to enhance on the person..
    if (!str_prefix(arg, "strength"))
    {
        if (ch != victim)
        {
            act("$n focuses $s psionic energy into your strength.", ch, NULL, victim, TO_VICT);
            act("You focus your psionic energy into $N's strength.", ch, NULL, victim, TO_CHAR);
            act("$n focuses $s psionic energy into $N's strength.", ch, NULL, victim, TO_NOTVICT);
        }
        else
        {
            send_to_char("You focus your psionic energy into your strength.\r\n", ch);
            act("$n focuses $s psionic energy into $s strength.", ch, NULL, NULL, TO_ROOM);
        }

        af.location = APPLY_STR;
        affect_to_char(victim, &af);
    }
    else if (!str_prefix(arg, "intelligence"))
    {
        if (ch != victim)
        {
            act("$n focuses $s psionic energy into your intelligence.", ch, NULL, victim, TO_VICT);
            act("You focus your psionic energy into $N's intelligence.", ch, NULL, victim, TO_CHAR);
            act("$n focuses $s psionic energy into $N's intelligence.", ch, NULL, victim, TO_NOTVICT);
        }
        else
        {
            send_to_char("You focus your psionic energy into your intelligence.\r\n", ch);
            act("$n focuses $s psionic energy into $s intelligence.", ch, NULL, NULL, TO_ROOM);
        }

        af.location = APPLY_INT;
        affect_to_char(victim, &af);
    }
    else if (!str_prefix(arg, "wisdom"))
    {
        if (ch != victim)
        {
            act("$n focuses $s psionic energy into your wisdom.", ch, NULL, victim, TO_VICT);
            act("You focus your psionic energy into $N's wisdom.", ch, NULL, victim, TO_CHAR);
            act("$n focuses $s psionic energy into $N's wisdom.", ch, NULL, victim, TO_NOTVICT);
        }
        else
        {
            send_to_char("You focus your psionic energy into your wisdom.\r\n", ch);
            act("$n focuses $s psionic energy into $s wisdom.", ch, NULL, NULL, TO_ROOM);
        }

        af.location = APPLY_WIS;
        affect_to_char(victim, &af);
    }
    else if (!str_prefix(arg, "dexterity"))
    {
        if (ch != victim)
        {
            act("$n focuses $s psionic energy into your dexterity.", ch, NULL, victim, TO_VICT);
            act("You focus your psionic energy into $N's dexterity.", ch, NULL, victim, TO_CHAR);
            act("$n focuses $s psionic energy into $N's dexterity.", ch, NULL, victim, TO_NOTVICT);
        }
        else
        {
            send_to_char("You focus your psionic energy into your dexterity.\r\n", ch);
            act("$n focuses $s psionic energy into $s dexterity.", ch, NULL, NULL, TO_ROOM);
        }

        af.location = APPLY_DEX;
        affect_to_char(victim, &af);
    }
    else if (!str_prefix(arg, "constitution"))
    {
        if (ch != victim)
        {
            act("$n focuses $s psionic energy into your constitution.", ch, NULL, victim, TO_VICT);
            act("You focus your psionic energy into $N's constitution.", ch, NULL, victim, TO_CHAR);
            act("$n focuses $s psionic energy into $N's constitution.", ch, NULL, victim, TO_NOTVICT);
        }
        else
        {
            send_to_char("You focus your psionic energy into your constitution.\r\n", ch);
            act("$n focuses $s psionic energy into $s constitution.", ch, NULL, NULL, TO_ROOM);
        }

        af.location = APPLY_CON;
        affect_to_char(victim, &af);
    }
    else
    {
        send_to_char("You failed.\r\n", ch);
    }

    return;
}


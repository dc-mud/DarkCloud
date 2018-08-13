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
*  Wizard/Wizardess Class                                                  *
*                                                                          *
*  The Wizard/Wizardess class is an advanced mage who will receive a       *
*  heightened casting ability as well as these advanced wizardry spells.   *
*                                                                          *
*    - increase armor                                                      *
*    - knock                                                               *
*    - feeble mind                                                         *
*    - heaviness                                                           *
*    - extend portal                                                       *
*    - blink                                                               *
*    - mental presence                                                     *
*    - poison spray                                                        *
*    - false life                                                          *
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
 * Increase armor - If a player has an armor spell currently on them then
 * this will increase it's potency as well as well as extend it's duration.
 */
void spell_increase_armor(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA *paf;

    if (!is_affected(victim, gsn_armor))
    {
        send_to_char("Spell failed.\r\n", ch);
        return;
    }

    for (paf = victim->affected; paf != NULL; paf = paf->next)
    {
        if (paf->type == gsn_armor)
        {
            // Double both the original modifier and duration... but hard code
            // that so this can't be doubled, then doubled, etc.
            paf->modifier = -40;
            paf->duration = 48;
        }
    }

    send_to_char("You feel an enhancement of your protection.\r\n", victim);

    if (ch != victim)
    {
        act("$N's protection has been enhanced by your magic.", ch, NULL, victim, TO_CHAR);
    }

    return;
}

/*
 * Looks at all of the exits in the rooms, finds locked doors and then has a chance
 * to unlock each door.
 */
void spell_knock(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    int dir = -1;

    // Make the find_door call silent, if it's a bad door or direction it fails
    if (IS_NULLSTR(target_name)
        || (dir = find_door(ch, target_name, TRUE)) < 0)
    {
        send_to_char("Spell failed.\r\n", ch);
        return;
    }

    // Unlock the door
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;
    EXIT_DATA *pexit_rev;

    pexit = ch->in_room->exit[dir];

    // If the door isn't closed, it doesn't need a key or it's not locked
    // the the spell fails
    if (!IS_SET(pexit->exit_info, EX_CLOSED)
        || pexit->key < 0
        || !IS_SET(pexit->exit_info, EX_LOCKED))
    {
        send_to_char("Spell failed.\r\n", ch);
        return;
    }

    // The higher the level the area, the harder it is to unlock things.  A level 51-51
    // area would have a 24%, 1-51 would have about a 49% chance.
    int average_level = (ch->in_room->area->min_level + ch->in_room->area->max_level) / 2;
    int chance = 75 - average_level;

    if (!CHANCE(chance))
    {
        send_to_char("Spell failed.\r\n", ch);
        return;
    }

    REMOVE_BIT(pexit->exit_info, EX_LOCKED);
    act("$n's magic unlocks the $d.", ch, NULL, pexit->keyword, TO_ROOM);
    act("You hear a click as your magic magic unlocks the $d.", ch, NULL, pexit->keyword, TO_CHAR);

    // Unlock the other side as well
    if ((to_room = pexit->u1.to_room) != NULL
        && (pexit_rev = to_room->exit[rev_dir[dir]]) != NULL
        && pexit_rev->u1.to_room == ch->in_room)
    {
        REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);
    }
}

/*
 * Spell that extends the duration of a portal that might be on a timer.
 */
void spell_extend_portal(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    OBJ_DATA *obj;;

    // Make the find_door call silent, if it's a bad door or direction it fails
    if (IS_NULLSTR(target_name)
        || (obj = get_obj_here(ch, target_name)) == NULL)
    {
        send_to_char("Spell failed.\r\n", ch);
        return;
    }

    if (obj->item_type != ITEM_PORTAL)
    {
        send_to_char("Spell failed.\r\n", ch);
    }

    // Extend the timer so the portal won't disappear for an additional two ticks, only
    // extend a timer that is already set.  If you extend a timer that has 0 it was already
    // not set to disappear (like reset portals in clan halls).
    if (obj->timer > 0)
    {
        obj->timer += 2;
    }

    act("$p is filled with energy from $n's magic.", ch, obj, NULL, TO_ROOM);
    act("$p is filled with energy from your magic.", ch, obj, NULL, TO_CHAR);
}

/*
 * Maladiction type spell that will lower a victim's movement and dexterity as well
 * as costing more movement per step.  It won't stack with slow, but either slow or
 * this will double movement cost.
 */
void spell_heaviness(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, gsn_heaviness))
    {
        send_to_char("They are already weighted with a magical heaviness.\r\n", ch);
        return;
    }

    if (saves_spell(level, victim, DAM_OTHER))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    // Lower their dex and their total movement.
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.location = APPLY_DEX;
    af.modifier = -2;
    af.duration = 10;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    af.location = APPLY_MOVE;
    af.modifier = -50;
    affect_to_char(victim, &af);

    // Lower their current by 66%'ish.
    ch->move /= 3;

    send_to_char("You are weighted down by a magical heaviness!\r\n", victim);
    act("$N is weighted down by a magical heaviness!", victim, NULL, NULL, TO_ROOM);

    return;
}

/*
 * Maladiction type spell that lowers int/wis as well as the victim's casting
 * level.  It will have a separate check as will to dispel imbue if the victim
 * is imbued.
 */
void spell_feeble_mind(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    // Initial saves check
    if (saves_spell(level, victim, DAM_OTHER))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    // #1 Imbue dispel check
    check_dispel(level, victim, gsn_imbue);

    // Lower their int and wisdom
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.location = APPLY_INT;
    af.modifier = -2;
    af.duration = 10;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    af.location = APPLY_WIS;
    affect_to_char(victim, &af);

    send_to_char("You feel a fog cast over your mental clarity!\r\n", victim);
    act("A fog is cast over $N's mental clarity!", victim, NULL, NULL, TO_ROOM);

}

/*
 * The blink spell will allow the wizard to phase out of the existence and into
 * a void where they are temporarly safe but unable to perform basic actions.  When
 * the blink spell fades (after a tick or so) the player will fade back to where
 * they came from.  This can't be cast from battle.  (it can also be broken by
 * entering a command).
 */
void spell_blink(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    AFFECT_DATA af;

    // Set the players old room so we know where to put them back to
    ch->was_in_room = ch->in_room;

    // They should not be fighting here, but if the spell is ever allowed to
    // be cast outside of standing this will be important.
    if (ch->fighting != NULL)
    {
        stop_fighting(ch, TRUE);
    }

    act("$n phases out of the mortal plane.", ch, NULL, NULL, TO_ROOM);
    send_to_char("You phase out of the mortal plane.\r\n", ch);

    char_from_room(ch);
    char_to_room(ch, get_room_index(ROOM_VNUM_ETHEREAL_PLANE));

    act("$n slowly phases into the ethereal plane.", ch, NULL, NULL, TO_ROOM);

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.duration = 1;
    af.bitvector = 0;
    affect_to_char(ch, &af);

}

/*
 * Checks to see if the player is in the ethereal plane, if so, removes them.  In fact,
 * we don't even really care if they're affected by blink per se, if their in this room
 * we want them out when any command is entered (the affect wearing off will remove them
 * as well.  A true being returned means they were in the ethereal plane and the caller
 * from the interp should exit the first command sent.
 */
bool blink_check(CHAR_DATA * ch, bool remove_affect)
{
    // We can't do much for these folks, make sure they exist and that they're in the
    // ethereal plane room.
    if (ch == NULL
        || ch->was_in_room == NULL
        || ch->in_room != get_room_index(ROOM_VNUM_ETHEREAL_PLANE))
    {
        return FALSE;
    }

    act("$n phases away into the mortal plane of existence.", ch, NULL, NULL, TO_ROOM);

    // The was_in_room is what's used for the void, but as this is case and then
    // they're evicted in the next tick or two it should be fine to use.
    char_from_room(ch);
    char_to_room(ch, ch->was_in_room);
    ch->was_in_room = NULL;

    // We want to remove this affect if it's being called from interpret, but NOT
    // if it's being called from the affects processing handler (it will remove itself
    // then).  We don't want to null it in that case and then have it work on a NULL
    // pointer
    if (remove_affect)
    {
        affect_strip(ch, gsn_blink);
    }

    // Show the room that they have returned.
    send_to_char("You phase back into the mortal plane of existence.\r\n", ch);
    act("$n phases back into this mortal plane of existence.", ch, NULL, NULL, TO_ROOM);

    // Now, we're giving them the smallest of a pause.. we don't want this to be
    // a 100% get of jail free card.. so they'll get a 1 pulse lag before their next
    // command will be entered.
    WAIT_STATE(ch, 1 * PULSE_VIOLENCE);

    return TRUE;
}

/*
 * A spell that while on will allow the caster a chance at a reduced stun when
 * that occurs.  It will not do away with stun, but it will make it less effective
 * which is huge for mage types.
 */
void spell_mental_presence(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (ch != victim)
    {
        send_to_char("Spell failed.\r\n", ch);
        return;
    }

    affect_strip(ch, gsn_mental_presence);

    act("$n is surrounded by an aura of clarity.", ch, NULL, NULL, TO_ROOM);
    send_to_char("You are surrounded by an aura of clarity.\r\n", ch);

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.duration = level / 6;
    af.bitvector = 0;
    affect_to_char(ch, &af);
}

/*
 * Casts a poison spray which hits all eligble targets in a room.  This will cause mild
 * damage and also has standard chance to poison any target.
 */
void spell_poison_spray(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    AFFECT_DATA af;
    int dam;

    act("Poison sprays from $n's finger tips!", ch, NULL, NULL, TO_ROOM);
    act("Poison sprays from your finger tips!", ch, NULL, NULL, TO_CHAR);

    // Base damage
    dam = level / 2;

    // Addition for spellcraft bonus
    if (CHANCE(get_skill(ch, gsn_spellcraft)))
    {
        dam += number_range(15, 25);
    }

    // Loop through the room looking for additional victims
    for (vch = victim->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        // Is the victim safe?
        if (is_safe_spell(ch, vch, TRUE)
            || (IS_NPC(ch) && IS_NPC(vch) && (ch->fighting != vch || vch->fighting != ch)))
            continue;

        // Doesn't hit anyone in your group
        if (is_same_group(ch, vch))
            continue;

        // Doesn't hit yourself
        if (vch == ch)
            continue;

        // Damage saves check against poison
        if (!saves_spell(level, vch, DAM_POISON))
        {
            // They didn't make the saves check.. add some damage
            dam += level / 2;
        }

        // Always some damage
        damage(ch, vch, dam, sn, DAM_POISON, TRUE);

        // They're already poisoned, no sense in adding it again
        if (is_affected(vch, gsn_poison))
        {
            continue;
        }

        // The saves check to see if they're poisoned, we're going to use the
        // poison gsn itself.
        if (!saves_spell(level, vch, DAM_POISON))
        {
            af.where = TO_AFFECTS;
            af.type = gsn_poison;
            af.level = level;
            af.duration = level / 5;
            af.location = APPLY_STR;
            af.modifier = -2;
            af.bitvector = AFF_POISON;
            affect_to_char(vch, &af);

            send_to_char("You feel very sick.\r\n", vch);
            act("$n looks very ill.", vch, NULL, NULL, TO_ROOM);
        }

    }
}

/*
 * False life will obfuscate the health condition of the wizard to others.  In other
 * other words, they won't be able to see that they're at big and nasty, pretty hurt,
 * awful, etc.  The other players will just see a message saying they can't tell their
 * condition, etc.
 */
void spell_false_life(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    // Can only be cast on self.
    if (ch != victim)
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    affect_strip(ch, gsn_false_life);

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level / 10;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("You have obscured the condition of your health to others.\r\n", victim);

    return;
}


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
 *  Healer Class (not to be confused with a healer mob who you can buy     *
 *  healing spells from)                                                   *
 *                                                                         *
 *  Healers are masters of healing people's bodies as well as having       *
 *  curative expertise such as removing disease, poison, etc.  They are    *
 *  more of a support class that are effective group individuals.          *
 ***************************************************************************/

/***************************************************************************
 *  TODO - Ranged heal, more curatives (remember to update sense affliction)
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
 * Will heal an individual to full health at great cost to the healer who will lose most of
 * their health.  In order to use this spell the healer must have most of their health
 * themselves.  This is the hail mary of heals.
 */
void spell_sacrificial_heal(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    victim = (CHAR_DATA *)vo;

    // Cannot cast the spell on themselves, credit their mana back.
    if (ch == victim)
    {
        ch->mana += 250;
        send_to_char("You cannot cast this spell on yourself.\r\n", ch);
        return;
    }

    // They can't use this if they're in in the yellow on damage, credit their mana
    // back to them.  Consider getting the mana cost out of the table in case it changes
    // down the line.
    if (ch->hit < ch->max_hit * 3 / 4)
    {
        ch->mana += 250;
        send_to_char("You are too weak currently to cast this spell\r\n", ch);
        return;
    }

    // Set the target to full health, set the healer to near death.
    victim->hit += ch->hit;
    ch->hit = 1;

    act("$n raises $e hands to the sky and surrounds $N with a healing aura.", ch, NULL, victim, TO_NOTVICT);
    act("$n raises $e hands to the sky and surrounds you with a healing aura.", ch, NULL, victim, TO_VICT);
    act("You raise your hands to the sky and surround $N with a healing aura.", ch, NULL, victim, TO_CHAR);

} // end spell_sacrificial_heal

/*
 * Mass refresh will refresh every visible character in the room for only
 * 3 more mana than a normal refresh.  It also has an additional 1 to 10
 * movement random bonus on top of the normal refresh.
 */
void spell_mass_refresh(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *gch;
    char buf[MAX_STRING_LENGTH];

    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
    {
        // If the character can't be seen they can't be refreshed, we don't
        // want this used to sniff out hidden characters.
        if (!can_see(ch, gch))
        {
            continue;
        }

        gch->move = UMIN(gch->move + level + number_range(1, 10), gch->max_move);

        if (gch->max_move == gch->move)
        {
            send_to_char("You feel fully refreshed!\r\n", gch);
        }
        else
        {
            send_to_char("You feel less tired.\r\n", gch);
        }

        if (gch != ch)
        {
            sprintf(buf, "%s has been refreshed.\r\n", gch->name);
            send_to_char(buf, ch);
        }

    }

} // end spell_mass_refresh

/*
 * A healing spell that will return small bits of health over the period of a few ticks
 * outside of the normal tick cycle.  This will cost half as much as a heal (because it
 * can only be cast once and is spread out.  Players will get 10hp every half tick if
 * they are below their max health.
 */
void spell_vitalizing_presence(int sn, int level, CHAR_DATA *ch, void *vo, int target)
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
            act("$N is already affected by the vitalizing presence.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;

    // Lasts longer on yourself
    if (ch == victim)
    {
        af.duration = 10;
    }
    else
    {
        af.duration = 5;
    }

    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    act("$N is vitalized with a healing presence.", victim, NULL, victim, TO_ROOM);
    send_to_char("You are vitalized with a healing presence.\r\n", victim);

    return;

} // end spell_vitalizing_presence

/*
 * Allows a healer to boost the life force of a recipient (e.g. increase their
 * max health points and movement for a level's worth of ticks).  The modifier
 * (how much hp and move they receive) will be calculated by the healer's casting
 * level.  This cannot be cast on NPC's as a way to make them stronger.
 */
void spell_life_boost(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;
    int modifier = 0;

    // Not on NPC's
    if (IS_NPC(victim))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    if (is_affected(victim, sn))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N is already affected by the increased vitality.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    // Base is the players level
    modifier = ch->level;

    // If the player is casting it on themselves we'll give them a 12hp-13hp bonus at 51.  Healer's
    // aren't going to be huge player killers so why not.
    if (ch == victim)
    {
        modifier += ch->level / 4;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.modifier = modifier;
    af.location = APPLY_HIT;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    af.location = APPLY_MOVE;
    affect_to_char(victim, &af);

    act("$N has been vitalized.", victim, NULL, victim, TO_ROOM);
    send_to_char("You feel an increased vitality.\r\n", victim);

    return;

} // end spell_life_boost

/*
 * A spell to help the healer resist some offensive magics against it.  Healer's don't
 * have many offensive weapons and thus are vulnerable characters, this should help
 * at least protect them a little more from spells.  This should be set to target
 * char_self so it can only be cast on the healer themselves.  We don't want to create
 * super chars with saves who make casters worthless.
 */
void spell_magic_resistance(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
        affect_strip(victim, sn);
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.modifier = (ch->level / 10) * -1;
    af.location = APPLY_SAVES;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    act("$N has an enhanced resistance to magic.", victim, NULL, victim, TO_ROOM);
    send_to_char("You feel an enhanced resistance to magic.\r\n", victim);

    return;

} // end magic resistance

/*
 * Mana transfer will allow the healer to transfer mana from themselves to a
 * recipient.  We'll start at 50 for casting and do a 1 to 1 transfer for now.
 */
void spell_mana_transfer(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if (IS_NPC(victim))
    {
        send_to_char("You cannot cast mana transfer on them.\r\n", ch);
        return;
    }

    if (victim->mana == victim->max_mana)
    {
        send_to_char("Their mana is at full capacity.\r\n", ch);
        return;
    }

    victim->mana = UMIN(victim->mana + 50, victim->max_mana);

    send_to_char("You feel a surge of mana course through your body.\r\n", victim);

    // Show the caster it's done
    if (ch != victim)
    {
        if (victim->mana == victim->max_mana)
        {
            // The caster is now at max with that cast, let the caster know.
            send_to_char("Their mana is now at full capacity.\r\n", ch);
        }
        else
        {
            send_to_char("Ok.\r\n", ch);
        }
    }

    return;
} // end spell_mana_transfer

/*
 * Allows a healer to remove a weaken spell specifically.  Healers get a +3 level
 * bonus on their curative spells.
 */
void spell_cure_weaken(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if (!is_affected(victim, gsn_weaken))
    {
        if (victim == ch)
        {
            send_to_char("You aren't afflicted by a magical weakeness.\r\n", ch);
        }
        else
        {
            act("$N doesn't appear to be afflicted by a magical weakness.", ch, NULL, victim, TO_CHAR);
        }
        return;
    }

    // Healer's get a casting bonus when removing certain spells.
    if (check_dispel(level + 3, victim, gsn_weaken))
    {
        send_to_char("Your no longer feel weak!\r\n", victim);
        act("$n is no longer weakened.", victim, NULL, NULL, TO_ROOM);
    }
    else
    {
        send_to_char("Spell failed.\r\n", ch);
    }

} // end spell_cure_weaken

/*
 * Spell to cure people affected by slow (haste does this also, but cleric and cleric
 * reclasses don't get haste (usually).  Healers get a +3 level bonus on their curative
 * spells.
 */
void spell_cure_slow(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if (!is_affected(victim, gsn_slow))
    {
        if (victim == ch)
        {
            send_to_char("You aren't afflicted by slow.\r\n", ch);
        }
        else
        {
            act("$N doesn't appear to be afflicted by slow.", ch, NULL, victim, TO_CHAR);
        }
        return;
    }

    if (check_dispel(level + 3, victim, gsn_slow))
    {
        send_to_char("Your no longer feel like you're moving slowly!\r\n", victim);
        act("$n is no longer moving slowly.", victim, NULL, NULL, TO_ROOM);
    }
    else
    {
        send_to_char("Spell failed.\r\n", ch);
    }

} // end spell_cure_slow

/*
 * Restore mental presence will allow the caster to remove any stun affect on a
 * character (from bash, etc.).  It will not be much use on themselves since they
 * have to be ablet to cast it and stun may stop that.  We will also have this spell
 * alleviate certain other affects like disorientation.
 */
void spell_restore_mental_presence(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if (IS_NPC(victim))
    {
        send_to_char("You cannot cast this spell on them.\r\n", ch);
        return;
    }

    // This is all the code needed, it will remove the stun state from the player.
    victim->daze = 0;

    // Disorientation isn't a spell, we aren't going to make a saves check but we will
    // add some fate to it.
    if (CHANCE(33))
    {
        if (is_affected(victim, gsn_disorientation))
        {
            affect_strip(victim, gsn_disorientation);
        }
    }

    send_to_char("Your mental presence has been restored.\r\n", victim);
    act("$n's mental presence has been restored.", victim, NULL, NULL, TO_ROOM);

} // end spell_restore_mental_presence

/*
 * Sense affliction will allow the healer to see an (Affliction) flag on a player when
 * they do a 'look' in the room if a player is afflicted by something the healer can
 * cure.  If the healer looks at the person specifically they will see everything they
 * are afflicted with that they can cure specifically.  The healer can cast this on
 * themselves but not others.  This has a long duration and is not dispelable.  I
 * suppose this could also have just been a skill.
 */
void spell_sense_affliction(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
        // Remove the affect so it can be re-added to yourself
        affect_strip(victim, sn);
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = ch->level + (ch->level / 2);
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("Your senses for those afflicted are heightened.\r\n", victim);

    if (ch != victim)
    {
        act("$N's senses for those afflicted are heightened.", victim, NULL, victim, TO_ROOM);
    }

    return;

} // end spell_sense_affliction

/*
 * A healer spell that will allow the healer to create a bind stone that will stay in the
 * room with some of their healing power imbued into it.  The bind stone will be an object
 * that can't be moved or sacrificed.  Only one can exist in an area.  It will have a certain
 * amount of charges that it can be used.  When a player touches it, they will be healed.
 * when the bind stone's magic is used up it will disappear into the ether.  The item won't
 * disappear with time however.. it must be used.  For now, this will not survive copyover's
 * or reboots.
 */
void spell_healers_bind(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj;
    OBJ_DATA *objSearch;
    int charges = 0;

    // First, let's see if there are any other bind stones in the area, if so, we
    // are not going to cast this.  These will not be in containers.
    for (objSearch = object_list; objSearch != NULL; objSearch = objSearch->next)
    {
        // Can't be in a null room
        if (objSearch->in_room != NULL)
        {
            // Only one per area at a time.
            if (objSearch->pIndexData->vnum == OBJ_VNUM_HEALERS_BIND
                && objSearch->in_room->area->vnum == ch->in_room->area->vnum)
            {
                send_to_char("A healer's bind stone already exists somewhere in this area...\r\n", ch);
                return;
            }
        }
    }

    // Base is level / 5
    charges = ch->level / 5;

    // +1 for bless
    if (is_affected(ch, gsn_bless))
    {
        charges += 1;
    }

    // +1 for enhanced recovery, the healer can then pass that along.
    if (is_affected(ch, gsn_enhanced_recovery))
    {
        charges += 1;
    }

    // Cut in half if these are the case..
    if (is_affected(ch, gsn_curse) || is_affected(ch, gsn_weaken))
    {
        charges = charges / 2;
    }

    obj = create_object(get_obj_index(OBJ_VNUM_HEALERS_BIND));
    obj->value[0] = charges;  // The number of charges
    obj->value[1] = 50;       // The amount it heals

    obj_to_room(obj, ch->in_room);
    act("$p slowly fades into existence.", ch, obj, NULL, TO_ROOM);
    act("$p slowly fades into existence.", ch, obj, NULL, TO_CHAR);

} // end spell_create_healers_bind

/*
 * The nourishment spell will allow the healer to nourish a person and alleviate any
 * hunger and thirst they might have without food and water.  I originally wrote this
 * spell (on 1/4/2000) for another class but it seems to fit here better.
 */
void spell_nourishment(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if (IS_NPC(victim))
    {
        send_to_char("You cannot provide them with nourishment.\r\n", ch);
        return;
    }

    // Only players as this hits the pcdata
    victim->pcdata->condition[COND_THIRST] = 48;
    victim->pcdata->condition[COND_FULL] = 48;
    victim->pcdata->condition[COND_HUNGER] = 48;

    act("$N has been filled with nourishment.", victim, NULL, victim, TO_ROOM);
    send_to_char("You feel nourishment filling your body.\r\n", victim);

} // end nourishment

/*
 * Enhanced recovery will allow the recipient to heal more on every tick.  Originally this
 * spell was written for another class (on 5/26/2000) but it fits better under a healer.
 */
void spell_enhanced_recovery(int sn, int level, CHAR_DATA *ch, void *vo, int target)
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
            act("$N is already blessed with enhanced recovery.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = (ch->level / 2);
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    act("$N has been blessed with an enhanced recovery.", victim, NULL, victim, TO_ROOM);
    send_to_char("You feel blessed with a enhanced recovery.\r\n", victim);

} // end spell_enhanced_recovery

/*
 * Allows a healer to remove a deafen affect.  Healers get a +3 level
 * bonus on their curative spells.  This should remove ALL skills/spells
 * that add a deafen affect.
 */
void spell_cure_deafness(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if (!is_affected(victim, gsn_song_of_dissonance))
    {
        if (victim == ch)
        {
            send_to_char("You aren't currently deaf.\r\n", ch);
        }
        else
        {
            act("$N doesn't appear to be currently deaf.", ch, NULL, victim, TO_CHAR);
        }

        return;
    }

    // Healer's get a casting bonus when removing certain spells.
    if (check_dispel(level + 3, victim, gsn_song_of_dissonance))
    {
        act("$n is no longer deaf.", victim, NULL, NULL, TO_ROOM);
    }
    else
    {
        send_to_char("Spell failed.\r\n", ch);
    }

} // end spell_cure_deafness

/*
 * Spell to remove faerie fire outline from an individual
 */
void spell_remove_faerie_fire(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if (!IS_AFFECTED(victim, AFF_FAERIE_FIRE))
    {
        if (victim == ch)
        {
            send_to_char("You aren't afflicted by faerie fire.\r\n", ch);
        }
        else
        {
            act("$N doesn't appear to be afflicted by faerie fire.", ch, NULL, victim, TO_CHAR);
        }
        return;
    }

    if (check_dispel(level + 3, victim, gsn_faerie_fire))
    {
        act("The pink aura around $n fades away.", victim, NULL, NULL, TO_ROOM);
    }
    else
    {
        send_to_char("Spell failed.\r\n", ch);
    }

} // end spell_remove_faerie_fire

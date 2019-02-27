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
 *  Priest Class                                                            *
 *                                                                          *
 *  Priest's are specialized clerics who attain their powers from their     *
 *  respective deity.  All players who are a priest must follow a god or    *
 *  goddess.  A priest or priestess must have prayed recently via the       *
 *  prayer command in order to use their advanced spells that go above      *
 *  and beyond those of a normal cleric.                                    *
 *                                                                          *
 ***************************************************************************/

// TODO - Desperate Prayer, crucify

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
#include "tables.h"

extern char *target_name;

/*
 * Priest rank table (integer rank, the name and the hours needed to achieve
 * the rank.  The higher the priest's rank the more powerful their spells and
 * skills will be.
 */
const struct priest_rank_type priest_rank_table[] = {
    { PRIEST_RANK_NOVITIATE,   "Novitiate",      0},
    { PRIEST_RANK_DEACON,      "Deacon",       100},
    { PRIEST_RANK_PRIEST,      "Priest",       250},
    { PRIEST_RANK_BISHOP,      "Bishop",       500},
    { PRIEST_RANK_ARCHBISHOP,  "Archbishop",   750},
    { PRIEST_RANK_CARDINAL,    "Cardinal",    1250},
    { PRIEST_RANK_HIGH_PRIEST, "High Priest", 2000}
};

/*
 * Calculates the players priest rank.  This will only work for priests and
 * will return otherwise.  This will be called on tick from the update handler.
 */
void calculate_priest_rank(CHAR_DATA *ch)
{
    // We don't need to calculate if they're not a priest or a high priest.
    if (ch == NULL || IS_NPC(ch)
        || ch->class != PRIEST_CLASS
        || ch->pcdata->priest_rank == PRIEST_RANK_HIGH_PRIEST)
    {
        return;
    }

    int x;
    int hours = hours_played(ch);
    int rank = 0;

    // Find the highest rank they qualify for
    for (x = 0; x <= PRIEST_RANK_HIGH_PRIEST; x++)
    {
        if (hours >= priest_rank_table[x].hours)
        {
            rank = x;
        }
    }

    // They have attained a new rank, rank them, then congratulate them.
    if (ch->pcdata->priest_rank != rank)
    {
        ch->pcdata->priest_rank = rank;

        sendf(ch, "You have attained the priest rank of %s.\r\n", priest_rank_table[rank].name);
        log_f("%s has attained the priest rank %s.", ch->name, priest_rank_table[rank].name);

        save_char_obj(ch);
    }

}

/*
 * A priest must pray to their god who will bestow upon them the ability to
 * cast the specialized spells they have been gifted.  E.g. This affect needs
 * to be on the player for them to cast their priest spells.  The priest will
 * have to time out when their prayer affect runs out while in PK.. since they
 * can't prayer back up until the pk_timer zeros out.
 */
void do_prayer(CHAR_DATA * ch, char *argument)
{
    if (ch == NULL)
    {
        return;
    }

    if (IS_FIGHTING(ch))
    {
        send_to_char("You cannot concentrate enough.\r\n", ch);
        return;
    }

    if (ch->class != PRIEST_CLASS || IS_NPC(ch))
    {
        send_to_char("You bow your head and pray.\r\n", ch);
        act("$n bows their head.", ch, NULL, NULL, TO_ROOM);
        return;
    }

    if (is_affected(ch, gsn_prayer))
    {
        send_to_char("You are still affected by the wisdom of your god.\r\n", ch);
        return;
    }

    if (ch->pcdata->pk_timer > 0)
    {
        send_to_char("You cannot concentrate on prayer this closely after battle.\r\n", ch);
        return;
    }

    switch (ch->substate)
    {
        default:
            add_timer(ch, TIMER_DO_FUN, 8, do_prayer, 1, NULL);
            send_to_char("You kneel and pray for the blessing of wisdom.\r\n", ch);
            act("$n kneels and prays for the blessing of wisdom.", ch, NULL, NULL, TO_ROOM);
            return;
        case 1:
            // Continue onward with the prayer.
            break;
        case SUB_TIMER_DO_ABORT:
            ch->substate = SUB_NONE;
            send_to_char("Your concentration was broken before your prayers were finished.\r\n", ch);
            return;
    }

    ch->substate = SUB_NONE;

    send_to_char("A feeling of divinity overtakes your presence.\r\n", ch);
    act("A feeling of divinity overtakes the room.", ch, NULL, NULL, TO_ROOM);

    // Add the prayer affect
    AFFECT_DATA af;

    af.where = TO_AFFECTS;
    af.type = gsn_prayer;
    af.level = ch->level;
    af.duration = (ch->level / 3) + ch->pcdata->priest_rank + 1;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    return;
}

/*
 * A priest must have recently praied to their god in order to cast the spells blessed on them
 * by their deity.
 */
bool prayer_check(CHAR_DATA *ch)
{
    if (is_affected(ch, gsn_prayer))
    {
        return TRUE;
    }
    else
    {
        send_to_char("Your gods wisdom must be present to produce such a magic.\r\n", ch);
        return FALSE;
    }
}

/*
 * Agony spell will allow the priest to maladict the victim in a way that will cause
 * them damage when they recall, word of recall, teleport or gate.  On NPC's this will
 * have a different affect and will lower int/wis/con by 1.
 */
void spell_agony(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (!prayer_check(ch))
    {
        return;
    }

    if (is_affected(victim, sn))
    {
        send_to_char("They are already affected by your deity's agony.\r\n", ch);
        return;
    }

    if (saves_spell(level, victim, DAM_OTHER))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 10;
    af.bitvector = 0;
    af.caster = ch;

    if (!IS_NPC(victim))
    {
        af.location = APPLY_NONE;
        af.modifier = ((ch->pcdata->priest_rank + 1) * 10);
        affect_to_char(victim, &af);
    }
    else
    {
        af.modifier = -1;
        af.location = APPLY_WIS;
        affect_to_char(victim, &af);
        af.location = APPLY_INT;
        affect_to_char(victim, &af);
        af.location = APPLY_CON;
        affect_to_char(victim, &af);
    }

    send_to_char("You feel an agony overshadow your soul.\r\n", victim);
    act("$n has an agony overshadow their soul.", victim, NULL, NULL, TO_ROOM);

    return;
}

/*
 * This procedure will check to see if a character takes agony damage
 */
void agony_damage_check(CHAR_DATA *ch)
{
    if (ch != NULL && !IS_NPC(ch) && is_affected(ch, gsn_agony))
    {
        AFFECT_DATA *af;
        int agony_damage = 0;

        // If they are affected by agony get the affect so that we can
        // get the modifier to know how much damage to process.  If the
        // afffect for some reason is null, ditch out.
        if ((af = affect_find(ch->affected, gsn_agony)) == NULL)
        {
            return;
        }

        // If this ends up on a ghost somehow, strip it and return, they shouldn't
        // receive damage as a ghost.
        if (IS_GHOST(ch))
        {
            affect_strip(ch, gsn_agony);
            return;
        }

        // The caster must still be valid for this damage to take place.
        if (af->caster != NULL)
        {
            agony_damage = af->modifier;
            damage(af->caster, ch, agony_damage, gsn_agony, DAM_HOLY, TRUE);

            // Check if this damage is occuring with the caster and victim in a different
            // room, if it is, stop fighting, they can't physically fight if they are in
            // different places.
            if (ch->in_room != af->caster->in_room)
            {
                stop_fighting(af->caster, FALSE);
            }

            return;
        }
        else
        {
            // The pointer to the caster is gone, strip the agony affect.
            affect_strip(ch, gsn_agony);
            return;
        }
    }
}

void spell_holy_presence(int sn, int level, CHAR_DATA * ch, void *vo, int target)
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
            act("$N is already protected by a holy presence.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = (level / 2) + (ch->pcdata->priest_rank * 2);
    af.location = APPLY_AC;
    af.modifier = -20 - (ch->pcdata->priest_rank * 2);
    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("You feel a holy presence protecting you.\r\n", victim);
    act("$N is protected by a holy presence.", victim, NULL, NULL, TO_ROOM);
    return;
}

/*
 * Allows a clanner if successful to force a victim to recall.
 */
void spell_displacement(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    ROOM_INDEX_DATA *location;
    int recall_vnum = 0;

    if (!prayer_check(ch))
    {
        return;
    }

    if (IS_NPC(victim))
    {
        send_to_char("Spell failed.\r\n", ch);
        return;
    }

    // They are a player and not a mob, we can safely access pcdata
    if (victim->pcdata->recall_vnum > 0)
    {
        recall_vnum = victim->pcdata->recall_vnum;
    }
    else
    {
        recall_vnum = ROOM_VNUM_TEMPLE;
    }

    // Make sure the recall vnum works
    if ((location = get_room_index(recall_vnum)) == NULL)
    {
        send_to_char("They are a lost soul.\r\n", ch);
        bugf("spell_displacement: vnum not found %d for victim %s", recall_vnum, victim->name);
        return;
    }

    if (IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
        || IS_AFFECTED(victim, AFF_CURSE)
        || IS_SET(victim->in_room->area->area_flags, AREA_NO_RECALL)
        || IS_SET(victim->in_room->room_flags, ROOM_ARENA)
        || victim->fighting != NULL)
    {
        send_to_char("You feel your body phase out, and then back to this plane.\r\n", victim);
        send_to_char("Spell failed.\r\n", ch);
        return;
    }

    // Casting level boost based on the priest's rank (rank / 2)
    if (!IS_NPC(ch))
    {
        level += ch->pcdata->priest_rank;
    }

    // Saves check
    if (saves_spell(level, victim, DAM_OTHER))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    if (ch == victim)
    {
        send_to_char("\r\nYou phase out of existence and disappear.\r\n\r\n", ch);
    }

    // Cut the users move, this is not affected by enhanced recall
    victim->move /= 2;

    // Show the room the user has been displaced from.
    act("$n phases out of existence and disappears.", victim, NULL, NULL, TO_ROOM);
    char_from_room(victim);

    // Show the roomthe user appears in
    char_to_room(victim,location);
    act("$n phases into existence and appears in the room.",victim,NULL,NULL,TO_ROOM);

    // Auto look so the user realizes they've been displaced to elsewhere.
    do_look(victim,"auto");

    // Priest, agony spell check/processing
    agony_damage_check(ch);

    // There is an additional lag when this succeeds as if it lands often in hard to reach
    // places could sway a fight.
    WAIT_STATE(ch, skill_table[gsn_displacement]->beats / 2);

}

/*
 * Holy flame is a priest attack spell that also surrounds the victim with faerie fire.
 * This spell originated from fireball (damage slightly boosted).
 */
void spell_holy_flame(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    // Must be prayed to cast
    if(!prayer_check(ch))
    {
        return;
    }

    // Damage rises up to 51, this table came from fireball with the upper
    // end receiving higher values.
    static const int dam_each[] =
    {
      3,
      3,    3,   4,   4,   5,      6,   7,   8,   9,  10,
      10,  15,  20,  25,  30,     35,  40,  45,  50,  55,
      60,  65,  70,  75,  80,     82,  84,  86,  88,  90,
      92,  94,  96,  98, 100,    102, 104, 106, 108, 110,
     112, 114, 116, 120, 124,    128, 132, 138, 145, 150
    };

    int dam;

    level = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);
    dam  = number_range( dam_each[level] / 2, dam_each[level] * 2 );

    // Saves check for reduced damage (by 33%)
    if (saves_spell(level, victim, DAM_FIRE))
    {
        dam = (dam * 2) / 3;
    }

    // Look for faerie fire, if it's not there in any form then add it
    if (!IS_AFFECTED(victim, AFF_FAERIE_FIRE)
        && !is_affected(victim, gsn_faerie_fire)
        && !is_affected(victim, gsn_holy_flame))
    {
        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = level;
        af.duration = level / 5; // (10 ticks at 51)
        af.location = APPLY_AC;
        af.modifier = number_range(1, 3) * level; // Chance of higher or lower modifier than faerie fire
        af.bitvector = AFF_FAERIE_FIRE;
        affect_to_char(victim, &af);

        send_to_char("Holy flames errupt all about you!\r\n", victim);
        act("$n is surrounded by holy flames.", victim, NULL, NULL, TO_ROOM);

        // If the victim is not in the same room (e.g. the holy flame was ranged, then
        // show the caster the message since they would not have seen the TO_ROOM message).
        if (ch->in_room != victim->in_room)
        {
            act("$N is surrounded by holy flames.", ch, NULL, victim, TO_CHAR);
        }
    }

    // Finally, do the damage
    damage(ch, victim, dam, sn, DAM_FIRE, TRUE);

    return;
}

/*
 * A spell which bestows a divine wisdom onto the recipient.  E.g. ups their wisdom.
 */
void spell_divine_wisdom(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    // Must be prayed to cast
    if(!prayer_check(ch))
    {
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
            act("$N is already instilled with a divine wisdom.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = (level / 2) + (ch->pcdata->priest_rank * 2);
    af.location = APPLY_WIS;
    af.modifier = 2;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("You feel a divine wisdom instilled into your soul.\r\n", victim);
    act("$N is instilled with a divine wisdom.", victim, NULL, victim, TO_ROOM);
    return;
}

/*
 * A spell which allows the priest to know another players religion
 */
void spell_know_religion(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    // Must be prayed to cast
    if(!prayer_check(ch))
    {
        return;
    }

    // NPC's don't have religion (although that would be interesting in the future...)
    if (IS_NPC(victim))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    // Deity check
    if (victim->pcdata->deity == 0)
    {
        sendf(ch, "%s does not follow a god or goddess.\r\n", PERS(victim, ch));
    }
    else
    {
        sendf(ch, "%s is a follower of %s, %s\r\n"
                                , PERS(victim, ch)
                                , deity_table[victim->pcdata->deity].name
                                , deity_table[victim->pcdata->deity].description);
    }

    return;
}

/*
 * Guardian angel is a spell that will allow a priest to summon a single charmie that
 * will instantly initiate a rescue on them.
 */
void spell_guardian_angel(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    MOB_INDEX_DATA *pMobIndex;
    int i;
    CHAR_DATA *mob;
    AFFECT_DATA af;
    char buf[MAX_STRING_LENGTH];

    // Must be prayed to cast
    if(!prayer_check(ch))
    {
        return;
    }

    // Check if they already lead a guardian
    if (leads_grouped_mob(ch, VNUM_GUARDIAN_ANGEL))
    {
        send_to_char("You may only have one guardian angel at a time.\r\n", ch);
        return;
    }

    // Check to make sure that the guardian angel vnum actually exists, if not,
    // report it.
    if ((pMobIndex = get_mob_index(VNUM_GUARDIAN_ANGEL)) == NULL)
    {
        send_to_char("You failed.\r\n", ch);
        bugf("spell_guardian_angel: no vnum %d for mob", VNUM_GUARDIAN_ANGEL);
        return;
    }

    // Create the guardian
    mob = create_mobile(pMobIndex);

    // Hit points, level, saves and the hitroll/damroll.
    mob->max_hit = UMIN(ch->max_hit, 1000); // Lowest of 1000 or the players max hit
    mob->hit = mob->max_hit;
    mob->level = 9 * (level / 10);    // 10% below the casters level
    mob->saving_throw = -1 * (level / 5);
    mob->hitroll = level / 2;
    mob->damroll = level / 3;

    // Armor class
    for (i = 0; i < 4; i++)
    {
        mob->armor[i] = level * -3;
    }

    // Dice rolls for the mob
    mob->damage[DICE_NUMBER] = level / 10;
    mob->damage[DICE_TYPE] = level / 5;

    // Set the stats of the guardian to those of the player
    for (i = 0; i < MAX_STATS; i++)
    {
        mob->perm_stat[i] = ch->perm_stat[i];
    }

    // Tailor the short and long descriptions
    sprintf(buf, "A guardian angel of %s", deity_table[ch->pcdata->deity].name);
    free_string(mob->short_descr);
    mob->short_descr = str_dup(buf);

    sprintf(buf, "A guardian angel of %s levitates above the ground.\r\n", deity_table[ch->pcdata->deity].name);
    free_string(mob->long_descr);
    mob->long_descr = str_dup(buf);

    // Send the guardian to the room
    char_to_room(mob, ch->in_room);

    // Show the message to the room
    act("$n has summoned $N!", ch, NULL, mob, TO_ROOM);
    act("$N appears in the room and is bound to your will.", ch, NULL, mob, TO_CHAR);

    // Add the follow to the players group with the player aqs the leader
    add_follower(mob, ch);
    mob->leader = ch;

    // Set the affect on the guardian with the charm bit
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.modifier = 0;
    af.location = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char(mob, &af);

    // Remove the guardian affect from the player if it exists, it will be re-added
    if (is_affected(ch, sn))
    {
        affect_strip(ch, sn);
    }

    // Add the guardian bit onto the player, but remove the charm first
    af.bitvector = 0;
    affect_to_char(ch, &af);

    // If the player is currently fighting and the guardian angel has been called
    // during combat then have it try to rescue right off the bat.
    if (IS_FIGHTING(ch))
    {
        do_function(mob, &do_rescue, ch->name);
    }

    return;
}

/*
 * A divination spell that allows a priest to walk on water for a tick.  This can't
 * and shouldn't be cast on everyone, we don't want the whole mud negating water
 * restrictions like the oceans.  The lag in the ocean should still take affect, just
 * not the movement cost.
 */
void spell_water_walk(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    // Must be prayed to cast
    if(!prayer_check(ch))
    {
        return;
    }

    // This will only work on themselves.
    if (victim != ch)
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
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 1;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("You are blessed with the ability to walk on water.\r\n", victim);
    act("$N has been blessed with the ability to walk on water.", victim, NULL, victim, TO_ROOM);
    return;
}


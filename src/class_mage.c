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
*  Mage Class                                                              *
*                                                                          *
*  Mage is a base class for which many reclasses may share spells with.    *
*  Spells that are particular to mage class line will be housed here.      *
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
 * Combat Spell - Acid Blast
 */
void spell_acid_blast(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int dam;

    dam = dice(level, 12);
    if (saves_spell(level, victim, DAM_ACID))
    {
        dam /= 2;
    }

    damage(ch, victim, dam, sn, DAM_ACID, TRUE);
    return;
}

/*
 * Combat Spell - Burning hands
 */
void spell_burning_hands(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    static const int dam_each[] = {
        0,
        0, 0, 0, 0, 14, 17, 20, 23, 26, 29,
        29, 29, 30, 30, 31, 31, 32, 32, 33, 33,
        34, 34, 35, 35, 36, 36, 37, 37, 38, 38,
        39, 39, 40, 40, 41, 41, 42, 42, 43, 43,
        44, 44, 45, 45, 46, 46, 47, 47, 48, 48
    };
    int dam;

    level = UMIN(level, sizeof(dam_each) / sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);

    dam = number_range(dam_each[level] / 2, dam_each[level] * 2);

    if (saves_spell(level, victim, DAM_FIRE))
    {
        dam /= 2;
    }

    damage(ch, victim, dam, sn, DAM_FIRE, TRUE);
    return;
}

/*
 * Combat Spell - Chain Lightning
 */
void spell_chain_lightning(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    CHAR_DATA *tmp_vict, *last_vict, *next_vict;
    bool found;
    int dam;

    /* first strike */

    act("A lightning bolt leaps from $n's hand and arcs to $N.", ch, NULL, victim, TO_ROOM);
    act("A lightning bolt leaps from your hand and arcs to $N.", ch, NULL, victim, TO_CHAR);
    act("A lightning bolt leaps from $n's hand and hits you!", ch, NULL, victim, TO_VICT);

    dam = dice(level, 6);

    if (saves_spell(level, victim, DAM_LIGHTNING))
    {
        dam /= 3;
    }

    damage(ch, victim, dam, sn, DAM_LIGHTNING, TRUE);
    last_vict = victim;
    level -= 4;                    /* decrement damage */

                                   /* new targets */
    while (level > 0)
    {
        found = FALSE;
        for (tmp_vict = ch->in_room->people;
            tmp_vict != NULL; tmp_vict = next_vict)
        {
            next_vict = tmp_vict->next_in_room;
            if (!is_safe_spell(ch, tmp_vict, TRUE) && tmp_vict != last_vict)
            {
                found = TRUE;
                last_vict = tmp_vict;
                act("The bolt arcs to $n!", tmp_vict, NULL, NULL, TO_ROOM);
                act("The bolt hits you!", tmp_vict, NULL, NULL, TO_CHAR);
                dam = dice(level, 6);
                if (saves_spell(level, tmp_vict, DAM_LIGHTNING))
                    dam /= 3;
                damage(ch, tmp_vict, dam, sn, DAM_LIGHTNING, TRUE);
                level -= 4;        /* decrement damage */
            }
        }                        /* end target searching loop */

        if (!found)
        {                        /* no target found, hit the caster */
            if (ch == NULL)
                return;

            if (last_vict == ch)
            {                    /* no double hits */
                act("The bolt seems to have fizzled out.", ch, NULL, NULL,
                    TO_ROOM);
                act("The bolt grounds out through your body.", ch, NULL,
                    NULL, TO_CHAR);
                return;
            }

            last_vict = ch;
            act("The bolt arcs to $n...whoops!", ch, NULL, NULL, TO_ROOM);
            send_to_char("You are struck by your own lightning!\r\n", ch);
            dam = dice(level, 6);
            if (saves_spell(level, ch, DAM_LIGHTNING))
                dam /= 3;
            damage(ch, ch, dam, sn, DAM_LIGHTNING, TRUE);
            level -= 4;            /* decrement damage */
            if (ch == NULL)
                return;
        }
        /* now go back and find more targets */
    }
}

/*
 * Combat spell - Chill Touch
 */
void spell_chill_touch(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    static const int dam_each[] = {
        0,
        0, 0, 6, 7, 8, 9, 12, 13, 13, 13,
        14, 14, 14, 15, 15, 15, 16, 16, 16, 17,
        17, 17, 18, 18, 18, 19, 19, 19, 20, 20,
        20, 21, 21, 21, 22, 22, 22, 23, 23, 23,
        24, 24, 24, 25, 25, 25, 26, 26, 26, 27
    };

    AFFECT_DATA af;
    int dam;

    level = UMIN(level, sizeof(dam_each) / sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);

    dam = number_range(dam_each[level] / 2, dam_each[level] * 2);

    if (!saves_spell(level, victim, DAM_COLD))
    {
        act("$n turns blue and shivers.", victim, NULL, NULL, TO_ROOM);
        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = level;
        af.duration = 6;
        af.location = APPLY_STR;
        af.modifier = -1;
        af.bitvector = 0;
        affect_join(victim, &af);
    }
    else
    {
        dam /= 2;
    }

    damage(ch, victim, dam, sn, DAM_COLD, TRUE);
    return;
}

/*
 * Combat spell - Color Spray
 */
void spell_color_spray(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    static const int dam_each[] = {
        0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        30, 35, 40, 45, 50, 55, 55, 55, 56, 57,
        58, 58, 59, 60, 61, 61, 62, 63, 64, 64,
        65, 66, 67, 67, 68, 69, 70, 70, 71, 72,
        73, 73, 74, 75, 76, 76, 77, 78, 79, 79
    };
    int dam;

    level = UMIN(level, sizeof(dam_each) / sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);

    dam = number_range(dam_each[level] / 2, dam_each[level] * 2);

    if (saves_spell(level, victim, DAM_LIGHT))
    {
        dam /= 2;
    }
    else
    {
        spell_blindness(gsn_blindness, level / 2, ch, (void *)victim, TARGET_CHAR);
    }

    damage(ch, victim, dam, sn, DAM_LIGHT, TRUE);
    return;
}

/*
 * Combat Spell - Fireball, this spell will do fire damage and support ranging.
 */
void spell_fireball(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    int dam;
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    static const int dam_each[] = {
        0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 30, 35, 40, 45, 50, 55,
        60, 65, 70, 75, 80, 82, 84, 86, 88, 90,
        92, 94, 96, 98, 100, 102, 104, 106, 108, 110,
        112, 114, 116, 118, 120, 122, 124, 126, 128, 130
    };

    level = UMIN(level, sizeof(dam_each) / sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);
    dam = number_range(dam_each[level] / 2, dam_each[level] * 2);

    if (saves_spell(level, victim, DAM_FIRE))
    {
        dam /= 2;
    }

    damage(ch, victim, dam, sn, DAM_FIRE, TRUE);
    return;
}

/*
* Combat Spell - Ice blast, this is the opposite of fireball.  This will do cold damage.
*/
void spell_ice_blast(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    int dam;
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    static const int dam_each[] = {
        0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 30, 35, 40, 45, 50, 55,
        60, 65, 70, 75, 80, 82, 84, 86, 88, 90,
        92, 94, 96, 98, 100, 102, 104, 106, 108, 110,
        112, 114, 116, 118, 120, 122, 124, 126, 128, 130
    };

    level = UMIN(level, sizeof(dam_each) / sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);
    dam = number_range(dam_each[level] / 2, dam_each[level] * 2);

    if (saves_spell(level, victim, DAM_COLD))
    {
        dam /= 2;
    }

    damage(ch, victim, dam, sn, DAM_COLD, TRUE);
    return;
}

/*
 * Combat Spell - Lightning Bolt
 */
void spell_lightning_bolt(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    static const int dam_each[] = {
        0,
        0, 0, 0, 0, 0, 0, 0, 0, 25, 28,
        31, 34, 37, 40, 40, 41, 42, 42, 43, 44,
        44, 45, 46, 46, 47, 48, 48, 49, 50, 50,
        51, 52, 52, 53, 54, 54, 55, 56, 56, 57,
        58, 58, 59, 60, 60, 61, 62, 62, 63, 64
    };
    int dam;

    level = UMIN(level, sizeof(dam_each) / sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);
    dam = number_range(dam_each[level] / 2, dam_each[level] * 2);
    if (saves_spell(level, victim, DAM_LIGHTNING))
        dam /= 2;
    damage(ch, victim, dam, sn, DAM_LIGHTNING, TRUE);
    return;
}

/*
 * Combat Spell - Magic Missle
 */
void spell_magic_missile(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    static const int dam_each[] = {
        0,
        3, 3, 4, 4, 5, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 8, 8, 8, 8, 8,
        9, 9, 9, 9, 9, 10, 10, 10, 10, 10,
        11, 11, 11, 11, 11, 12, 12, 12, 12, 12,
        13, 13, 13, 13, 13, 14, 14, 14, 14, 14
    };
    int dam;

    level = UMIN(level, sizeof(dam_each) / sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);

    dam = number_range(dam_each[level] / 2, dam_each[level] * 2);

    if (saves_spell(level, victim, DAM_ENERGY))
    {
        dam /= 2;
    }

    damage(ch, victim, dam, sn, DAM_ENERGY, TRUE);
    return;
}

/*
 * Combat Spell - Shocking Grasp
 */
void spell_shocking_grasp(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    static const int dam_each[] = {
        0,
        0, 0, 0, 0, 0, 0, 20, 25, 29, 33,
        36, 39, 39, 39, 40, 40, 41, 41, 42, 42,
        43, 43, 44, 44, 45, 45, 46, 46, 47, 47,
        48, 48, 49, 49, 50, 50, 51, 51, 52, 52,
        53, 53, 54, 54, 55, 55, 56, 56, 57, 57
    };
    int dam;

    level = UMIN(level, sizeof(dam_each) / sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);

    dam = number_range(dam_each[level] / 2, dam_each[level] * 2);

    if (saves_spell(level, victim, DAM_LIGHTNING))
    {
        dam /= 2;
    }

    damage(ch, victim, dam, sn, DAM_LIGHTNING, TRUE);
    return;
}

/*
 * Beguilding Spell - Calm, RT calm spell stops all fighting in the room
 */
void spell_calm(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *vch;
    int mlevel = 0;
    int count = 0;
    int high_level = 0;
    int chance;
    AFFECT_DATA af;

    /* get sum of all mobile levels in the room */
    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->position == POS_FIGHTING)
        {
            count++;
            if (IS_NPC(vch))
                mlevel += vch->level;
            else
                mlevel += vch->level / 2;
            high_level = UMAX(high_level, vch->level);
        }
    }

    /* compute chance of stopping combat */
    chance = 4 * level - high_level + 2 * count;

    if (IS_IMMORTAL(ch))        /* always works */
        mlevel = 0;

    if (number_range(0, chance) >= mlevel)
    {                            /* hard to stop large fights */
        for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            if (IS_NPC(vch) && (IS_SET(vch->imm_flags, IMM_MAGIC) ||
                IS_SET(vch->act, ACT_UNDEAD)))
                return;

            if (IS_AFFECTED(vch, AFF_CALM) || IS_AFFECTED(vch, AFF_BERSERK)
                || is_affected(vch, gsn_frenzy))
                return;

            send_to_char("A wave of calm passes over you.\r\n", vch);

            if (vch->fighting || vch->position == POS_FIGHTING)
                stop_fighting(vch, FALSE);


            af.where = TO_AFFECTS;
            af.type = sn;
            af.level = level;
            af.duration = level / 4;
            af.location = APPLY_HITROLL;
            if (!IS_NPC(vch))
                af.modifier = -5;
            else
                af.modifier = -2;
            af.bitvector = AFF_CALM;
            affect_to_char(vch, &af);

            af.location = APPLY_DAMROLL;
            affect_to_char(vch, &af);
        }
    }
}

/*
 * Beguiling Spell - Charm Person
 */
void spell_charm_person(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_safe(ch, victim))
        return;

    if (victim == ch)
    {
        send_to_char("You like yourself even better!\r\n", ch);
        return;
    }

    if (IS_AFFECTED(victim, AFF_CHARM)
        || IS_AFFECTED(ch, AFF_CHARM)
        || level < victim->level || IS_SET(victim->imm_flags, IMM_CHARM)
        || saves_spell(level, victim, DAM_CHARM))
        return;


    if (IS_SET(victim->in_room->room_flags, ROOM_LAW))
    {
        send_to_char("Charming is not allowed charming in the city limits.\r\n", ch);
        return;
    }

    if (victim->master)
    {
        stop_follower(victim);
    }

    add_follower(victim, ch);
    victim->leader = ch;
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = number_fuzzy(level / 4);
    af.location = 0;
    af.modifier = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char(victim, &af);

    act("Isn't $n just so nice?", ch, NULL, victim, TO_VICT);

    if (ch != victim)
    {
        act("$N looks at you with adoring eyes.", ch, NULL, victim, TO_CHAR);
    }

    return;
}

/*
 * Beguilding Spell - Sleep
 */
void spell_sleep(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_SLEEP))
    {
        send_to_char("They are already affected by a sleep spell.\r\n", ch);
        return;
    }

    if ((IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD))
        || (level + 2) < victim->level
        || saves_spell(level - 4, victim, DAM_CHARM))
    {
        send_to_char("The spell failed.\r\n", ch);
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 4 + level;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_SLEEP;
    affect_join(victim, &af);

    if (IS_AWAKE(victim))
    {
        send_to_char("You feel very sleepy ..... zzzzzz.\r\n", victim);
        act("$n goes to sleep.", victim, NULL, NULL, TO_ROOM);
        victim->position = POS_SLEEPING;
    }

    return;
}

/*
 * Illusion Spell - Invisibility
 */
void spell_invis(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;

    /* object invisibility */
    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *)vo;
        separate_obj(obj);

        if (IS_OBJ_STAT(obj, ITEM_INVIS))
        {
            act("$p is already invisible.", ch, obj, NULL, TO_CHAR);
            return;
        }

        // Can't make any corpses invisible.
        if (obj->item_type == ITEM_CORPSE_NPC
            || obj->item_type == ITEM_CORPSE_PC)
        {
            send_to_char("Your spell failed.\r\n", ch);
            return;
        }

        af.where = TO_OBJECT;
        af.type = sn;
        af.level = level;
        af.duration = level + 12;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = ITEM_INVIS;
        affect_to_obj(obj, &af);

        act("$p fades out of sight.", ch, obj, NULL, TO_ALL);
        return;
    }

    /* character invisibility */
    victim = (CHAR_DATA *)vo;

    if (IS_AFFECTED(victim, AFF_INVISIBLE))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            // You cannot re-add this spell to someone else, this will stop people with lower
            // casting levels from replacing someone elses spells.
            act("$N is already invisible.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    act("$n fades out of existence.", victim, NULL, NULL, TO_ROOM);

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level + 12;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_INVISIBLE;
    affect_to_char(victim, &af);
    send_to_char("You fade out of existence.\r\n", victim);
    return;
}

/*
 * Illusion Spell - Mass Invisibility
 */
void spell_mass_invis(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    AFFECT_DATA af;
    CHAR_DATA *gch;

    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
    {
        if (!is_same_group(gch, ch) || IS_AFFECTED(gch, AFF_INVISIBLE))
        {
            continue;
        }

        act("$n slowly fades out of existence.", gch, NULL, NULL, TO_ROOM);
        send_to_char("You slowly fade out of existence.\r\n", gch);

        af.where = TO_AFFECTS;
        af.type = sn;
        af.level = level / 2;
        af.duration = 24;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = AFF_INVISIBLE;
        affect_to_char(gch, &af);
    }

    send_to_char("Ok.\r\n", ch);

    return;
}

/*
 * Illusion Spell - Ventriloquote
 */
void spell_ventriloquate(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char speaker[MAX_INPUT_LENGTH];
    CHAR_DATA *vch;

    target_name = one_argument(target_name, speaker);

    sprintf(buf1, "%s says '%s'.\r\n", speaker, target_name);
    sprintf(buf2, "Someone makes %s say '%s'.\r\n", speaker, target_name);
    buf1[0] = UPPER(buf1[0]);

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (!is_exact_name(speaker, vch->name) && IS_AWAKE(vch))
        {
            send_to_char(saves_spell(level, vch, DAM_OTHER) ? buf2 : buf1, vch);
        }
    }

    return;
}

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

/****************************************************************************
 *  Mage Cleric                                                             *
 *                                                                          *
 *  Cleric is a base class for which many reclasses may share spells with.  *
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
 * Benediction - Bless Spell
 */
void spell_bless(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;

    /* deal with the object case first */
    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *)vo;
        separate_obj(obj);

        if (IS_OBJ_STAT(obj, ITEM_BLESS))
        {
            act("$p is already blessed.", ch, obj, NULL, TO_CHAR);
            return;
        }

        if (IS_OBJ_STAT(obj, ITEM_EVIL))
        {
            AFFECT_DATA *paf;

            paf = affect_find(obj->affected, gsn_curse);
            if (!saves_dispel
            (level, paf != NULL ? paf->level : obj->level, 0))
            {
                if (paf != NULL)
                    affect_remove_obj(obj, paf);
                act("$p glows a pale blue.", ch, obj, NULL, TO_ALL);
                REMOVE_BIT(obj->extra_flags, ITEM_EVIL);
                return;
            }
            else
            {
                act("The evil of $p is too powerful for you to overcome.",
                    ch, obj, NULL, TO_CHAR);
                return;
            }
        }

        af.where = TO_OBJECT;
        af.type = sn;
        af.level = level;
        af.duration = 6 + level;
        af.location = APPLY_SAVES;
        af.modifier = -1;
        af.bitvector = ITEM_BLESS;
        affect_to_obj(obj, &af);

        act("$p glows with a holy aura.", ch, obj, NULL, TO_ALL);

        if (obj->wear_loc != WEAR_NONE)
            ch->saving_throw -= 1;
        return;
    }

    /* character target */
    victim = (CHAR_DATA *)vo;


    if (victim->position == POS_FIGHTING || is_affected(victim, sn))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N already has divine favor.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 6 + level;
    af.location = APPLY_HITROLL;
    af.modifier = level / 8;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    af.location = APPLY_SAVES;
    af.modifier = 0 - level / 8;
    affect_to_char(victim, &af);
    send_to_char("You feel righteous.\r\n", victim);
    if (ch != victim)
        act("You grant $N the favor of your god.", ch, NULL, victim,
            TO_CHAR);
    return;
}

/*
 * Benediction - Frenzy Spell (RT clerical berserking spell)
 */
void spell_frenzy(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn) || IS_AFFECTED(victim, AFF_BERSERK))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N is already in a frenzy.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    if (is_affected(victim, gsn_calm))
    {
        if (victim == ch)
            send_to_char("Why don't you just relax for a while?\r\n", ch);
        else
            act("$N doesn't look like $e wants to fight anymore.",
                ch, NULL, victim, TO_CHAR);
        return;
    }

    if ((IS_GOOD(ch) && !IS_GOOD(victim)) ||
        (IS_NEUTRAL(ch) && !IS_NEUTRAL(victim)) ||
        (IS_EVIL(ch) && !IS_EVIL(victim)))
    {
        act("Your god doesn't seem to like $N", ch, NULL, victim, TO_CHAR);
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level / 3;
    af.modifier = level / 6;
    af.bitvector = 0;

    af.location = APPLY_HITROLL;
    affect_to_char(victim, &af);

    af.location = APPLY_DAMROLL;
    affect_to_char(victim, &af);

    af.modifier = 10 * (level / 12);
    af.location = APPLY_AC;
    affect_to_char(victim, &af);

    send_to_char("You are filled with holy wrath!\r\n", victim);
    act("$n gets a wild look in $s eyes!", victim, NULL, NULL, TO_ROOM);
}

/*
 * Benediction - Holy Word Spell (RT really nasty high-level attack spell)
 */
void spell_holy_word(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;

    act("$n utters a word of divine power!", ch, NULL, NULL, TO_ROOM);
    send_to_char("You utter a word of divine power.\r\n", ch);

    for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if ((IS_GOOD(ch) && IS_GOOD(vch)) ||
            (IS_EVIL(ch) && IS_EVIL(vch)) ||
            (IS_NEUTRAL(ch) && IS_NEUTRAL(vch)))
        {
            send_to_char("You feel full more powerful.\r\n", vch);
            spell_frenzy(gsn_frenzy, level, ch, (void *)vch, TARGET_CHAR);
            spell_bless(gsn_bless, level, ch, (void *)vch, TARGET_CHAR);
        }

        else if ((IS_GOOD(ch) && IS_EVIL(vch)) ||
            (IS_EVIL(ch) && IS_GOOD(vch)))
        {
            if (!is_safe_spell(ch, vch, TRUE))
            {
                spell_curse(gsn_curse, level, ch, (void *)vch, TARGET_CHAR);
                send_to_char("You are struck down!\r\n", vch);
                dam = dice(level, 6);
                damage(ch, vch, dam, sn, DAM_ENERGY, TRUE);
            }
        }

        else if (IS_NEUTRAL(ch))
        {
            if (!is_safe_spell(ch, vch, TRUE))
            {
                spell_curse(gsn_curse, level / 2, ch, (void *)vch,
                    TARGET_CHAR);
                send_to_char("You are struck down!\r\n", vch);
                dam = dice(level, 4);
                damage(ch, vch, dam, sn, DAM_ENERGY, TRUE);
            }
        }
    }

    send_to_char("You feel drained.\r\n", ch);
    ch->move = 0;
    ch->hit /= 2;
}

/*
 * Benediction - Remove Curse Spell
 */
void spell_remove_curse(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    bool found = FALSE;

    /* do object cases first */
    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *)vo;

        if (IS_OBJ_STAT(obj, ITEM_NODROP)
            || IS_OBJ_STAT(obj, ITEM_NOREMOVE))
        {
            if (!IS_OBJ_STAT(obj, ITEM_NOUNCURSE)
                && !saves_dispel(level + 2, obj->level, 0))
            {
                REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
                REMOVE_BIT(obj->extra_flags, ITEM_NOREMOVE);
                act("$p glows blue.", ch, obj, NULL, TO_ALL);
                return;
            }

            act("The curse on $p is beyond your power.", ch, obj, NULL,
                TO_CHAR);
            return;
        }
        act("There doesn't seem to be a curse on $p.", ch, obj, NULL,
            TO_CHAR);
        return;
    }

    /* characters */
    victim = (CHAR_DATA *)vo;

    if (check_dispel(level, victim, gsn_curse))
    {
        send_to_char("You feel better.\r\n", victim);
        act("$n looks more relaxed.", victim, NULL, NULL, TO_ROOM);
    }

    for (obj = victim->carrying; (obj != NULL && !found);
        obj = obj->next_content)
    {
        if ((IS_OBJ_STAT(obj, ITEM_NODROP)
            || IS_OBJ_STAT(obj, ITEM_NOREMOVE))
            && !IS_OBJ_STAT(obj, ITEM_NOUNCURSE))
        {                        /* attempt to remove curse */
            if (!saves_dispel(level, obj->level, 0))
            {
                found = TRUE;
                REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
                REMOVE_BIT(obj->extra_flags, ITEM_NOREMOVE);
                act("Your $p glows blue.", victim, obj, NULL, TO_CHAR);
                act("$n's $p glows blue.", victim, obj, NULL, TO_ROOM);
            }
        }
    }
}

/*
* A cleric spell which raise the casting level of the person it has been cast on.
* The modifier is the value of the levels that will rise, it is set here so that
* lower level clerics cast lower casting gains with imbue.
*/
void spell_imbue(int sn, int level, CHAR_DATA *ch, void *vo, int target)
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
            // You cannot re-add this spell to someone else, this will stop people with lower
            // casting levels from replacing someone elses spells.
            act("$N is already imbued.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level / 12;
    af.location = 0;
    af.modifier = level / 15;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("Your magical aptitude has been augmented.\r\n", victim);
    act("$n's magical aptitude has been augmented.", victim, NULL, NULL, TO_ROOM);

    return;
}

/*
 * Attack - Demonfire Spell (RT replacement demonfire spell )
 */
void spell_demonfire(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int dam;

    if (!IS_NPC(ch) && !IS_EVIL(ch))
    {
        victim = ch;
        send_to_char("The demons turn upon you!\r\n", ch);
    }

    if (victim != ch)
    {
        act("$n calls forth the demons of Hell upon $N!", ch, NULL, victim, TO_ROOM);
        act("$n has assailed you with the demons of Hell!", ch, NULL, victim, TO_VICT);
        send_to_char("You conjure forth the demons of hell!\r\n", ch);
    }

    dam = dice(level, 10);

    if (saves_spell(level, victim, DAM_NEGATIVE))
    {
        dam /= 2;
    }

    // Damage, with a chance to curse at a lower level.
    damage(ch, victim, dam, sn, DAM_NEGATIVE, TRUE);
    spell_curse(gsn_curse, 3 * level / 4, ch, (void *)victim, TARGET_CHAR);
}

/*
 * Attack - Dispel Evil Spell
 */
void spell_dispel_evil(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int dam;

    if (!IS_NPC(ch) && IS_EVIL(ch))
        victim = ch;

    if (IS_GOOD(victim))
    {
        act("The gods protect $N.", ch, NULL, victim, TO_ROOM);
        return;
    }

    if (IS_NEUTRAL(victim))
    {
        act("$N does not seem to be affected.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (victim->hit > (ch->level * 4))
        dam = dice(level, 4);
    else
        dam = UMAX(victim->hit, dice(level, 4));
    if (saves_spell(level, victim, DAM_HOLY))
        dam /= 2;
    damage(ch, victim, dam, sn, DAM_HOLY, TRUE);
    return;
}

/*
 * Attack - Dispel Good Spell
 */
void spell_dispel_good(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int dam;

    if (!IS_NPC(ch) && IS_GOOD(ch))
        victim = ch;

    if (IS_EVIL(victim))
    {
        act("$N is protected by $S evil.", ch, NULL, victim, TO_ROOM);
        return;
    }

    if (IS_NEUTRAL(victim))
    {
        act("$N does not seem to be affected.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (victim->hit > (ch->level * 4))
        dam = dice(level, 4);
    else
        dam = UMAX(victim->hit, dice(level, 4));
    if (saves_spell(level, victim, DAM_NEGATIVE))
        dam /= 2;
    damage(ch, victim, dam, sn, DAM_NEGATIVE, TRUE);
    return;
}

/*
 * Attack - Earthquake
 */
void spell_earthquake(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;

    send_to_char("The earth trembles beneath your feet!\r\n", ch);
    act("$n makes the earth tremble and shiver.", ch, NULL, NULL, TO_ROOM);

    for (vch = char_list; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next;
        if (vch->in_room == NULL)
            continue;
        if (vch->in_room == ch->in_room)
        {
            if (vch != ch && !is_safe_spell(ch, vch, TRUE))
            {
                if (IS_AFFECTED(vch, AFF_FLYING))
                    damage(ch, vch, 0, sn, DAM_BASH, TRUE);
                else
                    damage(ch, vch, level + dice(2, 8), sn, DAM_BASH, TRUE);
            }
            continue;
        }

        if (vch->in_room->area == ch->in_room->area)
            send_to_char("The earth trembles and shivers.\r\n", vch);
    }

    return;
}

/*
 * Attack - Flamestrike Spell
 */
void spell_flamestrike(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int dam;

    dam = dice(6 + level / 2, 8);
    if (saves_spell(level, victim, DAM_FIRE))
        dam /= 2;
    damage(ch, victim, dam, sn, DAM_FIRE, TRUE);
    return;
}

/*
 * Attack - Heat Metal Spell
 */
void spell_heat_metal(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    OBJ_DATA *obj_lose, *obj_next;
    int dam = 0;
    bool fail = TRUE;

    if (!saves_spell(level + 2, victim, DAM_FIRE)
        && !IS_SET(victim->imm_flags, IMM_FIRE))
    {
        for (obj_lose = victim->carrying;
            obj_lose != NULL; obj_lose = obj_next)
        {
            obj_next = obj_lose->next_content;
            if (number_range(1, 2 * level) > obj_lose->level
                && !saves_spell(level, victim, DAM_FIRE)
                && !IS_OBJ_STAT(obj_lose, ITEM_NONMETAL)
                && !IS_OBJ_STAT(obj_lose, ITEM_BURN_PROOF))
            {
                switch (obj_lose->item_type)
                {
                    case ITEM_ARMOR:
                        if (obj_lose->wear_loc != -1)
                        {        /* remove the item */
                            if (can_drop_obj(victim, obj_lose)
                                && (obj_lose->weight / 10) <
                                number_range(1,
                                    2 * get_curr_stat(victim,
                                        STAT_DEX))
                                && remove_obj(victim, obj_lose->wear_loc,
                                    TRUE))
                            {
                                act("$n yelps and throws $p to the ground!",
                                    victim, obj_lose, NULL, TO_ROOM);
                                act
                                ("You remove and drop $p before it burns you.",
                                    victim, obj_lose, NULL, TO_CHAR);
                                dam +=
                                    (number_range(1, obj_lose->level) / 3);

                                separate_obj(obj_lose);
                                obj_from_char(obj_lose);
                                obj_to_room(obj_lose, victim->in_room);
                                fail = FALSE;
                            }
                            else
                            {    /* stuck on the body! ouch! */

                                act("Your skin is seared by $p!",
                                    victim, obj_lose, NULL, TO_CHAR);
                                dam += (number_range(1, obj_lose->level));
                                fail = FALSE;
                            }

                        }
                        else
                        {        /* drop it if we can */

                            if (can_drop_obj(victim, obj_lose))
                            {
                                act("$n yelps and throws $p to the ground!",
                                    victim, obj_lose, NULL, TO_ROOM);
                                act("You and drop $p before it burns you.",
                                    victim, obj_lose, NULL, TO_CHAR);
                                dam +=
                                    (number_range(1, obj_lose->level) / 6);
                                separate_obj(obj_lose);
                                obj_from_char(obj_lose);
                                obj_to_room(obj_lose, victim->in_room);
                                fail = FALSE;
                            }
                            else
                            {    /* cannot drop */

                                act("Your skin is seared by $p!",
                                    victim, obj_lose, NULL, TO_CHAR);
                                dam +=
                                    (number_range(1, obj_lose->level) / 2);
                                fail = FALSE;
                            }
                        }
                        break;
                    case ITEM_WEAPON:
                        if (obj_lose->wear_loc != -1)
                        {        /* try to drop it */
                            if (IS_WEAPON_STAT(obj_lose, WEAPON_FLAMING))
                                continue;

                            if (can_drop_obj(victim, obj_lose)
                                && remove_obj(victim, obj_lose->wear_loc,
                                    TRUE))
                            {
                                act
                                ("$n is burned by $p, and throws it to the ground.",
                                    victim, obj_lose, NULL, TO_ROOM);
                                send_to_char
                                ("You throw your red-hot weapon to the ground!\r\n",
                                    victim);
                                dam += 1;
                                separate_obj(obj_lose);
                                obj_from_char(obj_lose);
                                obj_to_room(obj_lose, victim->in_room);
                                fail = FALSE;

                                // Can't dual with without a primary, consider changing this to moving the
                                // dual wielded weapon to the primary arm.
                                OBJ_DATA *vobj;
                                if ((vobj = get_eq_char(victim, WEAR_SECONDARY_WIELD)) != NULL)
                                {
                                    act("$n stops using $p.", victim, vobj, NULL, TO_ROOM);
                                    act("You stop using $p.", victim, vobj, NULL, TO_CHAR);
                                    separate_obj(vobj);
                                    unequip_char(victim, vobj);
                                }

                            }
                            else
                            {    /* YOWCH! */

                                send_to_char
                                ("Your weapon sears your flesh!\r\n",
                                    victim);
                                dam += number_range(1, obj_lose->level);
                                fail = FALSE;
                            }
                        }
                        else
                        {        /* drop it if we can */

                            if (can_drop_obj(victim, obj_lose))
                            {
                                act
                                ("$n throws a burning hot $p to the ground!",
                                    victim, obj_lose, NULL, TO_ROOM);
                                act("You and drop $p before it burns you.",
                                    victim, obj_lose, NULL, TO_CHAR);
                                dam +=
                                    (number_range(1, obj_lose->level) / 6);
                                separate_obj(obj_lose);
                                obj_from_char(obj_lose);
                                obj_to_room(obj_lose, victim->in_room);
                                fail = FALSE;
                            }
                            else
                            {    /* cannot drop */

                                act("Your skin is seared by $p!",
                                    victim, obj_lose, NULL, TO_CHAR);
                                dam +=
                                    (number_range(1, obj_lose->level) / 2);
                                fail = FALSE;
                            }
                        }
                        break;
                }
            }
        }
    }
    if (fail)
    {
        send_to_char("Your spell had no effect.\r\n", ch);
        send_to_char("You feel momentarily warmer.\r\n", victim);
    }
    else
    {                            /* damage! */

        if (saves_spell(level, victim, DAM_FIRE))
            dam = 2 * dam / 3;
        damage(ch, victim, dam, sn, DAM_FIRE, TRUE);
    }
}

/*
 * Attack - Ray of Truth Spell
 */
void spell_ray_of_truth(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int dam;

    if (IS_EVIL(ch))
    {
        victim = ch;
        send_to_char("The energy explodes inside you!\r\n", ch);
    }

    if (victim != ch)
    {
        act("$n raises $s hand, and a blinding ray of light shoots forth!", ch, NULL, NULL, TO_ROOM);
        send_to_char("You raise your hand and a blinding ray of light shoots forth!\r\n", ch);
    }

    if (IS_GOOD(victim))
    {
        act("$n seems unharmed by the light.", victim, NULL, victim, TO_ROOM);
        send_to_char("The light seems powerless to affect you.\r\n", victim);
        return;
    }

    dam = dice(level, 10);

    if (saves_spell(level, victim, DAM_HOLY))
    {
        dam /= 2;
    }

    dam = (dam * 350 * 350) / 1000000;

    // Damage, and then a chance to blind also but at a lower casting level.
    damage(ch, victim, dam, sn, DAM_HOLY, TRUE);
    spell_blindness(gsn_blindness, 3 * level / 4, ch, (void *)victim, TARGET_CHAR);
}

/*
 * Curative - Cure blindess can individually remove a blindness on any player.  Healer's
 * get a casting bonus on this spell.
 */
void spell_cure_blindness(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if (!is_affected(victim, gsn_blindness))
    {
        if (victim == ch)
            send_to_char("You aren't blind.\r\n", ch);
        else
            act("$N doesn't appear to be blinded.", ch, NULL, victim,
                TO_CHAR);
        return;
    }

    // Healer bonus
    if (ch->class == HEALER_CLASS)
    {
        level += 3;
    }

    if (check_dispel(level, victim, gsn_blindness))
    {
        send_to_char("Your vision returns!\r\n", victim);
        act("$n is no longer blinded.", victim, NULL, NULL, TO_ROOM);
    }
    else
    {
        send_to_char("Spell failed.\r\n", ch);
    }

} // end spell_cure_blindness

/*
 * Curative - Spell to allow cleric and reclasses to cure the plague.  Healer's will get
 * a bonus on this spell.
 */
void spell_cure_disease(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    if (!is_affected(victim, gsn_plague))
    {
        if (victim == ch)
        {
            send_to_char("You aren't ill.\r\n", ch);
        }
        else
        {
            act("$N doesn't appear to be diseased.", ch, NULL, victim, TO_CHAR);
        }
        return;
    }

    // Healer bonus
    if (ch->class == HEALER_CLASS)
    {
        level += 3;
    }

    if (check_dispel(level, victim, gsn_plague))
    {
        // The message to the player comes from the spell config in skills.dat
        act("$n looks relieved as $s sores vanish.", victim, NULL, NULL, TO_ROOM);
    }
    else
    {
        send_to_char("Spell failed.\r\n", ch);
    }

} // end spell_cure_disease

/*
 * Curative - Spell to cure poison.  Healers will get a bonus on this spell.
 */
void spell_cure_poison(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    bool success = FALSE;

    if (!is_affected(victim, gsn_poison) && !is_affected(victim, gsn_poison_prick))
    {
        if (victim == ch)
        {
            send_to_char("You aren't poisoned.\r\n", ch);
        }
        else
        {
            act("$N doesn't appear to be poisoned.", ch, NULL, victim, TO_CHAR);
        }
        return;
    }

    // Healer bonus
    if (ch->class == HEALER_CLASS)
    {
        level += 3;
    }

    if (check_dispel(level, victim, gsn_poison))
    {
        success = TRUE;
        send_to_char("A warm feeling runs through your body.\r\n", victim);
        act("$n looks much better.", victim, NULL, NULL, TO_ROOM);
    }

    if (check_dispel(level, victim, gsn_poison_prick))
    {
        // Show them the message if they haven't already seen it.
        if (success == FALSE)
        {
            send_to_char("A warm feeling runs through your body.\r\n", victim);
            act("$n looks much better.", victim, NULL, NULL, TO_ROOM);
        }

        success = TRUE;
    }

    if (success == FALSE)
    {
        send_to_char("Spell failed.\r\n", ch);
    }

} // end spell_cure_poison

/*
 * Harmful - Cause Light Spell
 */
void spell_cause_light(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    damage(ch, (CHAR_DATA *)vo, dice(1, 8) + level / 3, sn, DAM_HARM, TRUE);
    return;
}

/*
* Harmful - Cause Critical Spell
*/
void spell_cause_critical(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    damage(ch, (CHAR_DATA *)vo, dice(3, 8) + level - 6, sn, DAM_HARM, TRUE);
    return;
}

/*
* Harmful - Cause Serious Spell
*/
void spell_cause_serious(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    damage(ch, (CHAR_DATA *)vo, dice(2, 8) + level / 2, sn, DAM_HARM, TRUE);
    return;
}

/*
 * Harmful- Harm Spell
 */
void spell_harm(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int dam;

    dam = UMAX(20, victim->hit - dice(1, 4));
    if (saves_spell(level, victim, DAM_HARM))
        dam = UMIN(50, dam / 2);
    dam = UMIN(100, dam);
    damage(ch, victim, dam, sn, DAM_HARM, TRUE);
    return;
}

/*
 * Healing - Cure Critical
 */
void spell_cure_critical(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int heal;

    heal = dice(3, 8) + level - 6;
    victim->hit = UMIN(victim->hit + heal, victim->max_hit);
    update_pos(victim);
    send_to_char("You feel better!\r\n", victim);
    if (ch != victim)
        send_to_char("Ok.\r\n", ch);
    return;
}

/*
 * Healing - Cure Light
 */
void spell_cure_light(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int heal;

    heal = dice(1, 8) + level / 3;
    victim->hit = UMIN(victim->hit + heal, victim->max_hit);
    update_pos(victim);
    send_to_char("You feel better!\r\n", victim);
    if (ch != victim)
        send_to_char("Ok.\r\n", ch);
    return;
}

/*
 * Healing - Cure Serious
 */
void spell_cure_serious(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int heal;

    heal = dice(2, 8) + level / 2;
    victim->hit = UMIN(victim->hit + heal, victim->max_hit);
    update_pos(victim);
    send_to_char("You feel better!\r\n", victim);
    if (ch != victim)
        send_to_char("Ok.\r\n", ch);
    return;
}

/*
 * Healing - Heal
 */
void spell_heal(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    victim->hit = UMIN(victim->hit + 100, victim->max_hit);
    update_pos(victim);
    send_to_char("A warm feeling fills your body.\r\n", victim);
    if (ch != victim)
        send_to_char("Ok.\r\n", ch);
    return;
}

/*
 * Healing - Mass Healing
 */
void spell_mass_healing(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *gch;

    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
    {
        if ((IS_NPC(ch) && IS_NPC(gch)) || (!IS_NPC(ch) && !IS_NPC(gch)))
        {
            spell_heal(gsn_heal, level, ch, (void *)gch, TARGET_CHAR);
            spell_refresh(gsn_refresh, level, ch, (void *)gch, TARGET_CHAR);
        }
    }
}

/*
 * Healing/Enhancement - Refresh spells, replenishes movement.  Healer's get a 10 movemement
 * bonus which was tacted onto the level since it's not used for anything else in this
 * spell.
 */
void spell_refresh(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;

    // Healer bonus
    if (ch->class == HEALER_CLASS)
    {
        level += 10;
    }

    victim->move = UMIN(victim->move + level, victim->max_move);

    if (victim->max_move == victim->move)
    {
        send_to_char("You feel fully refreshed!\r\n", victim);
    }
    else
    {
        send_to_char("You feel less tired.\r\n", victim);
    }

    if (ch != victim)
    {
        send_to_char("Ok.\r\n", ch);
    }

    return;

} // end spell_refresh

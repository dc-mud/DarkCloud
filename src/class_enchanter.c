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
 *  Enchanter class                                                        *
 *                                                                         *
 *  This class will be an expert at enchanting or altering items           *
 *  as well as having the potential to also enchant other characters       *
 *  abilities as well as receive bonuses on tradtional mage spells like    *
 *  charm.  The enchanter is a mage who specializes in enhancing others    *
 *  and objects.                                                           *
 *                                                                         *
 **************************************************************************/

// General Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"

// Local Declarations
void set_obj_enchanted(CHAR_DATA * ch, OBJ_DATA * obj, bool clear_first);

/*
 * Spell that sets a rot-death flag on an item so it crumbles when the
 * player dies.  The spell is set to obj_inventory so it can only be cast
 * on an item a player holds.
 */
void spell_withering_enchant(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    if (!IS_SET(obj->extra_flags, ITEM_ROT_DEATH))
    {
        separate_obj(obj);
        SET_BIT(obj->extra_flags, ITEM_ROT_DEATH);
        act("$p glows with a withered aura.", ch, obj, NULL, TO_CHAR);
        act("$p glows with a withered aura.", ch, obj, NULL, TO_ROOM);
    }
    else
    {
        act("$p already has a withered aura about it.", ch, obj, NULL, TO_CHAR);
    }

} // end withering_enchant

/*
 * Spell that enchants a person, it adds hitroll and damroll to the person
 * based off of the users casting level.  The target will also gain a small
 * mana boost if the victim is not the caster as well as a small AC boost.
 */
void spell_enchant_person(int sn, int level, CHAR_DATA * ch, void *vo, int target)
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
            act("$N has already been enchanted.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    int value = UMAX(1, ch->level / 10);

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = APPLY_HITROLL;
    af.modifier = value;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    af.location = APPLY_DAMROLL;
    af.modifier = value;
    affect_to_char(victim, &af);

    af.modifier = -5;
    af.location = APPLY_AC;
    affect_to_char(victim, &af);

    send_to_char("You are surrounded with a light translucent {Bblue{x aura.\r\n", victim);
    act("$n is surrounded with a light translucent {Bblue{x aura.", victim, NULL, NULL, TO_ROOM);

    if (ch != victim)
    {
        // Small mana transfer from the caster to the target, but no more than their max mana.
        victim->mana += number_range(10, 20);

        if (victim->mana > victim->max_mana)
        {
            victim->mana = victim->max_mana;
        }
    }

} // end spell_enchant_person

/*
 * Enchanter spell that toggles whether an object is no locate or not.
 */
void spell_sequestor(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    separate_obj(obj);

    if (IS_OBJ_STAT(obj, ITEM_NOLOCATE))
    {
        act("$p turns translucent and then slowly returns to it's orginal form.", ch, obj, NULL, TO_CHAR);
        act("$p turns translucent and then slowly returns to it's orginal form.", ch, obj, NULL, TO_ROOM);
        REMOVE_BIT(obj->extra_flags, ITEM_NOLOCATE);
    }
    else
    {
        SET_BIT(obj->extra_flags, ITEM_NOLOCATE);
        act("$p turns translucent and then slowly returns to it's orginal form.", ch, obj, NULL, TO_CHAR);
        act("$p turns translucent and then slowly returns to it's orginal form.", ch, obj, NULL, TO_ROOM);
    }

} // end spell_sequestor

/*
 * Spell that makes it so the weapon or armor is only usable by people of the same alignment.
 * This could potentially make an object unusable if it's already set as ANTI something.
 */
void spell_interlace_spirit(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *)vo;
    char buf[MAX_STRING_LENGTH];

    if (obj->item_type != ITEM_WEAPON  && obj->item_type != ITEM_ARMOR)
    {
        send_to_char("You can only interlace your spirit into weapons or armor.\r\n", ch);
        return;
    }

    if (obj->wear_loc != -1)
    {
        send_to_char("You must be able to carry an item to interlace your spirit into it.\r\n", ch);
        return;
    }

    separate_obj(obj);

    if (IS_GOOD(ch))
    {
        SET_BIT(obj->extra_flags, ITEM_ANTI_EVIL);
        SET_BIT(obj->extra_flags, ITEM_ANTI_NEUTRAL);
    }
    else if (IS_NEUTRAL(ch))
    {
        SET_BIT(obj->extra_flags, ITEM_ANTI_GOOD);
        SET_BIT(obj->extra_flags, ITEM_ANTI_EVIL);
    }
    else
    {
        SET_BIT(obj->extra_flags, ITEM_ANTI_GOOD);
        SET_BIT(obj->extra_flags, ITEM_ANTI_NEUTRAL);
    }

    sprintf(buf, "%s has had %s's spirit interlaced into it.", obj->short_descr, ch->name);
    act(buf, ch, NULL, NULL, TO_ROOM);
    act(buf, ch, NULL, NULL, TO_CHAR);

    // If it's not unwieldable by all alignments send an additional message.
    if (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) &&
        IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) &&
        IS_OBJ_STAT(obj, ITEM_ANTI_EVIL))
    {
        sprintf(buf, "%s is surrounded by a dull gray aura that quickly dissipates.", obj->short_descr);
        act(buf, ch, NULL, NULL, TO_ROOM);
        act(buf, ch, NULL, NULL, TO_CHAR);
    }

} // end interlace_spirit

/*
 * Spell that allows an enchanter to wizard mark an item.  A wizard marked item can always
 * be located in the world with locate if it exists, the enchanter can also see (for a lot of mana)
 * at the location of the object if it's not a private place or clan hall, etc.
 */
void spell_wizard_mark(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *)vo;
    char buf[MAX_INPUT_LENGTH];

    if (obj->item_type != ITEM_WEAPON  && obj->item_type != ITEM_ARMOR)
    {
        send_to_char("You can only wizard mark weapons or armor.\r\n", ch);
        return;
    }

    if (obj->wizard_mark != NULL)
    {
        if (strstr(obj->wizard_mark, ch->name) != NULL)
        {
            send_to_char("This item already carries your wizard mark.\r\n", ch);
            return;
        }
    }

    separate_obj(obj);

    sprintf(buf, "%s", ch->name);
    free_string(obj->wizard_mark);
    obj->wizard_mark = str_dup(buf);
    act("You mark $p with your name.", ch, obj, NULL, TO_CHAR);

} // spell_wizard_mark

/*
 * Spell that allows an enchanter to enchant any type of gem and turn it into
 * a warpstone.
 */
void spell_enchant_gem(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *)vo;
    OBJ_DATA *obj_warpstone;
    char buf[MAX_STRING_LENGTH];

    if (obj->item_type != ITEM_GEM)
    {
        send_to_char("That item is not a gem.\r\n", ch);
        return;
    }

    if (obj->wear_loc != -1)
    {
        send_to_char("The gem must be carried to be enchanted.\r\n", ch);
        return;
    }

    // Create the warpstone, create the message while both objects exist, then take the gem and give the
    // warpstone to the player
    obj_warpstone = create_object(get_obj_index(OBJ_VNUM_WARPSTONE));
    sprintf(buf, "%s glows a bright {Mmagenta{x and changes into %s.", obj->short_descr, obj_warpstone->short_descr);

    separate_obj(obj);
    obj_from_char(obj);
    obj_to_char(obj_warpstone, ch);

    // Show the caster and the room what has happened
    act(buf, ch, NULL, NULL, TO_ROOM);
    act(buf, ch, NULL, NULL, TO_CHAR);

} // end spell_enchant_gem

/*
*  Spell that enchants a piece of armor.  Mages and certain mage classes will be able
*  to enchant but enchanters are far more skilled at it. This spell is the merc/rom
*  version slightly modified for enchanter bonuses.  This is modified from the base
*  Diku/Merc/Rom version.
*/
void spell_enchant_armor(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *)vo;
    AFFECT_DATA *paf;
    int result, fail;
    int ac_bonus, added;
    bool ac_found = FALSE;

    if (obj->item_type != ITEM_ARMOR)
    {
        send_to_char("That isn't an armor.\r\n", ch);
        return;
    }

    if (obj->wear_loc != -1)
    {
        send_to_char("The item must be carried to be enchanted.\r\n", ch);
        return;
    }

    // this means they have no bonus
    ac_bonus = 0;
    // base 25% chance of failure
    fail = 25;

    // Enchanters have a lower base chance of failure
    if (ch->class == ENCHANTER_CLASS)
    {
        fail = 5;
    }

    // find the bonuses
    separate_obj(obj);

    if (!obj->enchanted)
    {
        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
        {
            if (paf->location == APPLY_AC)
            {
                ac_bonus = paf->modifier;
                ac_found = TRUE;
                fail += 5 * (ac_bonus * ac_bonus);
            }
            else
            {
                // things get a little harder
                fail += 20;
            }
        }
    }

    for (paf = obj->affected; paf != NULL; paf = paf->next)
    {
        if (paf->location == APPLY_AC)
        {
            ac_bonus = paf->modifier;
            ac_found = TRUE;
            fail += 5 * (ac_bonus * ac_bonus);
        }
        else
        {
            // things get a little harder
            fail += 20;
        }
    }

    /* apply other modifiers */
    fail -= level;

    if (IS_OBJ_STAT(obj, ITEM_BLESS))
        fail -= 15;

    if (IS_OBJ_STAT(obj, ITEM_GLOW))
        fail -= 5;

    if (is_affected(ch, gsn_enchant_person))
        fail -= 2;

    fail = URANGE(5, fail, 85);

    result = number_percent();

    // Cap in case they get over -14AC
    for (paf = obj->affected; paf != NULL; paf = paf->next)
    {
        if (paf->location == APPLY_AC)
        {
            ac_bonus = paf->modifier;
            if (ac_bonus <= -14)
            {
                result = 0;
            }
        }
    }

    // Game owner may max enchant, the item will still reflect it was enchanted by
    // them.  Ideally this would be for special quest items.
    if (IS_IMMORTAL(ch) && ch->level == MAX_LEVEL)
    {
        fail = 0;
    }

    // the moment of truth
    if (result < (fail / 5))
    {
        // item destroyed
        act("$p flares blindingly... and evaporates!", ch, obj, NULL, TO_CHAR);
        act("$p flares blindingly... and evaporates!", ch, obj, NULL, TO_ROOM);
        extract_obj(obj);
        return;
    }

    if (result < (fail / 3))
    {
        // item disenchanted
        AFFECT_DATA *paf_next;

        act("$p glows brightly, then fades...oops.", ch, obj, NULL, TO_CHAR);
        act("$p glows brightly, then fades.", ch, obj, NULL, TO_ROOM);
        set_obj_enchanted(ch, obj, TRUE);

        // remove all affects
        for (paf = obj->affected; paf != NULL; paf = paf_next)
        {
            paf_next = paf->next;
            free_affect(paf);
        }
        obj->affected = NULL;

        // clear all flags
        obj->extra_flags = 0;
        return;
    }

    if (result <= fail)
    {
        // failed, no bad result
        send_to_char("Nothing seemed to happen.\r\n", ch);
        return;
    }

    // move all the old flags into new vectors if we have to
    if (!obj->enchanted)
    {
        AFFECT_DATA *af_new;
        obj->enchanted = TRUE;

        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
        {
            af_new = new_affect();

            af_new->next = obj->affected;
            obj->affected = af_new;

            af_new->where = paf->where;
            af_new->type = UMAX(0, paf->type);
            af_new->level = paf->level;
            af_new->duration = paf->duration;
            af_new->location = paf->location;
            af_new->modifier = paf->modifier;
            af_new->bitvector = paf->bitvector;
        }
    }

    // They have a success at this point, go ahead and set the enchanted_by
    set_obj_enchanted(ch, obj, FALSE);

    if (ch->class == ENCHANTER_CLASS && result <= (90 - level))
    {
        // Enchanters only have a chance for the highest enchant.
        act("$p glows a brilliant {Wwhite{x!", ch, obj, NULL, TO_CHAR);
        act("$p glows a brilliant {Wwhite{x!", ch, obj, NULL, TO_ROOM);
        SET_BIT(obj->extra_flags, ITEM_MAGIC);
        SET_BIT(obj->extra_flags, ITEM_GLOW);
        added = -3;
    }
    else if (result <= (90 - level / 5))
    {
        // success!
        act("$p shimmers with a gold aura.", ch, obj, NULL, TO_CHAR);
        act("$p shimmers with a gold aura.", ch, obj, NULL, TO_ROOM);
        SET_BIT(obj->extra_flags, ITEM_MAGIC);
        added = -1;
    }

    else
    {
        // exceptional enchant, highest a non enchanter can go.
        act("$p glows a brilliant gold!", ch, obj, NULL, TO_CHAR);
        act("$p glows a brilliant gold!", ch, obj, NULL, TO_ROOM);
        SET_BIT(obj->extra_flags, ITEM_MAGIC);
        SET_BIT(obj->extra_flags, ITEM_GLOW);
        added = -2;
    }

    // now, add the enchantments
    // level of the object will increase by one
    if (obj->level < LEVEL_HERO)
        obj->level = UMIN(LEVEL_HERO - 1, obj->level + 1);

    if (ac_found)
    {
        for (paf = obj->affected; paf != NULL; paf = paf->next)
        {
            if (paf->location == APPLY_AC)
            {
                paf->type = sn;
                paf->level = UMAX(paf->level, level);

                if (IS_IMMORTAL(ch) && ch->level == MAX_LEVEL)
                {
                    // Game owner max enchant
                    paf->modifier = -14;
                }
                else
                {
                    // Regular enchant
                    paf->modifier += added;
                }
            }
        }
    }
    else
    {                            /* add a new affect */

        paf = new_affect();

        paf->where = TO_OBJECT;
        paf->type = sn;
        paf->level = level;
        paf->duration = -1;
        paf->location = APPLY_AC;

        if (IS_IMMORTAL(ch) && ch->level == MAX_LEVEL)
        {
            // Game owner max enchant
            paf->modifier = -14;
        }
        else
        {
            // Regular enchant
            paf->modifier = added;
        }

        paf->bitvector = 0;
        paf->next = obj->affected;
        obj->affected = paf;
    }

} // end spell_enchant_armor

/*
* Spell that enchants a weapon increasing it's hit and dam roll.  Some classes like
* mages can enchant but  Enchanters specialize in this and get bonuses.  This spell
* is the merc/rom version slightly modified for enchanter bonuses.
*/
void spell_enchant_weapon(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *)vo;
    AFFECT_DATA *paf;
    int result, fail;
    int hit_bonus, dam_bonus, added;
    bool hit_found = FALSE, dam_found = FALSE;

    if (obj->item_type != ITEM_WEAPON)
    {
        send_to_char("That isn't a weapon.\r\n", ch);
        return;
    }

    if (obj->wear_loc != -1)
    {
        send_to_char("The item must be carried to be enchanted.\r\n", ch);
        return;
    }

    // this means they have no bonus
    hit_bonus = 0;
    dam_bonus = 0;

    // base 25% chance of failure
    fail = 25;

    // Enchanters have a lower base chance of failure
    if (ch->class == ENCHANTER_CLASS)
    {
        fail = 5;
    }

    // find the bonuses
    separate_obj(obj);

    if (!obj->enchanted)
    {
        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
        {
            if (paf->location == APPLY_HITROLL)
            {
                hit_bonus = paf->modifier;
                hit_found = TRUE;
                fail += 2 * (hit_bonus * hit_bonus);
            }

            else if (paf->location == APPLY_DAMROLL)
            {
                dam_bonus = paf->modifier;
                dam_found = TRUE;
                fail += 2 * (dam_bonus * dam_bonus);
            }
            else
            {
                // things get a little harder
                fail += 25;
            }
        }
    }

    for (paf = obj->affected; paf != NULL; paf = paf->next)
    {
        if (paf->location == APPLY_HITROLL)
        {
            hit_bonus = paf->modifier;
            hit_found = TRUE;
            fail += 2 * (hit_bonus * hit_bonus);
        }
        else if (paf->location == APPLY_DAMROLL)
        {
            dam_bonus = paf->modifier;
            dam_found = TRUE;
            fail += 2 * (dam_bonus * dam_bonus);
        }
        else
        {
            // things get a little harder
            fail += 25;
        }
    }

    // apply other modifiers
    fail -= 3 * level / 2;

    if (IS_OBJ_STAT(obj, ITEM_BLESS))
        fail -= 15;

    if (IS_OBJ_STAT(obj, ITEM_GLOW))
        fail -= 5;

    if (is_affected(ch, gsn_enchant_person))
        fail -= 2;

    fail = URANGE(5, fail, 95);
    result = number_percent();

    // Enchanting cap at 12 hit or 12 damage (which should be very rare)
    for (paf = obj->affected; paf != NULL; paf = paf->next)
    {
        if (paf->location == APPLY_HITROLL)
        {
            hit_bonus = paf->modifier;
            if (hit_bonus >= 12) { result = 0; }
        }
        else if (paf->location == APPLY_DAMROLL)
        {
            dam_bonus = paf->modifier;
            if (dam_bonus >= 12) { result = 0; }
        }
    }

    // Game owner may max enchant, the item will still reflect it was enchanted by
    // them.  Ideally this would be for special quest items.
    if (IS_IMMORTAL(ch) && ch->level == MAX_LEVEL)
    {
        fail = 0;
    }

    // the moment of truth
    if (result < (fail / 5))
    {
        // item destroyed
        act("$p shivers violently and explodes!", ch, obj, NULL, TO_CHAR);
        act("$p shivers violently and explodeds!", ch, obj, NULL, TO_ROOM);
        extract_obj(obj);
        return;
    }

    if (result < (fail / 2))
    {
        // item disenchant
        AFFECT_DATA *paf_next;

        act("$p glows brightly, then fades...oops.", ch, obj, NULL, TO_CHAR);
        act("$p glows brightly, then fades.", ch, obj, NULL, TO_ROOM);
        set_obj_enchanted(ch, obj, TRUE);

        // remove all affects
        for (paf = obj->affected; paf != NULL; paf = paf_next)
        {
            paf_next = paf->next;
            free_affect(paf);
        }
        obj->affected = NULL;

        // clear all flags
        obj->extra_flags = 0;

        if (IS_WEAPON_STAT(obj, WEAPON_VAMPIRIC))
            REMOVE_BIT(obj->value[4], WEAPON_VAMPIRIC);

        if (IS_WEAPON_STAT(obj, WEAPON_LEECH))
            REMOVE_BIT(obj->value[4], WEAPON_LEECH);

        if (IS_WEAPON_STAT(obj, WEAPON_POISON))
            REMOVE_BIT(obj->value[4], WEAPON_POISON);

        if (IS_WEAPON_STAT(obj, WEAPON_FLAMING))
              REMOVE_BIT(obj->value[4], WEAPON_FLAMING);

        if (IS_WEAPON_STAT(obj, WEAPON_FROST))
            REMOVE_BIT(obj->value[4], WEAPON_FROST);

        if (IS_WEAPON_STAT(obj, WEAPON_SHOCKING))
            REMOVE_BIT(obj->value[4], WEAPON_SHOCKING);

        return;
    }

    if (result <= fail)
    {
        // failed, no bad result
        send_to_char("Nothing seemed to happen.\r\n", ch);
        return;
    }

    // move all the old flags into new vectors if we have to
    if (!obj->enchanted)
    {
        AFFECT_DATA *af_new;
        obj->enchanted = TRUE;

        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
        {
            af_new = new_affect();

            af_new->next = obj->affected;
            obj->affected = af_new;

            af_new->where = paf->where;
            af_new->type = UMAX(0, paf->type);
            af_new->level = paf->level;
            af_new->duration = paf->duration;
            af_new->location = paf->location;
            af_new->modifier = paf->modifier;
            af_new->bitvector = paf->bitvector;
        }
    }

    // They have a success at this point, go ahead and set the enchanted_by
    set_obj_enchanted(ch, obj, FALSE);

    if (ch->class == ENCHANTER_CLASS && result >= (110 - level / 5))
    {
        // exceptional enchant for an enchanter
        act("$p glows a brilliant {Wwhite{x!", ch, obj, NULL, TO_CHAR);
        act("$p glows a brilliant {Wwhite{x!", ch, obj, NULL, TO_ROOM);
        SET_BIT(obj->extra_flags, ITEM_MAGIC);
        SET_BIT(obj->extra_flags, ITEM_GLOW);
        added = 3;
    }
    else if (result <= (100 - level / 5))
    {
        // success
        act("$p glows blue.", ch, obj, NULL, TO_CHAR);
        act("$p glows blue.", ch, obj, NULL, TO_ROOM);
        SET_BIT(obj->extra_flags, ITEM_MAGIC);
        added = 1;
    }
    else
    {
        // exceptional enchant
        act("$p glows a brillant blue!", ch, obj, NULL, TO_CHAR);
        act("$p glows a brillant blue!", ch, obj, NULL, TO_ROOM);
        SET_BIT(obj->extra_flags, ITEM_MAGIC);
        SET_BIT(obj->extra_flags, ITEM_GLOW);
        added = 2;
    }

    // add the enchantments
    if (obj->level < LEVEL_HERO - 1)
        obj->level = UMIN(LEVEL_HERO - 1, obj->level + 1);

    if (dam_found)
    {
        for (paf = obj->affected; paf != NULL; paf = paf->next)
        {
            if (paf->location == APPLY_DAMROLL)
            {
                paf->type = sn;

                if (IS_IMMORTAL(ch) && ch->level == MAX_LEVEL)
                {
                    // Game owner max enchant
                    paf->modifier = 12;
                }
                else
                {
                    // Regular enchant
                    paf->modifier += added;
                }

                paf->level = UMAX(paf->level, level);
                if (paf->modifier > 4)
                    SET_BIT(obj->extra_flags, ITEM_HUM);
            }
        }
    }
    else
    {
        // add a new affect
        paf = new_affect();
        paf->where = TO_OBJECT;
        paf->type = sn;
        paf->level = level;
        paf->duration = -1;
        paf->location = APPLY_DAMROLL;

        if (IS_IMMORTAL(ch) && ch->level == MAX_LEVEL)
        {
            // Game owner max enchant
            paf->modifier = 12;
        }
        else
        {
            // Regular enchant
            paf->modifier = added;
        }

        paf->bitvector = 0;
        paf->next = obj->affected;
        obj->affected = paf;
    }

    if (hit_found)
    {
        for (paf = obj->affected; paf != NULL; paf = paf->next)
        {
            if (paf->location == APPLY_HITROLL)
            {
                paf->type = sn;

                if (IS_IMMORTAL(ch) && ch->level == MAX_LEVEL)
                {
                    // Game owner max enchant
                    paf->modifier = 12;
                }
                else
                {
                    // Regular enchant
                    paf->modifier += added;
                }

                paf->level = UMAX(paf->level, level);
                if (paf->modifier > 4)
                    SET_BIT(obj->extra_flags, ITEM_HUM);
            }
        }
    }
    else
    {
        // add a new affect
        paf = new_affect();
        paf->type = sn;
        paf->level = level;
        paf->duration = -1;
        paf->location = APPLY_HITROLL;

        if (IS_IMMORTAL(ch) && ch->level == MAX_LEVEL)
        {
            // Game owner max enchant
            paf->modifier = 12;
        }
        else
        {
            // Regular enchant
            paf->modifier = added;
        }


        paf->bitvector = 0;
        paf->next = obj->affected;
        obj->affected = paf;
    }

} // end spell_enchant_weapon

/*
 * Spell that restores a faded, altered or enchanted weapon to it's original state.  This
 * will basically return it to it's stock value.
 */
void spell_restore_weapon(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *)vo;
    OBJ_DATA *obj2;
    char buf[MAX_STRING_LENGTH];
    int chance;

    if (obj->item_type != ITEM_WEAPON)
    {
        send_to_char("That isn't a weapon.\r\n", ch);
        return;
    }

    if (obj->wear_loc != -1)
    {
        send_to_char("The weapon must be carried to be restored.\r\n", ch);
        return;
    }

    // Default, 40% won't work (20% nothing happens, 20% crumble)
    chance = number_range(1, 10);

    // bless increase chances and lowers crumbling chance
    if (IS_OBJ_STAT(obj, ITEM_BLESS))
        chance += 1;

    // Contiual light increases chance and lowers crumbling chance
    if (IS_OBJ_STAT(obj, ITEM_GLOW))
        chance += 1;

    // If the caster is enchanted themselves, lowers crumbling chance.
    if (is_affected(ch, gsn_enchant_person))
        chance += 1;

    if (chance <= 2)
    {
        sprintf(buf, "%s crumbles into dust...", obj->short_descr);
        separate_obj(obj);
        extract_obj(obj);
        act(buf, ch, NULL, NULL, TO_ROOM);
        act(buf, ch, NULL, NULL, TO_CHAR);
        return;
    }
    else if (chance >= 3 && chance <= 4)
    {
        send_to_char("Nothing happened.\r\n", ch);
        return;
    }
    else
    {
        obj2 = create_object(get_obj_index(obj->pIndexData->vnum));
        obj_to_char(obj2, ch);
        separate_obj(obj);
        extract_obj(obj);
        sprintf(buf, "With a {Wbright{x flash of light, %s has been restored to it's original form.", obj2->short_descr);

        if (IS_OBJ_STAT(obj2, ITEM_VIS_DEATH))
            REMOVE_BIT(obj2->extra_flags, ITEM_VIS_DEATH);

        act(buf, ch, NULL, NULL, TO_ROOM);
        act(buf, ch, NULL, NULL, TO_CHAR);
        return;
    }

} // end spell_restore_weapon

/*
 * Spell that restores a faded, altered or enchanted piece or armor  to it's original state.
 * This will basically return it to it's stock value.
 */
void spell_restore_armor(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *)vo;
    OBJ_DATA *obj2;
    char buf[MAX_STRING_LENGTH];
    int chance;

    if (obj->item_type != ITEM_ARMOR)
    {
        send_to_char("That isn't armor.\r\n", ch);
        return;
    }

    if (obj->wear_loc != -1)
    {
        send_to_char("The armor must be carried to be restored.\r\n", ch);
        return;
    }

    // Default, 40% won't work (20% nothing happens, 20% crumble)
    chance = number_range(1, 10);

    // bless increase chances and lowers crumbling chance
    if (IS_OBJ_STAT(obj, ITEM_BLESS))
        chance += 1;

    // Contiual light increases chance and lowers crumbling chance
    if (IS_OBJ_STAT(obj, ITEM_GLOW))
        chance += 1;

    if (chance <= 2)
    {
        sprintf(buf, "%s crumbles into dust...", obj->short_descr);
        separate_obj(obj);
        extract_obj(obj);
        act(buf, ch, NULL, NULL, TO_ROOM);
        act(buf, ch, NULL, NULL, TO_CHAR);
        return;
    }
    else if (chance >= 3 && chance <= 4)
    {
        send_to_char("Nothing happened.\r\n", ch);
        return;
    }
    else
    {
        obj2 = create_object(get_obj_index(obj->pIndexData->vnum));
        obj_to_char(obj2, ch);
        separate_obj(obj);
        extract_obj(obj);
        sprintf(buf, "With a {Wbright{x flash of light, %s has been restored to it's original form.", obj2->short_descr);

        if (IS_OBJ_STAT(obj2, ITEM_VIS_DEATH))
            REMOVE_BIT(obj2->extra_flags, ITEM_VIS_DEATH);

        act(buf, ch, NULL, NULL, TO_ROOM);
        act(buf, ch, NULL, NULL, TO_CHAR);
        return;
    }

} // end spell_restore_armor

/*
 * Spell to disenchant a weapon or piece of armor.
 */
void spell_disenchant(int sn, int level, CHAR_DATA *ch, void *vo, int target) {
    OBJ_DATA *obj = (OBJ_DATA *)vo;
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;


    if (obj->item_type != ITEM_WEAPON && obj->item_type != ITEM_ARMOR)
    {
        send_to_char("That item is neither weapon or armor.\r\n", ch);
        return;
    }

    if (obj->wear_loc != -1)
    {
        send_to_char("The item must be carried to be disenchanted.\r\n", ch);
        return;
    }

    act("$p glows brightly, then fades to a dull color", ch, obj, NULL, TO_CHAR);
    act("$p glows brightly, then fades to a dull color.", ch, obj, NULL, TO_ROOM);
    separate_obj(obj);
    set_obj_enchanted(ch, obj, TRUE);

    /* remove all affects */
    for (paf = obj->affected; paf != NULL; paf = paf_next)
    {
        paf_next = paf->next;
        free_affect(paf);
    }

    obj->affected = NULL;

    /* clear all flags */
    obj->extra_flags = 0;

    if (IS_WEAPON_STAT(obj, WEAPON_VAMPIRIC))
        REMOVE_BIT(obj->value[4], WEAPON_VAMPIRIC);

    if (IS_WEAPON_STAT(obj, WEAPON_LEECH))
        REMOVE_BIT(obj->value[4], WEAPON_LEECH);

    if (IS_WEAPON_STAT(obj, WEAPON_POISON))
        REMOVE_BIT(obj->value[4], WEAPON_POISON);

    if (IS_WEAPON_STAT(obj, WEAPON_FLAMING))
        REMOVE_BIT(obj->value[4], WEAPON_FLAMING);

    if (IS_WEAPON_STAT(obj, WEAPON_FROST))
        REMOVE_BIT(obj->value[4], WEAPON_FROST);

    if (IS_WEAPON_STAT(obj, WEAPON_SHOCKING))
        REMOVE_BIT(obj->value[4], WEAPON_SHOCKING);

    return;

} // end spell_disenchant

  /*
  * This spell is a offshoot of locate object that just searches for Wizard Marks.  It
  * gives more detail about the objects found and has a significantly higher chance of
  * locating the objects.  This is an enchanter spell but it will be in the detection
  * group.
  */
void spell_locate_wizard_mark(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    char buf[MAX_INPUT_LENGTH];
    BUFFER *buffer;
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0, max_found;

    found = FALSE;
    number = 0;
    max_found = IS_IMMORTAL(ch) ? 200 : 2 * level;

    buffer = new_buf();

    for (obj = object_list; obj != NULL; obj = obj->next)
    {
        // The enchanter's name
        if (!is_name(ch->name, obj->wizard_mark))
            continue;

        found = TRUE;
        number++;

        for (in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj);

        if (in_obj->carried_by != NULL)
        {
            sprintf(buf, "%s is carried by %s\r\n", in_obj->short_descr, PERS(in_obj->carried_by, ch));
        }
        else
        {
            if (IS_IMMORTAL(ch) && in_obj->in_room != NULL)
            {
                sprintf(buf, "%s is in %s [Room %d]\r\n",
                    in_obj->short_descr, in_obj->in_room->name, in_obj->in_room->vnum);
            }
            else
            {
                sprintf(buf, "%s is in %s\r\n", in_obj->short_descr,
                    in_obj->in_room == NULL
                    ? "somewhere" : in_obj->in_room->name);
            }
        }

        buf[0] = UPPER(buf[0]);
        add_buf(buffer, buf);

        if (number >= max_found)
            break;
    }

    if (!found)
    {
        send_to_char("You found no wizard markings of your own in heaven or earth.\r\n", ch);
    }
    else
    {
        page_to_char(buf_string(buffer), ch);
    }

    free_buf(buffer);

    return;

}

/*
 * Waves of weariness will allow the enchanter to charm the victim into removing their
 * weapon (or weapons) before lulling them to sleep.
 */
void spell_waves_of_weariness(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    OBJ_DATA *wield;
    OBJ_DATA *dualwield;
    CHAR_DATA *victim;
    victim = (CHAR_DATA *)vo;

    // Make a saves check
    if (saves_spell(level, victim, DAM_OTHER))
    {
        send_to_char("The waves of weariness enchantment failed.\r\n", ch);
        return;
    }

    // Remove it, but send it to the inventory and not to the ground.
    if ((wield = get_eq_char(victim, WEAR_WIELD)) != NULL)
    {
        act("You remove $p.", victim, wield, NULL, TO_CHAR);
        act("$n removes $p.", victim, wield, NULL, TO_ROOM);
        obj_from_char(wield);
        obj_to_char(wield, victim);
    }

    // Remove the secondary weapon also
    if ((dualwield = get_eq_char(victim, WEAR_SECONDARY_WIELD)) != NULL)
    {
        act("You remove $p.", victim, dualwield, NULL, TO_CHAR);
        act("$n removes $p.", victim, dualwield, NULL, TO_ROOM);
        obj_from_char(dualwield);
        obj_to_char(dualwield, victim);
    }

    // Add the sleep affect with a random 1 or 2 tick duration
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = number_range(1, 2);
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_SLEEP;
    affect_join(victim, &af);

    // Zzzzzzzzz
    send_to_char("You feel weariness overtake your body...\r\n", victim);
    act("$n looks very weary and falls to the ground asleep.", victim, NULL, NULL, TO_ROOM);
    victim->position = POS_SLEEPING;

} // end spell_waves_of_weariness

/*
 * Sets an object as being enchanted by an enchanter and allows for the history
 * to be cleared (or kept).  This sets the obj->enchanted value and also the
 * obj->enchanted_by for history of the object.  This will not separate the object
 * currently, that should be done ahead of time.
 */
void set_obj_enchanted(CHAR_DATA * ch, OBJ_DATA * obj, bool clear_first)
{
    char buf[MAX_STRING_LENGTH];

    if (ch == NULL || obj == NULL)
    {
        return;
    }

    obj->enchanted = TRUE;

    if (!clear_first)
    {
        // Don't clear the list, check to see if they are already in it
        // before adding them
        if (!is_exact_name(ch->name, obj->enchanted_by))
        {
            if (IS_NULLSTR(obj->enchanted_by))
            {
                sprintf(buf, "%s", ch->name);
            }
            else
            {
                sprintf(buf, "%s %s", obj->enchanted_by, ch->name);
            }

            free_string(obj->enchanted_by);
            obj->enchanted_by = str_dup(buf);
        }
    }
    else
    {
        // Clear the list first, then add them in.
        sprintf(buf, "%s", ch->name);
        free_string(obj->enchanted_by);
        obj->enchanted_by = str_dup(buf);
    }

    return;

} // end set_obj_enchanted

/*
 * Preserves an item that is ready to rot/crumble.  It has a cool down period
 * of 160 minutes which means it can only be used on 1 item every 160 minutes
 * of play time (has mitigating rot death can be a huge deal for a rare item).
 */
void spell_preserve(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    OBJ_DATA *obj = (OBJ_DATA *) vo;

    if (is_affected(ch, sn))
    {
        send_to_char("You failed to cast preserve again so soon.\r\n", ch);
        return;
    }

    if (obj->timer < 1)
    {
        send_to_char("That item doesn't require preserving.\r\n", ch);
        return;
    }

    // 240 tick cool down period, e.g. it can't be used again for that long.
    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 240;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(ch, &af);

    // Any future checks that would mitigate this being used.
    /*if ()
    {
        send_to_char("You cannot preserve that item.\r\n", ch);
        return;
    }*/

    // Reset the timer..
    obj->timer = 0;

    act("$p fades out of existence and then back into visible form.", ch, obj, NULL, TO_CHAR);
    act("$p fades out of existence and then back into visible form.", ch, obj, NULL, TO_ROOM);

    return;
}

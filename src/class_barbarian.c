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
 *  Barbarians                                                             *
 *                                                                         *
 *  Barbarians are highly skilled fighters who are unable to learn the     *
 *  ways of magic (at all).  They typically have higher health and         *
 *  movement available to them and try to win victory with their sheer     *
 *  power.  This makes larger, dumber races much better barbarians.        *
 *                                                                         *
 *  Skills                                                                 *
 *                                                                         *
 *    - 4th attack                                                         *
 *    - Warcry                                                             *
 *    - Acute Vision                                                       *
 *    - Cleanse                                                            *
 *    - Natural Refresh                                                    *
 *    - Power swing                                                        *
 *    - Second wind                                                        *
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
#include "tables.h"

/*
 * Warcry, like berserk only better.
 */
void do_warcry(CHAR_DATA * ch, char *argument)
{
    int chance, hp_percent;

    if ((chance = get_skill(ch, gsn_warcry)) == 0
        || (IS_NPC(ch) && !IS_SET(ch->off_flags, OFF_BERSERK))
        || (!IS_NPC(ch) && ch->level < skill_table[gsn_warcry]->skill_level[ch->class]))
    {
        send_to_char("You turn red in the face, but nothing happens.\r\n", ch);
        return;
    }

    // We're going to re-use the berserk affect, no need to create a new one
    // when these mostly have the same behavior (warcry has higher stat output
    // for the person using it).
    if (IS_AFFECTED(ch, AFF_BERSERK)
        || is_affected(ch, gsn_berserk)
        || is_affected(ch, gsn_frenzy)
        || is_affected(ch, gsn_warcry))
    {
        send_to_char("You get a little madder.\r\n", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CALM))
    {
        send_to_char("You are far too calm to use your warcry.\r\n", ch);
        return;
    }

    if (ch->mana < 50)
    {
        send_to_char("You can't get up enough energy.\r\n", ch);
        return;
    }

    // Modifiers for chance
    // First off, sector type (better chances in some, worse in others)
    switch (ch->in_room->sector_type)
    {
        default:
            chance += 5;
            break;
        case SECT_INSIDE:
            chance -= 5;
            break;
        case SECT_CITY:
            chance -= 5;
            break;
        case SECT_WATER_SWIM:
        case SECT_WATER_NOSWIM:
        case SECT_UNDERWATER:
        case SECT_OCEAN:
            chance -= 15;
            break;
    }

    // Better chance while fighting
    if (ch->position == POS_FIGHTING)
    {
        chance += 15;
    }

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

        send_to_char("Your pulse races as you let out as massive WARCRY!\r\n", ch);
        act("$n lets out a massive WARCRY!", ch, NULL, NULL, TO_ROOM);
        check_improve(ch, gsn_warcry, TRUE, 3);

        af.where = TO_AFFECTS;
        af.type = gsn_warcry;
        af.level = ch->level;
        af.duration = number_fuzzy(ch->level / 6);
        af.modifier = UMAX(1, ch->level / 4);
        af.bitvector = AFF_BERSERK;

        af.location = APPLY_HITROLL;
        affect_to_char(ch, &af);

        af.location = APPLY_DAMROLL;
        affect_to_char(ch, &af);

        af.location = APPLY_DEX;
        af.modifier = -1;
        affect_to_char(ch, &af);

        af.location = APPLY_STR;
        af.modifier = 1;
        affect_to_char(ch, &af);
    }

    else
    {
        WAIT_STATE(ch, 3 * PULSE_VIOLENCE);
        ch->mana -= 25;
        ch->move = (ch->move * 4) / 5;

        send_to_char("Your attempt to muster a warcry but nothing happens.\r\n", ch);
        check_improve(ch, gsn_warcry, FALSE, 3);
    }
}

/*
 * Cleanse skill to allow a barbarian a chance to clear their eyes from physical debris (like
 * the much used dirt kick skill).
 */
void do_cleanse(CHAR_DATA * ch, char *argument)
{
    // Ditch out if they aren't affected by dirt kick (or other typess of future physical blinds)
    if (!is_affected(ch, gsn_dirt))
    {
        send_to_char("There is nothing physical in your eyes clouding your vision.\r\n", ch);
        return;
    }

    // Check if they have the skill
    if (get_skill(ch, gsn_cleanse) == 0
        || (!IS_NPC(ch) && ch->level < skill_table[gsn_cleanse]->skill_level[ch->class]))
    {
        send_to_char("You failed, perhaps you aren't skilled enough.\r\n", ch);
        return;
    }

    OBJ_DATA *obj;
    bool found = FALSE;

    // Look to see if they have a drink container in their inventory that has water
    // in it, we look for them becaause if they are blind, they won't be able to see it
    // themselves (perk of this skill).  They may have multiple drink contianers, find
    // anyone that has water (if the first doesn't or is empty, proceed to the next)
    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
        if (obj->item_type == ITEM_DRINK_CON
            && obj->value[1] > 0
            && obj->value[2] == 0)
        {
            // We found it, break out leaving the obj referenced as the drink container
            // we are going to use.
            found = TRUE;
            break;
        }
    }

    if (!found || obj == NULL)
    {
        send_to_char("You look in desperation for a container with water but find none.\r\n", ch);
        act("$n looks in desperation for a container with water but finds none.", ch, NULL, NULL, TO_ROOM);
        return;
    }

    // The skill check
    if (get_skill(ch, gsn_cleanse) > number_percent())
    {
        sendf(ch, "You rinse the debris from your eyes with water from %s.\r\n", obj->short_descr);
        act("$n rinses the debris from $s eyes with water from $p.", ch, obj, NULL, TO_ROOM);

        // Remove it, but don't send the message off.
        affect_strip(ch, gsn_dirt);

        // Check for improve
        check_improve(ch, gsn_cleanse, TRUE, 2);
    }
    else
    {
        send_to_char("You failed to rinse the debris from your eyes.\r\n", ch);
        act("$n attempts to rinse debris from $s eyes.es.", ch, NULL, NULL, TO_ROOM);

        check_improve(ch, gsn_cleanse, FALSE, 4);
    }

    WAIT_STATE(ch, skill_table[gsn_cleanse]->beats);

    return;
}

/*
 * Barbarian's power swing, taking a two handed weapon and pummelling people with it.
 */
void do_power_swing(CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int chance = 0;
    int dam = 0;
    int stun_chance = 0;

    if (IS_NPC(ch))
    {
        return;
    }

    if (!IS_NPC(ch) && ch->level < skill_table[gsn_power_swing]->skill_level[ch->class])
    {
        send_to_char("You better leave the powering swinging to the barbarians.\r\n", ch);
        return;
    }

    if ((victim = ch->fighting) == NULL)
    {
        send_to_char("You aren't fighting anyone.\r\n", ch);
        return;
    }

    obj = get_eq_char(ch, WEAR_WIELD);

    if (obj == NULL)
    {
        send_to_char("You cannot power swing without a primary weapon.\r\n", ch);
        WAIT_STATE(ch, skill_table[gsn_power_swing]->beats);
        return;
    }

    chance = get_skill(ch, gsn_power_swing);
    dam = number_range(ch->level, ch->level * 2);

    // Show to testers
    if (IS_TESTER(ch))
    {
        sendf(ch, "[Powerswing Chance {W%d{x]\r\n", chance);
    }

    // The moment of truth.
    if (CHANCE(chance))
    {
        // Success!
        damage(ch, victim, dam, gsn_power_swing, DAM_BASH, TRUE);
        check_improve(ch, gsn_power_swing, TRUE, 1);

        // 25% base stun chance.
        stun_chance = 25;

        // Factor in the standard modifier (blind, haste, slow, etc.).
        stun_chance += standard_modifier(ch, victim);

        // Object should not be null by here, two handed gets a bonus on the stun.
        if (IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS))
        {
            // Random 5% boost for a 2 handed weapon
            stun_chance += 5;
            return;
        }

        // 1% to 5% boost for warcry
        if (is_affected(ch, gsn_warcry))
        {
            stun_chance += number_range(1, 5);
        }

        // Show to testers
        if (IS_TESTER(ch))
        {
            sendf(ch, "[Powerswing Stun Chance {W%d{x]\r\n", stun_chance);
        }

        // Now, chance for stun.
        if (CHANCE(stun_chance))
        {
            act("$n's power swing knocks you backwards!", ch, NULL, victim, TO_VICT);
            act("Your power swing knocks $N backwards!", ch, NULL, victim, TO_CHAR);
            act("$n's power swing knocks $N backwards!", ch, NULL, victim, TO_NOTVICT);
            DAZE_STATE(victim, 2 * PULSE_VIOLENCE);
        }

    }
    else
    {
        // Failure
        damage(ch, victim, 0, gsn_power_swing, DAM_BASH, TRUE);
        check_improve(ch, gsn_power_swing, FALSE, 2);
    }

    // Check wanted, apply the lag, then we're done folks
    check_wanted(ch, victim);
    WAIT_STATE(ch, skill_table[gsn_power_swing]->beats);

    return;
}

/*
 * Second wind allows a barbarian a chance to get a second wind which heals
 * a percentage of their health once they are hurt.  This will process on
 * damage taken, it will have a cool down period before it takes affect again
 * and it will be passive in that the character cannot invoke it themselves.
 */
void second_wind(CHAR_DATA * ch)
{
    // Class check, ditch out, no need to process further if any of these are true.
    if (IS_NPC(ch)
        || ch->class != BARBARIAN_CLASS
        || get_skill(ch, gsn_second_wind) == 0
        || ch->level < skill_table[gsn_second_wind]->skill_level[ch->class])
    {
        return;
    }

    int health_percent = 100;

    // Get their health percentage
    if (ch->max_hit > 0)
    {
        health_percent = (100 * ch->hit) / ch->max_hit;
    }

    // Exits: Too much health or failed chance, also, not going to happen if
    // they have been slowed.
    if (health_percent > 20
        || !CHANCE_SKILL(ch, gsn_second_wind)
        || is_affected(ch, gsn_slow))
    {
        check_improve(ch, gsn_second_wind, FALSE, 1);
        return;
    }

    // We have gotten here.. they are successful, if they don't have second wind's
    // cool down then we'll credit some health and add the second wind affect.
    if (!is_affected(ch, gsn_second_wind))
    {
        send_to_char("You surge with a sudden burst of energy!\r\n", ch);
        act("$n surges with a sudden burst of energy!", ch, NULL, NULL, TO_ROOM);

        // Credit back 20% of their health.
        int hp = ch->max_hit * .2;
        ch->hit += hp;

        // Add the affect so this won't be processed again until it's gone, 50 ticks
        // is a little over a half hour real time.
        AFFECT_DATA af;

        af.where = TO_AFFECTS;
        af.type = gsn_second_wind;
        af.level = ch->level;
        af.duration = 50;
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.bitvector = 0;
        affect_to_char(ch, &af);

        check_improve(ch, gsn_second_wind, TRUE, 1);
    }

}

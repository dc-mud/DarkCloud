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
 *  Rangers                                                                *
 *                                                                         *
 *  Rangers are warriors who are associated with the wisdom of nature.     *
 *  They tend to be wise, cunning, and perceptive in addition to being     *
 *  skilled woodsmen. Many are skilled in stealth, wilderness survival,    *
 *  beast-mastery, herbalism, and tracking.  They are adept at swordplay   *
 *  and fight much better in a wilderness setting.  Rangers typically have *
 *  keen eye sight and are excellent at hiding in forest areas.            *
 *                                                                         *
 *  Skills                                                                 *
 *                                                                         *
 *    - Acute Vision                                                       *
 *    - Butcher                                                            *
 *    - Find Water                                                         *
 *    - Bark Skin                                                          *
 *    - Self Growth                                                        *
 *    - Quiet Movement                                                     *
 *    - Camp                                                               *
 *    - Camouflage                                                         *
 *    - Survey Terrain                                                     *
 *    - Track (hunt.c)                                                     *
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
 * Skill a ranger can use to butcher steaks from PC and NPC corpses.  Yum.
 * This will leverage the timer callback methods.
 */
void do_butcher(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;
    OBJ_DATA *steak;
    char buf[MAX_STRING_LENGTH];

    // What do they want to butcher?
    if (IS_NULLSTR(argument))
    {
        send_to_char("What do you want to butcher?\r\n", ch);
        return;
    }

    // Find it in the room
    obj = get_obj_list(ch, argument, ch->in_room->contents);

    if (obj == NULL)
    {
        send_to_char("You can't find it.\r\n", ch);
        return;
    }

    // Is it a corpse of some kind?
    if ((obj->item_type != ITEM_CORPSE_NPC) && (obj->item_type != ITEM_CORPSE_PC))
    {
        send_to_char("You can only butcher corpses.\r\n", ch );
        return;
    }

    // Instead of dumping items to the ground, we'll just make them clean the corpse first
    if (obj->contains)
    {
       send_to_char("You will need to remove all items from the corpse first.\r\n",ch);
       return;
    }

    switch (ch->substate)
    {
        default:
            add_timer(ch, TIMER_DO_FUN, number_range(12, 18), do_butcher, 1, argument);
            sprintf(buf, "You begin to butcher a steak from %s\r\n", obj->short_descr);
            send_to_char(buf, ch);
            act("$n begins to butcher a steak from $p.", ch, obj, NULL, TO_ROOM);
            return;
        case 1:
            break;
        case SUB_TIMER_DO_ABORT:
            ch->substate = SUB_NONE;
            send_to_char("You stop butchering.\r\n", ch);
            act("$n stops butchering.", ch, NULL, NULL, TO_ROOM);
            return;
    }

    ch->substate = SUB_NONE;

    // The moment of truth, do they fail and mutilate the corpse?
    if (!check_skill_improve(ch, gsn_butcher, 3, 3))
    {
        send_to_char("You fail your attempt to butcher the corpse.\r\n", ch);
        act( "$n has failed their attempt at butchering.", ch, NULL, NULL, TO_ROOM );
        separate_obj(obj);
        extract_obj(obj);
        return;
    }

    // Require that an object with a VNUM of 27 is created (as the steak), I didn't
    // feel the need to make a global constant when it's only used once here.
    int count = number_range(1, 4);
    int x = 0;

    for (x = 1; x <= count; x++)
    {
        steak = create_object(get_obj_index(OBJ_VNUM_STEAK));
        obj_to_char(steak, ch);
    }

    // Show the player and the room the spoils (not spoiled)
    sprintf(buf, "$n has prepared %d steak%s.", count, (count > 1 ? "s" : ""));
    act(buf, ch, NULL, NULL, TO_ROOM );
    sprintf(buf, "You have successfully prepared %d steak%s", count, (count > 1 ? "s" : ""));
    act(buf, ch, NULL, NULL, TO_CHAR );

    // Seprate and extract the corpse from the room.
    separate_obj(obj);
    extract_obj(obj);

    return;

} // end do_butcher

/*
 * The bandage skill will allow a ranger or healer to attempt to bandage up themselves
 * or another person/mob who is critically injured.  It will restore a small amount
 * of HP although it could cause a slight damage itself if done incorrectly.
 *
 * This skill orginated from from UltraEnvy/Magma Mud by Xangis.
 */
void do_bandage(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    int chance;
    int change;

    if (IS_NPC(ch) || ch->level < skill_table[gsn_bandage]->skill_level[ch->class])
    {
        send_to_char("You don't know how to bandage!\r\n", ch);
        return;
    }

    if (IS_NULLSTR(argument))
    {
        send_to_char( "Bandage whom?\r\n", ch );
        return;
    }

    if (!(victim = get_char_room(ch, argument)))
    {
        send_to_char("They're not here.\r\n", ch);
        return;
    }

    // The original implementation was for > 0 which meant only stunned people
    // could be bandaged, we'll allow this to be used on people under 100 to
    // make it a little more useful, those people are still pretty hurt.
    if (victim->hit > 100)
    {
        if (victim == ch)
        {
            send_to_char("You do not need any bandages currently.\r\n", ch);
            return;
        }
        else
        {
            send_to_char("They do not need your help.\r\n", ch);
            return;
        }
    }

    // Make sure they have enough movement to perform this, even though it takes
    // less than 50 we'll make them have more than 50.
    if (ch->move < 50)
    {
        send_to_char("You are too tired.\r\n", ch);
        return;
    }

    chance = get_skill(ch, gsn_bandage);

    // Ranger's and Healer's get a bonus
    switch (ch->class)
    {
        default:
            break;
        case RANGER_CLASS:
            chance += 4;
            break;
        case HEALER_CLASS:
            chance += 6;
            break;
    }

    /* Don't allow someone doing more than 1 pt. of damage with bandage. */
    change = (UMAX(chance - number_percent(), -1) / 20) + 1;

    // The wait period for the user of the skill.
    WAIT_STATE(ch, skill_table[gsn_bandage]->beats);

    if(change < 0)
    {
        send_to_char("You just made the problem worse!\r\n", ch);
        act("$n tries bandage you but your condition only worsens.", ch, NULL, victim, TO_VICT);
        act("$n tries bandage $N but $S condition only worsens.", ch, NULL, victim, TO_NOTVICT);
        check_improve(ch, gsn_bandage, FALSE, 1);
    }
    else if(change > 0)
    {
        send_to_char("You manage to fix them up a bit.\r\n", ch);
        act("$n bandages you.", ch, NULL, victim, TO_VICT);
        act("$n bandages $N.", ch, NULL, victim, TO_NOTVICT);
        check_improve(ch, gsn_bandage, TRUE, 1);
    }
    else
    {
        send_to_char("Your bandaging attempt had no effect.\r\n", ch);
        act("$n tries to bandage you but the wounds are too great.", ch, NULL, victim, TO_VICT);
        act("$n tries to bandage $N but is unable to have any effect.", ch, NULL, victim, TO_NOTVICT);
    }

    // Add the additional health
    victim->hit += change;

    // Have the act of bandaging remove some move from the character using it.
    ch->move -= change;

    // Update their position in case they were stunned.
    update_pos(victim);

    return;

} // end do_bandage

/*
 * Bark skin - enhances a characters armor class.
 */
void spell_bark_skin(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(ch, sn))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N's skin already has the toughness of bark.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = UMIN (UMAX(15, level), 50);
    af.location = APPLY_AC;
    af.modifier = -10;
    af.bitvector = 0;
    affect_to_char(victim, &af);
    act("$n's skin gains the texture and toughness of bark.", victim, NULL, NULL, TO_ROOM);
    send_to_char("Your skin gains the texture and toughness of bark.\r\n", victim);
    return;

} // end spell_bark_skin

/*
 * This spell allows the ranger to channel the powers of nature to promote growth.
 * This spell should be castable only on one's self.
 */
void spell_self_growth(int sn, int level, CHAR_DATA * ch, void *vo, int target)
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
            act("$N is already instilled with self growth.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = APPLY_CON;
    af.modifier = 1;
    af.bitvector = 0;
    affect_to_char(victim, &af);
    send_to_char("You feel vitalized.\r\n", victim);
    act("$n's looks more vitalized.", victim, NULL, NULL, TO_ROOM);
    return;

} // end self growth

/*
 * Quiet movement - Will allow rangers to move mostly undetected through
 * various types of wilderness terrain.  Will not work in cities, water,
 * underwater, etc.
 */
void do_quiet_movement(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;

    send_to_char("You attempt to move quietly.\r\n", ch );
    affect_strip(ch, gsn_quiet_movement);

    if (is_affected(ch, gsn_quiet_movement))
        return;

    if (CHANCE_SKILL(ch, gsn_quiet_movement))
    {
        af.where = TO_AFFECTS;
        af.type = gsn_quiet_movement;
        af.level = ch->level;
        af.duration = ch->level;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = 0;
        affect_to_char(ch, &af);

        check_improve(ch, gsn_quiet_movement, TRUE, 3);
    }
    else
    {
        check_improve(ch, gsn_quiet_movement, FALSE, 3);
    }

    return;
} // end do_quiet_movement

/*
 * Camping - allows a ranger to camp in certain wilderness type areas.  Will bring
 * increased regen to the ranger and anyone with them that is in their group while
 * camp is setup.  Camp will broken when a ranger leaves the room, breaks camp, is
 * attacked or attacks.
 */
void do_camp(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    CHAR_DATA *gch;
    OBJ_DATA *obj;
    char buf[MAX_STRING_LENGTH];

    // No NPC's
    if (IS_NPC(ch))
    {
        send_to_char("You cannot setup camp.\r\n", ch);
        return;
    }

    if (is_affected(ch, gsn_camping))
    {
        send_to_char("You're already in a camp, no reason to setup another.\r\n", ch);
        return;
    }

    // Only Rangers
    if (ch->class != RANGER_CLASS)
    {
        send_to_char("Only rangers are adept at setting up good camps.\r\n", ch);
        return;
    }

    if (is_affected(ch, gsn_camping))
    {
        send_to_char("You have already setup camp.\r\n", ch);
        return;
    }

    // Can't setup a camp while you're fighting.
    if (ch->fighting != NULL)
    {
        send_to_char("You can't setup camp while you're fighting.\r\n", ch);
        return;
    }

    // Are they the level yet?
    if (ch->level < skill_table[gsn_camping]->skill_level[ch->class])
    {
        send_to_char("You are not yet skilled enough to setup a good camp.\r\n", ch);
        return;
    }

    if (!CHANCE_SKILL(ch, gsn_camping))
    {
        send_to_char("You failed at setting up camp.\r\n", ch);
        check_improve(ch, gsn_camping, FALSE, 5);
        return;
    }

    switch (ch->in_room->sector_type)
    {
        default:
            // Success
            send_to_char("You setup camp.\r\n", ch);
            act("$n is sets up camp.", ch, NULL, NULL, TO_ROOM);
            break;
        case SECT_INSIDE:
            // Fail
            act("You are unable to setup camp inside.", ch, NULL, NULL, TO_CHAR);
            act("$n is unable to setup camp inside.", ch, NULL, NULL, TO_ROOM);
            return;
        case SECT_CITY:
            // Fail
            act("You are unable to setup camp within the city.", ch, NULL, NULL, TO_CHAR);
            act("$n is unable to setup camp within the city.", ch, NULL, NULL, TO_ROOM);
            return;
        case SECT_WATER_SWIM:
        case SECT_WATER_NOSWIM:
        case SECT_UNDERWATER:
        case SECT_OCEAN:
            // Fail
            act("You are unable to setup camp in the water.", ch, NULL, NULL, TO_CHAR);
            act("$n are unable to setup camp in the water.", ch, NULL, NULL, TO_ROOM);
            return;
        case SECT_AIR:
            // Fail
            act("In the air?!", ch, NULL, NULL, TO_CHAR);
            return;
        case SECT_FIELD:
            // Success
            act("You setup camp here in the field.", ch, NULL, NULL, TO_CHAR);
            act("$n sets up camp here in the field.", ch, NULL, NULL, TO_ROOM);
            break;
        case SECT_HILLS:
            // Success
            act("You setup camp here in the hills.", ch, NULL, NULL, TO_CHAR);
            act("$n sets up camp here in the hills.", ch, NULL, NULL, TO_ROOM);
            break;
        case SECT_DESERT:
            // Success
            act("You setup camp here in the desert.", ch, NULL, NULL, TO_CHAR);
            act("$n sets up camp here in the desert.", ch, NULL, NULL, TO_ROOM);
            break;
        case SECT_MOUNTAIN:
            // Success
            act("You setup camp here in the mountains.", ch, NULL, NULL, TO_CHAR);
            act("$n sets up camp here in the mountains.", ch, NULL, NULL, TO_ROOM);
            break;
        case SECT_FOREST:
            // Success
            act("You setup camp here under the canopy of the forest.", ch, NULL, NULL, TO_CHAR);
            act("$n sets up camp here under the canopy of the forest.", ch, NULL, NULL, TO_ROOM);
            break;
        case SECT_BEACH:
            // Success
            act("You setup camp here on the the beach.", ch, NULL, NULL, TO_CHAR);
            act("$n sets up camp here on the beach.", ch, NULL, NULL, TO_ROOM);
            break;
    }

    // Create the campfire, put it on the ground, set the timer so it will burn out.
    obj = create_object(get_obj_index(OBJ_VNUM_CAMPFIRE));
    obj->timer = ch->level / 3;
    obj_to_room(obj, ch->in_room);

    af.where = TO_AFFECTS;
    af.type = gsn_camping;
    af.level = ch->level;
    af.duration = UMAX(5, ch->level / 3);
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;

    // Find group members including the ranger who aren't NPC's
    for (gch = char_list; gch != NULL; gch = gch->next)
    {
        if (!IS_NPC(gch) && is_same_group(gch, ch))
        {
            affect_strip(gch, gsn_camping);
            affect_to_char(gch, &af);

            if (ch != gch)
            {
                sprintf(buf, "You join %s's camp.\r\n", ch->name);
                send_to_char(buf, gch);
            }
        }
    }

    check_improve(ch, gsn_camping, TRUE, 5);

    // The wait period for the user of the skill.
    WAIT_STATE(ch, skill_table[gsn_camping]->beats);

} // end do_camp

/*
 * Ranger skill similiar to hide.  Camouflage however will be required in order for
 * a ranger to use their ambush skill.  The tandem of these two skills have been seem in
 * many muds.  The idea for these came from Anatolia mud though the implementation did not.
 */
void do_camouflage(CHAR_DATA *ch, char *argument)
{
    AFFECT_DATA af;
    int chance;

    if (get_skill(ch, gsn_camouflage) == 0
        || IS_NPC(ch)
        || ch->level < skill_table[gsn_camouflage]->skill_level[ch->class])
    {
        send_to_char("You don't know how to camouflage yourself.\r\n", ch);
        return;
    }

    if (ch->in_room->sector_type != SECT_FOREST &&
        ch->in_room->sector_type != SECT_FIELD &&
        ch->in_room->sector_type != SECT_DESERT &&
        ch->in_room->sector_type != SECT_HILLS &&
        ch->in_room->sector_type != SECT_MOUNTAIN)
    {
        send_to_char("You cannot camoflage yourself here.\r\n", ch);
        return;
    }

    // Remove the affect if it exists
    affect_strip(ch, gsn_camouflage);

    // Calculate the chance, start with the base.
    chance = get_skill(ch, gsn_camouflage);

    // Faerie fire or blind cuts it in third
    if (IS_AFFECTED(ch, AFF_FAERIE_FIRE)
        || IS_AFFECTED(ch, AFF_BLIND))
    {
        chance /= 3;
    }

    // Dexterity, intelligence and wisdom increases chance
    chance += get_curr_stat(ch, STAT_DEX);
    chance += get_curr_stat(ch, STAT_INT) / 3;
    chance += get_curr_stat(ch, STAT_WIS) / 3;

    // There will always be some chance of success or failure.
    chance = URANGE(5, chance, 90);

    // The ranger will only know they attempted it.
    send_to_char("You attempt to camouflage yourself.\r\n", ch);

    if (CHANCE(chance))
    {
        // Success.
        af.where = TO_AFFECTS;
        af.type = gsn_camouflage;
        af.level = ch->level;
        af.duration = 2;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = 0;
        affect_to_char(ch, &af);

        check_improve(ch, gsn_camouflage, TRUE, 3);
    }
    else
    {
        // Failure, the people in the room will hear something if the ranger fails
        // to camouflage themselves.
        act("You hear something rustling near.", ch, NULL, NULL, TO_ROOM);
        check_improve(ch, gsn_camouflage, FALSE, 3);
    }

    return;

} // end do_camouflage

/*
 * Ranger's ambush skill.  This will allow a camouflaged ranger to ambush a victim
 * for a good one hit damage plus a chance at an additional init round.
 */
void do_ambush(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *weapon;

    // If they don't have ambush, not the level yet or they're an NPC exit out.
    if (get_skill(ch, gsn_ambush) == 0
        || IS_NPC(ch)
        || ch->level < skill_table[gsn_ambush]->skill_level[ch->class])
    {
        send_to_char("You don't have the skill to properly ambush someone.\r\n",ch);
        return;
    }

    // Can't be fighting someone and ambush.
    if( ch->position == POS_FIGHTING || ch->fighting != NULL)
    {
        send_to_char("You cannot ambush someone while you're fighting.\r\n", ch);
        return;
    }

    one_argument(argument,arg);

    // Figure out how it is they want to ambush, if it's no one of they can't see them
    // then get out.
    if (arg[0] == '\0')
    {
        send_to_char("Ambush who?\r\n", ch);
        return;
    }
    else if ((victim = get_char_room(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\r\n",ch);
        return;
    }

    // Can't ambush yourself
    if (victim == ch)
    {
        send_to_char("You can't ambush yourself.\r\n",ch);
        return;
    }

    // Check if the victim is safe from the attacker
    if (is_safe(ch, victim))
        return;

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
    {
        act("But $N is your friend!",ch,NULL,victim,TO_CHAR);
        return;
    }

    if (IS_NPC(victim) && victim->fighting != NULL && !is_same_group(ch, victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\r\n", ch);
        return;
    }

    if (!is_affected(ch, gsn_camouflage))
    {
        send_to_char("You must be camouflaged in order to ambush someone.\r\n",ch);
        return;
    }

    // Straight up skill check
    if (!CHANCE_SKILL(ch, gsn_ambush))
    {
        send_to_char("You fail your ambush attempt.\r\n", ch);
        act("You hear something rustling near.", ch, NULL, NULL, TO_ROOM);
        check_improve(ch, gsn_ambush, FALSE, 2);
        WAIT_STATE(ch,skill_table[gsn_ambush]->beats);
        return;
    }

    // Get their weapon
    weapon = get_eq_char(ch, WEAR_WIELD);

    // See if they are wanted or need a wanted flag.
    check_wanted(ch, victim);

    // Show the event
    act("You AMBUSH $N!", ch, NULL, victim, TO_CHAR);
    act("$n AMBUSHES you!!!", ch, NULL, victim, TO_VICT);
    act("$n AMBUSHES $N!!!", ch, NULL, victim, TO_NOTVICT);

    // Do the damage
    if (weapon == NULL)
    {
        damage(ch, victim, ((ch->level * 4) + GET_DAMROLL(ch, NULL)), gsn_ambush, DAM_BASH, TRUE);
    }
    else
    {
        damage(ch, victim, ((ch->level * 4) + GET_DAMROLL(ch, weapon)), gsn_ambush, weapon->value[3], TRUE);
    }

    // Check improve
    check_improve(ch, gsn_ambush, TRUE, 1);

    // Remove camouflage
    affect_strip(ch, gsn_camouflage);

    // Calculate whether they get a bonus init round
    // Dexterity, strength and a little luck
    int chance = get_curr_stat(ch, STAT_DEX) + get_curr_stat(ch, STAT_STR) + number_range(10, 25);
    chance += (get_skill(ch, gsn_ambush) / 4);

    // Give the victim a slight bonus if they have detect hidden
    if (IS_AFFECTED(victim, AFF_DETECT_HIDDEN))
        chance -= 10;

    // See if they get a bonus init round of damage.
    if (CHANCE(chance))
        multi_hit(ch, victim, TYPE_UNDEFINED);

    // Final lag for skill
    WAIT_STATE(ch,skill_table[gsn_ambush]->beats);

} // end do_ambush

/*
 * Find water skill that allows rangers to find a source of water while
 * in most outside areas (other than say, the desert).
 */
void do_find_water(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *spring;

    // Check if they have the skill
    if (get_skill(ch, gsn_find_water) == 0
        ||(!IS_NPC(ch)
        && ch->level < skill_table[gsn_find_water]->skill_level[ch->class]))
    {
        send_to_char("You search the area for water but find nothing.\r\n", ch);
        return;
    }

    // See if a spring already exists in the room.
    if (obj_in_room(ch, OBJ_VNUM_SPRING_2))
    {
        send_to_char("There is already a water source here.\r\n", ch);
        return;
    }

    // We'll check the failure cases and ditch out if one is found.
    switch (ch->in_room->sector_type)
    {
        default:
            break;
        case SECT_INSIDE:
            // Fail
            send_to_char("There is no water to be found inside.\r\n", ch);
            return;
        case SECT_CITY:
            // Fail
            send_to_char("There is no water to be found here.\r\n", ch);
            return;
        case SECT_DESERT:
            // Fail
            send_to_char("The desert is arid, there is no water to be found here.\r\n", ch);
            return;
        case SECT_WATER_SWIM:
        case SECT_WATER_NOSWIM:
        case SECT_UNDERWATER:
        case SECT_OCEAN:
            // Fail
            send_to_char("All the water around you and none to drink!\r\n", ch);
            return;
        case SECT_AIR:
            // Fail
            act("In the air?!", ch, NULL, NULL, TO_CHAR);
            return;
    }

    if (!CHANCE_SKILL(ch, gsn_find_water))
    {
        act("You search the area for water but find nothing.", ch, NULL, NULL, TO_CHAR);
        act("$n searches the area for water and finds nothing.", ch, NULL, NULL, TO_ROOM);

        check_improve(ch, gsn_find_water, FALSE, 5);
        return;
    }

    // Create the spring, set it to dry up after 5 ticks.
    spring = create_object(get_obj_index(OBJ_VNUM_SPRING_2));
    spring->timer = 5;
    obj_to_room(spring, ch->in_room);

    // Show the room
    act("You search the area and find $p!", ch, spring, NULL, TO_CHAR);
    act("$n searches the area and finds $p!", ch, spring, NULL, TO_ROOM);

    // Skill check to see if player improves
    check_improve(ch, gsn_find_water, TRUE, 3);

    // Short lag
    WAIT_STATE(ch, skill_table[gsn_find_water]->beats);

    return;
}

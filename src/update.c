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
 * Update                                                                  *
 ***************************************************************************
 *                                                                         *
 * This module is intended to update various in game mechanics and         *
 * structures on a schedule or assist in those updates.                    *
 *                                                                         *
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
#include <string.h>
#include "merc.h"
#include "interp.h"

/*
 * Local functions.
 */
int  hit_gain           (CHAR_DATA * ch);
int  mana_gain          (CHAR_DATA * ch);
int  move_gain          (CHAR_DATA * ch);
void check_death        (CHAR_DATA *victim, int dt);
void mobile_update      (void);
void weather_update     (void);
void char_update        (void);
void obj_update         (void);
void aggr_update        (void);
void tick_update        (void);
void half_tick_update   (void);
void shore_update       (void);
void environment_update (void);
void improves_update    (void);

/* used for saving */
int save_number = 0;

/*
 * Handle all kinds of updates, called once per pulse from the game loop.
 * This will call the various update handlers that occur at different intervals.
 */
void update_handler(bool forced)
{
    static int pulse_area;      // 120 seconds
    static int pulse_mobile;    //   4 seconds
    static int pulse_violence;  //   3 seconds
    static int pulse_tick;      //  40 seconds
    static int pulse_half_tick; //  20 seconds
    static int pulse_minute;    //  60 seconds

    if (--pulse_area <= 0)
    {
        pulse_area = PULSE_AREA;
        area_update();
    }

    if (--pulse_mobile <= 0)
    {
        pulse_mobile = PULSE_MOBILE;
        mobile_update();
    }

    /*
     * We're going to put the environment after the violence update to seperate them but
     * they will run on the same schedule for now (which is about every 3 seconds).  The
     * environment update will be used for light-weight but quick happening checks like
     * losing movement while treading water in the ocean.  The violence_update handles the
     * rounds of battle.
     */
    if (--pulse_violence <= 0)
    {
        pulse_violence = PULSE_VIOLENCE;
        violence_update();
        environment_update();
    }

    /*
     * This happens every half tick which is currently about 20 seconds.  The shore update
     * to show the waves hitting the beaches for characters next to the ocean are all that's
     * currently happening here.
     */
    if (--pulse_half_tick <= 0)
    {
        pulse_half_tick = PULSE_HALF_TICK;
        half_tick_update();
        shore_update();
    }

    /*
     * Every minute...
     */
    if (--pulse_minute <= 0)
    {
        pulse_minute = PULSE_MINUTE;

        // Save pits and player corpses that might be laying around, consider saving
        // player corposes very frequently but pits on a lesser schedule.
        save_game_objects();

        // Save the game statistics.  This can probably be done on a lesser schedule.  Consider
        // making a longer term method, something that occurs every 10 minutes or so.  We'll start
        // here though as it's writing out very litte.
        save_statistics();

        // This will process the skills people are focusing on to improve.  This must run
        // once a minute, it will decriment the minutes left on the pcdata and adjust upward
        // any skill that has improved (then reset the timer).
        improves_update();

        // Flush pending database operations that have been buffered.  Nothing will be executed
        // if there is nothing in the buffer.
        flush_db();
    }

    // Just firing the tick, not messing with violence, mobiles or areas.
    if (forced)
    {
        pulse_tick = 0;
    }

    /*
     * The tick update is the big event where lots of updates occur.  ROM traditionally runs
     * ticks at random times around a minute, we're going to do one every 40 seconds (as defined
     * in merc.h).
     */
    if (--pulse_tick <= 0)
    {
        wiznet("TICK!", NULL, NULL, WIZ_TICKS, 0, 0);
        pulse_tick = PULSE_TICK;
        quest_update();
        weather_update();
        char_update();
        obj_update();
        tick_update();
    }

    aggr_update();
    tail_chain();
    return;
}


/*
 * Health Regeneration stuff.
 */
int hit_gain(CHAR_DATA * ch)
{
    int gain;
    int number;

    if (ch->in_room == NULL)
    {
        bugf("hit_gain: ch->in_room was null (ch = %s)", ch->name);
        return 0;
    }

    if (IS_NPC(ch))
    {
        gain = 5 + ch->level;
        if (IS_AFFECTED(ch, AFF_REGENERATION))
            gain *= 2;

        switch (ch->position)
        {
            default:
                gain /= 2;
                break;
            case POS_SLEEPING:
                gain = 3 * gain / 2;
                break;
            case POS_RESTING:
                break;
            case POS_FIGHTING:
                gain /= 3;
                break;
        }
    }
    else
    {
        gain = UMAX(3, get_curr_stat(ch, STAT_CON) - 3 + ch->level / 2);
        gain += class_table[ch->class]->hp_max - 10;
        number = number_percent();
        if (number < get_skill(ch, gsn_fast_healing))
        {
            gain += number * gain / 100;
            if (ch->hit < ch->max_hit)
                check_improve(ch, gsn_fast_healing, TRUE, 8);
        }

        switch (ch->position)
        {
            default:
                // Awake and standing
                gain /= 4;
                break;
            case POS_SLEEPING:
                // Maximum regen, sleeping
                break;
            case POS_SITTING:
                // Sitting, better than standing, worse than resting.
                gain /= 3;
                break;
            case POS_RESTING:
                // Better than standing and sitting, worse than sleeping
                gain /= 2;
                break;
            case POS_FIGHTING:
                // Fighting, ack, less regen!
                gain /= 6;
                break;
        }

        if (ch->pcdata->condition[COND_HUNGER] == 0)
            gain /= 2;

        if (ch->pcdata->condition[COND_THIRST] == 0)
            gain /= 2;

        // No HP regen for players in the ocean
        if (ch->in_room->sector_type == SECT_OCEAN)
            gain = 0;

    }

    gain = gain * ch->in_room->heal_rate / 100;

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
        gain = gain * ch->on->value[3] / 100;

    if (IS_AFFECTED(ch, AFF_POISON))
        gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(ch, AFF_HASTE) || IS_AFFECTED(ch, AFF_SLOW))
        gain /= 2;

    // Healer's enhanced recovery, gains occur after other affect splits
    // happen.
    if (is_affected(ch, gsn_enhanced_recovery))
    {
        gain += number_range(20, 50);
    }

    // Healing dream, only can be on those who are sleeping and disappears
    // once they wake.
    if (is_affected(ch, gsn_healing_dream))
    {
        gain += number_range(40, 70);
    }

    // Merit - Healthy
    if (!IS_NPC(ch) && IS_SET(ch->pcdata->merit, MERIT_HEALTHY))
    {
        gain += number_range(15, 25);
    }

    // Rangers camping - regen bonus is greater at night than during
    // during the day.
    if (is_affected(ch, gsn_camping))
    {
        if (IS_NIGHT())
        {
            gain += number_range(20, 40);
        }
        else
        {
            gain += number_range(15, 25);
        }
    }

    return UMIN(gain, ch->max_hit - ch->hit);

} // end int hit_gain

/*
 * Mana Regenration
 */
int mana_gain(CHAR_DATA * ch)
{
    int gain;
    int number;

    if (ch->in_room == NULL)
    {
        bugf("mana_gain: ch->in_room was null (ch = %s)", ch->name);
        return 0;
    }

    if (IS_NPC(ch))
    {
        gain = 5 + ch->level;
        switch (ch->position)
        {
            default:
                gain /= 2;
                break;
            case POS_SLEEPING:
                gain = 3 * gain / 2;
                break;
            case POS_RESTING:
                break;
            case POS_FIGHTING:
                gain /= 3;
                break;
        }
    }
    else
    {
        gain = (get_curr_stat(ch, STAT_WIS) + get_curr_stat(ch, STAT_INT) + ch->level) / 2;
        number = number_percent();

        if (number < get_skill(ch, gsn_meditation))
        {
            gain += number * gain / 100;
            if (ch->mana < ch->max_mana)
                check_improve(ch, gsn_meditation, TRUE, 8);
        }

        if (!class_table[ch->class]->mana)
        {
            gain /= 2;
        }

        switch (ch->position)
        {
            default:
                gain /= 4;
                break;
            case POS_SLEEPING:
                break;
            case POS_RESTING:
                gain /= 2;
                break;
            case POS_FIGHTING:
                gain /= 6;
                break;
        }

        if (ch->pcdata->condition[COND_HUNGER] == 0)
            gain /= 2;

        if (ch->pcdata->condition[COND_THIRST] == 0)
            gain /= 2;

        if (CHANCE_SKILL(ch, gsn_spellcraft))
            gain += number_range(5, 10);

        // No mana gain for players in the ocean
        if (ch->in_room->sector_type == SECT_OCEAN)
            gain = 0;

    }

    gain = gain * ch->in_room->mana_rate / 100;

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
        gain = gain * ch->on->value[4] / 100;

    if (IS_AFFECTED(ch, AFF_POISON))
        gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(ch, AFF_HASTE) || IS_AFFECTED(ch, AFF_SLOW))
        gain /= 2;

    // Healer's enhanced recovery, gains happen after other splits occur
    if (is_affected(ch, gsn_enhanced_recovery))
    {
        gain += number_range(20, 50);
    }

    // Healing dream, only can be on those who are sleeping and disappears
    // once they wake.
    if (is_affected(ch, gsn_healing_dream))
    {
        gain += number_range(40, 70);
    }

    // Rangers camping - regen bonus is greater at night than during
    // during the day.
    if (is_affected(ch, gsn_camping))
    {
        if (IS_NIGHT())
        {
            gain += number_range(20, 40);
        }
        else
        {
            gain += number_range(15, 25);
        }
    }

    return UMIN(gain, ch->max_mana - ch->mana);

} // end int mana_gain

/*
 * Move regeneration
 */
int move_gain(CHAR_DATA * ch)
{
    int gain;

    if (ch->in_room == NULL)
    {
        bugf("hit_gain: ch->in_room was null (ch = %s)", ch->name);
        return 0;
    }

    if (IS_NPC(ch))
    {
        gain = ch->level;
    }
    else
    {
        gain = UMAX(15, ch->level);

        switch (ch->position)
        {
            case POS_SLEEPING:
                gain += get_curr_stat(ch, STAT_DEX);
                break;
            case POS_RESTING:
                gain += get_curr_stat(ch, STAT_DEX) / 2;
                break;
        }

        if (ch->pcdata->condition[COND_HUNGER] == 0)
            gain /= 2;

        if (ch->pcdata->condition[COND_THIRST] == 0)
            gain /= 2;

        // No gain for players in the ocean
        if (ch->in_room->sector_type == SECT_OCEAN)
            gain = 0;
    }

    gain = gain * ch->in_room->heal_rate / 100;

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
        gain = gain * ch->on->value[3] / 100;

    if (IS_AFFECTED(ch, AFF_POISON))
        gain /= 4;

    if (IS_AFFECTED(ch, AFF_PLAGUE))
        gain /= 8;

    if (IS_AFFECTED(ch, AFF_HASTE) || IS_AFFECTED(ch, AFF_SLOW))
        gain /= 2;

    // Healer's enhanced recovery, gains happen after other splits occur.
    if (is_affected(ch, gsn_enhanced_recovery))
    {
        gain += number_range(20, 50);
    }

    // Healing dream, only can be on those who are sleeping and disappears
    // once they wake.
    if (is_affected(ch, gsn_healing_dream))
    {
        gain += number_range(40, 70);
    }

    // Rangers camping - regen bonus is greater at night than during
    // during the day.
    if (is_affected(ch, gsn_camping))
    {
        if (IS_NIGHT())
        {
            gain += number_range(20, 40);
        }
        else
        {
            gain += number_range(15, 25);
        }
    }

    // Barbarians, natural refresh skill gives a boost in movement regen.
    if (number_percent() < get_skill(ch, gsn_natural_refresh))
    {
        gain += ch->level / 2;

        if (ch->move < ch->max_move)
        {
            check_improve(ch, gsn_natural_refresh, TRUE, 6);
        }
    }

    return UMIN(gain, ch->max_move - ch->move);

} // end int move_gain

/*
 * Adjusts the various conditions (hunger, thirst, drunkeness
 */
void gain_condition(CHAR_DATA * ch, int iCond, int value)
{
    int condition;

    if (value == 0 || IS_NPC(ch) || ch->level >= LEVEL_IMMORTAL)
        return;

    // If a player is reclassing they won't see their condition messages
    // and it also won't degrade.
    if (ch->pcdata->is_reclassing == TRUE)
        return;

    condition = ch->pcdata->condition[iCond];
    if (condition == -1)
        return;
    ch->pcdata->condition[iCond] = URANGE(0, condition + value, 48);

    if (ch->pcdata->condition[iCond] == 0)
    {
        switch (iCond)
        {
            case COND_HUNGER:
                send_to_char("You are hungry.\r\n", ch);
                break;

            case COND_THIRST:
                send_to_char("You are thirsty.\r\n", ch);
                break;

            case COND_DRUNK:
                if (condition != 0)
                    send_to_char("You are sober.\r\n", ch);
                break;
        }
    }

    return;
} // end void gain_condition

/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Merc cpu time.
 * -- Furey
 */
void mobile_update(void)
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    EXIT_DATA *pexit;
    int door;

    /* Examine all mobs. */
    for (ch = char_list; ch != NULL; ch = ch_next)
    {
        ch_next = ch->next;

        if (!IS_NPC(ch) || ch->in_room == NULL
            || IS_AFFECTED(ch, AFF_CHARM)) continue;

        if (ch->in_room->area->empty && !IS_SET(ch->act, ACT_UPDATE_ALWAYS))
            continue;

        /* Examine call for special procedure */
        if (ch->spec_fun != 0)
        {
            if ((*ch->spec_fun) (ch))
                continue;
        }

        if (ch->pIndexData->pShop != NULL)
        {
            /* give him some gold */
            if ((ch->gold * 100 + ch->silver) < ch->pIndexData->wealth)
            {
                ch->gold += ch->pIndexData->wealth * number_range(1, 20) / 5000000;
                ch->silver += ch->pIndexData->wealth * number_range(1, 20) / 50000;
            }
        }

        // Timers for mobiles
        if (IS_NPC(ch) && !ch->desc)
        {
            timer_update(ch);
        }

        /*
         * Check triggers only if mobile still in default position
         */
        if (ch->position == ch->pIndexData->default_pos)
        {
            /* Delay */
            if (HAS_TRIGGER(ch, TRIG_DELAY) && ch->mprog_delay > 0)
            {
                if (--ch->mprog_delay <= 0)
                {
                    mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_DELAY);
                    continue;
                }
            }
            if (HAS_TRIGGER(ch, TRIG_RANDOM))
            {
                if (mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_RANDOM))
                    continue;
            }
        }

        /* That's all for sleeping / busy monster, and empty zones */
        if (ch->position != POS_STANDING)
            continue;

        /* Scavenge */
        if (IS_SET(ch->act, ACT_SCAVENGER)
            && ch->in_room->contents != NULL && number_bits(6) == 0)
        {
            OBJ_DATA *obj;
            OBJ_DATA *obj_best;
            int max;

            max = 1;
            obj_best = 0;
            for (obj = ch->in_room->contents; obj; obj = obj->next_content)
            {
                if (CAN_WEAR(obj, ITEM_TAKE)
                    && !IS_OBJ_STAT(obj, ITEM_BURIED)
                    && can_loot(ch, obj)
                    && obj->cost > max && obj->cost > 0)
                {
                    obj_best = obj;
                    max = obj->cost;
                }
            }

            if (obj_best)
            {
                obj_from_room(obj_best);
                obj_to_char(obj_best, ch);
                act("$n gets $p.", ch, obj_best, NULL, TO_ROOM);
            }
        }

        /* Wander */
        if (!IS_SET(ch->act, ACT_SENTINEL)
            && number_bits(3) == 0
            && (door = number_bits(5)) <= 5
            && (pexit = ch->in_room->exit[door]) != NULL
            && pexit->u1.to_room != NULL
            && !IS_SET(pexit->exit_info, EX_CLOSED)
            && !IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)
            && (!IS_SET(ch->act, ACT_STAY_AREA)
                || pexit->u1.to_room->area == ch->in_room->area)
            && (!IS_SET(ch->act, ACT_OUTDOORS)
                || !IS_SET(pexit->u1.to_room->room_flags, ROOM_INDOORS))
            && (!IS_SET(ch->act, ACT_INDOORS)
                || IS_SET(pexit->u1.to_room->room_flags, ROOM_INDOORS)))
        {
            move_char(ch, door, FALSE);
        }
    }

    return;
}

/*
 * Update the weather.
 */
void weather_update(void)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    int diff;

    buf[0] = '\0';

    switch (++time_info.hour)
    {
        case 5:
            weather_info.sunlight = SUN_LIGHT;
            strcat(buf, "The day has begun.\r\n");
            break;

        case 6:
            weather_info.sunlight = SUN_RISE;
            strcat(buf, "The sun rises in the east.\r\n");
            break;

        case 19:
            weather_info.sunlight = SUN_SET;
            strcat(buf, "The sun slowly disappears in the west.\r\n");
            break;

        case 20:
            weather_info.sunlight = SUN_DARK;
            strcat(buf, "The night has begun.\r\n");
            break;

        case 24:
            time_info.hour = 0;
            time_info.day++;
            break;
    }

    if (time_info.day >= 35)
    {
        time_info.day = 0;
        time_info.month++;
    }

    if (time_info.month >= 17)
    {
        time_info.month = 0;
        time_info.year++;
    }

    /*
     * Weather change.
     */
    if (time_info.month >= 9 && time_info.month <= 16)
        diff = weather_info.mmhg > 985 ? -2 : 2;
    else
        diff = weather_info.mmhg > 1015 ? -2 : 2;

    weather_info.change += diff * dice(1, 4) + dice(2, 6) - dice(2, 6);
    weather_info.change = UMAX(weather_info.change, -12);
    weather_info.change = UMIN(weather_info.change, 12);

    weather_info.mmhg += weather_info.change;
    weather_info.mmhg = UMAX(weather_info.mmhg, 960);
    weather_info.mmhg = UMIN(weather_info.mmhg, 1040);

    switch (weather_info.sky)
    {
        default:
            bug("Weather_update: bad sky %d.", weather_info.sky);
            weather_info.sky = SKY_CLOUDLESS;
            break;

        case SKY_CLOUDLESS:
            if (weather_info.mmhg < 990
                || (weather_info.mmhg < 1010 && number_bits(2) == 0))
            {
                strcat(buf, "The sky is getting cloudy.\r\n");
                weather_info.sky = SKY_CLOUDY;
            }
            break;

        case SKY_CLOUDY:
            if (weather_info.mmhg < 970
                || (weather_info.mmhg < 990 && number_bits(2) == 0))
            {
                strcat(buf, "It starts to rain.\r\n");
                weather_info.sky = SKY_RAINING;
            }

            if (weather_info.mmhg > 1030 && number_bits(2) == 0)
            {
                strcat(buf, "The clouds disappear.\r\n");
                weather_info.sky = SKY_CLOUDLESS;
            }
            break;

        case SKY_RAINING:
            if (weather_info.mmhg < 970 && number_bits(2) == 0)
            {
                strcat(buf, "Lightning flashes in the sky.\r\n");
                weather_info.sky = SKY_LIGHTNING;
            }

            if (weather_info.mmhg > 1030
                || (weather_info.mmhg > 1010 && number_bits(2) == 0))
            {
                strcat(buf, "The rain stopped.\r\n");
                weather_info.sky = SKY_CLOUDY;
            }
            break;

        case SKY_LIGHTNING:
            if (weather_info.mmhg > 1010
                || (weather_info.mmhg > 990 && number_bits(2) == 0))
            {
                strcat(buf, "The lightning has stopped.\r\n");
                weather_info.sky = SKY_RAINING;
                break;
            }
            break;
    }

    if (buf[0] != '\0')
    {
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->connected == CON_PLAYING
                && IS_OUTSIDE(d->character)
                && IS_AWAKE(d->character)
                && d->character->in_room->sector_type != SECT_UNDERGROUND
                && d->character->in_room->sector_type != SECT_UNDERWATER)
            {
                send_to_char(buf, d->character);
            }
        }
    }

    return;
}

/*
 * Update all chars, including mobs.
 */
void char_update(void)
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *ch_quit;

    ch_quit = NULL;

    /* update save counter */
    save_number++;

    if (save_number > 29)
        save_number = 0;

    for (ch = char_list; ch != NULL; ch = ch_next)
    {
        AFFECT_DATA *paf;
        AFFECT_DATA *paf_next;

        ch_next = ch->next;

        if (ch->timer > 30)
        {
            ch_quit = ch;
        }

        // Player Kill Timer - This will make it so the players involved in pk have to wait
        // a few ticks after in order to quit.
        if (!IS_NPC(ch))
        {
            if (IS_IMMORTAL(ch))
            {
                ch->pcdata->pk_timer = 0;
            }

            // This will be reduced to none... tell them their hostile timer is over.
            if (ch->pcdata->pk_timer == 1)
            {
                send_to_char("You are no longer hostile.\r\n", ch);
            }

            if (ch->pcdata->pk_timer)
            {
                --ch->pcdata->pk_timer;
            }

        }

        // Things that need to happen to players only.
        if (!IS_NPC(ch))
        {
            // If their a priest, calculate the priest rank (all needed checks are done in this procedure).
            calculate_priest_rank(ch);

            // Add any merit affects on (or back on) if they aren't on the player (checks done in this procedure).
            apply_merit_affects(ch);

            // Apply any keep affects
            apply_keep_affects_to_char(ch);

            // Reset racial affected by's if they have been removed by something like a dispel (e.g. dark vision).
            ch->affected_by = ch->affected_by | race_table[ch->race].aff;
        }

        if (IS_AWAKE(ch))
        {
            // Remove healing dream if they are no longer asleep
            affect_strip(ch, gsn_healing_dream);
        }

        if (ch->position >= POS_STUNNED)
        {
            /* check to see if we need to go home */
            if (IS_NPC(ch) && ch->zone != NULL
                && ch->zone != ch->in_room->area && ch->desc == NULL
                && ch->fighting == NULL && !IS_AFFECTED(ch, AFF_CHARM)
                && number_percent() < 5)
            {
                act("$n wanders on home.", ch, NULL, NULL, TO_ROOM);
                extract_char(ch, TRUE);
                continue;
            }

            if (ch->hit < ch->max_hit)
                ch->hit += hit_gain(ch);
            else
                ch->hit = ch->max_hit;

            if (ch->mana < ch->max_mana)
                ch->mana += mana_gain(ch);
            else
                ch->mana = ch->max_mana;

            if (ch->move < ch->max_move)
                ch->move += move_gain(ch);
            else
                ch->move = ch->max_move;
        }

        if (ch->position == POS_STUNNED)
            update_pos(ch);

        if (!IS_NPC(ch) && ch->level < LEVEL_IMMORTAL)
        {
            OBJ_DATA *obj;

            if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL
                && obj->item_type == ITEM_LIGHT && obj->value[2] > 0)
            {
                if (--obj->value[2] == 0 && ch->in_room != NULL)
                {
                    --ch->in_room->light;
                    act("$p goes out.", ch, obj, NULL, TO_ROOM);
                    act("$p flickers and goes out.", ch, obj, NULL, TO_CHAR);
                    extract_obj(obj);
                }
                else if (obj->value[2] <= 5 && ch->in_room != NULL)
                    act("$p flickers.", ch, obj, NULL, TO_CHAR);
            }

            // Immortals and testers won't disappear into the void
            if (IS_IMMORTAL(ch) || IS_TESTER(ch))
                ch->timer = 0;

            if (++ch->timer >= 12)
            {
                if (ch->was_in_room == NULL && ch->in_room != NULL)
                {
                    ch->was_in_room = ch->in_room;

                    if (ch->fighting != NULL)
                    {
                        stop_fighting(ch, TRUE);
                    }

                    act("$n disappears into the void.", ch, NULL, NULL, TO_ROOM);
                    send_to_char("You disappear into the void.\r\n", ch);

                    if (ch->level > 1)
                    {
                        save_char_obj(ch);
                    }

                    char_from_room(ch);
                    char_to_room(ch, get_room_index(ROOM_VNUM_LIMBO));
                }
            }

            gain_condition(ch, COND_DRUNK, -1);
            gain_condition(ch, COND_FULL, ch->size > SIZE_MEDIUM ? -4 : -2);
            gain_condition(ch, COND_THIRST, -1);
            gain_condition(ch, COND_HUNGER, ch->size > SIZE_MEDIUM ? -2 : -1);
        }

        // Process affects for this tick, this is decrimenting their duration and
        // removing them if the spell has faded.
        for (paf = ch->affected; paf != NULL; paf = paf_next)
        {
            paf_next = paf->next;

            if (paf->duration > 0)
            {
                // Lower the duration by 1
                paf->duration--;

                // Spell strength fades with time, there is a 20% chance the
                // level of the spell will lower in power over time, for long
                // lasting spells this could be significant.  Casters can always
                // refresh a spell to mitigate this
                if (number_range(0, 4) == 0 && paf->level > 0)
                {
                    paf->level--;
                }
            }
            else if (paf->duration < 0);  // Permanent
            else
            {
                // Remove the affect and process any messages (and execute code
                // that might be required as a result of this affect being removed)
                if (paf_next == NULL
                    || paf_next->type != paf->type || paf_next->duration > 0)
                {
                    if (paf->type > 0 && skill_table[paf->type]->msg_off)
                    {
                        send_to_char(skill_table[paf->type]->msg_off, ch);
                        send_to_char("\r\n", ch);
                    }
                }

                // Processing for things that need to happen when an affect goes
                // away.. put these things in their own functions to keep this section
                // of code compact and easy to read.
                if (paf->type == gsn_blink)
                {
                    // blink check but the affect won't be removed as per the FALSE.  This
                    // will be removed just below (as opposed to when blink_check is called
                    // from interpret.
                    blink_check(ch, FALSE);
                }

                affect_remove(ch, paf);
            }
        }

        /*
         * Careful with the damages here,
         *   MUST NOT refer to ch after damage taken,
         *   as it may be lethal damage (on NPC).
         */

        if (is_affected(ch, gsn_plague) && ch != NULL)
        {
            AFFECT_DATA *af, plague;
            CHAR_DATA *vch;
            int dam;

            if (ch->in_room == NULL)
                continue;

            act("$n writhes in agony as plague sores erupt from $s skin.",
                ch, NULL, NULL, TO_ROOM);
            send_to_char("You writhe in agony from the plague.\r\n", ch);
            for (af = ch->affected; af != NULL; af = af->next)
            {
                if (af->type == gsn_plague)
                    break;
            }

            if (af == NULL)
            {
                REMOVE_BIT(ch->affected_by, AFF_PLAGUE);
                continue;
            }

            if (af->level == 1)
                continue;

            plague.where = TO_AFFECTS;
            plague.type = gsn_plague;
            plague.level = af->level - 1;
            plague.duration = number_range(1, 2 * plague.level);
            plague.location = APPLY_STR;
            plague.modifier = -5;
            plague.bitvector = AFF_PLAGUE;

            for (vch = ch->in_room->people; vch != NULL;
            vch = vch->next_in_room)
            {
                if (!saves_spell(plague.level - 2, vch, DAM_DISEASE)
                    && !IS_IMMORTAL(vch)
                    && !IS_AFFECTED(vch, AFF_PLAGUE) && number_bits(4) == 0)
                {
                    send_to_char("You feel hot and feverish.\r\n", vch);
                    act("$n shivers and looks very ill.", vch, NULL, NULL,
                        TO_ROOM);
                    affect_join(vch, &plague);
                }
            }

            dam = UMIN(ch->level, af->level / 5 + 1);
            ch->mana -= dam;
            ch->move -= dam;
            damage(ch, ch, dam, gsn_plague, DAM_DISEASE, FALSE);
        }
        else if (IS_AFFECTED(ch, AFF_POISON) && ch != NULL
            && !IS_AFFECTED(ch, AFF_SLOW))
        {
            AFFECT_DATA *poison;

            poison = affect_find(ch->affected, gsn_poison);

            if (poison != NULL)
            {
                act("$n shivers and suffers.", ch, NULL, NULL, TO_ROOM);
                send_to_char("You shiver and suffer.\r\n", ch);
                damage(ch, ch, poison->level / 10 + 1, gsn_poison,
                    DAM_POISON, FALSE);
            }
        }

        else if (ch->position == POS_INCAP && number_range(0, 1) == 0)
        {
            damage(ch, ch, 1, TYPE_UNDEFINED, DAM_NONE, FALSE);
        }
        else if (ch->position == POS_MORTAL)
        {
            damage(ch, ch, 1, TYPE_UNDEFINED, DAM_NONE, FALSE);
        }

    }

    /*
     * Autosave and autoquit.
     * Check that these chars still exist.
     */
    for (ch = char_list; ch != NULL; ch = ch_next)
    {
        /*
         * Edwin's fix for possible pet-induced problem
         * JR -- 10/15/00
         */
        if (!IS_VALID(ch))
        {
            bug("update_char: Trying to work with an invalidated character.\n", 0);
            break;
        }

        ch_next = ch->next;

        if (ch->desc != NULL && ch->desc->descriptor % 30 == save_number)
        {
            save_char_obj(ch);
        }

        if (ch == ch_quit || (!IS_NPC(ch) && !ch->desc && IS_SET(ch->act,PLR_AUTOQUIT)))
        {
            do_function(ch, &do_quit, "");
        }
    }

    return;
}

/*
 * Update all objs.
 * This function is performance sensitive.
 */
void obj_update(void)
{
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    AFFECT_DATA *paf, *paf_next;

    for (obj = object_list; obj != NULL; obj = obj_next)
    {
        CHAR_DATA *rch;
        char *message;

        obj_next = obj->next;

        /* go through affects and decrement */
        for (paf = obj->affected; paf != NULL; paf = paf_next)
        {
            paf_next = paf->next;
            if (paf->duration > 0)
            {
                paf->duration--;
                if (number_range(0, 4) == 0 && paf->level > 0)
                    paf->level--;    /* spell strength fades with time */
            }
            else if (paf->duration < 0);
            else
            {
                if (paf_next == NULL
                    || paf_next->type != paf->type || paf_next->duration > 0)
                {
                    if (paf->type > 0 && skill_table[paf->type]->msg_obj)
                    {
                        if (obj->carried_by != NULL)
                        {
                            rch = obj->carried_by;
                            act(skill_table[paf->type]->msg_obj,
                                rch, obj, NULL, TO_CHAR);
                        }
                        if (obj->in_room != NULL
                            && obj->in_room->people != NULL)
                        {
                            rch = obj->in_room->people;
                            act(skill_table[paf->type]->msg_obj,
                                rch, obj, NULL, TO_ALL);
                        }
                    }
                }

                affect_remove_obj(obj, paf);
            }
        }

        // Seed processing.
        if (obj->item_type == ITEM_SEED && IS_OBJ_STAT(obj, ITEM_BURIED))
        {
            seed_grow_check(obj);

            // Check to see if the seed exists anymore after it was grown, if it doesn't
            // skip the rest of the loop.
            if (obj == NULL)
            {
                continue;
            }
        }

        // This skipos out if the timer isn't 0... this means the code that runs below this runs
        // and then the item goes *poof*.  Code that processes an item for something needs to go
        // above here.
        if (obj->timer <= 0 || --obj->timer > 0)
            continue;

        switch (obj->item_type)
        {
            default:
                message = "$p crumbles into dust.";
                break;
            case ITEM_FOUNTAIN:
                message = "$p dries up.";
                break;
            case ITEM_CORPSE_NPC:
                message = "$p decays into dust.";
                break;
            case ITEM_CORPSE_PC:
                message = "$p decays into dust.";
                break;
            case ITEM_FOOD:
                message = "$p decomposes.";
                break;
            case ITEM_POTION:
                message = "$p has evaporated from disuse.";
                break;
            case ITEM_PORTAL:
                message = "$p fades out of existence.";
                break;
            case ITEM_FOG:
                message = "$p begins to dissipate.";
                break;
            case ITEM_CONTAINER:
                if (CAN_WEAR(obj, ITEM_WEAR_FLOAT))
                {
                    if (obj->contains)
                    {
                        message = "$p flickers and vanishes, spilling its contents on the floor.";
                    }
                    else
                    {
                        message = "$p flickers and vanishes.";
                    }
                }
                else
                {
                    message = "$p crumbles into dust.";
                }
                break;
            case ITEM_TRASH:
                // Override type message for specific well know vnums that need unique messages.
                if (obj->pIndexData->vnum == OBJ_VNUM_CAMPFIRE)
                {
                    message = "$p stops smoldering and burns out.";
                }
                else
                {
                    message = "$p crumbles into dust.";
                }
                break;
        }

        if (obj->carried_by != NULL)
        {
            if (IS_NPC(obj->carried_by)
                && obj->carried_by->pIndexData->pShop != NULL)
                obj->carried_by->silver += obj->cost / 5;
            else
            {
                act(message, obj->carried_by, obj, NULL, TO_CHAR);
                if (obj->wear_loc == WEAR_FLOAT)
                    act(message, obj->carried_by, obj, NULL, TO_ROOM);
            }
        }
        else if (obj->in_room != NULL && (rch = obj->in_room->people) != NULL)
        {
            if (!(obj->in_obj && obj->in_obj->pIndexData->vnum == OBJ_VNUM_PIT
                && !CAN_WEAR(obj->in_obj, ITEM_TAKE)))
            {
                act(message, rch, obj, NULL, TO_ROOM);
                act(message, rch, obj, NULL, TO_CHAR);
            }
        }

        if ((obj->item_type == ITEM_CORPSE_PC || obj->wear_loc == WEAR_FLOAT)
            && obj->contains)
        {                        /* save the contents */
            OBJ_DATA *t_obj, *next_obj;

            for (t_obj = obj->contains; t_obj != NULL; t_obj = next_obj)
            {
                next_obj = t_obj->next_content;
                obj_from_obj(t_obj);

                if (obj->in_obj)    /* in another object */
                    obj_to_obj(t_obj, obj->in_obj);

                else if (obj->carried_by)    /* carried */
                    if (obj->wear_loc == WEAR_FLOAT)
                        if (obj->carried_by->in_room == NULL)
                            extract_obj(t_obj);
                        else
                            obj_to_room(t_obj, obj->carried_by->in_room);
                    else
                        obj_to_char(t_obj, obj->carried_by);

                else if (obj->in_room == NULL)    /* destroy it */
                    extract_obj(t_obj);

                else            /* to a room */
                    obj_to_room(t_obj, obj->in_room);
            }
        }

        extract_obj(obj);
    }

    return;
}

/*
 * Aggress.
 *
 * for each mortal PC
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function takes 25% to 35% of ALL Merc cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 * -- Furey
 */
void aggr_update(void)
{
    CHAR_DATA *wch;
    CHAR_DATA *wch_next;
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    CHAR_DATA *victim;

    for (wch = char_list; wch != NULL; wch = wch_next)
    {
        wch_next = wch->next;
        if (IS_NPC(wch)
            || wch->level >= LEVEL_IMMORTAL
            || wch->in_room == NULL || wch->in_room->area->empty) continue;

        for (ch = wch->in_room->people; ch != NULL; ch = ch_next)
        {
            int count;

            ch_next = ch->next_in_room;

            if (!IS_NPC(ch)
                || !IS_SET(ch->act, ACT_AGGRESSIVE)
                || IS_SET(ch->in_room->room_flags, ROOM_SAFE)
                || IS_AFFECTED(ch, AFF_CALM)
                || ch->fighting != NULL || IS_AFFECTED(ch, AFF_CHARM)
                || !IS_AWAKE(ch)
                || (IS_SET(ch->act, ACT_WIMPY) && IS_AWAKE(wch))
                || !can_see(ch, wch) || number_bits(1) == 0)
                continue;

            /*
             * Ok we have a 'wch' player character and a 'ch' npc aggressor.
             * Now make the aggressor fight a RANDOM pc victim in the room,
             *   giving each 'vch' an equal chance of selection.
             */
            count = 0;
            victim = NULL;
            for (vch = wch->in_room->people; vch != NULL; vch = vch_next)
            {
                vch_next = vch->next_in_room;

                if (!IS_NPC(vch)
                    && vch->level < LEVEL_IMMORTAL
                    && ch->level >= vch->level - 5
                    && (!IS_SET(ch->act, ACT_WIMPY) || !IS_AWAKE(vch))
                    && can_see(ch, vch))
                {
                    if (number_range(0, count) == 0)
                        victim = vch;
                    count++;
                }
            }

            if (victim == NULL)
                continue;

            multi_hit(ch, victim, TYPE_UNDEFINED);
        }
    }

    return;
}

/*
 *  Arbitrary code that needs to execute on every tick.  Will occur at the end of processing
 *  of each tick (e.g. weather, char and obj, etc. updates will come first).
 */
void tick_update()
{
    char buf[MSL];

    // If a copyover is happening make the countdown and execute it when it hits 0.
    if (global.is_copyover == TRUE)
    {
        global.copyover_timer = global.copyover_timer - 1;

        if (global.copyover_timer <= 0)
        {
            global.is_copyover = FALSE;

            // If the copyover person is still active and not null execute the copyover,
            // otherwise cancel it.
            if (global.copyover_ch != NULL)
            {
                do_function(global.copyover_ch, &do_copyover, "now");
            }
            else
            {
                sprintf(buf, "\r\n{RWARNING{x: auto-{R{*copyover{x cancelled.\r\n");
                send_to_all_char(buf);
            }

        }
        else
        {
            // If a reason was specified then show it with each tick.
            char reason[MAX_STRING_LENGTH];

            if (!IS_NULLSTR(global.copyover_reason))
            {
                sprintf(reason, "\r\n{WReason{x: %s\r\n", global.copyover_reason);
            }
            else
            {
                sprintf(reason, "\r\n");
            }

            // Show the current countdown to the game.
            if (global.copyover_ch != NULL)
            {
                sprintf(buf, "\r\n{RWARNING{x: auto-{R{*copyover{x by {B%s{x will occur in {B%d{x tick%s.%s",
                    global.copyover_ch->name, global.copyover_timer, global.copyover_timer > 1 ? "s" : "", reason);
            }
            else
            {
                sprintf(buf, "\r\n{RWARNING{x: auto-{R{*copyover{x will occur in {B%d{x tick%s.%s",
                    global.copyover_timer, global.copyover_timer > 1 ? "s" : "", reason);
            }

            send_to_all_char(buf);
        }

    } // end copyover

    // Send line feeds to anyone who has the comm bit set for it.
    linefeed_update();
}

/*
 * An update that occurs ever half tick (about 20 seconds).
 */
void half_tick_update()
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;

    for (ch = char_list; ch != NULL; ch = ch_next)
    {
        ch_next = ch->next;

        // Healing presence will heal the 10 hp every 20 seconds.  This is an actual
        // heal (100hp) spread out over 5 full ticks.
        if (is_affected(ch, gsn_vitalizing_presence))
        {
            ch->hit = UMIN(ch->hit + 10, ch->max_hit);
            update_pos(ch);

            // Only show the healing message if they're actually healing
            if (ch->hit < ch->max_hit)
            {
                send_to_char("You feel a healing presence throughout your body\r\n", ch);
            }

        }
    }

} // end half_tick_update

/*
 * Whether or not a character is standing on the shore as defined by having an adjacent
 * room being flagged as being of type SECT_OCEAN.
 */
bool on_shore(CHAR_DATA *ch)
{
    ROOM_INDEX_DATA *scan_room;
    EXIT_DATA *pExit;
    int door;

    if (!ch || !ch->in_room)
    {
        return FALSE;
    }

    if (ch->in_room->sector_type == SECT_OCEAN)
    {
        return FALSE;
    }

    for (door = 0; door < MAX_DIR; door++)
    {
        scan_room = ch->in_room;

        if ((pExit = scan_room->exit[door]) != NULL)
        {
            scan_room = pExit->u1.to_room;
        }

        if (scan_room->sector_type == SECT_OCEAN)
        {
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * Updates for characters on the shore which are players that are next to an ocean room.
 */
void shore_update()
{
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];

    // This really should come from some kind of weather system we should implement.  It's random for now which
    // I don't love, but at least they can hear the ocean waves when they're on the shore.
    switch (number_range(0, 2))
    {
        case 0:
            sprintf(buf, "{cThe waves from the ocean gently roll onto the shore that surrounds you.{x\r\n");
            break;
        case 1:
            sprintf(buf, "{cWaves gently break as they reach the shore and a light breeze from the{x\r\n{cocean flows in.{x\r\n");
            break;
        case 2:
            sprintf(buf, "{cWaves crash against the shore as a moderate breeze blows in.{x\r\n");
            break;
    }

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING
            && d->character
            && IS_OUTSIDE(d->character)
            && IS_AWAKE(d->character)
            && d->character->in_room
            && on_shore(d->character))
        {
            send_to_char(buf, d->character);
        }
    }

} // end short_update()

/*
 * Environment update will fire on the same timing as violence update which is about every
 * three seconds.  We'll handle light weight fast moving actions here, like losing movement
 * from treading water for quick but short affect updates from spells.
 */
void environment_update()
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;

    for (ch = char_list; ch != NULL; ch = ch_next)
    {
        ch_next = ch->next;

        // Ocean - Immortals are immune to drowning and losing movement in the ocean.
        if (!IS_NPC(ch) &&
            ch->in_room != NULL  &&
            ch->in_room->sector_type == SECT_OCEAN &&
            ch->desc != NULL &&
            !IS_IMMORTAL(ch))
        {

            // They are under 0 HP and floating in the ocean, we will need to push them
            // closer to death.
            if (ch->hit < 0)
            {
                ch->hit -= 5;
                check_death(ch, DAM_DROWNING);
            }

            // Water breathing won't help their movement loss but it will keep them from
            // drowning.
            if (ch->move <= 0 && !is_affected(ch, gsn_water_breathing))
            {
                send_to_char("You gasp for air as water fills your lungs!\r\n", ch);
                ch->hit -= (UMAX(1, (ch->hit / 6)));
                check_death(ch, DAM_DROWNING);
            }
            else
            {
                // Everyone loses movement treading water in the ocean.  Consider adding
                // swim skill to lessen this, also maybe make stronger people lose less.
                // Priest's who are affected by water walking are exempt.
                if (!is_affected(ch, gsn_water_walk))
                {
                    send_to_char("You struggle to tread water in the ocean.\r\n", ch);
                    ch->move -= 10;
                }

                // Don't allow movement to go below 0
                if (ch->move < 0)
                {
                    ch->move = 0;
                }
            }
        } // end ocean logic

        // Underwater - Slightly different logic than the ocean, here they must have water
        // breathing or they will start sucking in water.
        if (!IS_NPC(ch) &&
            ch->in_room != NULL  &&
            ch->in_room->sector_type == SECT_UNDERWATER &&
            ch->desc != NULL &&
            !IS_IMMORTAL(ch))
        {
            if (!is_affected(ch, gsn_water_breathing))
            {
                ch->move -= 10;

                // Don't allow movement to go below 0
                if (ch->move < 0)
                {
                    ch->move = 0;
                }

                send_to_char("You gasp for air as water fills your lungs!\r\n", ch);
                ch->hit -= (UMAX(1, (ch->hit / 6)));
                check_death(ch, DAM_DROWNING);
            }

        } // end underwater logic

    } // end for

} // end environment_update

/*
 * Updates any outstanding timer and keeps them rolling.
 */
void timer_update(CHAR_DATA *ch)
{
    TIMER *timer, *timer_next;

    for (timer = ch->first_timer; timer; timer = timer_next)
    {
        timer_next = timer->next;

        if (--timer->count <= 0)
        {
            // Check the timer type, then do something with it
            if (timer->type == TIMER_DO_FUN)
            {
                int tempsub;
                tempsub = ch->substate;
                ch->substate = timer->value;
                (timer->do_fun)(ch, timer->cmd);
                ch->substate = tempsub;
            }

            // Now that we've done something with it, clean it up.
            extract_timer(ch, timer);
        }
    }
} // end timer_update

/*
 * Loops through only the players in the game and sees if they have improved
 * on the skill they are focused on.
 */
void improves_update()
{
    DESCRIPTOR_DATA *d;

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING
            && d->character
            && d->character->in_room)
        {
            check_time_improve(d->character);
        }
    }
}

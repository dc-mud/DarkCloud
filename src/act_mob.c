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

/**************************************************************************
*   This file is for any functions that might be coded as mob specific    *
*   functions but aren't mob progs.  E.g. the healer, specialized mobs    *
*   that do something at a players request                                *
**************************************************************************/

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
#include <stdlib.h>
#include "merc.h"
#include "magic.h"
#include "tables.h"

/*
 * Determines whether a mob with the specific ACT flag is in the room with the specified character
 * and if so returns that mob (otherwise, a NULL is returned).
 */
CHAR_DATA *find_mob_by_act(CHAR_DATA * ch, long act_flag)
{
    CHAR_DATA *mob;

    if (ch == NULL)
    {
        return NULL;
    }

    // Check for portal merchant
    for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
    {
        if (mob != NULL && IS_NPC(mob) && IS_SET(mob->act, act_flag))
        {
            return mob;
        }
    }

    return NULL;

} // end find_mob_by_act


/*
 * Command for a healer mob that can sell spell
 */
void do_heal(CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *mob;
    char arg[MAX_INPUT_LENGTH];
    int cost, sn;
    SPELL_FUN *spell;
    char *words;

    /* check for healer */
    for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
    {
        if (IS_NPC(mob) && IS_SET(mob->act, ACT_IS_HEALER))
            break;
    }

    if (mob == NULL)
    {
        send_to_char("You can't do that here.\r\n", ch);
        return;
    }

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        /* display price list */
        act("$N says 'I offer the following spells:'", ch, NULL, mob, TO_CHAR);
        send_to_char("  light: cure light wounds      10 gold\r\n", ch);
        send_to_char("  serious: cure serious wounds  15 gold\r\n", ch);
        send_to_char("  critic: cure critical wounds  25 gold\r\n", ch);
        send_to_char("  heal: healing spell           50 gold\r\n", ch);
        send_to_char("  blind: cure blindness         20 gold\r\n", ch);
        send_to_char("  disease: cure disease         15 gold\r\n", ch);
        send_to_char("  poison:  cure poison          25 gold\r\n", ch);
        send_to_char("  cancel:  cancellation         40 gold\r\n", ch);
        send_to_char("  uncurse: remove curse         50 gold\r\n", ch);
        send_to_char("  refresh: restore movement      5 gold\r\n", ch);
        send_to_char("  mana:  restore mana           10 gold\r\n", ch);
        send_to_char(" Type heal <type> to be healed.\r\n", ch);
        return;
    }

    if (!str_prefix(arg, "light"))
    {
        spell = spell_cure_light;
        sn = gsn_cure_light;
        words = "judicandus dies";
        cost = 1000;
    }
    else if (!str_prefix(arg, "serious"))
    {
        spell = spell_cure_serious;
        sn = gsn_cure_serious;
        words = "judicandus gzfuajg";
        cost = 1500;
    }
    else if (!str_prefix(arg, "critical"))
    {
        spell = spell_cure_critical;
        sn = gsn_cure_critical;
        words = "judicandus qfuhuqar";
        cost = 2500;
    }
    else if (!str_prefix(arg, "heal"))
    {
        spell = spell_heal;
        sn = gsn_heal;
        words = "pzar";
        cost = 5000;
    }
    else if (!str_prefix(arg, "blindness"))
    {
        spell = spell_cure_blindness;
        sn = gsn_cure_blindness;
        words = "judicandus noselacri";
        cost = 2000;
    }
    else if (!str_prefix(arg, "disease"))
    {
        spell = spell_cure_disease;
        sn = gsn_cure_disease;
        words = "judicandus eugzagz";
        cost = 1500;
    }
    else if (!str_prefix(arg, "poison"))
    {
        spell = spell_cure_poison;
        sn = gsn_cure_poison;
        words = "judicandus sausabru";
        cost = 2500;
    }
    else if (!str_prefix(arg, "uncurse") || !str_prefix(arg, "curse"))
    {
        spell = spell_remove_curse;
        sn = gsn_remove_curse;
        words = "candussido judifgz";
        cost = 5000;
    }
    else if (!str_prefix(arg, "mana") || !str_prefix(arg, "energize"))
    {
        spell = NULL;
        sn = -1;
        words = "energizer";
        cost = 1000;
    }
    else if (!str_prefix(arg, "refresh") || !str_prefix(arg, "moves"))
    {
        spell = spell_refresh;
        sn = gsn_refresh;
        words = "candusima";
        cost = 500;
    }
    else if (!str_prefix(arg, "cancel"))
    {
        spell = spell_cancellation;
        sn = gsn_cancellation;
        words = "clarivoix";
        cost = 4000;
    }
    else
    {
        act("$N says 'Type 'heal' for a list of spells.'", ch, NULL, mob, TO_CHAR);
        return;
    }

    if (cost > (ch->gold * 100 + ch->silver))
    {
        act("$N says 'You do not have enough gold for my services.'", ch, NULL, mob, TO_CHAR);
        return;
    }

    WAIT_STATE(ch, PULSE_VIOLENCE);

    deduct_cost(ch, cost);
    mob->gold += cost / 100;
    mob->silver += cost % 100;
    act("$n utters the words '$T'.", mob, NULL, words, TO_ROOM);

    if (spell == NULL)
    {                            /* restore mana trap...kinda hackish */
        ch->mana += dice(2, 8) + mob->level / 3;
        ch->mana = UMIN(ch->mana, ch->max_mana);
        send_to_char("A warm glow passes through you.\r\n", ch);
        return;
    }

    if (sn == -1)
        return;

    spell(sn, mob->level, mob, ch, TARGET_CHAR);

} // end do_heal

extern const struct portal_shop_type portal_shop_table[];

/*
 * Procedure which allows a player to purchase a portal from a mob.  The mob must be
 * set as act portalmerchant.  There is a base cost in the portal_shop_table and those
 * prices will be altered here based on the desination (e.g. it will cost more to create
 * a portal across continents than it will to travel inter-continent.  This procedure
 * will be called from do_list and do_buy to make it function like a normal shop.
 */
void process_portal_merchant(CHAR_DATA * ch, char *argument)
{
    int x = 0;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    bool found = FALSE;
    CHAR_DATA *mob;
    OBJ_DATA *portal;
    int cost = 0;
    int roll = 0;

    if ((mob = find_mob_by_act(ch, ACT_IS_PORTAL_MERCHANT)) == NULL)
    {
        send_to_char("You must find a portal merchant in order to purchase a portal.\r\n", ch);
        return;
    }

    // Not with people that can't be seen.. this will depend on the mob.. if the mob has detect hidden
    // or detect invis they will see most people unless they are in another for of non-detect, etc.
    if (!can_see(mob, ch))
    {
        act("{x$N says '{gI don't trade with folks I can't see, please make yourself 'visible'.{x", ch, NULL, mob, TO_CHAR);
        return;
    }

    // No argument was sent, send them the list of destinations.
    if (IS_NULLSTR(argument))
    {
        act("{x$N says '{gI offer the creation of portals to following destinations.{x'\r\n", ch, NULL, mob, TO_CHAR);

        // Loop through the destinations for display
        for (x = 0; portal_shop_table[x].name != NULL; x++)
        {
            int temp_cost = 0;

            if (same_continent(ch->in_room->vnum, portal_shop_table[x].to_vnum))
            {
                // They're in the same continent, normal cost (converted to gold).
                temp_cost = portal_shop_table[x].cost / 100;
            }
            else
            {
                // They are in different continents, double the cost (converted to gold).
                temp_cost = (portal_shop_table[x].cost / 100) * 2;
            }

            // To get the : into the formatted string
            sprintf(buf2, "{_%s{x:", portal_shop_table[x].name);

            sprintf(buf, "  %-17s{x {c%-40s{x %d gold\r\n",
                buf2,
                get_room_name(portal_shop_table[x].to_vnum),
                temp_cost);
            send_to_char(buf, ch);
        }

        send_to_char("\r\nType buy <location> to purchase a portal to that destination.\r\n", ch);
        return;
    }

    // Did the user select a valid input?
    for (x = 0; portal_shop_table[x].name != NULL; x++)
    {
        if (!str_prefix(argument, portal_shop_table[x].name))
        {
            if (same_continent(ch->in_room->vnum, portal_shop_table[x].to_vnum))
            {
                // They're in the same continent, normal cost (the raw cost).
                cost = portal_shop_table[x].cost;
            }
            else
            {
                // They are in different continents, double the cost (the raw cost)
                cost = (portal_shop_table[x].cost) * 2;
            }

            found = TRUE;
            break;
        }
    }

    // What the location found?
    if (!found)
    {
        act("{x$N says '{gThat is not a place that I know of.{x", ch, NULL, mob, TO_CHAR);
        return;
    }

    // Do they have enough money?
    if (cost > (ch->gold * 100 + ch->silver))
    {
        act("{x$N says '{gYou do not have enough gold for my services.{x'", ch, NULL, mob, TO_CHAR);
        return;
    }

    // Can the player haggle, if successful, adjust the price down?
    roll = number_percent();

    if (roll < get_skill(ch, gsn_haggle))
    {
        cost -= cost / 2 * roll / 100;
        sprintf(buf, "You haggle the price down to %d coins.\r\n", cost);
        send_to_char(buf, ch);
        check_improve(ch, gsn_haggle, TRUE, 4);
    }
    else
    {
        check_improve(ch, gsn_haggle, FALSE, 2);
    }

    // Slight purchase lag, same as buy.
    WAIT_STATE(ch, PULSE_VIOLENCE);

    // Deduct the money.
    deduct_cost(ch, cost);

    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL));
    portal->timer = 2;  // 2 ticks.
    portal->value[3] = portal_shop_table[x].to_vnum;

    // Alter the keywords for the portal to include the destination to make the portals easier to use
    // than the enter 3.portal syntax (if there are multiples in the room)
    sprintf(buf, "portal gate %s", portal_shop_table[x].name);
    free_string(portal->name);
    portal->name = str_dup(buf);

    // Alter the portal's description to the room, we'll presume we know where these are going.
    sprintf(buf, "A shimmering black gate rises from the ground, leading to %s.", get_area_name(portal_shop_table[x].to_vnum));
    free_string(portal->description);
    portal->description = str_dup(buf);

    // Put said object in said room.
    obj_to_room(portal, ch->in_room);

    act("$N raises $s hand and begans chanting.", ch, NULL, mob, TO_CHAR);
    act("$p rises up from the ground.", ch, portal, NULL, TO_ALL);

} // end do_portal

/*
 * Duplicates a piece of parchment for a cost at a scribe.
 */
void do_duplicate(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *mob;
    OBJ_DATA *obj;
    OBJ_DATA *clone_obj;
    int cost = 1000; // 10 gold

    if ((mob = find_mob_by_act(ch, ACT_SCRIBE)) == NULL)
    {
        send_to_char("You must find a scribe in order to duplicate pieces of parchment.\r\n", ch);
        return;
    }

    // Not with people that can't be seen.. this will depend on the mob.. if the mob has detect hidden
    // or detect invis they will see most people unless they are in another for of non-detect, etc.
    if (!can_see(mob, ch))
    {
        act("{x$N says '{gI don't trade with folks I can't see, please make yourself 'visible'.'{x", ch, NULL, mob, TO_CHAR);
        return;
    }

    // No argument was sent, tell them how much it costs
    if (IS_NULLSTR(argument))
    {
        act("{x$N says '{gI will duplicate a parchment for 10 gold pieces.{x'", ch, NULL, mob, TO_CHAR);
        act("{x$N says '{gYou may ask me to 'duplicate' a specific parchment in your possession.{x'", ch, NULL, mob, TO_CHAR);
        return;
    }

    if (cost > (ch->gold * 100 + ch->silver))
    {
        act("{x$N says '{gI apologize, but you do not appear to have enough wealth for my services.'{x", ch, NULL, mob, TO_CHAR);
        return;
    }

    if ((obj = get_obj_carry(ch, argument, ch)) == NULL)
    {
        act("{x$N says '{gI do not see that you have that item.'{x", ch, NULL, mob, TO_CHAR);
        return;
    }

    // Make sure the item is a piece of parchment.
    if (obj->item_type != ITEM_PARCHMENT)
    {
        act("{x$N says '{gThat is not a piece of parchment.'{x", ch, NULL, mob, TO_CHAR);
        return;
    }

    // Only copy the parchment if it has been written to.
    if (obj->value[1] == FALSE)
    {
        act("{x$N says '{gThat parchment has not been written to yet, please provide one that has.'{x", ch, NULL, mob, TO_CHAR);
        return;
    }

    // Deduct the cost and then clone the parchment.
    deduct_cost(ch, cost);
    mob->gold += cost / 100;
    mob->silver += cost % 100;

    clone_obj = create_object(obj->pIndexData);
    clone_object(obj, clone_obj);
    obj_to_char(clone_obj, ch);

    act("$N dips his quill in ink and begins writing on a piece of parchment.", ch, NULL, mob, TO_ROOM);
    act("$N hands you parchment with the identical text of your original.", ch, NULL, mob, TO_CHAR);

    // A little lag
    WAIT_STATE(ch, PULSE_VIOLENCE);
}

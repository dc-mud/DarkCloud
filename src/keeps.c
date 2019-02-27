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
#include "tables.h"
#include "grid.h"
#include "lookup.h"

/*
 * Keep guard, who will attack any player in a keep who is not in the clan who
 * owns the keep.
 */
bool spec_keep_guard(CHAR_DATA * ch)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *v_next;

    if (!IS_AWAKE(ch) || ch->fighting != NULL)
    {
        return FALSE;
    }

    // Set the clan of the keep guard to the clan who owns the keep currently
    if (ch->pIndexData->vnum == STORM_KEEP_GUARD && ch->clan != settings.storm_keep_owner)
    {
        ch->clan = settings.storm_keep_owner;
    }

    // Look for a victim, keep guards see all
    for (victim = ch->in_room->people; victim != NULL; victim = v_next)
    {
        v_next = victim->next_in_room;

        // They have to be a player, whose not a ghost and whose clan doesn't
        // own the keep.
        if (!IS_NPC(victim)
            && !IS_IMMORTAL(victim)
            && !IS_GHOST(victim)
            && victim->clan != ch->clan)
        {
            break;
        }
    }

    // Do we have a victim?
    if (victim != NULL)
    {
        sprintf(buf, "%s is invading the keep!  A call to arms!", victim->name);
        REMOVE_BIT(ch->comm, COMM_NOSHOUT);
        do_function(ch, &do_yell, buf);
        multi_hit(ch, victim, TYPE_UNDEFINED);
        return TRUE;
    }

    return FALSE;
}

/*
 * The keep lord will attack any clan who doesn't own the keep
 */
bool spec_keep_lord(CHAR_DATA * ch)
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    bool result;

    result = FALSE;

    // Keep lord is gonna attack all players who don't own the keep, whether it's
    // already fighting or not.
    if (!IS_AWAKE(ch))
    {
        return FALSE;
    }

    // Set the clan of the keep lord to the clan who owns the keep currently
    if (ch->pIndexData->vnum == STORM_KEEP_LORD && ch->clan != settings.storm_keep_owner)
    {
        ch->clan = settings.storm_keep_owner;
    }

    // Look for a victim, keep guards see all
    for (victim = ch->in_room->people; victim != NULL; victim = v_next)
    {
        v_next = victim->next_in_room;

        // Checks to see if the keep lord attacks (this should hit a multi hit on each player
        // in the room every few seconds out of the normal band of battle meaning more attacks
        // and everyone is getting hit).
        if (!IS_NPC(victim)
            && !IS_IMMORTAL(victim)
            && !IS_GHOST(victim)
            && victim->clan != ch->clan)
        {
            multi_hit(ch, victim, TYPE_UNDEFINED);
            result = TRUE;
        }
    }

    return result;
}

/*
 * Looks through the characters in the room with a person, if one is the keep lord they
 * will notify when a player is in the room (this should be called from do_enter so it's
 * only triggered when someone enters through a portal).
 */
void keep_lord_notify(CHAR_DATA *ch)
{
    CHAR_DATA *vch;

    // Look for the a keep lord in the room, if they are found, have them announce to
    // the owning clan that the keep is being attacked.
    for (vch = ch->in_room->people; vch != NULL; vch = vch->next)
    {
        if (IS_NPC(vch) && vch->pIndexData->vnum == STORM_KEEP_LORD)
        {
            vch->clan = settings.storm_keep_owner;
            break;
        }
    }

    // If this is null, no keep lord was found.
    if (vch == NULL)
    {
        return;
    }

    // If they are different clans, announce.
    if (vch->clan != ch->clan && !IS_IMMORTAL(ch))
    {
        char buf[MAX_STRING_LENGTH];

        if (!IS_NPC(ch))
        {
            sprintf(buf, "%s is invading the Storm Keep!", ch->name);
        }
        else
        {
            sprintf(buf, "%s is invading the Storm Keep!", ch->short_descr);
        }

        REMOVE_BIT(vch->comm, COMM_NOCHANNELS);
        do_clantalk(vch, buf);
    }

    return;
}

/*
 * Globally apply the keep affect to the players (this will be called off tick
 * when an event calls for the removing/re-adding of the affects).  For instance
 * this will be called when the keep is taken.
 */
void apply_keep_affects()
{
    DESCRIPTOR_DATA *d;

    for (d = descriptor_list; d; d = d->next)
    {
        if (d == NULL || d->character == NULL)
        {
            continue;
        }

        if (d->connected == CON_PLAYING)
        {
            apply_keep_affects_to_char(d->character);
        }
    }

    return;
}

/*
 * Apply the keep affect to one char (generally on login).
 */
void apply_keep_affects_to_char(CHAR_DATA * ch)
{
    AFFECT_DATA af;

    if (IS_NPC(ch))
    {
        return;
    }

    // Storm Keep (Must be a clanner, must be targeted, must not already have the affect)
    if (is_clan(ch)
        && settings.storm_keep_target == ch->clan
        && !is_affected(ch, gsn_curse_of_the_storm))
    {
        // 60 AC Penalty
        af.where = TO_AFFECTS;
        af.level = MAX_LEVEL;
        af.duration = -1;
        af.location = APPLY_AC;
        af.modifier = 60;
        af.type = gsn_curse_of_the_storm;
        af.bitvector = 0;
        affect_to_char(ch, &af);
    }
    else if (is_affected(ch, gsn_curse_of_the_storm) && settings.storm_keep_target != ch->clan)
    {
        affect_strip(ch, gsn_curse_of_the_storm);
    }

    return;
}

/*
 *  Command that shows what clan (if any) owns which keeps.
 */
void do_keeps(CHAR_DATA * ch, char *argument)
{
    GRID_DATA *grid;
    GRID_ROW *row;
    char buf[MAX_STRING_LENGTH];

    send_to_char("\r\n", ch);

    // Create the grid
    grid = create_grid(75);

    // Row 1
    row = create_row_padded(grid, 0, 0, 2, 2);
    sprintf(buf, "%s", center_string_padded("{WKeeps{x", 71));
    row_append_cell(row, 75, buf);

    // Row 2
    row = create_row_padded(grid, 1, 1, 2, 2);

    row_append_cell(row, 75, "Storm Keep Owner:   {C%s{x\nStorm Keep Target:  {C%s{x",
        (settings.storm_keep_owner == 0 ? "None" : clan_table[settings.storm_keep_owner].friendly_name),
        (settings.storm_keep_target == 0 ? "None" : clan_table[settings.storm_keep_target].friendly_name));

    // Finally, send the grid to the character
    grid_to_char(grid, ch, TRUE);
}

/*
 * Allows the clan leader of the clan that owns the storm keep to set it's affect
 * onto another clan.
 */
void do_setstorm(CHAR_DATA * ch, char *argument)
{
    int storm_keep_target;
    char buf[MAX_STRING_LENGTH];

    if (!is_clan(ch) || IS_NPC(ch))
    {
        send_to_char("You are not in a clan.\r\n", ch);
        return;
    }

    if (!IS_SET(ch->pcdata->clan_flags, CLAN_LEADER))
    {
        send_to_char("You are not the leader of your clan.\r\n", ch);
        return;
    }

    if (settings.storm_keep_owner != ch->clan)
    {
        send_to_char("Your clan does not own the Storm Keep.\r\n", ch);
        return;
    }

    if (IS_NULLSTR(argument))
    {
        send_to_char("Please enter the clan you wish to set the curse of the storm onto.\r\n", ch);
        return;
    }

    if ((storm_keep_target = clan_lookup(argument)) > 0)
    {
        settings.storm_keep_target = storm_keep_target;
    }
    else
    {
        send_to_char("Please enter the clan to set the curse of the storm onto.\r\n", ch);
        return;
    }

    sprintf(buf, "$N has set the Storm Keep target to %s.", clan_table[storm_keep_target].friendly_name);
    wiznet(buf, ch, NULL, 0, 0, 0);

    sendf(ch, "The Storm Keep target has been set to %s.\r\n", clan_table[storm_keep_target].friendly_name);

    save_settings();
}

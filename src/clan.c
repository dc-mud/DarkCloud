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
 *  Clan Code                                                              *
 *                                                                         *
 *  We consolidate code relating to clans here excluding the communication *
 *  commands which will continue to live in act_comm.c                     *
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
#include <stdlib.h>
#include <ctype.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"

/*
 * Clan Table
 *
 * Name, Who Name, Friendly Name, Death Transfer Room, Recall VNUM, Independent,
 * Guild Message, Regen Room Directions
 *
 * Independent should be FALSE if it is a real clan
 */
const struct clan_type clan_table[MAX_CLAN] = {
    { "",           "",                      "",             ROOM_VNUM_ALTAR,           ROOM_VNUM_ALTAR,            TRUE,  FALSE,
      ""
    },
    { "loner",      "[ {WLoner{x ] ",        "Loner",        ROOM_VNUM_ALTAR,           ROOM_VNUM_ALTAR,            TRUE,  TRUE,
      "%s walks alone.\r\n", ""
    },
    { "renegade",   "[ {WRenegade{x ] ",     "Renegade",     ROOM_VNUM_ALTAR,           ROOM_VNUM_ALTAR,            TRUE,  TRUE,
      "%s has been renegaded.\r\n", ""
    }
};

/*
 * The clan ranks that a leader can rank their members to (these are role play ranks).  Unfortunately, to link these
 * to a clan we will use the position which makes the position in the above table important.
 *
 * name, clan, is_default
 */
const struct clan_rank_type clan_rank_table[] =
{
    { "",            CLAN_NONE,         FALSE },
    { "Page",        CLAN_KNIGHTS,      TRUE  },
    { "Squire",      CLAN_KNIGHTS,      FALSE },
    { "Knight",      CLAN_KNIGHTS,      FALSE },
    { "Lord Crown",  CLAN_KNIGHTS,      FALSE },
    { "Entrant",     CLAN_VALOR,        TRUE  },
    { "Soldier",     CLAN_VALOR,        FALSE },
    { "Commander",   CLAN_VALOR,        FALSE },
    { "General",     CLAN_VALOR,        FALSE },
    { "Peon",        CLAN_MALICE,       TRUE  },
    { "Henchman",    CLAN_MALICE,       FALSE },
    { "Overlord",    CLAN_MALICE,       FALSE },
    { "Minion",      CLAN_CULT,         TRUE  },
    { "Dark Lord",   CLAN_CULT,         FALSE },
    { "Nameless",    CLAN_SYLVAN,       TRUE  },
    { "Named",       CLAN_SYLVAN,       FALSE },
    { "Regent",      CLAN_SYLVAN,       FALSE },
    { "Senator",     CLAN_SYLVAN,       FALSE },
    { "Speaker",     CLAN_SYLVAN,       FALSE },
    { "Member",      CLAN_WAR_HAMMER,   TRUE  },
    { "War Council", CLAN_WAR_HAMMER,   FALSE },
    { "Thane",       CLAN_WAR_HAMMER,   FALSE },
    { "Apprentice",  CLAN_CONCLAVE,     TRUE  },
    { "Student",     CLAN_CONCLAVE,     FALSE },
    { "Sorcerer",    CLAN_CONCLAVE,     FALSE },
    { "High Wizard", CLAN_CONCLAVE,     FALSE },
    { NULL,          CLAN_NONE,         FALSE }
};

/*
 * These are flags that can be set on a player in a clan.
 */
const struct flag_type pc_clan_flags[] = {
    {"leader",    CLAN_LEADER,    TRUE},
    {"recruiter", CLAN_RECRUITER, TRUE},
    {"exile",     CLAN_EXILE,     TRUE},
    {NULL, 0, 0}
};

/*
 * Whether or not the character is in a clan or not.
 */
bool is_clan(CHAR_DATA * ch)
{
    return ch->clan;
}

/*
 * Whether or not two characters are in the same clan.
 */
bool is_same_clan(CHAR_DATA * ch, CHAR_DATA * victim)
{
    // Loner's and renegades should never be considered in the same clan.
    if (clan_table[ch->clan].independent)
    {
        return FALSE;
    }
    else
    {
        return (ch->clan == victim->clan);
    }
}

/*
 * Gets the int for a clan based on it's name.
 */
int clan_lookup(const char *name)
{
    int clan;

    for (clan = 0; clan < MAX_CLAN; clan++)
    {
        if (LOWER(name[0]) == LOWER(clan_table[clan].name[0])
            && !str_prefix(name, clan_table[clan].name))
        {
            return clan;
        }
    }

    return 0;
}

/*
 * Looks up a player's clan rank, if nothing is found then return 0.  The character
 * is required so we can make sure what's found is in the right clan.
 */
int clan_rank_lookup(const char *name, CHAR_DATA *ch)
{
    int rank;

    // Independents won't have ranks.
    if (clan_table[ch->clan].independent)
    {
        return 0;
    }

    for (rank = 0; clan_rank_table[rank].name != NULL; rank++)
    {
        if (LOWER(name[0]) == LOWER(clan_rank_table[rank].name[0])
            && !str_prefix(name, clan_rank_table[rank].name)
            && clan_rank_table[rank].clan == ch->clan)
        {
            return rank;
        }
    }

    // This could be because the rank was removed, log it.
    log_f("%s's clan rank of %s was invalid, setting to default rank.", ch->name, name);

    // If the rank wasn't found... try to get the default rank
    for (rank = 0; clan_rank_table[rank].name != NULL; rank++)
    {
        if (clan_rank_table[rank].clan == ch->clan && clan_rank_table[rank].is_default == TRUE)
        {
            return rank;
        }
    }

    // This is a bug, all clans should have a default rank
    bugf("%s's clan has no valid default rank", ch->name);

    // All out fail, return 0.
    return 0;
}

/*
 * Sets the default rank for a player.  When a player is newly guilded through any means
 * this function should be called to get the default rank.
 */
void set_default_rank(CHAR_DATA *ch)
{
    if (ch == NULL || IS_NPC(ch))
    {
        return;
    }

    int x;

    for (x = 0; clan_rank_table[x].name != NULL; x++)
    {
        if (clan_rank_table[x].clan == ch->clan && clan_rank_table[x].is_default == TRUE)
        {
            ch->pcdata->clan_rank = x;
            return;
        }
    }

    ch->pcdata->clan_rank = 0;
    return;
}

/*
 * A command to allow a player to become a loner without having to have an
 * immortal guild them.
 */
void do_loner(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
    {
        send_to_char("NPC's can not guild as a loner.\r\n", ch);
        return;
    }

    // We will require them to enter their name as a confirmation
    if ( argument[ 0 ] == '\0' )
    {
        send_to_char( "Syntax: loner <your name>\r\n",ch);
        return;
    }

    if (str_cmp(argument, ch->name))
    {
        send_to_char("This is not your name...\r\n", ch);
        return;
    }

    if (ch->level < 5 || ch->level > 25)
    {
        send_to_char("You can only choose to become a loner between levels 5 and 25.\r\n", ch);
        return;
    }

    if (is_clan(ch))
    {
        send_to_char("You are already part of the clanned world.\r\n", ch);
        return;
    }

    // Guild the player and send a global message to those online
    ch->clan = clan_lookup("loner");

    sprintf(buf, "\r\n%s walks alone. %s\r\n", ch->name, clan_table[ch->clan].who_name);
    send_to_all_char(buf);

    log_f("%s guilds themselves to [ Loner ]", ch->name);
}

/*
 * Lists all of the clans in the game.  Consider making the guild command that allows for getting
 * info as well as putting players in commands with both immortal and player based features so players
 * can manage their own clans without immortals (will need leader and recruiter flags).
 */
void do_guildlist(CHAR_DATA *ch, char *argument)
{
    int clan;
    char buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *location;

    send_to_char("--------------------------------------------------------------------------------\r\n", ch);
    send_to_char("{WClan               Independent   Continent  Recall Point{x\r\n", ch);
    send_to_char("--------------------------------------------------------------------------------\r\n", ch);

    for (clan = 0; clan < MAX_CLAN; clan++)
    {
        if (IS_NULLSTR(clan_table[clan].name) || !clan_table[clan].enabled)
        {
           continue;
        }

        location = get_room_index(clan_table[clan].recall_vnum);

        sprintf(buf, "%-22s{x %-13s %-10s {c%-20s{x\r\n",
            clan_table[clan].who_name,
            clan_table[clan].independent == TRUE ? "True" : "False",
            (location != NULL && location->area != NULL) ? capitalize(continent_table[location->area->continent].name) : "Unknown",
            (location != NULL && !IS_NULLSTR(location->name)) ? location->name : "Unknown"
            );

        send_to_char(buf, ch);
    }

} // end guildlist

/*
 * If the guild command is run by a mortal, it will re-route them to here.  As a result, this
 * specific procedure won't be wired up in interp.
 */
void guild_clan(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;

    // Go through the checks to make sure this is legit, first will be the recruiter/leader
    // checks, then we will have to validate the victim and make sure this is an action both
    // parties want (the victim will need to have invited the recruiter).

    // No NPC's, the recruiter flag uses pcdata so exit here.
    if (IS_NPC(ch))
    {
        return;
    }

    // If the caller isn't even in a clan, exit.
    if (!is_clan(ch))
    {
        send_to_char("You must be the leader or recruiter of a clan to guild someone.\r\n", ch);
        return;
    }

    // Are they a leader or a recruiter?
    if (!IS_SET(ch->pcdata->clan_flags, CLAN_LEADER) &&
        !IS_SET(ch->pcdata->clan_flags, CLAN_RECRUITER))
    {
        send_to_char("You must be the leader or recruiter of a clan to guild someone.\r\n", ch);
        return;
    }

    // Did they pass in the character's name?
    if (IS_NULLSTR(argument))
    {
        send_to_char( "Syntax: guild <character>\r\n", ch);
        return;
    }

    // Find the player they requested, if none is found then exit
    if ((victim = get_char_world(ch, argument)) == NULL)
    {
        send_to_char("They aren't playing or you cannot see them at this moment.\r\n", ch);
        return;
    }

    // NPC's are not valid.
    if (IS_NPC(victim))
    {
        act("$N cannot be guilded.", ch, NULL, victim, TO_CHAR);
        return;
    }

    // They must be between level's 5 and 25 unless they are a loner or a renegade.
    if ((victim->level < 5 || victim->level > 25) && !is_clan(victim))
    {
        send_to_char("The candidate must be between level 5 and 25.\r\n", ch);
        return;
    }

    // We don't want them resetting all of their own flags or echo'ingto the world
    if (victim == ch)
    {
        send_to_char("You cannot guild yourself.\r\n", ch);
        return;
    }

    // They're already in the same clan, no guilding needed.
    if (victim->clan == ch->clan)
    {
        send_to_char("They are already in your clan.\r\n", ch);
        return;
    }

    // Check that the victim has give their consent to the player guilding them.
    if (victim->pcdata->consent == NULL || victim->pcdata->consent != ch)
    {
        sendf(ch, "%s has not provided their consent to you to be guilded.\r\n", victim->name);
        return;
    }

    // Here is where we will enforce recruiting restrictions (e.g. elves only in Sylvan, dwarves only in
    // War Hammer, neutral/evil only in Cult, etc.).
    switch (ch->clan)
    {
        case CLAN_SYLVAN:
            // Victim must be any elf type, except dark elves (restriction warrants CSR)
            if (!IS_ELF(victim))
            {
                send_to_char("They are not an elf.\r\n", ch);
                return;
            }

            break;
        case CLAN_WAR_HAMMER:
            // Victim must be a dwarf (restrction warrants CSR)
            if (!IS_DWARF(victim))
            {
                send_to_char("They are not a dwarf.\r\n", ch);
                return;
            }

            break;
        case CLAN_CULT:
            // Must be evil and human (opposite of knights, restrction warrants CSR).
            if (!IS_EVIL(victim) || victim->race != HUMAN_RACE)
            {
                send_to_char("They are neither evil nor a human which is required by the order of the Cult.\r\n", ch);
                return;
            }

            break;
        case CLAN_VALOR:
            // Must be good or neutral, no race restrictions
            if (IS_EVIL(victim))
            {
                send_to_char("Only those who are good or neutral may join Valor.\r\n", ch);
                return;
            }

            break;
        case CLAN_MALICE:
            // Must be evil or neutral, no race restrictions
            if (IS_GOOD(victim))
            {
                send_to_char("Only those who are evil or neutral may join Malice.\r\n", ch);
                return;
            }

            break;
        case CLAN_KNIGHTS:
            // Must be good and human (opposite of knights, restrction warrants CSR).
            if (!IS_GOOD(victim) || victim->race != HUMAN_RACE)
            {
                send_to_char("They are neither good nor a human which is required by the order of the Knights.\r\n", ch);
                return;
            }

            break;
        case CLAN_CONCLAVE:
            // Must be a mage
            if (victim->class != MAGE_CLASS
                && victim->class != PSIONICIST_CLASS
                && victim->class != WIZARD_CLASS
                && victim->class != ENCHANTER_CLASS)
            {
                send_to_char("They must follow the path of a mage in order to join the towers of Conclave.\r\n", ch);
                return;
            }

            return;
        default:
            break;
    }

    // Set the new clan
    victim->clan = ch->clan;

    // Set the default rank
    set_default_rank(victim);

    // Reset old clan bits that might exist
    int x = 0;

    for (x = 0; pc_clan_flags[x].name != NULL; x++)
    {
        REMOVE_BIT(victim->pcdata->clan_flags, pc_clan_flags[x].bit);
    }

    // Now that the consent has been used, unset the reference.
    victim->pcdata->consent = NULL;

    // Send the echo to the world that's specific for this clan (each echo in the clan table
    // expects a %s to be there as a spot for the players name (this is important)
    if (!IS_NULLSTR(clan_table[victim->clan].guild_msg))
    {
        char buf[MAX_STRING_LENGTH];
        sprintf(buf, clan_table[victim->clan].guild_msg, victim->name);
        send_to_all_char(buf);
    }
    else
    {
        // There was no message specified, just local pecho to the players.
        send_to_char("They have been guilded.\r\n", ch);
        send_to_char("You have been guilded.\r\n", victim);
    }

}

/*
 * Allows a leader to rank another player as a leader, a recruiter or any of their role
 * play ranks for their clan.
 */
void do_rank(CHAR_DATA *ch, char *argument)
{
    // Hmm, no
    if (IS_NPC(ch))
        return;

    CHAR_DATA *victim;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int rank = 0;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (IS_NULLSTR(arg1) || IS_NULLSTR(arg2))
    {
        send_to_char("Rank who with what rank?\r\n",ch);

        // If their in a clan (and that clan isn't indepedent, e.g. has ranks)
        if (is_clan(ch) && !clan_table[ch->clan].independent)
        {
            sendf(ch, "\r\nYour clan has the following ranks available:\r\n\r\n");

            for (rank = 0; clan_rank_table[rank].name != NULL; rank++)
            {
                if (clan_rank_table[rank].clan == ch->clan)
                {
                    sendf(ch, " * %s\r\n", clan_rank_table[rank].name);
                }
            }
        }

        return;
    }

    if (!IS_IMMORTAL(ch) && !IS_SET(ch->pcdata->clan_flags, CLAN_LEADER))
    {
        send_to_char("Only clan leaders may rank their members.\r\n", ch);
        return;
    }

    if ((victim = get_char_world(ch, arg1)) == NULL)
    {
        send_to_char("They are not here.\r\n",ch);
        return;
    }

    if (!is_same_clan(ch, victim))
    {
        sendf(ch, "%s isn't in your clan.\r\n", victim->name);
        return;
    }

    // We have two types of ranks.  The first is rank that has a game affect of some
    // kind, this are in the pc_clan_flags table.  These are leaders, recruiters and
    // whether someone is exiled from their clan.  The second type of rank is an roleplay
    // rank within a clan, lord, general, peon, minion, senator, etc.  They are used to
    // grant RP hierarchy.
    int flag = flag_lookup(arg2, pc_clan_flags);

    if (flag != NO_FLAG)
    {
        int pos = flag_position_lookup(arg2, pc_clan_flags);

        if (IS_SET(victim->pcdata->clan_flags, flag))
        {
            REMOVE_BIT(victim->pcdata->clan_flags, flag);
            sendf(ch, "You have removed the %s flag from %s.\r\n", pc_clan_flags[pos].name, victim->name);

            if (flag == CLAN_EXILE)
            {
                sendf(victim, "%s has lifted your exile.\r\n", ch->name);
            }
            else
            {
                sendf(victim, "%s has removed the %s flag.\r\n", ch->name, pc_clan_flags[pos].name);
            }

            return;
        }
        else
        {
            SET_BIT(victim->pcdata->clan_flags, flag);
            sendf(ch, "You have added the %s flag to %s.\r\n", pc_clan_flags[pos].name, victim->name);

            if (flag == CLAN_EXILE)
            {
                sendf(victim, "%s has exiled you from the clan!\r\n", ch->name);
            }
            else
            {
                sendf(victim, "%s has bestowed upon you the %s flag.\r\n", ch->name, pc_clan_flags[pos].name);
            }

            return;
        }
    }

    if ((rank = clan_rank_lookup(arg2, victim)) == 0)
    {
        send_to_char("That flag or rank was not found for your clan.\r\n", ch);
        return;
    }

    // Set the rank
    victim->pcdata->clan_rank = rank;

    if (victim != ch)
    {
        sendf(ch, "You have given %s the rank of %s.\r\n", victim->name, clan_rank_table[rank].name);
        sendf(victim, "%s has bestowed the rank of %s upon you.\r\n", ch->name, clan_rank_table[rank].name);
    }
    else
    {
        sendf(ch, "You have ranked yourself %s.\r\n", clan_rank_table[rank].name);
    }

    return;
}

/*
 * Command tha allows a clan leader to renegade a member from their clan.
 */
void do_renegade(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    ROOM_INDEX_DATA *room;
    char buf[MAX_STRING_LENGTH];

    if (IS_NULLSTR(argument))
    {
        send_to_char("Renegade whom?\r\n", ch);
        return;
    }

    // Are they a leader or an immortal (although an immortal should really just guild them)?
    if (!IS_SET(ch->pcdata->clan_flags, CLAN_LEADER) && !IS_IMMORTAL(ch))
    {
        send_to_char("You must be the leader of your clan to renegade another.\r\n", ch);
        return;
    }

    // Find the player to renegade
    if ((victim = get_char_world(ch, argument)) == NULL || IS_NPC(victim))
    {
        send_to_char("They are not here.\r\n", ch);
        return;
    }

    // Check if they are in the same clan
    if (!is_same_clan(ch, victim) && !IS_IMMORTAL(ch))
    {
        sendf(ch, "%s isn't in your clan.\r\n", victim->name);
        return;
    }

    // No renegading another leader, this will have to be done by an immortal
    if (IS_SET(victim->pcdata->clan_flags, CLAN_LEADER) && !IS_IMMORTAL(ch))
    {
        send_to_char("You cannot renegade another leader, an immortal will need to assist you.\r\n", ch);
        return;
    }

    // If they're in a clan hall room send them to their recall point
    if (victim->in_room->clan && victim->clan == victim->in_room->clan)
    {
        // Get the recall point to send them to
        room = get_room_index(clan_table[victim->clan].recall_vnum);

        // Show the room that the player is being removed.
        act("$n is dragged by guards from the area!", victim, NULL, NULL, TO_ROOM);
        char_from_room(victim);

        // Show the room that the player is being placed in.
        char_to_room(victim, room);
        act("$n has been dragged into the room and tossed to the ground.", victim, NULL, NULL, TO_ROOM);

        // Have the player auto look to they know where they are.
        do_function(victim, &do_look, "auto");
    }

    // Tell the victim
    sendf(victim, "%s has renegaded you.\r\n", ch->name);

    // Tell the world
    sprintf(buf, "%s has been renegaded from %s\r\n", victim->name, clan_table[victim->clan].who_name);
    send_to_all_char(buf);

    // Tell the logs
    log_f("%s renegaded %s from %s", ch->name, victim->name, clan_table[victim->clan].who_name);
    log_string(buf);

    // Set the final bits, they're a renegade and all of their ranks are stripped.
    victim->clan = CLAN_RENEGADE;
    victim->pcdata->clan_rank = 0;
    victim->pcdata->clan_flags = 0;

    return;
}


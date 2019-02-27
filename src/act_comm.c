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
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "sha256.h"
#include <ctype.h> /* for isalpha() and isspace() -- JR */

/*
 * Void to force player to fully type out delete if they want to delete.
 */
void do_delet(CHAR_DATA * ch, char *argument)
{
    send_to_char("You must type the full command to delete yourself.\r\n", ch);
} // end void do_delet

/*
 * Delete command that will allow a player to delete their pfile.
 */
void do_delete(CHAR_DATA * ch, char *argument)
{
    char strsave[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    if (IS_NPC(ch))
        return;

    // Wiznet and log, but without including the password
    sprintf(buf, "Log %s: Delete", ch->name);
    wiznet(buf, ch, NULL, 0, 0, get_trust(ch));
    log_string(buf);

    if (ch->pcdata->confirm_delete)
    {
        if (strcmp(sha256_crypt_with_salt(argument, ch->name), ch->pcdata->pwd))
        {
            send_to_char("Delete status removed.\n\r", ch);
            ch->pcdata->confirm_delete = FALSE;
            return;
        }
        else
        {
            sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(ch->name));
            wiznet("$N turns $Mself into line noise.", ch, NULL, 0, 0, 0);
            stop_fighting(ch, TRUE);
            act("$n slowly fades away into the ether...", ch, NULL, NULL, TO_ROOM);
            do_quit(ch, "");

#if defined(_WIN32)
            _unlink(strsave);
#else
            unlink(strsave);
#endif

            return;
        }
    }

    if (strcmp(sha256_crypt_with_salt(argument, ch->name), ch->pcdata->pwd))
    {
        send_to_char("Just type delete and your password.\n\r", ch);
        return;
    }

    send_to_char("Type delete and your password again to confirm this command.\r\n", ch);
    send_to_char("{RWARNING:{x this command is irreversible.\r\n", ch);
    send_to_char("Typing delete with an argument will undo delete status.\r\n", ch);
    ch->pcdata->confirm_delete = TRUE;
    wiznet("$N is contemplating deletion.", ch, NULL, 0, 0, get_trust(ch));
} // end void do_delete

/*
 * RT code to display channel status
 */
void do_channels(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    /* lists all channels and their status */
    send_to_char("Channel            Status\r\n", ch);
    send_to_char("-------------------------\r\n", ch);

    if (is_clan(ch))
    {
        send_to_char("clan               ", ch);

        if (!IS_SET(ch->comm, COMM_NOCLAN))
            send_to_char("{GON{x\r\n", ch);
        else
            send_to_char("{ROFF{x\r\n", ch);

        send_to_char("ooc clan           ", ch);

        if (!IS_SET(ch->comm, COMM_NOOCLAN))
            send_to_char("{GON{x\r\n", ch);
        else
            send_to_char("{ROFF{x\r\n", ch);

    }

    send_to_char("gossip             ", ch);
    if (!IS_SET(ch->comm, COMM_NOGOSSIP))
        send_to_char("{GON{x\r\n", ch);
    else
        send_to_char("{ROFF{x\r\n", ch);

    send_to_char("clan gossip        ", ch);
    if (!IS_SET(ch->comm, COMM_NOCGOSSIP))
        send_to_char("{GON{x\r\n", ch);
    else
        send_to_char("{ROFF{x\r\n", ch);

    send_to_char("auction            ", ch);
    if (!IS_SET(ch->comm, COMM_NOAUCTION))
        send_to_char("{GON{x\r\n", ch);
    else
        send_to_char("{ROFF{x\r\n", ch);

    send_to_char("ooc                ", ch);
    if (!IS_SET(ch->comm, COMM_NOOOC))
        send_to_char("{GON{x\r\n", ch);
    else
        send_to_char("{ROFF{x\r\n", ch);

    send_to_char("Q/A                ", ch);
    if (!IS_SET(ch->comm, COMM_NOQUESTION))
        send_to_char("{GON{x\r\n", ch);
    else
        send_to_char("{ROFF{x\r\n", ch);

    send_to_char("grats              ", ch);
    if (!IS_SET(ch->comm, COMM_NOGRATS))
        send_to_char("{GON{x\r\n", ch);
    else
        send_to_char("{ROFF{x\r\n", ch);

    if (IS_IMMORTAL(ch))
    {
        send_to_char("imm channel        ", ch);
        if (!IS_SET(ch->comm, COMM_NOWIZ))
            send_to_char("{GON{x\r\n", ch);
        else
            send_to_char("{ROFF{x\r\n", ch);
    }

    send_to_char("tells              ", ch);
    if (!IS_SET(ch->comm, COMM_DEAF))
        send_to_char("{GON{x\r\n", ch);
    else
        send_to_char("{ROFF{x\r\n", ch);

    send_to_char("quiet mode         ", ch);
    if (IS_SET(ch->comm, COMM_QUIET))
        send_to_char("{GON{x\r\n", ch);
    else
        send_to_char("{ROFF{x\r\n", ch);

    send_to_char("line feed on tick  ", ch);
    if (IS_SET(ch->comm, COMM_LINEFEED_TICK))
        send_to_char("{GON{x\r\n", ch);
    else
        send_to_char("{ROFF{x\r\n", ch);


    if (IS_SET(ch->comm, COMM_AFK))
        send_to_char("You are AFK.\r\n", ch);

    if (IS_SET(ch->comm, COMM_SNOOP_PROOF))
        send_to_char("You are immune to snooping.\r\n", ch);

    if (ch->lines != PAGELEN)
    {
        if (ch->lines)
        {
            sprintf(buf, "You display %d lines of scroll.\r\n",
                ch->lines + 2);
            send_to_char(buf, ch);
        }
        else
            send_to_char("Scroll buffering is off.\r\n", ch);
    }

    if (ch->prompt != NULL)
    {
        sprintf(buf, "Your current prompt is: %s\r\n", ch->prompt);
        send_to_char(buf, ch);
    }

    if (IS_SET(ch->comm, COMM_NOSHOUT))
        send_to_char("You cannot shout.\r\n", ch);

    if (IS_SET(ch->comm, COMM_NOTELL))
        send_to_char("You cannot use tell.\r\n", ch);

    if (IS_SET(ch->comm, COMM_NOCHANNELS))
        send_to_char("You cannot use channels.\r\n", ch);

    if (IS_SET(ch->comm, COMM_NOEMOTE))
        send_to_char("You cannot show emotions.\r\n", ch);

} // end void do_channels

/*
 * RT deaf blocks out all shouts and a few other forms of communication.
 */
void do_deaf(CHAR_DATA * ch, char *argument)
{
    if (IS_SET(ch->comm, COMM_DEAF))
    {
        send_to_char("You can now hear tells again.\r\n", ch);
        REMOVE_BIT(ch->comm, COMM_DEAF);
    }
    else
    {
        send_to_char("From now on, you won't hear tells.\r\n", ch);
        SET_BIT(ch->comm, COMM_DEAF);
    }
} // end do_deaf

/*
 * RT quiet blocks out all communication
 */
void do_quiet(CHAR_DATA * ch, char *argument)
{
    if (IS_SET(ch->comm, COMM_QUIET))
    {
        send_to_char("Quiet mode removed.\r\n", ch);
        REMOVE_BIT(ch->comm, COMM_QUIET);
    }
    else
    {
        send_to_char("From now on, you will only hear says and emotes.\r\n",
            ch);
        SET_BIT(ch->comm, COMM_QUIET);
    }
} // end do_quiet

/*
 * AFK (away from keyboard) command
 */
void do_afk(CHAR_DATA * ch, char *argument)
{
    if (IS_SET(ch->comm, COMM_AFK))
    {
        send_to_char("AFK mode removed. Type 'replay' to see tells.\r\n",
            ch);
        REMOVE_BIT(ch->comm, COMM_AFK);
    }
    else
    {
        send_to_char("You are now in AFK mode.\r\n", ch);
        SET_BIT(ch->comm, COMM_AFK);
    }
} // end do_afk

/*
 * Replay command, attempts to show the users tells that they missed while they
 * were AFK or in a mode where they could not see them.
 */
void do_replay(CHAR_DATA * ch, char *argument)
{
    if (IS_NPC(ch))
    {
        send_to_char("You can't replay.\r\n", ch);
        return;
    }

    if (buf_string(ch->pcdata->buffer)[0] == '\0')
    {
        send_to_char("You have no tells to replay.\r\n", ch);
        return;
    }

    page_to_char(buf_string(ch->pcdata->buffer), ch);
    clear_buf(ch->pcdata->buffer);
} // end do_replay

/*
 * RT auction channel rewritten in ROM style
 */
void do_auction(CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET(ch->comm, COMM_NOAUCTION))
        {
            send_to_char("{aAuction channel is now ON.{x\r\n", ch);
            REMOVE_BIT(ch->comm, COMM_NOAUCTION);
        }
        else
        {
            send_to_char("{aAuction channel is now OFF.{x\r\n", ch);
            SET_BIT(ch->comm, COMM_NOAUCTION);
        }

        return;

    }
    else
    {                            /* auction message sent, turn auction on if it is off */

        if (IS_SET(ch->comm, COMM_QUIET))
        {
            send_to_char("You must turn off quiet mode first.\r\n", ch);
            return;
        }

        if (IS_SET(ch->comm, COMM_NOCHANNELS))
        {
            send_to_char("The gods have revoked your channel priviliges.\r\n", ch);
            return;
        }

        REMOVE_BIT(ch->comm, COMM_NOAUCTION);
    }

    argument = make_drunk(argument, ch);
    sendf(ch, "{xYou auction '{m%s{x'\r\n", argument);

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        CHAR_DATA *victim;

        victim = d->original ? d->original : d->character;

        if (d->connected == CON_PLAYING &&
            d->character != ch &&
            !IS_SET(d->character->affected_by, AFF_DEAFEN) &&
            !IS_SET(victim->comm, COMM_NOAUCTION) &&
            !IS_SET(victim->comm, COMM_QUIET))
        {
            act_new("{x$n auctions '{m$t{x'", ch, argument, d->character, TO_VICT, POS_DEAD);
        }
    }
} // end do_auction

/* RT chat replaced with ROM gossip */
void do_gossip(CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET(ch->comm, COMM_NOGOSSIP))
        {
            send_to_char("Gossip channel is now ON.\r\n", ch);
            REMOVE_BIT(ch->comm, COMM_NOGOSSIP);
        }
        else
        {
            send_to_char("Gossip channel is now OFF.\r\n", ch);
            SET_BIT(ch->comm, COMM_NOGOSSIP);
        }
    }
    else
    {                            /* gossip message sent, turn gossip on if it isn't already */

        if (IS_SET(ch->comm, COMM_QUIET))
        {
            send_to_char("You must turn off quiet mode first.\r\n", ch);
            return;
        }

        if (IS_SET(ch->comm, COMM_NOCHANNELS))
        {
            send_to_char("The gods have revoked your channel priviliges.\r\n", ch);
            return;
        }

        REMOVE_BIT(ch->comm, COMM_NOGOSSIP);

        argument = make_drunk(argument, ch);
        sendf(ch, "{xYou gossip '{W%s{x'\r\n", argument);

        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                !IS_SET(d->character->affected_by, AFF_DEAFEN) &&
                !IS_SET(victim->comm, COMM_NOGOSSIP) &&
                !IS_SET(victim->comm, COMM_QUIET))
            {
                act_new("{x$n gossips '{W$t{x'", ch, argument, d->character, TO_VICT, POS_SLEEPING);
            }
        }
    }
} // end do_gossip

/*
 * Clan gossip channel
 */
void do_cgossip(CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET(ch->comm, COMM_NOCGOSSIP))
        {
            send_to_char("Clan gossip channel is now ON.\r\n", ch);
            REMOVE_BIT(ch->comm, COMM_NOCGOSSIP);
        }
        else
        {
            send_to_char("Clan gossip channel is now OFF.\r\n", ch);
            SET_BIT(ch->comm, COMM_NOCGOSSIP);
        }
    }
    else
    {
        /* gossip message sent, turn gossip on if it isn't already */
        if (IS_SET(ch->comm, COMM_QUIET))
        {
            send_to_char("You must turn off quiet mode first.\r\n", ch);
            return;
        }

        if (IS_SET(ch->comm, COMM_NOCHANNELS))
        {
            send_to_char("The gods have revoked your channel priviliges.\r\n", ch);
            return;
        }

        REMOVE_BIT(ch->comm, COMM_NOCGOSSIP);

        argument = make_drunk(argument, ch);
        sendf(ch, "{xYou clan gossip '{R%s{x'\r\n", argument);

        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                !IS_SET(d->character->affected_by, AFF_DEAFEN) &&
                !IS_SET(victim->comm, COMM_NOCGOSSIP) &&
                !IS_SET(victim->comm, COMM_QUIET))
            {
                act_new("{x$n clan gossips '{R$t{x'", ch, argument, d->character, TO_VICT, POS_SLEEPING);
            }
        }
    }
} // end do_cgossip

/*
 * OOC channel for out of character conversation. (no drunk conversion on ooc)
 */
void do_ooc(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET(ch->comm, COMM_NOOOC))
        {
            send_to_char("OOC (out of character) channel is now ON.\r\n", ch);
            REMOVE_BIT(ch->comm, COMM_NOOOC);
        }
        else
        {
            send_to_char("OOC (out of character) channel is now OFF.\r\n", ch);
            SET_BIT(ch->comm, COMM_NOOOC);
        }
    }
    else
    {                            /* gossip message sent, turn gossip on if it isn't already */

        if (IS_SET(ch->comm, COMM_QUIET))
        {
            send_to_char("You must turn off quiet mode first.\r\n", ch);
            return;
        }

        if (IS_SET(ch->comm, COMM_NOCHANNELS))
        {
            send_to_char("The gods have revoked your channel priviliges.\r\n", ch);
            return;
        }

        REMOVE_BIT(ch->comm, COMM_NOOOC);

        sprintf(buf, "{xYou OOC '{C%s{x'\r\n", argument);
        send_to_char(buf, ch);

        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                !IS_SET(d->character->affected_by, AFF_DEAFEN) &&
                !IS_SET(victim->comm, COMM_NOGOSSIP) &&
                !IS_SET(victim->comm, COMM_QUIET))
            {
                act_new("{x$n OOC '{C$t{x'", ch, argument, d->character, TO_VICT, POS_SLEEPING);
            }
        }
    }
} // end do_gossip

/*
 * Grats channel for congratulations (no drunk conversion on grats)
 */
void do_grats(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET(ch->comm, COMM_NOGRATS))
        {
            send_to_char("Grats channel is now ON.\r\n", ch);
            REMOVE_BIT(ch->comm, COMM_NOGRATS);
        }
        else
        {
            send_to_char("Grats channel is now OFF.\r\n", ch);
            SET_BIT(ch->comm, COMM_NOGRATS);
        }
    }
    else
    {                            /* grats message sent, turn grats on if it isn't already */

        if (IS_SET(ch->comm, COMM_QUIET))
        {
            send_to_char("You must turn off quiet mode first.\r\n", ch);
            return;
        }

        if (IS_SET(ch->comm, COMM_NOCHANNELS))
        {
            send_to_char("The gods have revoked your channel priviliges.\r\n", ch);
            return;

        }

        REMOVE_BIT(ch->comm, COMM_NOGRATS);

        sprintf(buf, "{xYou grats '{c%s{x'\r\n", argument);
        send_to_char(buf, ch);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                !IS_SET(victim->comm, COMM_NOGRATS) &&
                !IS_SET(victim->comm, COMM_QUIET))
            {
                act_new("{x$n grats '{c$t{x'",
                    ch, argument, d->character, TO_VICT, POS_SLEEPING);
            }
        }
    }
}

/* RT question channel (no drunk conversion on question/answer) */
void do_question(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET(ch->comm, COMM_NOQUESTION))
        {
            send_to_char("Q/A channel is now ON.\r\n", ch);
            REMOVE_BIT(ch->comm, COMM_NOQUESTION);
        }
        else
        {
            send_to_char("Q/A channel is now OFF.\r\n", ch);
            SET_BIT(ch->comm, COMM_NOQUESTION);
        }
    }
    else
    {                            /* question sent, turn Q/A on if it isn't already */

        if (IS_SET(ch->comm, COMM_QUIET))
        {
            send_to_char("You must turn off quiet mode first.\r\n", ch);
            return;
        }

        if (IS_SET(ch->comm, COMM_NOCHANNELS))
        {
            send_to_char
                ("The gods have revoked your channel priviliges.\r\n", ch);
            return;
        }

        REMOVE_BIT(ch->comm, COMM_NOQUESTION);

        sprintf(buf, "{xYou question '{Y%s{x'\r\n", argument);
        send_to_char(buf, ch);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                !IS_SET(victim->comm, COMM_NOQUESTION) &&
                !IS_SET(victim->comm, COMM_QUIET))
            {
                act_new("{x$n questions '{Y$t{x'",
                    ch, argument, d->character, TO_VICT, POS_SLEEPING);
            }
        }
    }
}

/* RT answer channel - uses same line as questions (no drunk conversion on answer) */
void do_answer(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET(ch->comm, COMM_NOQUESTION))
        {
            send_to_char("Q/A channel is now ON.\r\n", ch);
            REMOVE_BIT(ch->comm, COMM_NOQUESTION);
        }
        else
        {
            send_to_char("Q/A channel is now OFF.\r\n", ch);
            SET_BIT(ch->comm, COMM_NOQUESTION);
        }
    }
    else
    {                            /* answer sent, turn Q/A on if it isn't already */

        if (IS_SET(ch->comm, COMM_QUIET))
        {
            send_to_char("You must turn off quiet mode first.\r\n", ch);
            return;
        }

        if (IS_SET(ch->comm, COMM_NOCHANNELS))
        {
            send_to_char
                ("The gods have revoked your channel priviliges.\r\n", ch);
            return;
        }

        REMOVE_BIT(ch->comm, COMM_NOQUESTION);

        sprintf(buf, "{xYou answer '{Y%s{x'\r\n", argument);
        send_to_char(buf, ch);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                !IS_SET(victim->comm, COMM_NOQUESTION) &&
                !IS_SET(victim->comm, COMM_QUIET))
            {
                act_new("{x$n answers '{Y$t{x'",
                    ch, argument, d->character, TO_VICT, POS_SLEEPING);
            }
        }
    }
}

/* clan channels */
void do_clantalk(CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (!is_clan(ch) || clan_table[ch->clan].independent)
    {
        send_to_char("You aren't in a clan.\r\n", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        if (IS_SET(ch->comm, COMM_NOCLAN))
        {
            send_to_char("Clan channel is now ON\r\n", ch);
            REMOVE_BIT(ch->comm, COMM_NOCLAN);
        }
        else
        {
            send_to_char("Clan channel is now OFF\r\n", ch);
            SET_BIT(ch->comm, COMM_NOCLAN);
        }
        return;
    }

    if (IS_SET(ch->comm, COMM_NOCHANNELS))
    {
        send_to_char("The gods have revoked your channel priviliges.\r\n", ch);
        return;
    }

    if (!IS_NPC(ch) && IS_SET(ch->pcdata->clan_flags, CLAN_EXILE))
    {
        send_to_char("You have been exiled and cannot communicate with your clan.\r\n", ch);
        return;
    }

    REMOVE_BIT(ch->comm, COMM_NOCLAN);

    argument = make_drunk(argument, ch);
    sendf(ch, "{xYou clan '{G%s{x'\r\n", argument);

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING &&
            d->character != ch &&
            is_same_clan(ch, d->character) &&
            !IS_SET(d->character->affected_by, AFF_DEAFEN) &&
            !IS_SET(d->character->comm, COMM_NOCLAN) &&
            !IS_SET(d->character->comm, COMM_QUIET) &&
            (!IS_NPC(d->character) && !IS_SET(d->character->pcdata->clan_flags, CLAN_EXILE)))
        {
            act_new("{x$n clans '{G$t{x'", ch, argument, d->character, TO_VICT, POS_DEAD);
        }
    }

    return;
}

/*
 * OOC Clan Talk (no drunk conversion on oclan)
 */
void do_oclantalk(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (!is_clan(ch) || clan_table[ch->clan].independent)
    {
        send_to_char("You aren't in a clan.\r\n", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        if (IS_SET(ch->comm, COMM_NOOCLAN))
        {
            send_to_char("OOC Clan channel is now ON\r\n", ch);
            REMOVE_BIT(ch->comm, COMM_NOOCLAN);
        }
        else
        {
            send_to_char("OOC Clan channel is now OFF\r\n", ch);
            SET_BIT(ch->comm, COMM_NOOCLAN);
        }
        return;
    }

    if (IS_SET(ch->comm, COMM_NOCHANNELS))
    {
        send_to_char("The gods have revoked your channel priviliges.\r\n", ch);
        return;
    }

    if (!IS_NPC(ch) && IS_SET(ch->pcdata->clan_flags, CLAN_EXILE))
    {
        send_to_char("You have been exiled and cannot communicate with your clan.\r\n", ch);
        return;
    }

    REMOVE_BIT(ch->comm, COMM_NOOCLAN);

    sprintf(buf, "{xYou OOC clan '{c%s{x'\r\n", argument);
    send_to_char(buf, ch);
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING &&
            d->character != ch &&
            is_same_clan(ch, d->character) &&
            !IS_SET(d->character->affected_by, AFF_DEAFEN) &&
            !IS_SET(d->character->comm, COMM_NOCLAN) &&
            !IS_SET(d->character->comm, COMM_QUIET) &&
            (!IS_NPC(d->character) && !IS_SET(d->character->pcdata->clan_flags, CLAN_EXILE)))
        {
            act_new("{x$n OOC clans '{c$t{x'", ch, argument, d->character, TO_VICT, POS_DEAD);
        }
    }

    return;
} // end oclantalk

void do_immtalk(CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET(ch->comm, COMM_NOWIZ))
        {
            send_to_char("Immortal channel is now ON\r\n", ch);
            REMOVE_BIT(ch->comm, COMM_NOWIZ);
        }
        else
        {
            send_to_char("Immortal channel is now OFF\r\n", ch);
            SET_BIT(ch->comm, COMM_NOWIZ);
        }
        return;
    }

    REMOVE_BIT(ch->comm, COMM_NOWIZ);

    act_new("{C$n: {W$t{x", ch, argument, NULL, TO_CHAR, POS_DEAD);
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING &&
            IS_IMMORTAL(d->character) &&
            !IS_SET(d->character->comm, COMM_NOWIZ))
        {
            act_new("{C$n: {W$t{x", ch, argument, d->character, TO_VICT, POS_DEAD);
        }
    }

    return;
}

/*
 * Prayer channel, a direct line from a mortal to the immortals online. (5/16/2015)
 */
void do_pray(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (IS_SET(ch->comm, COMM_NOPRAY))
    {
        send_to_char("You are allowed to pray currently.\r\n", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        send_to_char("What do you wish to pray?\r\n", ch);
        return;
    }
    else
    {
        sprintf(buf, "{xYou pray '{G%s{x'\r\n", argument);
        send_to_char(buf, ch);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING &&
                victim != ch &&
                IS_IMMORTAL(victim) &&
                !IS_SET(victim->comm, COMM_QUIET))
            {
                act_new("{x$n prays '{G$t{x'", ch, argument, victim, TO_VICT, POS_SLEEPING);
            }
        }
    }
} // end do_pray

/*
 * Allows a player to say something to the awake people in the room who aren't asleep.
 */
void do_say(CHAR_DATA * ch, char *argument)
{
    if (argument[0] == '\0')
    {
        send_to_char("Say what?\r\n", ch);
        return;
    }


    if (ch->in_room != NULL)
    {
        CHAR_DATA *to;

        to = ch->in_room->people;

        argument = make_drunk(argument, ch);
        act_new("{xYou say '{g$T{x'", ch, NULL, argument, TO_CHAR, POS_RESTING);

        for (; to != NULL; to = to->next_in_room)
        {
            if (!IS_SET(to->affected_by, AFF_DEAFEN))
            {
                act_new("{x$n says '{g$t{x'", ch, argument, to, TO_VICT, POS_RESTING);
            }
            else
            {
                act_new("{x$n appears to be saying something but you cannot hear them because you're deaf.", ch, NULL, to, TO_VICT, POS_RESTING);
            }
        }
    }

    if (!IS_NPC(ch))
    {
        CHAR_DATA *mob, *mob_next;
        for (mob = ch->in_room->people; mob != NULL; mob = mob_next)
        {
            mob_next = mob->next_in_room;

            if (IS_NPC(mob) && HAS_TRIGGER(mob, TRIG_SPEECH)
                && mob->position == mob->pIndexData->default_pos)
            {
                mp_act_trigger(argument, mob, ch, NULL, NULL, TRIG_SPEECH);
            }
        }
    }
    return;
}

/*
 * Allows a player to whisper something to the room (more of an RP say to denote inflection).
 */
void do_whisper(CHAR_DATA * ch, char *argument)
{
    if (argument[0] == '\0')
    {
        send_to_char("Whisper what?\r\n", ch);
        return;
    }


    if (ch->in_room != NULL)
    {
        CHAR_DATA *to;

        to = ch->in_room->people;

        argument = make_drunk(argument, ch);
        act_new("{xYou whisper '{c$T{x'", ch, NULL, argument, TO_CHAR, POS_RESTING);

        for (; to != NULL; to = to->next_in_room)
        {
            if (!IS_SET(to->affected_by, AFF_DEAFEN))
            {
                act_new("{x$n whispers '{c$t{x'", ch, argument, to, TO_VICT, POS_RESTING);
            }
            else
            {
                act_new("{x$n appears to be whispering something but you cannot hear them because you're deaf.", ch, NULL, to, TO_VICT, POS_RESTING);
            }
        }
    }

    if (!IS_NPC(ch))
    {
        CHAR_DATA *mob, *mob_next;
        for (mob = ch->in_room->people; mob != NULL; mob = mob_next)
        {
            mob_next = mob->next_in_room;

            if (IS_NPC(mob) && HAS_TRIGGER(mob, TRIG_SPEECH)
                && mob->position == mob->pIndexData->default_pos)
            {
                mp_act_trigger(argument, mob, ch, NULL, NULL, TRIG_SPEECH);
            }
        }
    }

    return;
}

void do_tell(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    if (IS_SET(ch->comm, COMM_NOTELL) || IS_SET(ch->comm, COMM_DEAF))
    {
        send_to_char("Your message didn't get through.\r\n", ch);
        return;
    }

    if (IS_SET(ch->comm, COMM_QUIET))
    {
        send_to_char("You must turn off quiet mode first.\r\n", ch);
        return;
    }

    if (IS_SET(ch->comm, COMM_DEAF))
    {
        send_to_char("You must turn off deaf mode first.\r\n", ch);
        return;
    }

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
        send_to_char("Tell whom what?\r\n", ch);
        return;
    }

    /*
     * Can tell to PC's anywhere, but NPC's only in same room.
     * -- Furey
     */
    if ((victim = get_char_world(ch, arg)) == NULL
        || (IS_NPC(victim) && victim->in_room != ch->in_room))
    {
        send_to_char("They aren't here.\r\n", ch);
        return;
    }

    argument = make_drunk(argument, ch);

    if (victim->desc == NULL && !IS_NPC(victim))
    {
        act("$N seems to have misplaced $S link...try again later.", ch, NULL, victim, TO_CHAR);
        sprintf(buf, "{x%s tells you '{W%s{x'{x\r\n", PERS(ch, victim), argument);
        buf[0] = UPPER(buf[0]);
        add_buf(victim->pcdata->buffer, buf);
        return;
    }

    if (!(IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL)
        && !IS_AWAKE(victim))
    {
        act("$E can't hear you.", ch, 0, victim, TO_CHAR);
        return;
    }

    if (IS_SET(victim->affected_by, AFF_DEAFEN))
    {
        act("$E is deaf and can't hear you currently.", ch, 0, victim, TO_CHAR);
        return;
    }

    if ((IS_SET(victim->comm, COMM_QUIET)
            || IS_SET(victim->comm, COMM_DEAF)) && !IS_IMMORTAL(ch))
    {
        act("$E is not receiving tells.", ch, 0, victim, TO_CHAR);
        return;
    }

    if (IS_SET(victim->comm, COMM_AFK))
    {
        if (IS_NPC(victim))
        {
            act("$E is AFK, and not receiving tells.", ch, NULL, victim, TO_CHAR);
            return;
        }

        act("$E is AFK, but your tell will go through when $E returns.", ch, NULL, victim, TO_CHAR);
        sprintf(buf, "{x%s tells you '{W%s{x'\r\n", PERS(ch, victim), argument);
        buf[0] = UPPER(buf[0]);
        add_buf(victim->pcdata->buffer, buf);
        return;
    }

    act("{xYou tell $N '{W$t{x'", ch, argument, victim, TO_CHAR);
    act_new("{x$n tells you '{W$t{x'", ch, argument, victim, TO_VICT, POS_DEAD);
    victim->reply = ch;

    if (!IS_NPC(ch) && IS_NPC(victim) && HAS_TRIGGER(victim, TRIG_SPEECH))
        mp_act_trigger(argument, victim, ch, NULL, NULL, TRIG_SPEECH);

    return;
}


void do_reply(CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    if (IS_SET(ch->comm, COMM_NOTELL))
    {
        send_to_char("Your message didn't get through.\r\n", ch);
        return;
    }

    if ((victim = ch->reply) == NULL)
    {
        send_to_char("They aren't here.\r\n", ch);
        return;
    }

    argument = make_drunk(argument, ch);

    if (victim->desc == NULL && !IS_NPC(victim))
    {
        act("$N seems to have misplaced $S link...try again later.", ch, NULL, victim, TO_CHAR);
        sprintf(buf, "{x%s tells you '{W%s{x'\r\n", PERS(ch, victim), argument);
        buf[0] = UPPER(buf[0]);
        add_buf(victim->pcdata->buffer, buf);
        return;
    }

    if (!IS_IMMORTAL(ch) && !IS_AWAKE(victim))
    {
        act("$E can't hear you.", ch, 0, victim, TO_CHAR);
        return;
    }

    if ((IS_SET(victim->comm, COMM_QUIET)
            || IS_SET(victim->comm, COMM_DEAF)) && !IS_IMMORTAL(ch)
        && !IS_IMMORTAL(victim))
    {
        act_new("$E is not receiving tells.", ch, 0, victim, TO_CHAR, POS_DEAD);
        return;
    }

    if (!IS_IMMORTAL(victim) && !IS_AWAKE(ch))
    {
        send_to_char("In your dreams, or what?\r\n", ch);
        return;
    }

    if (IS_SET(victim->comm, COMM_AFK))
    {
        if (IS_NPC(victim))
        {
            act_new("$E is AFK, and not receiving tells.", ch, NULL, victim, TO_CHAR, POS_DEAD);
            return;
        }

        act_new("$E is AFK, but your tell will go through when $E returns.", ch, NULL, victim, TO_CHAR, POS_DEAD);
        sprintf(buf, "{x%s tells you '{W%s{x'\r\n", PERS(ch, victim), argument);
        buf[0] = UPPER(buf[0]);
        add_buf(victim->pcdata->buffer, buf);
        return;
    }

    act_new("{xYou tell $N '{W$t{x'", ch, argument, victim, TO_CHAR, POS_DEAD);
    act_new("{x$n tells you '{W$t{x'", ch, argument, victim, TO_VICT, POS_DEAD);
    victim->reply = ch;

    return;
}

void do_yell(CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (IS_SET(ch->comm, COMM_NOSHOUT))
    {
        send_to_char("You can't yell.\r\n", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        send_to_char("Yell what?\r\n", ch);
        return;
    }

    argument = make_drunk(argument, ch);

    act("{xYou yell '{Y$t{x'", ch, argument, NULL, TO_CHAR);

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING
            && d->character != ch
            && d->character->in_room != NULL
            && !IS_SET(d->character->affected_by, AFF_DEAFEN)
            && d->character->in_room->area == ch->in_room->area
            && !IS_SET(d->character->comm, COMM_QUIET))
        {
            act("{x$n yells '{Y$t{x'", ch, argument, d->character, TO_VICT);
        }
    }

    return;
}

void do_emote(CHAR_DATA * ch, char *argument)
{
    if (!IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE))
    {
        send_to_char("You can't show your emotions.\r\n", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        send_to_char("Emote what?\r\n", ch);
        return;
    }

    /* little hack to fix the ',{' bug posted to rom list
     * around 4/16/01 -- JR
     */
    if (!(isalpha(argument[0])) || (isspace(argument[0])))
    {
        send_to_char("You cannot emote that!\r\n", ch);
        return;
    }

    MOBtrigger = FALSE;
    act("{x$n $T", ch, NULL, argument, TO_ROOM);
    act("{x$n $T", ch, NULL, argument, TO_CHAR);
    MOBtrigger = TRUE;
    return;
}


void do_pmote(CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *vch;
    char *letter, *name;
    char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
    int matches = 0;

    if (!IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE))
    {
        send_to_char("You can't show your emotions.\r\n", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        send_to_char("Emote what?\r\n", ch);
        return;
    }

    /* little hack to fix the ',{' bug posted to rom list
     * around 4/16/01 -- JR
     */
    if (!(isalpha(argument[0])) || (isspace(argument[0])))
    {
        send_to_char("You cannot pmote that.\r\n", ch);
        return;
    }

    act("{x$n $t", ch, argument, NULL, TO_CHAR);

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->desc == NULL || vch == ch)
            continue;

        if ((letter = strstr(argument, vch->name)) == NULL)
        {
            MOBtrigger = FALSE;
            act("{x$N $t", vch, argument, ch, TO_CHAR);
            MOBtrigger = TRUE;
            continue;
        }

        strcpy(temp, argument);
        temp[strlen(argument) - strlen(letter)] = '\0';
        last[0] = '\0';
        name = vch->name;

        for (; *letter != '\0'; letter++)
        {
            if (*letter == '\'' && matches == strlen(vch->name))
            {
                strcat(temp, "r");
                continue;
            }

            if (*letter == 's' && matches == strlen(vch->name))
            {
                matches = 0;
                continue;
            }

            if (matches == strlen(vch->name))
            {
                matches = 0;
            }

            if (*letter == *name)
            {
                matches++;
                name++;
                if (matches == strlen(vch->name))
                {
                    strcat(temp, "you");
                    last[0] = '\0';
                    name = vch->name;
                    continue;
                }
                strncat(last, letter, 1);
                continue;
            }

            matches = 0;
            strcat(temp, last);
            strncat(temp, letter, 1);
            last[0] = '\0';
            name = vch->name;
        }

        MOBtrigger = FALSE;
        act("{x$N $t", vch, temp, ch, TO_CHAR);
        MOBtrigger = TRUE;
    }

    return;
}

void do_bug(CHAR_DATA * ch, char *argument)
{
    append_file(ch, BUG_FILE, argument);
    send_to_char("Bug logged.\r\n", ch);
    return;
}

void do_typo(CHAR_DATA * ch, char *argument)
{
    append_file(ch, TYPO_FILE, argument);
    send_to_char("Typo logged.\r\n", ch);
    return;
}

void do_qui(CHAR_DATA * ch, char *argument)
{
    send_to_char("If you want to QUIT, you have to spell it out.\r\n", ch);
    return;
}

void do_quit(CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d, *d_next;
    int id;
    bool main_menu = FALSE;

    if (IS_NPC(ch))
        return;

    if (ch->position == POS_FIGHTING)
    {
        send_to_char("No way! You are fighting.\r\n", ch);
        return;
    }

    if (ch->position < POS_STUNNED)
    {
        send_to_char("You're not DEAD yet.\r\n", ch);
        return;
    }

    // Don't allow a player to quit for a few ticks after they have been involved in player
    // killing.  This will stop a player from fleeing and quitting to get out of battle.
    if (ch->pcdata->pk_timer > 0 && !IS_IMMORTAL(ch))
    {
        send_to_char("You must wait a few moments to quit after fighting another player.\r\n", ch);
        return;
    }

    send_to_char("Alas, all good things must come to an end.\r\n", ch);
    act("$n has left the game.", ch, NULL, NULL, TO_ROOM);

    if (ch->desc != NULL && ch->desc->host != NULL)
    {
        log_f("%s@%s has quit.", ch->name, ch->desc->host);
    }
    else
    {
        log_f("%s has quit.", ch->name);
    }

    wiznet("$N rejoins the real world.", ch, NULL, WIZ_LOGINS, 0, get_trust(ch));

    // If they specify that they want to go back to the main menu leave them connnected, log
    // them out properly and then shuttle them there without disconnecting them.
    if (!str_cmp(argument, "menu"))
    {
        send_to_char("\r\n{G[{WPush Enter to Continue{G]{x\r\n", ch);
        main_menu = TRUE;
    }

    /*
     * After extract_char the ch is no longer valid!
     */
    save_char_obj(ch);

    id = ch->id;
    d = ch->desc;
    extract_char(ch, TRUE);

    // If they're going to the main menu set the connected state for it
    // otherwise we need to close the socket to disconnect them.
    if (main_menu && d != NULL)
    {
        d->connected = CON_LOGIN_MENU;
    }
    else
    {
        if (d != NULL)
        {
            close_socket(d);
        }
    }

    /* toast evil cheating bastards */
    for (d = descriptor_list; d != NULL; d = d_next)
    {
        CHAR_DATA *tch;

        d_next = d->next;
        tch = d->original ? d->original : d->character;
        if (tch && tch->id == id)
        {
            extract_char(tch, TRUE);
            close_socket(d);
        }
    }

    return;
}



/*
 * Allows a character to save their pfile.
 */
void do_save(CHAR_DATA * ch, char *argument)
{
    if (IS_NPC(ch))
    {
        return;
    }

    save_char_obj(ch);

    if (!IS_NULLSTR(settings.mud_name))
    {
        sendf(ch, "Saving.  Remember that %s has automatic saving.\r\n", settings.mud_name);
    }
    else
    {
        send_to_char("Saving. Remember that this game has automatic saving.\r\n", ch);
    }

    // Add lag on save, but on if it's not an immortal
    if (!IS_IMMORTAL(ch))
    {
        WAIT_STATE(ch, 4 * PULSE_VIOLENCE);
    }

    return;
}



void do_follow(CHAR_DATA * ch, char *argument)
{
/* RT changed to allow unlimited following and follow the NOFOLLOW rules */
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Follow whom?\r\n", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\r\n", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL)
    {
        act("But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR);
        return;
    }

    if (victim == ch)
    {
        if (ch->master == NULL)
        {
            send_to_char("You already follow yourself.\r\n", ch);
            return;
        }

        stop_follower(ch);
        return;
    }

    if (!IS_NPC(victim) && IS_SET(victim->act, PLR_NOFOLLOW)
        && !IS_IMMORTAL(ch))
    {
        act("$N doesn't seem to want any followers.\r\n", ch, NULL, victim,
            TO_CHAR);
        return;
    }

    REMOVE_BIT(ch->act, PLR_NOFOLLOW);

    if (ch->master != NULL)
        stop_follower(ch);

    add_follower(ch, victim);
    return;
}


void add_follower(CHAR_DATA * ch, CHAR_DATA * master)
{
    if (ch->master != NULL)
    {
        bug("Add_follower: non-null master.", 0);
        return;
    }

    ch->master = master;
    ch->leader = NULL;

    if (can_see(master, ch))
        act("$n now follows you.", ch, NULL, master, TO_VICT);

    act("You now follow $N.", ch, NULL, master, TO_CHAR);

    return;
}



void stop_follower(CHAR_DATA * ch)
{
    if (ch->master == NULL)
    {
        bug("Stop_follower: null master.", 0);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM))
    {
        REMOVE_BIT(ch->affected_by, AFF_CHARM);
        affect_strip(ch, gsn_charm_person);
    }

    // Guardian Angel
    if (IS_NPC(ch) && ch->pIndexData->vnum == VNUM_GUARDIAN_ANGEL)
    {
        affect_strip(ch->master, gsn_guardian_angel);
        affect_strip(ch, gsn_guardian_angel);
    }

    if (can_see(ch->master, ch) && ch->in_room != NULL)
    {
        act("$n stops following you.", ch, NULL, ch->master, TO_VICT);
        act("You stop following $N.", ch, NULL, ch->master, TO_CHAR);
    }
    if (ch->master->pet == ch)
        ch->master->pet = NULL;

    ch->master = NULL;
    ch->leader = NULL;
    return;
}

/* nukes charmed monsters and pets */
void nuke_pets(CHAR_DATA * ch)
{
    CHAR_DATA *pet;

    if ((pet = ch->pet) != NULL)
    {
        stop_follower(pet);
        if (pet->in_room != NULL)
            act("$N slowly fades away.", ch, NULL, pet, TO_NOTVICT);
        extract_char(pet, TRUE);
    }
    ch->pet = NULL;

    return;
}


void die_follower(CHAR_DATA * ch)
{
    CHAR_DATA *fch;

    if (ch->master != NULL)
    {
        if (ch->master->pet == ch)
        {
            ch->master->pet = NULL;
        }

        stop_follower(ch);
    }

    ch->leader = NULL;

    for (fch = char_list; fch != NULL; fch = fch->next)
    {
        if (fch->master == ch)
        {
            stop_follower(fch);
        }

        if (fch->leader == ch)
        {
            fch->leader = fch;
        }
    }

    return;
} // end die_follower

/*
 * Order command, where a magic user who has charmed someone can order them
 * to do various things.
 */
void do_order(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *och;
    CHAR_DATA *och_next;
    bool found;
    bool fAll;

    argument = one_argument(argument, arg);
    one_argument(argument, arg2);

    if (!str_prefix(arg2, "del") || !str_cmp(arg2, "mob") || !str_cmp(arg2, "consent") || !str_prefix(arg2, "drop"))
    {
        send_to_char("That will NOT be done.\r\n", ch);
        return;
    }

    if (arg[0] == '\0' || argument[0] == '\0')
    {
        send_to_char("Order whom to do what?\r\n", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM))
    {
        send_to_char("You feel like taking, not giving, orders.\r\n", ch);
        return;
    }

    if (!str_cmp(arg, "all"))
    {
        fAll = TRUE;
        victim = NULL;
    }
    else
    {
        fAll = FALSE;
        if ((victim = get_char_room(ch, arg)) == NULL)
        {
            send_to_char("They aren't here.\r\n", ch);
            return;
        }

        if (victim == ch)
        {
            send_to_char("Aye aye, right away!\r\n", ch);
            return;
        }

        if (!IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch
            || (IS_IMMORTAL(victim) && victim->trust >= ch->trust))
        {
            send_to_char("Do it yourself!\r\n", ch);
            return;
        }
    }

    found = FALSE;
    for (och = ch->in_room->people; och != NULL; och = och_next)
    {
        och_next = och->next_in_room;

        if (IS_AFFECTED(och, AFF_CHARM)
            && och->master == ch && (fAll || och == victim))
        {
            found = TRUE;
            sprintf(buf, "$n orders you to '%s'.", argument);
            act(buf, ch, NULL, och, TO_VICT);
            interpret(och, argument);
        }
    }

    if (found)
    {
        WAIT_STATE(ch, PULSE_VIOLENCE);
        send_to_char("Ok.\r\n", ch);
    }
    else
        send_to_char("You have no followers here.\r\n", ch);
    return;
} // end do_order

/*
 * Group command, will allow you to add and remove players from your
 * group, see your groups condition.  Grouped members can then communicate
 * through gtell.
 */
void do_group(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        CHAR_DATA *gch;
        CHAR_DATA *leader;

        leader = (ch->leader != NULL) ? ch->leader : ch;
        sprintf(buf, "%s's group:\r\n", PERS(leader, ch));
        send_to_char(buf, ch);

        for (gch = char_list; gch != NULL; gch = gch->next)
        {
            if (is_same_group(gch, ch))
            {
                sprintf(buf, "[%2d %s] %-16s {%s%3d{x%% hp {%s%3d{x%% mana {%s%3d{x%% mv\r\n",
                    gch->level,
                    IS_NPC(gch) ? "Mob" : class_table[gch->class]->who_name, capitalize(PERS(gch, ch)),
                    gch->hit < gch->max_hit / 2 ? "R" : gch->hit < gch->max_hit * 3 / 4 ? "Y" : "W",
                    (int)(((float)gch->hit / (float)gch->max_hit) * 100),
                    gch->mana < gch->max_mana / 2 ? "R" : gch->mana < gch->max_mana * 3 / 4 ? "Y" : "W",
                    (int)(((float)gch->mana / (gch->max_mana == 0 ? 1.0 : (float)gch->max_mana)) * 100),
                    gch->move < gch->max_move / 2 ? "R" : gch->move < gch->max_move * 3 / 4 ? "Y" : "W",
                    (int)(((float)gch->move / (float)gch->max_move) * 100));
                send_to_char(buf, ch);
            }
        }

        return;
    }

    if ((victim = get_char_room(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\r\n", ch);
        return;
    }

    if (ch->master != NULL || (ch->leader != NULL && ch->leader != ch))
    {
        send_to_char("But you are following someone else!\r\n", ch);
        return;
    }

    if (victim->master != ch && ch != victim)
    {
        act_new("$N isn't following you.", ch, NULL, victim, TO_CHAR, POS_SLEEPING);
        return;
    }

    if (IS_AFFECTED(victim, AFF_CHARM))
    {
        send_to_char("You can't remove charmed mobs from your group.\r\n", ch);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM))
    {
        act_new("You like your master too much to leave $m!", ch, NULL, victim, TO_VICT, POS_SLEEPING);
        return;
    }

    if (is_same_group(victim, ch) && ch != victim)
    {
        victim->leader = NULL;
        act_new("$n removes $N from $s group.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
        act_new("$n removes you from $s group.", ch, NULL, victim, TO_VICT, POS_SLEEPING);
        act_new("You remove $N from your group.", ch, NULL, victim, TO_CHAR, POS_SLEEPING);
        return;
    }

    if ((abs(victim->level - ch->level) > 8) && !IS_IMMORTAL(ch))
    {
        send_to_char("You cannot group outside of the pkill range.\r\n", ch);
        return;
    }

    victim->leader = ch;
    act_new("$N joins $n's group.", ch, NULL, victim, TO_NOTVICT, POS_RESTING);
    act_new("You join $n's group.", ch, NULL, victim, TO_VICT, POS_SLEEPING);
    act_new("$N joins your group.", ch, NULL, victim, TO_CHAR, POS_SLEEPING);

    return;

} // end do_group

/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void do_split(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    int members;
    int amount_gold = 0, amount_silver = 0;
    int share_gold, share_silver;
    int extra_gold, extra_silver;

    argument = one_argument(argument, arg1);
    one_argument(argument, arg2);

    if (arg1[0] == '\0')
    {
        send_to_char("Split how much?\r\n", ch);
        return;
    }

    amount_silver = atoi(arg1);

    if (arg2[0] != '\0')
        amount_gold = atoi(arg2);

    if (amount_gold < 0 || amount_silver < 0)
    {
        send_to_char("Your group wouldn't like that.\r\n", ch);
        return;
    }

    if (amount_gold == 0 && amount_silver == 0)
    {
        send_to_char("You hand out zero coins, but no one notices.\r\n", ch);
        return;
    }

    if (ch->gold < amount_gold || ch->silver < amount_silver)
    {
        send_to_char("You don't have that much to split.\r\n", ch);
        return;
    }

    members = 0;
    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
    {
        if (is_same_group(gch, ch) && !IS_AFFECTED(gch, AFF_CHARM))
            members++;
    }

    if (members < 2)
    {
        send_to_char("Just keep it all.\r\n", ch);
        return;
    }

    share_silver = amount_silver / members;
    extra_silver = amount_silver % members;

    share_gold = amount_gold / members;
    extra_gold = amount_gold % members;

    if (share_gold == 0 && share_silver == 0)
    {
        send_to_char("Don't even bother, cheapskate.\r\n", ch);
        return;
    }

    ch->silver -= amount_silver;
    ch->silver += share_silver + extra_silver;
    ch->gold -= amount_gold;
    ch->gold += share_gold + extra_gold;

    if (share_silver > 0)
    {
        sprintf(buf,
            "You split %d silver coins. Your share is %d silver.\r\n",
            amount_silver, share_silver + extra_silver);
        send_to_char(buf, ch);
    }

    if (share_gold > 0)
    {
        sprintf(buf,
            "You split %d gold coins. Your share is %d gold.\r\n",
            amount_gold, share_gold + extra_gold);
        send_to_char(buf, ch);
    }

    if (share_gold == 0)
    {
        sprintf(buf, "$n splits %d silver coins. Your share is %d silver.",
            amount_silver, share_silver);
    }
    else if (share_silver == 0)
    {
        sprintf(buf, "$n splits %d gold coins. Your share is %d gold.",
            amount_gold, share_gold);
    }
    else
    {
        sprintf(buf,
            "$n splits %d silver and %d gold coins, giving you %d silver and %d gold.\r\n",
            amount_silver, amount_gold, share_silver, share_gold);
    }

    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
    {
        if (gch != ch && is_same_group(gch, ch)
            && !IS_AFFECTED(gch, AFF_CHARM))
        {
            act(buf, ch, NULL, gch, TO_VICT);
            gch->gold += share_gold;
            gch->silver += share_silver;
        }
    }

    return;
}

/*
 * Group talk, e.g. group tells.  All group members will see these awake
 * or asleep.
 */
void do_gtell(CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *gch;

    if (argument[0] == '\0')
    {
        send_to_char("Tell your group what?\r\n", ch);
        return;
    }

    if (IS_SET(ch->comm, COMM_NOTELL))
    {
        send_to_char("Your message didn't get through!\r\n", ch);
        return;
    }

    argument = make_drunk(argument, ch);

    act("{xYou tell the group '{C$T{x'", ch, NULL, argument, TO_CHAR);

    for (gch = char_list; gch != NULL; gch = gch->next)
    {
        if (is_same_group(gch, ch))
        {
            act_new("{x$n tells the group '{C$t{x'", ch, argument, gch, TO_VICT, POS_SLEEPING);
        }
    }

    return;
}

/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group(CHAR_DATA * ach, CHAR_DATA * bch)
{
    if (ach == NULL || bch == NULL)
        return FALSE;

    if (ach->leader != NULL)
        ach = ach->leader;
    if (bch->leader != NULL)
        bch = bch->leader;
    return ach == bch;
}

/*
 * Lopes Color, Oct-94
 */
void do_color(CHAR_DATA * ch, char *argument)
{
    if (IS_NPC(ch))
    {
        send_to_char("Color is not available for mobs or while switched.\r\n", ch);
        return;
    }

    if (!IS_SET(ch->act, PLR_COLOR))
    {
        ch->desc->ansi = TRUE;
        SET_BIT(ch->act, PLR_COLOR);
        send_to_char("{RC{Yo{Bl{Go{Cr{x is now ON, Way Cool!\r\n", ch);
    }
    else
    {
        ch->desc->ansi = FALSE;
        REMOVE_BIT(ch->act, PLR_COLOR);
        send_to_char("Color is now OFF, <sigh>\r\n", ch);
    }
    return;

}

/*
 * Clears the screen using the VT100 escape code (if the client supports it) (4/11/2015)
 */
void do_clear(CHAR_DATA * ch, char *argument)
{
    send_to_char("\033[2J", ch);
}

/*
 * Toggles whether the player wants a line feed sent down every tick.
 */
void do_linefeed(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
    {
        return;
    }

    if (IS_SET(ch->comm, COMM_LINEFEED_TICK))
    {
        send_to_char("You will no longer receive a line feed each tick.\r\n", ch);
        REMOVE_BIT(ch->comm, COMM_LINEFEED_TICK);
    }
    else
    {
        send_to_char("You will get a line feed each tick.\r\n",ch);
        SET_BIT(ch->comm, COMM_LINEFEED_TICK);
    }
}

/*
 * Sends a line feed to everyone who has COMM_LINEFEED_TICK set.  This should be
 * called on the tick.
 */
void linefeed_update()
{
    CHAR_DATA *ch;

    for (ch = char_list; ch != NULL; ch = ch->next)
    {
        if (!IS_NPC(ch) && ch->fighting == NULL && IS_SET(ch->comm, COMM_LINEFEED_TICK))
        {
            send_to_char("\r\n", ch);
        }
    }
}

/*
 * Directs a say towards a specific individual.  The room, the sayer and the
 * directed person will all get the property direction of where the say is
 * directed.
 */
void do_direct(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument(argument, arg);

    if (argument[0] == '\0' || arg[0] == '\0')
    {
        send_to_char("Say what and to whom?\r\n", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg)) == NULL)
    {
        send_to_char("They arent here.\r\n", ch);
        return;
    }

    argument = make_drunk(argument, ch);

    sprintf(buf, "{x$n says (to {g$N{x) '{g%s{x'", argument);
    act(buf, ch, NULL, victim, TO_NOTVICT);

    sprintf(buf, "{xYou say (to {g$N{x) '{g%s{x'", argument);
    act(buf, ch, NULL, victim, TO_CHAR);

    if (!IS_SET(victim->affected_by, AFF_DEAFEN))
    {
        sprintf(buf, "{x$n says (to {gYou{x) '{g%s{x'", argument);
        act(buf, ch, NULL, victim, TO_VICT);
    }
    else
    {
        // They're currently deaf
        act("{x$n appears to be saying something to you but you cannot hear them because you're deaf.", ch, NULL, victim, TO_VICT);
    }

    // This was snagged from do_say
    if (!IS_NPC(ch))
    {
        CHAR_DATA *mob, *mob_next;
        for (mob = ch->in_room->people; mob != NULL; mob = mob_next)
        {
            mob_next = mob->next_in_room;
            if (IS_NPC(mob) && HAS_TRIGGER(mob, TRIG_SPEECH)
                && mob->position == mob->pIndexData->default_pos)
                mp_act_trigger(argument, mob, ch, NULL, NULL, TRIG_SPEECH);
        }
    }

    return;

} // end do_direct

/*
 * Grants a single player (at a time) consent to perform an action, such as guild you.
 */
void do_consent(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;

    if (IS_NPC(ch))
        return;

    if (!str_cmp(argument, "view"))
    {
        if (ch->pcdata->consent == NULL)
        {
            send_to_char("You have not consented to anyone.\r\n", ch);
            return;
        }
        else
        {
            sendf(ch, "You have granted consent to %s.\r\n", ch->pcdata->consent->name);
            return;
        }
    }

    if (IS_AFFECTED(ch, AFF_CHARM))
    {
        send_to_char("You cannot give someone consent while charmed.\r\n", ch);
        return;
    }

    if (IS_NULLSTR(argument))
    {
        send_to_char("Whom do you wish to give consent to?\r\n", ch);
        return;
    }

    // Ability to clear the consent.
    if (!str_cmp(argument, "clear"))
    {
        send_to_char("You have removed any consent you had given.\r\n", ch);
        ch->pcdata->consent = NULL;
        return;
    }

    if ((victim = get_char_world(ch, argument)) == NULL
        || IS_NPC(victim))
    {
        send_to_char("They are not here.\r\n", ch);
        return;
    }

    // Clear it if the victim is the person
    if (victim == ch)
    {
        send_to_char("You have removed any consent you had given.\r\n", ch);
        ch->pcdata->consent = NULL;
        return;
    }

    // Set the consent pointer (very important, if the victim logs out, in the process
    // of freeing the character it should dereference this pointer)
    ch->pcdata->consent = victim;

    sendf(ch, "You have given %s consent.\r\n", victim->name);
    sendf(victim, "%s has given you consent.\r\n", ch->name);
}

/*
 * Go through all the players and remove any consent to the player provided here (this should
 * be called when a player is removed from the world).  This has to be done when a player leaves
 * the world otherwise an invalid memory location might be referenced afterwards.
 */
void clear_consents(CHAR_DATA *ch)
{
    DESCRIPTOR_DATA *d;

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        CHAR_DATA *wch;

        if (d->character == NULL)
        {
            continue;
        }

        if (IS_NPC(d->character))
        {
            continue;
        }

        wch = (d->original != NULL) ? d->original : d->character;

        if (wch->pcdata->consent == ch)
        {
            wch->pcdata->consent = NULL;
        }
    }
}

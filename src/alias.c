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
#include "recycle.h"

/*
 * Performs the aliasing and other fun stuff.
 */
void substitute_alias(DESCRIPTOR_DATA * d, char *argument)
{
    CHAR_DATA *ch;
    char buf[MAX_STRING_LENGTH], prefix[MAX_INPUT_LENGTH],
        name[MAX_INPUT_LENGTH];
    char *point;
    int alias;

    ch = d->original ? d->original : d->character;

    /* check for prefix */
    if (ch->prefix[0] != '\0' && str_prefix("prefix", argument))
    {
        if (strlen(ch->prefix) + strlen(argument) > MAX_INPUT_LENGTH - 2)
            send_to_char("Line to long, prefix not processed.\r\n", ch);
        else
        {
            sprintf(prefix, "%s %s", ch->prefix, argument);
            argument = prefix;
        }
    }

    if (IS_NPC(ch) || ch->pcdata->alias[0] == NULL
        || !str_prefix("alias", argument) || !str_prefix("una", argument)
        || !str_prefix("prefix", argument))
    {
        interpret(d->character, argument);
        return;
    }

    strcpy(buf, argument);

    for (alias = 0; alias < MAX_ALIAS; alias++)
    {                            /* go through the aliases */
        if (ch->pcdata->alias[alias] == NULL)
            break;

        if (!str_prefix(ch->pcdata->alias[alias], argument))
        {
            point = one_argument(argument, name);
            if (!strcmp(ch->pcdata->alias[alias], name))
            {
                /* More Edwin inspired fixes. JR -- 10/15/00 */
                buf[0] = '\0';
                strcat(buf, ch->pcdata->alias_sub[alias]);
                if (point[0])
                {
                    strcat(buf, " ");
                    strcat(buf, point);
                }

                if (strlen(buf) > MAX_INPUT_LENGTH - 1)
                {
                    send_to_char
                        ("Alias substitution too long. Truncated.\r\n", ch);
                    buf[MAX_INPUT_LENGTH - 1] = '\0';
                }
                break;
            }
        }
    }
    interpret(d->character, buf);
} // end void substitute_alias

/*
 * Function to force the player to fully type out alias.
 */
void do_alia(CHAR_DATA * ch, char *argument)
{
    send_to_char("I'm sorry, alias must be entered in full.\r\n", ch);
    return;
} // end do_alia

/*
 * Command to allow a player to set one of their aliases.
 */
void do_alias(CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *rch;
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    int pos;

    smash_tilde(argument);

    if (ch->desc == NULL)
        rch = ch;
    else
        rch = ch->desc->original ? ch->desc->original : ch;

    if (IS_NPC(rch))
        return;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        if (rch->pcdata->alias[0] == NULL)
        {
            send_to_char("You have no aliases defined.\r\n", ch);
            return;
        }

        send_to_char("Your current aliases are:\r\n", ch);

        for (pos = 0; pos < MAX_ALIAS; pos++)
        {
            if (rch->pcdata->alias[pos] == NULL
                || rch->pcdata->alias_sub[pos] == NULL)
                break;

            sprintf(buf, "  %-15s:  %s\r\n", rch->pcdata->alias[pos],
                rch->pcdata->alias_sub[pos]);
            send_to_char(buf, ch);
        }

        sprintf(buf, "\r\nYou are currently using %d of your %d allotted aliases.\r\n", pos, MAX_ALIAS);
        send_to_char(buf, ch);

        return;
    }

    if (!str_prefix("una", arg) || !str_cmp("alias", arg))
    {
        send_to_char("Sorry, that word is reserved.\r\n", ch);
        return;
    }

    /* More Edwin-inspired fixes. JR -- 10/15/00 */
    if (strchr(arg, ' ') || strchr(arg, '"') || strchr(arg, '\''))
    {
        send_to_char("The word to be aliased should not contain a space, a tick or a double-quote\r\n", ch);
        return;
    }

    if (strlen(arg) > 15)
    {
        send_to_char("The alias name can be no longer than 15 characters.\r\n", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        for (pos = 0; pos < MAX_ALIAS; pos++)
        {
            if (rch->pcdata->alias[pos] == NULL
                || rch->pcdata->alias_sub[pos] == NULL)
                break;

            if (!str_cmp(arg, rch->pcdata->alias[pos]))
            {
                sprintf(buf, "%s aliases to '%s'.\r\n",
                    rch->pcdata->alias[pos],
                    rch->pcdata->alias_sub[pos]);
                send_to_char(buf, ch);
                return;
            }
        }

        send_to_char("That alias is not defined.\r\n", ch);
        return;
    }

    if (!str_prefix(argument, "delete") || !str_prefix(argument, "prefix"))
    {
        send_to_char("That shall not be done!\r\n", ch);
        return;
    }

    for (pos = 0; pos < MAX_ALIAS; pos++)
    {
        if (rch->pcdata->alias[pos] == NULL)
            break;

        if (!str_cmp(arg, rch->pcdata->alias[pos]))
        {                        /* redefine an alias */
            free_string(rch->pcdata->alias_sub[pos]);
            rch->pcdata->alias_sub[pos] = str_dup(argument);
            sprintf(buf, "%s is now realiased to '%s'.\r\n", arg, argument);
            send_to_char(buf, ch);
            return;
        }
    }

    if (pos >= MAX_ALIAS)
    {
        send_to_char("Sorry, you have reached the alias limit.\r\n", ch);
        return;
    }

    /* make a new alias */
    rch->pcdata->alias[pos] = str_dup(arg);
    rch->pcdata->alias_sub[pos] = str_dup(argument);
    sprintf(buf, "%s is now aliased to '%s'.\r\n", arg, argument);
    send_to_char(buf, ch);
} // end do_alias

/*
 * Command to allow a player to remove one of their aliases
 */
void do_unalias(CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *rch;
    char arg[MAX_INPUT_LENGTH];
    int pos;
    bool found = FALSE;

    if (ch->desc == NULL)
        rch = ch;
    else
        rch = ch->desc->original ? ch->desc->original : ch;

    if (IS_NPC(rch))
        return;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Unalias what?\r\n", ch);
        return;
    }

    for (pos = 0; pos < MAX_ALIAS; pos++)
    {
        if (rch->pcdata->alias[pos] == NULL)
            break;

        if (found)
        {
            rch->pcdata->alias[pos - 1] = rch->pcdata->alias[pos];
            rch->pcdata->alias_sub[pos - 1] = rch->pcdata->alias_sub[pos];
            rch->pcdata->alias[pos] = NULL;
            rch->pcdata->alias_sub[pos] = NULL;
            continue;
        }

        if (!strcmp(arg, rch->pcdata->alias[pos]))
        {
            send_to_char("Alias removed.\r\n", ch);
            free_string(rch->pcdata->alias[pos]);
            free_string(rch->pcdata->alias_sub[pos]);
            rch->pcdata->alias[pos] = NULL;
            rch->pcdata->alias_sub[pos] = NULL;
            found = TRUE;
        }
    }

    if (!found)
    {
        send_to_char("No alias of that name to remove.\r\n", ch);
    }

} // end do_unalias


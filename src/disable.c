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
*  Disable Module                                                          *
*                                                                          *
*  Written by:  Erwin S. Andreasen                                         *
*               http://www.andreasen.org                                   *
*                                                                          *
*  This snippet was written by Erwin S. Andreasen, erwin@andreasen.org.    *
*  You may use this code freely, as long as you retain my name in all of   *
*  the files. You also have to mail me telling that you are using it. I am *
*  giving this, hopefully useful, piece of source code to you for free,    *
*  and all I require from you is some feedback.                            *
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
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"

/*
 * Command to disable other commands that a higher level immortal may want/need to disable.
 */
void do_disable(CHAR_DATA *ch, char *argument)
{
    int i;
    DISABLED_DATA *p, *q;

    if (IS_NPC(ch))
    {
        send_to_char("RETURN first.\r\n", ch);
        return;
    }

    if (!argument[0]) /* Nothing specified. Show disabled commands. */
    {
        if (!disabled_first) /* Any disabled at all ? */
        {
            send_to_char("There are no commands disabled.\r\n", ch);
            return;
        }

        send_to_char("--------------------------------------------------------------------------------\r\n", ch);
        send_to_char("{WDisabled Command      Level   Disabled by{x\r\n", ch);
        send_to_char("--------------------------------------------------------------------------------\r\n", ch);


        for (p = disabled_first; p; p = p->next)
        {
            sendf(ch, "%-21s %5d   %-12s\r\n", p->command->name, p->level, p->disabled_by);
        }

        return;
    }

    /* command given */

    /* First check if it is one of the disabled commands */
    for (p = disabled_first; p; p = p->next)
    {
        if (!str_cmp(argument, p->command->name))
        {
            break;
        }
    }

    if (p) /* this command is disabled */
    {
        /*
         * Optional: The level of the imm to enable the command must equal or exceed level
         * of the one that disabled it
         */
        if (get_trust(ch) < p->level)
        {
            send_to_char("This command was disabled by a higher power.\r\n", ch);
            return;
        }

        /* Remove */
        if (disabled_first == p) /* node to be removed == head ? */
        {
            disabled_first = p->next;
        }
        else /* Find the node before this one */
        {
            for (q = disabled_first; q->next != p; q = q->next); /*empty for */
            q->next = p->next;
        }

        free_string(p->disabled_by); /* free name of disabler */
        free_mem(p, sizeof(DISABLED_DATA)); /* free node */
        save_disabled(); /* save to disk */
        send_to_char("Command enabled.\r\n", ch);
    }
    else /* not a disabled command, check if that command exists */
    {
        /* IQ test */
        if (!str_cmp(argument, "disable"))
        {
            send_to_char("You cannot disable the disable command.\r\n", ch);
            return;
        }

        /* Search for the command */
        for (i = 0; cmd_table[i].name[0] != '\0'; i++)
        {
            if (!str_cmp(cmd_table[i].name, argument))
            {
                break;
            }
        }

        /* Found? */
        if (cmd_table[i].name[0] == '\0')
        {
            send_to_char("No such command.\r\n", ch);
            return;
        }

        /* Can the imm use this command at all ? */
        if (cmd_table[i].level > get_trust(ch))
        {
            send_to_char("You don't have access to that command; you cannot disable it.\r\n", ch);
            return;
        }

        /* Disable the command */
        p = alloc_mem(sizeof(DISABLED_DATA));

        p->command = &cmd_table[i];
        p->disabled_by = str_dup(ch->name); /* save name of disabler */
        p->level = get_trust(ch); /* save trust */
        p->next = disabled_first;
        disabled_first = p; /* add before the current first element */

        send_to_char("Command disabled.\r\n", ch);
        save_disabled(); /* save to disk */
    }
}

/*
 * Check if that command is disabled
 *  Note that we check for equivalence of the do_fun pointers; this means
 *  that disabling 'chat' will also disable the '.' command
 */
bool check_disabled(const struct cmd_type *command)
{
    DISABLED_DATA *p;

    for (p = disabled_first; p; p = p->next)
    {
        if (p->command->do_fun == command->do_fun)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * Load disabled commands
 */
void load_disabled()
{
    FILE *fp;
    DISABLED_DATA *p;
    char *name;
    int i;

    disabled_first = NULL;

    fp = fopen(DISABLED_FILE, "r");

    if (!fp) /* No disabled file.. no disabled commands : */
    {
        global.last_boot_result = MISSING;
        return;
    }

    name = fread_word(fp);

    while (str_cmp(name, "END"))
    {
        /* Find the command in the table */
        for (i = 0; cmd_table[i].name[0]; i++)
        {
            if (!str_cmp(cmd_table[i].name, name))
            {
                break;
            }
        }

        if (!cmd_table[i].name[0]) /* command does not exist? */
        {
            bug("Skipping uknown command in " DISABLED_FILE " file.", 0);
            fread_number(fp); /* level */
            fread_word(fp); /* disabled_by */
        }
        else /* add new disabled command */
        {
            p = alloc_mem(sizeof(DISABLED_DATA));
            p->command = &cmd_table[i];
            p->level = fread_number(fp);
            p->disabled_by = str_dup(fread_word(fp));
            p->next = disabled_first;

            disabled_first = p;
        }

        name = fread_word(fp);
    }

    fclose(fp);

    global.last_boot_result = TRUE;
    return;
}

/*
 * Save disabled commands
 */
void save_disabled()
{
    FILE *fp;
    DISABLED_DATA *p;

    if (!disabled_first) /* delete file if no commands are disabled */
    {
#if defined(_WIN32)
        // In MS C rename will fail if the file exists (not so on POSIX).  In Windows, it will never
        // save past the first pfile save if this isn't done.
        _unlink(DISABLED_FILE);
#else
        unlink(DISABLED_FILE);
#endif
        return;
    }

    fp = fopen(DISABLED_FILE, "w");

    if (!fp)
    {
        bug("Could not open " DISABLED_FILE " for writing", 0);
        return;
    }

    for (p = disabled_first; p; p = p->next)
    {
        fprintf(fp, "%s %d %s\n", p->command->name, p->level, p->disabled_by);
    }

    fprintf(fp, "%s\n", "END");
    fclose(fp);
}

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

BAN_DATA *ban_list;

/*
 * Saves the currently banned sites out to the system directory so they can
 * be persisted across boots.
 */
void save_bans(void)
{
    BAN_DATA *pban;
    FILE *fp;
    bool found = FALSE;

    fclose(fpReserve);
    if ((fp = fopen(BAN_FILE, "w")) == NULL)
    {
        perror(BAN_FILE);
    }

    for (pban = ban_list; pban != NULL; pban = pban->next)
    {
        if (IS_SET(pban->ban_flags, BAN_PERMANENT))
        {
            found = TRUE;
            fprintf(fp, "%-20s %-2d %s\n", pban->name, pban->level, print_flags(pban->ban_flags));
        }
    }

    fclose(fp);
    fpReserve = fopen(NULL_FILE, "r");

    if (!found) {
#if defined(_WIN32)
        _unlink(BAN_FILE);
#else
        unlink(BAN_FILE);
#endif
    }

} // end save_bans

/*
 * Loads the ban file from the system directory.
 */
void load_bans(void)
{
    FILE *fp;
    BAN_DATA *ban_last;

    if ((fp = fopen(BAN_FILE, "r")) == NULL)
    {
        global.last_boot_result = MISSING;
        log_string("STATUS: No ban file available to load in the system directory.");
        return;
    }

    ban_last = NULL;
    for (;;)
    {
        BAN_DATA *pban;
        if (feof(fp))
        {
            if (global.last_boot_result == UNKNOWN)
            {
                global.last_boot_result = SUCCESS;
            }

            fclose(fp);
            return;
        }

        pban = new_ban();

        pban->name = str_dup(fread_word(fp));
        pban->level = fread_number(fp);
        pban->ban_flags = fread_flag(fp);
        fread_to_eol(fp);

        if (ban_list == NULL)
        {
            ban_list = pban;
        }
        else
        {
            ban_last->next = pban;
        }
        ban_last = pban;
    }

    if (global.last_boot_result == UNKNOWN)
        global.last_boot_result = SUCCESS;

} // end load_bans

/*
 * Checks to see whether a site is banned.
 */
bool check_ban(char *site, int type)
{
    BAN_DATA *pban;
    char host[MAX_STRING_LENGTH];

    strcpy(host, capitalize(site));
    host[0] = LOWER(host[0]);

    if (settings.whitelist_lock)
    {
        // The whitelist ban overrides regular bans and only works if the global setting for it
        // is enabled.  This will only allow IP's that are in the list through as opposed to
        // blocking specific IP's.
        for (pban = ban_list; pban != NULL; pban = pban->next)
        {
            if (!IS_SET(pban->ban_flags, BAN_WHITELIST))
                continue;

            // Exact match (assuming both prefix and suffix)
            if (!str_cmp(pban->name, host))
                return FALSE;

            if (IS_SET(pban->ban_flags, BAN_PREFIX)
                && !str_suffix(pban->name, host))
                return FALSE;

            if (IS_SET(pban->ban_flags, BAN_SUFFIX)
                && !str_prefix(pban->name, host))
                return FALSE;

        }

        return TRUE;
    }
    else
    {
        for (pban = ban_list; pban != NULL; pban = pban->next)
        {
            if (!IS_SET(pban->ban_flags, type))
                continue;

            // Exact match
            if (!str_cmp(pban->name, host))
                return TRUE;

            if (IS_SET(pban->ban_flags, BAN_PREFIX)
                && IS_SET(pban->ban_flags, BAN_SUFFIX)
                && strstr(pban->name, host) != NULL)
                return TRUE;

            if (IS_SET(pban->ban_flags, BAN_PREFIX)
                && !str_suffix(pban->name, host))
                return TRUE;

            if (IS_SET(pban->ban_flags, BAN_SUFFIX)
                && !str_prefix(pban->name, host))
                return TRUE;
        }
    }

    return FALSE;
} // end check_ban


/*
 * Bans a site from being able to login to the mud.
 */
void ban_site(CHAR_DATA * ch, char *argument, bool fPerm)
{
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    char *name;
    BUFFER *buffer;
    BAN_DATA *pban, *prev;
    bool prefix = FALSE, suffix = FALSE;
    int type;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0')
    {
        if (ban_list == NULL)
        {
            send_to_char("No sites banned at this time.\r\n", ch);
            return;
        }

        buffer = new_buf();

        sprintf(buf, "%-38s    %-5s  %-9s  %s\r\n", "Banned Sites", "Level", "Type", "Status");
        add_buf(buffer, buf);
        add_buf(buffer, HEADER);

        for (pban = ban_list; pban != NULL; pban = pban->next)
        {
            sprintf(buf2, "%s%s%s",
                IS_SET(pban->ban_flags, BAN_PREFIX) ? "*" : "",
                pban->name,
                IS_SET(pban->ban_flags, BAN_SUFFIX) ? "*" : "");
            sprintf(buf, "%-38s    %-5d  %-9s  %s\r\n",
                buf2, pban->level,
                IS_SET(pban->ban_flags, BAN_NEWBIES) ? "Newbies" :
                IS_SET(pban->ban_flags, BAN_ALL) ? "All" :
                IS_SET(pban->ban_flags, BAN_WHITELIST) ? "Whitelist" : "",
                IS_SET(pban->ban_flags, BAN_PERMANENT) ? "Permanent" : "Temp");
            add_buf(buffer, buf);
        }

        page_to_char(buf_string(buffer), ch);
        free_buf(buffer);
        return;
    }

    /* find out what type of ban */
    if (arg2[0] == '\0' || !str_prefix(arg2, "all"))
    {
        type = BAN_ALL;
    }
    else if (!str_prefix(arg2, "newbies"))
    {
        type = BAN_NEWBIES;
    }
    else if (!str_prefix(arg2, "whitelist"))
    {
        type = BAN_WHITELIST;
    }
    else
    {
        send_to_char("Acceptable ban types are all, newbies, whitelist.\r\n", ch);
        return;
    }

    name = arg1;

    if (name[0] == '*')
    {
        prefix = TRUE;
        name++;
    }

    if (name[strlen(name) - 1] == '*')
    {
        suffix = TRUE;
        name[strlen(name) - 1] = '\0';
    }

    if (strlen(name) == 0)
    {
        send_to_char("You have to ban SOMETHING.\r\n", ch);
        return;
    }

    prev = NULL;
    for (pban = ban_list; pban != NULL; prev = pban, pban = pban->next)
    {
        if (!str_cmp(name, pban->name))
        {
            if (pban->level > get_trust(ch))
            {
                send_to_char("That ban was set by a higher power.\r\n", ch);
                return;
            }
            else
            {
                if (prev == NULL)
                {
                    ban_list = pban->next;
                }
                else
                {
                    prev->next = pban->next;
                }
                free_ban(pban);
            }
        }
    }

    pban = new_ban();
    pban->name = str_dup(name);
    pban->level = get_trust(ch);

    /* set ban type */
    pban->ban_flags = type;

    if (prefix)
        SET_BIT(pban->ban_flags, BAN_PREFIX);
    if (suffix)
        SET_BIT(pban->ban_flags, BAN_SUFFIX);
    if (fPerm)
        SET_BIT(pban->ban_flags, BAN_PERMANENT);

    pban->next = ban_list;
    ban_list = pban;
    save_bans();

    if (type != BAN_WHITELIST)
    {
        sendf(ch, "%s has been banned.\r\n", pban->name);
        return;
    }
    else
    {
        sendf(ch, "%s has been added to the whitelist.\r\n", pban->name);
        return;
    }

    return;

} // end ban_site

/*
 * Command to ban the site, will pass the information along to the ban
 * method.
 */
void do_ban(CHAR_DATA * ch, char *argument)
{
    ban_site(ch, argument, FALSE);
}

/*
 * This will permanently ban a site.
 */
void do_permban(CHAR_DATA * ch, char *argument)
{
    ban_site(ch, argument, TRUE);
}

/*
 * Unbans a site.
 */
void do_allow(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    BAN_DATA *prev;
    BAN_DATA *curr;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Remove which site from the ban list?\r\n", ch);
        return;
    }

    prev = NULL;
    for (curr = ban_list; curr != NULL; prev = curr, curr = curr->next)
    {
        if (!str_cmp(arg, curr->name))
        {
            if (curr->level > get_trust(ch))
            {
                send_to_char("You are not powerful enough to lift that ban.\r\n", ch);
                return;
            }

            if (prev == NULL)
            {
                ban_list = ban_list->next;
            }
            else
            {
                prev->next = curr->next;
            }

            free_ban(curr);
            sprintf(buf, "Ban on %s lifted.\r\n", arg);
            send_to_char(buf, ch);
            save_bans();
            return;
        }
    }

    send_to_char("Site is not banned.\r\n", ch);
    return;

} // end do_allow

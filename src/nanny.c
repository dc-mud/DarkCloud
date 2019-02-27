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
    #include <io.h>
#else
    #include <sys/types.h>
    #include <sys/time.h>
    #include <time.h>
    #include <unistd.h>  //OLC -- for close read write etc
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>  // sendf
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "sha256.h"

#if defined(_WIN32)
    extern const char echo_off_str[];
    extern const char echo_on_str[];
    extern const char go_ahead_str[];
#endif

#if    defined(unix)
    #include <fcntl.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include "telnet.h"
    extern const char echo_off_str[];
    extern const char echo_on_str[];
    extern const char go_ahead_str[];
#endif

/*
 * Other local functions (OS-independent).
 */
bool check_parse_name(char *name);
bool check_reconnect(DESCRIPTOR_DATA * d, char *name, bool fConn);
bool check_playing(DESCRIPTOR_DATA * d, char *name);
void max_players_check(void);

/*
 * Global variables.
 */
extern DESCRIPTOR_DATA *d_next;        /* Next descriptor in loop  */

/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny(DESCRIPTOR_DATA * d, char *argument)
{
    DESCRIPTOR_DATA *d_old, *d_next;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *ch;
    char *pwdnew;
    char *p;
    int iClass, race, i, weapon;
    bool fOld;
    extern int top_class;

    while (isspace(*argument))
        argument++;

    ch = d->character;

    switch (d->connected)
    {

        default:
            bug("Nanny: bad d->connected %d.", d->connected);
            close_socket(d);
            return;

        case CON_COLOR:
            // There are cases where some clients like telnet try to negotiate, a ASCII 34 ends up getting sent
            // which borks the Y/N check.  If the argument isn't null, it does have the quote and the length contains
            // more than one character then lop off leftmost character with argument++
            if (!IS_NULLSTR(argument) && argument[0] == '\'' && strlen(argument) > 1)
            {
                argument++;
            }

            // Yes or nothing defaults them to yes.
            if (argument[0] == '\0' || UPPER(argument[0]) == 'Y')
            {
                d->ansi = TRUE;
                d->connected = CON_LOGIN_MENU;
                show_greeting(d);
                show_login_menu(d);
                break;
            }

            if (UPPER(argument[0]) == 'N')
            {
                d->ansi = FALSE;
                d->connected = CON_LOGIN_MENU;
                show_greeting(d);
                show_login_menu(d);
                break;
            }
            else
            {
                send_to_desc("Do you want color? (Y/N) ->  ", d);
                return;
            }
        case CON_LOGIN_MENU:
            // There are cases where some clients like telnet try to negotiate, a ASCII 34 ends up getting sent
            // which borks initial input.  If the argument isn't null, it does have the quote and the length contains
            // more than one character then lop off leftmost character with argument++
            if (!IS_NULLSTR(argument) && argument[0] == '\'' && strlen(argument) > 1)
            {
                argument++;
            }

            // Since the login menu is now wrapped in an ASCII graphic that looks like a parchment, we need to
            // push the start of the input down for these menu options 4 rows so it doesn't ackwardly start
            // writing them over pieces of already rendered text.
            sprintf(buf, "%s%s%s", DOWN, DOWN, DOWN);
            write_to_desc(buf, d);

            switch( argument[0] )
            {
                case 'n' : case 'N' :
                    // Check the new lock first before letting the player through
                    if (settings.newlock)
                    {
                        send_to_desc("\r\nThe game is new locked.  Please try again later.\r\n", d);
                        send_to_desc("\r\n{G[{WPush Enter to Continue{G]{x ", d);
                        d->connected = CON_LOGIN_RETURN;  // Make them confirm before showing them the menu again
                        return;
                    }

                    send_to_desc("\r\nBy what name do you wish to be known? ", d);
                    d->connected = CON_NEW_CHARACTER;
                    return;
                case 'p' : case 'P' :
                    send_to_desc("\r\nWhat is your character's name? ", d);
                    d->connected = CON_GET_NAME;
                    return;
                case 'q' : case 'Q' :
                    send_to_desc("\r\nAlas, all good things must come to an end.\r\n", d);
                    close_socket(d);
                    return;
                case 'w' : case 'W' :
                    if (settings.login_who_list_enabled)
                    {
                        show_login_who(d);
                        send_to_desc("\r\n{G[{WPush Enter to Continue{G]{x ", d);
                        d->connected = CON_LOGIN_RETURN;  // Make them confirm before showing them the menu again
                        return;
                    }

                    break;
                case 'c' : case 'C' :
                    show_login_credits(d);
                    send_to_desc("\r\n{G[{WPush Enter to Continue{G]{x ", d);
                    d->connected = CON_LOGIN_RETURN;  // Make them confirm before showing them the menu again
                    return;
                case 'r': case 'R':
                    show_random_names(d);
                    send_to_desc("\r\n{G[{WPush Enter to Continue{G]{x ", d);
                    d->connected = CON_LOGIN_RETURN;  // Make them confirm before showing them the menu again
                    return;
            }

            show_login_menu(d);
            return;
        case CON_LOGIN_RETURN:
            // This will show the login menu and set that state after a previous step has shown the [Press Enter to Continue] prompt.
            // It will allow us to pause before showing the menu after an informative screen off of the menu has been show (like who is
            // currently online.
            show_login_menu(d);
            d->connected = CON_LOGIN_MENU;
            return;
        case CON_GET_NAME:
            // We no longer disconnected someone who enters a blank, we will route them back to the menu
            if (argument[0] == '\0')
            {
                d->connected = CON_LOGIN_MENU;
                show_login_menu(d);
                return;
            }

            // Get the player's name in the format we want it in, e.g. upper case the first character.
            argument[0] = UPPER(argument[0]);

            if (!check_parse_name(argument))
            {
                send_to_desc("Illegal name, try another.\r\nName: ", d);
                return;
            }

            // Load the player (if they exist), set them onto the descriptor
            fOld = load_char_obj(d, argument);
            ch = d->character;

            // Have they been denied?  If so, close the socket which will boot them and then
            // free the resources.
            if (IS_SET(ch->act, PLR_DENY))
            {
                log_f("Denying access to %s@%s.", argument, d->host);
                send_to_desc("\r\nYou are denied access.\r\n", d);
                close_socket(d);
                return;
            }

            // They have been site banned, close the socket, send them away.
            if (check_ban(d->host, BAN_ALL))
            {
                log_f("Denying access to %s@%s.", argument, d->host);
                send_to_desc("\r\nYour site is currently banned.\r\n", d);
                close_socket(d);
                return;
            }

            bool reconnect = FALSE;
            reconnect = check_reconnect(d, argument, FALSE);

            if (fOld || reconnect)
            {
                // Allow them reconnect if wizlocked, but not a new login (e.g. if they dropped link while wizlocked
                // they may reconnect).  Otherwise, check the lock and send them back to the main menu if they're
                // not an immortal.
                if (settings.wizlock && !IS_IMMORTAL(ch) && !reconnect)
                {
                    free_char(d->character);
                    d->character = NULL;

                    send_to_desc("\r\nThe game is locked to all except immortals.  Please try again later.\r\n", d);
                    send_to_desc("\r\n{G[{WPush Enter to Continue{G]{x ", d);
                    d->connected = CON_LOGIN_RETURN;  // Make them confirm before showing them the menu again
                    return;
                }

                // The exist and have passed all checks, ask them for their password.
                send_to_desc("Password: ", d);
                write_to_buffer(d, echo_off_str);
                d->connected = CON_GET_OLD_PASSWORD;
                return;
            }
            else
            {
                // The player doesn't exist, free the char and send them back to the menu.
                free_char(d->character);
                d->character = NULL;

                send_to_desc("\r\nNo character exists with that name, you may create a new character\r\n", d);
                send_to_desc("from the main menu.\r\n", d);

                send_to_desc("\r\n{G[{WPush Enter to Continue{G]{x ", d);
                d->connected = CON_LOGIN_RETURN;  // Make them confirm before showing them the menu again

                return;
            }

            break;
        case CON_NEW_CHARACTER:
            // We no longer disconnected someone who enters a blank, we will route
            // them back to the menu.
            if (argument[0] == '\0')
            {
                d->connected = CON_LOGIN_MENU;
                show_login_menu(d);
                return;
            }

            if (settings.wizlock || settings.newlock)
            {
                send_to_desc("\r\nThe game is currently locked to new players.  Please try again later.\r\n", d);
                send_to_desc("\r\n{G[{WPush Enter to Continue{G]{x ", d);
                d->connected = CON_LOGIN_RETURN;  // Make them confirm before showing them the menu again
                return;
            }

            // Get the character name into the format we want it.
            argument[0] = UPPER(argument[0]);

            // The new lock was checked on the menu option, no need to check it again, we will
            // check the ban though.
            if (check_ban(d->host, BAN_NEWBIES) || check_ban(d->host, BAN_ALL))
            {
                log_f("Denying access to %s@%s.", argument, d->host);
                send_to_desc("\r\nNew players are not allowed from your site.\r\n", d);
                close_socket(d);
                return;
            }

            // Check for straight up illegal/invalid names.
            if (!check_parse_name(argument))
            {
                send_to_desc("Illegal name, try another.\r\nName: ", d);
                return;
            }

            // Does a player with the same name already exist?
            if (player_exists(argument))
            {
                send_to_desc("\r\nA player with that name already exists.\r\n\r\nPlease choose another name: ", d);
                return;
            }

            // fOld should always be false here unless something goes haywire in the load_char_obj.  This will start
            // the setup process by initializing some things in the ch struct.  New Player/Existing player used to
            // be from the same CON state which is why it's done like this.
            fOld = load_char_obj(d, argument);
            ch = d->character;

            if (fOld)
            {
                // Sanity checking
                bugf("A character (%s) existed in load_char_obj check after passing player_exists == false check.", ch->name);
            }

            if (check_reconnect(d, argument, FALSE))
            {
                // More sanity checking
                bugf("A character (%s) existed in check_reconnect after passing player_exists == false check.", ch->name);
                fOld = TRUE;
            }

            if (fOld)
            {
                // Never should fOld be true if they want to creeate a new char, get out.
                send_to_desc("\r\nAn error occurred in the creation process.  This error has been logged, please.\r\n", d);
                send_to_desc("contact the game administrators with further details.\r\n", d);
                send_to_desc("\r\n{G[{WPush Enter to Continue{G]{x ", d);
                d->connected = CON_LOGIN_RETURN;  // Make them confirm before showing them the menu again
                return;
            }
            else
            {
                // New Player
                // The new character has passed all the checks, ask them to confirm the name and
                // then begin the creation process.
                sprintf(buf, "Did I get that right, %s (Y/N)? ", argument);
                send_to_desc(buf, d);
                d->connected = CON_CONFIRM_NEW_NAME;
                return;
            }
            break;

        case CON_GET_OLD_PASSWORD:
#if defined(unix)
            write_to_buffer(d, "\r\n");
#endif

            // We no longer disconnect for a bad password, send them back to the login
            // menu.  If any brute force attacks happen, consider a lag and also a disconnect
            // after so many attempts.
            if (strcmp(sha256_crypt_with_salt(argument, ch->name), ch->pcdata->pwd))
            {
                send_to_desc("Wrong password.\r\n", d);

                // Log the failed login attempt using WIZ_SITES
                sprintf(buf, "%s@%s entered an incorrect password.", ch->name, d->host);
                log_string(buf);
                wiznet(buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));

                // Bad password, reset the char
                if (d->character != NULL)
                {
                    free_char(d->character);
                    d->character = NULL;
                }

                // Turn string echoing back on and send them back to the login menu.
                write_to_buffer(d, echo_on_str);
                send_to_desc("\r\n{G[{WPush Enter to Continue{G]{x ", d);
                d->connected = CON_LOGIN_RETURN;  // Make them confirm before showing them the menu again

                return;
            }

            write_to_buffer(d, echo_on_str);

            if (check_playing(d, ch->name))
                return;

            if (check_reconnect(d, ch->name, TRUE))
                return;

            sprintf(buf, "%s@%s has connected.", ch->name, d->host);
            log_string(buf);
            wiznet(buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));

            if (ch->desc->ansi)
                SET_BIT(ch->act, PLR_COLOR);
            else
                REMOVE_BIT(ch->act, PLR_COLOR);

            if (IS_IMMORTAL(ch))
            {
                do_function(ch, &do_help, "imotd");
                d->connected = CON_READ_IMOTD;
            }
            else
            {
                do_function(ch, &do_help, "motd");
                d->connected = CON_READ_MOTD;
            }
            break;

/* RT code for breaking link */

        case CON_BREAK_CONNECT:
            switch (*argument)
            {
                case 'y':
                case 'Y':
                    for (d_old = descriptor_list; d_old != NULL;
                    d_old = d_next)
                    {
                        d_next = d_old->next;
                        if (d_old == d || d_old->character == NULL)
                            continue;

                        if (str_cmp(ch->name, d_old->original ?
                            d_old->original->name : d_old->
                            character->name))
                            continue;

                        close_socket(d_old);
                    }

                    if (check_reconnect(d, ch->name, TRUE))
                        return;

                    send_to_desc("Reconnect attempt failed.\r\nName: ", d);

                    if (d->character != NULL)
                    {
                        free_char(d->character);
                        d->character = NULL;
                    }

                    d->connected = CON_GET_NAME;
                    break;

                case 'n':
                case 'N':
                    send_to_desc("Name: ", d);
                    if (d->character != NULL)
                    {
                        free_char(d->character);
                        d->character = NULL;
                    }
                    d->connected = CON_GET_NAME;
                    break;

                default:
                    send_to_desc("Please type Y or N? ", d);
                    break;
            }
            break;

        case CON_CONFIRM_NEW_NAME:
            switch (*argument)
            {
                case 'y':
                case 'Y':
                    sprintf(buf, "\r\nGive me a password for %s: %s", ch->name, echo_off_str);
                    send_to_desc(buf, d);
                    d->connected = CON_GET_NEW_PASSWORD;

                    if (ch->desc->ansi)
                    {
                        SET_BIT(ch->act, PLR_COLOR);
                    }

                    break;

                case 'n':
                case 'N':
                    send_to_desc("Ok, what IS it, then? ", d);
                    free_char(d->character);
                    d->character = NULL;
                    d->connected = CON_NEW_CHARACTER;
                    break;

                default:
                    send_to_desc("Please type Yes or No? ", d);
                    break;
            }
            break;

        case CON_GET_NEW_PASSWORD:
#if defined(unix)
            write_to_buffer(d, "\r\n");
#endif

            if (strlen(argument) < 5)
            {
                send_to_desc("Password must be at least five characters long.\r\nPassword: ", d);
                return;
            }
            else if (strlen(argument) > 20)
            {
                send_to_desc("Password must be less than 20 characters long.\r\nPassword: ", d);
                return;
            }

            pwdnew = sha256_crypt_with_salt(argument, ch->name);
            for (p = pwdnew; *p != '\0'; p++)
            {
                if (*p == '~')
                {
                    send_to_desc("New password not acceptable, try again.\r\nPassword: ", d);
                    return;
                }
            }

            free_string(ch->pcdata->pwd);
            ch->pcdata->pwd = str_dup(pwdnew);
            send_to_desc("Please retype password: ", d);
            d->connected = CON_CONFIRM_NEW_PASSWORD;
            break;

        case CON_CONFIRM_NEW_PASSWORD:
#if defined(unix)
            write_to_buffer(d, "\r\n");
#endif

            if (strcmp(sha256_crypt_with_salt(argument, ch->name), ch->pcdata->pwd))
            {
                send_to_desc("Passwords don't match.\r\nRetype password: ", d);
                d->connected = CON_GET_NEW_PASSWORD;
                return;
            }

            // Turn echo'ing back on for asking for the email.
            write_to_buffer(d, echo_on_str);
            d->connected = CON_GET_EMAIL;
            send_to_desc("\r\nWe ask for an optional email address in case the game admin\r\n", d);
            send_to_desc("need to verify your identity to reset your password.  If you do\r\n", d);
            send_to_desc("not want to enter an email simply press enter for a blank address.\r\n\r\n", d);
            send_to_desc("Please enter your email address (optional):", d);

            break;
        case CON_GET_EMAIL:
            if (argument[0] != '\0')
            {
               // Implement some basic error checking.
                if (strlen(argument) > 50)
                {
                    send_to_desc("Email address must be under 50 characters:", d);
                    return;
                }

                if (strstr(argument, "@") == NULL || strstr(argument, ".") == NULL)
                {
                    send_to_desc("Invalid email address.\r\n", d);
                    send_to_desc("Please re-enter your email address (optional):", d);
                    return;
                }

                free_string(ch->pcdata->email);
                ch->pcdata->email = str_dup(argument);
            }

            send_to_desc("The following races are available:\r\n", d);
            for (race = 1; race_table[race].name != NULL; race++)
            {
                if (!race_table[race].pc_race || !pc_race_table[race].player_selectable)
                    break;

                write_to_buffer(d, "  {G*{x ");
                write_to_buffer(d, race_table[race].name);
                write_to_buffer(d, "\r\n");
            }

            write_to_buffer(d, "\r\n");
            send_to_desc("What is your race (help for more information)? ", d);
            d->connected = CON_GET_NEW_RACE;

            break;
        case CON_GET_NEW_RACE:
            one_argument(argument, arg);

            if (!strcmp(arg, "help"))
            {
                argument = one_argument(argument, arg);
                if (argument[0] == '\0')
                    do_function(ch, &do_help, "race help");
                else
                    do_function(ch, &do_help, argument);
                send_to_desc("What is your race (help for more information)? ", d);
                break;
            }

            race = race_lookup(argument);

            if (race == 0 || !race_table[race].pc_race || !pc_race_table[race].player_selectable)
            {
                send_to_desc("\r\n{RThat is not a valid race.{x\r\n", d);
                send_to_desc("The following races are available:\r\n", d);

                for (race = 1; race_table[race].name != NULL; race++)
                {
                    if (!race_table[race].pc_race || !pc_race_table[race].player_selectable)
                        break;

                    write_to_buffer(d, "  {G*{x ");
                    write_to_buffer(d, race_table[race].name);
                    write_to_buffer(d, "\r\n");
                }

                write_to_buffer(d, "\r\n");
                send_to_desc("What is your race? (help for more information) ", d);
                break;
            }

            ch->race = race;

            /* initialize stats */
            for (i = 0; i < MAX_STATS; i++)
            {
                ch->perm_stat[i] = pc_race_table[race].stats[i];
            }

            ch->affected_by = ch->affected_by | race_table[race].aff;
            ch->imm_flags = ch->imm_flags | race_table[race].imm;
            ch->res_flags = ch->res_flags | race_table[race].res;
            ch->vuln_flags = ch->vuln_flags | race_table[race].vuln;
            ch->form = race_table[race].form;
            ch->parts = race_table[race].parts;

            /* add skills */
            for (i = 0; i < 5; i++)
            {
                if (pc_race_table[race].skills[i] == NULL)
                    break;
                group_add(ch, pc_race_table[race].skills[i], FALSE);
            }
            /* add cost */
            ch->pcdata->points = pc_race_table[race].points;
            ch->size = pc_race_table[race].size;

            send_to_desc("What is your sex (M/F)? ", d);
            d->connected = CON_GET_NEW_SEX;
            break;


        case CON_GET_NEW_SEX:
            switch (argument[0])
            {
                case 'm':
                case 'M':
                    ch->sex = SEX_MALE;
                    ch->pcdata->true_sex = SEX_MALE;
                    break;
                case 'f':
                case 'F':
                    ch->sex = SEX_FEMALE;
                    ch->pcdata->true_sex = SEX_FEMALE;
                    break;
                default:
                    send_to_desc("That's not a sex.\r\nWhat IS your sex? ", d);
                    return;
            }

            // reclass
            if (!IS_NULLSTR(settings.mud_name))
            {
                sendf(ch, "\r\n%s has many specialized reclasses although each character\r\n", settings.mud_name);
            }
            else
            {
                send_to_char("\r\nThis game has many specialized reclasses although each character\r\n", ch);
            }

            send_to_char("must start off as one of four base classes (you can reclass as early\r\n", ch);
            send_to_char("level 10).  You will now select your initial base class.\r\n\r\n", ch);

            send_to_char("The following initial classes are available:\r\n", ch);
            for (iClass = 0; iClass < top_class; iClass++)
            {
                if (class_table[iClass]->name == NULL)
                {
                    log_string("BUG: null class");
                    continue;
                }

                // Show only base classes, not reclasses.
                if (class_table[iClass]->is_reclass == FALSE)
                {
                    send_to_char("  {G*{x ", ch);
                    send_to_char(class_table[iClass]->name, ch);
                    send_to_char("\r\n", ch);
                }

            }

            write_to_buffer(d, "\r\n");
            send_to_char("Select an initial class: ", ch);
            d->connected = CON_GET_NEW_CLASS;
            break;

        case CON_GET_NEW_CLASS:
            //reclass
            iClass = class_lookup(argument);

            if (iClass == -1)
            {
                send_to_desc("\r\n{RThat's not a valid class.{x\r\nWhat IS your class? ", d);
                return;
            }
            else if (class_table[iClass]->is_reclass == TRUE)
            {
                send_to_desc("\r\n{RYou must choose a base class.{x\r\nWhat IS your class? ", d);
                return;
            }

            send_to_desc("\r\nYou will be able to choose a specialized class later if\r\n", d);
            send_to_desc("you so choose (you can enter 'help reclass' once you're\r\n", d);
            send_to_desc("completed the creation process for more information.\r\n", d);

            ch->class = iClass;

            sprintf(buf, "%s@%s new player.", ch->name, d->host);
            log_string(buf);
            wiznet("Newbie alert!  $N sighted.", ch, NULL, WIZ_NEWBIE, 0, 0);
            wiznet(buf, NULL, NULL, WIZ_SITES, 0, get_trust(ch));

            write_to_buffer(d, "\r\n");
            send_to_desc("You may be good, neutral, or evil.\r\n", d);
            send_to_desc("Which alignment (G/N/E)? ", d);
            d->connected = CON_GET_ALIGNMENT;
            break;

        case CON_GET_ALIGNMENT:
            switch (argument[0])
            {
                case 'g':
                case 'G':
                    ch->alignment = ALIGN_GOOD;
                    break;
                case 'n':
                case 'N':
                    ch->alignment = ALIGN_NEUTRAL;
                    break;
                case 'e':
                case 'E':
                    ch->alignment = ALIGN_EVIL;
                    break;
                default:
                    send_to_char("That's not a valid alignment.\r\n", ch);
                    send_to_char("Which alignment (G/N/E)? ", ch);
                    return;
            }

            send_to_char("\r\nWould you like to choose a merit for your character for 10cp (Y/N)?", ch);
            d->connected = CON_ASK_MERIT;

            break;
        case CON_ASK_MERIT:
            switch (argument[0])
            {
                case 'y':
                case 'Y':
                    send_to_char("\r\nThe following are valid merits:\r\n", ch);

                    for (i = 0; merit_table[i].name != NULL; i++)
                    {
                        sendf(ch, "  {G*{x %s\r\n", merit_table[i].name);
                    }

                    send_to_char("\r\nPlease choose a merit: ", ch);

                    d->connected = CON_CHOOSE_MERIT;
                    return;
                case 'n':
                case 'N':
                    // No merit, proceed to choose deity
                    send_to_char("\r\nThe following are the deities available for you to follow:\r\n", ch);

                    // Loop through the available deities and show the player only the
                    // ones that they're allowed to take.
                    for (i = 0; deity_table[i].name != NULL; i++)
                    {
                        if (deity_table[i].align != ALIGN_ALL)
                        {
                            if (deity_table[i].align != ch->alignment)
                            {
                                continue;
                            }
                        }

                        sendf(ch, "  {G*{x %s, %s\r\n", deity_table[i].name, deity_table[i].description);
                    }

                    send_to_char("\r\nWhich deity do you wish to follow? ", ch);

                    d->connected = CON_GET_DEITY;
                    return;
                default:
                    // No valid, repeat ask
                    send_to_char("Please choose yes or no ('Y' or 'N').\r\n", ch);
                    return;
            }

            break;
        case CON_CHOOSE_MERIT:
            one_argument(argument, arg);
            i = merit_lookup(argument);

            if (i < 0)
            {
                send_to_char("That's not a valid merit.  Which merit do you want? ", ch);
                return;
            }

            // Add 10 creation points and then add the merit
            ch->pcdata->points += 10;
            add_merit(ch, merit_table[i].merit);

            // Deity is on deck
            send_to_char("\r\n", ch);
            send_to_char("The following are the deities available for you to follow:\r\n", ch);

            // Loop through the available deities and show the player only the
            // ones that they're allowed to take.
            for (i = 0; deity_table[i].name != NULL; i++)
            {
                if (deity_table[i].align != ALIGN_ALL)
                {
                    if (deity_table[i].align != ch->alignment)
                    {
                        continue;
                    }
                }

                sendf(ch, "  {G*{x %s, %s\r\n", deity_table[i].name, deity_table[i].description);
            }

            send_to_char("Which deity do you wish to follow? ", ch);

            d->connected = CON_GET_DEITY;

            return;
        case CON_GET_DEITY:
            one_argument(argument, arg);
            i = deity_lookup(argument);

            // Weed out invalid picks since deity_lookup will return anything (not just what was
            // previously shown.
            if (i < 0 || (deity_table[i].align != ch->alignment && deity_table[i].align != ALIGN_ALL))
            {
                send_to_char("That's not a valid deity for you.\r\n", ch);
                send_to_char("Which deity do you wish to follow? ", ch);
                return;
            }

            // It's valid, set the players deity.
            ch->pcdata->deity = i;

            // Prepare to take all the skills needed, setup default skills
            send_to_char("\r\n", ch);

            // Add the game basics group and the base group for the players class.
            group_add(ch, "rom basics", FALSE);
            group_add(ch, class_table[ch->class]->base_group, FALSE);

            // Skills they get for free
            ch->pcdata->learned[gsn_recall] = 50;

            send_to_char("Do you wish to customize this character?\r\n", ch);
            send_to_char("Customization takes time, but allows a wider range of skills and abilities.\r\n", ch);
            send_to_char("Customize (Y/N)? ", ch);
            d->connected = CON_DEFAULT_CHOICE;

            break;
        case CON_DEFAULT_CHOICE:
            write_to_buffer(d, "\r\n");
            switch (argument[0])
            {
                case 'y':
                case 'Y':
                    ch->gen_data = new_gen_data();
                    ch->gen_data->points_chosen = ch->pcdata->points;

                    send_to_char("\r\nThe following skills and spells are available to your character:\r\n", ch);
                    send_to_char("(this list may be seen again by typing 'list')\r\n\r\n", ch);

                    list_group_costs(ch);

                    // Commented out, they shouldn't have any skills at this point..
                    //write_to_buffer(d, "You already have the following skills:\r\n");
                    //do_function(ch, &do_skills, "");

                    send_to_char("Choice: (list,add,drop,learned,info,help,done)? \r\n", ch);
                    d->connected = CON_GEN_GROUPS;
                    break;
                case 'n':
                case 'N':
                    group_add(ch, class_table[ch->class]->default_group, TRUE);
                    write_to_buffer(d, "\r\n");
                    write_to_buffer(d, "Please pick a weapon from the following choices:\r\n");
                    buf[0] = '\0';

                    for (i = 0; weapon_table[i].name != NULL; i++)
                    {
                        if (ch->pcdata->learned[*weapon_table[i].gsn] > 0)
                        {
                            strcat(buf, weapon_table[i].name);
                            strcat(buf, " ");
                        }
                    }

                    strcat(buf, "\r\nYour choice? ");
                    write_to_buffer(d, buf);
                    d->connected = CON_PICK_WEAPON;
                    break;
                default:
                    write_to_buffer(d, "Please answer (Y/N)? ");
                    return;
            }
            break;

        case CON_PICK_WEAPON:
            write_to_buffer(d, "\r\n");
            weapon = weapon_lookup(argument);
            if (weapon == -1
                || ch->pcdata->learned[*weapon_table[weapon].gsn] <= 0)
            {
                write_to_buffer(d, "That's not a valid selection. Choices are:\r\n");
                buf[0] = '\0';

                for (i = 0; weapon_table[i].name != NULL; i++)
                {
                    if (ch->pcdata->learned[*weapon_table[i].gsn] > 0)
                    {
                        strcat(buf, weapon_table[i].name);
                        strcat(buf, " ");
                    }
                }

                strcat(buf, "\r\nYour choice? ");
                write_to_buffer(d, buf);
                return;
            }

            ch->pcdata->learned[*weapon_table[weapon].gsn] = 40;
            write_to_buffer(d, "\r\n");
            do_function(ch, &do_help, "motd");
            d->connected = CON_READ_MOTD;
            break;

        case CON_GEN_GROUPS:
            send_to_char("\r\n", ch);

            if (!str_cmp(argument, "done"))
            {
                if (ch->pcdata->points == pc_race_table[ch->race].points)
                {
                    send_to_char("You didn't pick anything.\r\n", ch);
                    break;
                }

                if (ch->pcdata->points < 40 + pc_race_table[ch->race].points)
                {
                    sprintf(buf, "You must take at least %d points of skills and groups", 40 + pc_race_table[ch->race].points);
                    send_to_char(buf, ch);
                    break;
                }

                sprintf(buf, "Creation points: %d\r\n", ch->pcdata->points);
                send_to_char(buf, ch);

                //sprintf (buf, "Experience per level: %d\r\n", exp_per_level (ch, ch->gen_data->points_chosen));
                sprintf(buf, "Experience per level: %d\r\n", exp_per_level(ch, ch->pcdata->points));
                send_to_char(buf, ch);

                // Does this check ever fire?
                if (ch->pcdata->points < 40)
                    ch->train = (40 - ch->pcdata->points + 1) / 2;

                free_gen_data(ch->gen_data);
                ch->gen_data = NULL;
                write_to_buffer(d, "\r\n");
                write_to_buffer(d, "Please pick a weapon from the following choices:\r\n");
                buf[0] = '\0';

                for (i = 0; weapon_table[i].name != NULL; i++)
                {
                    if (ch->pcdata->learned[*weapon_table[i].gsn] > 0)
                    {
                        strcat(buf, weapon_table[i].name);
                        strcat(buf, " ");
                    }
                }

                strcat(buf, "\r\nYour choice? ");
                write_to_buffer(d, buf);
                d->connected = CON_PICK_WEAPON;
                break;
            }

            parse_gen_groups(ch, argument);

            send_to_char("Choice: (list,add,drop,learned,info,help,done)? \r\n", ch);

            break;

        case CON_READ_IMOTD:
            write_to_buffer(d, "\r\n");
            do_function(ch, &do_help, "motd");
            d->connected = CON_READ_MOTD;
            break;

        case CON_READ_MOTD:
            if (ch->pcdata == NULL || ch->pcdata->pwd[0] == '\0')
            {
                write_to_buffer(d, "Warning! Null password!\r\n");
                write_to_buffer(d, "Please report old password with bug or contact a game administrator.\r\n");
            }

            if (!IS_NULLSTR(settings.login_greeting))
            {
                sendf(ch, "\r\n%s.\r\n\r\n", settings.login_greeting);
            }
            else if (!IS_NULLSTR(settings.mud_name))
            {
                sendf(ch, "\r\nWelcome to %s.\r\n\r\n", settings.mud_name);
            }

            // If the user is reclassing they will already be in the list, if not, add them.
            if (ch->pcdata->is_reclassing == FALSE)
            {
                ch->next = char_list;
                char_list = ch;
            }

            d->connected = CON_PLAYING;
            reset_char(ch);

            if (ch->level == 0)
            {
                // First time character bits
                SET_BIT(ch->act, PLR_COLOR);
                SET_BIT(ch->comm, COMM_TELNET_GA);
                SET_BIT(ch->act, PLR_AUTOQUIT);

                ch->perm_stat[class_table[ch->class]->attr_prime] += 3;
                ch->perm_stat[class_table[ch->class]->attr_second] += 1;

                ch->level = 1;
                ch->exp = exp_per_level(ch, ch->pcdata->points);
                ch->hit = ch->max_hit;
                ch->mana = ch->max_mana;
                ch->move = ch->max_move;
                ch->train = 3;
                ch->practice = 5;
                ch->gold = 20;

                sprintf(buf, "%s", "the Neophyte");
                set_title(ch, buf);

                // Auto exit for first time players
                SET_BIT(ch->act, PLR_AUTOEXIT);

                // Will set auto loot by default
                SET_BIT(ch->act, PLR_AUTOLOOT);

                do_function(ch, &do_outfit, "");
                obj_to_char(create_object(get_obj_index(OBJ_VNUM_MAP)), ch);

                char_to_room(ch, get_room_index(ROOM_VNUM_SCHOOL));
                send_to_char("\r\n", ch);
                do_function(ch, &do_help, "newbie info");
                send_to_char("\r\n", ch);

                // Incriment the stat for total characters created
                statistics.total_characters++;

            }
            else if (ch->pcdata->is_reclassing == TRUE)
            {
                // Reclass, we need to reset their exp now that they've reclassed so they don't end up
                // with double the exp needed for the first level.
                ch->exp = exp_per_level(ch, ch->pcdata->points);

                // The user is no longer reclassing, set the flag as false so they will save properly.
                ch->pcdata->is_reclassing = FALSE;

                char_from_room(ch);

                // This will transfer the player to their clan hall's altar.  If they aren't
                // in a clan then ch->clan will be 0 and will go to the default altar.
                char_to_room(ch, get_room_index(clan_table[ch->clan].hall));

                // Outfit them with sub issue gear if they need it.
                do_function(ch, &do_outfit, "");
            }
            else if (ch->in_room != NULL)
            {
                char_to_room(ch, ch->in_room);
            }
            else if (IS_IMMORTAL(ch))
            {
                char_to_room(ch, get_room_index(ROOM_VNUM_CHAT));
            }
            else
            {
                char_to_room(ch, get_room_index(ROOM_VNUM_TEMPLE));
            }

            // Apply any keep affects that need to be applied upon login.
            apply_keep_affects_to_char(ch);

            act("$n has entered the game.", ch, NULL, NULL, TO_ROOM);
            do_function(ch, &do_look, "auto");

            // Line break in between the look and the unread for formatting
            send_to_char("\r\n", ch);

            do_unread(ch, "");

            wiznet("$N has left real life behind.", ch, NULL, WIZ_LOGINS, WIZ_SITES, get_trust(ch));

            // Update the statistics for total logins
            statistics.logins++;

            // This will check to see if this login pushes us over the maximum players online
            // and if so will incriment that statistic and then save them.
            max_players_check();

            if (ch->pet != NULL)
            {
                char_to_room(ch->pet, ch->in_room);
                act("$n has entered the game.", ch->pet, NULL, NULL, TO_ROOM);
            }

            // Write the players state to the dss database if db logging is enabled.
            log_player(ch);

            break;
    }

    return;
}

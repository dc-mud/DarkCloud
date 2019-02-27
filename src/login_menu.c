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
 *  Login Menu                                                             *
 ***************************************************************************
 *                                                                         *
 *  Though control flow of the game state is handed in nanny.c, the        *
 *  functions that deal with menu items on the login screen and the        *
 *  rendering of the login screen will go through here.  The options       *
 *  will be fired from the CON_LOGIN_MENU case in nanny.c                  *
 *                                                                         *
 ***************************************************************************/

// System Specific Includes
#if defined(_WIN32)
#include <sys/types.h>
#include <time.h>
#include <io.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>  // OLC -- for close read write etc
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h> // sendf
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"

#define BLANK_MENU_LINE "{w   |                                                                        |\r\n"

/*
* Sends the greeting on login.
*/
void show_greeting(DESCRIPTOR_DATA *d)
{
    extern char *help_greeting;

    if (help_greeting[0] == '.')
    {
        write_to_descriptor(d->descriptor, help_greeting + 1, d);
    }
    else
    {
        write_to_descriptor(d->descriptor, help_greeting, d);
    }
}

/*
* Shows random names simialiar to the do_randomnames command but formatted for
* the login screen.
*/
void show_random_names(DESCRIPTOR_DATA *d)
{
    int row = 0;
    int col = 0;

    show_menu_top(d);
    show_menu_header("Random Names", d);
    write_to_descriptor(d->descriptor, BLANK_MENU_LINE, d);

    for (row = 0; row < 8; row++)
    {
        write_to_descriptor(d->descriptor, "{w   |    {g", d);

        // Since the random function returns a static char we have to use it in
        // separate calls.

        for (col = 0; col < 4; col++)
        {
            writef(d, "%-16s", generate_random_name());
        }

        write_to_descriptor(d->descriptor, "{w    |\r\n", d);
    }

    show_menu_bottom(d);

    return;
}

/*
* Show the credits to the login screen, this is a little hacky.  I would prefer to use the
* help system but it's tooled for CH's and down the line (in the string pager) has issues
* with descriptors on a a straight conversion of page_to_char.
*/
void show_login_credits(DESCRIPTOR_DATA *d)
{
    int space = 0;
    int x = 0;

    show_menu_top(d);
    show_menu_header("Credits", d);
    write_to_descriptor(d->descriptor, BLANK_MENU_LINE, d);

    // This line should dynamically space up to a version number
    writef(d, "{w   |    {G*{x {WDark Cloud{w %s (1998-2019)", VERSION);

    space = 43 - strlen(VERSION);

    for (x = 0; x < space; x++)
    {
        write_to_descriptor(d->descriptor, " ", d);
    }

    write_to_descriptor(d->descriptor, "|\r\n", d);

    write_to_descriptor(d->descriptor, "{w   |    {G*{x {WROM 2.4{w (1993-1998)                                               |\r\n", d);
    write_to_descriptor(d->descriptor, "{w   |          Russ Taylor, Gabrielle Taylor, Brian Moore                    |\r\n", d);
    write_to_descriptor(d->descriptor, "{w   |    {G*{x {WMerc DikuMud{w (1991-1993)                                          |\r\n", d);
    write_to_descriptor(d->descriptor, "{w   |          Michael Chastain, Michael Quan, Mitchel Tse                   |\r\n", d);

    write_to_descriptor(d->descriptor, "{w   |    {G*{x {WDikuMud{w (1993-1998)                                               |\r\n", d);
    write_to_descriptor(d->descriptor, "{w   |          Katja Nyboe, Tom Madsen, Hans Henrik Staerfeldt,              |\r\n", d);
    write_to_descriptor(d->descriptor, "{w   |          Michael Seifert, Sebastian Hammer                             |\r\n", d);

    write_to_descriptor(d->descriptor, BLANK_MENU_LINE, d);
    write_to_descriptor(d->descriptor, "{w   |    {G*{w Detailed additional credits can be viewed in game via the         |\r\n", d);
    write_to_descriptor(d->descriptor, "{w   |      credits command.  These additional credits include the names of   |\r\n", d);
    write_to_descriptor(d->descriptor, "{w   |      many who have contributed through the mud community over the      |\r\n", d);
    write_to_descriptor(d->descriptor, "{w   |      years where those contributions have been used here.              |\r\n", d);

    show_menu_bottom(d);

    return;
}

/*
* Shows who is logged into the mud from the login menu.
*/
void show_login_who(DESCRIPTOR_DATA *d)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *dl;
    int col = 0;
    int count = 0;
    int total_count = 0;
    bool end_row = TRUE;

    show_menu_top(d);
    show_menu_header("Online Players", d);
    write_to_descriptor(d->descriptor, BLANK_MENU_LINE, d);
    write_to_descriptor(d->descriptor, "{w   |                            {G( {WImmortals {G){w                               |\r\n", d);
    write_to_descriptor(d->descriptor, BLANK_MENU_LINE, d);

    for (dl = descriptor_list; dl != NULL; dl = dl->next)
    {
        CHAR_DATA *ch;

        if (dl->connected != CON_PLAYING)
        {
            continue;
        }

        ch = (dl->original != NULL) ? dl->original : dl->character;

        if (!IS_IMMORTAL(ch) || ch->invis_level >= LEVEL_HERO || ch->incog_level >= LEVEL_HERO)
        {
            continue;
        }

        count++;
        col++;

        // Start a row?
        if (end_row)
        {
            write_to_descriptor(d->descriptor, "{w   |    ", d);
            end_row = FALSE;
        }

        sprintf(buf, "{g%-16s{x", ch->name);
        write_to_descriptor(d->descriptor, buf, d);

        // End a row?
        if (col % 4 == 0)
        {
            write_to_descriptor(d->descriptor, "{w|\r\n", d);
            end_row = TRUE;
        }
    }

    // Fill out the rest of the last row if it wasn't a full row
    while (col % 4 != 0)
    {
        write_to_descriptor(d->descriptor, "                ", d);
        col++;
    }

    if (count > 0 && end_row == FALSE)
    {
        // End that row
        write_to_descriptor(d->descriptor, "    {w|\r\n", d);
        write_to_descriptor(d->descriptor, BLANK_MENU_LINE, d);
    }
    else if (count > 0 && end_row == TRUE)
    {
        write_to_descriptor(d->descriptor, BLANK_MENU_LINE, d);
    }

    // Display if there are no immortals online.
    if (count == 0)
    {
        write_to_descriptor(d->descriptor, "{w   |    * {CThere are no immortals currently online.{w                          |\r\n", d);
        write_to_descriptor(d->descriptor, BLANK_MENU_LINE, d);
    }

    write_to_descriptor(d->descriptor, "{w   |                             {G( {WMortals {G){w                                |\r\n", d);
    write_to_descriptor(d->descriptor, BLANK_MENU_LINE, d);

    total_count += count;

    // The characters playing
    end_row = TRUE;
    count = 0;
    col = 0;

    for (dl = descriptor_list; dl != NULL; dl = dl->next)
    {
        CHAR_DATA *ch;

        if (dl->connected != CON_PLAYING)
        {
            continue;
        }

        ch = (dl->original != NULL) ? dl->original : dl->character;

        if (IS_IMMORTAL(ch))
        {
            continue;
        }

        count++;
        col++;

        // Start a row?
        if (end_row)
        {
            write_to_descriptor(d->descriptor, "{w   |    ", d);
            end_row = FALSE;
        }

        sprintf(buf, "{g%-16s{x", ch->name);
        write_to_descriptor(d->descriptor, buf, d);

        // End a row?
        if (col % 4 == 0)
        {
            write_to_descriptor(d->descriptor, "    {w|\r\n", d);
            end_row = TRUE;
        }

    }

    // Fill out the rest of the last row if it wasn't a full row
    while (col % 4 != 0)
    {
        write_to_descriptor(d->descriptor, "                ", d);
        col++;
    }

    if (count > 0 && end_row == FALSE)
    {
        // End that row
        write_to_descriptor(d->descriptor, "    {w|\r\n", d);
        write_to_descriptor(d->descriptor, BLANK_MENU_LINE, d);
    }
    else if (count > 0 && end_row == TRUE)
    {
        write_to_descriptor(d->descriptor, BLANK_MENU_LINE, d);
    }

    // Display if there are no mortals online.
    if (count == 0)
    {
        write_to_descriptor(d->descriptor, "{w   |    * {CThere are no mortals currently online.{w                            |\r\n", d);
        write_to_descriptor(d->descriptor, BLANK_MENU_LINE, d);
    }

    total_count += count;

    if (total_count > 0)
    {
        sprintf(buf, "{w   |    Total Players Online: %-3d                                           |\r\n", total_count);
        write_to_descriptor(d->descriptor, buf, d);
    }

    show_menu_bottom(d);

    return;
}

/*
* Renders the current login menu to the player.
*/
void show_login_menu(DESCRIPTOR_DATA *d)
{
    int x = 0;

    // This probably shouldn't happen but better safe than sorry on a high run method.
    if (d == NULL)
    {
        return;
    }

    char buf[MAX_STRING_LENGTH];
    bool ban_newbie = check_ban(d->host, BAN_NEWBIES);
    bool ban_all = check_ban(d->host, BAN_ALL);

    // The login menu header
    write_to_descriptor(d->descriptor, "\r\n", d);
    show_menu_top(d);
    show_menu_header("Login Menu", d);
    write_to_descriptor(d->descriptor, BLANK_MENU_LINE, d);

    // Column 1.1 - Create a new character option.  The option is disabled if the game is wizlocked
    // newlocked, if their host is banned all together or if they are newbie banned.
    if (settings.wizlock || settings.newlock || ban_newbie || ban_all)
    {
        sprintf(buf, "{w   |    {w({DN{w){Dew Character{x                                                     |\r\n");
    }
    else
    {
        sprintf(buf, "{w   |    {w(%sN{w)%sew Character{x                                                     |\r\n",
            settings.login_menu_light_color,
            settings.login_menu_dark_color);
    }

    write_to_descriptor(d->descriptor, buf, d);

    // Column 2.1 - Play existing character, the login option is disabled if the player is banned or the game is wizlocked.
    if (ban_all || settings.wizlock)
    {
        sprintf(buf, "{w   |    {w({DP{w){Dlay Existing Character{x                                           |\r\n");
    }
    else
    {
        sprintf(buf, "{w   |    {w(%sP{w)%slay Existing Character{x                                           |\r\n",
            settings.login_menu_light_color,
            settings.login_menu_dark_color);
    }

    write_to_descriptor(d->descriptor, buf, d);

    // Column 3.1 - Who is currently online
    if (settings.login_who_list_enabled)
    {
        sprintf(buf, "{w   |    {w(%sW{w)%sho is on now?{w                                                    |\r\n",
            settings.login_menu_light_color,
            settings.login_menu_dark_color);

        write_to_descriptor(d->descriptor, buf, d);
    }

    // Column 4.1 - Random name generator
    sprintf(buf, "{w   |    {w(%sR{w)%sandom Name Generator{w                                             |\r\n",
        settings.login_menu_light_color,
        settings.login_menu_dark_color);

    write_to_descriptor(d->descriptor, buf, d);

    // Column 5.1 - Credits
    sprintf(buf, "{w   |    {w(%sC{w)%sredits{w                                                           |\r\n",
        settings.login_menu_light_color,
        settings.login_menu_dark_color);

    write_to_descriptor(d->descriptor, buf, d);

    // Column 6.1 & 6.2 - Quit and System Time
    sprintf(buf, "{w   |    {w(%sQ{w)%suit{w                                                              |\r\n",
        settings.login_menu_light_color,
        settings.login_menu_dark_color);

    write_to_descriptor(d->descriptor, buf, d);

    // Column 7.1 - Prompt
    write_to_descriptor(d->descriptor, BLANK_MENU_LINE, d);
    sprintf(buf, "{w   |     {WYour selection? {w->                                                 |\r\n");
    write_to_descriptor(d->descriptor, buf, d);

    show_menu_bottom(d);

    // Since we've wrapped the menu in an ASCII graphic that looks like a parchment, we'll need to
    // reposition the cursor to move up to "Your selection? ->".. then after the selection we'll
    // need to move the cursor back down before displaying whatever we're going to display.

    // Move up 4 rows, then to the right 28
    sprintf(buf, "%s%s%s%s", UP, UP, UP, UP);

    // Move to the right 28
    for (x = 0; x < 28; x++)
    {
        strcat(buf, RIGHT);
    }

    write_to_descriptor(d->descriptor, buf, d);

    return;
}

/*
 * Show's the top portion of the menu
 */
void show_menu_top(DESCRIPTOR_DATA *d)
{
    write_to_descriptor(d->descriptor, "{w  _________________________________________________________________________\r\n", d);
    write_to_descriptor(d->descriptor, "{w /`                                                                        \\\r\n", d);
}

/*
 * Show's the bottom porition of the menu
 */
void show_menu_bottom(DESCRIPTOR_DATA *d)
{
    write_to_descriptor(d->descriptor, BLANK_MENU_LINE, d);
    write_to_descriptor(d->descriptor, "{w / |  -==================================================================-  |\r\n", d);
    write_to_descriptor(d->descriptor, "{w \\/________________________________________________________________________/{x\r\n", d);
}

/*
 * Sends a header of the menu that is centered.
 */
void show_menu_header(char *caption, DESCRIPTOR_DATA *d)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char center_text[MAX_STRING_LENGTH];

    sprintf(buf, "{w \\_|%%s|\r\n");
    sprintf(center_text, "{W-=-=-=-=-=-=))) %s: {W%s {w(((=-=-=-=-=-=-{w", settings.mud_name, caption);
    sprintf(center_text, "%s", center_string_padded(center_text, 73));

    sprintf(buf2, buf, center_text);
    write_to_descriptor(d->descriptor, buf2, d);
}

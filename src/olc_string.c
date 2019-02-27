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

/***************************************************************************
 *  File: string.c                                                         *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "tables.h"
#include "olc.h"

char *string_linedel(char *, int);
char *string_lineadd(char *, char *, int);
char *numlines(char *);

/*****************************************************************************
 Name:        string_edit
 Purpose:     Clears string and puts player into editing mode.
 Called by:   none
 ****************************************************************************/
void string_edit(CHAR_DATA * ch, char **pString)
{
    send_to_char("-========- Entering EDIT Mode -=========-\r\n", ch);
    send_to_char("    Type .h on a new line for help\r\n", ch);
    send_to_char(" Terminate with a ~ or @ on a blank line.\r\n", ch);
    send_to_char("-=======================================-\r\n", ch);

    if (*pString == NULL)
    {
        *pString = str_dup("");
    }
    else
    {
        **pString = '\0';
    }

    ch->desc->pString = pString;

    return;
}

/*****************************************************************************
 Name:       string_append
 Purpose:    Puts player into append mode for given string.
 Called by:  (many)olc_act.c
 ****************************************************************************/
void string_append(CHAR_DATA * ch, char **pString)
{
    send_to_char("-=======- Entering APPEND Mode -========-\r\n", ch);
    send_to_char("    Type .h on a new line for help\r\n", ch);
    send_to_char(" Terminate with a ~ or @ on a blank line.\r\n", ch);
    send_to_char("-=======================================-\r\n", ch);

    if (*pString == NULL)
    {
        *pString = str_dup("");
    }
    send_to_char(numlines(*pString), ch);

/* numlines sends the string with \r\n */
/*  if ( *(*pString + strlen( *pString ) - 1) != '\r' )
    send_to_char( "\r\n", ch ); */

    ch->desc->pString = pString;

    return;
}

/*****************************************************************************
 Name:       string_replace
 Purpose:    Substitutes one string for another.
 Called by:  string_add(string.c) (aedit_builder)olc_act.c.
 ****************************************************************************/
char *string_replace(char *orig, char *old, char *new)
{
    char xbuf[MAX_STRING_LENGTH];
    int i;

    xbuf[0] = '\0';
    strcpy(xbuf, orig);
    if (strstr(orig, old) != NULL)
    {
        i = strlen(orig) - strlen(strstr(orig, old));
        xbuf[i] = '\0';
        strcat(xbuf, new);
        strcat(xbuf, &orig[i + strlen(old)]);
        free_string(orig);
    }

    return str_dup(xbuf);
}

/*****************************************************************************
 Name:       string_add
 Purpose:    Interpreter for string editing.
 Called by:  game_loop_xxxx(comm.c).
 ****************************************************************************/
void string_add(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    /*
     * Thanks to James Seng
     */
    smash_tilde(argument);

    if (*argument == '.')
    {
        char arg1[MAX_INPUT_LENGTH];
        char arg2[MAX_INPUT_LENGTH];
        char arg3[MAX_INPUT_LENGTH];
        char tmparg3[MAX_INPUT_LENGTH];

        argument = one_argument(argument, arg1);
        argument = first_arg(argument, arg2, FALSE);
        strcpy(tmparg3, argument);
        argument = first_arg(argument, arg3, FALSE);

        if (!str_cmp(arg1, ".c"))
        {
            send_to_char("String cleared.\r\n", ch);
            free_string(*ch->desc->pString);
            *ch->desc->pString = str_dup("");
            return;
        }

        if (!str_cmp(arg1, ".s"))
        {
            send_to_char("String so far:\r\n", ch);
            send_to_char(numlines(*ch->desc->pString), ch);
            return;
        }

        if (!str_cmp(arg1, ".r"))
        {
            if (arg2[0] == '\0')
            {
                send_to_char("usage:  .r \"old string\" \"new string\"\r\n",
                    ch);
                return;
            }

            *ch->desc->pString =
                string_replace(*ch->desc->pString, arg2, arg3);
            sprintf(buf, "'%s' replaced with '%s'.\r\n", arg2, arg3);
            send_to_char(buf, ch);
            return;
        }

        if (!str_cmp(arg1, ".f"))
        {
            *ch->desc->pString = format_string(*ch->desc->pString);
            send_to_char("String formatted.\r\n", ch);
            return;
        }

        if (!str_cmp(arg1, ".ld") || !str_cmp(arg1, ".dl"))
        {
            *ch->desc->pString = string_linedel(*ch->desc->pString, atoi(arg2));
            send_to_char("Line deleted.\r\n", ch);
            return;
        }

        if (!str_cmp(arg1, ".li"))
        {
            *ch->desc->pString =
                string_lineadd(*ch->desc->pString, tmparg3, atoi(arg2));
            send_to_char("Line inserted.\r\n", ch);
            return;
        }

        if (!str_cmp(arg1, ".lr"))
        {
            *ch->desc->pString =
                string_linedel(*ch->desc->pString, atoi(arg2));
            *ch->desc->pString =
                string_lineadd(*ch->desc->pString, tmparg3, atoi(arg2));
            send_to_char("Line replaced.\r\n", ch);
            return;
        }

        if (!str_cmp(arg1, ".h"))
        {
            send_to_char("Sedit help (commands on blank line):   \r\n", ch);
            send_to_char(".r 'old' 'new'   - replace a substring \r\n", ch);
            send_to_char("                   (requires '', \"\") \r\n", ch);
            send_to_char(".h               - get help (this info)\r\n", ch);
            send_to_char(".s               - show string so far  \r\n", ch);
            send_to_char(".f               - word wrap (format string)\r\n", ch);
            send_to_char(".c               - clear string so far \r\n", ch);
            send_to_char(".ld <num>        - delete line number <num>\r\n", ch);
            send_to_char(".li <num> <str>  - insert <str> at line <num>\r\n", ch);
            send_to_char(".lr <num> <str>  - replace line <num> with <str>\r\n", ch);
            send_to_char("@                - end string          \r\n", ch);
            return;
        }

        send_to_char("SEdit:  Invalid dot command.\r\n", ch);
        return;
    }

    if (*argument == '~' || *argument == '@')
    {
        if (ch->desc->editor == ED_MPCODE)
        {                        /* for the mobprogs */
            MOB_INDEX_DATA *mob;
            int hash;
            MPROG_LIST *mpl;
            MPROG_CODE *mpc;

            EDIT_MPCODE(ch, mpc);

            if (mpc != NULL)
            {
                for (hash = 0; hash < MAX_KEY_HASH; hash++)
                {
                    for (mob = mob_index_hash[hash]; mob; mob = mob->next)
                    {
                        for (mpl = mob->mprogs; mpl; mpl = mpl->next)
                        {
                            if (mpl->vnum == mpc->vnum)
                            {
                                sprintf(buf, "Updating mob %d.\r\n", mob->vnum);
                                send_to_char(buf, ch);
                                mpl->code = mpc->code;
                                mpl->name = mpl->name;
                            }
                        }
                    }
                }
            }
        }

        ch->desc->pString = NULL;
        return;
    }

    strcpy(buf, *ch->desc->pString);

    /*
     * Truncate strings to MAX_STRING_LENGTH.
     * --------------------------------------
     * Edwin strikes again! Fixed avoid adding to a too-long
     * note. JR -- 10/15/00
     */
    if (strlen(*ch->desc->pString) + strlen(argument) >= (MAX_STRING_LENGTH - 4))
    {
        send_to_char("String too long, last line skipped.\r\n", ch);
        log_f("%s has a string in the editor that exceeds the current limit.", ch->name);

        /* Force character out of editing mode. */
        ch->desc->pString = NULL;
        return;
    }

    /*
     * Ensure no tilde's inside string.
     * --------------------------------
     */
    smash_tilde(argument);

    strcat(buf, argument);
    strcat(buf, "\r\n");
    free_string(*ch->desc->pString);
    *ch->desc->pString = str_dup(buf);
    return;
}

/*
 * Thanks to Kalgen for the new procedure (no more bug!)
 * Original wordwrap() written by Surreality.
 */
/*****************************************************************************
 Name:       format_string
 Purpose:    Special string formating and word-wrapping.
 Called by:  string_add(olc_string.c) (many)olc_act.c
 ****************************************************************************/
char *format_string(char *oldstring)
{
    char xbuf[MAX_STRING_LENGTH * 15];
    char xbuf2[MAX_STRING_LENGTH * 15];
    char *rdesc;
    int i = 0;
    int end_of_line;
    bool cap = TRUE;
    bool bFormat = TRUE;

    xbuf[0] = xbuf2[0] = 0;

    i = 0;

    for (rdesc = oldstring; *rdesc; rdesc++)
    {
        if (*rdesc != '{')
        {
            if (bFormat)
            {
                if (*rdesc == '\n')
                {
                    if (*(rdesc + 1) == '\r' && *(rdesc + 2) == ' ' && *(rdesc + 3) == '\n' && xbuf[i - 1] != '\r')
                    {
                        xbuf[i] = '\n';
                        xbuf[i + 1] = '\r';
                        xbuf[i + 2] = '\n';
                        xbuf[i + 3] = '\r';
                        i += 4;
                        rdesc += 2;
                    }
                    else if (*(rdesc + 1) == '\r' && *(rdesc + 2) == ' ' && *(rdesc + 2) == '\n' && xbuf[i - 1] == '\r')
                    {
                        xbuf[i] = '\n';
                        xbuf[i + 1] = '\r';
                        i += 2;
                    }
                    else if (*(rdesc + 1) == '\r' && *(rdesc + 2) == '\n' && xbuf[i - 1] != '\r')
                    {
                        xbuf[i] = '\n';
                        xbuf[i + 1] = '\r';
                        xbuf[i + 2] = '\n';
                        xbuf[i + 3] = '\r';
                        i += 4;
                        rdesc += 1;
                    }
                    else if (*(rdesc + 1) == '\r' && *(rdesc + 2) == '\n' && xbuf[i - 1] == '\r')
                    {
                        xbuf[i] = '\n';
                        xbuf[i + 1] = '\r';
                        i += 2;
                    }
                    else if (xbuf[i - 1] != ' ' && xbuf[i - 1] != '\r')
                    {
                        xbuf[i] = ' ';
                        i++;
                    }
                }
                else if (*rdesc == '\r');
                else if (*rdesc == 'i' && *(rdesc + 1) == '.' && *(rdesc + 2) == 'e' && *(rdesc + 3) == '.')
                {
                    xbuf[i] = 'i';
                    xbuf[i + 1] = '.';
                    xbuf[i + 2] = 'e';
                    xbuf[i + 3] = '.';
                    i += 4;
                    rdesc += 3;
                }
                else if (*rdesc == ' ')
                {
                    if (xbuf[i - 1] != ' ')
                    {
                        xbuf[i] = ' ';
                        i++;
                    }
                }
                else if (*rdesc == ')')
                {
                    if (xbuf[i - 1] == ' ' && xbuf[i - 2] == ' '
                        && (xbuf[i - 3] == '.' || xbuf[i - 3] == '?' || xbuf[i - 3] == '!'))
                    {
                        xbuf[i - 2] = *rdesc;
                        xbuf[i - 1] = ' ';
                        xbuf[i] = ' ';
                        i++;
                    }
                    else if (xbuf[i - 1] == ' ' && (xbuf[i - 2] == ',' || xbuf[i - 2] == ';'))
                    {
                        xbuf[i - 1] = *rdesc;
                        xbuf[i] = ' ';
                        i++;
                    }
                    else
                    {
                        xbuf[i] = *rdesc;
                        i++;
                    }
                }
                else if (*rdesc == ',' || *rdesc == ';')
                {
                    if (xbuf[i - 1] == ' ')
                    {
                        xbuf[i - 1] = *rdesc;
                        xbuf[i] = ' ';
                        i++;
                    }
                    else
                    {
                        xbuf[i] = *rdesc;
                        if (*(rdesc + 1) != '\"')
                        {
                            xbuf[i + 1] = ' ';
                            i += 2;
                        }
                        else
                        {
                            xbuf[i + 1] = '\"';
                            xbuf[i + 2] = ' ';
                            i += 3;
                            rdesc++;
                        }
                    }


                }
                else if (*rdesc == '.' || *rdesc == '?' || *rdesc == '!')
                {
                    if (xbuf[i - 1] == ' ' && xbuf[i - 2] == ' '
                        && (xbuf[i - 3] == '.' || xbuf[i - 3] == '?' || xbuf[i - 3] == '!'))
                    {
                        xbuf[i - 2] = *rdesc;
                        if (*(rdesc + 1) != '\"')
                        {
                            xbuf[i - 1] = ' ';
                            xbuf[i] = ' ';
                            i++;
                        }
                        else
                        {
                            xbuf[i - 1] = '\"';
                            xbuf[i] = ' ';
                            xbuf[i + 1] = ' ';
                            i += 2;
                            rdesc++;
                        }
                    }
                    else
                    {
                        xbuf[i] = *rdesc;
                        if (*(rdesc + 1) != '\"')
                        {
                            xbuf[i + 1] = ' ';
                            xbuf[i + 2] = ' ';
                            i += 3;
                        }
                        else
                        {
                            xbuf[i + 1] = '\"';
                            xbuf[i + 2] = ' ';
                            xbuf[i + 3] = ' ';
                            i += 4;
                            rdesc++;
                        }
                    }
                    cap = TRUE;
                }
                else
                {
                    xbuf[i] = *rdesc;
                    if (cap)
                    {
                        cap = FALSE;
                        xbuf[i] = UPPER(xbuf[i]);
                    }
                    i++;
                }
            }
            else
            {
                xbuf[i] = *rdesc;
                i++;
            }
        }
        else
        {
            if (*(rdesc + 1) == 'Z')
                bFormat = !bFormat;
            xbuf[i] = *rdesc;
            i++;
            rdesc++;
            xbuf[i] = *rdesc;
            i++;
        }
    }
    xbuf[i] = 0;
    strcpy(xbuf2, xbuf);

    rdesc = xbuf2;

    xbuf[0] = 0;

    for (;;)
    {
        end_of_line = 77;
        for (i = 0; i < end_of_line; i++)
        {
            if (*(rdesc + i) == '{')
            {
                end_of_line += 2;
                i++;
            }


            if (!*(rdesc + i))
                break;


            if (*(rdesc + i) == '\r')
                end_of_line = i;
        }
        if (i < end_of_line)
        {
            break;
        }
        if (*(rdesc + i - 1) != '\r')
        {
            for (i = (xbuf[0] ? (end_of_line - 1) : (end_of_line - 4)); i; i--)
            {
                if (*(rdesc + i) == ' ')
                    break;
            }
            if (i)
            {
                *(rdesc + i) = 0;
                strcat(xbuf, rdesc);
                strcat(xbuf, "\r\n");
                rdesc += i + 1;
                while (*rdesc == ' ')
                    rdesc++;
            }
            else
            {
                bug("Wrap_string: No spaces", 0);
                *(rdesc + (end_of_line - 2)) = 0;
                strcat(xbuf, rdesc);
                strcat(xbuf, "-\r\n");
                rdesc += end_of_line - 1;
            }
        }
        else
        {
            *(rdesc + i - 1) = 0;
            strcat(xbuf, rdesc);
            strcat(xbuf, "\r");
            rdesc += i;
            while (*rdesc == ' ')
                rdesc++;
        }
    }
    while (*(rdesc + i) && (*(rdesc + i) == ' ' ||
        *(rdesc + i) == '\n' ||
        *(rdesc + i) == '\r'))
        i--;
    *(rdesc + i + 1) = 0;
    strcat(xbuf, rdesc);
    if (xbuf[strlen(xbuf) - 2] != '\n')
        strcat(xbuf, "\r\n");

    free_string(oldstring);
    return (str_dup(xbuf));

} // end format_string

/*
 * Used above in string_add.  Because this function does not
 * modify case if fCase is FALSE and because it understands
 * parenthesis, it would probably make a nice replacement
 * for one_argument.
 */
/*****************************************************************************
 Name:      first_arg
 Purpose:   Pick off one argument from a string and return the rest.
            Understands quates, parenthesis (barring ) ('s) and
            percentages.
 Called by: string_add(string.c)
 ****************************************************************************/
char *first_arg(char *argument, char *arg_first, bool fCase)
{
    char cEnd;

    while (*argument == ' ')
        argument++;

    cEnd = ' ';
    if (*argument == '\'' || *argument == '"'
        || *argument == '%' || *argument == '(')
    {
        if (*argument == '(')
        {
            cEnd = ')';
            argument++;
        }
        else
            cEnd = *argument++;
    }

    while (*argument != '\0')
    {
        if (*argument == cEnd)
        {
            argument++;
            break;
        }
        if (fCase)
            *arg_first = LOWER(*argument);
        else
            *arg_first = *argument;
        arg_first++;
        argument++;
    }
    *arg_first = '\0';

    while (*argument == ' ')
        argument++;

    return argument;
}

/*
 * Used in olc_act.c for aedit_builders.
 */
char *string_unpad(char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char *s;

    s = argument;

    while (*s == ' ')
        s++;

    strcpy(buf, s);
    s = buf;

    if (*s != '\0')
    {
        while (*s != '\0')
            s++;
        s--;

        while (*s == ' ')
            s--;
        s++;
        *s = '\0';
    }

    free_string(argument);
    return str_dup(buf);
}

/*
 * Same as capitalize but changes the pointer's data.
 * Used in olc_act.c in aedit_builder.
 */
char *string_proper(char *argument)
{
    char *s;

    s = argument;

    while (*s != '\0')
    {
        if (*s != ' ')
        {
            *s = UPPER(*s);
            while (*s != ' ' && *s != '\0')
                s++;
        }
        else
        {
            s++;
        }
    }

    return argument;
}

char *string_linedel(char *string, int line)
{
    char *strtmp = string;
    char buf[MAX_STRING_LENGTH];
    int cnt = 1, tmp = 0;

    buf[0] = '\0';

    for (; *strtmp != '\0'; strtmp++)
    {
        if (cnt != line)
            buf[tmp++] = *strtmp;

        if (*strtmp == '\n')
        {
            if (*(strtmp + 1) == '\r')
            {
                if (cnt != line)
                    buf[tmp++] = *(++strtmp);
                else
                    ++strtmp;
            }

            cnt++;
        }
    }

    buf[tmp] = '\0';

    free_string(string);
    return str_dup(buf);
}

char *string_lineadd(char *string, char *newstr, int line)
{
    char *strtmp = string;
    int cnt = 1, tmp = 0;
    bool done = FALSE;
    char buf[MAX_STRING_LENGTH];

    buf[0] = '\0';

    for (; *strtmp != '\0' || (!done && cnt == line); strtmp++)
    {
        if (cnt == line && !done)
        {
            strcat(buf, newstr);
            strcat(buf, "\r\n");
            tmp += strlen(newstr) + 2;
            cnt++;
            done = TRUE;
        }

        buf[tmp++] = *strtmp;

        if (done && *strtmp == '\0')
            break;

        if (*strtmp == '\n')
        {
            if (*(strtmp + 1) == '\r')
                buf[tmp++] = *(++strtmp);

            cnt++;
        }

        buf[tmp] = '\0';
    }

    free_string(string);
    return str_dup(buf);
}

char *merc_getline(char *str, char *buf)
{
    int tmp = 0;
    bool found = FALSE;

    while (*str)
    {
        if (*str == '\n')
        {
            found = TRUE;
            break;
        }

        buf[tmp++] = *(str++);
    }

    if (found)
    {
        if (*(str + 1) == '\r')
            str += 2;
        else
            str += 1;
    }

    buf[tmp] = '\0';

    return str;
}

/*
 * This will return a string with the each line of the string prepended with
 * the line number.  This modified version originated from the Ice project on
 * mudbytes and replaces the stock OLC version which had a buffer overflow
 * vulnerability.  Slight modifications for formatting have been made.
 */
char *numlines(char *string)
{
    int cnt = 1;
    static char buf[MAX_STRING_LENGTH * 2] = "\0\0\0\0\0\0\0";
    char tmp_line[MAX_STRING_LENGTH] = "\0\0\0\0\0\0\0";

    buf[0] = '\0'; /* keep me, static */

    while (*string)
    {
        int buf_left = 0;
        int size_wanted = 0;

        string = merc_getline(string, tmp_line);
        buf_left = (MAX_STRING_LENGTH * 2) - strlen(buf) - 12; /* See format string below */
        size_wanted = snprintf(buf + strlen(buf), buf_left, "{C%3.3d{x> %s\r\n", cnt++, tmp_line);

        if (size_wanted >= buf_left)
        {
            log_f("STRING OVERFLOW in char *numlines - wanted %d, had %d", size_wanted, buf_left);
        }
    }

    return buf;
}

/*
   This is the punct snippet from Desden el Chaman Tibetano - Nov 1998
   Email: jlalbatros@mx2.redestb.es

   This snippit can only be used once per sprintf, printf, printf_to_char, sendf, etc.  If used
   more than once the first value will always be returend.
*/
char *num_punct(int foo)
{
    int index_new, rest, x;
    unsigned int nindex;
    char buf[16];
    static char buf_new[16];

    snprintf(buf, 16, "%d", foo);
    rest = strlen(buf) % 3;

    for (nindex = index_new = 0; nindex < strlen(buf); nindex++, index_new++)
    {
        x = nindex - rest;
        if (nindex != 0 && (x % 3) == 0)
        {
            buf_new[index_new] = ',';
            index_new++;
            buf_new[index_new] = buf[nindex];
        }
        else
        {
            buf_new[index_new] = buf[nindex];
        }
    }

    buf_new[index_new] = '\0';
    return buf_new;
}

/*
   This is the punct snippet from Desden el Chaman Tibetano - Nov 1998
   Email: jlalbatros@mx2.redestb.es

   This snippit can only be used once per sprintf, printf, printf_to_char, etc.  If used
   more than once the first value will always be returend.
*/
char *num_punct_long(long foo)
{
    int index_new, rest, x;
    unsigned int nindex;
    char buf[16];
    static char buf_new[16];

    snprintf(buf, 16, "%ld", foo);
    rest = strlen(buf) % 3;

    for (nindex = index_new = 0; nindex < strlen(buf); nindex++, index_new++)
    {
        x = nindex - rest;
        if (nindex != 0 && (x % 3) == 0)
        {
            buf_new[index_new] = ',';
            index_new++;
            buf_new[index_new] = buf[nindex];
        }
        else
        {
            buf_new[index_new] = buf[nindex];
        }
    }

    buf_new[index_new] = '\0';
    return buf_new;
}

/*
 * Counts the number of lines in a string as defined by \r
 */
int line_count(char *orig)
{
    char   *rdesc;
    int     current_line = 1;

    for (rdesc = orig; *rdesc; rdesc++)
    {
        if (*rdesc == '\r')
            current_line++;
    }

    return current_line;
}

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
 *  This code file houses miscellaneous methods that don't fit anywhere    *
 *  else.                                                                  *
 **************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include "merc.h"

/*
 * Extended Bitvector Routines (originating in the Smaug code base by Thoric)
 */

/*
 * Check to see if the extended bitvector is completely empty
 */
bool ext_is_empty(EXT_BV *bits)
{
    int x;

    for (x = 0; x < XBI; x++)
        if (bits->bits[x] != 0)
            return FALSE;

    return TRUE;
}

/*
 * Clears any bits from the extended
 */
void ext_clear_bits(EXT_BV *bits)
{
    int x;

    for (x = 0; x < XBI; x++)
        bits->bits[x] = 0;
}

/*
 * For use by xHAS_BITS() -- works like IS_SET()
 */
int ext_has_bits(EXT_BV *var, EXT_BV *bits)
{
    int x, bit;

    for (x = 0; x < XBI; x++)
        if ((bit = (var->bits[x] & bits->bits[x])) != 0)
            return bit;

    return 0;
}

/*
 * For use by xSAME_BITS() -- works like ==
 */
bool ext_same_bits(EXT_BV *var, EXT_BV *bits)
{
    int x;

    for (x = 0; x < XBI; x++)
        if (var->bits[x] != bits->bits[x])
            return FALSE;

    return TRUE;
}

/*
 * For use by xSET_BITS() -- works like SET_BIT()
 */
void ext_set_bits(EXT_BV *var, EXT_BV *bits)
{
    int x;

    for (x = 0; x < XBI; x++)
        var->bits[x] |= bits->bits[x];
}

/*
 * For use by xREMOVE_BITS() -- works like REMOVE_BIT()
 */
void ext_remove_bits(EXT_BV *var, EXT_BV *bits)
{
    int x;

    for (x = 0; x < XBI; x++)
        var->bits[x] &= ~(bits->bits[x]);
}

/*
 * For use by xTOGGLE_BITS() -- works like TOGGLE_BIT()
 */
void ext_toggle_bits(EXT_BV *var, EXT_BV *bits)
{
    int x;

    for (x = 0; x < XBI; x++)
        var->bits[x] ^= bits->bits[x];
}

/*
 * Read an extended bitvector from a file.                      -Thoric
 */
EXT_BV fread_bitvector(FILE *fp)
{
    EXT_BV ret;
    int c, x = 0;
    int num = 0;

    memset(&ret, '\0', sizeof(ret));
    for (;; )
    {
        num = fread_number(fp);
        if (x < XBI)
            ret.bits[x] = num;
        ++x;
        if ((c = getc(fp)) != '&')
        {
            ungetc(c, fp);
            break;
        }
    }

    return ret;
}

/*
 * Return a string for writing a bitvector to a file
 */
char *print_bitvector(EXT_BV *bits)
{
    static char buf[XBI * 12];
    char *p = buf;
    int x, cnt = 0;

    for (cnt = XBI - 1; cnt > 0; cnt--)
        if (bits->bits[cnt])
            break;
    for (x = 0; x <= cnt; x++)
    {
        sprintf(p, "%d", bits->bits[x]);
        p += strlen(p);
        if (x < cnt)
            *p++ = '&';
    }
    *p = '\0';

    return buf;
}

/*
 * Write an extended bitvector to a file                        -Thoric
 */
void fwrite_bitvector(EXT_BV *bits, FILE *fp)
{
    fputs(print_bitvector(bits), fp);
}

EXT_BV meb(int bit)
{
    EXT_BV bits;

    xCLEAR_BITS(bits);
    if (bit >= 0)
        xSET_BIT(bits, bit);

    return bits;
}

EXT_BV multimeb(int bit, ...)
{
    EXT_BV bits;
    va_list param;
    int b;

    xCLEAR_BITS(bits);
    if (bit < 0)
        return bits;

    xSET_BIT(bits, bit);

    va_start(param, bit);

    while ((b = va_arg(param, int)) != -1)
        xSET_BIT(bits, b);

    va_end(param);

    return bits;
}

/*
 * Returns whether a file exists or not.
 */
bool file_exists(const char *filename)
{
    FILE *file;

    if ((file = fopen(filename, "r")))
    {
        fclose(file);
        return TRUE;
    }

    return FALSE;
}

/*
 * Returns whether a player file exists or not for the specified player.
 */
bool player_exists(const char *player)
{
    FILE *file;
    char filename[MAX_STRING_LENGTH];

    // Player directory + the name of the player that's capitalized.
    sprintf(filename, "%s%s", PLAYER_DIR, capitalize(player));

    if ((file = fopen(filename, "r")))
    {
        fclose(file);
        return TRUE;
    }

    return FALSE;
}

/*
 * Returns the location to where a player file would be stored based off
 * of their name (whether it exists or not is another story).
 */
char *player_file_location(const char *player)
{
    static char filename[MAX_STRING_LENGTH];

    // Player directory + the name of the player that's capitalized.
    sprintf(filename, "%s%s", PLAYER_DIR, capitalize(player));

    return filename;
}

/*
 * Returns the date a file was last modified (local time)
 */
char *file_last_modified(const char *filename)
{
    static char buf[100];
    struct stat attrib;

    // Stat the file
    stat(filename, &attrib);

    // Put the results changed to local time into a static string
    strftime(buf, 100, "%m-%d-%y", localtime(&(attrib.st_ctime)));

    return buf;
}

/*
 * Returns the string true or false for the bool value.
 */
char *bool_truefalse(bool value)
{
    static char buf[6];

    if (value)
    {
        sprintf(buf, "%s", "true");
    }
    else
    {
        sprintf(buf, "%s", "false");
    }

    return buf;
}

/*
 * Returns the string yes or no for the bool value.
 */
char *bool_yesno(bool value)
{
    static char buf[4];

    if (value)
    {
        sprintf(buf, "%s", "yes");
    }
    else
    {
        sprintf(buf, "%s", "no");
    }

    return buf;
}

/*
 * Returns the string on or off for the bool value.
 */
char *bool_onoff(bool value)
{
    static char buf[4];

    if (value)
    {
        sprintf(buf, "%s", "on");
    }
    else
    {
        sprintf(buf, "%s", "off");
    }

    return buf;
}

/*
 * Returns a string centered in the required width.  If null is passed in
 * a string of spaces that is the width passed in will be returned.  If the
 * string is * larger than the width it's just passed back out (but length
 * checked to not overflow MSL).
 */
char *center_string_padded(const char *str, int width)
{
    static char buf[MAX_STRING_LENGTH];

    // If it's null, pass back the string padded with all spaces
    if (str == NULL)
    {
        snprintf(buf, MAX_STRING_LENGTH, "%*s", width - 1, "");
        return buf;
    }

    int length = 0;
    int color_length = 0;

    length = strlen(str);
    color_length = count_color(str);

    // We can't center the string if the the string is larger than the
    // width to center it in, just send it back.
    if ((length - color_length) > width)
    {
        snprintf(buf, MAX_STRING_LENGTH, "%s", str);
        return buf;
    }

    // Center the string within the width specified, use snprintf to ensure
    // the string doesn't overflow the MAX_STRING_LENGTH
    int pad_left = 0;
    int pad_right = 0;

    pad_left = (width / 2) + (length / 2);
    pad_right = width - pad_left - 1;

    pad_left += color_length / 2;
    pad_right += color_length / 2;

    // Use snprintf to pad the string
    snprintf(buf, MAX_STRING_LENGTH, "%*s%*s", pad_left, str, pad_right, "");

    return buf;
}

/*
 * Case insentive version of strstr
 */
char *stristr(const char *str1, const char *str2)
{
    const char* p1 = str1 ;
    const char* p2 = str2 ;
    const char* r = *p2 == 0 ? str1 : 0 ;

    while( *p1 != 0 && *p2 != 0 )
    {
        if( tolower( (unsigned char)*p1 ) == tolower( (unsigned char)*p2 ) )
        {
            if( r == 0 )
            {
                r = p1 ;
            }

            p2++ ;
        }
        else
        {
            p2 = str2 ;
            if( r != 0 )
            {
                p1 = r + 1 ;
            }

            if( tolower( (unsigned char)*p1 ) == tolower( (unsigned char)*p2 ) )
            {
                r = p1 ;
                p2++ ;
            }
            else
            {
                r = 0 ;
            }
        }

        p1++ ;
    }

    return *p2 == 0 ? (char*)r : 0 ;
}


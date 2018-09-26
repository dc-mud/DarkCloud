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
  *  File: log.c                                                            *
  *                                                                         *
  *  This file should contain general purpose logging functions for the     *
  *  game.  Some of these functions may write to the log file or some may   *
  *  write to specialized files (or databases in the future).  For now      *
  *  we are going to keep the flat file logs, they're easy enough to grep   *
  *  for info.  The general ROM logging functions have been moved here as   *
  *  well as some additions.                                                *
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>     /* bugf, log_f */
#include <ctype.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"

// globals from db.c used in bug
extern FILE * fpArea;
extern char strArea[MAX_INPUT_LENGTH];

/*
 * Reports a bug.
 */
void bug(const char *str, int param)
{
    char buf[MAX_STRING_LENGTH * 5];

    if (fpArea != NULL)
    {
        int iLine;
        int iChar;

        if (fpArea == stdin)
        {
            iLine = 0;
        }
        else
        {
            iChar = ftell(fpArea);
            fseek(fpArea, 0, 0);
            for (iLine = 0; ftell(fpArea) < iChar; iLine++)
            {
                while (getc(fpArea) != '\n');
            }
            fseek(fpArea, iChar, 0);
        }

        // If the line exists in an area file loading, show both to the log.
        sprintf(buf, "[*****] FILE: %s LINE: %d", strArea, iLine);
        log_string(buf);
    }

    // Log the bug to the disk log file.
    strcpy(buf, "[*****] BUG: ");
    sprintf(buf + strlen(buf), str, param);
    log_string(buf);

    // Make a call to wiznet to show the in game immortals something is up.
    wiznet(buf, NULL, NULL, WIZ_BUGS, 0, 0);

    return;
}

/*
 * Writes a string to the log.
 */
void log_string(const char *str)
{
    char *strtime;

    strtime = ctime(&current_time);
    strtime[strlen(strtime) - 1] = '\0';
    fprintf(stderr, "%s :: %s\n", strtime, str);
    return;
}

/*
 * Writes a formatted entry to the log file.
 */
void log_f(char *fmt, ...)
{
    char buf[2 * MSL];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    log_string(buf);
}

/*
 * Writes a formatted bug entry out to the log file.
 */
void bugf(char *fmt, ...)
{
    char buf[2 * MSL];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    bug(buf, 0);
}

/*
 * Writes information about an objects state to the log file.  This should be
 * used to debug an object state and not to over zealously log data.
 */
void log_obj(OBJ_DATA * obj)
{
    char buf[MAX_STRING_LENGTH];

    if (obj == NULL)
    {
        // If it's null, write it and get out
        log_f("Object State Log: NULL");
        return;
    }
    else
    {
        // The objects index data for the VNUM in case it's a restring.
        if (obj->pIndexData != NULL)
        {
            log_f("Object State Log: %s, vnum %d", strip_color(obj->short_descr), obj->pIndexData->vnum);
        }
        else
        {
            log_f("Object State Log: %s, pIndexData=NULL", strip_color(obj->short_descr));
        }
    }

    // Int and bool values
    log_f("  * valid=%s, enchanted=%s, item_type=%d, level=%d, timer=%d",
        obj->valid ? "true" : "false",
        obj->enchanted ? "true" : "false",
        obj->item_type, obj->level, obj->timer);

    log_f("  * wear_loc=%d, condition=%d, weight=%d, cost=%d, count=%d",
        obj->wear_loc, obj->condition, obj->weight, obj->cost, obj->count);

    // Now log the strings in the object that need extra handling

    // The owner
    if (obj->owner != NULL)
    {
        sprintf(buf, "  * owner=%s", obj->owner);
    }
    else
    {
        sprintf(buf, "  * owner=NULL");
    }

    // The last person to enchant the item.
    if (obj->enchanted_by != NULL)
    {
        sprintf(buf, "%s, enchanted_by=%s", buf, obj->enchanted_by);
    }
    else
    {
        sprintf(buf, "%s, enchanted_by=NULL", buf);
    }

    // In case someone has wizard marked the item.
    if (obj->wizard_mark != NULL)
    {
        sprintf(buf, "%s, wizard_mark=%s", buf, obj->wizard_mark);
    }
    else
    {
        sprintf(buf, "%s, wizard_mark=NULL", buf);
    }

    log_string(buf);

    // Reset line on this one

    // The name of the object (the keywords)
    if (obj->name != NULL)
    {
        sprintf(buf, "  * name=%s", obj->name);
    }
    else
    {
        sprintf(buf, "  * name=NULL");
    }

    // The set material
    if (obj->material != NULL)
    {
        sprintf(buf, "%s, material=%s", buf, obj->material);
    }
    else
    {
        sprintf(buf, "%s, material=NULL", buf);
    }

    log_string(buf);

    // Reset line on this one
    // If someone is carrying the item and whether they're an NPC or not.
    if (obj->carried_by != NULL)
    {
        sprintf(buf, "  * carried_by=%s", obj->carried_by->name);

        if (IS_NPC(obj->carried_by))
        {
            sprintf(buf, "%s (NPC)", buf);
        }
    }
    else
    {
        sprintf(buf, "  * carried_by=NULL");
    }

    // If the object is currently in a room, log data about that room.
    if (obj->in_room != NULL)
    {
        sprintf(buf, "%s, in_room->vnum=%d", buf, obj->in_room->vnum);

        if (obj->in_room->name != NULL)
        {
            sprintf(buf, "%s, in_room->name=%s", buf, obj->in_room->name);
        }

        if (obj->in_room->area != NULL && obj->in_room->area->name != NULL)
        {
            sprintf(buf, "%s, area->name=%s", buf, obj->in_room->area->name);
        }

    }
    else
    {
        sprintf(buf, "%s, in_room=NULL", buf);
    }

    log_string(buf);

    // Reset line on this one
    // If it's in another object, a little data about that.
    if (obj->in_obj != NULL && obj->in_obj->short_descr != NULL)
    {
        sprintf(buf, "  * in_obj->short_descr=%s", obj->in_obj->short_descr);

        if (obj->in_obj->carried_by != NULL && obj->in_obj->carried_by->name != NULL)
        {
            sprintf(buf, "%s, in_obj->carried_by->name=%s", buf, obj->in_obj->carried_by->name);
        }

    }
    else
    {
        sprintf(buf, "  * in_obj=NULL");
    }

    // If it's on another object, a little data about that.
    if (obj->on != NULL && obj->on->short_descr != NULL)
    {
        sprintf(buf, "%s, on->short_descr=%s", buf, obj->on->short_descr);
    }
    else
    {
        sprintf(buf, "%s, obj->on=NULL", buf);
    }

    log_string(buf);

    return;
}

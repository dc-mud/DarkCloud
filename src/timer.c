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
* [S]imulated [M]edieval [A]dventure multi[U]ser [G]ame      |   \\._.//   *
* -----------------------------------------------------------|   (0...0)   *
* SMAUG 1.4 (C) 1994, 1995, 1996, 1998  by Derek Snider      |    ).:.(    *
* -----------------------------------------------------------|    {o o}    *
* SMAUG code team: Thoric, Altrag, Blodkai, Narn, Haus,      |   / ' ' \   *
* Scryn, Rennard, Swordbearer, Gorog, Grishnakh, Nivek,      |~'~.VxvxV.~'~*
* Tricops and Fireblade                                      |             *
****************************************************************************/

/***************************************************************************
*  These timer methods all come from the Smaug code base and will assist   *
*  with timing events as well as setting up callbacks from commands that   *
*  can occur after a set period (like digging something up, etc).  New     *
*  time based functions should be added here in the future.                *
***************************************************************************/

// System Specific Includes
#if defined(_WIN32)
    #include <sys/types.h>
    #include <time.h>
    #include <winsock2.h> // for timeval in Windows
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

struct timeval start_time, end_time;

/*
 * Starts a timer.  To be used in tandem with end_timer to time an event.  On Windows the microseconds (tv_usec)
 * returns as 0 resulting in only a whole number of seconds count which isn't ideal.  For windows use consider
 * directives that use the WIN32 api for high resolution performance counters.
 */
void start_timer()
{
    gettimeofday(&start_time, NULL);
    return;
}

/*
 * Ends a timer.  To be used in tandem with start_timer in order to time an event.
 */
double end_timer()
{
    /* Mark time before checking stime, so that we get a better reading.. */
    gettimeofday(&end_time, NULL);
    long elapsed = ((end_time.tv_sec - start_time.tv_sec) * 1000000) + (end_time.tv_usec - start_time.tv_usec);
    return (double)elapsed / (double)1000000;
}

/*
 * Subtracts two times.
 */
void subtract_times(struct timeval *etime, struct timeval *stime)
{
    etime->tv_sec -= stime->tv_sec;
    etime->tv_usec -= stime->tv_usec;

    while (etime->tv_usec < 0)
    {
        etime->tv_usec += 1000000;
        etime->tv_sec--;
    }

    return;
}  // end subtract_times

/*
 * Adds a timer to an action with the information necessary to callback later
 * when the timer is up.
 */
void add_timer(CHAR_DATA *ch, int type, int count, DO_FUN *fun, int value, char *argument) {
    //log_f("add_timer ch=%s, type=%d, count=%d, value=%d, argument=%s", ch->name, type, count, value, argument);
    TIMER *timer;

    for (timer = ch->first_timer; timer; timer = timer->next)
    {
        if (timer->type == type && timer->type)
        {
            timer->count = count;
            timer->do_fun = fun;
            timer->value = value;
            timer->cmd = str_dup(argument);
            break;
        }
    }

    if (!timer)
    {
        timer = new_timer();
        //CREATE( timer, TIMER, 1 );
        timer->count = count;
        timer->type = type;
        timer->do_fun = fun;
        timer->value = value;
        timer->cmd = str_dup(argument);
        LINK(timer, ch->last_timer, ch->first_timer, prev, next);
    }

} // end add_timer

/*
 * Returns a pointer to a TIMER for a character and a specific type.
 * This would be the first one found.
 */
TIMER *get_timerptr(CHAR_DATA *ch, int type)
{
    TIMER *timer;
    for (timer = ch->first_timer; timer; timer = timer->next)
    {
        if (timer->type == type)
        {
            return timer;
        }
    }

    return NULL;
} // end get_timerptr

/*
 * Returns the timer->count for the specified type of timer on a
 * player.  This returns the first of the specified type found.
 */
int get_timer(CHAR_DATA *ch, int type)
{
    TIMER *timer;

    if ((timer = get_timerptr(ch, type)) != NULL)
    {
        return timer->count;
    }
    else
    {
        return 0;
    }
} // end get_timer

/*
 * Extracts a specific timer from a player.
 */
void extract_timer(CHAR_DATA *ch, TIMER *timer)
{

    if (!timer)
    {
        bug("extract_timer: NULL timer", 0);
        return;
    }

    UNLINK(timer, ch->first_timer, ch->last_timer, next, prev);
    free_timer(timer);
    return;

} // end extract_timer

/*
 * Removes a timer of a specified type.  This removes the first instance
 * of this timer type.
 */
void remove_timer(CHAR_DATA *ch, int type)
{
    TIMER *timer;

    for (timer = ch->first_timer; timer; timer = timer->next)
    {
        if (timer->type == type)
        {
            break;
        }
    }

    if (timer)
    {
        extract_timer(ch, timer);
    }

} // end remove_timer

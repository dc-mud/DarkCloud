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

/**************************************************************************/
/* mlkesl@stthomas.edu	=====>	Ascii Automapper utility                  */
/* Let me know if you use this. Give a newbie some credit,                */
/* at least I'm not asking how to add classes...                          */
/* Also, if you fix something could ya send me mail about, thanks         */
/* PLEASE mail me if you use this ot like it, that way I will keep it up  */
/**************************************************************************/
/* map_area -> when given a room, ch, x, and y,...                        */
/*             this function will fill in values of map as it should      */
/* show_map -> will simply spit out the contents of map array             */
/*             Would look much nicer if you built your own areas          */
/*             without all of the overlapping stock Rom has               */
/* do_map  ->  core function, takes map size as argument                  */
/**************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "merc.h"

#define MAX_MAP 72
#define MAX_MAP_DIR 10

char *map[MAX_MAP][MAX_MAP];
int offsets[10][2] ={ {-1, 0},{ 0, 1},{ 1, 0},{ 0,-1},{0.0},{0,0},{-1,-1},{-1,1},{1,-1},{1,1}};

/*
 * Method to handle the construction of the map of the surrounding areas.
 */
void map_area(ROOM_INDEX_DATA *room, CHAR_DATA *ch, int x, int y, int min, int max)
{
    ROOM_INDEX_DATA *prospect_room;
    EXIT_DATA *pexit;
    int door;

    /* marks the room as visited */
    switch (room->sector_type)
    {
        case SECT_INSIDE:       map[x][y]="{W%";        break;
        case SECT_CITY:         map[x][y]="{W#";        break;
        case SECT_FIELD:        map[x][y]="{G\"";       break;
        case SECT_FOREST:       map[x][y]="{g@";        break;
        case SECT_HILLS:        map[x][y]="{G^";        break;
        case SECT_MOUNTAIN:     map[x][y]="{y^";        break;
        case SECT_WATER_SWIM:   map[x][y]="{B~";        break;
        case SECT_OCEAN:        map[x][y]="{C~";        break;
        case SECT_WATER_NOSWIM: map[x][y]="{b~";        break;
        case SECT_UNDERWATER:   map[x][y]="{w~";        break;
        case SECT_AIR:          map[x][y]="{C:";        break;
        case SECT_DESERT:       map[x][y]="{Y=";        break;
        case SECT_BEACH:        map[x][y]="{yb";        break;
        default:                map[x][y]="{yo";
}

    for (door = 0; door < MAX_MAP_DIR; door++)
    {
        if ((pexit = room->exit[door]) != NULL && pexit->u1.to_room != NULL
            && can_see_room(ch,pexit->u1.to_room)  /* optional */
            && !IS_SET(pexit->exit_info, EX_CLOSED))
        {
            /* if exit there */
            prospect_room = pexit->u1.to_room;

            if (prospect_room->exit[rev_dir[door]] && prospect_room->exit[rev_dir[door]]->u1.to_room!=room)
            {
                /* if not two way */
                if ((prospect_room->sector_type == SECT_CITY) || (prospect_room->sector_type == SECT_INSIDE))
                {
                    map[x][y]="{W@";
                }
                else
                {
                    map[x][y]="{D?";
                }

                return;
            } /* end two way */

            if ((x <= min) || (y <= min) || (x >= max) || (y >= max))
            {
                return;
            }

            if (map[x + offsets[door][0]][y + offsets[door][1]] == NULL)
            {
                map_area(pexit->u1.to_room, ch, x+offsets[door][0], y+offsets[door][1], min, max);
            }

        } /* end if exit there */
    }

    return;
}

/*
 * Method to handle the actual sending of the map to the player.
 */
void show_map(CHAR_DATA *ch, int min, int max)
{
    char buf[MAX_STRING_LENGTH * 4];
    int x,y;

    for (x = min; x < max; ++x)
    {
        buf[0] = '\0';

        for (y = min; y < max; ++y)
        {
            if (map[x][y] == NULL)
            {
                strcat(buf, " ");
            }
            else
            {
                strcat(buf, map[x][y]);
            }
        }

        strcat(buf,"\r\n");
        send_to_char(buf, ch);
    }

    return;
}

/*
 * Command to show the player the ASCII map representation of the immediate
 * area around them.
 */
void do_map(CHAR_DATA *ch, char *argument)
{
    int size, center, x, y, min, max;
    char arg1[MAX_STRING_LENGTH];

    one_argument(argument, arg1);
    size = atoi(arg1);

    size = URANGE(7,size,36);
    center = MAX_MAP / 2;

    min = MAX_MAP / 2 - size / 2;
    max = MAX_MAP / 2 + size / 2;

    for (x = 0; x < MAX_MAP; ++x)
    {
        for (y = 0; y < MAX_MAP; ++y)
        {
            map[x][y]=NULL;
        }
    }

    /* starts the mapping with the center room */
    map_area(ch->in_room, ch, center, center, min, max);

    /* marks the center, where ch is */
    map[center][center] = "{R*{x";
    show_map(ch, min, max);

    return;
}

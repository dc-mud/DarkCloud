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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"

/*
 * Function finds out if a seed will grow into something else.
 */
void seed_grow_check(OBJ_DATA *obj)
{
    OBJ_DATA *obj_temp;
    int chance = 0;
    int count = 0;

    // Initial checks to ditch out, object must exist, room must exist
    // and the seed must be buried.  Also no growing in certain sectors
    // should an item somehow get buried there.
    if (obj == NULL
        || obj->in_room == NULL
        || obj->item_type != ITEM_SEED
        || !IS_OBJ_STAT(obj, ITEM_BURIED))
    {
        return;
    }

    // Places it should never be buried.
    if (obj->in_room->sector_type == SECT_INSIDE
        || obj->in_room->sector_type == SECT_WATER_SWIM
        || obj->in_room->sector_type == SECT_WATER_NOSWIM
        || obj->in_room->sector_type == SECT_UNDERWATER
        || obj->in_room->sector_type == SECT_AIR)
    {
        bugf("Item buried in a sector_type (%d) it shouldn't be at room %d", obj->in_room->sector_type,  obj->in_room->vnum);
        return;
    }

    // If the object doesn't have a vnum set to grow, report it, extract it.
    if (obj->value[0] == 0)
    {
        bugf("Seed vnum %d does not have a vnum set for the item to grow, extracting object.", obj->pIndexData->vnum);
        separate_obj(obj);
        extract_obj(obj);
        return;
    }

    // Setup the base chance based off of sector type of the room it's planted in.
    switch (obj->in_room->sector_type)
    {
        case SECT_DESERT:
            chance -= 75;
            break;
        case SECT_BEACH:
            chance -= 50;
            break;
        case SECT_FIELD:
            chance += 35;
            break;
        case SECT_MOUNTAIN:
            chance -= 15;
            break;
        case SECT_HILLS:
        case SECT_FOREST:
            chance += 25;
            break;
        default:
            chance = 0;
            break;
    }

    if (IS_OBJ_STAT(obj, ITEM_BLESS))
    {
        chance += 5;
    }

    if (IS_OBJ_STAT(obj, ITEM_GLOW))
    {
        chance += 5;
    }

    if (IS_OBJ_STAT(obj, ITEM_EVIL))
    {
        chance -= 5;
    }

    // Add the immortal initial set chance plus randomness.
    chance += obj->value[1];
    chance += number_range(1, 20);

    // Count any other seeds that might be buried in the room.
    for (obj_temp = obj->in_room->contents; obj_temp; obj_temp = obj_temp->next_content)
    {
        if (obj_temp->item_type == ITEM_SEED && IS_OBJ_STAT(obj_temp, ITEM_BURIED))
        {
            count++;
        }
    }

    // The more seeds that are planted the less chance any will grow.
    if (count > 3)
    {
        chance -= (3 * count);
    }

     // Seeds have a greaater chance of growing during the day.
    if (IS_DAY())
    {
        chance += 15;
    }
    else
    {
        chance -= 15;
    }

    // Always at least 1 percent chance, always a chance for failure
    chance = URANGE(1, chance, 95);

    // The moment of truth.
    if (!CHANCE(chance))
    {
        return;
    }

    // Remove the buried bit, it's going to get extracted but we want people to be able
    // to see it's short description in the act message and they won't unless it's not buried.
    separate_obj(obj);
    REMOVE_BIT(obj->extra_flags, ITEM_BURIED);

    // Show the room that something has sprouted.
    if (obj->in_room != NULL && obj->in_room->people != NULL)
    {
        act("$p sprouts and grows from the ground!", obj->in_room->people, obj, NULL, TO_ALL);
    }

    // Create the new object that has been grown, set it's intiial values and
    // put it in the room.
    obj_temp = create_object(get_obj_index(obj->value[0]));
    obj_to_room(obj_temp, obj->in_room);
    obj_temp->cost = ((obj->cost * 3) / 2);
    obj_temp->timer = number_range(75, 200); // Life span of the grown item

    // Extract the seed (it's already been separated, it's job in this world is complete.
    extract_obj(obj);

    return;
}

/*
 * Command to allow the player to show the terrain type of the room they
 * are in.
 */
void do_terrain(CHAR_DATA *ch, char *argument)
{
    if (ch == NULL || ch->in_room == NULL)
    {
        return;
    }

    switch (ch->in_room->sector_type)
    {
        case(SECT_INSIDE):
            send_to_char("You are indoors.\r\n", ch);
            break;
        case(SECT_CITY):
            send_to_char("You see the city about you... not a lot of terrain.\r\n", ch);
            break;
        case(SECT_FIELD):
            send_to_char("The terrain is that of fields.\r\n", ch);
            break;
        case(SECT_FOREST):
            send_to_char("The terrain is that of the forest.\r\n", ch);
            break;
        case(SECT_HILLS):
            send_to_char("The terrain is that of the hills.\r\n", ch);
            break;
        case(SECT_MOUNTAIN):
            send_to_char("The terrain is that of the mountains.\r\n", ch);
            break;
        case(SECT_AIR):
            send_to_char("There is no terrain, your in the air!\r\n", ch);
            break;
        case(SECT_DESERT):
            send_to_char("The terrain is that of the desert.\r\n", ch);
            break;
        case(SECT_BEACH):
            send_to_char("The terrain is that of the beach.\r\n", ch);
            break;
        case(SECT_OCEAN):
            send_to_char("You are in the ocean!\r\n", ch);
            break;
        case(SECT_UNDERWATER):
            send_to_char("You are underwater!\r\n", ch);
            break;
        case(SECT_WATER_SWIM):
        case(SECT_WATER_NOSWIM):
            send_to_char("You are in the water.\r\n", ch);
            break;
        case(SECT_UNDERGROUND):
            send_to_char("The terrain is that of the underground.\r\n", ch);
            break;
        default:
            send_to_char("The terrain type is undetermined.\r\n", ch);
            bugf("Unhandled terrain type in do_terrain for vnum %d", ch->in_room->vnum);
            break;
    }

    act("$n takes a look around $mself and examines the terrain.", ch, NULL, NULL, TO_ROOM);
    return;

}

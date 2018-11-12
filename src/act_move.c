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
    #include <unistd.h>
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
#include "tables.h"

// The supported directions, there are corresponding DIR_* constants that are in merc.h that are in the 
// same order as this.
char *const dir_name[] = {
    "north", "east", "south", "west", "up", "down", "northwest", "northeast", "southwest", "southeast"
};

// This is the reverse direction to the above list (south is reverse to north, up to down, etc.)
const int rev_dir[] = {
    2, 3, 0, 1, 5, 4, 9, 8, 7, 6
};

// The movement lose per sector.  The position corresponds with the value of the SECT_* and the sector_flags table.
const int movement_loss[SECT_MAX] = {
    1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6, 30, 5, 10, 6
};

/*
 * Local functions.
 */
bool has_key (CHAR_DATA * ch, int key);

void move_char(CHAR_DATA * ch, int door, bool follow)
{
    CHAR_DATA *fch;
    CHAR_DATA *fch_next;
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;


    if (door < 0 || door > MAX_DIR - 1)
    {
        bug("Do_move: bad door %d.", door);
        return;
    }

    /*
     * Exit trigger, if activated, bail out. Only PCs are triggered.
     */
    if (!IS_NPC(ch) && mp_exit_trigger(ch, door))
        return;

    in_room = ch->in_room;
    if ((pexit = in_room->exit[door]) == NULL
        || (to_room = pexit->u1.to_room) == NULL
        || !can_see_room(ch, pexit->u1.to_room))
    {
        send_to_char("Alas, you cannot go that way.\r\n", ch);
        return;
    }

    if (IS_SET(pexit->exit_info, EX_CLOSED)
        && (!IS_AFFECTED(ch, AFF_PASS_DOOR)
            || IS_SET(pexit->exit_info, EX_NOPASS)))
    {
        act("The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR);
        return;
    }

    if (IS_AFFECTED(ch, AFF_CHARM)
        && ch->master != NULL && in_room == ch->master->in_room)
    {
        send_to_char("What?  And leave your beloved master?\r\n", ch);
        return;
    }

    if (!is_room_owner(ch, to_room) && room_is_private(to_room))
    {
        send_to_char("That room is private right now.\r\n", ch);
        return;
    }

    if (!IS_NPC(ch))
    {
        int iGuild;
        int move;
        bool water_walk = FALSE;

        // Is it a guild room?  If so, see if they can go in.
        if (IS_SET(to_room->room_flags, ROOM_GUILD))
        {
            bool guildFound = FALSE;

            for (iGuild = 0; iGuild < MAX_GUILD; iGuild++)
            {
                if (to_room->vnum == class_table[ch->class]->guild[iGuild])
                {
                    guildFound = TRUE;
                }
            }

            if (!guildFound)
            {
                send_to_char("That is not your guild.  You aren't allowed in there.\r\n", ch);
                return;
            }

        }

        if (in_room->sector_type == SECT_AIR
            || to_room->sector_type == SECT_AIR)
        {
            if (!IS_AFFECTED(ch, AFF_FLYING) && !IS_IMMORTAL(ch))
            {
                send_to_char("You can't fly.\r\n", ch);
                return;
            }
        }

        // Ocean, see if they have water walk on.
        if (in_room->sector_type == SECT_WATER_NOSWIM
            || to_room->sector_type == SECT_WATER_NOSWIM
            || to_room->sector_type == SECT_OCEAN)
        {
            water_walk = is_affected(ch, gsn_water_walk);
        }

        // Water or ocean.. although we let people swim now with a skill, so
        // the no swim only makes it a requirement that they have swim.
        if ((in_room->sector_type == SECT_WATER_NOSWIM
            || to_room->sector_type == SECT_WATER_NOSWIM
            || to_room->sector_type == SECT_OCEAN)
            && !IS_AFFECTED(ch, AFF_FLYING))
        {
            bool found;

            /*
             * Look for a boat.
             */
            found = has_item_type(ch, ITEM_BOAT);

            // Immortal gets a courtesy boat, so do those with the water walk spell on.
            if (IS_IMMORTAL(ch) || water_walk)
            {
                found = TRUE;
            }

            // Check swim skill, this should be the last one as we're going to deal
            // some damage and return out at this point if none of the other cases
            // are met.
            if (!found)
            {
                if (CHANCE_SKILL(ch, gsn_swim))
                {
                    // Success, they can swim.
                    found = TRUE;
                    check_improve(ch, gsn_swim, TRUE, 2);
                }
                else
                {
                    // Failure on swim, they can suck down some water.
                    act("You are not able to swim and {Rchoke{x on a big gulp of {Cwater{x!", ch, NULL, NULL, TO_CHAR);
                    act("$n is not able to swim and {Rchokes{x on a big gulp of {Cwater{x!", ch, NULL, NULL, TO_ROOM);
                    ch->hit -= number_range(5, 10);
                    check_death(ch, DAM_DROWNING);
                    check_improve(ch, gsn_swim, FALSE, 5);
                    return;
                }

            }

            if (!found)
            {
                send_to_char("You need a boat, to be able to swim or to be flying to go there.\r\n", ch);
                return;
            }
        }

        // Average the movement cost of the room your in with the room that you're going to.
        move = movement_loss[UMIN(SECT_MAX - 1, in_room->sector_type)] + movement_loss[UMIN(SECT_MAX - 1, to_room->sector_type)];
        move /= 2;

        // Immortals don't lose movement, also, players on water with the water walk
        // spell on don't lose movement (water walk will only be true if they're on
        // water somewhere and have the spell on).
        if (IS_IMMORTAL(ch) || water_walk)
            move = 0;

        /* conditional effects */
        if (IS_AFFECTED(ch, AFF_FLYING) || IS_AFFECTED(ch, AFF_HASTE))
            move /= 2;

        if (IS_AFFECTED(ch, AFF_SLOW) || is_affected(ch, gsn_heaviness))
            move *= 2;

        // Merit - Light Footed, 25% chance of a 20% movement reduction cost
        if (!IS_NPC(ch) && IS_SET(ch->pcdata->merit, MERIT_LIGHT_FOOTED && CHANCE(25)))
        {
            move = (move * 8) / 10;
        }

        if (ch->move < move)
        {
            send_to_char("You are too exhausted.\r\n", ch);
            return;
        }

        // Lag per movement, more lag in the ocean, less out of it.  I don't
        // want to over do it here, so the ocean is the only sector type we
        // are really going to slow down since we don't want people swimming
        // the worlds length in the ocean.
        if (in_room->sector_type == SECT_OCEAN && !IS_IMMORTAL(ch))
        {
            // Ocean Lag, Phew
            WAIT_STATE(ch, 12);
        }
        else
        {
            // Normal Movement Lag
            WAIT_STATE(ch, 1);
        }

        ch->move -= move;
    }

    // Break camp if they are moving, no need to spam the room though with the
    // message if a bunch of people are in a group.
    if (is_affected(ch, gsn_camping))
    {
        // Extra line feeds to make it stand out since moving will show
        // the next room description and make it hard to see.
        send_to_char("\r\nYou break camp.\r\n\r\n", ch);
        affect_strip(ch, gsn_camping);
    }

    if (!IS_AFFECTED(ch, AFF_SNEAK)
        && !is_affected(ch, gsn_quiet_movement)
        && ch->invis_level < LEVEL_HERO)
    {
        act("$n leaves $T.", ch, NULL, dir_name[door], TO_ROOM);
    }

    // Remove camouflage before moving.
    if (is_affected(ch, gsn_camouflage))
    {
        send_to_char("You are no longer camouflaged.\r\n", ch);
        affect_strip(ch, gsn_camouflage);
    }

    // Move the character
    char_from_room(ch);
    char_to_room(ch, to_room);

    if (!IS_AFFECTED(ch, AFF_SNEAK)
        && !is_affected(ch, gsn_quiet_movement)
        && ch->invis_level < LEVEL_HERO)
    {
        act("$n has arrived.", ch, NULL, NULL, TO_ROOM);
    }

    do_function(ch, &do_look, "auto");

    if (in_room == to_room)        /* no circular follows */
        return;

    for (fch = in_room->people; fch != NULL; fch = fch_next)
    {
        fch_next = fch->next_in_room;

        // If the victim is charmed, make them stand up so they will follow.
        if (fch->master == ch && IS_AFFECTED(fch, AFF_CHARM) && fch->position < POS_STANDING)
        {
            do_function(fch, &do_stand, "");
        }

        if (fch->master == ch && fch->position == POS_STANDING && can_see_room(fch, to_room))
        {

            if (IS_SET(ch->in_room->room_flags, ROOM_LAW) && (IS_NPC(fch) && IS_SET(fch->act, ACT_AGGRESSIVE)))
            {
                act("You can't bring $N into the city.", ch, NULL, fch, TO_CHAR);
                act("You aren't allowed in the city.", fch, NULL, NULL, TO_CHAR);
                continue;
            }

            // If the players wisdom or intelligence is on the higher end then we're going to let
            // them know the direction they followed, otherwise, they'll follow as they always did
            // without the direction in the act message.
            if (get_curr_stat(fch, STAT_INT) >= 20 || get_curr_stat(fch, STAT_WIS) >= 20)
            {
                char buf[MAX_STRING_LENGTH];
                sprintf(buf, "You follow $N %s.", dir_name[door]);
                act(buf, fch, NULL, ch, TO_CHAR);
            }
            else
            {
                act("You follow $N", fch, NULL, ch, TO_CHAR);
            }

            move_char(fch, door, TRUE);
        }
    }

    /*
     * If someone is following the char, these triggers get activated
     * for the followers before the char, but it's safer this way...
     */
    if (IS_NPC(ch) && HAS_TRIGGER(ch, TRIG_ENTRY))
        mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_ENTRY);
    if (!IS_NPC(ch))
        mp_greet_trigger(ch);

    return;
}

void do_north(CHAR_DATA * ch, char *argument)
{
    move_char(ch, DIR_NORTH, FALSE);
    return;
}

void do_east(CHAR_DATA * ch, char *argument)
{
    move_char(ch, DIR_EAST, FALSE);
    return;
}

void do_south(CHAR_DATA * ch, char *argument)
{
    move_char(ch, DIR_SOUTH, FALSE);
    return;
}

void do_west(CHAR_DATA * ch, char *argument)
{
    move_char(ch, DIR_WEST, FALSE);
    return;
}

void do_up(CHAR_DATA * ch, char *argument)
{
    move_char(ch, DIR_UP, FALSE);
    return;
}

void do_down(CHAR_DATA * ch, char *argument)
{
    move_char(ch, DIR_DOWN, FALSE);
    return;
}

void do_northeast(CHAR_DATA * ch, char *argument)
{
    move_char(ch, DIR_NORTHEAST, FALSE);
    return;
}

void do_northwest(CHAR_DATA * ch, char *argument)
{
    move_char(ch, DIR_NORTHWEST, FALSE);
    return;
}

void do_southeast(CHAR_DATA * ch, char *argument)
{
    move_char(ch, DIR_SOUTHEAST, FALSE);
    return;
}

void do_southwest(CHAR_DATA * ch, char *argument)
{
    move_char(ch, DIR_SOUTHWEST, FALSE);
    return;
}

/*
 * Returns the int direction for the exit for the given english verbiage (this does
 * not check door names)
 */
int direction(char *arg)
{
    if (!str_cmp(arg, "n") || !str_cmp(arg, "north"))
    {
        return 0;
    }
    else if (!str_cmp(arg, "e") || !str_cmp(arg, "east"))
    {
        return 1;
    }
    else if (!str_cmp(arg, "s") || !str_cmp(arg, "south"))
    {
        return 2;
    }
    else if (!str_cmp(arg, "w") || !str_cmp(arg, "west"))
    {
        return 3;
    }
    else if (!str_cmp(arg, "u") || !str_cmp(arg, "up"))
    {
        return 4;
    }
    else if (!str_cmp(arg, "d") || !str_cmp(arg, "down"))
    {
        return 5;
    }
   else if (!str_cmp(arg, "nw") || !str_cmp(arg, "northwest"))
    {
        return 6;
    }
    else if (!str_cmp(arg, "ne") || !str_cmp(arg, "northeast"))
    {
        return 7;
    }
    else if (!str_cmp(arg, "sw") || !str_cmp(arg, "southwest"))
    {
        return 8;
    }
    else if (!str_cmp(arg, "se") || !str_cmp(arg, "southeast"))
    {
        return 9;
    }
    else
    {
        return -1;
    }
}

int find_door(CHAR_DATA * ch, char *arg, bool silent)
{
    EXIT_DATA *pexit;
    int door;

    if (!str_cmp(arg, "n") || !str_cmp(arg, "north"))
        door = 0;
    else if (!str_cmp(arg, "e") || !str_cmp(arg, "east"))
        door = 1;
    else if (!str_cmp(arg, "s") || !str_cmp(arg, "south"))
        door = 2;
    else if (!str_cmp(arg, "w") || !str_cmp(arg, "west"))
        door = 3;
    else if (!str_cmp(arg, "u") || !str_cmp(arg, "up"))
        door = 4;
    else if (!str_cmp(arg, "d") || !str_cmp(arg, "down"))
        door = 5;
    else if (!str_cmp(arg, "nw") || !str_cmp(arg, "northwest"))
        door = 6;
    else if (!str_cmp(arg, "ne") || !str_cmp(arg, "northeast"))
        door = 7;
    else if (!str_cmp(arg, "sw") || !str_cmp(arg, "southwest"))
        door = 8;
    else if (!str_cmp(arg, "se") || !str_cmp(arg, "southeast"))
        door = 9;
    else
    {
        for (door = 0; door < MAX_DIR; door++)
        {
            if ((pexit = ch->in_room->exit[door]) != NULL
                && IS_SET(pexit->exit_info, EX_ISDOOR)
                && pexit->keyword != NULL && is_name(arg, pexit->keyword))
                return door;
        }

        if (!silent)
        {
            act("I see no $T here.", ch, NULL, arg, TO_CHAR);
        }

        return -1;
    }

    if ((pexit = ch->in_room->exit[door]) == NULL)
    {
        if (!silent)
        {
            act("I see no door $T of here.", ch, NULL, arg, TO_CHAR);
        }

        return -1;
    }

    if (!IS_SET(pexit->exit_info, EX_ISDOOR))
    {
        if (!silent)
        {
            send_to_char("You can't do that.\r\n", ch);
        }

        return -1;
    }

    return door;
}

void do_open(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Open what?\r\n", ch);
        return;
    }

    if ((obj = get_obj_here(ch, arg)) != NULL)
    {
        /* open portal */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET(obj->value[1], EX_ISDOOR))
            {
                send_to_char("You can't do that.\r\n", ch);
                return;
            }

            if (!IS_SET(obj->value[1], EX_CLOSED))
            {
                send_to_char("It's already open.\r\n", ch);
                return;
            }

            if (IS_SET(obj->value[1], EX_LOCKED))
            {
                send_to_char("It's locked.\r\n", ch);
                return;
            }

            REMOVE_BIT(obj->value[1], EX_CLOSED);
            act("You open $p.", ch, obj, NULL, TO_CHAR);
            act("$n opens $p.", ch, obj, NULL, TO_ROOM);
            return;
        }

        /* 'open object' */
        if (obj->item_type != ITEM_CONTAINER)
        {
            send_to_char("That's not a container.\r\n", ch);
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSED))
        {
            send_to_char("It's already open.\r\n", ch);
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSEABLE))
        {
            send_to_char("You can't do that.\r\n", ch);
            return;
        }
        if (IS_SET(obj->value[1], CONT_LOCKED))
        {
            send_to_char("It's locked.\r\n", ch);
            return;
        }

        REMOVE_BIT(obj->value[1], CONT_CLOSED);
        act("You open $p.", ch, obj, NULL, TO_CHAR);
        act("$n opens $p.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if ((door = find_door(ch, arg, FALSE)) >= 0)
    {
        /* 'open door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if (!IS_SET(pexit->exit_info, EX_CLOSED))
        {
            send_to_char("It's already open.\r\n", ch);
            return;
        }
        if (IS_SET(pexit->exit_info, EX_LOCKED))
        {
            send_to_char("It's locked.\r\n", ch);
            return;
        }

        REMOVE_BIT(pexit->exit_info, EX_CLOSED);
        act("$n opens the $d.", ch, NULL, pexit->keyword, TO_ROOM);
        send_to_char("Ok.\r\n", ch);

        /* open the other side */
        if ((to_room = pexit->u1.to_room) != NULL
            && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
            && pexit_rev->u1.to_room == ch->in_room)
        {
            CHAR_DATA *rch;

            REMOVE_BIT(pexit_rev->exit_info, EX_CLOSED);
            for (rch = to_room->people; rch != NULL; rch = rch->next_in_room)
                act("The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR);
        }
    }

    return;
}

void do_close(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Close what?\r\n", ch);
        return;
    }

    if ((obj = get_obj_here(ch, arg)) != NULL)
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {

            if (!IS_SET(obj->value[1], EX_ISDOOR)
                || IS_SET(obj->value[1], EX_NOCLOSE))
            {
                send_to_char("You can't do that.\r\n", ch);
                return;
            }

            if (IS_SET(obj->value[1], EX_CLOSED))
            {
                send_to_char("It's already closed.\r\n", ch);
                return;
            }

            SET_BIT(obj->value[1], EX_CLOSED);
            act("You close $p.", ch, obj, NULL, TO_CHAR);
            act("$n closes $p.", ch, obj, NULL, TO_ROOM);
            return;
        }

        /* 'close object' */
        if (obj->item_type != ITEM_CONTAINER)
        {
            send_to_char("That's not a container.\r\n", ch);
            return;
        }
        if (IS_SET(obj->value[1], CONT_CLOSED))
        {
            send_to_char("It's already closed.\r\n", ch);
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSEABLE))
        {
            send_to_char("You can't do that.\r\n", ch);
            return;
        }

        SET_BIT(obj->value[1], CONT_CLOSED);
        act("You close $p.", ch, obj, NULL, TO_CHAR);
        act("$n closes $p.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if ((door = find_door(ch, arg, FALSE)) >= 0)
    {
        /* 'close door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if (IS_SET(pexit->exit_info, EX_CLOSED))
        {
            send_to_char("It's already closed.\r\n", ch);
            return;
        }

        SET_BIT(pexit->exit_info, EX_CLOSED);
        act("$n closes the $d.", ch, NULL, pexit->keyword, TO_ROOM);
        send_to_char("Ok.\r\n", ch);

        /* close the other side */
        if ((to_room = pexit->u1.to_room) != NULL
            && (pexit_rev = to_room->exit[rev_dir[door]]) != 0
            && pexit_rev->u1.to_room == ch->in_room)
        {
            CHAR_DATA *rch;

            SET_BIT(pexit_rev->exit_info, EX_CLOSED);
            for (rch = to_room->people; rch != NULL; rch = rch->next_in_room)
                act("The $d closes.", rch, NULL, pexit_rev->keyword,
                    TO_CHAR);
        }
    }

    return;
}

bool has_key(CHAR_DATA * ch, int key)
{
    OBJ_DATA *obj;

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
        if (obj->pIndexData->vnum == key)
            return TRUE;
    }

    return FALSE;
}

void do_lock(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Lock what?\r\n", ch);
        return;
    }

    if ((obj = get_obj_here(ch, arg)) != NULL)
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET(obj->value[1], EX_ISDOOR)
                || IS_SET(obj->value[1], EX_NOCLOSE))
            {
                send_to_char("You can't do that.\r\n", ch);
                return;
            }
            if (!IS_SET(obj->value[1], EX_CLOSED))
            {
                send_to_char("It's not closed.\r\n", ch);
                return;
            }

            if (obj->value[4] < 0 || IS_SET(obj->value[1], EX_NOLOCK))
            {
                send_to_char("It can't be locked.\r\n", ch);
                return;
            }

            if (!has_key(ch, obj->value[4]))
            {
                send_to_char("You lack the key.\r\n", ch);
                return;
            }

            if (IS_SET(obj->value[1], EX_LOCKED))
            {
                send_to_char("It's already locked.\r\n", ch);
                return;
            }

            SET_BIT(obj->value[1], EX_LOCKED);
            act("You lock $p.", ch, obj, NULL, TO_CHAR);
            act("$n locks $p.", ch, obj, NULL, TO_ROOM);
            return;
        }

        /* 'lock object' */
        if (obj->item_type != ITEM_CONTAINER)
        {
            send_to_char("That's not a container.\r\n", ch);
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSED))
        {
            send_to_char("It's not closed.\r\n", ch);
            return;
        }
        if (obj->value[2] < 0)
        {
            send_to_char("It can't be locked.\r\n", ch);
            return;
        }
        if (!has_key(ch, obj->value[2]))
        {
            send_to_char("You lack the key.\r\n", ch);
            return;
        }
        if (IS_SET(obj->value[1], CONT_LOCKED))
        {
            send_to_char("It's already locked.\r\n", ch);
            return;
        }

        SET_BIT(obj->value[1], CONT_LOCKED);
        act("You lock $p.", ch, obj, NULL, TO_CHAR);
        act("$n locks $p.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if ((door = find_door(ch, arg, FALSE)) >= 0)
    {
        /* 'lock door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if (!IS_SET(pexit->exit_info, EX_CLOSED))
        {
            send_to_char("It's not closed.\r\n", ch);
            return;
        }
        if (pexit->key < 0)
        {
            send_to_char("It can't be locked.\r\n", ch);
            return;
        }
        if (!has_key(ch, pexit->key))
        {
            send_to_char("You lack the key.\r\n", ch);
            return;
        }
        if (IS_SET(pexit->exit_info, EX_LOCKED))
        {
            send_to_char("It's already locked.\r\n", ch);
            return;
        }

        SET_BIT(pexit->exit_info, EX_LOCKED);
        send_to_char("*Click*\r\n", ch);
        act("$n locks the $d.", ch, NULL, pexit->keyword, TO_ROOM);

        /* lock the other side */
        if ((to_room = pexit->u1.to_room) != NULL
            && (pexit_rev = to_room->exit[rev_dir[door]]) != 0
            && pexit_rev->u1.to_room == ch->in_room)
        {
            SET_BIT(pexit_rev->exit_info, EX_LOCKED);
        }
    }

    return;
}

void do_unlock(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Unlock what?\r\n", ch);
        return;
    }

    if ((obj = get_obj_here(ch, arg)) != NULL)
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET(obj->value[1], EX_ISDOOR))
            {
                send_to_char("You can't do that.\r\n", ch);
                return;
            }

            if (!IS_SET(obj->value[1], EX_CLOSED))
            {
                send_to_char("It's not closed.\r\n", ch);
                return;
            }

            if (obj->value[4] < 0)
            {
                send_to_char("It can't be unlocked.\r\n", ch);
                return;
            }

            if (!has_key(ch, obj->value[4]))
            {
                send_to_char("You lack the key.\r\n", ch);
                return;
            }

            if (!IS_SET(obj->value[1], EX_LOCKED))
            {
                send_to_char("It's already unlocked.\r\n", ch);
                return;
            }

            REMOVE_BIT(obj->value[1], EX_LOCKED);
            act("You unlock $p.", ch, obj, NULL, TO_CHAR);
            act("$n unlocks $p.", ch, obj, NULL, TO_ROOM);
            return;
        }

        /* 'unlock object' */
        if (obj->item_type != ITEM_CONTAINER)
        {
            send_to_char("That's not a container.\r\n", ch);
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSED))
        {
            send_to_char("It's not closed.\r\n", ch);
            return;
        }
        if (obj->value[2] < 0)
        {
            send_to_char("It can't be unlocked.\r\n", ch);
            return;
        }
        if (!has_key(ch, obj->value[2]))
        {
            send_to_char("You lack the key.\r\n", ch);
            return;
        }
        if (!IS_SET(obj->value[1], CONT_LOCKED))
        {
            send_to_char("It's already unlocked.\r\n", ch);
            return;
        }

        REMOVE_BIT(obj->value[1], CONT_LOCKED);
        act("You unlock $p.", ch, obj, NULL, TO_CHAR);
        act("$n unlocks $p.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if ((door = find_door(ch, arg, FALSE)) >= 0)
    {
        /* 'unlock door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if (!IS_SET(pexit->exit_info, EX_CLOSED))
        {
            send_to_char("It's not closed.\r\n", ch);
            return;
        }
        if (pexit->key < 0)
        {
            send_to_char("It can't be unlocked.\r\n", ch);
            return;
        }
        if (!has_key(ch, pexit->key))
        {
            send_to_char("You lack the key.\r\n", ch);
            return;
        }
        if (!IS_SET(pexit->exit_info, EX_LOCKED))
        {
            send_to_char("It's already unlocked.\r\n", ch);
            return;
        }

        REMOVE_BIT(pexit->exit_info, EX_LOCKED);
        send_to_char("*Click*\r\n", ch);
        act("$n unlocks the $d.", ch, NULL, pexit->keyword, TO_ROOM);

        /* unlock the other side */
        if ((to_room = pexit->u1.to_room) != NULL
            && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
            && pexit_rev->u1.to_room == ch->in_room)
        {
            REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);
        }
    }

    return;
}

void do_pick(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    OBJ_DATA *obj;
    int door;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Pick what?\r\n", ch);
        return;
    }

    WAIT_STATE(ch, skill_table[gsn_pick_lock]->beats);

    /* look for guards */
    for (gch = ch->in_room->people; gch; gch = gch->next_in_room)
    {
        if (IS_NPC(gch) && IS_AWAKE(gch) && ch->level + 5 < gch->level)
        {
            act("$N is standing too close to the lock.",
                ch, NULL, gch, TO_CHAR);
            return;
        }
    }

    if (!IS_NPC(ch) && number_percent() > get_skill(ch, gsn_pick_lock))
    {
        send_to_char("You failed.\r\n", ch);
        check_improve(ch, gsn_pick_lock, FALSE, 2);
        return;
    }

    if ((obj = get_obj_here(ch, arg)) != NULL)
    {
        /* portal stuff */
        if (obj->item_type == ITEM_PORTAL)
        {
            if (!IS_SET(obj->value[1], EX_ISDOOR))
            {
                send_to_char("You can't do that.\r\n", ch);
                return;
            }

            if (!IS_SET(obj->value[1], EX_CLOSED))
            {
                send_to_char("It's not closed.\r\n", ch);
                return;
            }

            if (obj->value[4] < 0)
            {
                send_to_char("It can't be unlocked.\r\n", ch);
                return;
            }

            if (IS_SET(obj->value[1], EX_PICKPROOF))
            {
                send_to_char("You failed.\r\n", ch);
                return;
            }

            separate_obj(obj);
            REMOVE_BIT(obj->value[1], EX_LOCKED);
            act("You pick the lock on $p.", ch, obj, NULL, TO_CHAR);
            act("$n picks the lock on $p.", ch, obj, NULL, TO_ROOM);
            check_improve(ch, gsn_pick_lock, TRUE, 2);
            return;
        }

        /* 'pick object' */
        if (obj->item_type != ITEM_CONTAINER)
        {
            send_to_char("That's not a container.\r\n", ch);
            return;
        }
        if (!IS_SET(obj->value[1], CONT_CLOSED))
        {
            send_to_char("It's not closed.\r\n", ch);
            return;
        }
        if (obj->value[2] < 0)
        {
            send_to_char("It can't be unlocked.\r\n", ch);
            return;
        }
        if (!IS_SET(obj->value[1], CONT_LOCKED))
        {
            send_to_char("It's already unlocked.\r\n", ch);
            return;
        }
        if (IS_SET(obj->value[1], CONT_PICKPROOF))
        {
            send_to_char("You failed.\r\n", ch);
            return;
        }

        REMOVE_BIT(obj->value[1], CONT_LOCKED);
        act("You pick the lock on $p.", ch, obj, NULL, TO_CHAR);
        act("$n picks the lock on $p.", ch, obj, NULL, TO_ROOM);
        check_improve(ch, gsn_pick_lock, TRUE, 2);
        return;
    }

    if ((door = find_door(ch, arg, FALSE)) >= 0)
    {
        /* 'pick door' */
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];
        if (!IS_SET(pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch))
        {
            send_to_char("It's not closed.\r\n", ch);
            return;
        }
        if (pexit->key < 0 && !IS_IMMORTAL(ch))
        {
            send_to_char("It can't be picked.\r\n", ch);
            return;
        }
        if (!IS_SET(pexit->exit_info, EX_LOCKED))
        {
            send_to_char("It's already unlocked.\r\n", ch);
            return;
        }
        if (IS_SET(pexit->exit_info, EX_PICKPROOF) && !IS_IMMORTAL(ch))
        {
            send_to_char("You failed.\r\n", ch);
            return;
        }

        REMOVE_BIT(pexit->exit_info, EX_LOCKED);
        send_to_char("*Click*\r\n", ch);
        act("$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM);
        check_improve(ch, gsn_pick_lock, TRUE, 2);

        /* pick the other side */
        if ((to_room = pexit->u1.to_room) != NULL
            && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
            && pexit_rev->u1.to_room == ch->in_room)
        {
            REMOVE_BIT(pexit_rev->exit_info, EX_LOCKED);
        }
    }

    return;
}

/*
 * Puts a player into a standing position, ready to move or fight.
 */
void do_stand(CHAR_DATA * ch, char *argument)
{
    OBJ_DATA *obj = NULL;

    if (argument[0] != '\0')
    {
        if (ch->position == POS_FIGHTING)
        {
            send_to_char("Maybe you should finish fighting first?\r\n", ch);
            return;
        }

        obj = get_obj_list(ch, argument, ch->in_room->contents);

        if (obj == NULL)
        {
            send_to_char("You don't see that here.\r\n", ch);
            return;
        }

        if (obj->item_type != ITEM_FURNITURE
            || (!IS_SET(obj->value[2], STAND_AT)
                && !IS_SET(obj->value[2], STAND_ON)
                && !IS_SET(obj->value[2], STAND_IN)))
        {
            send_to_char("You can't seem to find a place to stand.\r\n", ch);
            return;
        }

        if (ch->on != obj && count_users(obj) >= obj->value[0])
        {
            act_new("There's no room to stand on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
            return;
        }

        ch->on = obj;
    }

    switch (ch->position)
    {
        case POS_SLEEPING:
            if (IS_AFFECTED(ch, AFF_SLEEP))
            {
                send_to_char("You can't wake up!\r\n", ch);
                return;
            }

            if (obj == NULL)
            {
                send_to_char("You wake and stand up.\r\n", ch);
                act("$n wakes and stands up.", ch, NULL, NULL, TO_ROOM);
                ch->on = NULL;
            }
            else if (IS_SET(obj->value[2], STAND_AT))
            {
                act_new("You wake and stand at $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
                act("$n wakes and stands at $p.", ch, obj, NULL, TO_ROOM);
            }
            else if (IS_SET(obj->value[2], STAND_ON))
            {
                act_new("You wake and stand on $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
                act("$n wakes and stands on $p.", ch, obj, NULL, TO_ROOM);
            }
            else
            {
                act_new("You wake and stand in $p.", ch, obj, NULL, TO_CHAR, POS_DEAD);
                act("$n wakes and stands in $p.", ch, obj, NULL, TO_ROOM);
            }
            ch->position = POS_STANDING;
            do_function(ch, &do_look, "auto");
            break;

        case POS_RESTING:
        case POS_SITTING:
            if (obj == NULL)
            {
                send_to_char("You stand up.\r\n", ch);
                act("$n stands up.", ch, NULL, NULL, TO_ROOM);
                ch->on = NULL;
            }
            else if (IS_SET(obj->value[2], STAND_AT))
            {
                act("You stand at $p.", ch, obj, NULL, TO_CHAR);
                act("$n stands at $p.", ch, obj, NULL, TO_ROOM);
            }
            else if (IS_SET(obj->value[2], STAND_ON))
            {
                act("You stand on $p.", ch, obj, NULL, TO_CHAR);
                act("$n stands on $p.", ch, obj, NULL, TO_ROOM);
            }
            else
            {
                act("You stand in $p.", ch, obj, NULL, TO_CHAR);
                act("$n stands on $p.", ch, obj, NULL, TO_ROOM);
            }
            ch->position = POS_STANDING;
            break;

        case POS_STANDING:
            send_to_char("You are already standing.\r\n", ch);
            break;

        case POS_FIGHTING:
            send_to_char("You are already fighting!\r\n", ch);
            break;
    }

    // Remove healing dream if they are no longer asleep
    affect_strip(ch, gsn_healing_dream);

    return;
} // end do_stand

/*
 * Puts a player into a resting position, either on the floor or on a
 * piece of furniture.  Resting players will regen health at a higher
 * rate than standing (but less than sleeping).
 */
void do_rest(CHAR_DATA * ch, char *argument)
{
    OBJ_DATA *obj = NULL;

    if (ch->position == POS_FIGHTING)
    {
        send_to_char("You are already fighting!\r\n", ch);
        return;
    }

    // No resting in the ocean
    if (ch->in_room && ch->in_room->sector_type == SECT_OCEAN)
    {
        send_to_char("You better keep treading water instead instead of resting!\r\n", ch);
        return;
    }

    // Cannot rest while flying.
    if (IS_AFFECTED(ch, AFF_FLYING))
    {
        send_to_char("You can not rest while flying, you must land first.\r\n", ch);
        return;
    }

    /* okay, now that we know we can rest, find an object to rest on */
    if (argument[0] != '\0')
    {
        obj = get_obj_list(ch, argument, ch->in_room->contents);
        if (obj == NULL)
        {
            send_to_char("You don't see that here.\r\n", ch);
            return;
        }
    }
    else
        obj = ch->on;

    if (obj != NULL)
    {
        if (obj->item_type != ITEM_FURNITURE
            || (!IS_SET(obj->value[2], REST_ON)
                && !IS_SET(obj->value[2], REST_IN)
                && !IS_SET(obj->value[2], REST_AT)))
        {
            send_to_char("You can't rest on that.\r\n", ch);
            return;
        }

        if (obj != NULL && ch->on != obj
            && count_users(obj) >= obj->value[0])
        {
            act_new("There's no more room on $p.", ch, obj, NULL, TO_CHAR,
                POS_DEAD);
            return;
        }

        ch->on = obj;
    }

    switch (ch->position)
    {
        case POS_SLEEPING:
            if (IS_AFFECTED(ch, AFF_SLEEP))
            {
                send_to_char("You can't wake up!\r\n", ch);
                return;
            }

            if (obj == NULL)
            {
                send_to_char("You wake up and start resting.\r\n", ch);
                act("$n wakes up and starts resting.", ch, NULL, NULL,
                    TO_ROOM);
            }
            else if (IS_SET(obj->value[2], REST_AT))
            {
                act_new("You wake up and rest at $p.",
                    ch, obj, NULL, TO_CHAR, POS_SLEEPING);
                act("$n wakes up and rests at $p.", ch, obj, NULL, TO_ROOM);
            }
            else if (IS_SET(obj->value[2], REST_ON))
            {
                act_new("You wake up and rest on $p.",
                    ch, obj, NULL, TO_CHAR, POS_SLEEPING);
                act("$n wakes up and rests on $p.", ch, obj, NULL, TO_ROOM);
            }
            else
            {
                act_new("You wake up and rest in $p.",
                    ch, obj, NULL, TO_CHAR, POS_SLEEPING);
                act("$n wakes up and rests in $p.", ch, obj, NULL, TO_ROOM);
            }
            ch->position = POS_RESTING;
            break;

        case POS_RESTING:
            send_to_char("You are already resting.\r\n", ch);
            break;

        case POS_STANDING:
            if (obj == NULL)
            {
                send_to_char("You rest.\r\n", ch);
                act("$n sits down and rests.", ch, NULL, NULL, TO_ROOM);
            }
            else if (IS_SET(obj->value[2], REST_AT))
            {
                act("You sit down at $p and rest.", ch, obj, NULL, TO_CHAR);
                act("$n sits down at $p and rests.", ch, obj, NULL, TO_ROOM);
            }
            else if (IS_SET(obj->value[2], REST_ON))
            {
                act("You sit on $p and rest.", ch, obj, NULL, TO_CHAR);
                act("$n sits on $p and rests.", ch, obj, NULL, TO_ROOM);
            }
            else
            {
                act("You rest in $p.", ch, obj, NULL, TO_CHAR);
                act("$n rests in $p.", ch, obj, NULL, TO_ROOM);
            }
            ch->position = POS_RESTING;
            break;

        case POS_SITTING:
            if (obj == NULL)
            {
                send_to_char("You rest.\r\n", ch);
                act("$n rests.", ch, NULL, NULL, TO_ROOM);
            }
            else if (IS_SET(obj->value[2], REST_AT))
            {
                act("You rest at $p.", ch, obj, NULL, TO_CHAR);
                act("$n rests at $p.", ch, obj, NULL, TO_ROOM);
            }
            else if (IS_SET(obj->value[2], REST_ON))
            {
                act("You rest on $p.", ch, obj, NULL, TO_CHAR);
                act("$n rests on $p.", ch, obj, NULL, TO_ROOM);
            }
            else
            {
                act("You rest in $p.", ch, obj, NULL, TO_CHAR);
                act("$n rests in $p.", ch, obj, NULL, TO_ROOM);
            }
            ch->position = POS_RESTING;
            break;
    }


    return;
} // end do_rest

/*
 * Puts a player into a sitting position, either on the floor or on a
 * piece of furniture.  Sitting players will regen health at a higher
 * rate than standing (but less than resting & sleeping).
 */
void do_sit(CHAR_DATA * ch, char *argument)
{
    OBJ_DATA *obj = NULL;

    if (ch->position == POS_FIGHTING)
    {
        send_to_char("Maybe you should finish this fight first?\r\n", ch);
        return;
    }

    // No sitting in the ocean
    if (ch->in_room && ch->in_room->sector_type == SECT_OCEAN)
    {
        send_to_char("You better keep treading water instead instead of sitting!\r\n", ch);
        return;
    }

    /* okay, now that we know we can sit, find an object to sit on */
    if (argument[0] != '\0')
    {
        obj = get_obj_list(ch, argument, ch->in_room->contents);
        if (obj == NULL)
        {
            send_to_char("You don't see that here.\r\n", ch);
            return;
        }
    }
    else
        obj = ch->on;

    if (obj != NULL)
    {
        if (obj->item_type != ITEM_FURNITURE
            || (!IS_SET(obj->value[2], SIT_ON)
                && !IS_SET(obj->value[2], SIT_IN)
                && !IS_SET(obj->value[2], SIT_AT)))
        {
            send_to_char("You can't sit on that.\r\n", ch);
            return;
        }

        if (obj != NULL && ch->on != obj
            && count_users(obj) >= obj->value[0])
        {
            act_new("There's no more room on $p.", ch, obj, NULL, TO_CHAR,
                POS_DEAD);
            return;
        }

        ch->on = obj;
    }
    switch (ch->position)
    {
        case POS_SLEEPING:
            if (IS_AFFECTED(ch, AFF_SLEEP))
            {
                send_to_char("You can't wake up!\r\n", ch);
                return;
            }

            if (obj == NULL)
            {
                send_to_char("You wake and sit up.\r\n", ch);
                act("$n wakes and sits up.", ch, NULL, NULL, TO_ROOM);
            }
            else if (IS_SET(obj->value[2], SIT_AT))
            {
                act_new("You wake and sit at $p.", ch, obj, NULL, TO_CHAR,
                    POS_DEAD);
                act("$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM);
            }
            else if (IS_SET(obj->value[2], SIT_ON))
            {
                act_new("You wake and sit on $p.", ch, obj, NULL, TO_CHAR,
                    POS_DEAD);
                act("$n wakes and sits at $p.", ch, obj, NULL, TO_ROOM);
            }
            else
            {
                act_new("You wake and sit in $p.", ch, obj, NULL, TO_CHAR,
                    POS_DEAD);
                act("$n wakes and sits in $p.", ch, obj, NULL, TO_ROOM);
            }

            ch->position = POS_SITTING;
            break;
        case POS_RESTING:
            if (obj == NULL)
                send_to_char("You stop resting.\r\n", ch);
            else if (IS_SET(obj->value[2], SIT_AT))
            {
                act("You sit at $p.", ch, obj, NULL, TO_CHAR);
                act("$n sits at $p.", ch, obj, NULL, TO_ROOM);
            }

            else if (IS_SET(obj->value[2], SIT_ON))
            {
                act("You sit on $p.", ch, obj, NULL, TO_CHAR);
                act("$n sits on $p.", ch, obj, NULL, TO_ROOM);
            }
            ch->position = POS_SITTING;
            break;
        case POS_SITTING:
            send_to_char("You are already sitting down.\r\n", ch);
            break;
        case POS_STANDING:
            if (obj == NULL)
            {
                send_to_char("You sit down.\r\n", ch);
                act("$n sits down on the ground.", ch, NULL, NULL, TO_ROOM);
            }
            else if (IS_SET(obj->value[2], SIT_AT))
            {
                act("You sit down at $p.", ch, obj, NULL, TO_CHAR);
                act("$n sits down at $p.", ch, obj, NULL, TO_ROOM);
            }
            else if (IS_SET(obj->value[2], SIT_ON))
            {
                act("You sit on $p.", ch, obj, NULL, TO_CHAR);
                act("$n sits on $p.", ch, obj, NULL, TO_ROOM);
            }
            else
            {
                act("You sit down in $p.", ch, obj, NULL, TO_CHAR);
                act("$n sits down in $p.", ch, obj, NULL, TO_ROOM);
            }
            ch->position = POS_SITTING;
            break;
    }
    return;
} // end do_sit

/*
 * Puts a player into a sleeping position, either on the floor or on a
 * piece of furniture.  Sleeping players will regen health at the best
 * rate.  They will also not be aware of what's going on in the room
 * around them.
 */
void do_sleep(CHAR_DATA * ch, char *argument)
{
    OBJ_DATA *obj = NULL;

    // No sleeping in the ocean
    if (ch->in_room && ch->in_room->sector_type == SECT_OCEAN)
    {
        send_to_char("You better keep treading water instead of sleeping!\r\n", ch);
        return;
    }

    // You cannot sleep while flying.
    if (IS_AFFECTED(ch, AFF_FLYING))
    {
        send_to_char("You can not sleep while flying, you must land first.\r\n", ch);
        return;
    }

    switch (ch->position)
    {
        case POS_SLEEPING:
            send_to_char("You are already sleeping.\r\n", ch);
            break;

        case POS_RESTING:
        case POS_SITTING:
        case POS_STANDING:
            if (argument[0] == '\0' && ch->on == NULL)
            {
                send_to_char("You go to sleep.\r\n", ch);
                act("$n goes to sleep.", ch, NULL, NULL, TO_ROOM);
                ch->position = POS_SLEEPING;
            }
            else
            {                    /* find an object and sleep on it */

                if (argument[0] == '\0')
                    obj = ch->on;
                else
                    obj = get_obj_list(ch, argument, ch->in_room->contents);

                if (obj == NULL)
                {
                    send_to_char("You don't see that here.\r\n", ch);
                    return;
                }
                if (obj->item_type != ITEM_FURNITURE
                    || (!IS_SET(obj->value[2], SLEEP_ON)
                        && !IS_SET(obj->value[2], SLEEP_IN)
                        && !IS_SET(obj->value[2], SLEEP_AT)))
                {
                    send_to_char("You can't sleep on that!\r\n", ch);
                    return;
                }

                if (ch->on != obj && count_users(obj) >= obj->value[0])
                {
                    act_new("There is no room on $p for you.",
                        ch, obj, NULL, TO_CHAR, POS_DEAD);
                    return;
                }

                ch->on = obj;
                if (IS_SET(obj->value[2], SLEEP_AT))
                {
                    act("You go to sleep at $p.", ch, obj, NULL, TO_CHAR);
                    act("$n goes to sleep at $p.", ch, obj, NULL, TO_ROOM);
                }
                else if (IS_SET(obj->value[2], SLEEP_ON))
                {
                    act("You go to sleep on $p.", ch, obj, NULL, TO_CHAR);
                    act("$n goes to sleep on $p.", ch, obj, NULL, TO_ROOM);
                }
                else
                {
                    act("You go to sleep in $p.", ch, obj, NULL, TO_CHAR);
                    act("$n goes to sleep in $p.", ch, obj, NULL, TO_ROOM);
                }
                ch->position = POS_SLEEPING;
            }
            break;

        case POS_FIGHTING:
            send_to_char("You are already fighting!\r\n", ch);
            break;
    }

    // Now that the above code as run, we should know if they are asleep or not, psionicists
    // can't cast healing dream on themselves because it can only be cast on sleeping people but
    // when they sleep it will automatically take affect for them.
    if (ch->position == POS_SLEEPING && ch->class == PSIONICIST_CLASS)
    {
        AFFECT_DATA af;
        affect_strip(ch, gsn_healing_dream);

        af.where = TO_AFFECTS;
        af.type = gsn_healing_dream;
        af.level = ch->level;
        af.duration = -1;
        af.modifier = 0;
        af.location = APPLY_NONE;
        af.bitvector = 0;
        affect_to_char(ch, &af);
        send_to_char("You fall into a deep and restful sleep.\r\n", ch);
    }

    return;
} // end do_sleep

/*
 * Wake will either cause the current player to wake up and stand (with no argument)
 * or will attempt to wake up another player (e.g. wake Isaac).
 */
void do_wake(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
        do_function(ch, &do_stand, "");
        return;
    }

    if (!IS_AWAKE(ch))
    {
        send_to_char("You are asleep yourself!\r\n", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg)) == NULL)
    {
        send_to_char("They aren't here.\r\n", ch);
        return;
    }

    if (IS_AWAKE(victim))
    {
        act("$N is already awake.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (IS_AFFECTED(victim, AFF_SLEEP))
    {
        act("You can't wake $M!", ch, NULL, victim, TO_CHAR);
        return;
    }

    act_new("$n wakes you.", ch, NULL, victim, TO_VICT, POS_SLEEPING);
    do_function(victim, &do_stand, "");

    return;
} // end do_wake

/*
 * Skill that allows the player to walk quietly so others don't see them enter
 * or exit a room, also hides them from players without acute vision or who don't
 * have their detect spells up.  This is a skill, but people who have the light
 * footed merit also have access to it.
 */
void do_sneak(CHAR_DATA * ch, char *argument)
{
    AFFECT_DATA af;

    affect_strip(ch, gsn_sneak);

    if (IS_AFFECTED(ch, AFF_SNEAK))
    {
        send_to_char("You are already sneaking.\r\n", ch);
        return;
    }

    bool light_footed = FALSE;

    if (!IS_NPC(ch) && IS_SET(ch->pcdata->merit, MERIT_LIGHT_FOOTED))
    {
        light_footed = TRUE;
    }

    if ((number_percent() < get_skill(ch, gsn_sneak)) || light_footed)
    {
        check_improve(ch, gsn_sneak, TRUE, 3);
        af.where = TO_AFFECTS;
        af.type = gsn_sneak;
        af.level = ch->level;
        af.duration = ch->level;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = AFF_SNEAK;
        affect_to_char(ch, &af);
        send_to_char("You attempt to move silently.\r\n", ch);
    }
    else
    {
        send_to_char("You fail to move silently.\r\n", ch);
        check_improve(ch, gsn_sneak, FALSE, 3);
    }

    return;
}

/*
 * A skill allowing a player to hide from being seen by most others
 * who don't have acute vision.
 */
void do_hide(CHAR_DATA * ch, char *argument)
{
    send_to_char("You attempt to hide.\r\n", ch);

    if (IS_AFFECTED(ch, AFF_HIDE))
    {
        REMOVE_BIT(ch->affected_by, AFF_HIDE);
    }

    if (number_percent() < get_skill(ch, gsn_hide))
    {
        SET_BIT(ch->affected_by, AFF_HIDE);
        check_improve(ch, gsn_hide, TRUE, 3);
    }
    else
    {
        check_improve(ch, gsn_hide, FALSE, 3);
    }

    return;
}

/*
 * Contributed by Alander.
 */
void do_visible(CHAR_DATA * ch, char *argument)
{
    affect_strip(ch, gsn_invisibility);
    affect_strip(ch, gsn_mass_invisibility);
    affect_strip(ch, gsn_sneak);
    affect_strip(ch, gsn_quiet_movement);
    affect_strip(ch, gsn_camouflage);
    REMOVE_BIT(ch->affected_by, AFF_HIDE);
    REMOVE_BIT(ch->affected_by, AFF_INVISIBLE);
    REMOVE_BIT(ch->affected_by, AFF_SNEAK);
    send_to_char("You are now visible.\r\n", ch);
    return;
}

/*
 * Recall - This is a skill (a prayer to the gods) that will allow a person to return
 * to their recall point.  This will be kept alongside the word of recall spell because
 * classes that can't cast magic will need it to survive.  This skill can be enhanced by
 * the enhanced recall skill.  When that is practiced it will raise the chance it works
 * and lower the movement it costs.
 */
void do_recall(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    ROOM_INDEX_DATA *location;
    int recall_vnum = 0;

    if (IS_NPC(ch) && !IS_SET(ch->act, ACT_PET))
    {
        send_to_char("Only players can recall.\r\n", ch);
        return;
    }

    // Cannot recall until a few ticks after battle with another player.
    if (!IS_NPC(ch) && ch->pcdata->pk_timer > 0)
    {
        send_to_char("You failed!\r\n", ch);
        return;
    }

    // If this is a player and they have a custom recall set to a bind stone
    // then use that, otherwise use the temple.
    if (!IS_NPC(ch) && ch->pcdata->recall_vnum > 0)
    {
        recall_vnum = ch->pcdata->recall_vnum;
    }
    else
    {
        recall_vnum = ROOM_VNUM_TEMPLE;
    }

    act("$n prays for transportation!", ch, 0, 0, TO_ROOM);

    if ((location = get_room_index(recall_vnum)) == NULL)
    {
        send_to_char("You are completely lost.\r\n", ch);
        return;
    }

    if (ch->in_room == location)
    {
        return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
        || IS_AFFECTED(ch, AFF_CURSE)
        || IS_SET(ch->in_room->area->area_flags, AREA_NO_RECALL))
    {
        send_to_char("The gods have forsaken you.\r\n", ch);
        return;
    }

    if ((victim = ch->fighting) != NULL)
    {
        int lose, skill;

        skill = get_skill(ch, gsn_recall);

        if (number_percent() < 80 * skill / 100)
        {
            check_improve(ch, gsn_recall, FALSE, 6);
            WAIT_STATE(ch, 4);
            sprintf(buf, "You failed!\r\n");
            send_to_char(buf, ch);
            return;
        }

        lose = (ch->desc != NULL) ? 25 : 50;
        gain_exp(ch, 0 - lose);
        check_improve(ch, gsn_recall, TRUE, 4);
        sprintf(buf, "You recall from combat!  You lose %d experience.\r\n", lose);
        send_to_char(buf, ch);
        stop_fighting(ch, TRUE);

    }

    // Movement penalty if you are not an immortal.  If you have the enhanced recall
    // skill and pass the skill check you will lose less movement.
    if (!IS_IMMORTAL(ch))
    {
        if (CHANCE_SKILL(ch, gsn_enhanced_recall))
        {
            // They passed the enhanced recall check, they only lose 25% of movement.
            ch->move = (ch->move * 3) / 4;
            check_improve(ch, gsn_enhanced_recall, TRUE, 4);
        }
        else
        {
            // Normal recall, costs 50% of movement
            ch->move /= 2;
        }
    }

    act("$n disappears.", ch, NULL, NULL, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, location);
    act("$n appears in the room.", ch, NULL, NULL, TO_ROOM);
    do_function(ch, &do_look, "auto");

    if (ch->pet != NULL)
    {
        do_function(ch->pet, &do_recall, "");
    }

    // Priest, agony spell check/processing
    agony_damage_check(ch);

    return;
}

/*
 * Command that will allow a player to set their recall point to the current
 * room they are in if a bind stone is in the room.
 */
void do_bind(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA * obj;
    ROOM_INDEX_DATA *location;

    if (ch == NULL || IS_NPC(ch))
    {
        return;
    }

    // Reset the recall vnum to the temple of the reset argument is issued, this can
    // be done without a bind stone in the room.
    if (!IS_NULLSTR(argument) && (!str_cmp(argument, "reset")))
    {
        // If they're in a clan, reset them to their clan's recall, otherwise, use the temple.
        if (is_clan(ch))
        {
            ROOM_INDEX_DATA *location;

            // Make sure the recall point is valid, then set it.
            if ((location = get_room_index(clan_table[ch->clan].recall_vnum)) != NULL)
            {
                ch->pcdata->recall_vnum = clan_table[ch->clan].recall_vnum;
                sendf(ch, "Your recall point has been reset to {c%s{x in {c%s{x.\r\n", location->name, location->area->name);
                return;
            }
        }

        ch->pcdata->recall_vnum = ROOM_VNUM_TEMPLE;
        send_to_char("Your recall point has been reset.\r\n", ch);
        return;
    }
    else if ((location = get_room_index(ch->pcdata->recall_vnum)) == NULL)
    {
        // The recall vnum is null... reset it
        ch->pcdata->recall_vnum = ROOM_VNUM_TEMPLE;
        send_to_char("Your recall point has been reset.\r\n", ch);
        return;
    }

    if (ch->in_room == NULL)
    {
        send_to_char("Failed!\r\n", ch);
        return;
    }

    // This looks for objects with the keyword bindstone in the keywords, fear not though, we will
    // actually check it to make sure it's the bindstone vnum as to prevent hacks from things like
    // parchments being named with this in the keywords.
    obj = get_obj_list(ch, "bindstone", ch->in_room->contents);

    if (obj == NULL || obj->pIndexData == NULL || obj->pIndexData->vnum != OBJ_VNUM_BIND_STONE)
    {
        send_to_char("There is no bind stone here.\r\n", ch);
        sprintf(buf, "Your current bind stone is set to {c%s{x in {c%s{x.\r\n", location->name, location->area->name);
        send_to_char(buf, ch);
        return;
    }

    sprintf(buf, "Your recall point has been bound to {c%s{x in {c%s{x.\r\n", ch->in_room->name, ch->in_room->area->name);
    send_to_char(buf, ch);
    ch->pcdata->recall_vnum = ch->in_room->vnum;
    return;
}

/*
 * The ability to train a stat, health, mana or movement.
 */
void do_train(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *mob;
    int stat = -1;
    char *pOutput = NULL;
    int cost;

    // Must be a player to train
    if (IS_NPC(ch))
    {
        return;
    }

    // Check for a trainer.
    for (mob = ch->in_room->people; mob; mob = mob->next_in_room)
    {
        if (IS_NPC(mob) && IS_SET(mob->act, ACT_TRAIN))
        {
            break;
        }
    }

    if (mob == NULL)
    {
        send_to_char("You can't do that here.\r\n", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        sprintf(buf, "You have %d training sessions.\r\n", ch->train);
        send_to_char(buf, ch);
        argument = "foo";
    }

    cost = 1;

    if (!str_cmp(argument, "str"))
    {
        stat = STAT_STR;
        pOutput = "strength";
    }
    else if (!str_cmp(argument, "int"))
    {
        stat = STAT_INT;
        pOutput = "intelligence";
    }
    else if (!str_cmp(argument, "wis"))
    {
        stat = STAT_WIS;
        pOutput = "wisdom";
    }
    else if (!str_cmp(argument, "dex"))
    {
        stat = STAT_DEX;
        pOutput = "dexterity";
    }

    else if (!str_cmp(argument, "con"))
    {
        stat = STAT_CON;
        pOutput = "constitution";
    }
    else if (!str_cmp(argument, "hp"))
    {
        cost = 1;
    }
    else if (!str_cmp(argument, "mana"))
    {
        cost = 1;
    }
    else
    {
        strcpy(buf, "You can train:");

        if (ch->perm_stat[STAT_STR] < get_max_train(ch, STAT_STR))
        {
            strcat(buf, " str");
        }
        if (ch->perm_stat[STAT_INT] < get_max_train(ch, STAT_INT))
        {
            strcat(buf, " int");
        }
        if (ch->perm_stat[STAT_WIS] < get_max_train(ch, STAT_WIS))
        {
            strcat(buf, " wis");
        }
        if (ch->perm_stat[STAT_DEX] < get_max_train(ch, STAT_DEX))
        {
            strcat(buf, " dex");
        }
        if (ch->perm_stat[STAT_CON] < get_max_train(ch, STAT_CON))
        {
            strcat(buf, " con");
        }

        // hp and mana are always available to train as long as trains exist.
        strcat(buf, " hp mana\r\n");
        send_to_char(buf, ch);
        return;
    }

    if (!str_cmp("hp", argument))
    {
        if (cost > ch->train)
        {
            send_to_char("You don't have enough training sessions.\r\n", ch);
            return;
        }

        ch->train -= cost;
        ch->pcdata->perm_hit += 10;
        ch->max_hit += 10;
        ch->hit += 10;
        act("Your durability increases!", ch, NULL, NULL, TO_CHAR);
        act("$n's durability increases!", ch, NULL, NULL, TO_ROOM);
        return;
    }

    if (!str_cmp("mana", argument))
    {
        if (cost > ch->train)
        {
            send_to_char("You don't have enough training sessions.\r\n", ch);
            return;
        }

        ch->train -= cost;
        ch->pcdata->perm_mana += 10;
        ch->max_mana += 10;
        ch->mana += 10;
        act("Your power increases!", ch, NULL, NULL, TO_CHAR);
        act("$n's power increases!", ch, NULL, NULL, TO_ROOM);
        return;
    }

    // Make sure they are not going over the maximum amount of the stat for
    // their race/class (with any special modifiers as defined in get_max_train).
    if (ch->perm_stat[stat] >= get_max_train(ch, stat))
    {
        act("Your $T is already at maximum.", ch, NULL, pOutput, TO_CHAR);
        return;
    }

    // Check that they have enough trains
    if (cost > ch->train)
    {
        send_to_char("You don't have enough training sessions.\r\n", ch);
        return;
    }

    // Deduct the train, up the stat.
    ch->train -= cost;
    ch->perm_stat[stat] += 1;

    act("Your $T increases!", ch, NULL, pOutput, TO_CHAR);
    act("$n's $T increases!", ch, NULL, pOutput, TO_ROOM);
    return;
}

/*
 * Random room selection procedure.  This originated from Quixadhal of the
 * ICE project (among many others).
 */
ROOM_INDEX_DATA *get_random_room(CHAR_DATA *ch)
{
    /*
     * top_room is the number of rooms loaded. room_index_hash[MAX_KEY_HASH] is the set
     * of rooms. Each room is stored in rih[vnum % MAX_KEY_HASH].
     */

    ROOM_INDEX_DATA **pRooms = NULL;
    ROOM_INDEX_DATA *pRoomIndex = NULL;
    int iHash = 0;
    int room_count = 0;
    int target_room = 0;

    if (!(pRooms = (ROOM_INDEX_DATA **)calloc(top_room, sizeof(ROOM_INDEX_DATA *))))
    {
        bug("get_random_room: can't alloc %d pointers for room pointer table.", top_room);
    }

    /*
     * First, we need to filter out rooms that aren't valid choices
     */
    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
        for (pRoomIndex = room_index_hash[iHash];
        pRoomIndex != NULL; pRoomIndex = pRoomIndex->next)
        {
            /*
             * Skip private/safe/arena rooms, no hiding in there!
             */
            if (IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)
                || IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY)
                || IS_SET(pRoomIndex->room_flags, ROOM_SAFE)
                || IS_SET(pRoomIndex->room_flags, ROOM_ARENA))
                continue;

            /*
             * Skip the ocean or underwater rooms just to be nice.
             */
            if (pRoomIndex->sector_type == SECT_OCEAN
                || pRoomIndex->sector_type == SECT_UNDERWATER)
                continue;

            /*
             * Skip rooms the target can't "see"
             */
            if (!can_see_room(ch, pRoomIndex))
                continue;

            /*
             * Skip rooms that are considered private beyond the flags
             */
            if (room_is_private(pRoomIndex))
                continue;

            /*
             * Skip rooms that are LAW if the target is a non-PK player
             */
            if (IS_SET(pRoomIndex->room_flags, ROOM_LAW)
                && !IS_NPC(ch) && !IS_SET(ch->act, ACT_AGGRESSIVE))
                continue;

            pRooms[room_count++] = pRoomIndex;
        }
    }

    /*
     * Now, we pick a random number and grab the room pointer
     */
    target_room = number_range(0, room_count - 1);
    pRoomIndex = pRooms[target_room];
    free(pRooms);

    return pRoomIndex;
}

/* RT Enter portals */
void do_enter(CHAR_DATA * ch, char *argument)
{
    ROOM_INDEX_DATA *location;

    if (ch->fighting != NULL)
        return;

    /* nifty portal stuff */
    if (argument[0] != '\0')
    {
        ROOM_INDEX_DATA *old_room;
        OBJ_DATA *portal;
        CHAR_DATA *fch, *fch_next;

        old_room = ch->in_room;

        portal = get_obj_list(ch, argument, ch->in_room->contents);

        if (portal == NULL)
        {
            send_to_char("You don't see that here.\r\n", ch);
            return;
        }

        if (portal->item_type != ITEM_PORTAL
            || (IS_SET(portal->value[1], EX_CLOSED)))
        {
            send_to_char("You can't seem to find a way in.\r\n", ch);
            return;
        }

        if (!IS_SET(portal->value[2], GATE_NOCURSE)
            && (IS_AFFECTED(ch, AFF_CURSE)
            || IS_SET(old_room->room_flags, ROOM_NO_RECALL)))
        {
            send_to_char("Something prevents you from leaving...\r\n", ch);
            return;
        }

        if (IS_SET(portal->value[2], GATE_RANDOM) || portal->value[3] == -1)
        {
            location = get_random_room(ch);
            portal->value[3] = location->vnum;    /* for record keeping :) */
        }
        else if (IS_SET(portal->value[2], GATE_BUGGY) && (number_percent() < 5))
        {
            location = get_random_room(ch);
        }
        else
        {
            location = get_room_index(portal->value[3]);
        }

        if (location == NULL
            || location == old_room
            || !can_see_room(ch, location)
            || (room_is_private(location) && !IS_TRUSTED(ch, IMPLEMENTOR)))
        {
            act("$p doesn't seem to go anywhere.", ch, portal, NULL, TO_CHAR);
            return;
        }

        if (IS_NPC(ch)
            && IS_SET(ch->act, ACT_AGGRESSIVE)
            && IS_SET(location->room_flags, ROOM_LAW))
        {
            send_to_char("Something prevents you from leaving...\r\n", ch);
            return;
        }

        // Ghosts cannot enter a portal to an area that's flagged as a keep
        if (location->area != NULL
            && IS_SET(location->area->area_flags, AREA_KEEP)
            && IS_GHOST(ch))
        {
            send_to_char("Ghosts may not enter keeps.\r\n", ch);
            return;
        }

        // You can only enter a keep portal if your clan owns that keep.
        if (location->vnum == STORM_KEEP_PORTAL
            && ch->clan != settings.storm_keep_owner)
        {
            send_to_char("Only those who own that keep may enter that portal.\r\n", ch);
            return;
        }

        act("$n steps into $p.", ch, portal, NULL, TO_ROOM);

        if (IS_SET(portal->value[2], GATE_NORMAL_EXIT))
        {
            act("You enter $p.", ch, portal, NULL, TO_CHAR);
        }
        else
        {
            act("You walk through $p and find yourself somewhere else...", ch, portal, NULL, TO_CHAR);
        }

        char_from_room(ch);
        char_to_room(ch, location);

        if (IS_SET(portal->value[2], GATE_GOWITH))
        {
            /* take the gate along */
            obj_from_room(portal);
            obj_to_room(portal, location);
        }

        if (IS_SET(portal->value[2], GATE_NORMAL_EXIT))
        {
            act("$n has arrived.", ch, portal, NULL, TO_ROOM);
        }
        else
        {
            act("$n has arrived through $p.", ch, portal, NULL, TO_ROOM);
        }

        // Check to see if a keep lord is in the room and if so, they should notify
        // the owning clan that someone is attacking it.
        keep_lord_notify(ch);

        do_function(ch, &do_look, "auto");

        /* charges */
        if (portal->value[0] > 0)
        {
            portal->value[0]--;
            if (portal->value[0] == 0)
                portal->value[0] = -1;
        }

        /* protect against circular follows */
        if (old_room == location)
            return;

        for (fch = old_room->people; fch != NULL; fch = fch_next)
        {
            fch_next = fch->next_in_room;

            if (portal == NULL || portal->value[0] == -1)
                /* no following through dead portals */
                continue;

            if (fch->master == ch && IS_AFFECTED(fch, AFF_CHARM)
                && fch->position < POS_STANDING)
                do_function(fch, &do_stand, "");

            if (fch->master == ch && fch->position == POS_STANDING)
            {

                if (IS_SET(ch->in_room->room_flags, ROOM_LAW)
                    && (IS_NPC(fch) && IS_SET(fch->act, ACT_AGGRESSIVE)))
                {
                    act("You can't bring $N into the city.", ch, NULL, fch, TO_CHAR);
                    act("You aren't allowed in the city.", fch, NULL, NULL, TO_CHAR);
                    continue;
                }

                act("You follow $N.", fch, NULL, ch, TO_CHAR);
                do_function(fch, &do_enter, argument);
            }
        }

        if (portal != NULL && portal->value[0] == -1)
        {
            act("$p fades out of existence.", ch, portal, NULL, TO_CHAR);
            if (ch->in_room == old_room)
                act("$p fades out of existence.", ch, portal, NULL, TO_ROOM);
            else if (old_room->people != NULL)
            {
                act("$p fades out of existence.",
                    old_room->people, portal, NULL, TO_CHAR);
                act("$p fades out of existence.",
                    old_room->people, portal, NULL, TO_ROOM);
            }
            extract_obj(portal);
        }

        /*
        * If someone is following the char, these triggers get activated
        * for the followers before the char, but it's safer this way...
        */
        if (IS_NPC(ch) && HAS_TRIGGER(ch, TRIG_ENTRY))
            mp_percent_trigger(ch, NULL, NULL, NULL, TRIG_ENTRY);
        if (!IS_NPC(ch))
            mp_greet_trigger(ch);

        return;
    }

    send_to_char("Nope, can't do it.\r\n", ch);
    return;
} // end do_enter

/*
 * Land will allow a player to float to the ground removing any
 * flying spells/bits they have on them.  This is useful so a
 * player can land to rest/sleep without having to cancel all
 * of their spells.
 */
void do_land(CHAR_DATA * ch, char *argument)
{
    if (IS_AFFECTED(ch, AFF_FLYING))
    {
        affect_strip(ch, gsn_fly);
        act("You float gently to the ground.", ch, NULL, NULL, TO_CHAR);
        act("$n floats gently to the ground.", ch, NULL, NULL, TO_ROOM);
        ch->position = POS_STANDING;
    }
    else
    {
        send_to_char("You aren't flying.\r\n", ch);
    }
} // end do_land

/*****************************************************
 Path Find

 This code is copyright (C) 2001 by Brian Graversen.
 It may be used, modified and distributed freely, as
 long as you do not remove this copyright notice.

 Feel free to mail me if you like the code.

 E-mail: jobo@daimi.au.dk
******************************************************/

#define RID ROOM_INDEX_DATA

/* local varibles */
bool gFound;

/* local functions */
bool  examine_room         (RID *pRoom, RID *tRoom, AREA_DATA *pArea, int steps);
void  dijkstra             (RID *chRoom, RID *victRoom);
RID  *heap_getMinElement   (HEAP *heap);
HEAP *init_heap            (RID *root);

/*
 * This function will return a shortest path from
 * room 'from' to room 'to'. If no path could be found,
 * it will return NULL. This function will only return
 * a path if both rooms where in the same area, feel
 * free to remove that restriction, and modify the rest
 * of the code so it will work with this.
 */
char *pathfind(RID *from, RID *to)
{
    int const exit_names[] = {'n', 'e', 's', 'w', 'u', 'd', '6', '7', '8', '9'};
    RID *pRoom;
    AREA_DATA *pArea;
    static char path[MAX_STRING_LENGTH]; // should be plenty.
    int iPath = 0, vnum, door;
    bool found;

    if (from == to) return "";
    if ((pArea = from->area) != to->area) return NULL;

    /* initialize all rooms in the area */
    for (vnum = pArea->min_vnum; vnum < pArea->max_vnum; vnum++)
    {
        if ((pRoom = get_room_index(vnum)))
        {
            pRoom->visited = FALSE;
            for (door = 0; door < MAX_DIR; door++)
            {
                if (pRoom->exit[door] == NULL) continue;
                pRoom->exit[door]->color = FALSE;
            }
        }
    }

    /* initialize variables */
    pRoom = from;
    gFound = FALSE;

    /* In the first run, we only count steps, no coloring is done */
    dijkstra(pRoom, to);

    /* If the target room was never found, we return NULL */
    if (!gFound) return NULL;

    /* in the second run, we color the shortest path using the step counts */
    if (!examine_room(pRoom, to, pArea, 0))
        return NULL;

        /* then we follow the trace left by examine_room() */
    while (pRoom != to)
    {
        found = FALSE;
        for (door = 0; door < MAX_DIR && !found; door++)
        {
            if (pRoom->exit[door] == NULL) continue;
            if (pRoom->exit[door]->u1.to_room == NULL) continue;
            if (!pRoom->exit[door]->color) continue;

            pRoom->exit[door]->color = FALSE;
            found = TRUE;
            path[iPath] = exit_names[door];
            iPath++;
            pRoom = pRoom->exit[door]->u1.to_room;
        }
        if (!found)
        {
            bug("Pathfind: Fatal Error in %d.", pRoom->vnum);
            return NULL;
        }
    }

    path[iPath] = '\0';
    return path;
}

/*
 * This function will examine all rooms in the same area as chRoom,
 * setting their step count to the minimum amount of steps needed
 * to walk from chRoom to the room. If victRoom is encountered, it
 * will set the global bool gFound to TRUE. Running time for this
 * function is O(n*log(n)), where n is the amount of rooms in the area.
 */
void dijkstra(RID *chRoom, RID *victRoom)
{
    RID *pRoom;
    RID *tRoom;
    RID *xRoom;
    HEAP *heap;
    int door, x;
    bool stop;

    heap = init_heap(chRoom);
    while (heap->iVertice)
    {
        if ((pRoom = heap_getMinElement(heap)) == NULL)
        {
            bug("Dijstra: Getting NULL room", 0);
            return;
        }
        if (pRoom == victRoom)
            gFound = TRUE;

          /* update all exits */
        for (door = 0; door < MAX_DIR; door++)
        {
            if (pRoom->exit[door] == NULL) continue;
            if (pRoom->exit[door]->u1.to_room == NULL) continue;

            /* update step count, and swap in the heap */
            if (pRoom->exit[door]->u1.to_room->steps > pRoom->steps + 1)
            {
                xRoom = pRoom->exit[door]->u1.to_room;
                xRoom->steps = pRoom->steps + 1;
                stop = FALSE;
                while ((x = xRoom->heap_index) != 1 && !stop)
                {
                    if (heap->knude[x / 2]->steps > xRoom->steps)
                    {
                        tRoom = heap->knude[x / 2];
                        heap->knude[x / 2] = xRoom;
                        xRoom->heap_index = xRoom->heap_index / 2;
                        heap->knude[x] = tRoom;
                        heap->knude[x]->heap_index = x;
                    }
                    else stop = TRUE;
                }
            }
        }
    }

  /* free the heap */
    free(heap);
}

/*
 * This function walks through pArea, searching for tRoom,
 * while it colors the path it takes to get there. It will
 * return FALSE if tRoom was never encountered, and TRUE if
 * it does find the room.
 */
bool examine_room(RID *pRoom, RID *tRoom, AREA_DATA *pArea, int steps)
{
    int door;

    /* been here before, out of the area or can we get here faster */
    if (pRoom->area != pArea) return FALSE;
    if (pRoom->visited) return FALSE;
    if (pRoom->steps < steps) return FALSE;

    /* Have we found the room we are searching for */
    if (pRoom == tRoom)
        return TRUE;

      /* mark the room so we know we have been here */
    pRoom->visited = TRUE;

    /* Depth first traversel of all exits */
    for (door = 0; door < MAX_DIR; door++)
    {
        if (pRoom->exit[door] == NULL) continue;
        if (pRoom->exit[door]->u1.to_room == NULL) continue;

        /* assume we are walking the right way */
        pRoom->exit[door]->color = TRUE;

        /* recursive return */
        if (examine_room(pRoom->exit[door]->u1.to_room, tRoom, pArea, steps + 1))
            return TRUE;

          /* it seems we did not walk the right way */
        pRoom->exit[door]->color = FALSE;
    }
    return FALSE;
}

/*
 * This function will initialize a heap used for the
 * pathfinding algorithm. It will return a pointer to
 * the heap if it succedes, and NULL if it did not.
 */
HEAP *init_heap(RID *root)
{
    AREA_DATA *pArea;
    RID *pRoom;
    HEAP *heap;
    int i, size, vnum;

    if ((pArea = root->area) == NULL) return NULL;
    size = pArea->max_vnum - pArea->min_vnum;

    /* allocate memory for heap */
    heap = calloc(1, sizeof(*heap));
    heap->knude = calloc(2 * (size + 1), sizeof(ROOM_INDEX_DATA *));
    heap->size = 2 * (size + 1);

    /* we want the root at the beginning */
    heap->knude[1] = root;
    heap->knude[1]->steps = 0;
    heap->knude[1]->heap_index = 1;

    /* initializing the rest of the heap */
    for (i = 2, vnum = pArea->min_vnum; vnum < pArea->max_vnum; vnum++)
    {
        if ((pRoom = get_room_index(vnum)))
        {
            if (pRoom == root) continue;
            heap->knude[i] = pRoom;
            heap->knude[i]->steps = 2 * heap->size;
            heap->knude[i]->heap_index = i;
            i++;
        }
    }

    heap->iVertice = i - 1;

    /* setting the rest to NULL */
    for (; i < heap->size; i++)
        heap->knude[i] = NULL;

    return heap;
}

/*
 * This function will return the smallest element in the heap,
 * remove the element from the heap, and make sure the heap
 * is still complete.
 */
RID *heap_getMinElement(HEAP *heap)
{
    RID *tRoom;
    RID *pRoom;
    bool done = FALSE;
    int i = 1;

    /* this is the element we wish to return */
    pRoom = heap->knude[1];

    /* We move the last vertice to the front */
    heap->knude[1] = heap->knude[heap->iVertice];
    heap->knude[1]->heap_index = 1;

    /* Decrease the size of the heap and remove the last entry */
    heap->knude[heap->iVertice] = NULL;
    heap->iVertice--;

    /* Swap places till it fits */
    while (!done)
    {
        if (heap->knude[i] == NULL)
            done = TRUE;
        else if (heap->knude[2 * i] == NULL)
            done = TRUE;
        else if (heap->knude[2 * i + 1] == NULL)
        {
            if (heap->knude[i]->steps > heap->knude[2 * i]->steps)
            {
                tRoom = heap->knude[i];
                heap->knude[i] = heap->knude[2 * i];
                heap->knude[i]->heap_index = i;
                heap->knude[2 * i] = tRoom;
                heap->knude[2 * i]->heap_index = 2 * i;
                i = 2 * i;
            }
            done = TRUE;
        }
        else if (heap->knude[i]->steps <= heap->knude[2 * i]->steps &&
            heap->knude[i]->steps <= heap->knude[2 * i + 1]->steps)
            done = TRUE;
        else if (heap->knude[2 * i]->steps <= heap->knude[2 * i + 1]->steps)
        {
            tRoom = heap->knude[i];
            heap->knude[i] = heap->knude[2 * i];
            heap->knude[i]->heap_index = i;
            heap->knude[2 * i] = tRoom;
            heap->knude[2 * i]->heap_index = 2 * i;
            i = 2 * i;
        }
        else
        {
            tRoom = heap->knude[i];
            heap->knude[i] = heap->knude[2 * i + 1];
            heap->knude[i]->heap_index = i;
            heap->knude[2 * i + 1] = tRoom;
            heap->knude[2 * i + 1]->heap_index = 2 * i + 1;
            i = 2 * i + 1;
        }
    }

    /* return the element */
    return pRoom;
}

void do_pathfind(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    char arg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char *path;

    one_argument(argument, arg);
    if ((victim = get_char_world(ch, arg)) == NULL) return;

    if ((path = pathfind(ch->in_room, victim->in_room)) != NULL)
        sprintf(buf, "Path: %s\r\n", path);
    else
        sprintf(buf, "Path: Unknown.\r\n");
    send_to_char(buf, ch);

    return;
}

/* End Path Find Code */

/*
 * Command to allow a player to knock on a door if it's closed, if
 * players are on the other side of the door they will hear the knocking.
 */
void do_knock(CHAR_DATA * ch, char *argument)
{
    int door;
    char arg[MAX_INPUT_LENGTH];

    one_argument(argument, arg);

    if (IS_NULLSTR(arg))
    {
        send_to_char ("Knock on what?\r\n", ch);
        return;
    }

    if ((door = find_door(ch, arg, FALSE)) >= 0)
    {
        ROOM_INDEX_DATA *to_room;
        EXIT_DATA *pexit;
        EXIT_DATA *pexit_rev;

        pexit = ch->in_room->exit[door];

        // See if the door in question is open, if it is you can't knock on it.
        if (!IS_SET( pexit->exit_info, EX_CLOSED))
        {
            send_to_char("Why knock?  It's open.\r\n", ch);
            return;
        }

        act("$n knocks on the $d.", ch, NULL, pexit->keyword, TO_ROOM);
        act("You knock on the $d.", ch, NULL, pexit->keyword, TO_CHAR);

        // The knock, from the other side of the door
        if ((to_room = pexit->u1.to_room) != NULL
            && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
            && pexit_rev->u1.to_room == ch->in_room)
        {
            CHAR_DATA *rch;

            for (rch = to_room->people; rch != NULL; rch = rch->next_in_room)
            {
                act("You hear someone knocking.", rch, NULL, pexit_rev->keyword, TO_CHAR);
            }
        }
    }

    return;
}

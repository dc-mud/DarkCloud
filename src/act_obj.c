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
#include "interp.h"

/*
 * Local functions.
 */
#define CD CHAR_DATA
#define OD OBJ_DATA

bool remove_obj (CHAR_DATA * ch, int iWear, bool fReplace);
void wear_obj (CHAR_DATA * ch, OBJ_DATA * obj, bool fReplace);
CD *find_keeper (CHAR_DATA * ch);
int get_cost (CHAR_DATA * keeper, OBJ_DATA * obj, bool fBuy);
void obj_to_keeper (OBJ_DATA * obj, CHAR_DATA * ch);
OD *get_obj_keeper (CHAR_DATA * ch, CHAR_DATA * keeper, char *argument);
void outfit (CHAR_DATA *ch, int wear_position, int vnum);

#undef OD
#undef CD

/* RT part of the corpse looting code */

bool can_loot(CHAR_DATA * ch, OBJ_DATA * obj)
{
    CHAR_DATA *owner, *wch;

    if (IS_IMMORTAL(ch))
        return TRUE;

    if (!obj->owner || obj->owner == NULL)
        return TRUE;

    owner = NULL;
    for (wch = char_list; wch != NULL; wch = wch->next)
        if (!str_cmp(wch->name, obj->owner))
            owner = wch;

    if (owner == NULL)
        return TRUE;

    if (!str_cmp(ch->name, owner->name))
        return TRUE;

    if (!IS_NPC(owner) && IS_SET(owner->act, PLR_CANLOOT))
        return TRUE;

    if (is_same_group(ch, owner))
        return TRUE;

    return FALSE;
}

/*
 * Gets and object.
 */
void get_obj(CHAR_DATA * ch, OBJ_DATA * obj, OBJ_DATA * container)
{
    /* variables for AUTOSPLIT */
    CHAR_DATA *gch;
    int members;
    char buffer[100];

    if (!CAN_WEAR(obj, ITEM_TAKE))
    {
        send_to_char("You can't take that.\r\n", ch);
        return;
    }

    if (ch->carry_number + get_obj_number(obj) > can_carry_n(ch))
    {
        act("$d: you can't carry that many items.", ch, NULL, obj->name, TO_CHAR);
        return;
    }

    if ((!obj->in_obj || obj->in_obj->carried_by != ch)
        && (get_carry_weight(ch) + get_obj_weight(obj) > can_carry_w(ch)))
    {
        act("$d: you can't carry that much weight.", ch, NULL, obj->name, TO_CHAR);
        return;
    }

    if (get_carry_weight(ch) + get_obj_weight(obj) > can_carry_w(ch))
    {
        // weight bug fix
        act("$d: you can't carry that much weight.", ch, NULL, obj->name, TO_CHAR);
        return;
    }

    if (obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC && !can_loot(ch, obj))
    {
        act("Corpse looting is not permitted.", ch, NULL, NULL, TO_CHAR);
        return;
    }
    else if (!can_loot(ch, obj))
    {
        act("Looting is not permitted.", ch, NULL, NULL, TO_CHAR);
        return;
    }

    if (obj->in_room != NULL)
    {
        for (gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room)
        {
            if (gch->on == obj)
            {
                act("$N appears to be using $p.", ch, obj, gch, TO_CHAR);
                return;
            }
        }
    }

    if (container != NULL)
    {
        if (container->pIndexData->vnum == OBJ_VNUM_PIT && get_trust(ch) < obj->level)
        {
            send_to_char("You are not powerful enough to use it.\r\n", ch);
            return;
        }

        if (container->pIndexData->vnum == OBJ_VNUM_PIT
            && !CAN_WEAR(container, ITEM_TAKE)
            && !IS_OBJ_STAT(obj, ITEM_HAD_TIMER))
        {
            obj->timer = 0;
        }

        if (!(container->carried_by))
        {
            act("$n gets $o from $P.", ch, obj, container, TO_ROOM);
            act("You get $o from $P.", ch, obj, container, TO_CHAR);
        }
        else
        {
            act("$n gets $o from $P.", ch, obj, container, TO_ROOM);
            act("You get $o from $P (in inventory).", ch, obj, container, TO_CHAR);
        }

        REMOVE_BIT(obj->extra_flags, ITEM_HAD_TIMER);
        obj_from_obj(obj);

        if (obj->owner && obj->item_type != ITEM_CORPSE_PC)
        {
            free_string(obj->owner);
            obj->owner = NULL;
        }
    }
    else
    {
        act("You get $o.", ch, obj, container, TO_CHAR);
        act("$n gets $o.", ch, obj, container, TO_ROOM);
        obj_from_room(obj);

        if (obj->owner && obj->item_type != ITEM_CORPSE_PC)
        {
            free_string(obj->owner);
            obj->owner = NULL;
        }
    }

    if (obj->item_type == ITEM_MONEY)
    {
        ch->silver += obj->value[0];
        ch->gold += obj->value[1];

        if (IS_SET(ch->act, PLR_AUTOSPLIT))
        {
            // AUTOSPLIT code
            members = 0;
            for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
            {
                if (!IS_AFFECTED(gch, AFF_CHARM) && is_same_group(gch, ch))
                    members++;
            }

            if (members > 1 && (obj->value[0] > 1 || obj->value[1]))
            {
                sprintf(buffer, "%d %d", obj->value[0], obj->value[1]);
                do_function(ch, &do_split, buffer);
            }
        }

        extract_obj(obj);
    }
    else
    {
        // In case an immortal (with holy light) picks up a buried object, we want to remove the
        // buried flag.  Players can't pick up buried items in this manner.
        if (IS_SET(obj->extra_flags, ITEM_BURIED))
        {
            REMOVE_BIT(obj->extra_flags, ITEM_BURIED);
        }
        
        // Give the object to the person
        obj_to_char(obj, ch);
    }

    return;
} // end get_obj

/*
 * Gets an item or items from the ground, from a container, etc.
 */
void do_get(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    OBJ_DATA *container;
    int number;
    bool found;

    argument = one_argument(argument, arg1);

    if (is_number(arg1))
    {
        number = atoi(arg1);

        if (number < 1)
        {
            send_to_char("You want to take nothing?\r\n", ch);
            return;
        }

        argument = one_argument(argument, arg1);
    }
    else
    {
        number = 0;
    }

    argument = one_argument(argument, arg2);

    if (!str_cmp(arg2, "from"))
    {
        argument = one_argument(argument, arg2);
    }

    /* Get type. */
    if (arg1[0] == '\0')
    {
        send_to_char("Get what?\r\n", ch);
        return;
    }

    if (arg2[0] == '\0')
    {
        if (number <= 1 && str_cmp(arg1, "all") && str_prefix("all.", arg1))
        {
            /* 'get obj' */
            obj = get_obj_list(ch, arg1, ch->in_room->contents);

            if (obj == NULL)
            {
                act("I see no $T here.", ch, NULL, arg1, TO_CHAR);
                return;
            }

            separate_obj(obj);
            get_obj(ch, obj, NULL);
        }
        else
        {
            int cnt = 0;
            bool fAll;
            char *chk;

            /* 'get all' or 'get all.obj' */
            if (!str_cmp(arg1, "all"))
            {
                fAll = TRUE;
            }
            else
            {
                fAll = FALSE;
            }

            if (number > 1)
            {
                chk = arg1;
            }
            else
            {
                chk = &arg1[4];
            }

            /* 'get all' or 'get all.obj' */
            found = FALSE;

            for (obj = ch->in_room->contents; obj != NULL; obj = obj_next)
            {
                obj_next = obj->next_content;

                if ((fAll || is_name(chk, obj->name)) && can_see_obj(ch, obj))
                {
                    found = TRUE;

                    if (number && (cnt + obj->count) > number)
                    {
                        split_obj(obj, number - cnt);
                    }

                    cnt += obj->count;
                    get_obj(ch, obj, NULL);

                    if (number && cnt >= number)
                    {
                        break;
                    }
                }
            }

            if (!found)
            {
                if (arg1[3] == '\0')
                {
                    send_to_char("I see nothing here.\r\n", ch);
                }
                else
                {
                    act("I see no $T here.", ch, NULL, chk, TO_CHAR);
                }
            }
        }
    }
    else
    {
        /* 'get ... container' */
        if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2))
        {
            send_to_char("You can't do that.\r\n", ch);
            return;
        }

        if ((container = get_obj_here(ch, arg2)) == NULL)
        {
            act("I see no $T here.", ch, NULL, arg2, TO_CHAR);
            return;
        }

        switch (container->item_type)
        {
            default:
                send_to_char("That's not a container.\r\n", ch);
                return;
            case ITEM_CONTAINER:
            case ITEM_CORPSE_NPC:
                break;
            case ITEM_CORPSE_PC:
            {
                if (!can_loot(ch, container))
                {
                    send_to_char("You can't do that.\r\n", ch);
                    return;
                }
            }
        }

        if (IS_SET(container->value[1], CONT_CLOSED))
        {
            act("The $d is closed.", ch, NULL, container->name, TO_CHAR);
            return;
        }

        if (number <= 1 && str_cmp(arg1, "all") && str_prefix("all.", arg1))
        {
            /* 'get obj container' */
            obj = get_obj_list(ch, arg1, container->contains);
            number_argument(arg1, arg3);

            if (container->item_type == ITEM_CORPSE_PC && strlen(arg3) < 2)
            {
                act("I see nothing like that in the $T.", ch, NULL, arg2, TO_CHAR);
                return;
            }

            if (obj == NULL)
            {
                act("I see nothing like that in the $T.", ch, NULL, arg2, TO_CHAR);
                return;
            }

            separate_obj(obj);
            get_obj(ch, obj, container);
        }
        else
        {
            int cnt = 0;
            bool fAll;
            char *chk;

            /* 'get all container' or 'get all.obj container' */
            if (container->item_type == ITEM_CORPSE_PC)
            {
                if (!can_loot(ch, container))
                {
                    send_to_char("You can't do that.\r\n", ch);
                    return;
                }
            }

            if (!str_cmp(arg1, "all"))
            {
                fAll = TRUE;
            }
            else
            {
                fAll = FALSE;
            }

            if (number > 1)
            {
                chk = arg1;
            }
            else
            {
                chk = &arg1[4];
            }

            found = FALSE;

            for (obj = container->contains; obj != NULL; obj = obj_next)
            {
                obj_next = obj->next_content;

                if ((fAll || is_name(chk, obj->name)) && can_see_obj(ch, obj))
                {
                    found = TRUE;

                    if (container->pIndexData->vnum == OBJ_VNUM_PIT && !IS_IMMORTAL(ch))
                    {
                        send_to_char("Don't be so greedy!\r\n", ch);
                        return;
                    }

                    if (number && (cnt + obj->count) > number)
                    {
                        split_obj(obj, number - cnt);
                    }

                    cnt += obj->count;
                    get_obj(ch, obj, container);

                    if (number && cnt >= number)
                    {
                        break;
                    }
                }
            }

            if (!found)
            {
                if (arg1[3] == '\0')
                {
                    act("I see nothing in the $T.", ch, NULL, arg2, TO_CHAR);
                }
                else
                {
                    act("I see nothing like that in the $T.", ch, NULL, arg2, TO_CHAR);
                }
            }
        }
    }

    return;

} // end do_get

/*
 * Puts an item in or on something or somewhere.
 */
void do_put(CHAR_DATA *ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *container;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    int number;

    argument = one_argument(argument, arg1);

    if (is_number(arg1))
    {
        number = atoi(arg1);

        if (number < 1)
        {
            send_to_char("You want to put nothing somewhere?\r\n", ch);
            return;
        }

        argument = one_argument(argument, arg1);
    }
    else
    {
        number = 0;
    }

    argument = one_argument(argument, arg2);

    if (!str_cmp(arg2, "in") || !str_cmp(arg2, "on"))
    {
        argument = one_argument(argument, arg2);
    }

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
        send_to_char("Put what in what?\r\n", ch);
        return;
    }

    if (!str_cmp(arg2, "all") || !str_prefix("all.", arg2))
    {
        send_to_char("You can't do that.\r\n", ch);
        return;
    }

    if ((container = get_obj_here(ch, arg2)) == NULL)
    {
        act("I see no $T here.", ch, NULL, arg2, TO_CHAR);
        return;
    }

    if (container->item_type != ITEM_CONTAINER)
    {
        send_to_char("That's not a container.\r\n", ch);
        return;
    }

    if (IS_SET(container->value[1], CONT_CLOSED))
    {
        act("The $d is closed.", ch, NULL, container->name, TO_CHAR);
        return;
    }

    if (number <= 1 && str_cmp(arg1, "all") && str_prefix("all.", arg1))
    {
        /* 'put obj container' */
        if ((obj = get_obj_carry(ch, arg1, ch)) == NULL)
        {
            send_to_char("You do not have that item.\r\n", ch);
            return;
        }

        if (obj == container)
        {
            send_to_char("You can't fold it into itself.\r\n", ch);
            return;
        }

        if (!can_drop_obj(ch, obj))
        {
            send_to_char("You can't let go of it.\r\n", ch);
            return;
        }

        if (WEIGHT_MULT(obj) != 100)
        {
            send_to_char("You have a feeling that would be a bad idea.\r\n", ch);
            return;
        }

        if ((get_obj_weight(obj) / obj->count) + (get_true_weight(container) / container->count)
        > (container->value[0] * 10)
            || (get_obj_weight(obj) / obj->count) > (container->value[3] * 10))
        {
            send_to_char("It won't fit.\r\n", ch);
            return;
        }

        if (container->pIndexData->vnum == OBJ_VNUM_PIT && !CAN_WEAR(container, ITEM_TAKE))
        {
            if (obj->timer)
            {
                SET_BIT(obj->extra_flags, ITEM_HAD_TIMER);
            }
            else
            {
                obj->timer = number_range(100, 200);
            }
        }

        separate_obj(obj);
        separate_obj(container);
        obj_from_char(obj);

        if (obj->owner)
        {
            free_string(obj->owner);
            obj->owner = NULL;
        }

        if (IS_SET(container->value[1], CONT_PUT_ON))
        {
            if (!(container->carried_by))
            {
                act("$n puts $p on $P.", ch, obj, container, TO_ROOM);
                act("You put $p on $P.", ch, obj, container, TO_CHAR);
            }
            else
            {
                act("$n puts $p on $P.", ch, obj, container, TO_ROOM);
                act("You put $p on $P (in inventory).", ch, obj, container, TO_CHAR);
            }
        }
        else
        {
            if (!(container->carried_by))
            {
                act("$n puts $p in $P.", ch, obj, container, TO_ROOM);
                act("You put $p in $P.", ch, obj, container, TO_CHAR);
            }
            else
            {
                act("$n puts $p in $P.", ch, obj, container, TO_ROOM);
                act("You put $p in $P (in inventory).", ch, obj, container, TO_CHAR);
            }
        }

        obj = obj_to_obj(obj, container);
    }
    else
    {
        /* 'put all container' or 'put all.obj container' */
        int cnt = 0;
        bool fAll;
        char *chk;

        if (!str_cmp(arg1, "all"))
        {
            fAll = TRUE;
        }
        else
        {
            fAll = FALSE;
        }

        if (number > 1)
        {
            chk = arg1;
        }
        else
        {
            chk = &arg1[4];
        }

        separate_obj(container);

        for (obj = ch->carrying; obj != NULL; obj = obj_next)
        {
            obj_next = obj->next_content;

            if ((fAll || is_name(chk, obj->name))
                && can_see_obj(ch, obj)
                && WEIGHT_MULT(obj) == 100
                && obj->wear_loc == WEAR_NONE
                && obj != container
                && can_drop_obj(ch, obj)
                && (get_obj_weight(obj) / obj->count) + (get_true_weight(container) / container->count) <= (container->value[0] * 10)
                && (get_obj_weight(obj) / obj->count) < (container->value[3] * 10))
            {

                if (number && (cnt + obj->count) > number)
                {
                    split_obj(obj, number - cnt);
                }

                cnt += obj->count;

                if (container->pIndexData->vnum == OBJ_VNUM_PIT && !CAN_WEAR(obj, ITEM_TAKE))
                {
                    if (obj->timer)
                    {
                        SET_BIT(obj->extra_flags, ITEM_HAD_TIMER);
                    }
                    else
                    {
                        obj->timer = number_range(100, 200);
                    }
                }

                obj_from_char(obj);

                if (obj->owner)
                {
                    free_string(obj->owner);
                    obj->owner = NULL;
                }

                if (IS_SET(container->value[1], CONT_PUT_ON))
                {
                    act("$n puts $o on $P.", ch, obj, container, TO_ROOM);
                    act("You put $o on $P.", ch, obj, container, TO_CHAR);
                }
                else
                {
                    act("$n puts $o in $P.", ch, obj, container, TO_ROOM);
                    act("You put $o in $P.", ch, obj, container, TO_CHAR);
                }

                obj = obj_to_obj(obj, container);

                if (number && cnt >= number)
                {
                    break;
                }
            }
        }
    }

    return;
} // end do_put

/*
 * Drops an item, items or money.
 */
void do_drop(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    bool found;
    int number;
    bool coins = FALSE;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Drop what?\r\n", ch);
        return;
    }

    if (is_number(arg))
    {
        /* 'drop NNNN coins' */
        int amount, gold = 0, silver = 0;

        amount = atoi(arg);
        number = amount;
        argument = one_argument(argument, arg);

        if (amount <= 0)
        {
            send_to_char("Sorry, you can't do that.\r\n", ch);
            return;
        }

        if ((!str_cmp(arg, "coins") || !str_cmp(arg, "coin") || !str_cmp(arg, "silver"))
            && !(IS_SET(ch->act, ACT_PET) && IS_NPC(ch)))
        {
            if (ch->silver < amount)
            {
                send_to_char("You don't have that much silver.\r\n", ch);
                return;
            }

            ch->silver -= amount;
            silver = amount;
            coins = TRUE;
        }
        else if (!str_prefix(arg, "gold") && !(IS_SET(ch->act, ACT_PET) && IS_NPC(ch)))
        {
            if (ch->gold < amount)
            {
                send_to_char("You don't have that much gold.\r\n", ch);
                return;
            }

            ch->gold -= amount;
            gold = amount;
            coins = TRUE;
        }

        if (coins)
        {
            for (obj = ch->in_room->contents; obj != NULL; obj = obj_next)
            {
                obj_next = obj->next_content;

                switch (obj->pIndexData->vnum)
                {
                    case OBJ_VNUM_SILVER_ONE:
                        silver += 1;
                        extract_obj(obj);
                        break;
                    case OBJ_VNUM_GOLD_ONE:
                        gold += 1;
                        extract_obj(obj);
                        break;
                    case OBJ_VNUM_SILVER_SOME:
                        silver += obj->value[0];
                        extract_obj(obj);
                        break;
                    case OBJ_VNUM_GOLD_SOME:
                        gold += obj->value[1];
                        extract_obj(obj);
                        break;
                    case OBJ_VNUM_COINS:
                        silver += obj->value[0];
                        gold += obj->value[1];
                        extract_obj(obj);
                        break;
                }
            }

            obj_to_room(create_money(gold, silver), ch->in_room);
            act("$n drops some coins.", ch, NULL, NULL, TO_ROOM);
            send_to_char("OK.\r\n", ch);
            return;
        }
    }
    else
    {
        number = 0;
    }

    if (number <= 1 && str_cmp(arg, "all") && str_prefix("all.", arg))
    {
        /* 'drop obj' */
        if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
        {
            send_to_char("You do not have that item.\r\n", ch);
            return;
        }

        if (!can_drop_obj(ch, obj))
        {
            send_to_char("You can't let go of it.\r\n", ch);
            return;
        }

        separate_obj(obj);

        if (obj->owner)
        {
            free_string(obj->owner);
            obj->owner = NULL;
        }

        obj_from_char(obj);

        act("$n drops $p.", ch, obj, NULL, TO_ROOM);
        act("You drop $p.", ch, obj, NULL, TO_CHAR);
        obj = obj_to_room(obj, ch->in_room);

        if (IS_OBJ_STAT(obj, ITEM_MELT_DROP))
        {
            act("$p dissolves into smoke.", ch, obj, NULL, TO_ROOM);
            act("$p dissolves into smoke.", ch, obj, NULL, TO_CHAR);
            separate_obj(obj);
            extract_obj(obj);
        }
    }
    else
    {
        int cnt = 0;
        char *chk;
        bool fAll;

        if (!str_cmp(arg, "all"))
        {
            fAll = TRUE;
        }
        else
        {
            fAll = FALSE;
        }

        if (number > 1)
        {
            chk = arg;
        }
        else
        {
            chk = &arg[4];
        }

        /* 'drop all' or 'drop all.obj' */
        found = FALSE;
        for (obj = ch->carrying; obj; obj = obj_next)
        {
            obj_next = obj->next_content;

            if ((fAll || is_name(chk, obj->name))
                && can_see_obj(ch, obj)
                && obj->wear_loc == WEAR_NONE
                && can_drop_obj(ch, obj))
            {
                found = TRUE;

                if (number && (cnt + obj->count) > number)
                {
                    split_obj(obj, number - cnt);
                }

                cnt += obj->count;

                if (obj->owner)
                {
                    free_string(obj->owner);
                    obj->owner = NULL;
                }

                obj_from_char(obj);
                act("$n drops $o.", ch, obj, NULL, TO_ROOM);
                act("You drop $o.", ch, obj, NULL, TO_CHAR);
                obj = obj_to_room(obj, ch->in_room);

                if (IS_OBJ_STAT(obj, ITEM_MELT_DROP))
                {
                    act("$o dissolves into smoke.", ch, obj, NULL, TO_ROOM);
                    act("$o dissolves into smoke.", ch, obj, NULL, TO_CHAR);
                    separate_obj(obj);
                    extract_obj(obj);
                }

                if (number && cnt >= number)
                {
                    break;
                }
            }
        }

        if (!found)
        {
            if (arg[3] == '\0')
            {
                act("You are not carrying anything.", ch, NULL, arg, TO_CHAR);
            }
            else
            {
                act("You are not carrying any $T.", ch, NULL, chk, TO_CHAR);
            }
        }
    }

    return;
} // end do_drop

/*
 * Gives an item or items from one character to another.
 */
void do_give(CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA  *obj;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0' || arg2[0] == '\0')
    {
        send_to_char("Give what to whom?\r\n", ch);
        return;
    }

    if (is_number(arg1))
    {
        /* 'give NNNN coins victim' */
        int amount;
        bool silver;

        amount = atoi(arg1);
        if (amount <= 0
            || (str_cmp(arg2, "coins") && str_cmp(arg2, "coin") &&
                str_cmp(arg2, "gold") && str_cmp(arg2, "silver")))
        {
            send_to_char("Sorry, you can't do that.\r\n", ch);
            return;
        }

        silver = str_cmp(arg2, "gold");

        argument = one_argument(argument, arg2);
        if (arg2[0] == '\0')
        {
            send_to_char("Give what to whom?\r\n", ch);
            return;
        }

        if ((victim = get_char_room(ch, arg2)) == NULL)
        {
            send_to_char("They aren't here.\r\n", ch);
            return;
        }

        if ((!silver && ch->gold < amount) || (silver && ch->silver < amount))
        {
            send_to_char("You haven't got that much.\r\n", ch);
            return;
        }

        if (silver)
        {
            ch->silver -= amount;
            victim->silver += amount;
        }
        else
        {
            ch->gold -= amount;
            victim->gold += amount;
        }

        sprintf(buf, "$n gives you %d %s.", amount, silver ? "silver" : "gold");
        act(buf, ch, NULL, victim, TO_VICT);
        act("$n gives $N some coins.", ch, NULL, victim, TO_NOTVICT);
        sprintf(buf, "You give $N %d %s.", amount, silver ? "silver" : "gold");
        act(buf, ch, NULL, victim, TO_CHAR);

        /*
         * Bribe trigger
         */
        if (IS_NPC(victim) && HAS_TRIGGER(victim, TRIG_BRIBE))
            mp_bribe_trigger(victim, ch, silver ? amount : amount * 100);

        if (IS_NPC(victim) && IS_SET(victim->act, ACT_IS_CHANGER))
        {
            int change;

            change = (silver ? 95 * amount / 100 / 100 : 95 * amount);

            if (!silver && change > victim->silver)
                victim->silver += change;

            if (silver && change > victim->gold)
                victim->gold += change;

            if (change < 1 && can_see(victim, ch))
            {
                act("$n tells you '{WI'm sorry, you did not give me enough to change.{x'", victim, NULL, ch, TO_VICT);
                ch->reply = victim;
                sprintf(buf, "%d %s %s", amount, silver ? "silver" : "gold", ch->name);
                do_function(victim, &do_give, buf);
            }
            else if (can_see(victim, ch))
            {
                sprintf(buf, "%d %s %s", change, silver ? "gold" : "silver", ch->name);
                do_function(victim, &do_give, buf);
                if (silver)
                {
                    sprintf(buf, "%d silver %s", (95 * amount / 100 - change * 100), ch->name);
                    do_function(victim, &do_give, buf);
                }
                act("$n tells you '{WThank you, come again.{x'", victim, NULL, ch, TO_VICT);
                ch->reply = victim;
            }
        }
        return;
    }

    if ((obj = get_obj_carry(ch, arg1, ch)) == NULL)
    {
        send_to_char("You do not have that item.\r\n", ch);
        return;
    }

    if (obj->wear_loc != WEAR_NONE)
    {
        send_to_char("You must remove it first.\r\n", ch);
        return;
    }

    if ((victim = get_char_room(ch, arg2)) == NULL)
    {
        send_to_char("They aren't here.\r\n", ch);
        return;
    }

    if (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
    {
        act("$N tells you 'Sorry, you'll have to sell that.'", ch, NULL, victim, TO_CHAR);
        ch->reply = victim;
        return;
    }

    if (!can_drop_obj(ch, obj))
    {
        send_to_char("You can't let go of it.\r\n", ch);
        return;
    }

    separate_obj(obj);

    if (victim->carry_number + get_obj_number(obj) > can_carry_n(victim))
    {
        act("$N has $S hands full.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (get_carry_weight(victim) + get_obj_weight(obj) > can_carry_w(victim))
    {
        act("$N can't carry that much weight.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (!can_see_obj(victim, obj))
    {
        act("$N can't see it.", ch, NULL, victim, TO_CHAR);
        return;
    }

    obj_from_char(obj);
    obj_to_char(obj, victim);

    if (obj->owner)
    {
        free_string(obj->owner);
        obj->owner = NULL;
    }

    MOBtrigger = FALSE;
    act("$n gives $p to $N.", ch, obj, victim, TO_NOTVICT);
    act("$n gives you $p.", ch, obj, victim, TO_VICT);
    act("You give $p to $N.", ch, obj, victim, TO_CHAR);
    MOBtrigger = TRUE;

    /*
     * Give trigger
     */
    if (IS_NPC(victim) && HAS_TRIGGER(victim, TRIG_GIVE))
        mp_give_trigger(victim, ch, obj);

    return;

} // end do_give

/*
 * Fills an item with a liquid.
 */
void do_fill(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *fountain;
    bool found;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Fill what?\r\n", ch);
        return;
    }

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
        send_to_char("You do not have that item.\r\n", ch);
        return;
    }

    found = FALSE;
    for (fountain = ch->in_room->contents; fountain != NULL;
    fountain = fountain->next_content)
    {
        if (fountain->item_type == ITEM_FOUNTAIN)
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        send_to_char("There is no fountain here!\r\n", ch);
        return;
    }

    if (obj->item_type != ITEM_DRINK_CON)
    {
        send_to_char("You can't fill that.\r\n", ch);
        return;
    }

    if (obj->value[1] != 0 && obj->value[2] != fountain->value[2])
    {
        send_to_char("There is already another liquid in it.\r\n", ch);
        return;
    }

    if (obj->value[1] >= obj->value[0])
    {
        send_to_char("Your container is full.\r\n", ch);
        return;
    }

    separate_obj(obj);

    sprintf(buf, "You fill $p with %s from $P.", liquid_table[fountain->value[2]].liq_name);
    act(buf, ch, obj, fountain, TO_CHAR);
    sprintf(buf, "$n fills $p with %s from $P.", liquid_table[fountain->value[2]].liq_name);
    act(buf, ch, obj, fountain, TO_ROOM);
    obj->value[2] = fountain->value[2];
    obj->value[1] = obj->value[0];
    return;
}

void do_pour(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_STRING_LENGTH], buf[MAX_STRING_LENGTH];
    OBJ_DATA *out, *in;
    CHAR_DATA *vch = NULL;
    int amount;

    argument = one_argument(argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
        send_to_char("Pour what into what?\r\n", ch);
        return;
    }


    if ((out = get_obj_carry(ch, arg, ch)) == NULL)
    {
        send_to_char("You don't have that item.\r\n", ch);
        return;
    }

    if (out->item_type != ITEM_DRINK_CON)
    {
        send_to_char("That's not a drink container.\r\n", ch);
        return;
    }

    if (!str_cmp(argument, "out"))
    {
        separate_obj(out);

        if (out->value[1] == 0)
        {
            send_to_char("It's already empty.\r\n", ch);
            return;
        }

        out->value[1] = 0;
        out->value[3] = 0;
        sprintf(buf, "You invert $p, spilling %s all over the ground.", liquid_table[out->value[2]].liq_name);
        act(buf, ch, out, NULL, TO_CHAR);
        sprintf(buf, "$n inverts $p, spilling %s all over the ground.", liquid_table[out->value[2]].liq_name);
        act(buf, ch, out, NULL, TO_ROOM);
        return;
    }

    if ((in = get_obj_here(ch, argument)) == NULL)
    {
        vch = get_char_room(ch, argument);

        if (vch == NULL)
        {
            send_to_char("Pour into what?\r\n", ch);
            return;
        }

        in = get_eq_char(vch, WEAR_HOLD);

        if (in == NULL)
        {
            send_to_char("They aren't holding anything.", ch);
            return;
        }
    }

    separate_obj(in);

    if (in->item_type != ITEM_DRINK_CON)
    {
        send_to_char("You can only pour into other drink containers.\r\n", ch);
        return;
    }

    if (in == out)
    {
        send_to_char("You cannot change the laws of physics!\r\n", ch);
        return;
    }

    if (in->value[1] != 0 && in->value[2] != out->value[2])
    {
        send_to_char("They don't hold the same liquid.\r\n", ch);
        return;
    }

    if (out->value[1] == 0)
    {
        act("There's nothing in $p to pour.", ch, out, NULL, TO_CHAR);
        return;
    }

    if (in->value[1] >= in->value[0])
    {
        act("$p is already filled to the top.", ch, in, NULL, TO_CHAR);
        return;
    }

    amount = UMIN(out->value[1], in->value[0] - in->value[1]);

    in->value[1] += amount;
    out->value[1] -= amount;
    in->value[2] = out->value[2];

    if (vch == NULL)
    {
        sprintf(buf, "You pour %s from $p into $P.",
            liquid_table[out->value[2]].liq_name);
        act(buf, ch, out, in, TO_CHAR);
        sprintf(buf, "$n pours %s from $p into $P.",
            liquid_table[out->value[2]].liq_name);
        act(buf, ch, out, in, TO_ROOM);
    }
    else
    {
        sprintf(buf, "You pour some %s for $N.",
            liquid_table[out->value[2]].liq_name);
        act(buf, ch, NULL, vch, TO_CHAR);
        sprintf(buf, "$n pours you some %s.",
            liquid_table[out->value[2]].liq_name);
        act(buf, ch, NULL, vch, TO_VICT);
        sprintf(buf, "$n pours some %s for $N.",
            liquid_table[out->value[2]].liq_name);
        act(buf, ch, NULL, vch, TO_NOTVICT);

    }
}

void do_drink(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int amount;
    int liquid;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        for (obj = ch->in_room->contents; obj; obj = obj->next_content)
        {
            if (obj->item_type == ITEM_FOUNTAIN)
                break;
        }

        if (obj == NULL)
        {
            send_to_char("Drink what?\r\n", ch);
            return;
        }
    }
    else
    {
        if ((obj = get_obj_here(ch, arg)) == NULL)
        {
            send_to_char("You can't find it.\r\n", ch);
            return;
        }
    }

    if (obj->count > 1 && obj->item_type != ITEM_FOUNTAIN)
    {
        separate_obj(obj);
    }

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10)
    {
        send_to_char("You fail to reach your mouth.  *Hic*\r\n", ch);
        return;
    }

    switch (obj->item_type)
    {
        default:
            send_to_char("You can't drink from that.\r\n", ch);
            return;

        case ITEM_FOUNTAIN:
            if ((liquid = obj->value[2]) < 0)
            {
                bug("Do_drink: bad liquid number %d.", liquid);
                liquid = obj->value[2] = 0;
            }
            amount = liquid_table[liquid].liq_affect[4] * 3;
            break;

        case ITEM_DRINK_CON:
            if (obj->value[1] <= 0)
            {
                send_to_char("It is already empty.\r\n", ch);
                return;
            }

            if ((liquid = obj->value[2]) < 0)
            {
                bug("Do_drink: bad liquid number %d.", liquid);
                liquid = obj->value[2] = 0;
            }

            amount = liquid_table[liquid].liq_affect[4];
            amount = UMIN(amount, obj->value[1]);
            break;
    }
    if (!IS_NPC(ch) && !IS_IMMORTAL(ch)
        && ch->pcdata->condition[COND_FULL] > 45)
    {
        send_to_char("You're too full to drink more.\r\n", ch);
        return;
    }

    act("$n drinks $T from $p.",
        ch, obj, liquid_table[liquid].liq_name, TO_ROOM);
    act("You drink $T from $p.",
        ch, obj, liquid_table[liquid].liq_name, TO_CHAR);

    gain_condition(ch, COND_DRUNK,
        amount * liquid_table[liquid].liq_affect[COND_DRUNK] / 36);
    gain_condition(ch, COND_FULL,
        amount * liquid_table[liquid].liq_affect[COND_FULL] / 4);
    gain_condition(ch, COND_THIRST,
        amount * liquid_table[liquid].liq_affect[COND_THIRST] / 10);
    gain_condition(ch, COND_HUNGER,
        amount * liquid_table[liquid].liq_affect[COND_HUNGER] / 2);

    if (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10)
        send_to_char("You feel drunk.\r\n", ch);
    if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 40)
        send_to_char("You are full.\r\n", ch);
    if (!IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] > 40)
        send_to_char("Your thirst is quenched.\r\n", ch);

    if (obj->value[3] != 0)
    {
        /* The drink was poisoned ! */
        AFFECT_DATA af;

        act("$n chokes and gags.", ch, NULL, NULL, TO_ROOM);
        send_to_char("You choke and gag.\r\n", ch);
        af.where = TO_AFFECTS;
        af.type = gsn_poison;
        af.level = number_fuzzy(amount);
        af.duration = 3 * amount;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = AFF_POISON;
        affect_join(ch, &af);
    }

    if (obj->value[0] > 0)
        obj->value[1] -= amount;

    return;
}



void do_eat(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument(argument, arg);
    if (arg[0] == '\0')
    {
        send_to_char("Eat what?\r\n", ch);
        return;
    }

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
        send_to_char("You do not have that item.\r\n", ch);
        return;
    }

    if (!IS_IMMORTAL(ch))
    {
        if (obj->item_type != ITEM_FOOD && obj->item_type != ITEM_PILL)
        {
            send_to_char("That's not edible.\r\n", ch);
            return;
        }

        if (!IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 40)
        {
            send_to_char("You are too full to eat more.\r\n", ch);
            return;
        }
    }

    separate_obj(obj);

    act("$n eats $p.", ch, obj, NULL, TO_ROOM);
    act("You eat $p.", ch, obj, NULL, TO_CHAR);

    switch (obj->item_type)
    {

        case ITEM_FOOD:
            if (!IS_NPC(ch))
            {
                int condition;

                condition = ch->pcdata->condition[COND_HUNGER];
                gain_condition(ch, COND_FULL, obj->value[0]);
                gain_condition(ch, COND_HUNGER, obj->value[1]);
                if (condition == 0 && ch->pcdata->condition[COND_HUNGER] > 0)
                    send_to_char("You are no longer hungry.\r\n", ch);
                else if (ch->pcdata->condition[COND_FULL] > 40)
                    send_to_char("You are full.\r\n", ch);
            }

            if (obj->value[3] != 0)
            {
                /* The food was poisoned! */
                AFFECT_DATA af;

                act("$n chokes and gags.", ch, 0, 0, TO_ROOM);
                send_to_char("You choke and gag.\r\n", ch);

                af.where = TO_AFFECTS;
                af.type = gsn_poison;
                af.level = number_fuzzy(obj->value[0]);
                af.duration = 2 * obj->value[0];
                af.location = APPLY_NONE;
                af.modifier = 0;
                af.bitvector = AFF_POISON;
                affect_join(ch, &af);
            }
            break;

        case ITEM_PILL:
            obj_cast_spell(obj->value[1], obj->value[0], ch, ch, NULL);
            obj_cast_spell(obj->value[2], obj->value[0], ch, ch, NULL);
            obj_cast_spell(obj->value[3], obj->value[0], ch, ch, NULL);
            break;
    }

    extract_obj(obj);
    return;
}



/*
 * Remove an object.
 */
bool remove_obj(CHAR_DATA * ch, int iWear, bool fReplace)
{
    OBJ_DATA *obj;
    OBJ_DATA *vobj;

    if ((obj = get_eq_char(ch, iWear)) == NULL)
        return TRUE;

    if (!fReplace)
        return FALSE;

    if (IS_SET(obj->extra_flags, ITEM_NOREMOVE))
    {
        act("You can't remove $p.", ch, obj, NULL, TO_CHAR);
        return FALSE;
    }

    // Things to happen when removing a weapon
    if (obj->wear_loc == WEAR_WIELD)
    {
        // If it's the primary weapon, also remove the secondary weapon if the
        // player is wearing one.
        if ((vobj = get_eq_char(ch, WEAR_SECONDARY_WIELD)) != NULL)
        {
            act("$n stops using $p.", ch, vobj, NULL, TO_ROOM);
            act("You stop using $p.", ch, vobj, NULL, TO_CHAR);
            unequip_char(ch, vobj);
        }

    }

    unequip_char(ch, obj);
    act("$n stops using $p.", ch, obj, NULL, TO_ROOM);
    act("You stop using $p.", ch, obj, NULL, TO_CHAR);
    return TRUE;
}



/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 */
void wear_obj(CHAR_DATA * ch, OBJ_DATA * obj, bool fReplace)
{
    char buf[MAX_STRING_LENGTH];

    separate_obj(obj);

    if (ch->level < obj->level)
    {
        sprintf(buf, "You must be level %d to use this object.\r\n", obj->level);
        send_to_char(buf, ch);
        act("$n tries to use $p, but is too inexperienced.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if (obj->item_type == ITEM_LIGHT)
    {
        if (!remove_obj(ch, WEAR_LIGHT, fReplace))
            return;
        act("$n lights $p and holds it.", ch, obj, NULL, TO_ROOM);
        act("You light $p and hold it.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_LIGHT);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
    {
        if (get_eq_char(ch, WEAR_FINGER_L) != NULL
            && get_eq_char(ch, WEAR_FINGER_R) != NULL
            && !remove_obj(ch, WEAR_FINGER_L, fReplace)
            && !remove_obj(ch, WEAR_FINGER_R, fReplace))
            return;

        if (get_eq_char(ch, WEAR_FINGER_L) == NULL)
        {
            act("$n wears $p on $s left finger.", ch, obj, NULL, TO_ROOM);
            act("You wear $p on your left finger.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_FINGER_L);
            return;
        }

        if (get_eq_char(ch, WEAR_FINGER_R) == NULL)
        {
            act("$n wears $p on $s right finger.", ch, obj, NULL, TO_ROOM);
            act("You wear $p on your right finger.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_FINGER_R);
            return;
        }

        bug("Wear_obj: no free finger.", 0);
        send_to_char("You already wear two rings.\r\n", ch);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_NECK))
    {
        if (get_eq_char(ch, WEAR_NECK_1) != NULL
            && get_eq_char(ch, WEAR_NECK_2) != NULL
            && !remove_obj(ch, WEAR_NECK_1, fReplace)
            && !remove_obj(ch, WEAR_NECK_2, fReplace))
            return;

        if (get_eq_char(ch, WEAR_NECK_1) == NULL)
        {
            act("$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM);
            act("You wear $p around your neck.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_NECK_1);
            return;
        }

        if (get_eq_char(ch, WEAR_NECK_2) == NULL)
        {
            act("$n wears $p around $s neck.", ch, obj, NULL, TO_ROOM);
            act("You wear $p around your neck.", ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_NECK_2);
            return;
        }

        bug("Wear_obj: no free neck.", 0);
        send_to_char("You already wear two neck items.\r\n", ch);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_BODY))
    {
        if (!remove_obj(ch, WEAR_BODY, fReplace))
            return;
        act("$n wears $p on $s torso.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your torso.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_BODY);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_HEAD))
    {
        if (!remove_obj(ch, WEAR_HEAD, fReplace))
            return;
        act("$n wears $p on $s head.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your head.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_HEAD);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_LEGS))
    {
        if (!remove_obj(ch, WEAR_LEGS, fReplace))
            return;
        act("$n wears $p on $s legs.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your legs.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_LEGS);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_FEET))
    {
        if (!remove_obj(ch, WEAR_FEET, fReplace))
            return;
        act("$n wears $p on $s feet.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your feet.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_FEET);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
    {
        if (!remove_obj(ch, WEAR_HANDS, fReplace))
            return;
        act("$n wears $p on $s hands.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your hands.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_HANDS);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_ARMS))
    {
        if (!remove_obj(ch, WEAR_ARMS, fReplace))
            return;
        act("$n wears $p on $s arms.", ch, obj, NULL, TO_ROOM);
        act("You wear $p on your arms.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_ARMS);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
    {
        if (!remove_obj(ch, WEAR_ABOUT, fReplace))
            return;
        act("$n wears $p about $s body.", ch, obj, NULL, TO_ROOM);
        act("You wear $p about your body.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_ABOUT);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
    {
        if (!remove_obj(ch, WEAR_WAIST, fReplace))
            return;
        act("$n wears $p about $s waist.", ch, obj, NULL, TO_ROOM);
        act("You wear $p about your waist.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_WAIST);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
    {
        if (get_eq_char(ch, WEAR_WRIST_L) != NULL
            && get_eq_char(ch, WEAR_WRIST_R) != NULL
            && !remove_obj(ch, WEAR_WRIST_L, fReplace)
            && !remove_obj(ch, WEAR_WRIST_R, fReplace))
            return;

        if (get_eq_char(ch, WEAR_WRIST_L) == NULL)
        {
            act("$n wears $p around $s left wrist.", ch, obj, NULL, TO_ROOM);
            act("You wear $p around your left wrist.",
                ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_WRIST_L);
            return;
        }

        if (get_eq_char(ch, WEAR_WRIST_R) == NULL)
        {
            act("$n wears $p around $s right wrist.",
                ch, obj, NULL, TO_ROOM);
            act("You wear $p around your right wrist.",
                ch, obj, NULL, TO_CHAR);
            equip_char(ch, obj, WEAR_WRIST_R);
            return;
        }

        bug("Wear_obj: no free wrist.", 0);
        send_to_char("You already wear two wrist items.\r\n", ch);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))
    {
        OBJ_DATA *weapon;

        if (!remove_obj(ch, WEAR_SHIELD, fReplace))
            return;

        weapon = get_eq_char(ch, WEAR_WIELD);
        if (weapon != NULL && ch->size < SIZE_LARGE
            && IS_WEAPON_STAT(weapon, WEAPON_TWO_HANDS))
        {
            send_to_char("Your hands are tied up with your weapon!\r\n", ch);
            return;
        }

        if (get_eq_char(ch, WEAR_SECONDARY_WIELD) != NULL)
        {
            send_to_char("Your hands are both full.\r\n", ch);
            return;
        }

        act("$n wears $p as a shield.", ch, obj, NULL, TO_ROOM);
        act("You wear $p as a shield.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_SHIELD);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WIELD))
    {
        int sn, skill;

        if (!remove_obj(ch, WEAR_WIELD, fReplace))
            return;

        if (!IS_NPC(ch)
            && get_obj_weight(obj) >
            (str_app[get_curr_stat(ch, STAT_STR)].wield * 10))
        {
            send_to_char("It is too heavy for you to wield.\r\n", ch);
            return;
        }

        if (!IS_NPC(ch) && ch->size < SIZE_LARGE
            && IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS)
            && get_eq_char(ch, WEAR_SHIELD) != NULL)
        {
            send_to_char("You need two hands free for that weapon.\r\n", ch);
            return;
        }

        act("$n wields $p.", ch, obj, NULL, TO_ROOM);
        act("You wield $p.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_WIELD);

        sn = get_weapon_sn(ch, FALSE);

        if (sn == gsn_hand_to_hand)
            return;

        skill = get_weapon_skill(ch, sn);

        if (skill >= 100)
            act("$p feels like a part of you!", ch, obj, NULL, TO_CHAR);
        else if (skill > 85)
            act("You feel quite confident with $p.", ch, obj, NULL, TO_CHAR);
        else if (skill > 70)
            act("You are skilled with $p.", ch, obj, NULL, TO_CHAR);
        else if (skill > 50)
            act("Your skill with $p is adequate.", ch, obj, NULL, TO_CHAR);
        else if (skill > 25)
            act("$p feels a little clumsy in your hands.", ch, obj, NULL,
                TO_CHAR);
        else if (skill > 1)
            act("You fumble and almost drop $p.", ch, obj, NULL, TO_CHAR);
        else
            act("You don't even know which end is up on $p.",
                ch, obj, NULL, TO_CHAR);

        return;
    }

    if (CAN_WEAR(obj, ITEM_HOLD))
    {
        if (!remove_obj(ch, WEAR_HOLD, fReplace))
            return;
        act("$n holds $p in $s hand.", ch, obj, NULL, TO_ROOM);
        act("You hold $p in your hand.", ch, obj, NULL, TO_CHAR);
        equip_char(ch, obj, WEAR_HOLD);
        return;
    }

    if (CAN_WEAR(obj, ITEM_WEAR_FLOAT))
    {
        if (!remove_obj(ch, WEAR_FLOAT, fReplace))
            return;
        act("$n releases $p to float next to $m.", ch, obj, NULL, TO_ROOM);
        act("You release $p and it floats next to you.", ch, obj, NULL,
            TO_CHAR);
        equip_char(ch, obj, WEAR_FLOAT);
        return;
    }

    if (fReplace)
        send_to_char("You can't wear, wield, or hold that.\r\n", ch);

    return;
}

/*
 * Removes all objects from a character.  This is silent and returns no
 * message to the user, the caller will be responsible for that.
 */
void remove_all_obj(CHAR_DATA *ch)
{
    remove_obj(ch, WEAR_LIGHT, TRUE);
    remove_obj(ch, WEAR_FINGER_L, TRUE);
    remove_obj(ch, WEAR_FINGER_R, TRUE);
    remove_obj(ch, WEAR_NECK_1, TRUE);
    remove_obj(ch, WEAR_NECK_2, TRUE);
    remove_obj(ch, WEAR_BODY, TRUE);
    remove_obj(ch, WEAR_HEAD, TRUE);
    remove_obj(ch, WEAR_LEGS, TRUE);
    remove_obj(ch, WEAR_FEET, TRUE);
    remove_obj(ch, WEAR_HANDS, TRUE);
    remove_obj(ch, WEAR_ARMS, TRUE);
    remove_obj(ch, WEAR_SHIELD, TRUE);
    remove_obj(ch, WEAR_ABOUT, TRUE);
    remove_obj(ch, WEAR_WAIST, TRUE);
    remove_obj(ch, WEAR_WRIST_L, TRUE);
    remove_obj(ch, WEAR_WRIST_R, TRUE);
    remove_obj(ch, WEAR_WIELD, TRUE);
    remove_obj(ch, WEAR_HOLD, TRUE);
    remove_obj(ch, WEAR_FLOAT, TRUE);
    remove_obj(ch, WEAR_SECONDARY_WIELD, TRUE);
}

void do_wear(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Wear, wield, or hold what?\r\n", ch);
        return;
    }

    if (!str_cmp(arg, "all"))
    {
        OBJ_DATA *obj_next;

        for (obj = ch->carrying; obj != NULL; obj = obj_next)
        {
            obj_next = obj->next_content;
            if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj))
                wear_obj(ch, obj, FALSE);
        }
        return;
    }
    else
    {
        if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
        {
            send_to_char("You do not have that item.\r\n", ch);
            return;
        }

        wear_obj(ch, obj, TRUE);
    }

    return;
}

/*
 * Removes a piece of gear from the characters body.
 */
void do_remove(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Remove what?\r\n", ch);
        return;
    }

    // Do they want to remove all their gear in one fell swoop?... allow
    // only if not charmed.
    if (!str_cmp(arg, "all") && !IS_AFFECTED(ch, AFF_CHARM))
    {
        remove_all_obj(ch);
        return;
    }

    if ((obj = get_obj_wear(ch, arg)) == NULL)
    {
        send_to_char("You do not have that item.\r\n", ch);
        return;
    }

    remove_obj(ch, obj->wear_loc, TRUE);
    return;
}



void do_sacrifice(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    int silver;

    /* variables for AUTOSPLIT */
    CHAR_DATA *gch;
    int members;
    char buffer[100];


    one_argument(argument, arg);

    if (arg[0] == '\0' || !str_cmp(arg, ch->name))
    {
        act("$n offers $mself to the gods, who graciously declines.", ch, NULL, NULL, TO_ROOM);
        send_to_char("The gods appreciate your offer and may accept it later.\r\n", ch);
        return;
    }

    if (ch->in_room != NULL && IS_SET(ch->in_room->room_flags, ROOM_ARENA))
    {
        send_to_char("You cannot sacrifice items in the arena.\r\n", ch);
        return;
    }

    obj = get_obj_list(ch, arg, ch->in_room->contents);

    if (obj == NULL)
    {
        send_to_char("You can't find it.\r\n", ch);
        return;
    }

    if (obj->item_type == ITEM_CORPSE_PC)
    {
        if (obj->contains)
        {
            send_to_char("The gods wouldn't like that.\r\n", ch);
            return;
        }
    }


    if (!CAN_WEAR(obj, ITEM_TAKE) || CAN_WEAR(obj, ITEM_NO_SAC))
    {
        act("$p is not an acceptable sacrifice.", ch, obj, 0, TO_CHAR);
        return;
    }

    if (obj->in_room != NULL)
    {
        for (gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room)
        {
            if (gch->on == obj)
            {
                act("$N appears to be using $p.", ch, obj, gch, TO_CHAR);
                return;
            }
        }
    }

    silver = UMAX(1, obj->level * 3);

    if (obj->item_type != ITEM_CORPSE_NPC && obj->item_type != ITEM_CORPSE_PC)
        silver = UMIN(silver, obj->cost);

    if (silver == 1)
    {
        send_to_char("The gods give you one silver coin for your sacrifice.\r\n", ch);
    }
    else
    {
        sprintf(buf, "The gods give you %d silver coins for your sacrifice.\r\n", silver);
        send_to_char(buf, ch);
    }

    ch->silver += silver;

    if (IS_SET(ch->act, PLR_AUTOSPLIT))
    {
        /* AUTOSPLIT code */
        members = 0;
        for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
        {
            if (is_same_group(gch, ch))
                members++;
        }

        if (members > 1 && silver > 1)
        {
            sprintf(buffer, "%d", silver);
            do_function(ch, &do_split, buffer);
        }
    }

    act("$n sacrifices $p to the gods.", ch, obj, NULL, TO_ROOM);
    wiznet("$N sends up $p as a burnt offering.", ch, obj, WIZ_SACCING, 0, 0);
    separate_obj(obj);
    extract_obj(obj);
    return;
}



void do_quaff(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Quaff what?\r\n", ch);
        return;
    }

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
        send_to_char("You do not have that potion.\r\n", ch);
        return;
    }

    if (obj->item_type != ITEM_POTION)
    {
        send_to_char("You can quaff only potions.\r\n", ch);
        return;
    }

    if (ch->level < obj->level)
    {
        send_to_char("This liquid is too powerful for you to drink.\r\n",
            ch);
        return;
    }

    act("$n quaffs $p.", ch, obj, NULL, TO_ROOM);
    act("You quaff $p.", ch, obj, NULL, TO_CHAR);

    obj_cast_spell(obj->value[1], obj->value[0], ch, ch, NULL);
    obj_cast_spell(obj->value[2], obj->value[0], ch, ch, NULL);
    obj_cast_spell(obj->value[3], obj->value[0], ch, ch, NULL);

    separate_obj(obj);
    extract_obj(obj);
    return;
}



void do_recite(CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *scroll;
    OBJ_DATA *obj;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if ((scroll = get_obj_carry(ch, arg1, ch)) == NULL)
    {
        send_to_char("You do not have that scroll.\r\n", ch);
        return;
    }

    if (scroll->item_type != ITEM_SCROLL)
    {
        send_to_char("You can recite only scrolls.\r\n", ch);
        return;
    }

    if (ch->level < scroll->level)
    {
        send_to_char("This scroll is too complex for you to comprehend.\r\n", ch);
        return;
    }

    obj = NULL;
    if (arg2[0] == '\0')
    {
        victim = ch;
    }
    else
    {
        if ((victim = get_char_room(ch, arg2)) == NULL
            && (obj = get_obj_here(ch, arg2)) == NULL)
        {
            send_to_char("You can't find it.\r\n", ch);
            return;
        }
    }

    act("$n recites $p.", ch, scroll, NULL, TO_ROOM);
    act("You recite $p.", ch, scroll, NULL, TO_CHAR);

    if (number_percent() >= 20 + get_skill(ch, gsn_scrolls) * 4 / 5)
    {
        send_to_char("You mispronounce a syllable.\r\n", ch);
        check_improve(ch, gsn_scrolls, FALSE, 2);
    }

    else
    {
        obj_cast_spell(scroll->value[1], scroll->value[0], ch, victim, obj);
        obj_cast_spell(scroll->value[2], scroll->value[0], ch, victim, obj);
        obj_cast_spell(scroll->value[3], scroll->value[0], ch, victim, obj);
        check_improve(ch, gsn_scrolls, TRUE, 2);
    }

    separate_obj(scroll);
    extract_obj(scroll);
    return;
}

void do_brandish(CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    OBJ_DATA *staff;
    int sn;

    if ((staff = get_eq_char(ch, WEAR_HOLD)) == NULL)
    {
        send_to_char("You hold nothing in your hand.\r\n", ch);
        return;
    }

    if (staff->item_type != ITEM_STAFF)
    {
        send_to_char("You can brandish only with a staff.\r\n", ch);
        return;
    }

    if ((sn = staff->value[3]) < 0
        || sn >= top_sn || skill_table[sn]->spell_fun == 0)
    {
        bug("Do_brandish: bad sn %d.", sn);
        return;
    }

    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

    if (staff->value[2] > 0)
    {
        act("$n brandishes $p.", ch, staff, NULL, TO_ROOM);
        act("You brandish $p.", ch, staff, NULL, TO_CHAR);
        if (ch->level < staff->level
            || number_percent() >= 20 + get_skill(ch, gsn_staves) * 4 / 5)
        {
            act("You fail to invoke $p.", ch, staff, NULL, TO_CHAR);
            act("...and nothing happens.", ch, NULL, NULL, TO_ROOM);
            check_improve(ch, gsn_staves, FALSE, 2);
        }

        else
            for (vch = ch->in_room->people; vch; vch = vch_next)
            {
                vch_next = vch->next_in_room;

                switch (skill_table[sn]->target)
                {
                    default:
                        bug("Do_brandish: bad target for sn %d.", sn);
                        return;

                    case TAR_IGNORE:
                        if (vch != ch)
                            continue;
                        break;

                    case TAR_CHAR_OFFENSIVE:
                        if (IS_NPC(ch) ? IS_NPC(vch) : !IS_NPC(vch))
                            continue;
                        break;

                    case TAR_CHAR_DEFENSIVE:
                        if (IS_NPC(ch) ? !IS_NPC(vch) : IS_NPC(vch))
                            continue;
                        break;

                    case TAR_CHAR_SELF:
                        if (vch != ch)
                            continue;
                        break;
                }

                obj_cast_spell(staff->value[3], staff->value[0], ch, vch,
                    NULL);
                check_improve(ch, gsn_staves, TRUE, 2);
            }
    }

    if (--staff->value[2] <= 0)
    {
        act("$n's $p blazes bright and is gone.", ch, staff, NULL, TO_ROOM);
        act("Your $p blazes bright and is gone.", ch, staff, NULL, TO_CHAR);

        separate_obj(staff);
        extract_obj(staff);
    }

    return;
}



void do_zap(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *wand;
    OBJ_DATA *obj;

    one_argument(argument, arg);
    if (arg[0] == '\0' && ch->fighting == NULL)
    {
        send_to_char("Zap whom or what?\r\n", ch);
        return;
    }

    if ((wand = get_eq_char(ch, WEAR_HOLD)) == NULL)
    {
        send_to_char("You hold nothing in your hand.\r\n", ch);
        return;
    }

    if (wand->item_type != ITEM_WAND)
    {
        send_to_char("You can zap only with a wand you are holding.\r\n", ch);
        return;
    }

    obj = NULL;
    if (arg[0] == '\0')
    {
        if (ch->fighting != NULL)
        {
            victim = ch->fighting;
        }
        else
        {
            send_to_char("Zap whom or what?\r\n", ch);
            return;
        }
    }
    else
    {
        if ((victim = get_char_room(ch, arg)) == NULL
            && (obj = get_obj_here(ch, arg)) == NULL)
        {
            send_to_char("You can't find it.\r\n", ch);
            return;
        }
    }

    WAIT_STATE(ch, 2 * PULSE_VIOLENCE);

    if (wand->value[2] > 0)
    {
        if (victim != NULL)
        {
            act("$n zaps $N with $p.", ch, wand, victim, TO_NOTVICT);
            act("You zap $N with $p.", ch, wand, victim, TO_CHAR);
            act("$n zaps you with $p.", ch, wand, victim, TO_VICT);
        }
        else
        {
            act("$n zaps $P with $p.", ch, wand, obj, TO_ROOM);
            act("You zap $P with $p.", ch, wand, obj, TO_CHAR);
        }

        if (ch->level < wand->level
            || number_percent() >= 20 + get_skill(ch, gsn_wands) * 4 / 5)
        {
            act("Your efforts with $p produce only smoke and sparks.",
                ch, wand, NULL, TO_CHAR);
            act("$n's efforts with $p produce only smoke and sparks.",
                ch, wand, NULL, TO_ROOM);
            check_improve(ch, gsn_wands, FALSE, 2);
        }
        else
        {
            obj_cast_spell(wand->value[3], wand->value[0], ch, victim, obj);
            check_improve(ch, gsn_wands, TRUE, 2);
        }
    }

    if (--wand->value[2] <= 0)
    {
        act("$n's $p explodes into fragments.", ch, wand, NULL, TO_ROOM);
        act("Your $p explodes into fragments.", ch, wand, NULL, TO_CHAR);
        separate_obj(wand);
        extract_obj(wand);
    }

    return;
}

/*
 * Shopping commands.
 */
CHAR_DATA *find_keeper(CHAR_DATA * ch)
{
    /*char buf[MAX_STRING_LENGTH]; */
    CHAR_DATA *keeper;
    SHOP_DATA *pShop;

    pShop = NULL;
    for (keeper = ch->in_room->people; keeper; keeper = keeper->next_in_room)
    {
        if (IS_NPC(keeper) && (pShop = keeper->pIndexData->pShop) != NULL)
            break;
    }

    if (pShop == NULL)
    {
        send_to_char("You can't do that here.\r\n", ch);
        return NULL;
    }

    /*
     * Undesirables.
     *
     if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_KILLER) )
     {
     do_function(keeper, &do_say, "Killers are not welcome!");
     sprintf(buf, "%s the KILLER is over here!\r\n", ch->name);
     do_function(keeper, &do_yell, buf );
     return NULL;
     }

     if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_THIEF) )
     {
     do_function(keeper, &do_say, "Thieves are not welcome!");
     sprintf(buf, "%s the THIEF is over here!\r\n", ch->name);
     do_function(keeper, &do_yell, buf );
     return NULL;
     }
     */
    /*
     * Shop hours.
     */
    if (time_info.hour < pShop->open_hour)
    {
        do_function(keeper, &do_say, "Sorry, I am closed. Come back later.");
        return NULL;
    }

    if (time_info.hour > pShop->close_hour)
    {
        do_function(keeper, &do_say,
            "Sorry, I am closed. Come back tomorrow.");
        return NULL;
    }

    /*
     * Invisible or hidden people.
     */
    if (!can_see(keeper, ch))
    {
        do_function(keeper, &do_say, "I don't trade with folks I can't see.");
        return NULL;
    }

    return keeper;
}

/* insert an object at the right spot for the keeper */
void obj_to_keeper(OBJ_DATA * obj, CHAR_DATA * ch)
{
    OBJ_DATA *t_obj, *t_obj_next;

    /* see if any duplicates are found */
    for (t_obj = ch->carrying; t_obj != NULL; t_obj = t_obj_next)
    {
        t_obj_next = t_obj->next_content;

        if (obj->pIndexData == t_obj->pIndexData
            && !str_cmp(obj->short_descr, t_obj->short_descr))
        {
            /* if this is an unlimited item, destroy the new one */
            if (IS_OBJ_STAT(t_obj, ITEM_INVENTORY))
            {
                extract_obj(obj);
                return;
            }
            obj->cost = t_obj->cost;    /* keep it standard */
            break;
        }
    }

    if (t_obj == NULL)
    {
        obj->next_content = ch->carrying;
        ch->carrying = obj;
    }
    else
    {
        obj->next_content = t_obj->next_content;
        t_obj->next_content = obj;
    }

    obj->carried_by = ch;
    obj->in_room = NULL;
    obj->in_obj = NULL;
    ch->carry_number += get_obj_number(obj);
    ch->carry_weight += get_obj_weight(obj);
}

/* get an object from a shopkeeper's list */
OBJ_DATA *get_obj_keeper(CHAR_DATA * ch, CHAR_DATA * keeper, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument(argument, arg);
    count = 0;
    for (obj = keeper->carrying; obj != NULL; obj = obj->next_content)
    {
        if (obj->wear_loc == WEAR_NONE && can_see_obj(keeper, obj)
            && can_see_obj(ch, obj) && is_name(arg, obj->name))
        {
            if (++count == number)
                return obj;

            /* skip other objects of the same name */
            while (obj->next_content != NULL
                && obj->pIndexData == obj->next_content->pIndexData
                && !str_cmp(obj->short_descr,
                    obj->next_content->short_descr)) obj =
                obj->next_content;
        }
    }

    return NULL;
}

int get_cost(CHAR_DATA * keeper, OBJ_DATA * obj, bool fBuy)
{
    SHOP_DATA *pShop;
    int cost;

    if (obj == NULL || (pShop = keeper->pIndexData->pShop) == NULL)
        return 0;

    if (fBuy)
    {
        cost = obj->cost * pShop->profit_buy / 100;
    }
    else
    {
        OBJ_DATA *obj2;
        int itype;

        cost = 0;
        for (itype = 0; itype < MAX_TRADE; itype++)
        {
            if (obj->item_type == pShop->buy_type[itype])
            {
                cost = obj->cost * pShop->profit_sell / 100;
                break;
            }
        }

        if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT))
            for (obj2 = keeper->carrying; obj2; obj2 = obj2->next_content)
            {
                if (obj->pIndexData == obj2->pIndexData
                    && !str_cmp(obj->short_descr, obj2->short_descr))
                {
                    if (IS_OBJ_STAT(obj2, ITEM_INVENTORY))
                        cost /= 2;
                    else
                        cost = cost * 3 / 4;
                }
            }
    }

    if (obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND)
    {
        if (obj->value[1] == 0)
            cost /= 4;
        else
            cost = cost * obj->value[2] / obj->value[1];
    }

    return cost;
}

/*
 * The buy commmand, for purchasing stuffs.  There are different merchants a
 * player can buy from, a regular merchant that has goods, a pet shop merchant
 * which sells pets that can assist players and a portal merchant which will
 * sell magical portals to other parts of the world.
 */
void do_buy(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    register int cost, roll;
    extern int obj_short_item_cnt;

    if (argument[0] == '\0')
    {
        send_to_char("Buy what?\r\n", ch);
        return;
    }

    // Check and see if it's a portal merchant, if not, continue on with
    // the normal list command.
    if (find_mob_by_act(ch, ACT_IS_PORTAL_MERCHANT) != NULL)
    {
        // Make the call to act_mob, just pass this call down the line.
        process_portal_merchant(ch, argument);
        return;
    }

    // Check to see if a quest master in the room, if so, send off the buy
    // parameter from here and send it there.
    if (find_quest_master(ch) != NULL)
    {
        sprintf(buf, "buy %s", argument);
        do_pquest(ch, buf);
        return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP))
    {
        char arg[MAX_INPUT_LENGTH];
        char buf[MAX_STRING_LENGTH];
        CHAR_DATA *pet;
        ROOM_INDEX_DATA *pRoomIndexNext;
        ROOM_INDEX_DATA *in_room;

        smash_tilde(argument);

        if (IS_NPC(ch))
            return;

        argument = one_argument(argument, arg);

        /* hack to make new thalos pets work */
        if (ch->in_room->vnum == 9621)
        {
            pRoomIndexNext = get_room_index(9706);
        }
        else
        {
            pRoomIndexNext = get_room_index(ch->in_room->vnum + 1);
        }

        if (pRoomIndexNext == NULL)
        {
            bug("Do_buy: bad pet shop at vnum %d.", ch->in_room->vnum);
            send_to_char("Sorry, you can't buy that here.\r\n", ch);
            return;
        }

        in_room = ch->in_room;
        ch->in_room = pRoomIndexNext;
        pet = get_char_room(ch, arg);
        ch->in_room = in_room;

        if (pet == NULL || !IS_SET(pet->act, ACT_PET))
        {
            send_to_char("Sorry, you can't buy that here.\r\n", ch);
            return;
        }

        if (ch->pet != NULL)
        {
            send_to_char("You already own a pet.\r\n", ch);
            return;
        }

        cost = 10 * pet->level * pet->level;

        if ((ch->silver + 100 * ch->gold) < cost)
        {
            send_to_char("You can't afford it.\r\n", ch);
            return;
        }

        if (ch->level < pet->level)
        {
            send_to_char("You're not powerful enough to master this pet.\r\n", ch);
            return;
        }

        /* haggle */
        roll = number_percent();
        if (roll < get_skill(ch, gsn_haggle))
        {
            cost -= cost / 2 * roll / 100;
            sprintf(buf, "You haggle the price down to %d coins.\r\n", cost);
            send_to_char(buf, ch);
            check_improve(ch, gsn_haggle, TRUE, 4);
        }

        deduct_cost(ch, cost);
        pet = create_mobile(pet->pIndexData);
        SET_BIT(pet->act, ACT_PET);
        SET_BIT(pet->affected_by, AFF_CHARM);
        pet->comm = COMM_NOTELL | COMM_NOSHOUT | COMM_NOCHANNELS;

        argument = one_argument(argument, arg);
        if (arg[0] != '\0')
        {
            sprintf(buf, "%s %s", pet->name, arg);
            free_string(pet->name);
            pet->name = str_dup(buf);
        }

        sprintf(buf, "%sA neck tag says 'I belong to %s'.\r\n", pet->description, ch->name);
        free_string(pet->description);
        pet->description = str_dup(buf);

        char_to_room(pet, ch->in_room);
        add_follower(pet, ch);
        pet->leader = ch;
        ch->pet = pet;
        send_to_char("Enjoy your pet.\r\n", ch);
        act("$n bought $N as a pet.", ch, NULL, pet, TO_ROOM);
        return;
    }
    else
    {
        CHAR_DATA *keeper;
        OBJ_DATA *obj, *t_obj;
        char arg[MAX_INPUT_LENGTH];
        register int number = 1, count = 1;

        if ((keeper = find_keeper(ch)) == NULL)
            return;

        number = mult_argument(argument, arg);
        obj = get_obj_keeper(ch, keeper, arg);
        cost = get_cost(keeper, obj, TRUE);

        if (number < 1 || number > 99)
        {
            act("$n tells you 'Get real!", keeper, NULL, ch, TO_VICT);
            return;
        }

        if (cost <= 0 || !can_see_obj(ch, obj))
        {
            act("$n tells you 'I don't sell that -- try 'list''.", keeper, NULL, ch, TO_VICT);
            ch->reply = keeper;
            return;
        }

        if (!IS_OBJ_STAT(obj, ITEM_INVENTORY))
        {
            for (t_obj = obj->next_content; count < number && t_obj != NULL; t_obj = t_obj->next_content)
            {
                if (t_obj->pIndexData == obj->pIndexData
                    && !str_cmp(t_obj->short_descr, obj->short_descr))
                {
                    count++;
                }
                else
                {
                    break;
                }
            }

            if (count < number)
            {
                interpret(keeper, "laugh");
                act("$n tells you 'I don't have enough of those in stock", keeper, NULL, ch, TO_VICT);
                ch->reply = keeper;
                return;
            }
        }

        if ((ch->silver + ch->gold * 100) < cost * number)
        {
            if (number > 1)
            {
                act("$n tells you 'You can't afford to buy that many.", keeper, obj, ch, TO_VICT);
            }
            else
            {
                act("$n tells you 'You can't afford to buy $p'.", keeper, obj, ch, TO_VICT);
            }

            ch->reply = keeper;
            return;
        }

        if (obj->level > ch->level)
        {
            act("$n tells you 'You can't use $p yet'.", keeper, obj, ch, TO_VICT);
            ch->reply = keeper;
            return;
        }

        if (ch->carry_number + number * get_obj_number(obj) > can_carry_n(ch))
        {
            send_to_char("You can't carry that many items.\r\n", ch);
            return;
        }

        if (ch->carry_weight + number * get_obj_weight(obj) > can_carry_w(ch))
        {
            send_to_char("You can't carry that much weight.\r\n", ch);
            return;
        }

        /* haggle */
        roll = number_percent();

        if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT) && roll < get_skill(ch, gsn_haggle))
        {
            cost -= obj->cost / 2 * roll / 100;
            cost = URANGE(obj->cost / 10, cost, obj->cost * 10); // so they can't haggle to negatives
            act("You haggle with $N.", ch, NULL, keeper, TO_CHAR);
            check_improve(ch, gsn_haggle, TRUE, 4);
        }

        obj_short_item_cnt = number;

        if (number > 1)
        {
            act("$n buys $o.", ch, obj, NULL, TO_ROOM);
            sprintf(buf, "You buy $o for %d silver.", cost * number);
            act(buf, ch, obj, NULL, TO_CHAR);
        }
        else
        {
            act("$n buys $p.", ch, obj, NULL, TO_ROOM);
            sprintf(buf, "You buy $p for %d silver.", cost);
            act(buf, ch, obj, NULL, TO_CHAR);
        }

        obj_short_item_cnt = 1;
        deduct_cost(ch, cost * number);
        keeper->gold += cost * number / 100;
        keeper->silver += cost * number - (cost * number / 100) * 100;

        if (IS_SET(obj->extra_flags, ITEM_INVENTORY))
        {
            t_obj = create_object(obj->pIndexData);
            t_obj->count = number;

            if (t_obj->timer > 0 && !IS_OBJ_STAT(t_obj, ITEM_HAD_TIMER))
            {
                t_obj->timer = 0;
            }

            REMOVE_BIT(t_obj->extra_flags, ITEM_HAD_TIMER);

            obj_to_char(t_obj, ch);

            if (cost < t_obj->cost)
            {
                t_obj->cost = cost;
            }
        }
        else
        {

            for (count = 0; count < number; count++)
            {
                t_obj = obj;
                obj = obj->next_content;
                obj_from_char(t_obj);

                if (t_obj->timer > 0 && !IS_OBJ_STAT(t_obj, ITEM_HAD_TIMER))
                {
                    t_obj->timer = 0;
                }

                REMOVE_BIT(t_obj->extra_flags, ITEM_HAD_TIMER);
                obj_to_char(t_obj, ch);

                if (cost < t_obj->cost)
                {
                    t_obj->cost = cost;
                }
            }
        }
    }
} // end do_buy

/*
 * Lists items in the given shop.  You won't be able to have two types of shop merchants
 * in the same room.  There are normal shops, pet shops and portal shops.
 */
void do_list(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    // Check and see if it's a portal merchant, if not, continue on with
    // the normal list command.
    if (find_mob_by_act(ch, ACT_IS_PORTAL_MERCHANT) != NULL)
    {
        // Make the call to act_mob.
        process_portal_merchant(ch, "");
        return;
    }

    if (find_mob_by_act(ch, ACT_SCRIBE) != NULL)
    {
        // Make the call to act_mob.
        do_duplicate(ch, "");
        return;
    }

    if (find_mob_by_act(ch, ACT_BANKER) != NULL)
    {
        do_bank(ch, "");
        return;
    }

    // Check to see if a quest master in the room.
    if (find_quest_master(ch) != NULL)
    {
        do_pquest(ch, "list");
        return;
    }

    if (find_mob_by_act(ch, ACT_IS_HEALER) != NULL)
    {
        do_heal(ch, "");
        return;
    }

    if (IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP))
    {
        ROOM_INDEX_DATA *pRoomIndexNext;
        CHAR_DATA *pet;
        bool found;

        /* hack to make new thalos pets work */
        if (ch->in_room->vnum == 9621)
            pRoomIndexNext = get_room_index(9706);
        else
            pRoomIndexNext = get_room_index(ch->in_room->vnum + 1);

        if (pRoomIndexNext == NULL)
        {
            bug("Do_list: bad pet shop at vnum %d.", ch->in_room->vnum);
            send_to_char("You can't do that here.\r\n", ch);
            return;
        }

        found = FALSE;
        for (pet = pRoomIndexNext->people; pet; pet = pet->next_in_room)
        {
            if (IS_SET(pet->act, ACT_PET))
            {
                if (!found)
                {
                    found = TRUE;
                    send_to_char("Pets for sale:\r\n", ch);
                }
                sprintf(buf, "[%2d] %8d - %s\r\n",
                    pet->level,
                    10 * pet->level * pet->level, pet->short_descr);
                send_to_char(buf, ch);
            }
        }

        if (!found)
        {
            send_to_char("Sorry, we're out of pets right now.\r\n", ch);
        }

        return;
    }
    else
    {
        CHAR_DATA *keeper;
        OBJ_DATA *obj;
        int cost, count;
        bool found;
        char arg[MAX_INPUT_LENGTH];

        if ((keeper = find_keeper(ch)) == NULL)
            return;

        one_argument(argument, arg);

        found = FALSE;
        for (obj = keeper->carrying; obj; obj = obj->next_content)
        {
            if (obj->wear_loc == WEAR_NONE && can_see_obj(ch, obj)
                && (cost = get_cost(keeper, obj, TRUE)) > 0
                && (arg[0] == '\0' || is_name(arg, obj->name)))
            {
                if (!found)
                {
                    found = TRUE;
                    send_to_char("[Lv Price Qty] Item\r\n", ch);
                }

                if (IS_OBJ_STAT(obj, ITEM_INVENTORY))
                    sprintf(buf, "[%2d %5d -- ] %s\r\n",
                        obj->level, cost, obj->short_descr);
                else
                {
                    count = 1;

                    while (obj->next_content != NULL
                        && obj->pIndexData == obj->next_content->pIndexData
                        && !str_cmp(obj->short_descr,
                            obj->next_content->short_descr))
                    {
                        obj = obj->next_content;
                        count++;
                    }
                    sprintf(buf, "[%2d %5d %2d ] %s\r\n",
                        obj->level, cost, count, obj->short_descr);
                }
                send_to_char(buf, ch);
            }
        }

        if (!found)
        {
            send_to_char("You can't buy anything here.\r\n", ch);
        }

        return;
    }
}

/*
 * Sells an item to a shop keeper.
 */
void do_sell(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost, roll;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Sell what?\r\n", ch);
        return;
    }

    if ((keeper = find_keeper(ch)) == NULL)
        return;

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
        act("$n tells you 'You don't have that item'.", keeper, NULL, ch, TO_VICT);
        ch->reply = keeper;
        return;
    }

    if (!can_drop_obj(ch, obj))
    {
        send_to_char("You can't let go of it.\r\n", ch);
        return;
    }

    if (!can_see_obj(keeper, obj))
    {
        act("$n doesn't see what you are offering.", keeper, NULL, ch, TO_VICT);
        return;
    }

    if ((cost = get_cost(keeper, obj, FALSE)) <= 0)
    {
        act("$n looks uninterested in $p.", keeper, obj, ch, TO_VICT);
        return;
    }
    if (cost > (keeper->silver + 100 * keeper->gold))
    {
        act("$n tells you 'I'm afraid I don't have enough wealth to buy $p.", keeper, obj, ch, TO_VICT);
        return;
    }

    act("$n sells $p.", ch, obj, NULL, TO_ROOM);

    /* haggle */
    roll = number_percent();
    if (!IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT) && roll < get_skill(ch, gsn_haggle))
    {
        send_to_char("You haggle with the shopkeeper.\r\n", ch);
        cost += obj->cost / 2 * roll / 100;
        cost = UMIN(cost, 95 * get_cost(keeper, obj, TRUE) / 100);
        cost = UMIN(cost, (keeper->silver + 100 * keeper->gold));
        check_improve(ch, gsn_haggle, TRUE, 4);
    }

    sprintf(buf, "You sell $p for %d silver and %d gold piece%s.", cost - (cost / 100) * 100, cost / 100, cost == 1 ? "" : "s");
    act(buf, ch, obj, NULL, TO_CHAR);

    ch->gold += cost / 100;
    ch->silver += cost - (cost / 100) * 100;
    deduct_cost(keeper, cost);

    if (keeper->gold < 0)
        keeper->gold = 0;
    if (keeper->silver < 0)
        keeper->silver = 0;

    separate_obj(obj);

    if (obj->item_type == ITEM_TRASH || IS_OBJ_STAT(obj, ITEM_SELL_EXTRACT))
    {
        extract_obj(obj);
    }
    else
    {
        obj_from_char(obj);

        if (obj->timer)
        {
            SET_BIT(obj->extra_flags, ITEM_HAD_TIMER);
        }
        else
        {
            obj->timer = number_range(50, 100);
        }

        obj_to_keeper(obj, keeper);
    }

    return;

} // end do_sell

void do_value(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;

    OBJ_DATA *obj;
    int cost;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Value what?\r\n", ch);
        return;
    }

    if ((keeper = find_keeper(ch)) == NULL)
        return;

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
        act("$n tells you 'You don't have that item'.",
            keeper, NULL, ch, TO_VICT);
        ch->reply = keeper;
        return;
    }

    if (!can_see_obj(keeper, obj))
    {
        act("$n doesn't see what you are offering.", keeper, NULL, ch,
            TO_VICT);
        return;
    }

    if (!can_drop_obj(ch, obj))
    {
        send_to_char("You can't let go of it.\r\n", ch);
        return;
    }

    if ((cost = get_cost(keeper, obj, FALSE)) <= 0)
    {
        act("$n looks uninterested in $p.", keeper, obj, ch, TO_VICT);
        return;
    }

    sprintf(buf,
        "$n tells you 'I'll give you %d silver and %d gold coins for $p'.",
        cost - (cost / 100) * 100, cost / 100);
    act(buf, keeper, obj, ch, TO_VICT);
    ch->reply = keeper;

    return;
}

/*
 * Equips a low level character with some sub issue gear.  If the player is a tester and
 * level 51 it will equip the with some test gear.
 */
void do_outfit(CHAR_DATA * ch, char *argument)
{
    OBJ_DATA *obj;
    int i, sn, vnum;
    char buf[MAX_STRING_LENGTH];

    // This section is only for testers who are level 51, this is so we can get
    // testers equipped quickly, because well, I hate finding and loading gear. ;-)
    // This is going to use stock gear in the game, if the vnums change or are
    // removed then this will *crash*.  We will set this gear to rot death in the off
    // chance that anyone tries to proliferate it.  Not as good as the real deal.
    if ((IS_TESTER(ch) && ch->level == 51) || IS_IMMORTAL(ch))
    {
        outfit(ch, WEAR_LIGHT, 9321);  // sceptre of might
        outfit(ch, WEAR_WIELD, 9401);  // sea sword
        outfit(ch, WEAR_FINGER_L, 2803);  // opal ring
        outfit(ch, WEAR_FINGER_R, 2803);  // opal ring
        outfit(ch, WEAR_NECK_1, 1396);  // shimmering cloak of many colors
        outfit(ch, WEAR_NECK_2, 1396);  // shimmering cloak of many colors
        outfit(ch, WEAR_ARMS, 9403);  // armplates of illerion
        outfit(ch, WEAR_WRIST_L, 5025);  // copper bracelet
        outfit(ch, WEAR_WRIST_R, 5025);  // copper bracelet
        outfit(ch, WEAR_HOLD, 9406);  // tentacle of a squid
        outfit(ch, WEAR_WAIST, 9304);  // titanic belt of orion
        outfit(ch, WEAR_HEAD, 9206);  // hurricane helmet
        outfit(ch, WEAR_LEGS, 656);  // golden dragonscale leggings
        outfit(ch, WEAR_BODY, 2301);  // spiked garde armor
        outfit(ch, WEAR_FEET, 642);  // platinum boots
        outfit(ch, WEAR_HANDS, 2300);  // minotaur combat gloves
        outfit(ch, WEAR_ABOUT, 4176);  // ciquala's robe

        send_to_char("You have been equipped with testing gear.\r\n", ch);
        sprintf(buf, "%s has equipped themselves with testing gear.", ch->name);
        wiznet(buf, NULL, NULL, WIZ_GENERAL, 0, 0);

        return;
    }

    if (ch->level > 5 || IS_NPC(ch))
    {
        send_to_char("You have trained beyond the point where you can outfit.\r\n", ch);
        return;
    }

    // Too many items (TODO, needs weight check also)
    if (ch->carry_number + 3 > can_carry_n(ch))
    {
        send_to_char("You are carrying too many items to be outfitted.\r\n", ch);

        // A little lag in case someone is trying to abuse this.
        WAIT_STATE(ch, PULSE_PER_SECOND * 2);

        return;
    }

    // Their below level 5, here's some basic sub issue gear.
    outfit(ch, WEAR_LIGHT, OBJ_VNUM_SCHOOL_BANNER);
    outfit(ch, WEAR_BODY, OBJ_VNUM_SCHOOL_VEST);

    // Do the weapon thing
    // We're not going to use the outfit function because this is doing
    // some extra checks to get the default weapon from the class table.
    if ((obj = get_eq_char(ch, WEAR_WIELD)) == NULL)
    {
        sn = 0;
        vnum = OBJ_VNUM_SCHOOL_SWORD;    /* just in case! */

        for (i = 0; weapon_table[i].name != NULL; i++)
        {
            if (ch->pcdata->learned[sn] <
                ch->pcdata->learned[*weapon_table[i].gsn])
            {
                sn = *weapon_table[i].gsn;
                vnum = weapon_table[i].vnum;
            }
        }

        obj = create_object(get_obj_index(vnum));
        obj_to_char(obj, ch);
        equip_char(ch, obj, WEAR_WIELD);
    }

    // If the weapon equipped isn't two handed and the character
    // isn't wearing a shield.
    if (((obj = get_eq_char(ch, WEAR_WIELD)) == NULL
        || !IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS))
        && (obj = get_eq_char(ch, WEAR_SHIELD)) == NULL)
    {
        outfit(ch, WEAR_SHIELD, OBJ_VNUM_SCHOOL_SHIELD);
    }

    send_to_char("You have been equipped by the gods.\r\n", ch);

    // A little lag in case someone is trying to abuse this (12 beats which is
    // about what a normal spell runs).
    WAIT_STATE(ch, 12);

} // end do_outfit

/*
 * Helper function for do_outfit.  This will load a vnum and equip
 * it onto the property wear position if the character isn't wearing
 * something there already.  This will set the item loaded's cost to
 * 0 and also mark it as ROT_DEATH, e.g. This shouldn't be loaded
 * for general loading and wearing.
 */
void outfit(CHAR_DATA *ch, int wear_position, int vnum)
{
    OBJ_DATA *obj;

    if ((obj = get_eq_char(ch, wear_position)) == NULL)
    {
        obj = create_object(get_obj_index(vnum));
        obj->cost = 0;
        obj_to_char(obj, ch);
        SET_BIT(obj->extra_flags, ITEM_ROT_DEATH);
        equip_char(ch, obj, wear_position);
    }

} // end void outfit

/*
 * Dual wield - This snippit is a modified version of Erwin Andreasen's dual wield
 * code.
 */
void do_second(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *vobj;
    OBJ_DATA *pWeapon;

    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("Wear which weapon in your off-hand?\r\n", ch);
        return;
    }

    if (ch->level < skill_table[gsn_dual_wield]->skill_level[ch->class] ||
        get_skill(ch, gsn_dual_wield) == 0)
    {
        send_to_char("You are not capable of dual wielding weapons.\r\n", ch);
        return;
    }

    obj = get_obj_carry(ch, arg, ch); /* find the obj withing ch's inventory */

    if (obj == NULL)
    {
        send_to_char("You are not carrying that.\r\n", ch);
        return;
    }

    if (obj->item_type != ITEM_WEAPON)
    {
        send_to_char("That is not a weapon.\r\n", ch);
        return;
    }

    /* check if the char is using a shield or a held weapon */
    if (get_eq_char(ch, WEAR_SHIELD) != NULL)
    {
        send_to_char("You cannot use a secondary weapon while using a shield.\r\n", ch);
        return;
    }

    if (ch->level < obj->level)
    {
        sprintf(buf, "You must be level %d to use this object.\r\n", obj->level);
        send_to_char(buf, ch);
        act("$n tries to use $p, but is too inexperienced.", ch, obj, NULL, TO_ROOM);
        return;
    }

    if ((pWeapon = get_eq_char(ch, WEAR_WIELD)) == NULL)
    {
        send_to_char("You're not using a primary weapon.\r\n", ch);
        return;
    }

    if (IS_WEAPON_STAT(pWeapon, WEAPON_TWO_HANDS) || IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS))
    {
        send_to_char("You cannot dual wield with two handed weapons.\r\n", ch);
        return;
    }

    // Remove the current dual wielded weapon, then equip the new one
    if ((vobj = get_eq_char(ch, WEAR_SECONDARY_WIELD)) != NULL)
    {
        unequip_char(ch, vobj);
    }

    equip_char(ch, obj, WEAR_SECONDARY_WIELD);
    act("You wield $p as a secondary weapon.", ch, obj, NULL, TO_CHAR);
    act("$n wields $p as a secondary weapon.", ch, obj, NULL, TO_ROOM);

    return;

} // end do_second

/*
 * Touch will allow a player to touch another object or player.  Most of the time, nothing
 * will happen though it can be made to interact with different objects differently.  Our
 * first case will be allowing players to touch healers bind stones (which can heal them).
 * We're only going to focus on objects until we need to touch players also (ya ya, I walked
 * into the that's what she said).
 */
void do_touch(CHAR_DATA * ch, char *argument)
{
    OBJ_DATA * obj;

    if (IS_NULLSTR(argument))
    {
        send_to_char("What would you like to touch?\r\n", ch);
        return;
    }

    // Look for an object in the room
    obj = get_obj_list(ch, argument, ch->in_room->contents);

    if (obj == NULL)
    {
        act("I see no $T here.", ch, NULL, argument, TO_CHAR);
        return;
    }

    // We're just going to use an if statement because we'll be dealing with multiple
    // fields (vnums maybe, item_types maybe, etc.).
    if (obj->pIndexData != NULL && obj->pIndexData->vnum == OBJ_VNUM_HEALERS_BIND)
    {
        // No NPC's wasting the charges
        if (IS_NPC(ch))
        {
            send_to_char("You touch the healers bind stone but nothing happens.\r\n", ch);
            act("$n touches the healer's bind stone but nothing happens.", ch, NULL, NULL, TO_ROOM);
        }

        // They're at full health, no need to waste charges
        if (ch->hit == ch->max_hit)
        {
            send_to_char("You touch the healers bind stone but nothing happens.\r\n", ch);
            act("$n touches the healer's bind stone but nothing happens.", ch, NULL, NULL, TO_ROOM);
            return;
        }

        // Add the hit/move, but don't go over max, then update the person's position.
        ch->hit = UMIN(ch->hit + obj->value[1], ch->max_hit);
        ch->move = UMIN(ch->move + obj->value[1], ch->max_move);
        update_pos(ch);

        send_to_char("The healer's bind stone glows {Bblue{x as you feel a warm feeling fill your body.\r\n", ch);
        act("The healer's bind stone glows {Bblue{x as $n touches it.", ch, NULL, NULL, TO_ROOM);

        // Make the character wait the length of a normal spell, assuming they're not an
        // immortal
        if (!IS_IMMORTAL(ch))
        {
            WAIT_STATE(ch, 12);
        }

        // Remove one charge, see if it should be extracted.
        obj->value[0] -= 1;

        if (obj->value[0] <= 0)
        {
            send_to_char("The healer's bind stone fades slowly out of existence.\r\n", ch);
            act("The healer's bind stone fades slowly out of existence.", ch, NULL, NULL, TO_ROOM);
            extract_obj(obj);
        }

        return;
    }
    else if (obj->pIndexData != NULL && obj->pIndexData->vnum == OBJ_VNUM_CAMPFIRE)
    {
        act("You touch $p... {ROUCH{x.. that fire sure was hot!", ch, obj, NULL, TO_CHAR);
        act("$n touches $p scorching $s hand.", ch, obj, NULL, TO_ROOM);

        if (ch->race != MINOTAUR_RACE)
        {
            // Non furry people
            ch->hit -= number_range(5, 10);
        }
        else
        {
            // Minotaur's are more vulnerable to fire.
            ch->hit -= number_range(30, 50);
        }

        if (IS_IMMORTAL(ch) && ch->hit < 1)
        {
            ch->hit = 1;
        }

        update_pos(ch);
    }
    else
    {
        send_to_char("Nothing happens.\r\n", ch);
        return;
    }

} // end do_touch

/*
 * Command to bury an item.  This comes to us via the Smaug code base
 */
void do_bury(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    bool shovel;
    int move;
    one_argument(argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char("What do you wish to bury?\r\n", ch);
        return;
    }

    if (ch->fighting != NULL)
    {
        send_to_char("Not while you are fighting!!!\r\n", ch);
        return;
    }

    shovel = has_item_type(ch, ITEM_SHOVEL);

    obj = get_obj_list(ch, arg, ch->in_room->contents);

    if (!obj)
    {
        send_to_char("You can't find it.\r\n", ch);
        return;
    }

    if (!can_loot(ch, obj))
    {
        act("You cannot bury $p.", ch, obj, 0, TO_CHAR);
        return;
    }

    switch (ch->in_room->sector_type)
    {
        case SECT_CITY:
        case SECT_INSIDE:
            send_to_char("The floor is too hard to dig through.\r\n", ch);
            return;
        case SECT_WATER_SWIM:
        case SECT_WATER_NOSWIM:
        case SECT_UNDERWATER:
            send_to_char("You cannot bury something here.\r\n", ch);
            return;
        case SECT_AIR:
            send_to_char("What?  In the air?!\r\n", ch);
            return;
    }

    if (obj->weight > (UMAX(5, (can_carry_w(ch) / 10))) && !shovel)
    {
        send_to_char("You'd need a shovel to bury something that big.\r\n", ch);
        return;
    }

    move = (obj->weight * 50 * (shovel ? 1 : 5)) / UMAX(1, can_carry_w(ch));
    move = URANGE(2, move, 1000);

    if (move > ch->move)
    {
        send_to_char("You don't have the energy to bury something of that size.\r\n", ch);
        return;
    }

    ch->move -= move;
    act("You bury $p...", ch, obj, NULL, TO_CHAR);
    act("$n buries $p...", ch, obj, NULL, TO_ROOM);
    separate_obj(obj);
    SET_BIT(obj->extra_flags, ITEM_BURIED);
    WAIT_STATE(ch, URANGE(10, move / 2, 100));

    return;

} // end do_bury

/*
 * Command to dig in a room to look for buried items.  A shovel helps.  This
 * comes to us via the Smaug code base.
 */
void do_dig(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;
    OBJ_DATA *startobj;
    bool found;
    bool shovel;

    switch (ch->substate)
    {
        default:
            if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM))
            {
                send_to_char("You can't concentrate enough for that.\r\n", ch);
                return;
            }

            switch (ch->in_room->sector_type)
            {
                case SECT_CITY:
                case SECT_INSIDE:
                    send_to_char("The floor is too hard to dig through.\r\n", ch);
                    return;
                case SECT_WATER_SWIM:
                case SECT_WATER_NOSWIM:
                case SECT_UNDERWATER:
                    send_to_char("You cannot dig here.\r\n", ch);
                    return;
                case SECT_AIR:
                    send_to_char("What?  In the air?!\r\n", ch);
                    return;
            }

            // Having a shovel speeds up digging
            shovel = has_item_type(ch, ITEM_SHOVEL);

            if (shovel)
            {
                add_timer(ch, TIMER_DO_FUN, 12, do_dig, 1, NULL);
                send_to_char("You begin digging with a shovel...\r\n", ch);
                act("$n begins digging with a shovel...", ch, NULL, NULL, TO_ROOM);
            }
            else
            {
                add_timer(ch, TIMER_DO_FUN, 36, do_dig, 1, NULL);
                send_to_char("You begin digging with your hands...\r\n", ch);
                act("$n begins digging with their hands...", ch, NULL, NULL, TO_ROOM);
            }

            return;
        case 1:
            // Continue onward with said digging.
            break;
        case SUB_TIMER_DO_ABORT:
            ch->substate = SUB_NONE;
            send_to_char("You stop digging...\r\n", ch);
            act("$n stops digging...", ch, NULL, NULL, TO_ROOM);
            return;
    }

    ch->substate = SUB_NONE;

    startobj = ch->in_room->contents;
    found = FALSE;

    for (obj = startobj; obj; obj = obj->next_content)
    {
        if (IS_OBJ_STAT(obj, ITEM_BURIED))
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        send_to_char("Your dig uncovered nothing.\r\n", ch);
        act("$n's dig uncovered nothing.", ch, NULL, NULL, TO_ROOM);
        return;
    }

    separate_obj(obj);
    REMOVE_BIT(obj->extra_flags, ITEM_BURIED);
    act("Your dig uncovered $p!", ch, obj, NULL, TO_CHAR);
    act("$n's dig uncovered $p!", ch, obj, NULL, TO_ROOM);

    return;

} // end do_dig

/*
 * Flips a coin in the air and determines head's or tails.  The character must have
 * a silver or gold coin on them.  This will take it out of their worth and create
 * a single coins that will land on the ground.
 */
void do_flipcoin(CHAR_DATA *ch, char *argument)
{
    bool gold = FALSE;
    bool silver = FALSE;

    // Get whether they have gold or silver, the if statement below will be setup
    // to use silver before gold if the player has both since silver is cheaper.
    if (ch->silver > 0)
        silver = TRUE;

    if (ch->gold > 0)
        gold = TRUE;

    switch (ch->substate)
    {
        default:
            if (!silver && !gold)
            {
                send_to_char("You don't have a single gold or silver coin to use.\r\n", ch);
                return;
            }

            if (silver)
            {
                act("You take a silver coin out and flip it high into the air.", ch, NULL, NULL, TO_CHAR);
                act("$n takes a silver coin out and flip it high into the air.", ch, NULL, NULL, TO_ROOM);
            }
            else if (gold)
            {
                act("You take a gold coin out and flip it high into the air.", ch, NULL, NULL, TO_CHAR);
                act("$n takes a gold coin out and flip it high into the air.", ch, NULL, NULL, TO_ROOM);
            }

            add_timer(ch, TIMER_DO_FUN, 6, do_flipcoin, 1, NULL);
            return;

        case 1:
        case SUB_TIMER_DO_ABORT:
            if (number_range(0, 1) == 0)
            {
                // Heads
                if (silver)
                {
                    act("The silver coin falls to the ground heads side up!", ch, NULL, NULL, TO_ALL);
                }
                else if (gold)
                {
                    act("The gold coin falls to the ground heads side up!", ch, NULL, NULL, TO_ALL);
                }
            }
            else
            {
                // Tails
                if (silver)
                {
                    act("The silver coin falls to the ground tails side up!", ch, NULL, NULL, TO_ALL);
                }
                else if (gold)
                {
                    act("The gold coin falls to the ground tails side up!", ch, NULL, NULL, TO_ALL);
                }
            }

            // Start with silver, it's cheaper.
            if (silver)
            {
                ch->silver -= 1;
                obj_to_room(create_money(0, 1), ch->in_room);
            }
            else if (gold)
            {
                ch->gold -= 1;
                obj_to_room(create_money(1, 0), ch->in_room);
            }

    }

    ch->substate = SUB_NONE;
    return;

} // end flipcoin

/*
 * Use command to simplfy the simplify the amount of commands needed to interact
 * with items.  We won't remove the other commands, but they can be called from
 * here (e.g. brandish, zap, eat, look in some cases, etc.).  We're basically going
 * to pass the input down the line.  I always forget what I'm supposed to brandish
 * anyway and try to zap first. :p  Since use interacts with an object, it maybe
 * less verbose to use the base command (e.g. use wand blake vs. zap blake).  We
 * will offer both.
 */
void do_use(CHAR_DATA * ch, char *argument)
{
    OBJ_DATA *obj;
    char arg1[MAX_INPUT_LENGTH];
    char orig_argument[MAX_INPUT_LENGTH];

    sprintf(orig_argument, "%s", argument);

    // arg1 will be the object we want to use, we'll verify it's available
    // and then pass the argument down the line.
    argument = one_argument(argument, arg1);

    if (IS_NULLSTR(arg1))
    {
        send_to_char("Use what?\r\n", ch);
        return;
    }

    if ((obj = get_obj_here(ch, arg1)) != NULL)
    {

        // Objects that need to be handled specifically first
        if (obj->pIndexData != NULL && obj->pIndexData->vnum == OBJ_VNUM_HEALERS_BIND)
        {
            do_function(ch, &do_touch, arg1);
            return;
        }

        // Now by type
        switch (obj->item_type)
        {
            default:
                send_to_char("You're not quite sure how to use that.\r\n", ch);
                break;
            case ITEM_MONEY:
            case ITEM_GEM:
                do_function(ch, &do_help, "money");
                break;
            case ITEM_FOUNTAIN:
                do_function(ch, &do_drink, "");
                break;
            case ITEM_DRINK_CON:
                do_function(ch, &do_drink, argument);
                break;
            case ITEM_JEWELRY:
            case ITEM_WARP_STONE:
            case ITEM_CONTAINER:
            case ITEM_WEAPON:
            case ITEM_ARMOR:
            case ITEM_LIGHT:
                do_function(ch, &do_wear, arg1);
                break;
            case ITEM_SCROLL:
                do_function(ch, &do_recite, orig_argument);
                break;
            case ITEM_WAND:
                do_function(ch, &do_zap, argument);
                break;
            case ITEM_STAFF:
                do_function(ch, &do_brandish, argument);
                break;
            case ITEM_POTION:
                do_function(ch, &do_quaff, arg1);
                break;
            case ITEM_FURNITURE:
                do_function(ch, &do_rest, arg1);
                break;
            case ITEM_FOOD:
            case ITEM_PILL:
                do_function(ch, &do_eat, arg1);
                break;
            case ITEM_MAP:
                do_function(ch, &do_look, arg1);
                break;
            case ITEM_PORTAL:
                do_function(ch, &do_enter, arg1);
                break;
        }
    }
    else
    {
        send_to_char("Use what?\r\n", ch);
        return;
    }

    return;
} // end do_use

/*
 * Shows a player additional information about an item.
 */
void show_lore(CHAR_DATA * ch, OBJ_DATA *obj)
{
    // If we're working we nothing then we're doing nothing.
    if (obj == NULL || ch == NULL)
    {
        return;
    }

    // Player doesn't have the skill at all yet (or will never have it)
    if (ch->level < skill_table[gsn_lore]->skill_level[ch->class]
        || get_skill(ch, gsn_lore) == 0)
    {
        return;
    }

    if (ch->position == POS_FIGHTING)
    {
        return;
    }

    if (obj->item_type == ITEM_PARCHMENT)
    {
        return;
    }

    // Skill check
    if (CHANCE_SKILL(ch, gsn_lore))
    {
        send_to_char("\r\nYour lore has garnered you this additional information:\r\n", ch);
        check_improve(ch, gsn_lore, TRUE, 9);
    }
    else
    {
        send_to_char("\r\nYou can't seem to recall any lore of this item.\r\n", ch);
        check_improve(ch, gsn_lore, FALSE, 9);
        return;
    }

    char buf[MAX_STRING_LENGTH];

    sprintf(buf, "\r\n%s is of type '%s' and comes from '%s'.\r\nIt weighs %d, costs %d silver and is level %d.\r\n",
        capitalize(obj->short_descr),
        item_name(obj->item_type),
         obj->pIndexData->area->name,
        obj->weight / 10,
        obj->cost,
        obj->level);
    send_to_char( buf, ch );

/*
    // Uncomment after condition is fixed
    if (obj->condition == 100)
        send_to_char("The object is in perfect condition", ch);
    else if (obj->condition > 90)
        send_to_char("The object is in excellent condition", ch);
    else if (obj->condition > 75)
        send_to_char("The object is in good condition", ch);
    else if (obj->condition > 50)
        send_to_char("The object is in average condition", ch);
    else if (obj->condition > 25)
        send_to_char("The object is in below average condition", ch);
    else if (obj->condition > 5 )
        send_to_char("The object is in poor condition", ch);
    else
        send_to_char("The object is worthless condition", ch);
*/

    switch (obj->item_type)
    {
        case ITEM_LIGHT:
            if (obj->value[2] < 0)
            {
                send_to_char("This item provides infinte light.\r\n", ch);
            }
            else
            {
                sprintf(buf, "This item is a light with %d ticks of light remaining.\r\n", obj->value[2]);
                send_to_char(buf, ch);
            }
            break;
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            sprintf(buf, "Level %d spells of:", obj->value[0]);
            send_to_char(buf, ch);

            if (obj->value[1] >= 0 && obj->value[1] < top_sn)
            {
                send_to_char(" '", ch);
                send_to_char(skill_table[obj->value[1]]->name, ch);
                send_to_char( "'", ch);
            }

            if (obj->value[2] >= 0 && obj->value[2] < top_sn)
            {
                send_to_char(" '", ch);
                send_to_char(skill_table[obj->value[2]]->name, ch);
                send_to_char("'", ch);
            }

            if (obj->value[3] >= 0 && obj->value[3] < top_sn)
            {
                send_to_char(" '", ch);
                send_to_char(skill_table[obj->value[3]]->name, ch);
                send_to_char("'", ch);
            }

            if (obj->value[4] >= 0 && obj->value[4] < top_sn)
            {
                send_to_char(" '", ch);
                send_to_char(skill_table[obj->value[4]]->name, ch);
                send_to_char("'", ch);
            }

            send_to_char(".\r\n", ch);
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            sprintf(buf, "Has %d charges of level %d", obj->value[2], obj->value[0]);
            send_to_char(buf, ch);

            if (obj->value[3] >= 0 && obj->value[3] < top_sn)
            {
                send_to_char(" '", ch);
                send_to_char(skill_table[obj->value[3]]->name, ch);
                send_to_char("'", ch);
            }

            send_to_char(".\r\n", ch);
            break;
        case ITEM_CONTAINER:
            sprintf(buf, "Capacity: %d#  Maximum weight: %d#  flags: %s\r\n",
                obj->value[0], obj->value[3],
                cont_bit_name(obj->value[1]));
            send_to_char(buf, ch);
            if (obj->value[4] != 100)
            {
                sprintf(buf, "Weight multiplier: %d%%\r\n", obj->value[4]);
                send_to_char(buf, ch);
            }
            break;
        case ITEM_WEAPON:
            send_to_char("Weapon type is ", ch);
            switch (obj->value[0])
            {
                case (WEAPON_EXOTIC) :
                    send_to_char("an exotic type", ch);
                    break;
                case (WEAPON_SWORD) :
                    send_to_char("a sword", ch);
                    break;
                case (WEAPON_DAGGER) :
                    send_to_char("a dagger", ch);
                    break;
                case (WEAPON_SPEAR) :
                    send_to_char("a spear/staff", ch);
                    break;
                case (WEAPON_MACE) :
                    send_to_char("a mace/club", ch);
                    break;
                case (WEAPON_AXE) :
                    send_to_char("a axe", ch);
                    break;
                case (WEAPON_FLAIL) :
                    send_to_char("a flail", ch);
                    break;
                case (WEAPON_WHIP) :
                    send_to_char("a whip", ch);
                    break;
                case (WEAPON_POLEARM) :
                    send_to_char("a polearm", ch);
                    break;
                default:
                    send_to_char("unknown", ch);
                    break;
            }

            sprintf(buf, " and does damage of %dd%d (average %d).\r\n",
                obj->value[1], obj->value[2],
                (1 + obj->value[2]) * obj->value[1] / 2);
            send_to_char(buf, ch);

            if (obj->value[4])
            {
                sprintf(buf, "Weapons flags: %s\r\n", weapon_bit_name(obj->value[4]));
                send_to_char(buf, ch);
            }
            break;
        case ITEM_ARMOR:
            sprintf(buf, "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic.\r\n",
                obj->value[0], obj->value[1], obj->value[2],
                obj->value[3]);
            send_to_char(buf, ch);
            break;

    }

} // end show_lore

  /*
   * Empties an objects contents into another object or a room.  This is brought to us
   * by the Smaug code base.
   */
bool empty_obj(OBJ_DATA *obj, OBJ_DATA *dest_obj, ROOM_INDEX_DATA *dest_room)
{
    OBJ_DATA *temp_obj, *temp_obj_next;
    CHAR_DATA *ch = obj->carried_by;
    bool moved_some = FALSE;

    if (!obj)
    {
        bug("empty_obj: NULL obj", 0);
        return FALSE;
    }

    // Move the contents from one container to another.
    if (dest_obj || (!dest_room && !ch && (dest_obj = obj->in_obj) != NULL))
    {
        for (temp_obj = obj->contains; temp_obj; temp_obj = temp_obj_next)
        {
            temp_obj_next = temp_obj->next_content;

            // Make sure the destination object is a container and that it can hold the
            // weight.
            if (dest_obj->item_type == ITEM_CONTAINER
                && (get_obj_weight(temp_obj) + get_true_weight(dest_obj) > (dest_obj->value[0] * 10)
                || get_obj_weight(temp_obj) > (dest_obj->value[3] * 10)))
                continue;

            // Remove the object from one container, put it into the next.  No need to separate
            // because we're taking it all.
            obj_from_obj(temp_obj);
            obj_to_obj(temp_obj, dest_obj);
            moved_some = TRUE;
        }

        return moved_some;
    }

    // Move the contents from one container to the the provided room.
    if (dest_room || (!ch && (dest_room = obj->in_room) != NULL))
    {
        for (temp_obj = obj->contains; temp_obj; temp_obj = temp_obj_next)
        {
            temp_obj_next = temp_obj->next_content;
            obj_from_obj(temp_obj);
            obj_to_room(temp_obj, dest_room);
            moved_some = TRUE;
        }

        return moved_some;
    }

    // Move the contents from one container to the character who is holding the container.
    if (ch)
    {
        for (temp_obj = obj->contains; temp_obj; temp_obj = temp_obj_next)
        {
            temp_obj_next = temp_obj->next_content;
            obj_from_obj(temp_obj);
            obj_to_char(temp_obj, ch);
            moved_some = TRUE;
        }
        return moved_some;
    }

    // If we got here then we were not provided the correct inputs, log it.
    bug("empty_obj: could not determine a destination for vnum %d", obj->pIndexData->vnum);
    return FALSE;
}

/*
 * The ability to empty a container into another container, empty a liquid container
 * or dump a container into your inventory.  This code originated from a piece Smaug who
 * acquired it from Crimson Blade (Noplex, Krowe, Emberlyna, Lanthos).  It was originally
 * part of liquids.c in Smaug.
 */
void do_empty(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if ((!str_cmp(arg2, "into") || !str_cmp(arg2, "in")) && argument[0] != '\0')
    {
        argument = one_argument(argument, arg2);
    }

    if (arg1[0] == '\0')
    {
        send_to_char("Empty what?\r\n", ch);
        return;
    }

    if ((obj = get_obj_carry(ch, arg1, ch)) == NULL)
    {
        send_to_char("You aren't carrying that.\r\n", ch);
        return;
    }

    if (obj->count > 1)
    {
        separate_obj(obj);
    }

    switch (obj->item_type)
    {
        default:
            act("You shake $p in an attempt to empty it...", ch, obj, NULL, TO_CHAR);
            act("$n begins to shake $p in an attempt to empty it...", ch, obj, NULL, TO_ROOM);
            return;
        case ITEM_DRINK_CON:
            if (obj->value[1] < 1)
            {
                send_to_char("It's already empty.\r\n", ch);
                return;
            }
            act("You empty $p.", ch, obj, NULL, TO_CHAR);
            act("$n empties $p.", ch, obj, NULL, TO_ROOM);
            obj->value[1] = 0;
            return;
        case ITEM_CONTAINER:
            if (IS_SET(obj->value[1], CONT_CLOSED))
            {
                act("The $d is closed.", ch, NULL, obj->name, TO_CHAR);
                return;
            }

            if (!obj->contains)
            {
                send_to_char("It's already empty.\r\n", ch);
                return;
            }

            if (arg2[0] == '\0')
            {
                if (IS_SET (ch->in_room->room_flags, ROOM_ARENA))
                {
                    send_to_char ("No littering in the arena.\r\n", ch);
                    return;
                }

                if (empty_obj(obj, NULL, ch->in_room))
                {
                    act("You empty the contents of $p onto the ground.", ch, obj, NULL, TO_CHAR);
                    act("$n empties the contents of $p onto the ground.", ch, obj, NULL, TO_ROOM);
                }
                else
                {
                    send_to_char("Hmmm... didn't work.\r\n", ch);
                }
            }
            else
            {
                OBJ_DATA *dest = get_obj_here(ch, arg2);

                if (!str_prefix("self", arg2))
                {
                    if (empty_obj(obj, NULL, NULL))
                    {
                        act("You empty the contents of $p into your inventory.", ch, obj, NULL, TO_CHAR);
                        act("$n empties the contents of $p into $s inventory.", ch, obj, NULL, TO_ROOM);
                    }

                    return;
                }

                if (!dest)
                {
                    send_to_char("You can't find it.\r\n", ch);
                    return;
                }

                if (dest == obj)
                {
                    send_to_char("You can't empty something into itself!\r\n", ch);
                    return;
                }

                if (dest->item_type != ITEM_CONTAINER)
                {
                    send_to_char("That's not a container!\r\n", ch);
                    return;

                }
                if (IS_SET(dest->value[1], CONT_CLOSED))
                {
                    act("The $d is closed.", ch, NULL, dest->name, TO_CHAR);
                    return;
                }

                separate_obj(dest);

                if (empty_obj(obj, dest, NULL))
                {
                    act("You empty the contents of $p into $P.", ch, obj, dest, TO_CHAR);
                    act("$n empties the contents of $p into $P.", ch, obj, dest, TO_ROOM);
                }
                else
                {
                    act("$P is too full.", ch, obj, dest, TO_CHAR);
                }
            }

        return;
    }
}

/*
 * Command to allow players to store up to 10 keys in a keyring
 * Contributed by Khlydes of LorenMud.
 */
void do_keyring(CHAR_DATA * ch, char * argument)
{
    OBJ_INDEX_DATA * obj;
    OBJ_DATA * obj2;
    OBJ_DATA * object;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    int i, key[10];
    bool found = FALSE;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    for (i = 0; i < 10; i++)
    {
        key[i] = ch->pcdata->key_ring[i];
    }

    if (IS_NULLSTR(arg1))
    {
        send_to_char("Syntax: keyring show\r\n", ch);
        send_to_char("        keyring add <key>\r\n", ch);
        send_to_char("        kerying remove <1-10>\r\n", ch);
        return;
    }

    if (!str_cmp(arg1, "show"))
    {
        send_to_char("The available keys on your key ring are:\r\n", ch);

        for (i = 0; i < 10; i++)
        {
            if (key[i] != -1)
            {
                obj = get_obj_index(key[i]);

                if (obj != NULL && obj->item_type == ITEM_KEY)
                {
                    sprintf(buf, "%2d.   %s\r\n", (i + 1), obj->short_descr);
                }
                else
                {
                    sprintf(buf, "%2d.   (empty)\r\n", (i + 1));
                    ch->pcdata->key_ring[i] = -1;
                }
            }
            else
            {
                sprintf(buf, "%2d.   (empty)\r\n", (i + 1));
            }

            send_to_char(buf, ch);
        }

        return;
    }

    if (IS_NULLSTR(arg2))
    {
        send_to_char("Syntax: keyring show\r\n", ch);
        send_to_char("        keyring add <key>\r\n", ch);
        send_to_char("        kerying remove <1-10>\r\n", ch);
        return;
    }

    if (!str_cmp(arg1, "add"))
    {
        if ((object = get_obj_carry(ch, arg2, ch)) == NULL)
        {
            send_to_char("You do not have that key.\r\n", ch);
        }
        else
        {
            if (object->item_type != ITEM_KEY)
            {
                send_to_char("That is not a key.\r\n", ch);
            }
            else if (object->timer > 0)
            {
                // If the key has a timer that means it's on the clock to crumbling, putting
                // it on the key ring would create a loophole to avoid that mechansim.  Rot
                // death items are ok as long as they haven't been triggered.
                send_to_char("That key is too fragile to add to your key ring.\r\n", ch);
            }
            else
            {
                for (i = 0; i < 10; i++)
                {
                    if (key[i] == -1)
                    {
                        // Set they key onto the ring, they separate it and extract it.  Separating it
                        // will only take one of the keys from the inventory if multiple exist that have
                        // been consolidated.
                        ch->pcdata->key_ring[i] = object->pIndexData->vnum;
                        separate_obj(object);
                        extract_obj(object);
                        send_to_char("The key has been added to your key ring.\r\n", ch);
                        found = TRUE;
                        break;
                    }
                }

                if (!found)
                {
                    send_to_char("Your key ring is full, you must remove a key first.\r\n", ch);
                }
            }
        }

        return;
    }

    if (!is_number(arg2))
    {
        send_to_char("You must choose a key between 1 and 10.\r\n", ch);
        return;
    }

    int value = atoi(arg2);

    if (!str_cmp(arg1, "remove"))
    {
        if (value < 1 || value > 10)
        {
            send_to_char("You must remove a key from 1 to 10.\r\n", ch);
            return;
        }
        else
        {
            value -= 1;
            obj2 = create_object(get_obj_index(ch->pcdata->key_ring[value]));

            // This key is getting removed, unset it from the ring
            ch->pcdata->key_ring[value] = -1;

            if (obj2 != NULL && obj2->item_type == ITEM_KEY)
            {
                sprintf(buf, "You have removed %s from your key ring.\r\n", obj2->short_descr);
                obj_to_char(obj2, ch);
            }
            else
            {
                // Only extract the object if it's not null (extracting a NULL object will crash)
                if (obj2 != NULL)
                {
                    bugf("Invalid key found in do_keyring (e.g. it's not a key, find out how it got there): vnum = %d.", obj2->pIndexData->vnum);
                    extract_obj(obj2);  // No need to separate, just extract
                }

                // One way or another, the key was invalid if we get here (perhaps an area was removed?)
                sprintf(buf, "They key crumbled in your hands.\r\n");
            }

            send_to_char(buf, ch);
        }
    }

    return;
}

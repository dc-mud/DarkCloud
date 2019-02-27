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

/****************************************************************************
*  Automated Quest code written by Vassago of MOONGATE, moongate.ams.com    *
*  4000. Copyright (c) 1996 Ryan Addams, All Rights Reserved. Use of this   *
*  code is allowed provided you add a credit line to the effect of:         *
*  "Quest Code (c) 1996 Ryan Addams" to your logon screen with the rest     *
*  of the standard diku/rom credits. If you use this or a modified version  *
*  of this code, let me know via email: moongate@moongate.ams.com. Further  *
*  updates will be posted to the rom mailing list. If you'd like to get     *
*  the latest version of quest.c, please send a request to the above add-   *
*  ress. Quest Code v2.03. Please do not remove this notice from this file. *
****************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"

DECLARE_DO_FUN(do_say);
DECLARE_DO_FUN(do_direct);

/* Object vnums for object quest 'tokens'. In Moongate, the tokens are
   things like 'the Shield of Moongate', 'the Sceptre of Moongate'. These
   items are worthless and have the rot-death flag, as they are placed
   into the world when a player receives an object quest. */

#define QUEST_OBJ1 35 // Blue shard
#define QUEST_OBJ2 36 // Crown jewels

/*
 * The structure to hold information about the quest items.
 */
struct quest_item_type
{
    char *keyword;
    char *desc;
    int vnum;
    int cost;
};

extern const struct quest_item_type quest_item_table[];

/*
 * These are the list of quest items offered, add new entries here but leave
 * the last item as NULL always so the loop where it's used knows when to
 * terminate without jumping out of bounds.
 *
 * Keywords, Description, Vnum, Quest Points Cost
 */
const struct quest_item_type quest_item_table[] = {
    {"mystic dagger", "A mystic dagger", 37, 2000},
    {"sword truth", "A sword of truth", 38, 2000},
    {"sword gods", "A sword of the Gods", 32, 1500},
    {"decanter endless water", "Decanter of endless water", 33, 750},
    {"book knowledge", "A book of knowledge", 34, 700},
    {"jeweled egg", "A jeweled egg", OBJ_VNUM_EGG, 700},
    {NULL, NULL, -1, -1}
};

/* Local functions */
void generate_quest(CHAR_DATA *ch, CHAR_DATA *questman);
void quest_update(void);
bool quest_level_diff(int clevel, int mlevel);
ROOM_INDEX_DATA *find_location(CHAR_DATA *ch, char *arg);
bool is_safe_quest(CHAR_DATA *ch, CHAR_DATA *victim);

/*
 * The main quest function that a player will interact with.
 */
void do_pquest(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *questman;
    OBJ_DATA *obj = NULL, *obj_next;
    OBJ_INDEX_DATA *questinfoobj;
    MOB_INDEX_DATA *questinfo;
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    int x = 0;

    if (IS_NPC(ch))
    {
        send_to_char("NPC's may not quest.\r\n", ch);
        return;
    }

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, arg2);

    if (arg1[0] == '\0')
    {
        send_to_char("QUEST commands: POINTS INFO TIME REQUEST COMPLETE CLEAR LIST BUY.\r\n", ch);

        if (IS_IMMORTAL(ch))
        {
            send_to_char("QUEST commands for immortals: RESET.\r\n", ch);
        }

        send_to_char("For more information, type 'HELP QUEST'.\r\n", ch);
        return;
    }

    if (!strcmp(arg1, "info"))
    {
        if (IS_SET(ch->act, PLR_QUESTOR))
        {
            if (ch->pcdata->quest_mob == -1 && ch->pcdata->quest_giver != NULL && ch->pcdata->quest_giver->short_descr != NULL)
            {
                sprintf(buf, "Your quest is ALMOST complete!\r\nGet back to %sbefore your time runs out!\r\n", ch->pcdata->quest_giver->short_descr);
                send_to_char(buf, ch);
            }
            else if (ch->pcdata->quest_obj > 0)
            {
                questinfoobj = get_obj_index(ch->pcdata->quest_obj);
                if (questinfoobj != NULL)
                {
                    sprintf(buf, "You are on a quest to recover the fabled %s!\r\n", questinfoobj->name);
                    send_to_char(buf, ch);
                }
                else
                {
                    send_to_char("You aren't currently on a quest.\r\n", ch);
                }
                return;
            }
            else if (ch->pcdata->quest_mob > 0)
            {
                questinfo = get_mob_index(ch->pcdata->quest_mob);
                if (questinfo != NULL)
                {
                    sprintf(buf, "You are on a quest to slay the dreaded %s!\r\n", questinfo->short_descr);
                    send_to_char(buf, ch);
                }
                else
                {
                    send_to_char("You aren't currently on a quest.\r\n", ch);
                }
                return;
            }
        }
        else
        {
            send_to_char("You aren't currently on a quest.\r\n", ch);
        }
        return;
    }

    if (!strcmp(arg1, "points"))
    {
        sprintf(buf, "You have %d quest points.\r\n", ch->pcdata->quest_points);
        send_to_char(buf, ch);
        return;
    }
    else if (!strcmp(arg1, "reset"))
    {
        send_to_char("Your next quest time has been reset.\r\n", ch);
        ch->pcdata->next_quest = 0;
        return;
    }
    else if (!strcmp(arg1, "time"))
    {
        if (!IS_SET(ch->act, PLR_QUESTOR))
        {
            send_to_char("You aren't currently on a quest.\r\n", ch);

            if (ch->pcdata->next_quest > 1)
            {
                sprintf(buf, "There are %d ticks remaining before you can quest again.\r\n", ch->pcdata->next_quest);
                send_to_char(buf, ch);
            }
            else if (ch->pcdata->next_quest == 1)
            {
                send_to_char("There is less than a tick remaining until you can go on another quest.\r\n", ch);
            }
        }
        else if (ch->pcdata->countdown > 0)
        {
            sprintf(buf, "Time left for current quest: %d ticks.\r\n", ch->pcdata->countdown);
            send_to_char(buf, ch);
        }
        return;
    }

    // Try to find a quest master in the room
    questman = find_quest_master(ch);

    if (questman == NULL || questman->spec_fun != spec_lookup("spec_questmaster"))
    {
        send_to_char("You can't do that here.\r\n", ch);
        return;
    }

    if (questman->fighting != NULL)
    {
        send_to_char("Wait until the fighting stops.\r\n", ch);
        return;
    }

    ch->pcdata->quest_giver = questman;

/* And, of course, you will need to change the following lines for YOUR
   quest item information. Quest items on Moongate are unbalanced, very
   very nice items, and no one has one yet, because it takes awhile to
   build up quest points :> Make the item worth their while. */

    if (!strcmp(arg1, "list"))
    {
        act("$n asks $N for a list of quest items.\r\n", ch, NULL, questman, TO_ROOM);
        act("You ask $N for a list of quest items.\r\n", ch, NULL, questman, TO_CHAR);

        for (x = 0; quest_item_table[x].keyword != NULL; x++)
        {
            sprintf(buf, "  {c%-30s{x %d Quest Points\r\n", quest_item_table[x].desc, quest_item_table[x].cost);
            send_to_char(buf, ch);
        }

        if (ch && ch->pcdata)
        {
            sprintf(buf, "\r\n  You currently have {W%d{x quest points to spend.\r\n", ch->pcdata->quest_points);
            send_to_char(buf, ch);
        }

        send_to_char("\r\nType {Gpquest buy <item name>{x to purchase a quest item.\r\n", ch);

        return;
    }

    else if (!strcmp(arg1, "buy"))
    {
        if (arg2[0] == '\0')
        {
            send_to_char("To buy an item, type 'QUEST BUY <item>'.\r\n", ch);
            return;
        }

        for (x = 0; quest_item_table[x].keyword != NULL; x++)
        {
            if (is_name(arg2, quest_item_table[x].keyword))
            {
                if (ch->pcdata->quest_points >= quest_item_table[x].cost)
                {
                    ch->pcdata->quest_points -= quest_item_table[x].cost;
                    obj = create_object(get_obj_index(quest_item_table[x].vnum));
                    break;
                }
                else
                {
                    sprintf(buf, "Sorry, %s, but you don't have enough quest points for that.", ch->name);
                    do_say(questman, buf);
                    return;
                }
            }

        }

        if (obj == NULL)
        {
            sprintf(buf, "I don't have that item, %s.", ch->name);
            do_say(questman, buf);

            sprintf(buf, "%s attempted to buy a quest item that was listed but does not exist.", ch->name);
            wiznet(buf, ch, NULL, WIZ_GENERAL, 0, 0);

            return;
        }
        else
        {
            act("$N gives $p to $n.", ch, obj, questman, TO_ROOM);
            act("$N gives you $p.", ch, obj, questman, TO_CHAR);
            obj_to_char(obj, ch);
        }

        return;
    }
    else if (!strcmp(arg1, "request"))
    {
        act("$n asks $N for a quest.", ch, NULL, questman, TO_ROOM);
        act("You ask $N for a quest.", ch, NULL, questman, TO_CHAR);

        if (IS_SET(ch->act, PLR_QUESTOR))
        {
            sprintf(buf, "But you're already on a quest!");
            do_say(questman, buf);
            return;
        }

        if (ch->pcdata->next_quest > 0)
        {
            sprintf(buf, "You're very brave, %s, but let someone else have a chance.", ch->name);
            do_say(questman, buf);
            do_say(questman, "Come back later.");
            return;
        }

        sprintf(buf, "Thank you, brave %s!", ch->name);
        do_say(questman, buf);
        ch->pcdata->quest_mob = 0;
        ch->pcdata->quest_obj = 0;

        generate_quest(ch, questman);

        if (ch->pcdata->quest_mob > 0 || ch->pcdata->quest_obj > 0)
        {
            ch->pcdata->countdown = number_range(40, 120);
            SET_BIT(ch->act, PLR_QUESTOR);
            sprintf(buf, "You have %d ticks to complete this quest.", ch->pcdata->countdown);
            do_say(questman, buf);
            do_say(questman, "May the gods go with you!");
        }

        return;
    }
    else if (!strcmp(arg1, "clear"))
    {
        if (ch->pcdata->quest_giver == NULL)
        {
            do_say(questman, "I never sent you on a quest! Perhaps you're thinking of someone else.");
            return;
        }

        if (IS_SET(ch->act, PLR_QUESTOR))
        {
            act("$n informs $N $e can not complete $s quest.", ch, NULL, questman, TO_ROOM);
            act("You inform $N you can not complete $s quest.", ch, NULL, questman, TO_CHAR);
            REMOVE_BIT(ch->act, PLR_QUESTOR);
            sprintf(buf, "%s I see my faith was misplaced with you.", ch->name);
            do_direct(questman, buf);
            ch->pcdata->quest_giver = NULL;
            ch->pcdata->countdown = 0;
            ch->pcdata->quest_mob = 0;
            ch->pcdata->quest_obj = 0;
            ch->pcdata->next_quest = 0;

            // Deduct quest points for clearing
            if (ch->pcdata->quest_points >= 20)
            {
                sprintf(buf, "%s I have deducted a small amount of credit for this courtesy.", ch->name);
                do_direct(questman, buf);
                ch->pcdata->quest_points -= 20;
            }
            else
            {
                if (ch->pcdata->quest_points > 0 && ch->pcdata->quest_points < 20)
                {
                    sprintf(buf, "%s I have deducted a small amount of credit for this courtesy.", ch->name);
                    do_direct(questman, buf);
                }

                ch->pcdata->quest_points = 0;
            }

            return;

        }
        else
        {
            sprintf(buf, "%s You are not currently on a quest!", ch->name);
            do_direct(questman, buf);
            return;
        }
    }
    else if (!strcmp(arg1, "complete"))
    {
        act("$n informs $N $e has completed $s quest.", ch, NULL, questman, TO_ROOM);
        act("You inform $N you have completed $s quest.", ch, NULL, questman, TO_CHAR);

        if (ch->pcdata->quest_giver != questman)
        {
            sprintf(buf, "I never sent you on a quest! Perhaps you're thinking of someone else.");
            do_say(questman, buf);
            return;
        }

        if (IS_SET(ch->act, PLR_QUESTOR))
        {
            if (ch->pcdata->quest_mob == -1 && ch->pcdata->countdown > 0)
            {
                int reward, pointreward;

                reward = number_range(60, 110);
                pointreward = number_range(25, 75);

                sprintf(buf, "Congratulations on completing your quest!");
                do_say(questman, buf);
                sprintf(buf, "As a reward, I am giving you %d quest points, and %d gold.", pointreward, reward);
                do_say(questman, buf);

                REMOVE_BIT(ch->act, PLR_QUESTOR);
                ch->pcdata->quest_giver = NULL;
                ch->pcdata->countdown = 0;
                ch->pcdata->quest_mob = 0;
                ch->pcdata->quest_obj = 0;
                ch->pcdata->next_quest = 10;
                ch->gold += reward;
                ch->pcdata->quest_points += pointreward;

                return;
            }
            else if (ch->pcdata->quest_obj > 0 && ch->pcdata->countdown > 0)
            {
                bool obj_found = FALSE;

                for (obj = ch->carrying; obj != NULL; obj = obj_next)
                {
                    obj_next = obj->next_content;

                    if (obj != NULL && obj->pIndexData->vnum == ch->pcdata->quest_obj)
                    {
                        obj_found = TRUE;
                        break;
                    }
                }
                if (obj_found == TRUE)
                {
                    int reward, pointreward;

                    reward = number_range(50, 100);
                    pointreward = number_range(25, 75);

                    act("You hand $p to $N.", ch, obj, questman, TO_CHAR);
                    act("$n hands $p to $N.", ch, obj, questman, TO_ROOM);

                    sprintf(buf, "Congratulations on completing your quest!");
                    do_say(questman, buf);
                    sprintf(buf, "As a reward, I am giving you %d quest points, and %d gold.", pointreward, reward);
                    do_say(questman, buf);

                    REMOVE_BIT(ch->act, PLR_QUESTOR);
                    ch->pcdata->quest_giver = NULL;
                    ch->pcdata->countdown = 0;
                    ch->pcdata->quest_mob = 0;
                    ch->pcdata->quest_obj = 0;
                    ch->pcdata->next_quest = 5;
                    ch->gold += reward;
                    ch->pcdata->quest_points += pointreward;
                    separate_obj(obj);
                    extract_obj(obj);
                    return;
                }
                else
                {
                    sprintf(buf, "You haven't completed the quest yet, but there is still time!");
                    do_say(questman, buf);
                    return;
                }
                return;
            }
            else if ((ch->pcdata->quest_mob > 0 || ch->pcdata->quest_obj > 0) && ch->pcdata->countdown > 0)
            {
                sprintf(buf, "You haven't completed the quest yet, but there is still time!");
                do_say(questman, buf);
                return;
            }
        }

        if (ch->pcdata->next_quest > 0)
        {
            sprintf(buf, "But you didn't complete your quest in time!");
        }
        else
        {
            sprintf(buf, "You have to REQUEST a quest first, %s.", ch->name);
        }

        do_say(questman, buf);
        return;
    }

    send_to_char("QUEST commands: POINTS INFO TIME REQUEST COMPLETE CLEAR LIST BUY.\r\n", ch);
    send_to_char("For more information, type 'HELP QUEST'.\r\n", ch);
    return;
}

/*
 * Attempts to generates a quest for a player.
 */
void generate_quest(CHAR_DATA *ch, CHAR_DATA *questman)
{
    CHAR_DATA *victim;
    ROOM_INDEX_DATA *room;
    OBJ_DATA *questitem;
    char buf[MAX_STRING_LENGTH];

    /*  Randomly selects a mob from the world mob list. If you don't
        want a mob to be selected, make sure it is immune to summon.
        Or, you could add a new mob flag called ACT_NOQUEST. The mob
        is selected for both mob and obj quests, even tho in the obj
        quest the mob is not used. This is done to assure the level
        of difficulty for the area isn't too great for the player. */
    for (victim = char_list; victim != NULL; victim = victim->next)
    {
        if (!IS_NPC(victim)
            || victim->in_room == NULL
            || victim->desc)
        {
            continue;
        }

        if (victim->in_room->sector_type == SECT_WATER_SWIM
            || victim->in_room->sector_type == SECT_OCEAN
            || victim->in_room->sector_type == SECT_UNDERWATER
            || !can_see_room(ch, victim->in_room)
            || is_safe_quest(ch, victim))
        {
            continue;
        }

        if (quest_level_diff(ch->level, victim->level) == TRUE
            && !IS_SET(victim->imm_flags, IMM_SUMMON)
            && victim->pIndexData != NULL
            && victim->pIndexData->pShop == NULL
            && !IS_SET(victim->act, ACT_PET)
            && !IS_SET(victim->affected_by, AFF_CHARM)
            && CHANCE(15))
        {
            break;
        }
    }

    if (victim == NULL)
    {
        do_say(questman, "I'm sorry, but I don't have any quests for you at this time.");
        do_say(questman, "Try again later.");
        ch->pcdata->next_quest = 2;
        return;
    }

    if ((room = victim->in_room) == NULL)
    {
        do_say(questman, "I'm sorry, but I don't have any quests for you at this time.");
        do_say(questman, "Try again later.");
        ch->pcdata->next_quest = 2;
        return;
    }

    /*  50% chance it will send the player on a 'recover item' quest. */
    if (CHANCE(50))
    {
        int objvnum = 0;

        switch (number_range(0, 1))
        {
            case 0:
                objvnum = QUEST_OBJ1;
                break;

            case 1:
                objvnum = QUEST_OBJ2;
                break;

        }

        questitem = create_object(get_obj_index(objvnum));
        questitem->timer = 130;
        obj_to_room(questitem, room);
        ch->pcdata->quest_obj = questitem->pIndexData->vnum;

        sprintf(buf, "Vile pilferers have stolen %s from the royal treasury!", questitem->short_descr);
        do_say(questman, buf);
        do_say(questman, "My court wizardess, with her magic mirror, has pinpointed its location.");

        sprintf(buf, "Look in the general area of %s for %s!", room->area->name, room->name);
        do_say(questman, buf);

        sprintf(buf, "You may find %s near the vicinity you seek.  Now go.", victim->short_descr);
        do_say(questman, buf);

        return;
    }

    /* Quest to kill a mob */

    else
    {
        switch (number_range(0, 1))
        {
            case 0:
                sprintf(buf, "An enemy of mine, %s, is making vile threats against the crown.", victim->short_descr);
                do_say(questman, buf);
                sprintf(buf, "This threat must be eliminated!");
                do_say(questman, buf);
                break;
            case 1:
                sprintf(buf, "A heinous criminal, %s, has escaped from the dungeon!", victim->short_descr);
                do_say(questman, buf);
                sprintf(buf, "Since the escape, %s has murdered %d civillians!", victim->short_descr, number_range(2, 20));
                do_say(questman, buf);
                do_say(questman, "The penalty for this crime is death, and you are to deliver the sentence!");
                break;
        }

        if (room->name != NULL)
        {
            sprintf(buf, "Seek %s out somewhere in the vicinity of %s!", victim->short_descr, room->name);
            do_say(questman, buf);

            sprintf(buf, "That location is in the general area of %s.", room->area->name);
            do_say(questman, buf);
        }

        ch->pcdata->quest_mob = victim->pIndexData->vnum;
    }
    return;
}

/*
 * Level differences to search for. Moongate has 350 levels, so you will
 * want to tweak these greater or less than statements for yourself. - Vassago
 */
bool quest_level_diff(int clevel, int mlevel)
{
    if ((clevel - 3 < mlevel) && (clevel + 5 > mlevel))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*
 * Quest update code that is called every tick from the update_handler().
 */
void quest_update(void)
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *ch;

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->character != NULL && d->connected == CON_PLAYING)
        {
            ch = d->character;

            // Probably not needed but since we're accessing pcdata we'll check anyway.
            if (IS_NPC(ch))
            {
                continue;
            }

            if (ch->pcdata->next_quest > 0)
            {
                ch->pcdata->next_quest--;

                if (ch->pcdata->next_quest <= 0)
                {
                    ch->pcdata->next_quest = 0;
                    send_to_char("You may now quest again.\r\n", ch);
                    return;
                }
            }
            else if (IS_SET(ch->act, PLR_QUESTOR) && !IS_NPC(ch))
            {
                ch->pcdata->countdown--;
                if (ch->pcdata->countdown <= 0)
                {
                    char buf[MAX_STRING_LENGTH];

                    ch->pcdata->next_quest = 5;
                    sprintf(buf, "You have run out of time for your quest!\r\nYou may quest again in %d ticks.\r\n",
                        ch->pcdata->next_quest);

                    send_to_char(buf, ch);
                    REMOVE_BIT(ch->act, PLR_QUESTOR);
                    ch->pcdata->quest_giver = NULL;
                    ch->pcdata->countdown = 0;
                    ch->pcdata->quest_mob = 0;
                }

                if (ch->pcdata->countdown > 0 && ch->pcdata->countdown < 6)
                {
                    send_to_char("Better hurry, you're almost out of time for your quest!\r\n", ch);
                }
            }
        }
    }

    return;
}

/*
 * Whether a mob/char is safe from being selected as part of the quest
 * for the person requesting.
 */
bool is_safe_quest(CHAR_DATA *ch, CHAR_DATA *victim)
{
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    // Check the players affects
    for (paf = ch->affected; paf != NULL; paf = paf_next)
    {
        paf_next = paf->next;

        // Ghosts are safe
        if (paf->type == gsn_ghost)
        {
            return TRUE;
        }
    }

    // Check the victims affects
    for (paf = victim->affected; paf != NULL; paf = paf_next)
    {
        paf_next = paf->next;

        if (paf->type == gsn_ghost)
        {
            return TRUE;
        }
    }

    if (victim->in_room == NULL || ch->in_room == NULL)
    {
        return TRUE;
    }

    if (victim->fighting == ch || victim == ch)
    {
        return FALSE;
    }

    if (IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL)
    {
        return FALSE;
    }

    /* killing mobiles */
    if (IS_NPC(victim))
    {
        /* safe room? */
        if (IS_SET(victim->in_room->room_flags, ROOM_SAFE))
            return TRUE;

        if (victim->pIndexData->pShop != NULL)
            return TRUE;

        /* no killing healers, trainers, etc */
        if (IS_SET(victim->act, ACT_TRAIN)
            || IS_SET(victim->act, ACT_PRACTICE)
            || IS_SET(victim->act, ACT_IS_HEALER)
            || IS_SET(victim->act, ACT_IS_CHANGER))
        {
            return TRUE;
        }

        if (!IS_NPC(ch))
        {
            /* no pets */
            if (IS_SET(victim->act, ACT_PET))
            {
                return TRUE;
            }

            /* no charmed creatures unless owner */
            if (IS_AFFECTED(victim, AFF_CHARM) && ch != victim->master)
            {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/*
 * Finds the char data for quest giver in the character list for
 * a given VNUM (used to re-load that pointer when a player logs
 * in).
 */
CHAR_DATA *get_quest_giver(int vnum)
{
    CHAR_DATA *wch;

    for (wch = char_list; wch != NULL; wch = wch->next)
    {
        if (!IS_NPC(wch) || wch->pIndexData->vnum != vnum)
        {
            continue;
        }
    }

    return wch;
}

/*
 * Finds a quest master in the current room if one exists.  This requires spec_questmaster
 * to be defined in special.c.
 */
CHAR_DATA *find_quest_master(CHAR_DATA * ch)
{
    CHAR_DATA *mob;

    if (ch == NULL)
    {
        return NULL;
    }

    // Check for a quest master
    for (mob = ch->in_room->people; mob != NULL; mob = mob->next_in_room)
    {
        if (IS_NPC(mob) && mob->spec_fun == spec_lookup("spec_questmaster"))
        {
            return mob;
        }
    }

    return NULL;

} // end find_quest_master

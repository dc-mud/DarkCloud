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

// General Includes
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"

char *print_flags(int flag)
{
    int count, pos = 0;
    static char buf[52];


    for (count = 0; count < 32; count++)
    {
        if (IS_SET(flag, 1 << count))
        {
            if (count < 26)
                buf[pos] = 'A' + count;
            else
                buf[pos] = 'a' + (count - 26);
            pos++;
        }
    }

    if (pos == 0)
    {
        buf[pos] = '0';
        pos++;
    }

    buf[pos] = '\0';

    return buf;
}

/*
 * Array of containers read for proper re-nesting of objects.
 */
#define MAX_NEST    100
static OBJ_DATA *rgObjNest[MAX_NEST];

/*
 * Local functions.
 */
void fwrite_char (CHAR_DATA * ch, FILE * fp);
void fwrite_obj (CHAR_DATA * ch, OBJ_DATA * obj, FILE * fp, int iNest);
void fwrite_pet (CHAR_DATA * pet, FILE * fp);
void fread_char (CHAR_DATA * ch, FILE * fp);
void fread_pet (CHAR_DATA * ch, FILE * fp);
void fread_obj (CHAR_DATA * ch, FILE * fp);

/*
 * Save a character and inventory.
 */
void save_char_obj(CHAR_DATA * ch)
{
    char save_file[MAX_INPUT_LENGTH];
    FILE *fp;

    if (IS_NPC(ch))
        return;

    // Do not save a character while they are reclassing.
    if (ch->pcdata->is_reclassing == TRUE)
        return;

    /*
     * Fix by Edwin. JR -- 10/15/00
     *
     * Don't save if the character is invalidated.
     * This might happen during the auto-logoff of players.
     * (or other places not yet found out)
     */
    if (!IS_VALID(ch)) {
        bug("save_char_obj: Trying to save an invalidated character.\n", 0);
        return;
    }

    if (ch->desc != NULL && ch->desc->original != NULL)
        ch = ch->desc->original;

#if defined(unix)
    /* create god log */
    if (IS_IMMORTAL(ch) || ch->level >= LEVEL_IMMORTAL)
    {
        fclose(fpReserve);
        sprintf(save_file, "%s%s", GOD_DIR, capitalize(ch->name));
        if ((fp = fopen(save_file, "w")) == NULL)
        {
            bug("Save_char_obj: fopen", 0);
            perror(save_file);
        }

        fprintf(fp, "Lev %2d Trust %2d  %s%s\n",
            ch->level, get_trust(ch), ch->name, ch->pcdata->title);
        fclose(fp);
        fpReserve = fopen(NULL_FILE, "r");
    }
#endif

    fclose(fpReserve);
    sprintf(save_file, "%s%s", PLAYER_DIR, capitalize(ch->name));

    if ((fp = fopen(TEMP_FILE, "w")) == NULL)
    {
        bug("Save_char_obj: fopen", 0);
        perror(save_file);
    }
    else
    {
        fwrite_char(ch, fp);
        if (ch->carrying != NULL)
            fwrite_obj(ch, ch->carrying, fp, 0);
        /* save the pets */
        if (ch->pet != NULL && ch->pet->in_room == ch->in_room)
            fwrite_pet(ch->pet, fp);
        fprintf(fp, "#END\n");
    }
    fclose(fp);

#if defined(_WIN32)
    // In MS C rename will fail if the file exists (not so on POSIX).  In Windows, it will never
    // save past the first pfile save if this isn't done.
    _unlink(save_file);
#endif

    rename(TEMP_FILE, save_file);
    fpReserve = fopen(NULL_FILE, "r");

    // Write the players state to the dss database if db logging is enabled.
    log_player(ch);

    return;
}

/*
 * Write the char.
 */
void fwrite_char(CHAR_DATA * ch, FILE * fp)
{
    AFFECT_DATA *paf;
    int sn, gn, pos;
    extern int top_group;

    // Although this could save a mob, it's never called from a place
    // that allows that, so pcdata calls should be safe.
    fprintf(fp, "#%s\n", IS_NPC(ch) ? "MOB" : "PLAYER");

    fprintf(fp, "Name %s~\n", ch->name);
    fprintf(fp, "Id   %ld\n", ch->id);
    fprintf(fp, "LogOffTime %ld\n", current_time);
    fprintf(fp, "Version %d\n", 1);

    if (ch->short_descr[0] != '\0')
        fprintf(fp, "ShortDescription %s~\n", ch->short_descr);
    if (ch->long_descr[0] != '\0')
        fprintf(fp, "LongDescription %s~\n", ch->long_descr);
    if (ch->description[0] != '\0')
        fprintf(fp, "Description %s~\n", ch->description);
    if (ch->prompt != NULL)
        fprintf(fp, "Prompt %s~\n", ch->prompt);
    fprintf(fp, "Race %s~\n", pc_race_table[ch->race].name);

    // Clan, clan flags and clan rank
    if (ch->clan)
    {
        fprintf(fp, "Clan %s~\n", clan_table[ch->clan].name);
        fprintf(fp, "ClanFlags %d\n", ch->pcdata->clan_flags);
        fprintf(fp, "ClanRank %s~\n", clan_rank_table[ch->pcdata->clan_rank].name);
    }

    fprintf(fp, "Sex  %d\n", ch->sex);
    fprintf(fp, "Class %d\n", ch->class);
    fprintf(fp, "Level %d\n", ch->level);
    if (ch->trust != 0)
        fprintf(fp, "TrustLevel %d\n", ch->trust);
    fprintf(fp, "Security %d\n", ch->pcdata->security);    /* OLC */
    fprintf(fp, "Played %d\n", ch->played + (int)(current_time - ch->logon));
    fprintf(fp, "Pkilled %d\n", ch->pcdata->pkilled);
    fprintf(fp, "Pkills %d\n", ch->pcdata->pkills);
    fprintf(fp, "PkillsArena %d\n", ch->pcdata->pkills_arena);

    fprintf(fp, "Scroll %d\n", ch->lines);
    fprintf(fp, "Room %d\n", (ch->in_room == get_room_index(ROOM_VNUM_LIMBO)
        && ch->was_in_room != NULL)
        ? ch->was_in_room->vnum
        : ch->in_room == NULL ? 3001 : ch->in_room->vnum);

    fprintf(fp, "HMV  %d %d %d %d %d %d\n",
        ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move,
        ch->max_move);

    fprintf(fp, "Gold %ld\n", ch->gold);
    fprintf(fp, "Silver %ld\n", ch->silver);

    // Money they have stored in the bank
    fprintf(fp, "BankGold %ld\n", ch->pcdata->bank_gold);

    fprintf(fp, "Experience  %d\n", ch->exp);

    if (ch->act != 0)
        fprintf(fp, "Act  %s\n", print_flags(ch->act));
    if (ch->affected_by != 0)
        fprintf(fp, "AfBy %s\n", print_flags(ch->affected_by));
    fprintf(fp, "Comm %s\n", print_flags(ch->comm));
    if (ch->wiznet)
        fprintf(fp, "Wiznet %s\n", print_flags(ch->wiznet));
    if (ch->invis_level)
        fprintf(fp, "InvisLevel %d\n", ch->invis_level);
    if (ch->incog_level)
        fprintf(fp, "IncognitoLevel %d\n", ch->incog_level);
    fprintf(fp, "Position %d\n",
        ch->position == POS_FIGHTING ? POS_STANDING : ch->position);
    if (ch->practice != 0)
        fprintf(fp, "Practice %d\n", ch->practice);
    if (ch->train != 0)
        fprintf(fp, "Train %d\n", ch->train);
    if (ch->saving_throw != 0)
        fprintf(fp, "Save  %d\n", ch->saving_throw);
    fprintf(fp, "Alignment %d\n", ch->alignment);
    if (ch->hitroll != 0)
        fprintf(fp, "Hitroll %d\n", ch->hitroll);
    if (ch->damroll != 0)
        fprintf(fp, "Damroll %d\n", ch->damroll);
    fprintf(fp, "ACs %d %d %d %d\n",
        ch->armor[0], ch->armor[1], ch->armor[2], ch->armor[3]);
    if (ch->wimpy != 0)
        fprintf(fp, "Wimpy %d\n", ch->wimpy);
    fprintf(fp, "Attributes %d %d %d %d %d\n",
        ch->perm_stat[STAT_STR],
        ch->perm_stat[STAT_INT],
        ch->perm_stat[STAT_WIS],
        ch->perm_stat[STAT_DEX],
        ch->perm_stat[STAT_CON]);

    fprintf(fp, "AttributeModifier %d %d %d %d %d\n",
        ch->mod_stat[STAT_STR],
        ch->mod_stat[STAT_INT],
        ch->mod_stat[STAT_WIS],
        ch->mod_stat[STAT_DEX],
        ch->mod_stat[STAT_CON]);

    if (IS_NPC(ch))
    {
        fprintf(fp, "Vnum %d\n", ch->pIndexData->vnum);
    }
    else
    {
        fprintf(fp, "Password %s~\n", ch->pcdata->pwd);

        // Recall vnum (if recall vnum is 0 then the default will be used)
        fprintf(fp, "RecallVnum %d\n", ch->pcdata->recall_vnum);

        // Update the last available IP address
        if (ch->desc != NULL
            && !IS_NULLSTR(ch->desc->host)
            && !IS_NULLSTR(ch->pcdata->last_ip)
            && !str_cmp(ch->pcdata->last_ip, ch->desc->host))
        {
            // Last IP was the same, just write it out
            fprintf(fp, "LastIpAddress %s~\n", ch->pcdata->last_ip);
        }
        else if (ch->desc != NULL && !IS_NULLSTR(ch->desc->host))
        {
            // The IP was different, update it, then write it out
            free_string(ch->pcdata->last_ip);
            ch->pcdata->last_ip = str_dup(ch->desc->host);
            fprintf(fp, "LastIpAddress %s~\n", ch->pcdata->last_ip);
        }
        else if (ch->desc == NULL && !IS_NULLSTR(ch->pcdata->last_ip))
        {
            // They're link dead, but we have a last_ip, save it
            fprintf(fp, "LastIpAddress %s~\n", ch->pcdata->last_ip);
        }
        else
        {
            fprintf(fp, "LastIpAddress Unknown~\n");
        }

        if (ch->pcdata->bamfin[0] != '\0')
            fprintf(fp, "Bamfin %s~\n", ch->pcdata->bamfin);

        if (ch->pcdata->bamfout[0] != '\0')
            fprintf(fp, "Bamfout %s~\n", ch->pcdata->bamfout);

        if (ch->pcdata->email[0] != '\0')
            fprintf(fp, "Email %s~\n", ch->pcdata->email);

        fprintf(fp, "Title %s~\n", ch->pcdata->title);
        fprintf(fp, "Points %d\n", ch->pcdata->points);
        fprintf(fp, "TrueSex %d\n", ch->pcdata->true_sex);
        fprintf(fp, "LastLevel %d\n", ch->pcdata->last_level);

        fprintf(fp, "HMVP %d %d %d\n", ch->pcdata->perm_hit,
            ch->pcdata->perm_mana, ch->pcdata->perm_move);

        fprintf(fp, "Condition %d %d %d %d\n",
            ch->pcdata->condition[0],
            ch->pcdata->condition[1],
            ch->pcdata->condition[2],
            ch->pcdata->condition[3]);

        fprintf(fp, "Stance %d\n", ch->stance);
        fprintf(fp, "VnumClairvoyance %d\n", ch->pcdata->vnum_clairvoyance);
        fprintf(fp, "PriestRank %d\n", ch->pcdata->priest_rank);
        fprintf(fp, "Deity %d\n", ch->pcdata->deity);

        // Notes
        fprintf(fp, "LastNote %ld\n", ch->pcdata->last_note);
        fprintf(fp, "LastPenalty %ld\n", ch->pcdata->last_penalty);
        fprintf(fp, "LastNews %ld\n", ch->pcdata->last_news);
        fprintf(fp, "LastChange %ld\n", ch->pcdata->last_change);
        fprintf(fp, "LastOoc %ld\n", ch->pcdata->last_ooc);
        fprintf(fp, "LastStoryNote %ld\n", ch->pcdata->last_story);
        fprintf(fp, "LastHistory %ld\n", ch->pcdata->last_history);
        fprintf(fp, "LastImmNote %ld\n", ch->pcdata->last_immnote);

        // Key Ring
        fprintf(fp, "KeyRing %d %d %d %d %d %d %d %d %d %d\n",
            ch->pcdata->key_ring[0], ch->pcdata->key_ring[1],
            ch->pcdata->key_ring[2], ch->pcdata->key_ring[3],
            ch->pcdata->key_ring[4], ch->pcdata->key_ring[5],
            ch->pcdata->key_ring[6], ch->pcdata->key_ring[7],
            ch->pcdata->key_ring[8], ch->pcdata->key_ring[9]);

        // Improves
        fprintf(fp, "ImproveFocus %s~\n", skill_table[ch->pcdata->improve_focus_gsn]->name);
        fprintf(fp, "ImproveMinutes %d\n", ch->pcdata->improve_minutes);

        // Questing
        if (ch->pcdata->quest_points != 0)
        {
            fprintf(fp, "QuestPoints %d\n", ch->pcdata->quest_points);
        }

        if (ch->pcdata->countdown != 0)
        {
            fprintf(fp, "QuestCount %d\n", ch->pcdata->countdown);
        }

        if (ch->pcdata->next_quest != 0)
        {
            fprintf(fp, "QuestNext %d\n", ch->pcdata->next_quest);
        }
        else if (ch->pcdata->countdown != 0)
        {
            fprintf(fp, "QuestNext %d\n", 10);
        }

        if (ch->pcdata->quest_obj != 0)
        {
            fprintf( fp, "QuestObj %d\n",  ch->pcdata->quest_obj);
        }

        if (ch->pcdata->quest_mob != 0)
        {
            fprintf( fp, "QuestMob %d\n",  ch->pcdata->quest_mob);
        }

        if (ch->pcdata->quest_giver != NULL)
        {
            fprintf( fp, "QuestGiver %d\n",  ch->pcdata->quest_giver->pIndexData->vnum);
        }

        if (ch->pcdata->merit != 0)
        {
            fprintf(fp, "Merit %s\n", print_flags(ch->pcdata->merit));
        }

        /* write alias */
        for (pos = 0; pos < MAX_ALIAS; pos++)
        {
            if (ch->pcdata->alias[pos] == NULL
                || ch->pcdata->alias_sub[pos] == NULL)
                break;

            fprintf(fp, "Alias %s %s~\n", ch->pcdata->alias[pos],
                ch->pcdata->alias_sub[pos]);
        }

        for (sn = 0; sn < top_sn; sn++)
        {
            if (skill_table[sn]->name != NULL && ch->pcdata->learned[sn] > 0)
            {
                fprintf(fp, "Skill %d '%s'\n",
                    ch->pcdata->learned[sn], skill_table[sn]->name);
            }
        }

        for (gn = 0; gn < top_group; gn++)
        {
            if (group_table[gn]->name != NULL && ch->pcdata->group_known[gn])
            {
                fprintf(fp, "Group '%s'\n", group_table[gn]->name);
            }
        }
    }

    for (paf = ch->affected; paf != NULL; paf = paf->next)
    {
        if (paf->type < 0 || paf->type >= top_sn)
            continue;

        fprintf(fp, "Affect '%s' %3d %3d %3d %3d %3d %10d\n",
            skill_table[paf->type]->name,
            paf->where,
            paf->level,
            paf->duration, paf->modifier, paf->location, paf->bitvector);
    }
    fprintf(fp, "End\n\n");
    return;
}

/* write a pet */
void fwrite_pet(CHAR_DATA * pet, FILE * fp)
{
    AFFECT_DATA *paf;

    fprintf(fp, "#PET\n");

    fprintf(fp, "Vnum %d\n", pet->pIndexData->vnum);

    fprintf(fp, "Name %s~\n", pet->name);
    fprintf(fp, "LogOffTime %ld\n", current_time);
    if (pet->short_descr != pet->pIndexData->short_descr)
        fprintf(fp, "ShortDescription %s~\n", pet->short_descr);
    if (pet->long_descr != pet->pIndexData->long_descr)
        fprintf(fp, "LongDescription %s~\n", pet->long_descr);
    if (pet->description != pet->pIndexData->description)
        fprintf(fp, "Description %s~\n", pet->description);
    if (pet->race != pet->pIndexData->race)
        fprintf(fp, "Race %s~\n", race_table[pet->race].name);
    if (pet->clan)
        fprintf(fp, "Clan %s~\n", clan_table[pet->clan].name);
    fprintf(fp, "Sex  %d\n", pet->sex);
    if (pet->level != pet->pIndexData->level)
        fprintf(fp, "Level %d\n", pet->level);
    fprintf(fp, "HMV  %d %d %d %d %d %d\n",
        pet->hit, pet->max_hit, pet->mana, pet->max_mana, pet->move,
        pet->max_move);
    if (pet->gold > 0)
        fprintf(fp, "Gold %ld\n", pet->gold);
    if (pet->silver > 0)
        fprintf(fp, "Silver %ld\n", pet->silver);
    if (pet->exp > 0)
        fprintf(fp, "Experience %d\n", pet->exp);
    if (pet->act != pet->pIndexData->act)
        fprintf(fp, "Act  %s\n", print_flags(pet->act));
    if (pet->affected_by != pet->pIndexData->affected_by)
        fprintf(fp, "AfBy %s\n", print_flags(pet->affected_by));
    if (pet->comm != 0)
        fprintf(fp, "Comm %s\n", print_flags(pet->comm));
    fprintf(fp, "Position %d\n", pet->position =
        POS_FIGHTING ? POS_STANDING : pet->position);
    if (pet->saving_throw != 0)
        fprintf(fp, "Save %d\n", pet->saving_throw);
    if (pet->alignment != pet->pIndexData->alignment)
        fprintf(fp, "Alignment %d\n", pet->alignment);
    if (pet->hitroll != pet->pIndexData->hitroll)
        fprintf(fp, "Hitroll %d\n", pet->hitroll);
    if (pet->damroll != pet->pIndexData->damage[DICE_BONUS])
        fprintf(fp, "Damroll %d\n", pet->damroll);
    fprintf(fp, "ACs  %d %d %d %d\n",
        pet->armor[0], pet->armor[1], pet->armor[2], pet->armor[3]);
    fprintf(fp, "Attributes %d %d %d %d %d\n",
        pet->perm_stat[STAT_STR], pet->perm_stat[STAT_INT],
        pet->perm_stat[STAT_WIS], pet->perm_stat[STAT_DEX],
        pet->perm_stat[STAT_CON]);
    fprintf(fp, "AttributeModifier %d %d %d %d %d\n",
        pet->mod_stat[STAT_STR], pet->mod_stat[STAT_INT],
        pet->mod_stat[STAT_WIS], pet->mod_stat[STAT_DEX],
        pet->mod_stat[STAT_CON]);

    for (paf = pet->affected; paf != NULL; paf = paf->next)
    {
        if (paf->type < 0 || paf->type >= top_sn)
            continue;

        fprintf(fp, "Affect '%s' %3d %3d %3d %3d %3d %10d\n",
            skill_table[paf->type]->name,
            paf->where, paf->level, paf->duration, paf->modifier,
            paf->location, paf->bitvector);
    }

    fprintf(fp, "End\n");
    return;
}

/*
 * Write an object and its contents.
 */
void fwrite_obj(CHAR_DATA * ch, OBJ_DATA * obj, FILE * fp, int iNest)
{
    EXTRA_DESCR_DATA *ed;
    AFFECT_DATA *paf;
    int room_vnum = 0;

    /*
     * Slick recursion to write lists backwards, so loading them will load in forwards order.
     * This will only run for players now since we're using this to save objects like pits and
     * corpses.
     */
    if ((ch && !IS_NPC(ch)) || obj->in_room == NULL)
    {
        if (obj->next_content != NULL)
        {
            fwrite_obj(ch, obj->next_content, fp, iNest);
        }
    }

    /*
     * Castrate storage characters.  This will skip writing objects that are more than
     * two levels higher out to the pfile.  This means, the player can carry them for
     * as long as they want but they won't be saved, when the logout and log back in the
     * objects will go *poof*.
     */
    if (ch)
    {
        if ((ch->level < obj->level - 2 && obj->item_type != ITEM_CONTAINER)
            || obj->item_type == ITEM_KEY
            || (obj->item_type == ITEM_MAP && !obj->value[0]))
            return;
    }
    else
    {
        if (obj->in_room != NULL)
        {
            room_vnum = obj->in_room->vnum;
        }
    }

    fprintf(fp, "#O\n");
    fprintf(fp, "Vnum %d\n", obj->pIndexData->vnum);

    if (obj->enchanted)
        fprintf(fp, "Enchanted\n");

    fprintf(fp, "Nest %d\n", iNest);

    /*
     * This won't be used when characters are loaded, rather it's for persisting special
     * objects in rooms across reboots/copyovers (like donation pits and player corpses).
     */
    if (!ch && room_vnum != 0)
    {
        fprintf(fp, "RoomVnum    %d\n", room_vnum);
    }

    if (obj->count > 1)
        fprintf(fp, "Count %d\n", obj->count);

    /* these data are only used if they do not match the defaults */
    if (obj->name != obj->pIndexData->name)
        fprintf(fp, "Name %s~\n", obj->name);
    if (obj->short_descr != obj->pIndexData->short_descr)
        fprintf(fp, "ShD  %s~\n", obj->short_descr);
    if (obj->description != obj->pIndexData->description)
        fprintf(fp, "Desc %s~\n", obj->description);
    if (obj->extra_flags != obj->pIndexData->extra_flags)
        fprintf(fp, "ExtF %d\n", obj->extra_flags);
    if (obj->wear_flags != obj->pIndexData->wear_flags)
        fprintf(fp, "WeaF %d\n", obj->wear_flags);
    if (obj->item_type != obj->pIndexData->item_type)
        fprintf(fp, "Ityp %d\n", obj->item_type);
    if (obj->weight != obj->pIndexData->weight)
        fprintf(fp, "Wt   %d\n", obj->weight);
    if (obj->condition != obj->pIndexData->condition)
        fprintf(fp, "Cond %d\n", obj->condition);
    if (obj->enchanted_by != NULL)
        fprintf(fp, "EnchantedBy %s~\n", obj->enchanted_by);
    if (obj->wizard_mark != NULL)
        fprintf(fp, "WizardMark %s~\n", obj->wizard_mark);

    /* variable data */

    fprintf(fp, "Wear %d\n", obj->wear_loc);
    if (obj->level != obj->pIndexData->level)
        fprintf(fp, "Lev  %d\n", obj->level);
    if (obj->timer != 0)
        fprintf(fp, "Time %d\n", obj->timer);
    fprintf(fp, "Cost %d\n", obj->cost);
    if (obj->value[0] != obj->pIndexData->value[0]
        || obj->value[1] != obj->pIndexData->value[1]
        || obj->value[2] != obj->pIndexData->value[2]
        || obj->value[3] != obj->pIndexData->value[3]
        || obj->value[4] != obj->pIndexData->value[4])
        fprintf(fp, "Val  %d %d %d %d %d\n",
            obj->value[0], obj->value[1], obj->value[2], obj->value[3],
            obj->value[4]);

    switch (obj->item_type)
    {
        case ITEM_POTION:
        case ITEM_SCROLL:
        case ITEM_PILL:
            if (obj->value[1] > 0)
            {
                fprintf(fp, "Spell 1 '%s'\n",
                    skill_table[obj->value[1]]->name);
            }

            if (obj->value[2] > 0)
            {
                fprintf(fp, "Spell 2 '%s'\n",
                    skill_table[obj->value[2]]->name);
            }

            if (obj->value[3] > 0)
            {
                fprintf(fp, "Spell 3 '%s'\n",
                    skill_table[obj->value[3]]->name);
            }

            break;

        case ITEM_STAFF:
        case ITEM_WAND:
            if (obj->value[3] > 0)
            {
                fprintf(fp, "Spell 3 '%s'\n",
                    skill_table[obj->value[3]]->name);
            }

            break;
    }

    for (paf = obj->affected; paf != NULL; paf = paf->next)
    {
        if (paf->type < 0 || paf->type >= top_sn)
            continue;
        fprintf(fp, "Affc '%s' %3d %3d %3d %3d %3d %10d\n",
            skill_table[paf->type]->name,
            paf->where,
            paf->level,
            paf->duration, paf->modifier, paf->location, paf->bitvector);
    }

    for (ed = obj->extra_descr; ed != NULL; ed = ed->next)
    {
        fprintf(fp, "ExDe %s~ %s~\n", ed->keyword, ed->description);
    }

    fprintf(fp, "End\n\n");

    if (obj->contains != NULL)
        fwrite_obj(ch, obj->contains, fp, iNest + 1);

    return;
}

/*
 * Load a char and inventory into a new ch structure.
 */
bool load_char_obj(DESCRIPTOR_DATA * d, char *name)
{
    char strsave[MAX_INPUT_LENGTH];
    CHAR_DATA *ch;
    FILE *fp;
    bool found;
    int stat;

    ch = new_char();
    ch->pcdata = new_pcdata();

    d->character = ch;
    ch->desc = d;
    ch->name = str_dup(name);
    ch->id = get_pc_id();
    ch->race = race_lookup("human");
    ch->act = PLR_NOSUMMON;
    ch->comm = COMM_COMBINE | COMM_PROMPT;
    ch->prompt = str_dup("<%hhp %mm %vmv {g%r {x({c%e{x)>{x ");
    ch->pcdata->confirm_delete = FALSE;
    ch->pcdata->pwd = str_dup("");
    ch->pcdata->bamfin = str_dup("");
    ch->pcdata->bamfout = str_dup("");
    ch->pcdata->email = str_dup("");
    ch->pcdata->last_ip = str_dup("");
    ch->pcdata->title = str_dup("");
    for (stat = 0; stat < MAX_STATS; stat++)
        ch->perm_stat[stat] = 13;
    ch->pcdata->condition[COND_THIRST] = 48;
    ch->pcdata->condition[COND_FULL] = 48;
    ch->pcdata->condition[COND_HUNGER] = 48;
    ch->pcdata->security = 0;    /* OLC */
    ch->pcdata->is_reclassing = FALSE;
    ch->pcdata->pk_timer = 0;
    ch->pcdata->pkills = 0;
    ch->pcdata->pkills_arena = 0;
    ch->pcdata->pkilled = 0;
    ch->pcdata->bank_gold = 0;
    ch->pcdata->recall_vnum = 0;
    ch->pcdata->vnum_clairvoyance = 1;
    ch->pcdata->priest_rank = 0;
    ch->pcdata->deity = 0;
    ch->stance = STANCE_NORMAL;

    found = FALSE;
    fclose(fpReserve);

#if defined(unix)
    /* decompress if .gz file exists */
    sprintf(strsave, "%s%s%s", PLAYER_DIR, capitalize(name), ".gz");
    if ((fp = fopen(strsave, "r")) != NULL)
    {
        char buf[100];
        fclose(fp);
        sprintf(buf, "gzip -dfq %s", strsave);
        system(buf);
    }
#endif

    sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(name));
    if ((fp = fopen(strsave, "r")) != NULL)
    {
        int iNest;

        for (iNest = 0; iNest < MAX_NEST; iNest++)
            rgObjNest[iNest] = NULL;

        found = TRUE;
        for (;;)
        {
            char letter;
            char *word;

            letter = fread_letter(fp);
            if (letter == '*')
            {
                fread_to_eol(fp);
                continue;
            }

            if (letter != '#')
            {
                bug("Load_char_obj: # not found.", 0);
                break;
            }

            word = fread_word(fp);
            if (!str_cmp(word, "PLAYER"))
                fread_char(ch, fp);
            else if (!str_cmp(word, "OBJECT"))
                fread_obj(ch, fp);
            else if (!str_cmp(word, "O"))
                fread_obj(ch, fp);
            else if (!str_cmp(word, "PET"))
                fread_pet(ch, fp);
            else if (!str_cmp(word, "END"))
                break;
            else
            {
                bug("Load_char_obj: bad section.", 0);
                break;
            }
        }
        fclose(fp);
    }

    fpReserve = fopen(NULL_FILE, "r");


    /* initialize race */
    if (found)
    {
        int i;

        if (ch->race == 0)
        {
            if (!IS_NPC(ch))
            {
                // Log a message if the users race wasn't found.
                bugf("Player %s had a null race.", ch->name);
                wiznet("BUG: $N had a null race and was auto-set to human.", ch, NULL, WIZ_GENERAL, 0, 0);
                send_to_char("{RYour race was corrupted.  Please contact an immortal for resolution.{x\r\n", ch);
            }

            ch->race = race_lookup("human");
        }

        ch->size = pc_race_table[ch->race].size;
        ch->dam_type = 17;        /*punch */

        for (i = 0; i < 5; i++)
        {
            if (pc_race_table[ch->race].skills[i] == NULL)
                break;
            group_add(ch, pc_race_table[ch->race].skills[i], FALSE);
        }
        ch->affected_by = ch->affected_by | race_table[ch->race].aff;
        ch->imm_flags = ch->imm_flags | race_table[ch->race].imm;
        ch->res_flags = ch->res_flags | race_table[ch->race].res;
        ch->vuln_flags = ch->vuln_flags | race_table[ch->race].vuln;
        ch->form = race_table[ch->race].form;
        ch->parts = race_table[ch->race].parts;
    }


    // Handle versioning of pfiles.  This is commented out here because we have brought
    // our pfiles back to version 1.  This code from ROM doesn't need to be run.
    /*
    if (found && ch->version < 2)
    {
        // need to add the new skills
        group_add(ch, "rom basics", FALSE);
        ch->pcdata->learned[gsn_recall] = 50;
    }
    */

    return found;
}

/*
 * Read in a char.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )                 \
                if ( !str_cmp( word, literal ) )     \
                {                                    \
                    field  = value;                  \
                    fMatch = TRUE;                   \
                    break;                           \
                }

/* provided to free strings */
#if defined(KEYS)
#undef KEYS
#endif

#define KEYS( literal, field, value )                \
                if ( !str_cmp( word, literal ) )     \
                {                                    \
                    free_string(field);              \
                    field  = value;                  \
                    fMatch = TRUE;                   \
                    break;                           \
                }

void fread_char(CHAR_DATA * ch, FILE * fp)
{
    char buf[MAX_STRING_LENGTH];
    char *word;
    bool fMatch;
    int count = 0;
    int lastlogoff = current_time;
    int percent;

    sprintf(buf, "Loading player %s.", ch->name);
    log_string(buf);

    for (;;)
    {
        word = feof(fp) ? "End" : fread_word(fp);
        fMatch = FALSE;

        switch (UPPER(word[0]))
        {
            case '*':
                fMatch = TRUE;
                fread_to_eol(fp);
                break;

            case 'A':
                KEY("Act", ch->act, fread_flag(fp));
                KEY("AffectedBy", ch->affected_by, fread_flag(fp));
                KEY("AfBy", ch->affected_by, fread_flag(fp));

                KEY("Alignment", ch->alignment, fread_number(fp));
                KEY("Alig", ch->alignment, fread_number(fp));

                if (!str_cmp(word, "Alias"))
                {
                    if (count >= MAX_ALIAS)
                    {
                        fread_to_eol(fp);
                        fMatch = TRUE;
                        break;
                    }

                    ch->pcdata->alias[count] = str_dup(fread_word(fp));
                    ch->pcdata->alias_sub[count] = fread_string(fp);
                    count++;
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "AC") || !str_cmp(word, "Armor"))
                {
                    fread_to_eol(fp);
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "ACs"))
                {
                    int i;

                    for (i = 0; i < 4; i++)
                        ch->armor[i] = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "AffD"))
                {
                    AFFECT_DATA *paf;
                    int sn;

                    paf = new_affect();

                    sn = skill_lookup(fread_word(fp));
                    if (sn < 0)
                        bug("Fread_char: unknown skill.", 0);
                    else
                        paf->type = sn;

                    paf->level = fread_number(fp);
                    paf->duration = fread_number(fp);
                    paf->modifier = fread_number(fp);
                    paf->location = fread_number(fp);
                    paf->bitvector = fread_number(fp);
                    paf->next = ch->affected;
                    ch->affected = paf;
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "Affc") || !str_cmp(word, "Affect"))
                {
                    AFFECT_DATA *paf;
                    int sn;

                    paf = new_affect();

                    sn = skill_lookup(fread_word(fp));
                    if (sn < 0)
                        bug("Fread_char: unknown skill.", 0);
                    else
                        paf->type = sn;

                    paf->where = fread_number(fp);
                    paf->level = fread_number(fp);
                    paf->duration = fread_number(fp);
                    paf->modifier = fread_number(fp);
                    paf->location = fread_number(fp);
                    paf->bitvector = fread_number(fp);
                    paf->next = ch->affected;
                    ch->affected = paf;
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "AttrMod") || !str_cmp(word, "AMod") || !str_cmp(word, "AttributeModifier"))
                {
                    int stat;
                    for (stat = 0; stat < MAX_STATS; stat++)
                        ch->mod_stat[stat] = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "AttrPerm") || !str_cmp(word, "Attr") || !str_cmp(word, "Attributes"))
                {
                    int stat;

                    for (stat = 0; stat < MAX_STATS; stat++)
                        ch->perm_stat[stat] = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'B':
                KEY("Bamfin", ch->pcdata->bamfin, fread_string(fp));
                KEY("Bamfout", ch->pcdata->bamfout, fread_string(fp));
                KEY("Bin", ch->pcdata->bamfin, fread_string(fp));
                KEY("Bout", ch->pcdata->bamfout, fread_string(fp));
                KEY("BankGold", ch->pcdata->bank_gold, fread_number(fp));
                break;

            case 'C':
                KEY("Class", ch->class, fread_number(fp));
                KEY("Cla", ch->class, fread_number(fp));

                KEY("Clan", ch->clan, clan_lookup(fread_string(fp)));
                KEY("ClanFlags", ch->pcdata->clan_flags, fread_number(fp));
                KEY("ClanRank", ch->pcdata->clan_rank, clan_rank_lookup(fread_string(fp), ch));

                KEY("Comm", ch->comm, fread_flag(fp));

                if (!str_cmp(word, "Condition") || !str_cmp(word, "Cond") || !str_cmp(word, "Cnd"))
                {
                    ch->pcdata->condition[0] = fread_number(fp);
                    ch->pcdata->condition[1] = fread_number(fp);
                    ch->pcdata->condition[2] = fread_number(fp);
                    ch->pcdata->condition[3] = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }

                break;

            case 'D':
                KEY("Damroll", ch->damroll, fread_number(fp));
                KEY("Dam", ch->damroll, fread_number(fp));

                KEY("Description", ch->description, fread_string(fp));
                KEY("Desc", ch->description, fread_string(fp));

                KEY("Deity", ch->pcdata->deity, fread_number(fp));

                break;
            case 'E':
                if (!str_cmp(word, "End"))
                {
                    /* adjust hp mana move up  -- here for speed's sake */
                    percent =
                        (current_time - lastlogoff) * 25 / (2 * 60 * 60);

                    percent = UMIN(percent, 100);

                    if (percent > 0 && !IS_AFFECTED(ch, AFF_POISON)
                        && !IS_AFFECTED(ch, AFF_PLAGUE))
                    {
                        ch->hit += (ch->max_hit - ch->hit) * percent / 100;
                        ch->mana += (ch->max_mana - ch->mana) * percent / 100;
                        ch->move += (ch->max_move - ch->move) * percent / 100;
                    }

                    // Any sanity checks we need to provide here, we only have 3 alignments
                    // currently, report it, then fix it with the old ROM logic.  This code
                    // can be removed in the future. (TODO)
                    if (ch->alignment < 1 || ch->alignment > 3)
                    {
                        bugf("%s has an alignment of %d, fixing", ch->name, ch->alignment);


                        if (ch->alignment < -350)
                        {
                            ch->alignment = ALIGN_EVIL;
                        }
                        else if (ch->alignment > 350)
                        {
                            ch->alignment = ALIGN_GOOD;
                        }
                        else
                        {
                            ch->alignment = ALIGN_NEUTRAL;
                        }
                    }

                    sprintf(buf, "Player %s Loaded.", ch->name);
                    log_string(buf);

                    return;
                }

                KEY("Experience", ch->exp, fread_number(fp));
                KEY("Exp", ch->exp, fread_number(fp));

                KEY("Email", ch->pcdata->email, fread_string(fp));
                break;

            case 'G':
                KEY("Gold", ch->gold, fread_number(fp));
                if (!str_cmp(word, "Group") || !str_cmp(word, "Gr"))
                {
                    int gn;
                    char *temp;

                    temp = fread_word(fp);
                    gn = group_lookup(temp);
                    /* gn    = group_lookup( fread_word( fp ) ); */
                    if (gn < 0)
                    {
                        bug("Fread_char: unknown group. ", 0);
                        fprintf(stderr, "%s", temp);
                    }
                    else
                        gn_add(ch, gn);
                    fMatch = TRUE;
                }
                break;

            case 'H':
                KEY("Hitroll", ch->hitroll, fread_number(fp));
                KEY("Hit", ch->hitroll, fread_number(fp));

                if (!str_cmp(word, "HpManaMove") || !str_cmp(word, "HMV"))
                {
                    ch->hit = fread_number(fp);
                    ch->max_hit = fread_number(fp);
                    ch->mana = fread_number(fp);
                    ch->max_mana = fread_number(fp);
                    ch->move = fread_number(fp);
                    ch->max_move = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "HpManaMovePerm")
                    || !str_cmp(word, "HMVP"))
                {
                    ch->pcdata->perm_hit = fread_number(fp);
                    ch->pcdata->perm_mana = fread_number(fp);
                    ch->pcdata->perm_move = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }

                break;

            case 'I':
                KEY("Id", ch->id, fread_number(fp));

                KEY("InvisLevel", ch->invis_level, fread_number(fp));
                KEY("Invi", ch->invis_level, fread_number(fp));

                KEY("IncognitoLevel", ch->incog_level, fread_number(fp));
                KEY("Inco", ch->incog_level, fread_number(fp));

                KEY("ImproveFocus", ch->pcdata->improve_focus_gsn, skill_lookup(fread_string(fp)));
                KEY("ImproveMinutes", ch->pcdata->improve_minutes, fread_number(fp));

                break;
            case 'K':
                if (!str_cmp(word, "KeyRing"))
                {
                    int key;

                    for (key = 0; key < 10; key++)
                    {
                        ch->pcdata->key_ring[key] = fread_number(fp);
                    }

                    fMatch = TRUE;
                    break;
                }

                break;
            case 'L':
                KEY("LastLevel", ch->pcdata->last_level, fread_number(fp));
                KEY("LLev", ch->pcdata->last_level, fread_number(fp));

                KEY("Level", ch->level, fread_number(fp));
                KEY("Lev", ch->level, fread_number(fp));
                KEY("Levl", ch->level, fread_number(fp));

                KEY("LogO", lastlogoff, fread_number(fp));
                KEY("LogOffTime", lastlogoff, fread_number(fp));

                KEY("LongDescription", ch->long_descr, fread_string(fp));
                KEY("LongDescr", ch->long_descr, fread_string(fp));
                KEY("LnD", ch->long_descr, fread_string(fp));

                KEY("LastNote", ch->pcdata->last_note, fread_number(fp));
                KEY("LastPenalty", ch->pcdata->last_penalty, fread_number(fp));
                KEY("LastNews", ch->pcdata->last_news, fread_number(fp));
                KEY("LastChange", ch->pcdata->last_change, fread_number(fp));
                KEY("LastOoc", ch->pcdata->last_ooc, fread_number(fp));
                KEY("LastStoryNote", ch->pcdata->last_story, fread_number(fp));
                KEY("LastHistory", ch->pcdata->last_history, fread_number(fp));
                KEY("LastImmNote", ch->pcdata->last_immnote, fread_number(fp));
                KEY("LastIpAddress", ch->pcdata->last_ip, fread_string(fp));

                break;
            case 'M':
                KEY("Merit", ch->pcdata->merit, fread_flag(fp));
                break;
            case 'N':
                KEYS("Name", ch->name, fread_string(fp));
                break;

            case 'P':
                KEY("Password", ch->pcdata->pwd, fread_string(fp));
                KEY("Pass", ch->pcdata->pwd, fread_string(fp));

                KEY("Played", ch->played, fread_number(fp));
                KEY("Plyd", ch->played, fread_number(fp));

                KEY("Points", ch->pcdata->points, fread_number(fp));
                KEY("Pnts", ch->pcdata->points, fread_number(fp));

                KEY("Position", ch->position, fread_number(fp));
                KEY("Pos", ch->position, fread_number(fp));

                KEY("Practice", ch->practice, fread_number(fp));
                KEY("Prac", ch->practice, fread_number(fp));

                KEYS("Prompt", ch->prompt, fread_string(fp));
                KEY("Prom", ch->prompt, fread_string(fp));

                KEY("Pkilled", ch->pcdata->pkilled,fread_number(fp));
                KEY("Pkills", ch->pcdata->pkills, fread_number(fp));
                KEY("PkillsArena", ch->pcdata->pkills_arena, fread_number(fp));

                KEY("PriestRank", ch->pcdata->priest_rank, (fread_number(fp)));

                break;
            case 'Q':
                KEY("QuestPoints", ch->pcdata->quest_points, fread_number(fp));
                KEY("QuestNext", ch->pcdata->next_quest, fread_number(fp));
                KEY("QuestCount", ch->pcdata->countdown, fread_number(fp));
                KEY("QuestObj", ch->pcdata->quest_obj, fread_number(fp));
                KEY("QuestMob", ch->pcdata->quest_mob, fread_number(fp));

                if (!str_cmp(word, "QuestGiver"))
                {
                    ch->pcdata->quest_giver = get_quest_giver(fread_number(fp));
                    fMatch = TRUE;
                    break;
                }

                break;
            case 'R':
                KEY("Race", ch->race, race_lookup(fread_string(fp)));
                KEY("RecallVnum", ch->pcdata->recall_vnum, (fread_number(fp)));

                if (!str_cmp(word, "Room"))
                {
                    ch->in_room = get_room_index(fread_number(fp));
                    if (ch->in_room == NULL)
                        ch->in_room = get_room_index(ROOM_VNUM_LIMBO);
                    fMatch = TRUE;
                    break;
                }

                break;

            case 'S':
                KEY("SavingThrow", ch->saving_throw, fread_number(fp));
                KEY("Save", ch->saving_throw, fread_number(fp));

                KEY("Scroll", ch->lines, fread_number(fp));
                KEY("Scro", ch->lines, fread_number(fp));

                KEY("Sex", ch->sex, fread_number(fp));

                KEY("ShortDescription", ch->short_descr, fread_string(fp));
                KEY("ShortDescr", ch->short_descr, fread_string(fp));
                KEY("ShD", ch->short_descr, fread_string(fp));

                KEY("Sec", ch->pcdata->security, fread_number(fp));    /* OLC */
                KEY("Security", ch->pcdata->security, fread_number(fp));

                KEY("Silver", ch->silver, fread_number(fp));
                KEY("Silv", ch->silver, fread_number(fp));

                KEY("Stance", ch->stance, fread_number(fp));

                if (!str_cmp(word, "Skill") || !str_cmp(word, "Sk"))
                {
                    int sn;
                    int value;
                    char *temp;

                    value = fread_number(fp);
                    temp = fread_word(fp);
                    sn = skill_lookup(temp);

                    if (sn < 0)
                    {
                        bugf("Fread_char: unknown skill '%s' ", temp);
                    }

                    else
                        ch->pcdata->learned[sn] = value;
                    fMatch = TRUE;
                }

                break;

            case 'T':
                KEY("TrueSex", ch->pcdata->true_sex, fread_number(fp));
                KEY("TSex", ch->pcdata->true_sex, fread_number(fp));

                KEY("Train", ch->train, fread_number(fp));
                KEY("Trai", ch->train, fread_number(fp));

                KEY("Trust", ch->trust, fread_number(fp));
                KEY("Tru", ch->trust, fread_number(fp));

                if (!str_cmp(word, "Title") || !str_cmp(word, "Titl"))
                {
                    ch->pcdata->title = fread_string(fp);
                    if (ch->pcdata->title[0] != '.'
                        && ch->pcdata->title[0] != ','
                        && ch->pcdata->title[0] != '!'
                        && ch->pcdata->title[0] != '?')
                    {
                        sprintf(buf, " %s", ch->pcdata->title);
                        free_string(ch->pcdata->title);
                        ch->pcdata->title = str_dup(buf);
                    }
                    fMatch = TRUE;
                    break;
                }

                break;

            case 'V':
                KEY("Version", ch->version, fread_number(fp));
                KEY("Vers", ch->version, fread_number(fp));
                KEY("VnumClairvoyance", ch->pcdata->vnum_clairvoyance, fread_number(fp));

                if (!str_cmp(word, "Vnum"))
                {
                    ch->pIndexData = get_mob_index(fread_number(fp));
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'W':
                KEY("Wimpy", ch->wimpy, fread_number(fp));
                KEY("Wimp", ch->wimpy, fread_number(fp));

                KEY("Wiznet", ch->wiznet, fread_flag(fp));
                KEY("Wizn", ch->wiznet, fread_flag(fp));

                break;
        }

        if (!fMatch)
        {
            bug("Fread_char: no match.", 0);
            bug(word, 0);
            fread_to_eol(fp);
        }
    }

}

/* load a pet from the forgotten reaches */
void fread_pet(CHAR_DATA * ch, FILE * fp)
{
    char *word;
    CHAR_DATA *pet;
    bool fMatch;
    int lastlogoff = current_time;
    int percent;
    int vnum = 0;

    /* first entry had BETTER be the vnum or we barf */
    word = feof(fp) ? "END" : fread_word(fp);
    if (!str_cmp(word, "Vnum"))
    {

        vnum = fread_number(fp);
        if (get_mob_index(vnum) == NULL)
        {
            bug("Fread_pet: bad vnum %d.", vnum);
            pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
        }
        else
            pet = create_mobile(get_mob_index(vnum));
    }
    else
    {
        bug("Fread_pet: no vnum in file.", 0);
        pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
    }

    for (;;)
    {
        word = feof(fp) ? "END" : fread_word(fp);
        fMatch = FALSE;

        switch (UPPER(word[0]))
        {
            case '*':
                fMatch = TRUE;
                fread_to_eol(fp);
                break;

            case 'A':
                KEY("Act", pet->act, fread_flag(fp));
                KEY("AfBy", pet->affected_by, fread_flag(fp));

                KEY("Alignment", pet->alignment, fread_number(fp));
                KEY("Alig", pet->alignment, fread_number(fp));

                if (!str_cmp(word, "ACs"))
                {
                    int i;

                    for (i = 0; i < 4; i++)
                        pet->armor[i] = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "AffD"))
                {
                    AFFECT_DATA *paf;
                    int sn;

                    paf = new_affect();

                    sn = skill_lookup(fread_word(fp));
                    if (sn < 0)
                        bug("Fread_char: unknown skill.", 0);
                    else
                        paf->type = sn;

                    paf->level = fread_number(fp);
                    paf->duration = fread_number(fp);
                    paf->modifier = fread_number(fp);
                    paf->location = fread_number(fp);
                    paf->bitvector = fread_number(fp);
                    paf->next = pet->affected;
                    pet->affected = paf;
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "Affc") || !str_cmp(word, "Affect"))
                {
                    AFFECT_DATA *paf;
                    int sn;

                    paf = new_affect();

                    sn = skill_lookup(fread_word(fp));
                    if (sn < 0)
                        bug("Fread_char: unknown skill.", 0);
                    else
                        paf->type = sn;

                    paf->where = fread_number(fp);
                    paf->level = fread_number(fp);
                    paf->duration = fread_number(fp);
                    paf->modifier = fread_number(fp);
                    paf->location = fread_number(fp);
                    paf->bitvector = fread_number(fp);
                    /* Added here after Chris Litchfield (The Mage's Lair)
                     * pointed out a bug with duplicating affects in saved
                     * pets. -- JR 2002/01/31
                     */
                    if (!check_pet_affected(vnum, paf))
                    {
                        paf->next = pet->affected;
                        pet->affected = paf;
                    }
                    else {
                        free_affect(paf);
                    }
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "AMod") || !str_cmp(word, "AttributeModifier"))
                {
                    int stat;

                    for (stat = 0; stat < MAX_STATS; stat++)
                        pet->mod_stat[stat] = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "Attr") || !str_cmp(word, "Attributes"))
                {
                    int stat;

                    for (stat = 0; stat < MAX_STATS; stat++)
                        pet->perm_stat[stat] = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'C':
                KEY("Clan", pet->clan, clan_lookup(fread_string(fp)));
                KEY("Comm", pet->comm, fread_flag(fp));
                break;

            case 'D':
                KEY("Damroll", pet->damroll, fread_number(fp));
                KEY("Dam", pet->damroll, fread_number(fp));

                KEY("Description", pet->description, fread_string(fp));
                KEY("Desc", pet->description, fread_string(fp));

                break;
            case 'E':
                if (!str_cmp(word, "End"))
                {
                    pet->leader = ch;
                    pet->master = ch;
                    ch->pet = pet;
                    /* adjust hp mana move up  -- here for speed's sake */
                    percent =
                        (current_time - lastlogoff) * 25 / (2 * 60 * 60);

                    if (percent > 0 && !IS_AFFECTED(ch, AFF_POISON)
                        && !IS_AFFECTED(ch, AFF_PLAGUE))
                    {
                        percent = UMIN(percent, 100);
                        pet->hit += (pet->max_hit - pet->hit) * percent / 100;
                        pet->mana +=
                            (pet->max_mana - pet->mana) * percent / 100;
                        pet->move +=
                            (pet->max_move - pet->move) * percent / 100;
                    }
                    return;
                }

                KEY("Experience", pet->exp, fread_number(fp));
                KEY("Exp", pet->exp, fread_number(fp));

                break;

            case 'G':
                KEY("Gold", pet->gold, fread_number(fp));
                break;

            case 'H':
                KEY("Hit", pet->hitroll, fread_number(fp));
                KEY("Hitroll", pet->hitroll, fread_number(fp));

                if (!str_cmp(word, "HMV"))
                {
                    pet->hit = fread_number(fp);
                    pet->max_hit = fread_number(fp);
                    pet->mana = fread_number(fp);
                    pet->max_mana = fread_number(fp);
                    pet->move = fread_number(fp);
                    pet->max_move = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'L':
                KEY("Levl", pet->level, fread_number(fp));
                KEY("Level", pet->level, fread_number(fp));

                KEY("LongDescription", pet->long_descr, fread_string(fp));
                KEY("LnD", pet->long_descr, fread_string(fp));

                KEY("LogO", lastlogoff, fread_number(fp));
                KEY("LogOffTime", lastlogoff, fread_number(fp));
                break;

            case 'N':
                KEY("Name", pet->name, fread_string(fp));
                break;

            case 'P':
                KEY("Position", pet->position, fread_number(fp));
                KEY("Pos", pet->position, fread_number(fp));
                break;

            case 'R':
                KEY("Race", pet->race, race_lookup(fread_string(fp)));
                break;

            case 'S':
                KEY("Save", pet->saving_throw, fread_number(fp));
                KEY("Sex", pet->sex, fread_number(fp));

                KEY("ShD", pet->short_descr, fread_string(fp));
                KEY("ShortDescription", pet->short_descr, fread_string(fp));

                KEY("Silver", pet->silver, fread_number(fp));
                KEY("Silv", pet->silver, fread_number(fp));
                break;

                if (!fMatch)
                {
                    bug("Fread_pet: no match.", 0);
                    fread_to_eol(fp);
                }

        }
    }
}

extern OBJ_DATA *obj_free;

void fread_obj(CHAR_DATA * ch, FILE * fp)
{
    OBJ_DATA *obj;
    char *word;
    int iNest;
    bool fMatch;
    bool fNest;
    bool fVnum;
    bool first;
    bool new_format;      /* to prevent errors */
    int room_vnum = 1;    /* For saved objects to reload in a room */

    fVnum = FALSE;
    obj = NULL;
    first = TRUE;                /* used to counter fp offset */
    new_format = FALSE;

    word = feof(fp) ? "End" : fread_word(fp);
    if (!str_cmp(word, "Vnum"))
    {
        int vnum;
        first = FALSE;            /* fp will be in right place */

        vnum = fread_number(fp);
        if (get_obj_index(vnum) == NULL)
        {
            bug("Fread_obj: bad vnum %d.", vnum);
        }
        else
        {
            obj = create_object(get_obj_index(vnum));
            new_format = TRUE;
        }

    }

    if (obj == NULL)
    {                            /* either not found or old style */
        obj = new_obj();
        obj->name = str_dup("");
        obj->short_descr = str_dup("");
        obj->description = str_dup("");
        obj->wizard_mark = str_dup("");
        obj->enchanted_by = str_dup("");
    }

    obj->count = 1;
    fNest = TRUE;
    fVnum = TRUE;
    iNest = 0;

    for (;;)
    {
        if (first)
            first = FALSE;
        else
            word = feof(fp) ? "End" : fread_word(fp);
        fMatch = FALSE;

        switch (UPPER(word[0]))
        {
            case '*':
                fMatch = TRUE;
                fread_to_eol(fp);
                break;

            case 'A':
                if (!str_cmp(word, "AffD"))
                {
                    AFFECT_DATA *paf;
                    int sn;

                    paf = new_affect();

                    sn = skill_lookup(fread_word(fp));
                    if (sn < 0)
                        bug("Fread_obj: unknown skill.", 0);
                    else
                        paf->type = sn;

                    paf->level = fread_number(fp);
                    paf->duration = fread_number(fp);
                    paf->modifier = fread_number(fp);
                    paf->location = fread_number(fp);
                    paf->bitvector = fread_number(fp);
                    paf->next = obj->affected;
                    obj->affected = paf;
                    fMatch = TRUE;
                    break;
                }
                if (!str_cmp(word, "Affc"))
                {
                    AFFECT_DATA *paf;
                    int sn;

                    paf = new_affect();

                    sn = skill_lookup(fread_word(fp));
                    if (sn < 0)
                        bug("Fread_obj: unknown skill.", 0);
                    else
                        paf->type = sn;

                    paf->where = fread_number(fp);
                    paf->level = fread_number(fp);
                    paf->duration = fread_number(fp);
                    paf->modifier = fread_number(fp);
                    paf->location = fread_number(fp);
                    paf->bitvector = fread_number(fp);
                    paf->next = obj->affected;
                    obj->affected = paf;
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'C':
                KEY("Cond", obj->condition, fread_number(fp));
                KEY("Cost", obj->cost, fread_number(fp));
                KEY("Count", obj->count, fread_number(fp));

                break;

            case 'D':
                KEY("Description", obj->description, fread_string(fp));
                KEY("Desc", obj->description, fread_string(fp));
                break;

            case 'E':

                if (!str_cmp(word, "Enchanted"))
                {
                    obj->enchanted = TRUE;
                    fMatch = TRUE;
                    break;
                }

                KEY("ExtraFlags", obj->extra_flags, fread_number(fp));
                KEY("ExtF", obj->extra_flags, fread_number(fp));
                KEY("EnchantedBy", obj->enchanted_by, fread_string(fp));

                if (!str_cmp(word, "ExtraDescr") || !str_cmp(word, "ExDe"))
                {
                    EXTRA_DESCR_DATA *ed;

                    ed = new_extra_descr();

                    ed->keyword = fread_string(fp);
                    ed->description = fread_string(fp);
                    ed->next = obj->extra_descr;
                    obj->extra_descr = ed;
                    fMatch = TRUE;
                }

                if (!str_cmp(word, "End"))
                {
                    if (!fNest || (fVnum && obj->pIndexData == NULL))
                    {
                        bug("Fread_obj: incomplete object.", 0);
                        free_obj(obj);
                        return;
                    }
                    else
                    {
                        if (!fVnum)
                        {
                            free_obj(obj);
                            obj = create_object(get_obj_index(OBJ_VNUM_DUMMY));
                        }

                        if (!new_format)
                        {
                            obj->next = object_list;
                            object_list = obj;
                            obj->pIndexData->count++;
                        }

                        if (fNest)        /* NEW NEST CODE */
                            rgObjNest[iNest] = obj;


                        if (iNest == 0 || rgObjNest[iNest] == NULL)
                        {
                            if (ch)
                            {
                                obj = obj_to_char(obj, ch);
                            }
                            else if (room_vnum != 1)
                            {
                                obj = obj_to_room(obj, get_room_index(room_vnum));
                            }
                        }
                        else
                        {
                            if (rgObjNest[iNest - 1]) /* NEW NEST CODE */
                            {
                                separate_obj(rgObjNest[iNest - 1]);
                                obj = obj_to_obj(obj, rgObjNest[iNest - 1]);
                            }
                            else
                            {
                                if (ch)
                                {
                                    obj_to_char(obj, ch);
                                }
                                else if (room_vnum != 1)
                                {
                                    obj = obj_to_room(obj, get_room_index(room_vnum));
                                }
                            }

                        }

                        if (fNest) /* NEW NEST CODE */
                            rgObjNest[iNest] = obj;

                        return;
                    }
                }
                break;
            case 'I':
                KEY("ItemType", obj->item_type, fread_number(fp));
                KEY("Ityp", obj->item_type, fread_number(fp));
                break;

            case 'L':
                KEY("Level", obj->level, fread_number(fp));
                KEY("Lev", obj->level, fread_number(fp));
                break;

            case 'N':
                KEY("Name", obj->name, fread_string(fp));

                if (!str_cmp(word, "Nest")) /* NEW NEST CODE */
                {
                    iNest = fread_number(fp);
                    if (iNest < 0 || iNest >= MAX_NEST)
                    {
                        bug("Fread_obj: bad nest %d.", iNest);
                        iNest = 0;
                        fNest = FALSE;
                    }
                    fMatch = TRUE;
                }

                break;
            case 'R':
                KEY("RoomVnum", room_vnum, fread_number(fp));
            case 'S':
                KEY("ShortDescr", obj->short_descr, fread_string(fp));
                KEY("ShD", obj->short_descr, fread_string(fp));

                if (!str_cmp(word, "Spell"))
                {
                    int iValue;
                    int sn;

                    iValue = fread_number(fp);
                    sn = skill_lookup(fread_word(fp));
                    if (iValue < 0 || iValue > 3)
                    {
                        bug("Fread_obj: bad iValue %d.", iValue);
                    }
                    else if (sn < 0)
                    {
                        bug("Fread_obj: unknown skill.", 0);
                    }
                    else
                    {
                        obj->value[iValue] = sn;
                    }
                    fMatch = TRUE;
                    break;
                }

                break;

            case 'T':
                KEY("Timer", obj->timer, fread_number(fp));
                KEY("Time", obj->timer, fread_number(fp));
                break;

            case 'V':
                if (!str_cmp(word, "Values") || !str_cmp(word, "Vals"))
                {
                    obj->value[0] = fread_number(fp);
                    obj->value[1] = fread_number(fp);
                    obj->value[2] = fread_number(fp);
                    obj->value[3] = fread_number(fp);
                    if (obj->item_type == ITEM_WEAPON && obj->value[0] == 0)
                        obj->value[0] = obj->pIndexData->value[0];
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "Val"))
                {
                    obj->value[0] = fread_number(fp);
                    obj->value[1] = fread_number(fp);
                    obj->value[2] = fread_number(fp);
                    obj->value[3] = fread_number(fp);
                    obj->value[4] = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }

                if (!str_cmp(word, "Vnum"))
                {
                    int vnum;

                    vnum = fread_number(fp);
                    if ((obj->pIndexData = get_obj_index(vnum)) == NULL)
                        bug("Fread_obj: bad vnum %d.", vnum);
                    else
                        fVnum = TRUE;
                    fMatch = TRUE;
                    break;
                }
                break;

            case 'W':
                KEY("WearFlags", obj->wear_flags, fread_number(fp));
                KEY("WeaF", obj->wear_flags, fread_number(fp));
                KEY("WearLoc", obj->wear_loc, fread_number(fp));
                KEY("Wear", obj->wear_loc, fread_number(fp));
                KEY("Weight", obj->weight, fread_number(fp));
                KEY("Wt", obj->weight, fread_number(fp));
                KEY("WizardMark", obj->wizard_mark, fread_string(fp));
                break;

        }

        if (!fMatch)
        {
            bug("Fread_obj: no match.", 0);
            fread_to_eol(fp);
        }
    }
}

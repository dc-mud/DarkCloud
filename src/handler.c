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

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"

/*
 * Local functions.
 */
void affect_modify(CHAR_DATA * ch, AFFECT_DATA * paf, bool fAdd);
OBJ_DATA *group_object(OBJ_DATA *obj1, OBJ_DATA *obj2);

/*
 * Returns number of people on an object.
 */
int count_users(OBJ_DATA * obj)
{
    CHAR_DATA *fch;
    int count = 0;

    if (obj->in_room == NULL)
        return 0;

    for (fch = obj->in_room->people; fch != NULL; fch = fch->next_in_room)
    {
        if (fch->on == obj)
            count++;
    }

    return count;
}

/*
 * TODO - Returns material number (do with nature/crafting code)
 */
int material_lookup(const char *name)
{
    return 0;
}

/*
 * Returns the weapon index from the weapon_type table.
 */
int weapon_lookup(const char *name)
{
    int type;

    for (type = 0; weapon_table[type].name != NULL; type++)
    {
        if (LOWER(name[0]) == LOWER(weapon_table[type].name[0])
            && !str_prefix(name, weapon_table[type].name))
            return type;
    }

    return -1;
}

/*
 * Returns the weapon type from the weapon type table (e.g. WEAPON_SWORD, etc.)
 */
int weapon_type_lookup(const char *name)
{
    int type;

    for (type = 0; weapon_table[type].name != NULL; type++)
    {
        if (LOWER(name[0]) == LOWER(weapon_table[type].name[0])
            && !str_prefix(name, weapon_table[type].name))
            return weapon_table[type].type;
    }

    return WEAPON_EXOTIC;
}

/*
 * Returns the name of the weapon provided the int value.  (e.g. sword, dagger, etc.)
 */
char *weapon_name(int weapon_type)
{
    int type;

    for (type = 0; weapon_table[type].name != NULL; type++)
        if (weapon_type == weapon_table[type].type)
            return weapon_table[type].name;
    return "exotic";
}

/*
 * Returns the name of the item type provided the index (e.g. light, scroll,
 * weapon, food, portal, gem, foundation, pc_corpse, etc.)
 */
char *item_name(int item_type)
{
    int type;

    for (type = 0; item_table[type].name != NULL; type++)
        if (item_type == item_table[type].type)
            return item_table[type].name;
    return "none";
}

/*
 * Returns the index in the table of the attack based on the name (e.g. slash,
 * pound, punch, slap, cleave, stab, slice, etc.)
 */
int attack_lookup(const char *name)
{
    int att;

    for (att = 0; attack_table[att].name != NULL; att++)
    {
        if (LOWER(name[0]) == LOWER(attack_table[att].name[0])
            && !str_prefix(name, attack_table[att].name))
            return att;
    }

    return 0;
}

/*
 * Returns the index in the table for the specified wiznet command.
 */
long wiznet_lookup(const char *name)
{
    int flag;

    for (flag = 0; wiznet_table[flag].name != NULL; flag++)
    {
        if (LOWER(name[0]) == LOWER(wiznet_table[flag].name[0])
            && !str_prefix(name, wiznet_table[flag].name))
            return flag;
    }

    return -1;
}

/*
 * Returns the class index for the specified class name.
 */
int class_lookup(const char *name)
{
    int class;
    extern int top_class;

    for (class = 0; class < top_class; class++)
    {
        if (LOWER(name[0]) == LOWER(class_table[class]->name[0])
            && !str_prefix(name, class_table[class]->name))
            return class;
    }

    return -1;
}

/*
 * for immunity, vulnerabiltiy, and resistant
 * the 'globals' (magic and weapons) may be overriden
 * three other cases -- wood, silver, and iron -- are checked in fight.c
 */
int check_immune(CHAR_DATA * ch, int dam_type)
{
    int immune, def;
    int bit;

    immune = -1;
    def = IS_NORMAL;

    if (dam_type == DAM_NONE)
        return immune;

    if (dam_type <= 3)
    {
        if (IS_SET(ch->imm_flags, IMM_WEAPON))
            def = IS_IMMUNE;
        else if (IS_SET(ch->res_flags, RES_WEAPON))
            def = IS_RESISTANT;
        else if (IS_SET(ch->vuln_flags, VULN_WEAPON))
            def = IS_VULNERABLE;
    }
    else
    {
        /* magical attack */
        if (IS_SET(ch->imm_flags, IMM_MAGIC))
            def = IS_IMMUNE;
        else if (IS_SET(ch->res_flags, RES_MAGIC))
            def = IS_RESISTANT;
        else if (IS_SET(ch->vuln_flags, VULN_MAGIC))
            def = IS_VULNERABLE;
    }

    /* set bits to check -- VULN etc. must ALL be the same or this will fail */
    switch (dam_type)
    {
        case (DAM_BASH) :
            bit = IMM_BASH;
            break;
        case (DAM_PIERCE) :
            bit = IMM_PIERCE;
            break;
        case (DAM_SLASH) :
            bit = IMM_SLASH;
            break;
        case (DAM_FIRE) :
            bit = IMM_FIRE;
            break;
        case (DAM_COLD) :
            bit = IMM_COLD;
            break;
        case (DAM_LIGHTNING) :
            bit = IMM_LIGHTNING;
            break;
        case (DAM_ACID) :
            bit = IMM_ACID;
            break;
        case (DAM_POISON) :
            bit = IMM_POISON;
            break;
        case (DAM_NEGATIVE) :
            bit = IMM_NEGATIVE;
            break;
        case (DAM_HOLY) :
            bit = IMM_HOLY;
            break;
        case (DAM_ENERGY) :
            bit = IMM_ENERGY;
            break;
        case (DAM_MENTAL) :
            bit = IMM_MENTAL;
            break;
        case (DAM_DISEASE) :
            bit = IMM_DISEASE;
            break;
        case (DAM_DROWNING) :
            bit = IMM_DROWNING;
            break;
        case (DAM_LIGHT) :
            bit = IMM_LIGHT;
            break;
        case (DAM_CHARM) :
            bit = IMM_CHARM;
            break;
        case (DAM_SOUND) :
            bit = IMM_SOUND;
            break;
        default:
            return def;
    }

    if (IS_SET(ch->imm_flags, bit))
        immune = IS_IMMUNE;
    else if (IS_SET(ch->res_flags, bit) && immune != IS_IMMUNE)
        immune = IS_RESISTANT;
    else if (IS_SET(ch->vuln_flags, bit))
    {
        if (immune == IS_IMMUNE)
            immune = IS_RESISTANT;
        else if (immune == IS_RESISTANT)
            immune = IS_NORMAL;
        else
            immune = IS_VULNERABLE;
    }

    if (immune == -1)
        return def;
    else
        return immune;
}

/*
 * Returns the weapon sn (skill number) for the weapon that the character
 * is currently wearing (either for the primary weapon or a secondary (dual)
 * wielded weapon.  e.g. this will return gsn_sword, gsn_dagger, gsn_mace, etc.
 */
int get_weapon_sn(CHAR_DATA * ch, bool dual)
{
    OBJ_DATA *wield;
    int sn;

    if (!dual)
    {
        wield = get_eq_char(ch, WEAR_WIELD);
    }
    else
    {
        wield = get_eq_char(ch, WEAR_SECONDARY_WIELD);
    }

    if (wield == NULL || wield->item_type != ITEM_WEAPON)
    {
        sn = gsn_hand_to_hand;
    }
    else
    {
        switch (wield->value[0])
        {
            default:
                sn = -1;
                break;
            case (WEAPON_SWORD) :
                sn = gsn_sword;
                break;
            case (WEAPON_DAGGER) :
                sn = gsn_dagger;
                break;
            case (WEAPON_SPEAR) :
                sn = gsn_spear;
                break;
            case (WEAPON_MACE) :
                sn = gsn_mace;
                break;
            case (WEAPON_AXE) :
                sn = gsn_axe;
                break;
            case (WEAPON_FLAIL) :
                sn = gsn_flail;
                break;
            case (WEAPON_WHIP) :
                sn = gsn_whip;
                break;
            case (WEAPON_POLEARM) :
                sn = gsn_polearm;
                break;
        }
    }
    return sn;
}

/*
 * Returns the proficiency % for the specified sn (skill number).  e.g. for a player
 * passing in gsn_sword would give the % learned that player is with swords.
 */
int get_weapon_skill(CHAR_DATA * ch, int sn)
{
    int skill;

    // -1 is exotic
    if (IS_NPC(ch))
    {
        // Mobiles
        if (sn == -1)
            skill = 3 * ch->level;
        else if (sn == gsn_hand_to_hand)
            skill = 40 + 2 * ch->level;
        else
            skill = 40 + 5 * ch->level / 2;
    }
    else
    {
        // Players
        if (sn == -1)
            skill = 3 * ch->level;
        else
            skill = ch->pcdata->learned[sn];
    }

    return URANGE(0, skill, 100);
}


/*
 * Used to de-screw characters
 */
void reset_char(CHAR_DATA * ch)
{
    int loc, mod, stat;
    OBJ_DATA *obj;
    AFFECT_DATA *af;
    int i;

    if (IS_NPC(ch))
        return;

    if (ch->pcdata->perm_hit == 0
        || ch->pcdata->perm_mana == 0
        || ch->pcdata->perm_move == 0)
    {
        // If we're doing a full reset we're going to strip all affects first.
        affect_strip_all(ch);

        /* do a FULL reset */
        for (loc = 0; loc < MAX_WEAR; loc++)
        {
            obj = get_eq_char(ch, loc);
            if (obj == NULL)
                continue;
            if (!obj->enchanted)
                for (af = obj->pIndexData->affected; af != NULL; af = af->next)
            {
                mod = af->modifier;
                switch (af->location)
                {
                    case APPLY_SEX:
                        ch->sex -= mod;
                        if (ch->sex < 0 || ch->sex > 2)
                            ch->sex =
                            IS_NPC(ch) ? 0 : ch->pcdata->true_sex;
                        break;
                    case APPLY_MANA:
                        ch->max_mana -= mod;
                        break;
                    case APPLY_HIT:
                        ch->max_hit -= mod;
                        break;
                    case APPLY_MOVE:
                        ch->max_move -= mod;
                        break;
                }
            }

            for (af = obj->affected; af != NULL; af = af->next)
            {
                mod = af->modifier;
                switch (af->location)
                {
                    case APPLY_SEX:
                        ch->sex -= mod;
                        break;
                    case APPLY_MANA:
                        ch->max_mana -= mod;
                        break;
                    case APPLY_HIT:
                        ch->max_hit -= mod;
                        break;
                    case APPLY_MOVE:
                        ch->max_move -= mod;
                        break;
                }
            }
        }
        /* now reset the permanent stats */
        ch->pcdata->perm_hit = ch->max_hit;
        ch->pcdata->perm_mana = ch->max_mana;
        ch->pcdata->perm_move = ch->max_move;
        ch->pcdata->last_level = ch->played / 3600;
        if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2)
        {
            if (ch->sex > 0 && ch->sex < 3)
                ch->pcdata->true_sex = ch->sex;
            else
                ch->pcdata->true_sex = 0;
        }

    }

    /* now restore the character to his/her true condition */
    for (stat = 0; stat < MAX_STATS; stat++)
        ch->mod_stat[stat] = 0;

    if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2)
        ch->pcdata->true_sex = 0;
    ch->sex = ch->pcdata->true_sex;
    ch->max_hit = ch->pcdata->perm_hit;
    ch->max_mana = ch->pcdata->perm_mana;
    ch->max_move = ch->pcdata->perm_move;

    for (i = 0; i < 4; i++)
        ch->armor[i] = 100;

    ch->hitroll = 0;
    ch->damroll = 0;
    ch->saving_throw = 0;

    /* now start adding back the effects */
    for (loc = 0; loc < MAX_WEAR; loc++)
    {
        obj = get_eq_char(ch, loc);
        if (obj == NULL)
            continue;
        for (i = 0; i < 4; i++)
            ch->armor[i] -= apply_ac(obj, loc, i);

        if (!obj->enchanted)
            for (af = obj->pIndexData->affected; af != NULL; af = af->next)
            {
                mod = af->modifier;
                switch (af->location)
                {
                    case APPLY_STR:
                        ch->mod_stat[STAT_STR] += mod;
                        break;
                    case APPLY_DEX:
                        ch->mod_stat[STAT_DEX] += mod;
                        break;
                    case APPLY_INT:
                        ch->mod_stat[STAT_INT] += mod;
                        break;
                    case APPLY_WIS:
                        ch->mod_stat[STAT_WIS] += mod;
                        break;
                    case APPLY_CON:
                        ch->mod_stat[STAT_CON] += mod;
                        break;
                    case APPLY_SEX:
                        ch->sex += mod;
                        break;
                    case APPLY_MANA:
                        ch->max_mana += mod;
                        break;
                    case APPLY_HIT:
                        ch->max_hit += mod;
                        break;
                    case APPLY_MOVE:
                        ch->max_move += mod;
                        break;
                    case APPLY_AC:
                        for (i = 0; i < 4; i++)
                        {
                            ch->armor[i] += mod;
                        }
                        break;
                    case APPLY_HITROLL:
                        // Add the hitroll if it's not on the weapon.. the weapons will have
                        // to be added individually depending on which is in use at the time
                        // damage is given.
                        if (obj->wear_loc != WEAR_WIELD && obj->wear_loc != WEAR_SECONDARY_WIELD)
                        {
                            ch->hitroll += mod;
                        }

                        break;
                    case APPLY_DAMROLL:
                        // Add the damroll if it's not on the weapon.. the weapons will have
                        // to be added individually depending on which is in use at the time
                        // damage is given.
                        if (obj->wear_loc != WEAR_WIELD && obj->wear_loc != WEAR_SECONDARY_WIELD)
                        {
                            ch->damroll += mod;
                        }

                        break;
                    case APPLY_SAVES:
                        ch->saving_throw += mod;
                        break;
                }
            }

        for (af = obj->affected; af != NULL; af = af->next)
        {
            mod = af->modifier;
            switch (af->location)
            {
                case APPLY_STR:
                    ch->mod_stat[STAT_STR] += mod;
                    break;
                case APPLY_DEX:
                    ch->mod_stat[STAT_DEX] += mod;
                    break;
                case APPLY_INT:
                    ch->mod_stat[STAT_INT] += mod;
                    break;
                case APPLY_WIS:
                    ch->mod_stat[STAT_WIS] += mod;
                    break;
                case APPLY_CON:
                    ch->mod_stat[STAT_CON] += mod;
                    break;
                case APPLY_SEX:
                    ch->sex += mod;
                    break;
                case APPLY_MANA:
                    ch->max_mana += mod;
                    break;
                case APPLY_HIT:
                    ch->max_hit += mod;
                    break;
                case APPLY_MOVE:
                    ch->max_move += mod;
                    break;
                case APPLY_AC:
                    for (i = 0; i < 4; i++)
                    {
                        ch->armor[i] += mod;
                    }
                    break;
                case APPLY_HITROLL:
                    // Add the hitroll if it's not on the weapon.. the weapons will have
                    // to be added individually depending on which is in use at the time
                    // damage is given.
                    if (obj->wear_loc != WEAR_WIELD && obj->wear_loc != WEAR_SECONDARY_WIELD)
                    {
                        ch->hitroll += mod;
                    }

                    break;
                case APPLY_DAMROLL:
                    // Add the damroll if it's not on the weapon.. the weapons will have
                    // to be added individually depending on which is in use at the time
                    // damage is given.
                    if (obj->wear_loc != WEAR_WIELD && obj->wear_loc != WEAR_SECONDARY_WIELD)
                    {
                        ch->damroll += mod;
                    }

                    break;
                case APPLY_SAVES:
                    ch->saving_throw += mod;
                    break;
            }
        }
    }

    /* now add back spell effects */
    for (af = ch->affected; af != NULL; af = af->next)
    {
        mod = af->modifier;
        switch (af->location)
        {
            case APPLY_STR:
                ch->mod_stat[STAT_STR] += mod;
                break;
            case APPLY_DEX:
                ch->mod_stat[STAT_DEX] += mod;
                break;
            case APPLY_INT:
                ch->mod_stat[STAT_INT] += mod;
                break;
            case APPLY_WIS:
                ch->mod_stat[STAT_WIS] += mod;
                break;
            case APPLY_CON:
                ch->mod_stat[STAT_CON] += mod;
                break;
            case APPLY_SEX:
                ch->sex += mod;
                break;
            case APPLY_MANA:
                ch->max_mana += mod;
                break;
            case APPLY_HIT:
                ch->max_hit += mod;
                break;
            case APPLY_MOVE:
                ch->max_move += mod;
                break;
            case APPLY_AC:
                for (i = 0; i < 4; i++)
                {
                    ch->armor[i] += mod;
                }
                break;
            case APPLY_HITROLL:
                ch->hitroll += mod;
                break;
            case APPLY_DAMROLL:
                ch->damroll += mod;
                break;
            case APPLY_SAVES:
                ch->saving_throw += mod;
                break;
        }
    }

    /* make sure sex is RIGHT!!!! */
    if (ch->sex < 0 || ch->sex > 2)
    {
        ch->sex = ch->pcdata->true_sex;
    }

}

/*
 * Retrieve a character's trusted level for permission checking.
 */
int get_trust(CHAR_DATA * ch)
{
    if (ch->desc != NULL && ch->desc->original != NULL)
        ch = ch->desc->original;

    if (ch->trust)
        return ch->trust;

    if (IS_NPC(ch) && ch->level >= LEVEL_HERO)
        return LEVEL_HERO - 1;
    else
        return ch->level;
}

/*
 * Retrieve a character's age.
 */
int get_age(CHAR_DATA * ch)
{
    return 17 + (ch->played + (int)(current_time - ch->logon)) / 72000;
}

/*
 * Returns the current value of the requested stat after modifications by spells,
 * equipment, etc.
 */
int get_curr_stat(CHAR_DATA * ch, int stat)
{
    int max;

    if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
    {
        max = 25;
    }
    else
    {
        // Stock ROM allowed players to go above their max stat by 4 with spells, this
        // is now a setting with 0 being the default.  Everyone getting into the 20's
        // for stats makes much less diversity.. spells are still useful without this
        // because they can help protect against maladictions.  Game admin can then set
        // this to their liking.
        max = pc_race_table[ch->race].max_stats[stat] + settings.stat_surge;

        // This is legit, the primary stat for your class gets you a +2 bonus over
        // your racial max.
        if (class_table[ch->class]->attr_prime == stat && ch->race == race_lookup("human"))
        {
            max += 3;
        }
        else if (class_table[ch->class]->attr_prime == stat)
        {
            max +=2;
        }


        // Secondary stat for the class
        if (class_table[ch->class]->attr_second == stat)
        {
            max += 1;
        }

        // Merit - Constitution, Intelligence, Wisdom (these up the max by 1)
        if ((IS_SET(ch->pcdata->merit, MERIT_CONSTITUTION) && stat == STAT_CON)
            || (IS_SET(ch->pcdata->merit, MERIT_INTELLIGENCE) && stat == STAT_INT)
            || (IS_SET(ch->pcdata->merit, MERIT_STRENGTH) && stat == STAT_STR)
            || (IS_SET(ch->pcdata->merit, MERIT_DEXTERITY) && stat == STAT_DEX)
            || (IS_SET(ch->pcdata->merit, MERIT_WISDOM) && stat == STAT_WIS))
        {
            max += 1;
        }

        max = UMIN(max, 25);
    }

    return URANGE(3, ch->perm_stat[stat] + ch->mod_stat[stat], max);
}

/*
 * Returns the maximum number a stat can be trained to for the race.  If it's the
 * primary stat for the class then they are afforded a bonus, 2 for most classes
 * and 3 for humans.
 */
int get_max_train(CHAR_DATA * ch, int stat)
{
    int max;

    if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
    {
        return 25;
    }

    // Get the max stats for the players race for the requested stat
    max = pc_race_table[ch->race].max_stats[stat];

    // Modify based off of the what the player's classes primary stat is
    if (class_table[ch->class]->attr_prime == stat)
    {
        if (ch->race == race_lookup("human"))
        {
            max += 3;
        }
        else
        {
            max += 2;
        }
    }

    if (class_table[ch->class]->attr_second == stat)
    {
        max += 1;
    }

    // Merit - Constitution, Intelligence, Wisdom (these up the max by 1)
    if ((IS_SET(ch->pcdata->merit, MERIT_CONSTITUTION) && stat == STAT_CON)
        || (IS_SET(ch->pcdata->merit, MERIT_INTELLIGENCE) && stat == STAT_INT)
        || (IS_SET(ch->pcdata->merit, MERIT_STRENGTH) && stat == STAT_STR)
        || (IS_SET(ch->pcdata->merit, MERIT_DEXTERITY) && stat == STAT_DEX)
        || (IS_SET(ch->pcdata->merit, MERIT_WISDOM) && stat == STAT_WIS))
    {
        max += 1;
    }

    return UMIN(max, 25);
}

/*
 * Retrieve a character's carry capacity.
 */
int can_carry_n(CHAR_DATA * ch)
{
    if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL)
        return 1000;

    if (IS_NPC(ch) && IS_SET(ch->act, ACT_PET))
        return 0;

    return MAX_WEAR + 2 * get_curr_stat(ch, STAT_DEX) + ch->level;
}

/*
 * Retrieve a character's carry capacity.
 */
int can_carry_w(CHAR_DATA * ch)
{
    if (!IS_NPC(ch) && IS_IMMORTAL(ch))
        return 10000000;

    if (IS_NPC(ch) && IS_SET(ch->act, ACT_PET))
        return 0;

    return str_app[get_curr_stat(ch, STAT_STR)].carry * 10 + ch->level * 25;
}

/*
 * See if a string is one of the names of an object using a partial match.  This can be
 * used to see if a string is contained in a list of other strings.  This uses str_prefix and
 * will return partial matches (e.g. searching for Isa should return true for Isaac).
 */
bool is_name(char *str, char *namelist)
{
    char name[MAX_INPUT_LENGTH], part[MAX_INPUT_LENGTH];
    char *list, *string;

    /* fix crash on NULL namelist */
    if (namelist == NULL || namelist[0] == '\0')
        return FALSE;

    /* fixed to prevent is_name on "" returning TRUE */
    if (str[0] == '\0')
        return FALSE;

    string = str;
    /* we need ALL parts of string to match part of namelist */
    for (;;)
    {                            /* start parsing string */
        str = one_argument(str, part);

        if (part[0] == '\0')
            return TRUE;

        /* check to see if this is part of namelist */
        list = namelist;
        for (;;)
        {                        /* start parsing namelist */
            list = one_argument(list, name);
            if (name[0] == '\0')    /* this name was not found */
                return FALSE;

            if (!str_prefix(string, name))
                return TRUE;    /* full pattern match */

            if (!str_prefix(part, name))
                break;
        }
    }
}

/*
 * See if a string is one of the names of an object using an exact match.  This can be
 * used to see if a string is contained in a list of other strings.  This uses str_cmp and
 * will return true only if a full match is found.
 */
bool is_exact_name(char *str, char *namelist)
{
    char name[MAX_INPUT_LENGTH];

    if (namelist == NULL)
        return FALSE;

    for (;;)
    {
        namelist = one_argument(namelist, name);
        if (name[0] == '\0')
            return FALSE;
        if (!str_cmp(str, name))
            return TRUE;
    }
}

/*
 * Not sure what this is intended to do in the grand scheme (it's only called from
 * the acid effect code when it pits an item.  This is slotting in a new affect.
 */
void affect_enchant(OBJ_DATA * obj)
{
    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted)
    {
        AFFECT_DATA *paf, *af_new;
        obj->enchanted = TRUE;

        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
        {
            af_new = new_affect();

            af_new->next = obj->affected;
            obj->affected = af_new;

            af_new->where = paf->where;
            af_new->type = UMAX(0, paf->type);
            af_new->level = paf->level;
            af_new->duration = paf->duration;
            af_new->location = paf->location;
            af_new->modifier = paf->modifier;
            af_new->bitvector = paf->bitvector;
        }
    }
}

/*
 * Apply or remove an affect to a character.  This is a lower level  method
 * used by affect_strip, affect_to_char, affect_strip_all, affect_remove, etc.
 */
void affect_modify(CHAR_DATA * ch, AFFECT_DATA * paf, bool fAdd)
{
    OBJ_DATA *wield;
    int mod, i;

    mod = paf->modifier;

    if (fAdd)
    {
        switch (paf->where)
        {
            case TO_AFFECTS:
                SET_BIT(ch->affected_by, paf->bitvector);
                break;
            case TO_IMMUNE:
                SET_BIT(ch->imm_flags, paf->bitvector);
                break;
            case TO_RESIST:
                SET_BIT(ch->res_flags, paf->bitvector);
                break;
            case TO_VULN:
                SET_BIT(ch->vuln_flags, paf->bitvector);
                break;
        }
    }
    else
    {
        switch (paf->where)
        {
            case TO_AFFECTS:
                REMOVE_BIT(ch->affected_by, paf->bitvector);
                break;
            case TO_IMMUNE:
                REMOVE_BIT(ch->imm_flags, paf->bitvector);
                break;
            case TO_RESIST:
                REMOVE_BIT(ch->res_flags, paf->bitvector);
                break;
            case TO_VULN:
                REMOVE_BIT(ch->vuln_flags, paf->bitvector);
                break;
        }
        mod = 0 - mod;
    }

    switch (paf->location)
    {
        default:
            bug("Affect_modify: unknown location %d.", paf->location);
            return;

        case APPLY_NONE:
            break;
        case APPLY_STR:
            ch->mod_stat[STAT_STR] += mod;
            break;
        case APPLY_DEX:
            ch->mod_stat[STAT_DEX] += mod;
            break;
        case APPLY_INT:
            ch->mod_stat[STAT_INT] += mod;
            break;
        case APPLY_WIS:
            ch->mod_stat[STAT_WIS] += mod;
            break;
        case APPLY_CON:
            ch->mod_stat[STAT_CON] += mod;
            break;
        case APPLY_SEX:
            ch->sex += mod;
            break;
        case APPLY_CLASS:
            break;
        case APPLY_LEVEL:
            break;
        case APPLY_AGE:
            break;
        case APPLY_HEIGHT:
            break;
        case APPLY_WEIGHT:
            break;
        case APPLY_MANA:
            ch->max_mana += mod;
            break;
        case APPLY_HIT:
            ch->max_hit += mod;
            break;
        case APPLY_MOVE:
            ch->max_move += mod;
            break;
        case APPLY_GOLD:
            break;
        case APPLY_EXP:
            break;
        case APPLY_AC:
            for (i = 0; i < 4; i++)
                ch->armor[i] += mod;
            break;
        case APPLY_HITROLL:
            ch->hitroll += mod;
            break;
        case APPLY_DAMROLL:
            ch->damroll += mod;
            break;
        case APPLY_SAVES:
            ch->saving_throw += mod;
            break;
        case APPLY_SPELL_AFFECT:
            break;
    }

    /*
     * Check for weapon wielding.
     * Guard against recursion (for weapons with affects).
     */
    if (!IS_NPC(ch) && (wield = get_eq_char(ch, WEAR_WIELD)) != NULL
        && get_obj_weight(wield) >
        (str_app[get_curr_stat(ch, STAT_STR)].wield * 10))
    {
        static int depth;

        if (depth == 0)
        {
            depth++;
            act("You drop $p.", ch, wield, NULL, TO_CHAR);
            act("$n drops $p.", ch, wield, NULL, TO_ROOM);
            obj_from_char(wield);
            obj_to_room(wield, ch->in_room);
            depth--;
        }
    }

    return;
}

/*
 * Finds an effect in an affect list.  It looks through the affect list
 * looking for the sn (skill number).
 */
AFFECT_DATA *affect_find(AFFECT_DATA * paf, int sn)
{
    AFFECT_DATA *paf_find;

    for (paf_find = paf; paf_find != NULL; paf_find = paf_find->next)
    {
        if (paf_find->type == sn)
            return paf_find;
    }

    return NULL;
}

/*
 * Fix object affects when removing one.
 */
void affect_check(CHAR_DATA * ch, int where, int vector)
{
    AFFECT_DATA *paf;
    OBJ_DATA *obj;

    if (where == TO_OBJECT || where == TO_WEAPON || vector == 0)
        return;

    for (paf = ch->affected; paf != NULL; paf = paf->next)
        if (paf->where == where && paf->bitvector == vector)
        {
            switch (where)
            {
                case TO_AFFECTS:
                    SET_BIT(ch->affected_by, vector);
                    break;
                case TO_IMMUNE:
                    SET_BIT(ch->imm_flags, vector);
                    break;
                case TO_RESIST:
                    SET_BIT(ch->res_flags, vector);
                    break;
                case TO_VULN:
                    SET_BIT(ch->vuln_flags, vector);
                    break;
            }
            return;
        }

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
        if (obj->wear_loc == -1)
            continue;

        for (paf = obj->affected; paf != NULL; paf = paf->next)
            if (paf->where == where && paf->bitvector == vector)
            {
                switch (where)
                {
                    case TO_AFFECTS:
                        SET_BIT(ch->affected_by, vector);
                        break;
                    case TO_IMMUNE:
                        SET_BIT(ch->imm_flags, vector);
                        break;
                    case TO_RESIST:
                        SET_BIT(ch->res_flags, vector);
                        break;
                    case TO_VULN:
                        SET_BIT(ch->vuln_flags, vector);

                }
                return;
            }

        if (obj->enchanted)
            continue;

        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
            if (paf->where == where && paf->bitvector == vector)
            {
                switch (where)
                {
                    case TO_AFFECTS:
                        SET_BIT(ch->affected_by, vector);
                        break;
                    case TO_IMMUNE:
                        SET_BIT(ch->imm_flags, vector);
                        break;
                    case TO_RESIST:
                        SET_BIT(ch->res_flags, vector);
                        break;
                    case TO_VULN:
                        SET_BIT(ch->vuln_flags, vector);
                        break;
                }
                return;
            }
    }
}

/*
 * Give an affect to a char.
 */
void affect_to_char(CHAR_DATA * ch, AFFECT_DATA * paf)
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect();

    *paf_new = *paf;

    VALIDATE(paf);                /* in case we missed it when we set up paf */
    paf_new->next = ch->affected;
    ch->affected = paf_new;

    affect_modify(ch, paf_new, TRUE);
    return;
}

/*
 * Give an affect to an object
 */
void affect_to_obj(OBJ_DATA * obj, AFFECT_DATA * paf)
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect();

    *paf_new = *paf;

    VALIDATE(paf);                /* in case we missed it when we set up paf */
    paf_new->next = obj->affected;
    obj->affected = paf_new;

    /* apply any affect vectors to the object's extra_flags */
    if (paf->bitvector)
        switch (paf->where)
        {
            case TO_OBJECT:
                SET_BIT(obj->extra_flags, paf->bitvector);
                break;
            case TO_WEAPON:
                if (obj->item_type == ITEM_WEAPON)
                    SET_BIT(obj->value[4], paf->bitvector);
                break;
        }


    return;
}

/*
 * Remove an affect from a char.
 */
void affect_remove(CHAR_DATA * ch, AFFECT_DATA * paf)
{
    int where;
    int vector;

    if (ch->affected == NULL)
    {
        bug("Affect_remove: no affect.", 0);
        return;
    }

    affect_modify(ch, paf, FALSE);
    where = paf->where;
    vector = paf->bitvector;

    if (paf == ch->affected)
    {
        ch->affected = paf->next;
    }
    else
    {
        AFFECT_DATA *prev;

        for (prev = ch->affected; prev != NULL; prev = prev->next)
        {
            if (prev->next == paf)
            {
                prev->next = paf->next;
                break;
            }
        }

        if (prev == NULL)
        {
            bug("Affect_remove: cannot find paf.", 0);
            return;
        }
    }

    free_affect(paf);

    affect_check(ch, where, vector);
    return;
}

/*
 * Removes an affect from an object.
 */
void affect_remove_obj(OBJ_DATA * obj, AFFECT_DATA * paf)
{
    int where, vector;
    if (obj->affected == NULL)
    {
        bug("Affect_remove_object: no affect.", 0);
        return;
    }

    if (obj->carried_by != NULL && obj->wear_loc != -1)
        affect_modify(obj->carried_by, paf, FALSE);

    where = paf->where;
    vector = paf->bitvector;

    /* remove flags from the object if needed */
    if (paf->bitvector)
        switch (paf->where)
        {
            case TO_OBJECT:
                REMOVE_BIT(obj->extra_flags, paf->bitvector);
                break;
            case TO_WEAPON:
                if (obj->item_type == ITEM_WEAPON)
                    REMOVE_BIT(obj->value[4], paf->bitvector);
                break;
        }

    if (paf == obj->affected)
    {
        obj->affected = paf->next;
    }
    else
    {
        AFFECT_DATA *prev;

        for (prev = obj->affected; prev != NULL; prev = prev->next)
        {
            if (prev->next == paf)
            {
                prev->next = paf->next;
                break;
            }
        }

        if (prev == NULL)
        {
            bug("Affect_remove_object: cannot find paf.", 0);
            return;
        }
    }

    free_affect(paf);

    if (obj->carried_by != NULL && obj->wear_loc != -1)
        affect_check(obj->carried_by, where, vector);
    return;
}

/*
 * Strip all affects of a given sn (skill number).
 */
void affect_strip(CHAR_DATA * ch, int sn)
{
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    for (paf = ch->affected; paf != NULL; paf = paf_next)
    {
        paf_next = paf->next;
        if (paf->type == sn)
            affect_remove(ch, paf);
    }

    return;
}

/*
 * Removes all affects from a character.
 */
void affect_strip_all(CHAR_DATA *ch)
{
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    for (paf = ch->affected; paf != NULL; paf = paf_next)
    {
        paf_next = paf->next;
        affect_remove(ch, paf);
    }

    // Remove all affects!
    ch->affected_by = 0;

}

/*
 * Return true if a char is affected by a spell.
 */
bool is_affected(CHAR_DATA * ch, int sn)
{
    AFFECT_DATA *paf;

    for (paf = ch->affected; paf != NULL; paf = paf->next)
    {
        if (paf->type == sn)
            return TRUE;
    }

    return FALSE;
}

/*
 * Add or enhance an affect.
 */
void affect_join(CHAR_DATA * ch, AFFECT_DATA * paf)
{
    AFFECT_DATA *paf_old;

    for (paf_old = ch->affected; paf_old != NULL; paf_old = paf_old->next)
    {
        if (paf_old->type == paf->type)
        {
            paf->level = (paf->level + paf_old->level) / 2;
            paf->duration += paf_old->duration;
            paf->modifier += paf_old->modifier;
            affect_remove(ch, paf_old);
            break;
        }
    }

    affect_to_char(ch, paf);
    return;
}

/*
 * Gets the modifier on a specific object for the given APPLY_ location (like, APPLY_DAMROLL).
 */
int obj_affect_modifier(OBJ_DATA *obj, int location)
{
    AFFECT_DATA *paf;
    int mod = 0;

    // If the caller passes a null, give them a 0 modifier in return.
    if (obj == NULL)
    {
        return 0;
    }

    // Find the modifier for the requested type, tally all required up then return it.
    if (!obj->enchanted)
    {
        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
        {
            if (paf->location == location)
            {
                mod += paf->modifier;
            }
        }
    }

    for (paf = obj->affected; paf != NULL; paf = paf->next)
    {
        if (paf->location == location)
        {
            mod += paf->modifier;
        }
    }

   return mod;
}

/*
 * Move a char out of a room.
 */
void char_from_room(CHAR_DATA * ch)
{
    OBJ_DATA *obj;

    if (ch->in_room == NULL)
    {
        bug("Char_from_room: NULL.", 0);
        return;
    }

    if (!IS_NPC(ch))
        --ch->in_room->area->nplayer;

    if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL
        && obj->item_type == ITEM_LIGHT
        && obj->value[2] != 0 && ch->in_room->light > 0)
        --ch->in_room->light;

    if (ch == ch->in_room->people)
    {
        ch->in_room->people = ch->next_in_room;
    }
    else
    {
        CHAR_DATA *prev;

        for (prev = ch->in_room->people; prev; prev = prev->next_in_room)
        {
            if (prev->next_in_room == ch)
            {
                prev->next_in_room = ch->next_in_room;
                break;
            }
        }

        if (prev == NULL)
            bug("Char_from_room: ch not found.", 0);
    }

    ch->in_room = NULL;
    ch->next_in_room = NULL;
    ch->on = NULL;                /* sanity check! */
    return;
}

/*
 * Move a char into a room.
 */
void char_to_room(CHAR_DATA * ch, ROOM_INDEX_DATA * pRoomIndex)
{
    OBJ_DATA *obj;

    if (pRoomIndex == NULL)
    {
        ROOM_INDEX_DATA *room;

        bug("Char_to_room: NULL.", 0);

        if ((room = get_room_index(ROOM_VNUM_TEMPLE)) != NULL)
            char_to_room(ch, room);

        return;
    }

    ch->in_room = pRoomIndex;
    ch->next_in_room = pRoomIndex->people;
    pRoomIndex->people = ch;

    if (!IS_NPC(ch))
    {
        if (ch->in_room->area->empty)
        {
            ch->in_room->area->empty = FALSE;
            ch->in_room->area->age = 0;
        }
        ++ch->in_room->area->nplayer;
    }

    if ((obj = get_eq_char(ch, WEAR_LIGHT)) != NULL
        && obj->item_type == ITEM_LIGHT && obj->value[2] != 0)
        ++ch->in_room->light;

    if (IS_AFFECTED(ch, AFF_PLAGUE))
    {
        AFFECT_DATA *af, plague;
        CHAR_DATA *vch;

        for (af = ch->affected; af != NULL; af = af->next)
        {
            if (af->type == gsn_plague)
                break;
        }

        if (af == NULL)
        {
            REMOVE_BIT(ch->affected_by, AFF_PLAGUE);
            return;
        }

        if (af->level == 1)
            return;

        plague.where = TO_AFFECTS;
        plague.type = gsn_plague;
        plague.level = af->level - 1;
        plague.duration = number_range(1, 2 * plague.level);
        plague.location = APPLY_STR;
        plague.modifier = -5;
        plague.bitvector = AFF_PLAGUE;

        for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            if (!saves_spell(plague.level - 2, vch, DAM_DISEASE)
                && !IS_IMMORTAL(vch) &&
                !IS_AFFECTED(vch, AFF_PLAGUE) && number_bits(6) == 0)
            {
                send_to_char("You feel hot and feverish.\r\n", vch);
                act("$n shivers and looks very ill.", vch, NULL, NULL,
                    TO_ROOM);
                affect_join(vch, &plague);
            }
        }
    }


    return;
}

/*
 * Give an obj to a char.
 */
OBJ_DATA *obj_to_char(OBJ_DATA *obj, CHAR_DATA *ch)
{
    OBJ_DATA *otmp;
    OBJ_DATA *oret = obj;
    bool grouped;
    int oweight = get_obj_weight(obj);
    int onum = get_obj_number(obj);

    grouped = FALSE;

    for (otmp = ch->carrying; otmp; otmp = otmp->next_content)
        if ((oret = group_object(otmp, obj)) == otmp)
        {
            grouped = TRUE;
            break;
        }

    if (!grouped)
    {
        obj->next_content = ch->carrying;
        ch->carrying = obj;
        obj->carried_by = ch;
        obj->in_room = NULL;
        obj->in_obj = NULL;
    }

    ch->carry_number += onum;
    ch->carry_weight += oweight;
    return (oret ? oret : obj);
}

/*
 * Take an obj from its character.
 */
void obj_from_char(OBJ_DATA * obj)
{
    CHAR_DATA *ch;

    // Character is null... do some logging.
    if ((ch = obj->carried_by) == NULL)
    {
        bug("Obj_from_char: null ch.", 0);
        log_obj(obj);
        return;
    }

    if (obj->wear_loc != WEAR_NONE)
        unequip_char(ch, obj);

    if (ch->carrying == obj)
    {
        ch->carrying = obj->next_content;
    }
    else
    {
        OBJ_DATA *prev;

        for (prev = ch->carrying; prev != NULL; prev = prev->next_content)
        {
            if (prev->next_content == obj)
            {
                prev->next_content = obj->next_content;
                break;
            }
        }

        if (prev == NULL)
            bug("Obj_from_char: obj not in list.", 0);
    }

    obj->carried_by = NULL;
    obj->next_content = NULL;
    ch->carry_number -= get_obj_number(obj);
    ch->carry_weight -= get_obj_weight(obj);
    return;
}

/*
 * Find the ac value of an obj, including position effect.
 */
int apply_ac(OBJ_DATA * obj, int iWear, int type)
{
    if (obj->item_type != ITEM_ARMOR)
        return 0;

    switch (iWear)
    {
        case WEAR_BODY:
            return 3 * obj->value[type];
        case WEAR_HEAD:
            return 2 * obj->value[type];
        case WEAR_LEGS:
            return 2 * obj->value[type];
        case WEAR_FEET:
            return obj->value[type];
        case WEAR_HANDS:
            return obj->value[type];
        case WEAR_ARMS:
            return obj->value[type];
        case WEAR_SHIELD:
            return obj->value[type];
        case WEAR_NECK_1:
            return obj->value[type];
        case WEAR_NECK_2:
            return obj->value[type];
        case WEAR_ABOUT:
            return 2 * obj->value[type];
        case WEAR_WAIST:
            return obj->value[type];
        case WEAR_WRIST_L:
            return obj->value[type];
        case WEAR_WRIST_R:
            return obj->value[type];
        case WEAR_HOLD:
            return obj->value[type];
    }

    return 0;
}

/*
 * Find a piece of equipment that a character is currently wearing at a
 * specified location
 */
OBJ_DATA *get_eq_char(CHAR_DATA * ch, int iWear)
{
    OBJ_DATA *obj;

    if (ch == NULL)
        return NULL;

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
        if (obj->wear_loc == iWear)
            return obj;
    }

    return NULL;
}

/*
 * Equip a char with an obj.
 */
void equip_char(CHAR_DATA * ch, OBJ_DATA * obj, int iWear)
{
    AFFECT_DATA *paf;
    int i;

    if (get_eq_char(ch, iWear) != NULL)
    {
        bug("Equip_char: already equipped (%d).", iWear);
        return;
    }

    separate_obj(obj);

    if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch))
        || (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch))
        || (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch)))
    {
        /*
         * Thanks to Morgenes for the bug fix here!
         */
        act("You are zapped by $p and drop it.", ch, obj, NULL, TO_CHAR);
        act("$n is zapped by $p and drops it.", ch, obj, NULL, TO_ROOM);
        obj_from_char(obj);
        obj_to_room(obj, ch->in_room);
        return;
    }

    for (i = 0; i < 4; i++)
    {
        ch->armor[i] -= apply_ac(obj, iWear, i);
    }

    obj->wear_loc = iWear;

    if (!obj->enchanted)
    {
        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
        {
            if (paf->location != APPLY_SPELL_AFFECT)
            {
                // Modify the affect if it's not a weapon or dual wielded weapon with +hit/+dam, those will be handled elsewhere
                if (!((iWear == WEAR_WIELD || iWear == WEAR_SECONDARY_WIELD) && (paf->location == APPLY_HITROLL || paf->location == APPLY_DAMROLL)))
                {
                    affect_modify(ch, paf, TRUE);
                }
            }
        }
    }

    for (paf = obj->affected; paf != NULL; paf = paf->next)
    {
        if (paf->location == APPLY_SPELL_AFFECT)
        {
            affect_to_char(ch, paf);
        }
        else
        {
            // Modify the affect if it's not a weapon or dual wielded weapon with +hit/+dam, those will be handled elsewhere
            if (!((iWear == WEAR_WIELD || iWear == WEAR_SECONDARY_WIELD) && (paf->location == APPLY_HITROLL || paf->location == APPLY_DAMROLL)))
            {
                affect_modify(ch, paf, TRUE);
            }
        }
    }

    if (obj->item_type == ITEM_LIGHT && obj->value[2] != 0 && ch->in_room != NULL)
    {
        ++ch->in_room->light;
    }

    return;
}

/*
 * Unequip a char with an obj.
 */
void unequip_char(CHAR_DATA * ch, OBJ_DATA * obj)
{
    AFFECT_DATA *paf = NULL;
    AFFECT_DATA *lpaf = NULL;
    AFFECT_DATA *lpaf_next = NULL;
    int i;

    if (obj->wear_loc == WEAR_NONE)
    {
        bug("Unequip_char: already unequipped.", 0);
        return;
    }

    for (i = 0; i < 4; i++)
    {
        ch->armor[i] += apply_ac(obj, obj->wear_loc, i);
    }

    if (!obj->enchanted)
    {
        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
        {
            if (paf->location == APPLY_SPELL_AFFECT)
            {
                for (lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next)
                {
                    lpaf_next = lpaf->next;
                    if ((lpaf->type == paf->type) &&
                        (lpaf->level == paf->level) &&
                        (lpaf->location == APPLY_SPELL_AFFECT))
                    {
                        affect_remove(ch, lpaf);
                        lpaf_next = NULL;
                    }
                }
            }
            else
            {
                // Modify the affect if it's not a weapon or dual wielded weapon with +hit/+dam, those will be handled elsewhere
                if (!((obj->wear_loc == WEAR_WIELD || obj->wear_loc == WEAR_SECONDARY_WIELD) && (paf->location == APPLY_HITROLL || paf->location == APPLY_DAMROLL)))
                {
                    affect_modify(ch, paf, FALSE);
                }

                affect_check(ch, paf->where, paf->bitvector);
            }
        }
    }

    for (paf = obj->affected; paf != NULL; paf = paf->next)
    {
        if (paf->location == APPLY_SPELL_AFFECT)
        {
            bug("Norm-Apply: %d", 0);
            for (lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next)
            {
                lpaf_next = lpaf->next;
                if ((lpaf->type == paf->type) &&
                    (lpaf->level == paf->level) &&
                    (lpaf->location == APPLY_SPELL_AFFECT))
                {
                    bug("location = %d", lpaf->location);
                    bug("type = %d", lpaf->type);
                    affect_remove(ch, lpaf);
                    lpaf_next = NULL;
                }
            }
        }
        else
        {
            // Modify the affect if it's not a weapon or dual wielded weapon with +hit/+dam, those will be handled elsewhere
            if (!((obj->wear_loc == WEAR_WIELD || obj->wear_loc == WEAR_SECONDARY_WIELD) && (paf->location == APPLY_HITROLL || paf->location == APPLY_DAMROLL)))
            {
                affect_modify(ch, paf, FALSE);
            }

            affect_check(ch, paf->where, paf->bitvector);
        }
    }

    obj->wear_loc = -1;

    if (obj->item_type == ITEM_LIGHT
        && obj->value[2] != 0
        && ch->in_room != NULL
        && ch->in_room->light > 0) --ch->in_room->light;

    return;
}

/*
 * Count occurrences of an obj in a list.
 */
int count_obj_list(OBJ_INDEX_DATA * pObjIndex, OBJ_DATA * list)
{
    OBJ_DATA *obj;
    int nMatch;

    nMatch = 0;
    for (obj = list; obj != NULL; obj = obj->next_content)
    {
        if (obj->pIndexData == pObjIndex)
            nMatch++;
    }

    return nMatch;
}

/*
 * Move an obj out of a room.
 */
void obj_from_room(OBJ_DATA * obj)
{
    ROOM_INDEX_DATA *in_room;
    CHAR_DATA *ch;

    if ((in_room = obj->in_room) == NULL)
    {
        bugf("obj_from_room: obj->in_room is NULL.  Object VNUM=%d", obj->pIndexData->vnum);
        log_obj(obj);
        return;
    }

    for (ch = in_room->people; ch != NULL; ch = ch->next_in_room)
        if (ch->on == obj)
            ch->on = NULL;

    if (obj == in_room->contents)
    {
        in_room->contents = obj->next_content;
    }
    else
    {
        OBJ_DATA *prev;

        for (prev = in_room->contents; prev; prev = prev->next_content)
        {
            if (prev->next_content == obj)
            {
                prev->next_content = obj->next_content;
                break;
            }
        }

        if (prev == NULL)
        {
            bug("Obj_from_room: obj not found.", 0);
            return;
        }
    }

    obj->in_room = NULL;
    obj->next_content = NULL;
    return;
}

/*
 * Move an obj into a room.
 */
OBJ_DATA *obj_to_room(OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex)
{
    OBJ_DATA *otmp, *oret;

    if (pRoomIndex == NULL)
    {
        bug("Obj_to_room: room not found removeing from game.", 0);
        extract_obj(obj);   /* This is to prevent memory leaks */
        return NULL;
    }

    if (obj == NULL)
    {
        bug("Obj_to_room: obj not found.", 0);
        return NULL;
    }

    for (otmp = pRoomIndex->contents; otmp; otmp = otmp->next_content)
        if ((oret = group_object(otmp, obj)) == otmp)
            return oret;

    obj->next_content = pRoomIndex->contents;
    pRoomIndex->contents = obj;
    obj->in_room = pRoomIndex;
    obj->carried_by = NULL;
    obj->in_obj = NULL;
    return obj;
}

/*
 * Move an object into an object.
 */
OBJ_DATA *obj_to_obj(OBJ_DATA *obj, OBJ_DATA *obj_to)
{
    OBJ_DATA *otmp, *oret;

    if (obj == obj_to)
    {
        bug("Obj_to_obj: trying to put object inside itself: vnum %d", obj->pIndexData->vnum);
        return obj;
    }

    if (!obj || !obj_to) return NULL;

    if (obj_to->carried_by)
    {
        obj_to->carried_by->carry_number += get_obj_number(obj);
        obj_to->carried_by->carry_weight += get_obj_weight(obj) * WEIGHT_MULT(obj_to) / 100;
    }


    for (otmp = obj_to->contains; otmp; otmp = otmp->next_content)
        if ((oret = group_object(otmp, obj)) == otmp)
            return oret;

    obj->next_content = obj_to->contains;
    obj_to->contains = obj;
    obj->in_obj = obj_to;
    obj->in_room = NULL;
    obj->carried_by = NULL;
    if (obj_to->pIndexData->vnum == OBJ_VNUM_PIT)
        obj->cost = 0;

    return obj;
}

/*
 * Move an object out of an object.
 */
void obj_from_obj(OBJ_DATA * obj)
{
    OBJ_DATA *obj_from;

    if ((obj_from = obj->in_obj) == NULL)
    {
        bug("Obj_from_obj: null obj_from.", 0);
        return;
    }

    if (obj == obj_from->contains)
    {
        obj_from->contains = obj->next_content;
    }
    else
    {
        OBJ_DATA *prev;

        for (prev = obj_from->contains; prev; prev = prev->next_content)
        {
            if (prev->next_content == obj)
            {
                prev->next_content = obj->next_content;
                break;
            }
        }

        if (prev == NULL)
        {
            bug("Obj_from_obj: obj not found.", 0);
            return;
        }
    }

    obj->next_content = NULL;
    obj->in_obj = NULL;

    for (; obj_from != NULL; obj_from = obj_from->in_obj)
    {
        if (obj_from->carried_by != NULL)
        {
            obj_from->carried_by->carry_number -= get_obj_number(obj);
            obj_from->carried_by->carry_weight -= get_obj_weight(obj)
                * WEIGHT_MULT(obj_from) / 100;
        }
    }

    return;
}

/*
 * Extract an obj from the world.
 */
void extract_obj(OBJ_DATA * obj)
{
    OBJ_DATA *obj_content;
    OBJ_DATA *obj_next;

    if (obj->in_room != NULL)
        obj_from_room(obj);
    else if (obj->carried_by != NULL)
        obj_from_char(obj);
    else if (obj->in_obj != NULL)
        obj_from_obj(obj);

    for (obj_content = obj->contains; obj_content; obj_content = obj_next)
    {
        obj_next = obj_content->next_content;
        extract_obj(obj_content);
    }

    if (object_list == obj)
    {
        object_list = obj->next;
    }
    else
    {
        OBJ_DATA *prev;

        for (prev = object_list; prev != NULL; prev = prev->next)
        {
            if (prev->next == obj)
            {
                prev->next = obj->next;
                break;
            }
        }

        if (prev == NULL)
        {
            if (obj->in_room != NULL)
            {
                bugf("Extract_obj: obj %d not found in room %d.", obj->pIndexData->vnum, obj->in_room->vnum);
            }
            else
            {
                bug("Extract_obj: obj %d not found.", obj->pIndexData->vnum);
            }

            return;
        }
    }

    --obj->pIndexData->count;
    free_obj(obj);
    return;
}

/*
 * Extract a char from the world.
 */
void extract_char(CHAR_DATA * ch, bool fPull)
{
    CHAR_DATA *wch;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    /* doesn't seem to be necessary
       if ( ch->in_room == NULL )
       {
       bug( "Extract_char: NULL.", 0 );
       return;
       }
     */

    nuke_pets(ch);
    ch->pet = NULL;                /* just in case */

    if (fPull)
    {
        die_follower(ch);
    }

    stop_fighting(ch, TRUE);

    for (obj = ch->carrying; obj != NULL; obj = obj_next)
    {
        obj_next = obj->next_content;
        extract_obj(obj);
    }

    if (ch->in_room != NULL)
        char_from_room(ch);

    // Death room is set in the clan table now.  Clanners go to their hall.  If the player
    // isn't a clanner, it uses the 0 index (which has the ROOM_VNUM_ALTAR variable in it).
    if (!fPull)
    {
        // Exiled clanner (not sure this actually gets hit as a PC but will put it here just in case)
        if (!IS_NPC(ch) && IS_SET(ch->pcdata->clan_flags, CLAN_EXILE))
        {
            char_to_room(ch, get_room_index(clan_table[ch->clan].recall_vnum));
            return;
        }

        // Everyone else
        char_to_room(ch, get_room_index(clan_table[ch->clan].hall));
        return;
    }

    if (IS_NPC(ch))
        --ch->pIndexData->count;

    if (ch->desc != NULL && ch->desc->original != NULL)
    {
        do_function(ch, &do_return, "");
        ch->desc = NULL;
    }

    for (wch = char_list; wch != NULL; wch = wch->next)
    {
        if (wch->reply == ch)
            wch->reply = NULL;
        if (ch->mprog_target == wch)
            wch->mprog_target = NULL;
    }

    if (ch == char_list)
    {
        char_list = ch->next;
    }
    else
    {
        CHAR_DATA *prev;

        for (prev = char_list; prev != NULL; prev = prev->next)
        {
            if (prev->next == ch)
            {
                prev->next = ch->next;
                break;
            }
        }

        if (prev == NULL)
        {
            if (ch != NULL)
            {
                bugf("%s was being extracted.\r\n", ch->name);
            }

            bug("Extract_char: char not found.", 0);
            return;
        }
    }

    if (ch->desc != NULL)
    {
        ch->desc->character = NULL;
    }

    free_char(ch);
    return;
}

/*
 * Find a char in the room.
 */
CHAR_DATA *get_char_room(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *rch;
    int number;
    int count;

    // If they passed a null in here, return them a null back but -don't- crash.
    if (IS_NULLSTR(argument))
    {
        return NULL;
    }

    number = number_argument(argument, arg);
    count = 0;

    if (!str_cmp(arg, "self"))
        return ch;

    for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
    {
        if (!can_see(ch, rch) || !is_name(arg, rch->name))
            continue;
        if (++count == number)
            return rch;
    }

    return NULL;
}

/*
 * Find a char in the area
 */
CHAR_DATA *get_char_area(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *wch;
    int number;
    int count;

    // If they passed a null in here, return them a null back but -don't- crash.
    if (IS_NULLSTR(argument))
    {
        return NULL;
    }

    if (!str_cmp(argument, "self"))
        return ch;

    if ((wch = get_char_room(ch, argument)) != NULL)
        return wch;

    number = number_argument(argument, arg);
    count = 0;

    for (wch = char_list; wch != NULL; wch = wch->next)
    {
        if (wch->in_room == NULL || !can_see (ch, wch)
            || !is_name (arg, wch->name)
            || (ch->in_room->area != wch->in_room->area))
            continue;

        if (++count == number)
            return wch;
    }

    return NULL;

} // end get_char_area

/*
 * Find a char in the world.
 */
CHAR_DATA *get_char_world(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *wch;
    int number;
    int count;

    // If they passed a null in here, return them a null back but -don't- crash.
    if (IS_NULLSTR(argument))
    {
        return NULL;
    }

    if ((wch = get_char_room(ch, argument)) != NULL)
        return wch;

    number = number_argument(argument, arg);
    count = 0;
    for (wch = char_list; wch != NULL; wch = wch->next)
    {
        if (wch->in_room == NULL || !can_see(ch, wch)
            || !is_name(arg, wch->name))
            continue;
        if (++count == number)
            return wch;
    }

    return NULL;
}

/*
 * Find some object with a given index data.
 * Used by area-reset 'P' command.
 */
OBJ_DATA *get_obj_type(OBJ_INDEX_DATA * pObjIndex)
{
    OBJ_DATA *obj;

    for (obj = object_list; obj != NULL; obj = obj->next)
    {
        if (obj->pIndexData == pObjIndex)
            return obj;
    }

    return NULL;
}

/*
 * Find an obj in a list.
 */
OBJ_DATA *get_obj_list(CHAR_DATA * ch, char *argument, OBJ_DATA * list)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument(argument, arg);
    count = 0;
    for (obj = list; obj != NULL; obj = obj->next_content)
    {
        if (can_see_obj(ch, obj) && is_name(arg, obj->name))
        {
            if ((count += obj->count) >= number)
            {
                return obj;
            }
        }
    }

    return NULL;
}

/*
 * Find an obj in player's inventory.
 */
OBJ_DATA *get_obj_carry(CHAR_DATA * ch, char *argument, CHAR_DATA * viewer)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument(argument, arg);
    count = 0;
    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
        if (obj->wear_loc == WEAR_NONE && (can_see_obj(viewer, obj))
            && is_name(arg, obj->name))
        {
            if ((count += obj->count) >= number)
                return obj;
        }
    }

    return NULL;
}

/*
 * Find an obj in player's equipment.
 */
OBJ_DATA *get_obj_wear(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument(argument, arg);
    count = 0;
    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
        if (obj->wear_loc != WEAR_NONE && can_see_obj(ch, obj)
            && is_name(arg, obj->name))
        {
            if ((count += obj->count) >= number)
                return obj;
        }
    }

    return NULL;
}

/*
 * Find an obj in the room or in inventory.
 */
OBJ_DATA *get_obj_here(CHAR_DATA * ch, char *argument)
{
    OBJ_DATA *obj;

    obj = get_obj_list(ch, argument, ch->in_room->contents);
    if (obj != NULL)
        return obj;

    if ((obj = get_obj_carry(ch, argument, ch)) != NULL)
        return obj;

    if ((obj = get_obj_wear(ch, argument)) != NULL)
        return obj;

    return NULL;
}

/*
 * Find an obj in the world.
 */
OBJ_DATA *get_obj_world(CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    if ((obj = get_obj_here(ch, argument)) != NULL)
        return obj;

    number = number_argument(argument, arg);
    count = 0;
    for (obj = object_list; obj != NULL; obj = obj->next)
    {
        if (can_see_obj(ch, obj) && is_name(arg, obj->name))
        {
            if ((count += obj->count) >= number)
                return obj;
        }
    }

    return NULL;
}

/*
 * Deduct cost from a character
 */
void deduct_cost(CHAR_DATA * ch, int cost)
{
    int silver = 0, gold = 0;

    silver = UMIN(ch->silver, cost);

    if (silver < cost)
    {
        gold = ((cost - silver + 99) / 100);
        silver = cost - 100 * gold;
    }

    ch->gold -= gold;
    ch->silver -= silver;

    if (ch->gold < 0)
    {
        bug("deduct costs: gold %d < 0", ch->gold);
        ch->gold = 0;
    }
    if (ch->silver < 0)
    {
        bug("deduct costs: silver %d < 0", ch->silver);
        ch->silver = 0;
    }
}

/*
 * Create a 'money' obj.
 */
OBJ_DATA *create_money(int gold, int silver)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;

    if (gold < 0 || silver < 0 || (gold == 0 && silver == 0))
    {
        bug("Create_money: zero or negative money.", UMIN(gold, silver));
        gold = UMAX(1, gold);
        silver = UMAX(1, silver);
    }

    if (gold == 0 && silver == 1)
    {
        obj = create_object(get_obj_index(OBJ_VNUM_SILVER_ONE));
    }
    else if (gold == 1 && silver == 0)
    {
        obj = create_object(get_obj_index(OBJ_VNUM_GOLD_ONE));
    }
    else if (silver == 0)
    {
        obj = create_object(get_obj_index(OBJ_VNUM_GOLD_SOME));
        sprintf(buf, obj->short_descr, gold);
        free_string(obj->short_descr);
        obj->short_descr = str_dup(buf);
        obj->value[1] = gold;
        obj->cost = gold;
        obj->weight = gold / 5;
    }
    else if (gold == 0)
    {
        obj = create_object(get_obj_index(OBJ_VNUM_SILVER_SOME));
        sprintf(buf, obj->short_descr, silver);
        free_string(obj->short_descr);
        obj->short_descr = str_dup(buf);
        obj->value[0] = silver;
        obj->cost = silver;
        obj->weight = silver / 20;
    }

    else
    {
        obj = create_object(get_obj_index(OBJ_VNUM_COINS));
        sprintf(buf, obj->short_descr, silver, gold);
        free_string(obj->short_descr);
        obj->short_descr = str_dup(buf);
        obj->value[0] = silver;
        obj->value[1] = gold;
        obj->cost = 100 * gold + silver;
        obj->weight = gold / 5 + silver / 20;
    }

    return obj;
}

/*
 * Return # of objects which an object counts as.
 * Thanks to Tony Chamberlain for the correct recursive code here.
 * This method skips containers, gems, jewelry and money as small
 * gems like diamonds could add up quickly (and you may have a reason
 * to carry a large amount on you in order to make purchases.
 */
int get_obj_number(OBJ_DATA * obj)
{
    int number;

    if (obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_MONEY
        || obj->item_type == ITEM_GEM || obj->item_type == ITEM_JEWELRY)
    {
        number = 0;
    }
    else
    {
        number = obj->count;
    }

    for (obj = obj->contains; obj != NULL; obj = obj->next_content)
    {
        number += get_obj_number(obj);
    }

    return number;
}

/*
 * Counts all objects of a specific type in a users inventory (this goes
 * through their containers also).
 */
int obj_count_by_type(OBJ_DATA *obj, int item_type)
{
    int number = 0;

    if (obj->item_type == item_type)
    {
        number = obj->count;
    }

    for (obj = obj->contains; obj != NULL; obj = obj->next_content)
    {
        number += obj_count_by_type(obj, item_type);
    }

    return number;

}

/*
 * Return weight of an object, including weight of contents.
 */
int get_obj_weight(OBJ_DATA * obj)
{
    int weight;
    OBJ_DATA *tobj;

    weight = obj->count * obj->weight;

    for (tobj = obj->contains; tobj != NULL; tobj = tobj->next_content)
        weight += get_obj_weight(tobj) * WEIGHT_MULT(obj) / 100;

    return weight;
}

/*
 * Returns the true weight of an object, including weight of contents without
 * the weight multiplier of the container if it is.
 */
int get_true_weight(OBJ_DATA * obj)
{
    int weight;

    weight = obj->count * obj->weight;

    for (obj = obj->contains; obj != NULL; obj = obj->next_content)
        weight += get_obj_weight(obj);

    return weight;
}

/*
 * True if room is dark.
 */
bool room_is_dark(ROOM_INDEX_DATA * pRoomIndex)
{
    if (pRoomIndex == NULL)
        return TRUE;

    if (pRoomIndex->light > 0)
        return FALSE;

    if (IS_SET(pRoomIndex->room_flags, ROOM_DARK) || pRoomIndex->sector_type == SECT_UNDERGROUND)
        return TRUE;

    if (pRoomIndex->sector_type == SECT_INSIDE || pRoomIndex->sector_type == SECT_CITY)
        return FALSE;

    if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
        return TRUE;

    return FALSE;
}

/*
 * Whether the character is the owner of the requested room.
 */
bool is_room_owner(CHAR_DATA * ch, ROOM_INDEX_DATA * room)
{
    if (room->owner == NULL || room->owner[0] == '\0')
        return FALSE;

    return is_name(ch->name, room->owner);
}

/*
 * True if room is private.
 */
bool room_is_private(ROOM_INDEX_DATA * pRoomIndex)
{
    CHAR_DATA *rch;
    int count;


    if (pRoomIndex->owner != NULL && pRoomIndex->owner[0] != '\0')
        return TRUE;

    count = 0;
    for (rch = pRoomIndex->people; rch != NULL; rch = rch->next_in_room)
        count++;

    if (IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE) && count >= 2)
        return TRUE;

    if (IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY) && count >= 1)
        return TRUE;

    if (IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY))
        return TRUE;

    return FALSE;
}

/*
 * Visibility on a room -- for entering and exits
 */
bool can_see_room(CHAR_DATA * ch, ROOM_INDEX_DATA * pRoomIndex)
{
    if (IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY)
        && get_trust(ch) < MAX_LEVEL)
        return FALSE;

    if (IS_SET(pRoomIndex->room_flags, ROOM_GODS_ONLY) && !IS_IMMORTAL(ch))
        return FALSE;

    if (IS_SET(pRoomIndex->room_flags, ROOM_HEROES_ONLY)
        && !IS_IMMORTAL(ch))
        return FALSE;

    if (IS_SET(pRoomIndex->room_flags, ROOM_NEWBIES_ONLY)
        && ch->level > 5 && !IS_IMMORTAL(ch))
        return FALSE;

    // The clan room check is different between PC/NPC.. because PC needs to check pcdata for exile flag
    // to make sure an exiled clan member can't enter the clan hall.
    if (IS_NPC(ch))
    {
        if (!IS_IMMORTAL(ch) && pRoomIndex->clan && ch->clan != pRoomIndex->clan)
        {
            return FALSE;
        }
    }
    else
    {
        if (!IS_IMMORTAL(ch) && pRoomIndex->clan
            && ((ch->clan != pRoomIndex->clan) || IS_SET(ch->pcdata->clan_flags, CLAN_EXILE)))
        {
            return FALSE;
        }
    }

    return TRUE;
}

/*
 * True if char can see victim.
 */
bool can_see(CHAR_DATA * ch, CHAR_DATA * victim)
{
/* RT changed so that WIZ_INVIS has levels */
    if (ch == victim)
        return TRUE;

    if (get_trust(ch) < victim->invis_level)
        return FALSE;


    if (get_trust(ch) < victim->incog_level
        && ch->in_room != victim->in_room) return FALSE;

    if ((!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
        || (IS_NPC(ch) && IS_IMMORTAL(ch)))
        return TRUE;

    if (IS_AFFECTED(ch, AFF_BLIND))
        return FALSE;

    if (room_is_dark(ch->in_room) && !IS_AFFECTED(ch, AFF_INFRARED))
        return FALSE;

    if (IS_AFFECTED(victim, AFF_INVISIBLE)
        && !IS_AFFECTED(ch, AFF_DETECT_INVIS))
        return FALSE;

    // Hiding
    if ((IS_AFFECTED(victim, AFF_HIDE) || is_affected(victim, gsn_camouflage))
       && victim->fighting == NULL
       && !check_skill_improve(ch, gsn_acute_vision, 4, 9)
       && ( !IS_NPC(victim)
       || (IS_NPC(victim)
       && !IS_AFFECTED(ch,AFF_DETECT_HIDDEN))))
        return FALSE;

    // Sneaking
    if ( IS_AFFECTED(victim, AFF_SNEAK)
        && !IS_AFFECTED(ch, AFF_DETECT_HIDDEN)
        && !check_skill_improve(ch, gsn_acute_vision, 4, 9)
        && victim->fighting == NULL)
    {
        int chance;
        chance = get_skill(victim, gsn_sneak);
        chance += get_curr_stat(victim, STAT_DEX) * 3 / 2;
        chance -= get_curr_stat(ch, STAT_INT) * 2;
        chance -= ch->level - victim->level * 3 / 2;

        if (number_percent() < chance)
            return FALSE;
    }

    // Rangers quiet movement, they can see each other with acute vision but
    // non-detectable to others in certain nature like room types.
    if (is_affected(victim, gsn_quiet_movement)
        && !check_skill_improve(ch, gsn_acute_vision, 4, 9)
        && victim->fighting == NULL
        && (victim->in_room->sector_type == SECT_FIELD
        || victim->in_room->sector_type == SECT_FOREST
        || victim->in_room->sector_type == SECT_HILLS
        || victim->in_room->sector_type == SECT_DESERT
        || victim->in_room->sector_type == SECT_MOUNTAIN))
    {
        int chance;
        chance = get_skill(victim,gsn_quiet_movement);
        chance += get_curr_stat(victim,STAT_DEX) * 3 / 2;
        chance -= get_curr_stat(ch,STAT_INT) * 2;
        chance -= ch->level - victim->level * 3 / 2;

        // Slim chance to see them, if this fails return false, they cannot see.
        if (CHANCE(chance))
            return FALSE;
    }


    return TRUE;
}

/*
 * True if char can see obj.
 */
bool can_see_obj(CHAR_DATA * ch, OBJ_DATA * obj)
{
    if (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
        return TRUE;

    if (IS_OBJ_STAT(obj, ITEM_BURIED))
        return FALSE;

    if (IS_SET(obj->extra_flags, ITEM_VIS_DEATH))
        return FALSE;

    if (IS_AFFECTED(ch, AFF_BLIND) && obj->item_type != ITEM_POTION)
        return FALSE;

    if (obj->item_type == ITEM_LIGHT && obj->value[2] != 0)
        return TRUE;

    if (IS_SET(obj->extra_flags, ITEM_INVIS)
        && !IS_AFFECTED(ch, AFF_DETECT_INVIS))
        return FALSE;

    if (IS_OBJ_STAT(obj, ITEM_GLOW))
        return TRUE;

    if (room_is_dark(ch->in_room) && !IS_AFFECTED(ch, AFF_DARK_VISION))
        return FALSE;

    return TRUE;
}

/*
 * True if char can drop obj.
 */
bool can_drop_obj(CHAR_DATA * ch, OBJ_DATA * obj)
{
    if (!IS_SET(obj->extra_flags, ITEM_NODROP))
        return TRUE;

    if (!IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL)
        return TRUE;

    return FALSE;
}

/*
 * Return ascii name of an affect location.
 */
char *affect_loc_name(int location)
{
    switch (location)
    {
        case APPLY_NONE:
            return "none";
        case APPLY_STR:
            return "strength";
        case APPLY_DEX:
            return "dexterity";
        case APPLY_INT:
            return "intelligence";
        case APPLY_WIS:
            return "wisdom";
        case APPLY_CON:
            return "constitution";
        case APPLY_SEX:
            return "sex";
        case APPLY_CLASS:
            return "class";
        case APPLY_LEVEL:
            return "level";
        case APPLY_AGE:
            return "age";
        case APPLY_MANA:
            return "mana";
        case APPLY_HIT:
            return "hp";
        case APPLY_MOVE:
            return "moves";
        case APPLY_GOLD:
            return "gold";
        case APPLY_EXP:
            return "experience";
        case APPLY_AC:
            return "armor class";
        case APPLY_HITROLL:
            return "hit roll";
        case APPLY_DAMROLL:
            return "damage roll";
        case APPLY_SAVES:
            return "saves";
        case APPLY_SPELL_AFFECT:
            return "none";
    }

    bug("Affect_location_name: unknown location %d.", location);
    return "(unknown)";
}

/*
 * Return ascii name of an affect bit vector.
 */
char *affect_bit_name(int vector)
{
    static char buf[512];

    buf[0] = '\0';
    if (vector & AFF_BLIND)
        strcat(buf, " blind");
    if (vector & AFF_INVISIBLE)
        strcat(buf, " invisible");
    if (vector & AFF_DETECT_EVIL)
        strcat(buf, " detect_evil");
    if (vector & AFF_DETECT_GOOD)
        strcat(buf, " detect_good");
    if (vector & AFF_DETECT_INVIS)
        strcat(buf, " detect_invis");
    if (vector & AFF_DETECT_MAGIC)
        strcat(buf, " detect_magic");
    if (vector & AFF_DETECT_HIDDEN)
        strcat(buf, " detect_hidden");
    if (vector & AFF_SANCTUARY)
        strcat(buf, " sanctuary");
    if (vector & AFF_FAERIE_FIRE)
        strcat(buf, " faerie_fire");
    if (vector & AFF_INFRARED)
        strcat(buf, " infrared");
    if (vector & AFF_CURSE)
        strcat(buf, " curse");
    if (vector & AFF_POISON)
        strcat(buf, " poison");
    if (vector & AFF_PROTECT_EVIL)
        strcat(buf, " prot_evil");
    if (vector & AFF_PROTECT_GOOD)
        strcat(buf, " prot_good");
    if (vector & AFF_SLEEP)
        strcat(buf, " sleep");
    if (vector & AFF_SNEAK)
        strcat(buf, " sneak");
    if (vector & AFF_HIDE)
        strcat(buf, " hide");
    if (vector & AFF_CHARM)
        strcat(buf, " charm");
    if (vector & AFF_FLYING)
        strcat(buf, " flying");
    if (vector & AFF_PASS_DOOR)
        strcat(buf, " pass_door");
    if (vector & AFF_BERSERK)
        strcat(buf, " berserk");
    if (vector & AFF_CALM)
        strcat(buf, " calm");
    if (vector & AFF_HASTE)
        strcat(buf, " haste");
    if (vector & AFF_SLOW)
        strcat(buf, " slow");
    if (vector & AFF_PLAGUE)
        strcat(buf, " plague");
    if (vector & AFF_DARK_VISION)
        strcat(buf, " dark_vision");
    if (vector & AFF_DEAFEN)
        strcat(buf, " deafen");
    return (buf[0] != '\0') ? buf + 1 : "none";
}

/*
 * Return ascii name of extra flags vector.
 */
char *extra_bit_name(int extra_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (extra_flags & ITEM_GLOW)
        strcat(buf, " glow");
    if (extra_flags & ITEM_HUM)
        strcat(buf, " hum");
    if (extra_flags & ITEM_DARK)
        strcat(buf, " dark");
    if (extra_flags & ITEM_LOCK)
        strcat(buf, " lock");
    if (extra_flags & ITEM_EVIL)
        strcat(buf, " evil");
    if (extra_flags & ITEM_INVIS)
        strcat(buf, " invis");
    if (extra_flags & ITEM_MAGIC)
        strcat(buf, " magic");
    if (extra_flags & ITEM_NODROP)
        strcat(buf, " nodrop");
    if (extra_flags & ITEM_BLESS)
        strcat(buf, " bless");
    if (extra_flags & ITEM_ANTI_GOOD)
        strcat(buf, " anti-good");
    if (extra_flags & ITEM_ANTI_EVIL)
        strcat(buf, " anti-evil");
    if (extra_flags & ITEM_ANTI_NEUTRAL)
        strcat(buf, " anti-neutral");
    if (extra_flags & ITEM_NOREMOVE)
        strcat(buf, " noremove");
    if (extra_flags & ITEM_INVENTORY)
        strcat(buf, " inventory");
    if (extra_flags & ITEM_NOPURGE)
        strcat(buf, " nopurge");
    if (extra_flags & ITEM_VIS_DEATH)
        strcat(buf, " vis_death");
    if (extra_flags & ITEM_ROT_DEATH)
        strcat(buf, " rot_death");
    if (extra_flags & ITEM_NOLOCATE)
        strcat(buf, " no_locate");
    if (extra_flags & ITEM_SELL_EXTRACT)
        strcat(buf, " sell_extract");
    if (extra_flags & ITEM_BURN_PROOF)
        strcat(buf, " burn_proof");
    if (extra_flags & ITEM_NOUNCURSE)
        strcat(buf, " no_uncurse");
    if (extra_flags & ITEM_MELT_DROP)
        strcat(buf, " melt_drop");
    return (buf[0] != '\0') ? buf + 1 : "none";
}

/*
 * Return ascii name of an act vector
 */
char *act_bit_name(int act_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (IS_SET(act_flags, ACT_IS_NPC))
    {
        strcat(buf, " npc");
        if (act_flags & ACT_SENTINEL)
            strcat(buf, " sentinel");
        if (act_flags & ACT_SCAVENGER)
            strcat(buf, " scavenger");
        if (act_flags & ACT_AGGRESSIVE)
            strcat(buf, " aggressive");
        if (act_flags & ACT_STAY_AREA)
            strcat(buf, " stay_area");
        if (act_flags & ACT_WIMPY)
            strcat(buf, " wimpy");
        if (act_flags & ACT_PET)
            strcat(buf, " pet");
        if (act_flags & ACT_TRAIN)
            strcat(buf, " train");
        if (act_flags & ACT_PRACTICE)
            strcat(buf, " practice");
        if (act_flags & ACT_BANKER)
            strcat(buf, " banker");
        if (act_flags & ACT_UNDEAD)
            strcat(buf, " undead");
        if (act_flags & ACT_CLERIC)
            strcat(buf, " cleric");
        if (act_flags & ACT_MAGE)
            strcat(buf, " mage");
        if (act_flags & ACT_THIEF)
            strcat(buf, " thief");
        if (act_flags & ACT_WARRIOR)
            strcat(buf, " warrior");
        if (act_flags & ACT_NOALIGN)
            strcat(buf, " no_align");
        if (act_flags & ACT_NOPURGE)
            strcat(buf, " no_purge");
        if (act_flags & ACT_IS_HEALER)
            strcat(buf, " healer");
        if (act_flags & ACT_IS_CHANGER)
            strcat(buf, " changer");
        if (act_flags & ACT_GAIN)
            strcat(buf, " skill_train");
        if (act_flags & ACT_UPDATE_ALWAYS)
            strcat(buf, " update_always");
        if (act_flags & ACT_IS_PORTAL_MERCHANT)
            strcat(buf, " portalmerchant");
        if (act_flags & ACT_SAFE)
            strcat(buf, " safe");
    }
    else
    {
        strcat(buf, " player");
        if (act_flags & PLR_AUTOASSIST)
            strcat(buf, " autoassist");
        if (act_flags & PLR_AUTOEXIT)
            strcat(buf, " autoexit");
        if (act_flags & PLR_AUTOLOOT)
            strcat(buf, " autoloot");
        if (act_flags & PLR_AUTOSAC)
            strcat(buf, " autosac");
        if (act_flags & PLR_AUTOGOLD)
            strcat(buf, " autogold");
        if (act_flags & PLR_AUTOSPLIT)
            strcat(buf, " autosplit");
        if (act_flags & PLR_HOLYLIGHT)
            strcat(buf, " holy_light");
        if (act_flags & PLR_CANLOOT)
            strcat(buf, " loot_corpse");
        if (act_flags & PLR_NOSUMMON)
            strcat(buf, " no_summon");
        if (act_flags & PLR_NOFOLLOW)
            strcat(buf, " no_follow");
        if (act_flags & PLR_FREEZE)
            strcat(buf, " frozen");
        if (act_flags & PLR_WANTED)
            strcat(buf, " wanted");
        if (act_flags & PLR_NOCANCEL)
            strcat(buf, " no_cancel");
    }
    return (buf[0] != '\0') ? buf + 1 : "none";
}

/*
 * Returns the text description of the comm flags.
 */
char *comm_bit_name(int comm_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (comm_flags & COMM_QUIET)
        strcat(buf, " quiet");
    if (comm_flags & COMM_DEAF)
        strcat(buf, " deaf");
    if (comm_flags & COMM_NOWIZ)
        strcat(buf, " no_wiz");
    if (comm_flags & COMM_NOAUCTION)
        strcat(buf, " no_auction");
    if (comm_flags & COMM_NOGOSSIP)
        strcat(buf, " no_gossip");
    if (comm_flags & COMM_NOQUESTION)
        strcat(buf, " no_question");
    if (comm_flags & COMM_COMPACT)
        strcat(buf, " compact");
    if (comm_flags & COMM_BRIEF)
        strcat(buf, " brief");
    if (comm_flags & COMM_PROMPT)
        strcat(buf, " prompt");
    if (comm_flags & COMM_COMBINE)
        strcat(buf, " combine");
    if (comm_flags & COMM_NOEMOTE)
        strcat(buf, " no_emote");
    if (comm_flags & COMM_NOSHOUT)
        strcat(buf, " no_shout");
    if (comm_flags & COMM_NOTELL)
        strcat(buf, " no_tell");
    if (comm_flags & COMM_NOCHANNELS)
        strcat(buf, " no_channels");


    return (buf[0] != '\0') ? buf + 1 : "none";
}

/*
 * Returns the text description of the immunity flags.
 */
char *imm_bit_name(int imm_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (imm_flags & IMM_SUMMON)
        strcat(buf, " summon");
    if (imm_flags & IMM_CHARM)
        strcat(buf, " charm");
    if (imm_flags & IMM_MAGIC)
        strcat(buf, " magic");
    if (imm_flags & IMM_WEAPON)
        strcat(buf, " weapon");
    if (imm_flags & IMM_BASH)
        strcat(buf, " blunt");
    if (imm_flags & IMM_PIERCE)
        strcat(buf, " piercing");
    if (imm_flags & IMM_SLASH)
        strcat(buf, " slashing");
    if (imm_flags & IMM_FIRE)
        strcat(buf, " fire");
    if (imm_flags & IMM_COLD)
        strcat(buf, " cold");
    if (imm_flags & IMM_LIGHTNING)
        strcat(buf, " lightning");
    if (imm_flags & IMM_ACID)
        strcat(buf, " acid");
    if (imm_flags & IMM_POISON)
        strcat(buf, " poison");
    if (imm_flags & IMM_NEGATIVE)
        strcat(buf, " negative");
    if (imm_flags & IMM_HOLY)
        strcat(buf, " holy");
    if (imm_flags & IMM_ENERGY)
        strcat(buf, " energy");
    if (imm_flags & IMM_MENTAL)
        strcat(buf, " mental");
    if (imm_flags & IMM_DISEASE)
        strcat(buf, " disease");
    if (imm_flags & IMM_DROWNING)
        strcat(buf, " drowning");
    if (imm_flags & IMM_LIGHT)
        strcat(buf, " light");
    if (imm_flags & VULN_IRON)
        strcat(buf, " iron");
    if (imm_flags & VULN_WOOD)
        strcat(buf, " wood");
    if (imm_flags & VULN_SILVER)
        strcat(buf, " silver");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

/*
 * Returns the text description of the wear flags.
 */
char *wear_bit_name(int wear_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (wear_flags & ITEM_TAKE)
        strcat(buf, " take");
    if (wear_flags & ITEM_WEAR_FINGER)
        strcat(buf, " finger");
    if (wear_flags & ITEM_WEAR_NECK)
        strcat(buf, " neck");
    if (wear_flags & ITEM_WEAR_BODY)
        strcat(buf, " torso");
    if (wear_flags & ITEM_WEAR_HEAD)
        strcat(buf, " head");
    if (wear_flags & ITEM_WEAR_LEGS)
        strcat(buf, " legs");
    if (wear_flags & ITEM_WEAR_FEET)
        strcat(buf, " feet");
    if (wear_flags & ITEM_WEAR_HANDS)
        strcat(buf, " hands");
    if (wear_flags & ITEM_WEAR_ARMS)
        strcat(buf, " arms");
    if (wear_flags & ITEM_WEAR_SHIELD)
        strcat(buf, " shield");
    if (wear_flags & ITEM_WEAR_ABOUT)
        strcat(buf, " body");
    if (wear_flags & ITEM_WEAR_WAIST)
        strcat(buf, " waist");
    if (wear_flags & ITEM_WEAR_WRIST)
        strcat(buf, " wrist");
    if (wear_flags & ITEM_WIELD)
        strcat(buf, " wield");
    if (wear_flags & ITEM_HOLD)
        strcat(buf, " hold");
    if (wear_flags & ITEM_NO_SAC)
        strcat(buf, " nosac");
    if (wear_flags & ITEM_WEAR_FLOAT)
        strcat(buf, " float");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

/*
 * Returns the form bit names from the form flags.
 */
char *form_bit_name(int form_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (form_flags & FORM_POISON)
        strcat(buf, " poison");
    else if (form_flags & FORM_EDIBLE)
        strcat(buf, " edible");
    if (form_flags & FORM_MAGICAL)
        strcat(buf, " magical");
    if (form_flags & FORM_INSTANT_DECAY)
        strcat(buf, " instant_rot");
    if (form_flags & FORM_OTHER)
        strcat(buf, " other");
    if (form_flags & FORM_ANIMAL)
        strcat(buf, " animal");
    if (form_flags & FORM_SENTIENT)
        strcat(buf, " sentient");
    if (form_flags & FORM_UNDEAD)
        strcat(buf, " undead");
    if (form_flags & FORM_CONSTRUCT)
        strcat(buf, " construct");
    if (form_flags & FORM_MIST)
        strcat(buf, " mist");
    if (form_flags & FORM_INTANGIBLE)
        strcat(buf, " intangible");
    if (form_flags & FORM_BIPED)
        strcat(buf, " biped");
    if (form_flags & FORM_CENTAUR)
        strcat(buf, " centaur");
    if (form_flags & FORM_INSECT)
        strcat(buf, " insect");
    if (form_flags & FORM_SPIDER)
        strcat(buf, " spider");
    if (form_flags & FORM_CRUSTACEAN)
        strcat(buf, " crustacean");
    if (form_flags & FORM_WORM)
        strcat(buf, " worm");
    if (form_flags & FORM_BLOB)
        strcat(buf, " blob");
    if (form_flags & FORM_MAMMAL)
        strcat(buf, " mammal");
    if (form_flags & FORM_BIRD)
        strcat(buf, " bird");
    if (form_flags & FORM_REPTILE)
        strcat(buf, " reptile");
    if (form_flags & FORM_SNAKE)
        strcat(buf, " snake");
    if (form_flags & FORM_DRAGON)
        strcat(buf, " dragon");
    if (form_flags & FORM_AMPHIBIAN)
        strcat(buf, " amphibian");
    if (form_flags & FORM_FISH)
        strcat(buf, " fish");
    if (form_flags & FORM_COLD_BLOOD)
        strcat(buf, " cold_blooded");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

/*
 * Returns the text description of the part bit flags.
 */
char *part_bit_name(int part_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (part_flags & PART_HEAD)
        strcat(buf, " head");
    if (part_flags & PART_ARMS)
        strcat(buf, " arms");
    if (part_flags & PART_LEGS)
        strcat(buf, " legs");
    if (part_flags & PART_HEART)
        strcat(buf, " heart");
    if (part_flags & PART_BRAINS)
        strcat(buf, " brains");
    if (part_flags & PART_GUTS)
        strcat(buf, " guts");
    if (part_flags & PART_HANDS)
        strcat(buf, " hands");
    if (part_flags & PART_FEET)
        strcat(buf, " feet");
    if (part_flags & PART_FINGERS)
        strcat(buf, " fingers");
    if (part_flags & PART_EAR)
        strcat(buf, " ears");
    if (part_flags & PART_EYE)
        strcat(buf, " eyes");
    if (part_flags & PART_LONG_TONGUE)
        strcat(buf, " long_tongue");
    if (part_flags & PART_EYESTALKS)
        strcat(buf, " eyestalks");
    if (part_flags & PART_TENTACLES)
        strcat(buf, " tentacles");
    if (part_flags & PART_FINS)
        strcat(buf, " fins");
    if (part_flags & PART_WINGS)
        strcat(buf, " wings");
    if (part_flags & PART_TAIL)
        strcat(buf, " tail");
    if (part_flags & PART_CLAWS)
        strcat(buf, " claws");
    if (part_flags & PART_FANGS)
        strcat(buf, " fangs");
    if (part_flags & PART_HORNS)
        strcat(buf, " horns");
    if (part_flags & PART_SCALES)
        strcat(buf, " scales");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

/*
 * Returns the text description of the weapon flags (e.g. frost, vampiric, sharp, etc.).
 */
char *weapon_bit_name(int weapon_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (weapon_flags & WEAPON_FLAMING)
        strcat(buf, " flaming");
    if (weapon_flags & WEAPON_FROST)
        strcat(buf, " frost");
    if (weapon_flags & WEAPON_VAMPIRIC)
        strcat(buf, " vampiric");
    if (weapon_flags & WEAPON_SHARP)
        strcat(buf, " sharp");
    if (weapon_flags & WEAPON_VORPAL)
        strcat(buf, " vorpal");
    if (weapon_flags & WEAPON_TWO_HANDS)
        strcat(buf, " two-handed");
    if (weapon_flags & WEAPON_SHOCKING)
        strcat(buf, " shocking");
    if (weapon_flags & WEAPON_POISON)
        strcat(buf, " poison");
    if (weapon_flags & WEAPON_LEECH)
        strcat(buf, " leech");
    if (weapon_flags & WEAPON_STUN)
        strcat(buf, " stun");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

/*
 * Returns container bit flags as text.
 */
char *cont_bit_name(int cont_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (cont_flags & CONT_CLOSEABLE)
        strcat(buf, " closable");
    if (cont_flags & CONT_PICKPROOF)
        strcat(buf, " pickproof");
    if (cont_flags & CONT_CLOSED)
        strcat(buf, " closed");
    if (cont_flags & CONT_LOCKED)
        strcat(buf, " locked");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

/*
 * Returns mob offensive bit flags as text.
 */
char *off_bit_name(int off_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (off_flags & OFF_AREA_ATTACK)
        strcat(buf, " area attack");
    if (off_flags & OFF_BACKSTAB)
        strcat(buf, " backstab");
    if (off_flags & OFF_BASH)
        strcat(buf, " bash");
    if (off_flags & OFF_BERSERK)
        strcat(buf, " berserk");
    if (off_flags & OFF_DISARM)
        strcat(buf, " disarm");
    if (off_flags & OFF_DODGE)
        strcat(buf, " dodge");
    if (off_flags & OFF_FADE)
        strcat(buf, " fade");
    if (off_flags & OFF_FAST)
        strcat(buf, " fast");
    if (off_flags & OFF_KICK)
        strcat(buf, " kick");
    if (off_flags & OFF_KICK_DIRT)
        strcat(buf, " kick_dirt");
    if (off_flags & OFF_PARRY)
        strcat(buf, " parry");
    if (off_flags & OFF_RESCUE)
        strcat(buf, " rescue");
    if (off_flags & OFF_TAIL)
        strcat(buf, " tail");
    if (off_flags & OFF_TRIP)
        strcat(buf, " trip");
    if (off_flags & OFF_CRUSH)
        strcat(buf, " crush");
    if (off_flags & ASSIST_ALL)
        strcat(buf, " assist_all");
    if (off_flags & ASSIST_ALIGN)
        strcat(buf, " assist_align");
    if (off_flags & ASSIST_RACE)
        strcat(buf, " assist_race");
    if (off_flags & ASSIST_PLAYERS)
        strcat(buf, " assist_players");
    if (off_flags & ASSIST_GUARD)
        strcat(buf, " assist_guard");
    if (off_flags & ASSIST_VNUM)
        strcat(buf, " assist_vnum");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

/*
 * Returns the clan flags a player is flagged with.
 */
char *pc_clan_flags_lookup(int flags)
{
    static char buf[128];

    buf[0] = '\0';

    if (flags & CLAN_LEADER)
        strcat(buf, " Leader");
    if (flags & CLAN_RECRUITER)
        strcat(buf, " Recruiter");
    if (flags & CLAN_EXILE)
        strcat(buf, " Exile");

    return (buf[0] != '\0') ? buf + 1 : "none";
}

/*
 * Returns the room name for a given vnum.
 */
char *get_room_name(int vnum)
{
    static char buf[512];
    ROOM_INDEX_DATA *room;

    room = get_room_index(vnum);

    if (room != NULL && room->name != NULL)
    {
        sprintf(buf, "%s", room->name);
    }
    else
    {
        bugf("Null room->name or ROOM_INDEX_DATA for vnum %d", vnum);
        sprintf(buf, "(null)");
    }

    return buf;
}

/*
 * Returns the area name for a given vnum.
 */
char *get_area_name(int vnum)
{
    static char buf[512];
    ROOM_INDEX_DATA *room;

    room = get_room_index(vnum);

    if (room != NULL && room->area != NULL && room->area->name != NULL)
    {
        sprintf(buf, "%s", room->area->name);
    }
    else
    {
        sprintf(buf, "(null)");
    }

    return buf;
}

/*
 * If possible group obj2 into obj1                             -Thoric
 * This code, along with clone_object, obj->count, and special support
 * for it implemented throughout handler.c and save.c should show improved
 * performance on MUDs with players that hoard tons of potions and scrolls
 * as this will allow them to be grouped together both in memory, and in
 * the player files.
 */
OBJ_DATA *group_object(OBJ_DATA *obj1, OBJ_DATA *obj2)
{
    if (!obj1 || !obj2)
        return NULL;

    if (IS_OBJ_STAT(obj2, ITEM_MELT_DROP) || obj2->item_type == ITEM_CONTAINER)
        return obj2;

    if (obj1 == obj2)
        return obj1;


    if (obj1->pIndexData == obj2->pIndexData
        && !str_cmp(obj1->name, obj2->name)
        && !str_cmp(obj1->short_descr, obj2->short_descr)
        && !str_cmp(obj1->description, obj2->description)
        && obj1->item_type == obj2->item_type
        && obj1->extra_flags == obj2->extra_flags
        && obj1->wear_flags == obj2->wear_flags
        && obj1->wear_loc == obj2->wear_loc
        && obj1->weight == obj2->weight
        && obj1->cost == obj2->cost
        && obj1->level == obj2->level
        && obj1->timer == obj2->timer
        && obj1->value[0] == obj2->value[0]
        && obj1->value[1] == obj2->value[1]
        && obj1->value[2] == obj2->value[2]
        && obj1->value[3] == obj2->value[3]
        && obj1->value[4] == obj2->value[4]
        && !obj1->extra_descr && !obj2->extra_descr
        && !obj1->affected && !obj2->affected
        && !obj1->contains && !obj2->contains
        &&  obj1->count + obj2->count > 0) /* prevent count overflow */
    {
        obj1->count += obj2->count;
        obj1->pIndexData->count += obj2->count; /* to be decremented in */
        /*numobjsloaded += obj2->count;*/               /* extract_obj */
        extract_obj(obj2);
        return obj1;
    }
    return obj2;
}

void split_obj_sub(OBJ_DATA *obj, int num, bool complete)
{
    int count = obj->count;
    OBJ_DATA *rest;

    if (count <= num || num == 0)
        return;

    rest = create_object(obj->pIndexData);
    clone_object(obj, rest);

    if (!complete)
    {
        --obj->pIndexData->count;   /* since clone_object() ups this value */
        rest->count = obj->count - num;
        obj->count = num;
    }
    else
    {
        --obj->pIndexData->count;   /* since clone_object() ups this value */
        rest->count = 1;
        obj->count--;
    }


    if (obj->carried_by)
    {

        rest->next_content = obj->carried_by->carrying;
        obj->carried_by->carrying = rest;
        rest->carried_by = obj->carried_by;
        rest->in_room = NULL;
        rest->in_obj = NULL;
    }
    else
        if (obj->in_room)
        {
            rest->next_content = obj->in_room->contents;
            obj->in_room->contents = rest;
            rest->carried_by = NULL;
            rest->in_room = obj->in_room;
            rest->in_obj = NULL;
        }
        else
            if (obj->in_obj)
            {
                rest->next_content = obj->in_obj->contains;
                obj->in_obj->contains = rest;
                rest->in_obj = obj->in_obj;
                rest->in_room = NULL;
                rest->carried_by = NULL;
            }
}

void split_obj(OBJ_DATA *obj, int num)
{
    split_obj_sub(obj, num, FALSE);
}

void separate_obj(OBJ_DATA *obj)
{
    split_obj(obj, 1);
}

void ungroup_obj(OBJ_DATA *obj)
{
    for (; obj->count > 1;)
        split_obj_sub(obj, 1, TRUE);
}

/*
 * This procedure will locate all pits in the world and empty their contents.  It is currently
 * setup to ignore all ITEM_NOPURGE items, when we want to empty pits, we want to empty pits, since
 * we're saving their contents across boots, this is important.
 */
void empty_pits(void)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA * obj;
    OBJ_DATA * obj_next;
    OBJ_DATA * obj_pit_item;
    OBJ_DATA * obj_pit_item_next;
    int base_objects_purged = 0;
    int all_objects_purged = 0;
    int pits_purged = 0;

    // Look for all pits in the world.
    for (obj = object_list; obj != NULL; obj = obj_next)
    {
        obj_next = obj->next;

        if ((obj->item_type == ITEM_CONTAINER && IS_OBJ_STAT(obj, ITEM_NOPURGE))
            && obj->wear_loc == WEAR_NONE
            && obj->in_room != NULL)
        {
            // We have a pit, let's go through it's contents.
            for (obj_pit_item = obj->contains; obj_pit_item; obj_pit_item = obj_pit_item_next)
            {
                // Get the count by items that are consolidated and their total counts.
                all_objects_purged += obj_pit_item->count;
                base_objects_purged++;

                obj_pit_item_next = obj_pit_item->next_content;

                // No need to separate, we want to dump everything in the pits.  The extract
                // must be done after the next item is gotten from it.
                extract_obj(obj_pit_item);
            }

            pits_purged++;
        }
    }

    sprintf(buf, "%d pit(s) found that contained %d object(s) (%d after consolidation) were purged.", pits_purged, all_objects_purged, base_objects_purged);
    log_string(buf);

    return;

} // end empty_pits

/*
 * Whether or not a player is carrying a certain type of item in their
 * inventory (e.g. ITEM_SHOVEL, ITEM_BOAT, etc.).  This does not look
 * into the contents of containers.
 */
bool has_item_type(CHAR_DATA *ch, int item_type)
{
    OBJ_DATA * obj;

    if (ch == NULL)
        return FALSE;

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
        if (obj->item_type == item_type)
        {
            return TRUE;
        }
    }

    return FALSE;

} // end has_item_type

/*
 * Turns a player into a ghost after they die.  Ghosts receive pass door and flying, they
 * cannot be attacked (or attack) and have their movement restored so they have a fighting
 * chance at retrieving their corpse.
 */
void make_ghost(CHAR_DATA *ch)
{
    AFFECT_DATA af;

    // This probably won't happen unless an immortal kills somebody, but might as we'll
    // strip a ghost affect if they're ghosted.
    if (IS_GHOST(ch))
    {
        affect_strip(ch, gsn_ghost);
        return;
    }

    // Add a ghost affect and tact on a pass door bit vector so as a ghost you can pass
    // through doors.
    af.where = TO_AFFECTS;
    af.type = gsn_ghost;
    af.level = 0;
    af.duration = 5;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_PASS_DOOR;
    affect_to_char(ch, &af);

    // Add a fly affect for the ghost with the same duration.  We're adding this seperate
    // so the ghost can land if they want to (and go to sleep).
    if (!IS_AFFECTED(ch, AFF_FLYING))
    {
        af.where = TO_AFFECTS;
        af.type = gsn_fly;
        af.level = 0;
        af.duration = 5;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = AFF_FLYING;
        affect_to_char(ch, &af);
    }

    // Restore the ghost's movement giving them a better chance of getting back to
    // their corpse to retrieve their items.
    ch->move = ch->max_move;

    send_to_char("Your spirit leaves your corpse as your body has perished.\r\n", ch);
    return;

} // end make_ghost

/*
 * Will determine if two vnums are on the same continent.  This is a raw vnum
 * comparison that doesn't account for limbo.  Limbo != Midgaard
 */
bool same_continent(int vnum_one, int vnum_two)
{
    ROOM_INDEX_DATA *room_one;
    ROOM_INDEX_DATA *room_two;

    room_one = get_room_index(vnum_one);

    if (room_one == NULL || room_one->area == NULL)
    {
        return FALSE;
    }

    room_two = get_room_index(vnum_two);

    if (room_two == NULL || room_two->area == NULL)
    {
        return FALSE;
    }

    // Now that we have all non null stuff checked, see if they're
    // on the same continent.
    if (room_one->area->continent == room_two->area->continent)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

} // end bool same_continent

/*
 * Determines if two players are on the same continent.  Limbo is treated
 * differently in that a victim in Limbo is seen as on the same continent.
 * This will allow someone to gate to someone in Limbo, but not gate from
 * Limbo out.
 */
bool same_continent_char(CHAR_DATA *ch, CHAR_DATA *victim)
{
    // Something is null / gone wrong, return that they are not on the
    // same continent.
    if (ch == NULL || victim == NULL || ch->in_room == NULL || victim->in_room == NULL)
    {
        return FALSE;
    }

    // They are on the same continet OR the victim is in Limbo.
    if (ch->in_room->area->continent == victim->in_room->area->continent
        || victim->in_room->area->continent == 0)
    {
        return TRUE;
    }

    // They are not on the same continent.
    return FALSE;
}

/*
 * The number of hours that a player has played.  For a mob, this will return
 * the number of hours in existence (which would be since boot or the last
 * time that they re-spawned).
 */
int hours_played(CHAR_DATA *ch)
{
    if (ch == NULL)
        return 0;

    return (ch->played + (int) (current_time - ch->logon)) / 3600;
}

/*
 * Returns whether or not an instance of a specific vnum exists in the room
 * a character is in.
 */
bool obj_in_room(CHAR_DATA *ch, int vnum)
{
    OBJ_DATA *obj;

    for (obj = ch->in_room->contents; obj; obj = obj->next_content)
    {
        if (obj->pIndexData->vnum == vnum)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * Whether two players are in the same room.
 */
bool in_same_room(CHAR_DATA *ch, CHAR_DATA *victim)
{
    if (ch != NULL && victim != NULL)
    {
        return (ch->in_room == victim->in_room);
    }

    return FALSE;
}

/*
 * Determines whether the character leads group that has the specified mob
 * in it.  This important for figuring out if a player already has a charmy
 * they have summoned via a skill.
 */
bool leads_grouped_mob(CHAR_DATA *ch, int mob_vnum)
{
    CHAR_DATA *fch;

    // Make sure that the player doesn't already the specified mob grouped.
    for (fch = char_list; fch != NULL; fch = fch->next)
    {
        if (fch->master == ch && IS_NPC(fch))
        {
            if (fch->pIndexData->vnum == mob_vnum)
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

/*
 * Return ascii name of an affect bit vector.
 */
char *deity_formatted_name(CHAR_DATA *ch)
{
    static char buf[64];

    if (IS_NPC(ch))
    {
        sprintf(buf, "None");
        return buf;
    }

    // Name and description
    sprintf(buf, "%s, %s", deity_table[ch->pcdata->deity].name, deity_table[ch->pcdata->deity].description);

    return buf;
}

/*
 * Returns the standard description of a person's health.  This requires both the person looking and the
 * person receiving as it will shift their name in certain circumstances (e.g. to someone if their blind).
 */
char *health_description(CHAR_DATA *ch, CHAR_DATA *victim)
{
    static char buf[128];
    int percent;

    // Reset, we don't want to strcat onto the last copy.
    buf[0] = '\0';

    percent = percent_health(victim);

    strcpy(buf, PERS(victim, ch));

    // False life spell makes it impossible for the looker to discern the health of
    // the victim.
    if (is_affected(victim, gsn_false_life))
    {
        strcat(buf, "'s condition has been obscured by a magical field.\r\n");
        buf[0] = UPPER(buf[0]);
        return buf;
    }

    if (percent >= 100)
    {
        strcat(buf, " is in excellent condition.\r\n");
    }
    else if (percent >= 90)
    {
        strcat(buf, " has a few scratches.\r\n");
    }
    else if (percent >= 75)
    {
        strcat(buf, " has some small wounds and bruises.\r\n");
    }
    else if (percent >= 50)
    {
        strcat(buf, " has quite a few wounds.\r\n");
    }
    else if (percent >= 30)
    {
        strcat(buf, " has some big nasty wounds and scratches.\r\n");
    }
    else if (percent >= 15)
    {
        strcat(buf, " looks pretty hurt.\r\n");
    }
    else if (percent >= 0)
    {
        strcat(buf, " is in awful condition.\r\n");
    }
    else
    {
        strcat(buf, " is {rbleeding{x to death.\r\n");
    }

    buf[0] = UPPER(buf[0]);

    return buf;
}

/*
 * Returns the standard description of a person's amount of mana.  This requires both the person looking and the
 * person receiving as it will shift their name in certain circumstances (e.g. to someone if their blind).
 */
char *mana_description(CHAR_DATA *ch, CHAR_DATA *victim)
{
    static char buf[128];
    int percent = 0;

    // Detect magic will show you how much mana a person has left
    if (victim->max_mana > 0)
    {
        percent = (100 * victim->mana) / victim->max_mana;
    }

    if (percent >= 100)
    {
        sprintf(buf, "%s has full magical ability.\r\n", PERS(victim, ch));
    }
    else if (percent >= 75)
    {
        sprintf(buf, "%s has a good amount of magical ability left.\r\n", PERS(victim, ch));
    }
    else if (percent >= 40)
    {
        sprintf(buf, "%s has a fair amount of magical ability left.\r\n", PERS(victim, ch));
    }
    else if (percent >= 25)
    {
        sprintf(buf, "%s has a low amount of magical ability left.\r\n", PERS(victim, ch));
    }
    else if (percent >= 1)
    {
        sprintf(buf, "%s has an almost depleted amount of magical ability left.\r\n", PERS(victim, ch));
    }
    else
    {
        sprintf(buf, "%s has an no magical ability left.\r\n", PERS(victim, ch));
    }

    buf[0] = UPPER(buf[0]);

    return buf;
}

/*
 * Returns an int percentage of the person's remaining health (e.g. 74, not .74).
 */
int percent_health(CHAR_DATA *ch)
{
    if (ch->max_hit > 0)
    {
        return (100 * ch->hit) / ch->max_hit;
    }
    else
    {
        return -1;
    }
}

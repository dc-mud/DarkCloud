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
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt' as well as the ROM license.  In particular,   *
 *  you may not remove these copyright notices.                            *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 **************************************************************************/

/***************************************************************************
*  Magic                                                                   *
*                                                                          *
*  This file contains all of the base logic for magic and casting as well  *
*  as spells that are generally shared and not specific to a type of magic *
*  like mage or clerical spells.                                           *
*                                                                          *
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

// General Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"

/*
 * Local functions.
 */
void say_spell(CHAR_DATA * ch, int sn);

int find_spell(CHAR_DATA * ch, const char *name)
{
    /* finds a spell the character can cast if possible */
    int sn, found = -1;

    if (IS_NPC(ch))
    {
        return skill_lookup(name);
    }

    for (sn = 0; sn < top_sn; sn++)
    {
        if (skill_table[sn]->name == NULL)
            break;

        if (LOWER(name[0]) == LOWER(skill_table[sn]->name[0])
            && !str_prefix(name, skill_table[sn]->name))
        {
            if (ch->level >= skill_table[sn]->skill_level[ch->class] && ch->pcdata->learned[sn] > 0)
            {
                return sn;
            }
        }
    }

    return found;
}

/*
 * Utter mystical words for an sn.
 */
void say_spell(CHAR_DATA * ch, int sn)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    CHAR_DATA *rch;
    char *pName;
    int iSyl;
    int length;

    struct syl_type {
        char *old;
        char *new;
    };

    static const struct syl_type syl_table[] = {
        {" ", " "},
        {"ar", "abra"},
        {"au", "kada"},
        {"bless", "fido"},
        {"blind", "nose"},
        {"bur", "mosa"},
        {"cu", "judi"},
        {"de", "oculo"},
        {"en", "unso"},
        {"light", "dies"},
        {"lo", "hi"},
        {"mor", "zak"},
        {"move", "sido"},
        {"ness", "lacri"},
        {"ning", "illa"},
        {"per", "duda"},
        {"ra", "gru"},
        {"fresh", "ima"},
        {"re", "candus"},
        {"son", "sabru"},
        {"tect", "infra"},
        {"tri", "cula"},
        {"ven", "nofo"},
        {"a", "a"}, {"b", "b"}, {"c", "q"}, {"d", "e"},
        {"e", "z"}, {"f", "y"}, {"g", "o"}, {"h", "p"},
        {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"},
        {"m", "w"}, {"n", "i"}, {"o", "a"}, {"p", "s"},
        {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"},
        {"u", "j"}, {"v", "z"}, {"w", "x"}, {"x", "n"},
        {"y", "l"}, {"z", "k"},
        {"", ""}
    };

    buf[0] = '\0';
    for (pName = skill_table[sn]->name; *pName != '\0'; pName += length)
    {
        for (iSyl = 0; (length = strlen(syl_table[iSyl].old)) != 0; iSyl++)
        {
            if (!str_prefix(syl_table[iSyl].old, pName))
            {
                strcat(buf, syl_table[iSyl].new);
                break;
            }
        }

        if (length == 0)
            length = 1;
    }

    sprintf(buf2, "$n utters the words, '%s'.", buf);
    sprintf(buf, "$n utters the words, '%s'.", skill_table[sn]->name);

    for (rch = ch->in_room->people; rch; rch = rch->next_in_room)
    {
        if (rch != ch)
        {
            if (CHANCE_SKILL(rch, gsn_spellcraft))
            {
                // Spellcraft allows the person to see what spell is being cast
                act(buf, ch, NULL, rch, TO_VICT);
            }
            else
            {
                // If the player is the same class they can see what's being cast, otherwise it's the
                // magicky looking form
                act((!IS_NPC(rch) && ch->class == rch->class) ? buf : buf2, ch, NULL, rch, TO_VICT);
            }
        }
    }

    return;
}

/*
 * Returns the casting level that the character casts at.
 */
int casting_level(CHAR_DATA *ch)
{
    int level;

    level = ch->level;

    // NPC's cast at level (if they even cast)
    if (IS_NPC(ch))
        return level;

    // Non mana classes cast below level, a level 51 will cast around level 38/39.
    if (!class_table[ch->class]->mana)
    {
        level = 3 * level / 4;
    }

    // Ogre's lose a casting level
    if (ch->race == OGRE_RACE || ch->race == GIANT_OGRE_RACE)
    {
        level -= 1;
    }

    // Elves gain a casting level
    if (ch->race == ELF_RACE)
    {
        level += 1;
    }

    // Spellcraft skill gains a bonus, making it a super useful skill, this will
    // require the skill check to pass though so it's not a given.
    if (CHANCE_SKILL(ch, gsn_spellcraft))
    {
        level += 1;
    }

    // Merit - Magic Affinity
    if (!IS_NPC(ch) && IS_SET(ch->pcdata->merit, MERIT_MAGIC_AFFINITY))
    {
        level += 1;
    }

    // Mage and mage reclassese get a bonus casting level, they are the
    // masters of spells.
    if (ch->class == MAGE_CLASS
        || ch->class == PSIONICIST_CLASS
        || ch->class == WIZARD_CLASS
        || ch->class == ENCHANTER_CLASS)
    {
        level += 1;
    }

    // Imbue raises the casting level, if the player is affected by it then get
    // the modifier off that affect (e.g. lower level characters raise the level
    // by less).
    if (is_affected(ch, gsn_imbue))
    {
        AFFECT_DATA *paf;

        for (paf = ch->affected; paf != NULL; paf = paf->next)
        {
            if (paf->type == gsn_imbue)
            {
                level += paf->modifier;
            }
        }
    }

    // Wizard's feeble mind can lower a casting level.
    if (is_affected(ch, gsn_feeble_mind))
    {
        level -= 1;
    }

    return level;

} // end casting_level

/*
 * Returns the saves for a player.  Saves are calculated elsewhere and stored in the
 * saving_throw field.  This will return that with any other real time code based
 * modifications we want to make such as bonuses or penalities based off of stats
 * or those for class/race combos, etc.
 */
int get_saves(CHAR_DATA * ch)
{
    int saves = 0;

    if (ch != NULL)
    {
        // Get their saves
        saves = ch->saving_throw;

        // If they are a player, run through any mods we have.
        if (!IS_NPC(ch))
        {
            saves += wis_app[get_curr_stat(ch, STAT_WIS)].saves_bonus;
        }

        return saves;
    }
    else
    {
        return saves;
    }
}

/*
 * Compute a saving throw.
 * Negative apply's to saving_throw make saving throw better (subtracting a negative raises
 * the chance of saving), adding to the save variable here for certain cases also helps the
 * victim.
 */
bool saves_spell(int level, CHAR_DATA * victim, int dam_type)
{
    int save;

    save = 50 + (victim->level - level) * 5 - get_saves(victim) * 2;

    // Beserk offers some magic resistance
    if (IS_AFFECTED(victim, AFF_BERSERK))
    {
        save += victim->level / 2;
    }

    // Check immunity, resistance and vulnerabilty
    switch (check_immune(victim, dam_type))
    {
        case IS_IMMUNE:
            return TRUE;
        case IS_RESISTANT:
            save += 2;
            break;
        case IS_VULNERABLE:
            save -= 2;
            break;
    }

    // Modifications for class and criteria
    switch (victim->class)
    {
        case RANGER_CLASS:
            // Rangers get bonus in the forest.
            if (victim->in_room != NULL && victim->in_room->sector_type == SECT_FOREST)
            {
                save += 1;
            }
        break;
    }

    if (!IS_NPC(victim) && class_table[victim->class]->mana)
    {
        save = 9 * save / 10;
    }

    save = URANGE(5, save, 95);
    return number_percent() < save;
}

/* RT save for dispels */

bool saves_dispel(int dis_level, int spell_level, int duration)
{
    int save;

    if (duration == -1)
        spell_level += 5;
    /* very hard to dispel permanent effects */

    save = 50 + (spell_level - dis_level) * 5;
    save = URANGE(5, save, 95);
    return number_percent() < save;
}

/* co-routine for dispel magic and cancellation */

bool check_dispel(int dis_level, CHAR_DATA * victim, int sn)
{
    AFFECT_DATA *af;

    if (is_affected(victim, sn))
    {
        for (af = victim->affected; af != NULL; af = af->next)
        {
            if (af->type == sn)
            {
                if (!saves_dispel(dis_level, af->level, af->duration))
                {
                    affect_strip(victim, sn);
                    if (skill_table[sn]->msg_off)
                    {
                        send_to_char(skill_table[sn]->msg_off, victim);
                        send_to_char("\r\n", victim);
                    }
                    return TRUE;
                }
                else
                {
                    af->level--;
                }
            }
        }
    }
    return FALSE;
}

/*
 * The kludgy global is for spells who want more stuff from command line.
 */
char *target_name;

void do_cast(CHAR_DATA * ch, char *argument)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    void *vo;
    int mana;
    int sn;
    int target;

    /*
     * Switched NPC's can cast spells, but others can't.
     */
    if (IS_NPC(ch) && ch->desc == NULL)
        return;

    target_name = one_argument(argument, arg1);
    one_argument(target_name, arg2);

    if (arg1[0] == '\0')
    {
        send_to_char("Cast which what where?\r\n", ch);
        return;
    }

    // This will make sure they have the spell and can use it.
    if ((sn = find_spell(ch, arg1)) < 1 || skill_table[sn]->spell_fun == spell_null)
    {
        send_to_char("You don't know any spells of that name.\r\n", ch);
        return;
    }

    if (ch->position < skill_table[sn]->minimum_position)
    {
        send_to_char("You can't concentrate enough.\r\n", ch);
        return;
    }

    // Spells used to be a higher cost the first few levels you had then, I've removed that
    // so that a spell costs what it costs.  You still have to practice it to become proficient
    // in it.
    mana = skill_table[sn]->min_mana;

    /*
     * Locate targets.
     */
    victim = NULL;
    obj = NULL;
    vo = NULL;
    target = TARGET_NONE;

    switch (skill_table[sn]->target)
    {
        default:
            bug("Do_cast: bad target for sn %d.", sn);
            return;

        case TAR_IGNORE:
            break;

        case TAR_CHAR_OFFENSIVE:
            if (arg2[0] == '\0')
            {
                if ((victim = ch->fighting) == NULL)
                {
                    send_to_char("Cast the spell on whom?\r\n", ch);
                    return;
                }
            }
            else
            {
                // This will get the target from a ranged room if it's ranged, otherwise it
                // will get the target fromt he current room as per usual.
                if ((victim = get_target(ch, target_name, skill_table[sn]->ranged)) == NULL)
                {
                    send_to_char( "They aren't here.\r\n", ch );
                    return;
                }
            }

            // Make sure the targeted room isn't safe by ownership.
            if (skill_table[sn]->ranged
                && ((victim && victim->in_room && !IS_NULLSTR(victim->in_room->owner)) || (ch && ch->in_room && !IS_NULLSTR(ch->in_room->owner))))
            {
                send_to_char("Not in that room.\r\n",ch);
                return;
            }

            if (!IS_NPC(ch))
            {
                if (is_safe(ch, victim) && victim != ch)
                {
                    send_to_char("Not on that target.\r\n", ch);
                    return;
                }

                check_wanted(ch, victim);
            }

            if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
            {
                send_to_char("You can't do that on your own follower.\r\n", ch);
                return;
            }

            vo = (void *)victim;
            target = TARGET_CHAR;
            break;

        case TAR_CHAR_DEFENSIVE:
            if (arg2[0] == '\0')
            {
                victim = ch;
            }
            else
            {
                if ((victim = get_char_room(ch, target_name)) == NULL)
                {
                    send_to_char("They aren't here.\r\n", ch);
                    return;
                }
            }

            vo = (void *)victim;
            target = TARGET_CHAR;
            break;

        case TAR_CHAR_SELF:
            if (arg2[0] != '\0' && !is_name(target_name, ch->name))
            {
                send_to_char("You cannot cast this spell on another.\r\n", ch);
                return;
            }

            vo = (void *)ch;
            target = TARGET_CHAR;
            break;

        case TAR_OBJ_INV:
            if (arg2[0] == '\0')
            {
                send_to_char("What should the spell be cast upon?\r\n", ch);
                return;
            }

            if ((obj = get_obj_carry(ch, target_name, ch)) == NULL)
            {
                send_to_char("You are not carrying that.\r\n", ch);
                return;
            }

            vo = (void *)obj;
            target = TARGET_OBJ;
            break;

        case TAR_OBJ_CHAR_OFF:
            if (arg2[0] == '\0')
            {
                if ((victim = ch->fighting) == NULL)
                {
                    send_to_char("Cast the spell on whom or what?\r\n", ch);
                    return;
                }

                target = TARGET_CHAR;
            }
            else if ((victim = get_char_room(ch, target_name)) != NULL)
            {
                target = TARGET_CHAR;
            }

            if (target == TARGET_CHAR)
            {                    /* check the sanity of the attack */
                if (is_safe_spell(ch, victim, FALSE) && victim != ch)
                {
                    send_to_char("Not on that target.\r\n", ch);
                    return;
                }

                if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim)
                {
                    send_to_char
                        ("You can't do that on your own follower.\r\n", ch);
                    return;
                }

                if (!IS_NPC(ch))
                {
                    check_wanted(ch, victim);
                }

                vo = (void *)victim;
            }
            else if ((obj = get_obj_here(ch, target_name)) != NULL)
            {
                vo = (void *)obj;
                target = TARGET_OBJ;
            }
            else
            {
                send_to_char("You don't see that here.\r\n", ch);
                return;
            }
            break;

        case TAR_OBJ_CHAR_DEF:
            if (arg2[0] == '\0')
            {
                vo = (void *)ch;
                target = TARGET_CHAR;
            }
            else if ((victim = get_char_room(ch, target_name)) != NULL)
            {
                vo = (void *)victim;
                target = TARGET_CHAR;
            }
            else if ((obj = get_obj_carry(ch, target_name, ch)) != NULL)
            {
                vo = (void *)obj;
                target = TARGET_OBJ;
            }
            else
            {
                send_to_char("You don't see that here.\r\n", ch);
                return;
            }
            break;
    }

    // If the character is affected by mental weight then all mana costs are
    // doubled.
    if (is_affected(ch, gsn_mental_weight))
    {
        mana *= 2;
    }

    if (!IS_NPC(ch) && ch->mana < mana)
    {
        send_to_char("You don't have enough mana.\r\n", ch);
        return;
    }

    if (str_cmp(skill_table[sn]->name, "ventriloquate"))
    {
        say_spell(ch, sn);
    }

    // How long does this spell take to complete?  The character will be lagged for
    // a specified amoutn of beats.  Immortals are excempt from waiting.
    if (!IS_IMMORTAL(ch))
    {
        if (skill_table[sn]->beats > 24 && CHANCE_SKILL(ch, gsn_spellcraft))
        {
            // If the beats of the spell are real long but the user has spellcraft and
            // it passes the check, cut the time in two.
            WAIT_STATE(ch, skill_table[sn]->beats / 2);
        }
        else
        {
            // This is the normal casting case that most everyone will get to
            WAIT_STATE(ch, skill_table[sn]->beats);
        }
    }

    if (number_percent() > get_skill(ch, sn))
    {
        send_to_char("You lost your concentration.\r\n", ch);
        check_improve(ch, sn, FALSE, 1);
        check_improve(ch, gsn_spellcraft, FALSE, 1);

        // If they lose concentration, remove the cost / 2, but if the spellcraft
        // check is successful then they don't lose mana.
        if (!CHANCE_SKILL(ch, gsn_spellcraft))
        {
            ch->mana -= mana / 2;
        }

    }
    else
    {
        // Cast the spell
        ch->mana -= mana;
        (*skill_table[sn]->spell_fun) (sn, casting_level(ch), ch, vo, target);

        check_improve(ch, sn, TRUE, 1);
        check_improve(ch, gsn_spellcraft, TRUE, 1);
    }

    if ((skill_table[sn]->target == TAR_CHAR_OFFENSIVE
        || (skill_table[sn]->target == TAR_OBJ_CHAR_OFF
        && target == TARGET_CHAR)) && victim != ch
        && victim->master != ch)
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        for (vch = ch->in_room->people; vch; vch = vch_next)
        {
            vch_next = vch->next_in_room;
            if (victim == vch && victim->fighting == NULL)
            {
                check_wanted(victim, ch);
                multi_hit(victim, ch, TYPE_UNDEFINED);
                break;
            }
        }
    }

    return;
}

/*
 * Cast spells at targets using a magical object.
 */
void obj_cast_spell(int sn, int level, CHAR_DATA * ch, CHAR_DATA * victim, OBJ_DATA * obj)
{
    void *vo;
    int target = TARGET_NONE;

    if (sn <= 0)
        return;

    if (sn >= MAX_SKILL || skill_table[sn]->spell_fun == 0)
    {
        bug("Obj_cast_spell: bad sn %d.", sn);
        return;
    }

    switch (skill_table[sn]->target)
    {
        default:
            bug("Obj_cast_spell: bad target for sn %d.", sn);
            return;

        case TAR_IGNORE:
            vo = NULL;
            break;

        case TAR_CHAR_OFFENSIVE:
            if (victim == NULL)
                victim = ch->fighting;
            if (victim == NULL)
            {
                send_to_char("You can't do that.\r\n", ch);
                return;
            }
            if (is_safe(ch, victim) && ch != victim)
            {
                send_to_char("Something isn't right...\r\n", ch);
                return;
            }
            vo = (void *)victim;
            target = TARGET_CHAR;
            break;

        case TAR_CHAR_DEFENSIVE:
        case TAR_CHAR_SELF:
            if (victim == NULL)
                victim = ch;
            vo = (void *)victim;
            target = TARGET_CHAR;
            break;

        case TAR_OBJ_INV:
            if (obj == NULL)
            {
                send_to_char("You can't do that.\r\n", ch);
                return;
            }
            vo = (void *)obj;
            target = TARGET_OBJ;
            break;

        case TAR_OBJ_CHAR_OFF:
            if (victim == NULL && obj == NULL)
            {
                if (ch->fighting != NULL)
                    victim = ch->fighting;
                else
                {
                    send_to_char("You can't do that.\r\n", ch);
                    return;
                }
            }

            if (victim != NULL)
            {
                if (is_safe_spell(ch, victim, FALSE) && ch != victim)
                {
                    send_to_char("Somehting isn't right...\r\n", ch);
                    return;
                }

                vo = (void *)victim;
                target = TARGET_CHAR;
            }
            else
            {
                vo = (void *)obj;
                target = TARGET_OBJ;
            }
            break;


        case TAR_OBJ_CHAR_DEF:
            if (victim == NULL && obj == NULL)
            {
                vo = (void *)ch;
                target = TARGET_CHAR;
            }
            else if (victim != NULL)
            {
                vo = (void *)victim;
                target = TARGET_CHAR;
            }
            else
            {
                vo = (void *)obj;
                target = TARGET_OBJ;
            }

            break;
    }

    target_name = "";
    (*skill_table[sn]->spell_fun) (sn, level, ch, vo, target);



    if ((skill_table[sn]->target == TAR_CHAR_OFFENSIVE
        || (skill_table[sn]->target == TAR_OBJ_CHAR_OFF
            && target == TARGET_CHAR)) && victim != ch
        && victim->master != ch)
    {
        CHAR_DATA *vch;
        CHAR_DATA *vch_next;

        for (vch = ch->in_room->people; vch; vch = vch_next)
        {
            vch_next = vch->next_in_room;
            if (victim == vch && victim->fighting == NULL)
            {
                check_wanted(victim, ch);
                multi_hit(victim, ch, TYPE_UNDEFINED);
                break;
            }
        }
    }

    return;
}

/*
 * Gets a target with the ability to find targets in other rooms if specified.
 */
CHAR_DATA *get_target(CHAR_DATA *ch, char *argument, bool ranged)
{
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];
    CHAR_DATA *victim = NULL;
    int number;
    int count = 0;
    bool found = FALSE;
    ROOM_INDEX_DATA *scan_room;
    EXIT_DATA *pExit;
    int door = 0;
    int depth = 0;
    int max_depth = 4;

    //int spell_range = 0;
    //int spell_dir = -1;

    argument = one_argument(argument, arg);
    number = number_argument(argument, arg2);
    sprintf(arg3, "%s", arg);
    //spell_dir = -1;

    if (ranged)
    {
        if (!str_cmp(arg, "n") || !str_cmp(arg, "north"))
        {
            door = 0;
        }
        else if (!str_cmp(arg, "e") || !str_cmp(arg, "east"))
        {
            door = 1;
        }
        else if (!str_cmp(arg, "s") || !str_cmp(arg, "south"))
        {
            door = 2;
        }
        else if (!str_cmp(arg, "w") || !str_cmp(arg, "west"))
        {
            door = 3;
        }
        else if (!str_cmp(arg, "u") || !str_cmp(arg, "up"))
        {
            door = 4;
        }
        else if (!str_cmp(arg, "d") || !str_cmp(arg, "down"))
        {
            door = 5;
        }
       else if (!str_cmp(arg, "nw") || !str_cmp(arg, "northwest"))
        {
            door = 6;
        }
        else if (!str_cmp(arg, "ne") || !str_cmp(arg, "northeast"))
        {
            door = 7;
        }
        else if (!str_cmp(arg, "sw") || !str_cmp(arg, "southwest"))
        {
            door = 8;
        }
        else if (!str_cmp(arg, "se") || !str_cmp(arg, "southeast"))
        {
            door = 9;
        }
        else
        {
            //spell_range = 0;
        }

        argument = one_argument(argument,arg);
        scan_room = ch->in_room;

        for (depth = 1; depth < max_depth; depth++)
        {
            if ((pExit = scan_room->exit[door]) != NULL)
            {
                // If the door is closed then you can't see through it.
                if (IS_SET(pExit->exit_info, EX_CLOSED))
                {
                    break;
                }

                scan_room = pExit->u1.to_room;

                for (victim = scan_room->people; victim != NULL; victim = victim->next_in_room)
                {
                    if (!can_see(ch, victim) || !is_name(arg, victim->name))
                    {
                        continue;
                    }

                    if (++count == number)
                    {
                        found = TRUE;
                        //spell_dir = door;
                        break;
                    }
                }

                if (found)
                {
                    //spell_range = depth;
                    break;
                }


            }
        }
    }

    if (!found)
    {
        if ((victim = get_char_room(ch, arg3)) != NULL)
        {
            found = TRUE;
            //spell_range = 0;
        }
    }

    if (found)
    {
        return victim;
    }
    else
    {
        return NULL;
    }
}

/*
 * Spell functions.
 */

/*
 * This contains the spells that can be dispelled or cancelled.  Those spells will
 * loop through these.  This table requires a GSN and an optional ACT message that
 * the room should see (leave it blank if not message is desired).
 */
const struct dispel_type dispel_table[] = {
    { &gsn_armor, "$n is no longer armored."},
    { &gsn_bless, "$n is no longer blessed."},
    { &gsn_blindness, "$n is no longer blinded."},
    { &gsn_calm, "$n no longer looks so peaceful..."},
    { &gsn_change_sex, "$n looks more like $mself again."},
    { &gsn_charm_person, "$n regains $s free will."},
    { &gsn_chill_touch, "$n looks warmer."},
    { &gsn_curse, ""},
    { &gsn_detect_evil, "$n can no longer detect evil."},
    { &gsn_detect_good, "$n can no longer detect good."},
    { &gsn_detect_hidden, "$n can no longer detect hidden."},
    { &gsn_detect_invis, "$n can no longer detect invisibility."},
    { &gsn_detect_magic, "$n can no longer detect magic."},
    { &gsn_detect_fireproof, ""},
    { &gsn_enhanced_recovery, ""},
    { &gsn_faerie_fire, "The pink outline around $n fades away."},
    { &gsn_fly, "$n falls to the ground!"},
    { &gsn_frenzy, "$n no longer looks so wild."},
    { &gsn_giant_strength, "$n no longer looks so mighty."},
    { &gsn_haste, "$n is no longer moving so quickly."},
    { &gsn_infravision, ""},
    { &gsn_invisibility, "$n fades into existance."},
    { &gsn_mass_invisibility, "$n fades into existance."},
    { &gsn_pass_door, "&n is no longer translucent"},
    { &gsn_protection_evil, "$n is no longer protected."},
    { &gsn_protection_good, "$n is no longer protected."},
    { &gsn_protection_neutral, "$n is no longer protected."},
    { &gsn_sanctuary, "The white aura around $n's body vanishes."},
    { &gsn_shield, "The shield protecting $n vanishes."},
    { &gsn_sleep, ""},
    { &gsn_slow, "$n is no longer moving so slowly."},
    { &gsn_stone_skin, "$n's skin regains its normal texture."},
    { &gsn_weaken, "$n looks stronger."},
    { &gsn_enchant_person, "$n no longer looks as if $e is enchanted."},
    { &gsn_water_breathing, "$n's breathing returns to normal."},
    { &gsn_vitalizing_presence, "The vitalizing presence leaves $n's body."},
    { &gsn_bark_skin, "$n's skin loses it's bark like texture."},
    { &gsn_self_growth, "$n no longer looks as vitalized."},
    { &gsn_life_boost, "$n no longer looks as vitalized."},
    { &gsn_sense_affliction, "$n can no longer sense afflication."},
    { &gsn_mental_weight, ""},
    { &gsn_clairvoyance, "$n is no longer clairvoyant."},
    { &gsn_imbue, "$n's magical ability is no longer augmented."},
    { &gsn_forget, "$n's mind looks clearer."},
    { &gsn_psionic_focus, "$n's mind looks less focused."},
    { &gsn_psionic_shield, "$n's psionic shield dissipates."},
    { &gsn_song_of_protection, ""},
    { &gsn_song_of_dissonance, ""},
    { &gsn_magic_resistance, "$n no longer looks resistant to magic."},
    { &gsn_agony, "The veil of agony over $n lifts."},
    { &gsn_holy_presence, "The holy presence protecting $n fades."},
    { &gsn_holy_flame, "The holy flames around $n fade."},
    { &gsn_divine_wisdom, "The divine wisdom leaves $n's soul."},
    { &gsn_water_walk, "$n's blessing of water walking fades."},
    { &gsn_self_projection, "$n's self projection disappears."},
    { &gsn_heaviness, "The magical weight affecting $n has lifted."},
    { &gsn_feeble_mind, "$n looks as if a fog has been lifted from their mind."},
    { &gsn_dark_vision, ""},
    { &gsn_mental_presence, "The aura of mental clarity surrounding $n fades."},
    { &gsn_false_life, ""},
    {NULL, NULL}
};


/*
 * Armor spell - increases a targets armor class.
 *
 * You can re-case this on yourself, but you can't recase it on others
 * if they're already affected by it.
 */
void spell_armor(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            // You cannot re-add this spell to someone else, this will stop people with lower
            // casting levels from replacing someone elses spells.
            act("$N is already armored.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 24;
    af.modifier = -20;
    af.location = APPLY_AC;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("You feel someone protecting you.\r\n", victim);

    if (ch != victim)
    {
        act("$N is protected by your magic.", ch, NULL, victim, TO_CHAR);
    }

    return;
}

void spell_blindness(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_BLIND))
    {
        send_to_char("They are already blinded.\r\n", ch);
        return;
    }

    if (saves_spell(level, victim, DAM_OTHER))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.location = APPLY_HITROLL;
    af.modifier = -4;
    af.duration = 1 + level;
    af.bitvector = AFF_BLIND;
    affect_to_char(victim, &af);
    send_to_char("You are blinded!\r\n", victim);
    act("$n appears to be blinded.", victim, NULL, NULL, TO_ROOM);
    return;
}


void spell_call_lightning(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;

    if (!IS_OUTSIDE(ch))
    {
        send_to_char("You must be out of doors.\r\n", ch);
        return;
    }

    if (weather_info.sky < SKY_RAINING)
    {
        send_to_char("You need bad weather.\r\n", ch);
        return;
    }

    dam = dice(level / 2, 8);

    send_to_char("Lightning strikes your foes!\r\n", ch);
    act("$n calls lightning to strike $s foes!", ch, NULL, NULL, TO_ROOM);

    for (vch = char_list; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next;

        if (vch->in_room == NULL)
        {
            continue;
        }

        if (vch->in_room == ch->in_room)
        {
            if (vch != ch && !is_same_group(ch, vch))
            {
                damage(ch, vch, saves_spell(level, vch, DAM_LIGHTNING) ? dam / 2 : dam, sn, DAM_LIGHTNING, TRUE);
            }

            continue;
        }

        if (vch->in_room->area == ch->in_room->area
            && IS_OUTSIDE(vch)
            && IS_AWAKE(vch))
            send_to_char("Lightning flashes in the sky.\r\n", vch);
    }

    return;
}

void spell_cancellation(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    bool found = FALSE;
    int x = 0;

    level += 2;

    if ((!IS_NPC(ch) && IS_NPC(victim)
         && !(IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim))
         || (IS_NPC(ch) && !IS_NPC(victim) && !IS_SET(ch->act, ACT_IS_HEALER))
         || (!IS_NPC(victim) && ch != victim && IS_SET(victim->act, PLR_NOCANCEL)))
    {
        send_to_char("You failed, try dispel magic.\r\n", ch);
        return;
    }

    // Unlike dispel magic, the victim gets NO save

    // Begin running through the spells in the dispel_table, if there is an
    // act message to show to the room show it, otherwise it will be silent.
    for (x = 0; dispel_table[x].gsn != NULL; x++)
    {
        if (check_dispel(level, victim, *dispel_table[x].gsn))
        {
            found = TRUE;

            // Check for the room message
            if (!IS_NULLSTR(dispel_table[x].room_msg))
            {
                act(dispel_table[x].room_msg, victim, NULL, NULL, TO_ROOM);
            }
        }
    }

    if (found)
    {
        send_to_char("Ok.\r\n", ch);
    }
    else
    {
        send_to_char("Spell failed.\r\n", ch);
    }
}

void spell_change_sex(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
        if (victim == ch)
        {
            send_to_char("You've already been changed.\r\n", ch);
        }
        else
        {
            act("$N has already had $s(?) sex changed.", ch, NULL, victim, TO_CHAR);
        }

        return;
    }

    if (saves_spell(level, victim, DAM_OTHER))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 2 * level;
    af.location = APPLY_SEX;

    do
    {
        af.modifier = number_range(0, 2) - victim->sex;
    } while (af.modifier == 0);

    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("You feel different.\r\n", victim);
    act("$n doesn't look like $mself anymore...", victim, NULL, NULL, TO_ROOM);

    return;
}

void spell_continual_light(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    OBJ_DATA *light;

    if (target_name[0] != '\0')
    {                            /* do a glow on some object */
        light = get_obj_carry(ch, target_name, ch);

        if (light == NULL)
        {
            send_to_char("You don't see that here.\r\n", ch);
            return;
        }

        separate_obj(light);

        if (IS_OBJ_STAT(light, ITEM_GLOW))
        {
            REMOVE_BIT(light->extra_flags, ITEM_INVIS);
            act("$p is already glowing.", ch, light, NULL, TO_CHAR);
            return;
        }

        SET_BIT(light->extra_flags, ITEM_GLOW);

        // Remove the invisible flag if continual light is cast on the item.
        REMOVE_BIT(light->extra_flags, ITEM_INVIS);

        act("$p glows with a white light.", ch, light, NULL, TO_ALL);
        return;
    }

    light = create_object(get_obj_index(OBJ_VNUM_LIGHT_BALL));
    obj_to_room(light, ch->in_room);

    act("$n twiddles $s thumbs and $p appears.", ch, light, NULL, TO_ROOM);
    act("You twiddle your thumbs and $p appears.", ch, light, NULL, TO_CHAR);

    return;
}



void spell_control_weather(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    if (!str_cmp(target_name, "better"))
    {
        weather_info.change += dice(level / 3, 4);
    }
    else if (!str_cmp(target_name, "worse"))
    {
        weather_info.change -= dice(level / 3, 4);
    }
    else
    {
        send_to_char("Do you want it to get better or worse?\r\n", ch);
    }

    send_to_char("Ok.\r\n", ch);
    return;
}



void spell_create_food(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    OBJ_DATA *mushroom;

    mushroom = create_object(get_obj_index(OBJ_VNUM_MUSHROOM));
    mushroom->value[0] = level / 2;
    mushroom->value[1] = level;
    obj_to_room(mushroom, ch->in_room);
    act("$p suddenly appears.", ch, mushroom, NULL, TO_ROOM);
    act("$p suddenly appears.", ch, mushroom, NULL, TO_CHAR);
    return;
}

void spell_create_rose(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    OBJ_DATA *rose;

    rose = create_object(get_obj_index(OBJ_VNUM_ROSE));
    act("$n has created a beautiful red rose.", ch, rose, NULL, TO_ROOM);
    send_to_char("You create a beautiful red rose.\r\n", ch);
    obj_to_char(rose, ch);
    return;
}

void spell_create_spring(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    OBJ_DATA *spring;

    spring = create_object(get_obj_index(OBJ_VNUM_SPRING));
    spring->timer = level;
    obj_to_room(spring, ch->in_room);
    act("$p flows from the ground.", ch, spring, NULL, TO_ROOM);
    act("$p flows from the ground.", ch, spring, NULL, TO_CHAR);
    return;
}

void spell_create_water(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *)vo;
    int water;

    if (obj->item_type != ITEM_DRINK_CON)
    {
        send_to_char("It is unable to hold water.\r\n", ch);
        return;
    }

    if (obj->value[2] != LIQ_WATER && obj->value[1] != 0)
    {
        send_to_char("It contains some other liquid.\r\n", ch);
        return;
    }

    water = UMIN(level * (weather_info.sky >= SKY_RAINING ? 4 : 2), obj->value[0] - obj->value[1]);

    if (water > 0)
    {
        separate_obj(obj);
        obj->value[2] = LIQ_WATER;
        obj->value[1] += water;

        if (!is_name("water", obj->name))
        {
            char buf[MAX_STRING_LENGTH];

            sprintf(buf, "%s water", obj->name);
            free_string(obj->name);
            obj->name = str_dup(buf);
        }

        act("$p is filled.", ch, obj, NULL, TO_CHAR);
    }

    return;
}

void spell_curse(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;

    /* deal with the object case first */
    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *)vo;

        if (IS_OBJ_STAT(obj, ITEM_EVIL))
        {
            act("$p is already filled with evil.", ch, obj, NULL, TO_CHAR);
            return;
        }

        separate_obj(obj);

        if (IS_OBJ_STAT(obj, ITEM_BLESS))
        {
            AFFECT_DATA *paf;

            paf = affect_find(obj->affected, gsn_bless);

            if (!saves_dispel(level, paf != NULL ? paf->level : obj->level, 0))
            {
                if (paf != NULL)
                {
                    affect_remove_obj(obj, paf);
                }

                act("$p glows with a red aura.", ch, obj, NULL, TO_ALL);
                REMOVE_BIT(obj->extra_flags, ITEM_BLESS);
                return;
            }
            else
            {
                act("The holy aura of $p is too powerful for you to overcome.", ch, obj, NULL, TO_CHAR);
                return;
            }
        }

        af.where = TO_OBJECT;
        af.type = sn;
        af.level = level;
        af.duration = 2 * level;
        af.location = APPLY_SAVES;
        af.modifier = +1;
        af.bitvector = ITEM_EVIL;
        affect_to_obj(obj, &af);

        act("$p glows with a malevolent aura.", ch, obj, NULL, TO_ALL);

        if (obj->wear_loc != WEAR_NONE)
        {
            ch->saving_throw += 1;
        }

        return;
    }

    /* character curses */
    victim = (CHAR_DATA *)vo;

    if (IS_AFFECTED(victim, AFF_CURSE))
    {
        act("$E's already been cursed.", ch, NULL, victim, TO_CHAR);
        return;
    }

    if (saves_spell(level, victim, DAM_NEGATIVE))
    {
        act("$E resists your magic!", ch, NULL, victim, TO_CHAR);
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 2 * level;
    af.location = APPLY_HITROLL;
    af.modifier = -1 * (level / 8);
    af.bitvector = AFF_CURSE;
    affect_to_char(victim, &af);

    af.location = APPLY_SAVES;
    af.modifier = level / 8;
    affect_to_char(victim, &af);

    send_to_char("You feel unclean.\r\n", victim);

    if (ch != victim)
    {
        act("$N looks very uncomfortable.", ch, NULL, victim, TO_CHAR);
        act("$N looks very uncomfortable.", ch, NULL, victim, TO_NOTVICT);
    }

    return;
}

void spell_detect_evil(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_DETECT_EVIL))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N can already detect evil.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_DETECT_EVIL;
    affect_to_char(victim, &af);

    send_to_char("Your eyes tingle.\r\n", victim);

    if (ch != victim)
    {
        send_to_char("Ok.\r\n", ch);
    }

    return;
}


void spell_detect_good(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_DETECT_GOOD))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N can already detect good.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_DETECT_GOOD;
    affect_to_char(victim, &af);

    send_to_char("Your eyes tingle.\r\n", victim);

    if (ch != victim)
    {
        send_to_char("Ok.\r\n", ch);
    }

    return;
}

void spell_detect_hidden(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_DETECT_HIDDEN))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N can already sense hidden lifeforms.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_DETECT_HIDDEN;
    affect_to_char(victim, &af);

    send_to_char("Your awareness improves.\r\n", victim);

    if (ch != victim)
    {
        send_to_char("Ok.\r\n", ch);
    }

    return;
}

void spell_detect_invis(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_DETECT_INVIS))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N can already see invisible things.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_DETECT_INVIS;
    affect_to_char(victim, &af);

    send_to_char("Your eyes tingle.\r\n", victim);

    if (ch != victim)
    {
        send_to_char("Ok.\r\n", ch);
    }

    return;
}

void spell_detect_magic(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_DETECT_MAGIC))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N can already detect magic.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_DETECT_MAGIC;
    affect_to_char(victim, &af);

    send_to_char("Your eyes tingle.\r\n", victim);

    if (ch != victim)
    {
        send_to_char("Ok.\r\n", ch);
    }

    return;
}



void spell_detect_poison(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    if (obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD)
    {
        if (obj->value[3] != 0)
        {
            send_to_char("You smell poisonous fumes.\r\n", ch);
        }
        else
        {
            send_to_char("It looks delicious.\r\n", ch);
        }
    }
    else
    {
        send_to_char("It doesn't look poisoned.\r\n", ch);
    }

    return;
}

/*
 * Allows a caster to see better in the darkness.
 */
void spell_dark_vision(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_DARK_VISION))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N already has a heightened sense in the darkness.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_DARK_VISION;
    affect_to_char(victim, &af);

    send_to_char("You have a heightened sense of vision in the darkness.\r\n", victim);

    if (ch != victim)
    {
        send_to_char("Ok.\r\n", ch);
    }

    return;
}

/*
 * A spell that will allow the caster to detect fireproofed items when examined.
 */
void spell_detect_fireproof(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, gsn_detect_fireproof))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N can already sense fireproofed items.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("Your awareness to detect fireproofed items improves.\r\n", victim);

    if (ch != victim)
    {
        send_to_char("Ok.\r\n", ch);
    }

    return;
}

void spell_dispel_magic(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    bool found = FALSE;
    int x = 0;

    if (saves_spell(level, victim, DAM_OTHER))
    {
        send_to_char("You feel a brief tingling sensation.\r\n", victim);
        send_to_char("You failed.\r\n", ch);
        return;
    }

    // Begin running through the spells in the dispel_table, if there is an
    // act message to show to the room show it, otherwise it will be silent.
    for (x = 0; dispel_table[x].gsn != NULL; x++)
    {
        if (check_dispel(level, victim, *dispel_table[x].gsn))
        {
            found = TRUE;

            // Check for the room message
            if (!IS_NULLSTR(dispel_table[x].room_msg))
            {
                act(dispel_table[x].room_msg, victim, NULL, NULL, TO_ROOM);
            }
        }
    }

    // This is different than cancel, sanc is weird on mobs because it's set with the AFF
    // bit and not the gsn affect.  In these cases will run the saves check like this.
    if (IS_AFFECTED(victim, AFF_SANCTUARY)
        && !saves_dispel(level, victim->level, -1)
        && !is_affected(victim, gsn_sanctuary))
    {
        REMOVE_BIT(victim->affected_by, AFF_SANCTUARY);
        act("The white aura around $n's body vanishes.", victim, NULL, NULL, TO_ROOM);
        found = TRUE;
    }

    if (found)
    {
        send_to_char("Ok.\r\n", ch);
    }
    else
    {
        send_to_char("Spell failed.\r\n", ch);
    }

    return;
}

/*
 * Drain XP, MANA, HP.
 * Caster gains HP.
 */
void spell_energy_drain(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int dam;

    if (saves_spell(level, victim, DAM_NEGATIVE))
    {
        send_to_char("You feel a momentary chill.\r\n", victim);
        return;
    }


    if (victim->level <= 2)
    {
        dam = ch->hit + 1;
    }
    else
    {
        gain_exp(victim, 0 - number_range(level / 2, 3 * level / 2));
        victim->mana /= 2;
        victim->move /= 2;
        dam = dice(1, level);
        ch->hit += dam;
    }

    send_to_char("You feel your life slipping away!\r\n", victim);
    send_to_char("Wow....what a rush!\r\n", ch);
    damage(ch, victim, dam, sn, DAM_NEGATIVE, TRUE);

    return;
}

void spell_fireproof(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *)vo;
    AFFECT_DATA af;

    if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF))
    {
        act("$p is already protected from burning.", ch, obj, NULL, TO_CHAR);
        return;
    }

    af.where = TO_OBJECT;
    af.type = sn;
    af.level = level;
    af.duration = level + (level / 2);
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = ITEM_BURN_PROOF;

    affect_to_obj(obj, &af);

    act("You protect $p from fire.", ch, obj, NULL, TO_CHAR);
    act("$p is surrounded by a protective aura.", ch, obj, NULL, TO_ROOM);
}

/*
 * Faerie fire spell - This will make the surround the user by a pink outline affecting their AC.
 */
void spell_faerie_fire(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_FAERIE_FIRE))
    {
        act("$N is already surrounded by a pink outline.", ch, NULL, victim, TO_CHAR);
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level / 2;
    af.location = APPLY_AC;
    af.modifier = level;
    af.bitvector = AFF_FAERIE_FIRE;
    affect_to_char(victim, &af);

    send_to_char("You are surrounded by a pink outline.\r\n", victim);
    act("$n is surrounded by a pink outline.", victim, NULL, NULL, TO_ROOM);

    return;
}

/*
 * Faerie fog is a spell that will seek to make visible people that are in the room and
 * hiding.  There is also a chance that if successful that it will also cast a slightly
 * weaker and shorter duration version of faerie fire on the victim.
 */
void spell_faerie_fog(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *ich;
    AFFECT_DATA af;

    act("$n conjures a cloud of purple smoke.", ch, NULL, NULL, TO_ROOM);
    send_to_char("You conjure a cloud of purple smoke.\r\n", ch);

    for (ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room)
    {
        if (ich->invis_level > 0)
            continue;

        if (ich == ch || saves_spell(level, ich, DAM_OTHER))
            continue;

        affect_strip(ich, gsn_invisibility);
        affect_strip(ich, gsn_mass_invisibility);
        affect_strip(ich, gsn_sneak);
        affect_strip(ich, gsn_quiet_movement);
        affect_strip(ich, gsn_camouflage);
        REMOVE_BIT(ich->affected_by, AFF_HIDE);
        REMOVE_BIT(ich->affected_by, AFF_INVISIBLE);
        REMOVE_BIT(ich->affected_by, AFF_SNEAK);
        act("$n is revealed!", ich, NULL, NULL, TO_ROOM);
        send_to_char("You are revealed!\r\n", ich);

        // Added 50% chance of being surrounded by faerie fire, much shorter duration
        // than the actual faerie fire spell however and weaker AC affect.  Only hits
        if (CHANCE(50) && !IS_AFFECTED(ich, AFF_FAERIE_FIRE))
        {
            af.where = TO_AFFECTS;
            af.type = sn;
            af.level = level;
            af.duration = level / 15;
            af.location = APPLY_AC;
            af.modifier = level / 2;
            af.bitvector = AFF_FAERIE_FIRE;
            affect_to_char(ich, &af);
            send_to_char("You are surrounded by a pink outline.\r\n", ich);
            act("$n is surrounded by a pink outline.", ich, NULL, NULL, TO_ROOM);
        }

    }

    return;
} // end spell_faerie_fog

void spell_floating_disc(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    OBJ_DATA *disc, *floating;

    floating = get_eq_char(ch, WEAR_FLOAT);
    if (floating != NULL && IS_OBJ_STAT(floating, ITEM_NOREMOVE))
    {
        act("You can't remove $p.", ch, floating, NULL, TO_CHAR);
        return;
    }

    disc = create_object(get_obj_index(OBJ_VNUM_DISC));
    disc->value[0] = ch->level * 10;    /* 10 pounds per level capacity */
    disc->value[3] = ch->level * 5;    /* 5 pounds per level max per item */
    disc->timer = ch->level * 2 - number_range(0, level / 2);

    act("$n has created a floating black disc.", ch, NULL, NULL, TO_ROOM);
    send_to_char("You create a floating disc.\r\n", ch);
    obj_to_char(disc, ch);
    wear_obj(ch, disc, TRUE);
    return;
}

/*
 * Spell that raises a character off of the ground and will also allow them to
 * reach rooms that are in the air.  The land command will allow them to land
 * without cancelling.
 */
void spell_fly(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_FLYING))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N doesn't need your help to fly.", ch, NULL, victim, TO_CHAR);
            return;
        }

    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level + 3;
    af.location = 0;
    af.modifier = 0;
    af.bitvector = AFF_FLYING;
    affect_to_char(victim, &af);

    send_to_char("Your feet rise off the ground.\r\n", victim);
    act("$n's feet rise off the ground.", victim, NULL, NULL, TO_ROOM);

    return;
}

/*
 * RT ROM-style gate
 * Spell that allows a player to transport themselves to another player or mob.
 */
void spell_gate(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim;
    bool gate_pet;

    // Psionicist boost, can gate to higher levels mobs than normal at
    // level casters.
    if (ch->class == PSIONICIST_CLASS)
    {
        level += 3;
    }

    if ((victim = get_char_world(ch, target_name)) == NULL
        || victim == ch
        || victim->in_room == NULL
        || victim->in_room->clan
        || !can_see_room(ch, victim->in_room)
        || IS_SET(victim->in_room->room_flags, ROOM_SAFE)
        || IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
        || IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
        || IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
        || IS_SET(victim->in_room->room_flags, ROOM_NO_GATE)
        || IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
        || IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
        || IS_SET(victim->in_room->area->area_flags, AREA_NO_GATE)
        || IS_SET(ch->in_room->area->area_flags, AREA_NO_GATE)
        || victim->level >= level + 3
        || (is_clan(victim) && !is_same_clan(ch, victim))
        || !same_continent_char(ch, victim)
        || (!IS_NPC(victim) && victim->level >= LEVEL_IMMORTAL)
        || (IS_NPC(victim) && IS_SET(victim->imm_flags, IMM_SUMMON))
        || (IS_NPC(victim) && saves_spell(level, victim, DAM_OTHER)))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    if (ch->pet != NULL && ch->in_room == ch->pet->in_room)
    {
        gate_pet = TRUE;
    }
    else
    {
        gate_pet = FALSE;
    }

    act("$n steps through a gate and vanishes.", ch, NULL, NULL, TO_ROOM);
    send_to_char("You step through a gate and vanish.\r\n", ch);
    char_from_room(ch);
    char_to_room(ch, victim->in_room);

    act("$n has arrived through a gate.", ch, NULL, NULL, TO_ROOM);
    do_function(ch, &do_look, "auto");

    if (gate_pet)
    {
        act("$n steps through a gate and vanishes.", ch->pet, NULL, NULL, TO_ROOM);
        send_to_char("You step through a gate and vanish.\r\n", ch->pet);
        char_from_room(ch->pet);
        char_to_room(ch->pet, victim->in_room);
        act("$n has arrived through a gate.", ch->pet, NULL, NULL, TO_ROOM);
        do_function(ch->pet, &do_look, "auto");
    }

    // Priest, agony spell check/processing
    agony_damage_check(ch);

}



void spell_giant_strength(int sn, int level, CHAR_DATA * ch, void *vo,
    int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N can't get any stronger.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = APPLY_STR;
    af.modifier = 1 + (level >= 18) + (level >= 25) + (level >= 32);
    af.bitvector = 0;
    affect_to_char(victim, &af);
    send_to_char("Your muscles surge with heightened power!\r\n", victim);
    act("$n's muscles surge with heightened power.", victim, NULL, NULL,
        TO_ROOM);
    return;
}

/* RT haste spell */
void spell_haste(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn) || IS_AFFECTED(victim, AFF_HASTE)
        || IS_SET(victim->off_flags, OFF_FAST))
    {
        if (victim == ch)
            send_to_char("You can't move any faster!\r\n", ch);
        else
            act("$N is already moving as fast as $E can.",
                ch, NULL, victim, TO_CHAR);
        return;
    }

    if (IS_AFFECTED(victim, AFF_SLOW))
    {
        if (!check_dispel(level, victim, gsn_slow))
        {
            if (victim != ch)
                send_to_char("Spell failed.\r\n", ch);
            send_to_char("You feel momentarily faster.\r\n", victim);
            return;
        }
        act("$n is moving less slowly.", victim, NULL, NULL, TO_ROOM);
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    if (victim == ch)
        af.duration = level / 2;
    else
        af.duration = level / 4;
    af.location = APPLY_DEX;
    af.modifier = 1 + (level >= 18) + (level >= 25) + (level >= 32);
    af.bitvector = AFF_HASTE;
    affect_to_char(victim, &af);
    send_to_char("You feel yourself moving more quickly.\r\n", victim);
    act("$n is moving more quickly.", victim, NULL, NULL, TO_ROOM);
    if (ch != victim)
        send_to_char("Ok.\r\n", ch);
    return;
}

void spell_identify(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *)vo;
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA *paf;

    sprintf(buf,
        "Object '%s' is type %s, extra flags %s.\r\nWeight is %d, value is %d, level is %d.\r\n",
        obj->name,
        item_name(obj->item_type),
        extra_bit_name(obj->extra_flags),
        obj->weight / 10, obj->cost, obj->level);
    send_to_char(buf, ch);

    if (obj->pIndexData->area)
    {
        sprintf(buf,"This item comes from '%s'.\r\n", obj->pIndexData->area->name);
        send_to_char(buf, ch);
    }

    switch (obj->item_type)
    {
        case ITEM_SCROLL:
        case ITEM_POTION:
        case ITEM_PILL:
            sprintf(buf, "Level %d spells of:", obj->value[0]);
            send_to_char(buf, ch);

            if (obj->value[1] >= 0 && obj->value[1] < MAX_SKILL)
            {
                send_to_char(" '", ch);
                send_to_char(skill_table[obj->value[1]]->name, ch);
                send_to_char("'", ch);
            }

            if (obj->value[2] >= 0 && obj->value[2] < MAX_SKILL)
            {
                send_to_char(" '", ch);
                send_to_char(skill_table[obj->value[2]]->name, ch);
                send_to_char("'", ch);
            }

            if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL)
            {
                send_to_char(" '", ch);
                send_to_char(skill_table[obj->value[3]]->name, ch);
                send_to_char("'", ch);
            }

            if (obj->value[4] >= 0 && obj->value[4] < MAX_SKILL)
            {
                send_to_char(" '", ch);
                send_to_char(skill_table[obj->value[4]]->name, ch);
                send_to_char("'", ch);
            }

            send_to_char(".\r\n", ch);
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            sprintf(buf, "Has %d charges of level %d",
                obj->value[2], obj->value[0]);
            send_to_char(buf, ch);

            if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL)
            {
                send_to_char(" '", ch);
                send_to_char(skill_table[obj->value[3]]->name, ch);
                send_to_char("'", ch);
            }

            send_to_char(".\r\n", ch);
            break;

        case ITEM_DRINK_CON:
            sprintf(buf, "It holds %s-colored %s.\r\n",
                liquid_table[obj->value[2]].liq_color,
                liquid_table[obj->value[2]].liq_name);
            send_to_char(buf, ch);
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
                    send_to_char("exotic.\r\n", ch);
                    break;
                case (WEAPON_SWORD) :
                    send_to_char("sword.\r\n", ch);
                    break;
                case (WEAPON_DAGGER) :
                    send_to_char("dagger.\r\n", ch);
                    break;
                case (WEAPON_SPEAR) :
                    send_to_char("spear/staff.\r\n", ch);
                    break;
                case (WEAPON_MACE) :
                    send_to_char("mace/club.\r\n", ch);
                    break;
                case (WEAPON_AXE) :
                    send_to_char("axe.\r\n", ch);
                    break;
                case (WEAPON_FLAIL) :
                    send_to_char("flail.\r\n", ch);
                    break;
                case (WEAPON_WHIP) :
                    send_to_char("whip.\r\n", ch);
                    break;
                case (WEAPON_POLEARM) :
                    send_to_char("polearm.\r\n", ch);
                    break;
                default:
                    send_to_char("unknown.\r\n", ch);
                    break;
            }
            sprintf(buf, "Damage is %dd%d (average %d).\r\n",
                obj->value[1], obj->value[2],
                (1 + obj->value[2]) * obj->value[1] / 2);
            send_to_char(buf, ch);
            if (obj->value[4])
            {                    /* weapon flags */
                sprintf(buf, "Weapons flags: %s\r\n",
                    weapon_bit_name(obj->value[4]));
                send_to_char(buf, ch);
            }
            break;

        case ITEM_ARMOR:
            sprintf(buf,
                "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic.\r\n",
                obj->value[0], obj->value[1], obj->value[2],
                obj->value[3]);
            send_to_char(buf, ch);
            break;
    }

    if (obj->enchanted_by != NULL)
    {
        sprintf(buf, "Enchanted by: %s\r\n", obj->enchanted_by);
        send_to_char(buf, ch);
    }

    if (obj->wizard_mark != NULL)
    {
        sprintf(buf, "Wizard mark: %s\r\n", obj->wizard_mark);
        send_to_char(buf, ch);
    }

    if (!obj->enchanted)
        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
        {
            if (paf->location != APPLY_NONE && paf->modifier != 0)
            {
                sprintf(buf, "Affects %s by %d.\r\n",
                    affect_loc_name(paf->location), paf->modifier);
                send_to_char(buf, ch);
                if (paf->bitvector)
                {
                    switch (paf->where)
                    {
                        case TO_AFFECTS:
                            sprintf(buf, "Adds %s affect.\r\n",
                                affect_bit_name(paf->bitvector));
                            break;
                        case TO_OBJECT:
                            sprintf(buf, "Adds %s object flag.\r\n",
                                extra_bit_name(paf->bitvector));
                            break;
                        case TO_IMMUNE:
                            sprintf(buf, "Adds immunity to %s.\r\n",
                                imm_bit_name(paf->bitvector));
                            break;
                        case TO_RESIST:
                            sprintf(buf, "Adds resistance to %s.\r\n",
                                imm_bit_name(paf->bitvector));
                            break;
                        case TO_VULN:
                            sprintf(buf, "Adds vulnerability to %s.\r\n",
                                imm_bit_name(paf->bitvector));
                            break;
                        default:
                            sprintf(buf, "Unknown bit %d: %d\r\n",
                                paf->where, paf->bitvector);
                            break;
                    }
                    send_to_char(buf, ch);
                }
            }
        }

    for (paf = obj->affected; paf != NULL; paf = paf->next)
    {
        if (paf->location != APPLY_NONE && paf->modifier != 0)
        {
            sprintf(buf, "Affects %s by %d",
                affect_loc_name(paf->location), paf->modifier);
            send_to_char(buf, ch);
            if (paf->duration > -1)
                sprintf(buf, ", %d hours.\r\n", paf->duration);
            else
                sprintf(buf, ".\r\n");
            send_to_char(buf, ch);
            if (paf->bitvector)
            {
                switch (paf->where)
                {
                    case TO_AFFECTS:
                        sprintf(buf, "Adds %s affect.\r\n",
                            affect_bit_name(paf->bitvector));
                        break;
                    case TO_OBJECT:
                        sprintf(buf, "Adds %s object flag.\r\n",
                            extra_bit_name(paf->bitvector));
                        break;
                    case TO_WEAPON:
                        sprintf(buf, "Adds %s weapon flags.\r\n",
                            weapon_bit_name(paf->bitvector));
                        break;
                    case TO_IMMUNE:
                        sprintf(buf, "Adds immunity to %s.\r\n",
                            imm_bit_name(paf->bitvector));
                        break;
                    case TO_RESIST:
                        sprintf(buf, "Adds resistance to %s.\r\n",
                            imm_bit_name(paf->bitvector));
                        break;
                    case TO_VULN:
                        sprintf(buf, "Adds vulnerability to %s.\r\n",
                            imm_bit_name(paf->bitvector));
                        break;
                    default:
                        sprintf(buf, "Unknown bit %d: %d\r\n",
                            paf->where, paf->bitvector);
                        break;
                }
                send_to_char(buf, ch);
            }
        }
    }

    return;
}



void spell_infravision(int sn, int level, CHAR_DATA * ch, void *vo,
    int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_INFRARED))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N already has infravision.\r\n", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    act("$n's eyes glow red.\r\n", ch, NULL, NULL, TO_ROOM);

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 2 * level;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_INFRARED;
    affect_to_char(victim, &af);
    send_to_char("Your eyes glow red.\r\n", victim);
    return;
}

void spell_know_alignment(int sn, int level, CHAR_DATA * ch, void *vo,
    int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    char *msg;

    if (IS_GOOD(victim)) {
        msg = "$N has a good aura.";
    }
    else if (IS_EVIL(victim)) {
        msg = "$N has a evil aura.";
    }
    else if (IS_NEUTRAL(victim)) {
        msg = "$N has a neutral aura.";
    }
    else {
        msg = "$N's aura cannot be determined.";
        bug("Invalid alignment, spell_know_alignment", 0);
    }

    act(msg, ch, NULL, victim, TO_CHAR);
    return;
} // end void spell_know_alignment


/*
 * Spell that locates an item in the world by keyword.  It will also search for
 * wizard marks of a wizard's name.
 */
void spell_locate_object(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    char buf[MAX_INPUT_LENGTH];
    BUFFER *buffer;
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0, max_found;

    found = FALSE;
    number = 0;
    max_found = IS_IMMORTAL(ch) ? 200 : 2 * level;

    buffer = new_buf();

    for (obj = object_list; obj != NULL; obj = obj->next)
    {
        if (!can_see_obj(ch, obj)
            || (!is_name(target_name, obj->name) && !is_name(target_name, obj->wizard_mark))
            || IS_OBJ_STAT(obj, ITEM_NOLOCATE)
            || number_percent() > 2 * level || ch->level < obj->level)
            continue;

        found = TRUE;
        number++;

        for (in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj);

        if (in_obj->carried_by != NULL && can_see(ch, in_obj->carried_by))
        {
            sprintf(buf, "one is carried by %s\r\n",
                PERS(in_obj->carried_by, ch));
        }
        else
        {
            if (IS_IMMORTAL(ch) && in_obj->in_room != NULL)
                sprintf(buf, "one is in %s%s [Room %d]\r\n",
                    in_obj->in_room->name,
                    IS_OBJ_STAT(obj, ITEM_BURIED) ? " ({yBuried{x)" : "",
                    in_obj->in_room->vnum);
            else
                sprintf(buf, "one is in %s%s\r\n",
                    in_obj->in_room == NULL ? "somewhere" : in_obj->in_room->name,
                    IS_OBJ_STAT(obj, ITEM_BURIED) ? " ({yBuried{x)" : "");
        }

        buf[0] = UPPER(buf[0]);
        add_buf(buffer, buf);

        if (number >= max_found)
            break;
    }

    if (!found)
        send_to_char("Nothing like that in heaven or earth.\r\n", ch);
    else
        page_to_char(buf_string(buffer), ch);

    free_buf(buffer);

    return;
} // end spell_locate_object

/*
 * Spell that locate the position of bind stones throughout the land.
 */
void spell_locate_bind(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    char buf[MAX_INPUT_LENGTH];
    BUFFER *buffer;
    OBJ_DATA *obj;
    bool found;

    found = FALSE;

    buffer = new_buf();

    for (obj = object_list; obj != NULL; obj = obj->next)
    {
        if (obj->pIndexData == NULL || obj->pIndexData->vnum != OBJ_VNUM_BIND_STONE)
        {
            continue;
        }

        found = TRUE;

        if (IS_IMMORTAL(ch) && obj->in_room != NULL)
        {
            sprintf(buf, "One is at {c%s{x in {c%s{x [Room %d]\r\n",
                obj->in_room->name,
                obj->in_room->area->name,
                obj->in_room->vnum);
        }
        else
        {
            sprintf(buf, "One is in {c%s{x in {c%s{x\r\n",
                obj->in_room->name,
                obj->in_room->area->name);
        }

        add_buf(buffer, buf);
    }

    if (!found)
    {
        send_to_char("Nothing like that in heaven or earth.\r\n", ch);
    }
    else
    {
        page_to_char(buf_string(buffer), ch);
    }

    free_buf(buffer);
    return;
}

void spell_null(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    send_to_char("That's not a spell!\r\n", ch);
    return;
}

void spell_pass_door(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_PASS_DOOR))
    {
        if (victim == ch)
            send_to_char("You are already out of phase.\r\n", ch);
        else
            act("$N is already shifted out of phase.", ch, NULL, victim,
                TO_CHAR);
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = number_fuzzy(level / 4);
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_PASS_DOOR;
    affect_to_char(victim, &af);
    act("$n turns translucent.", victim, NULL, NULL, TO_ROOM);
    send_to_char("You turn translucent.\r\n", victim);
    return;
}

/*
 * RT plague spell, very nasty
 */
void spell_plague(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
        send_to_char("They have already been plagued.\r\n", ch);
        return;
    }

    if (saves_spell(level, victim, DAM_DISEASE) || (IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD)))
    {
        send_to_char("You feel momentarily ill, but it passes.\r\n", victim);
        act("$n looks slightly ill, but it passes.", victim, NULL, NULL, TO_ROOM);
        return;
    }

    // Merit - Healthy (50% chance to avoiding plague after it passes the saves check)
    if (!IS_NPC(victim) && IS_SET(victim->pcdata->merit, MERIT_HEALTHY) && CHANCE(50))
    {
        send_to_char("You feel momentarily ill, but it passes.\r\n", victim);
        act("$n looks slightly ill, but it passes.", victim, NULL, NULL, TO_ROOM);
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level * 3 / 4;
    af.duration = level;
    af.location = APPLY_STR;
    af.modifier = -5;
    af.bitvector = AFF_PLAGUE;
    affect_to_char(victim, &af);

    af.location = APPLY_CON;
    af.modifier = -5;
    affect_to_char(victim, &af);

    send_to_char("You scream in agony as plague sores erupt from your skin.\r\n", victim);
    act("$n screams in agony as plague sores erupt from $s skin.", victim, NULL, NULL, TO_ROOM);
}

void spell_poison(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;


    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *)vo;
        separate_obj(obj);

        if (obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON)
        {
            if (IS_OBJ_STAT(obj, ITEM_BLESS)
                || IS_OBJ_STAT(obj, ITEM_BURN_PROOF))
            {
                act("Your spell fails to corrupt $p.", ch, obj, NULL, TO_CHAR);
                return;
            }

            obj->value[3] = 1;
            act("$p is infused with poisonous vapors.", ch, obj, NULL, TO_ALL);
            return;
        }

        if (obj->item_type == ITEM_WEAPON)
        {
            if (IS_WEAPON_STAT(obj, WEAPON_FLAMING)
                || IS_WEAPON_STAT(obj, WEAPON_FROST)
                || IS_WEAPON_STAT(obj, WEAPON_VAMPIRIC)
                || IS_WEAPON_STAT(obj, WEAPON_SHARP)
                || IS_WEAPON_STAT(obj, WEAPON_LEECH)
                || IS_WEAPON_STAT(obj, WEAPON_VORPAL)
                || IS_WEAPON_STAT(obj, WEAPON_SHOCKING)
                || IS_OBJ_STAT(obj, ITEM_BLESS)
                || IS_OBJ_STAT(obj, ITEM_BURN_PROOF))
            {
                act("You can't seem to envenom $p.", ch, obj, NULL, TO_CHAR);
                return;
            }

            if (IS_WEAPON_STAT(obj, WEAPON_POISON))
            {
                act("$p is already envenomed.", ch, obj, NULL, TO_CHAR);
                return;
            }

            af.where = TO_WEAPON;
            af.type = sn;
            af.level = level / 2;
            af.duration = level / 8;
            af.location = 0;
            af.modifier = 0;
            af.bitvector = WEAPON_POISON;
            affect_to_obj(obj, &af);

            act("$p is coated with deadly venom.", ch, obj, NULL, TO_ALL);
            return;
        }

        act("You can't poison $p.", ch, obj, NULL, TO_CHAR);
        return;
    }

    victim = (CHAR_DATA *)vo;

    if (is_affected(victim, sn))
    {
        send_to_char("They are already poisoned.\r\n", ch);
        return;
    }

    if (saves_spell(level, victim, DAM_POISON))
    {
        act("$n turns slightly green, but it passes.", victim, NULL, NULL, TO_ROOM);
        send_to_char("You feel momentarily ill, but it passes.\r\n", victim);
        return;
    }

    // Merit - Healthy (50% chance to avoiding plague after it passes the saves check)
    if (!IS_NPC(victim) && IS_SET(victim->pcdata->merit, MERIT_HEALTHY) && CHANCE(50))
    {
        act("$n turns slightly green, but it passes.", victim, NULL, NULL, TO_ROOM);
        send_to_char("You feel momentarily ill, but it passes.\r\n", victim);
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = APPLY_STR;
    af.modifier = -2;
    af.bitvector = AFF_POISON;
    affect_join(victim, &af);
    send_to_char("You feel very sick.\r\n", victim);
    act("$n looks very ill.", victim, NULL, NULL, TO_ROOM);
    return;
}

/*
 * Protection evil - You can only have protection evil OR protection good on at one time.
 *
 * The caster can refresh the spell on their self but not on others.
 */
void spell_protection_evil(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    // Remove this specific effect from the caster if it's on themselves.
    if (victim == ch)
    {
        affect_strip(victim, sn);
    }

    if (IS_AFFECTED(victim, AFF_PROTECT_EVIL)
        || IS_AFFECTED(victim, AFF_PROTECT_GOOD)
        || is_affected(victim, gsn_protection_neutral))
    {
        if (victim == ch)
        {
            send_to_char("You are already protected.\r\n", ch);
        }
        else
        {
            act("$N is already protected.", ch, NULL, victim, TO_CHAR);
        }

        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 24;
    af.location = APPLY_SAVES;
    af.modifier = -1;
    af.bitvector = AFF_PROTECT_EVIL;
    affect_to_char(victim, &af);

    send_to_char("You feel holy and pure.\r\n", victim);

    if (ch != victim)
    {
        act("$N is protected from evil.", ch, NULL, victim, TO_CHAR);
    }

    return;
}

/*
 * Protection good - You can only have protection evil OR protection good on at one time.
 *
 * The caster can refresh the spell on their self but not on others.
 */
void spell_protection_good(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    // Remove this specific effect from the caster if it's on themselves.
    if (victim == ch)
    {
        affect_strip(victim, sn);
    }

    if (IS_AFFECTED(victim, AFF_PROTECT_GOOD)
        || IS_AFFECTED(victim, AFF_PROTECT_EVIL)
        || is_affected(victim, gsn_protection_neutral))
    {
        if (victim == ch)
        {
            send_to_char("You are already protected.\r\n", ch);
        }
        else
        {
            act("$N is already protected.", ch, NULL, victim, TO_CHAR);
        }

        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 24;
    af.location = APPLY_SAVES;
    af.modifier = -1;
    af.bitvector = AFF_PROTECT_GOOD;
    affect_to_char(victim, &af);

    send_to_char("You feel aligned with darkness.\r\n", victim);

    if (ch != victim)
    {
        act("$N is protected from good.", ch, NULL, victim, TO_CHAR);
    }

    return;
}

void spell_protection_neutral(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    // Remove this specific effect from the caster if it's on themselves.
    if (victim == ch)
    {
        affect_strip(victim, sn);
    }

    if (IS_AFFECTED(victim, AFF_PROTECT_GOOD)
        || IS_AFFECTED(victim, AFF_PROTECT_EVIL)
        || is_affected(victim, gsn_protection_neutral))
    {
        if (victim == ch)
        {
            send_to_char("You are already protected.\r\n", ch);
        }
        else
        {
            act("$N is already protected.", ch, NULL, victim, TO_CHAR);
        }

        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 24;
    af.location = APPLY_SAVES;
    af.modifier = -1;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("You are protected from neutral people.\r\n", victim);

    if (ch != victim)
    {
        act("$N is protected from neutral.",ch,NULL,victim,TO_CHAR);
    }

    return;
}

void spell_recharge(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *)vo;
    int chance, percent;

    if (obj->item_type != ITEM_WAND && obj->item_type != ITEM_STAFF)
    {
        send_to_char("That item does not carry charges.\r\n", ch);
        return;
    }

    if (obj->value[3] >= 3 * level / 2)
    {
        send_to_char("Your skills are not great enough for that.\r\n", ch);
        return;
    }

    if (obj->value[1] == 0)
    {
        send_to_char("That item has already been recharged once.\r\n", ch);
        return;
    }

    separate_obj(obj);

    chance = 40 + 2 * level;

    chance -= obj->value[3];    /* harder to do high-level spells */
    chance -= (obj->value[1] - obj->value[2]) *
        (obj->value[1] - obj->value[2]);

    chance = UMAX(level / 2, chance);

    percent = number_percent();

    if (percent < chance / 2)
    {
        act("$p glows softly.", ch, obj, NULL, TO_CHAR);
        act("$p glows softly.", ch, obj, NULL, TO_ROOM);
        obj->value[2] = UMAX(obj->value[1], obj->value[2]);
        obj->value[1] = 0;
        return;
    }

    else if (percent <= chance)
    {
        int chargeback, chargemax;

        act("$p glows softly.", ch, obj, NULL, TO_CHAR);
        act("$p glows softly.", ch, obj, NULL, TO_CHAR);

        chargemax = obj->value[1] - obj->value[2];

        if (chargemax > 0)
            chargeback = UMAX(1, chargemax * percent / 100);
        else
            chargeback = 0;

        obj->value[2] += chargeback;
        obj->value[1] = 0;
        return;
    }

    else if (percent <= UMIN(95, 3 * chance / 2))
    {
        send_to_char("Nothing seems to happen.\r\n", ch);
        if (obj->value[1] > 1)
            obj->value[1]--;
        return;
    }

    else
    {                            /* whoops! */

        act("$p glows brightly and explodes!", ch, obj, NULL, TO_CHAR);
        act("$p glows brightly and explodes!", ch, obj, NULL, TO_ROOM);
        extract_obj(obj);
    }
}

/*
 * Santuary spell - This is a staple spell, it will greatly increase the damage done to you,
 * without it in some form you can become toast quickly.
 *
 * You can refresh this spell on yourself but cannot on others to prevent someone of a lower
 * casting level casting it on you.
 */
void spell_sanctuary(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_SANCTUARY))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N is already in sanctuary.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level / 6;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_SANCTUARY;
    affect_to_char(victim, &af);
    act("$n is surrounded by a white aura.", victim, NULL, NULL, TO_ROOM);
    send_to_char("You are surrounded by a white aura.\r\n", victim);
    return;
}

/*
 * Shield spell - this spell will enhance your armor class.
 *
 * The caster can refresh this spell on themselves but cannot on others to avoid someone
 * of a lower casting level casting on you.
 */
void spell_shield(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N is already protected by a shield.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 8 + level;
    af.location = APPLY_AC;
    af.modifier = -20;
    af.bitvector = 0;
    affect_to_char(victim, &af);
    act("$n is surrounded by a force shield.", victim, NULL, NULL, TO_ROOM);
    send_to_char("You are surrounded by a force shield.\r\n", victim);
    return;
}

void spell_slow(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn) || IS_AFFECTED(victim, AFF_SLOW))
    {
        if (victim == ch)
        {
            send_to_char("You can't move any slower!\r\n", ch);
        }
        else
        {
            act("$N can't get any slower than that.", ch, NULL, victim, TO_CHAR);
        }

        return;
    }

    // If the person cast it on themselves, and they are hasted, remove the haste, skip
    // this if they are charmed and make it go through the normal checks.
    if (ch == victim && IS_AFFECTED(victim, AFF_HASTE) && !IS_AFFECTED(victim, AFF_CHARM))
    {
        affect_strip(victim, gsn_haste);
        if (skill_table[gsn_haste]->msg_off)
        {
            sendf(ch, "%s\r\n", skill_table[gsn_haste]->msg_off);
        }

        return;
    }

    if (saves_spell(level, victim, DAM_OTHER) || IS_SET(victim->imm_flags, IMM_MAGIC))
    {
        if (victim != ch)
        {
            send_to_char("Nothing seemed to happen.\r\n", ch);
        }

        send_to_char("You feel momentarily lethargic.\r\n", victim);
        return;
    }

    if (IS_AFFECTED(victim, AFF_HASTE))
    {
        if (!check_dispel(level, victim, gsn_haste))
        {
            if (victim != ch)
            {
                send_to_char("Spell failed.\r\n", ch);
            }

            send_to_char("You feel momentarily slower.\r\n", victim);
            return;
        }

        act("$n is moving less quickly.", victim, NULL, NULL, TO_ROOM);
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level / 2;
    af.location = APPLY_DEX;
    af.modifier = -1 - (level >= 18) - (level >= 25) - (level >= 32);
    af.bitvector = AFF_SLOW;
    affect_to_char(victim, &af);
    send_to_char("You feel yourself slowing d o w n...\r\n", victim);
    act("$n starts to move in slow motion.", victim, NULL, NULL, TO_ROOM);
    return;
}

/*
 * Stone skin spell - enhances a characters armor class.
 *
 * The caster can refresh this spell on themselves but cannot refresh it on others
 * to prevent someone of a lower casting level casting it on you if you're already
 * protected by it.
 */
void spell_stone_skin(int sn, int level, CHAR_DATA * ch, void *vo,
    int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(ch, sn))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$N is already as hard as can be.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = APPLY_AC;
    af.modifier = -40;
    af.bitvector = 0;
    affect_to_char(victim, &af);
    act("$n's skin turns to stone.", victim, NULL, NULL, TO_ROOM);
    send_to_char("Your skin turns to stone.\r\n", victim);
    return;
}

/*
 * Spell to summon another player or mob.
 */
void spell_summon(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim;

    if ((victim = get_char_world(ch, target_name)) == NULL
        || victim == ch
        || victim->in_room == NULL
        || IS_SET(ch->in_room->room_flags, ROOM_SAFE)
        || IS_SET(victim->in_room->room_flags, ROOM_SAFE)
        || IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
        || IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
        || IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
        || IS_SET(victim->in_room->room_flags, ROOM_NO_GATE)
        || (IS_NPC(victim) && IS_SET(victim->act, ACT_AGGRESSIVE))
        || victim->level >= level + 3
        || (!IS_NPC(victim) && victim->level >= LEVEL_IMMORTAL)
        || victim->fighting != NULL
        || (IS_NPC(victim) && IS_SET(victim->imm_flags, IMM_SUMMON))
        || (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
        || (!IS_NPC(victim) && IS_SET(victim->act, PLR_NOSUMMON))
        || IS_SET(victim->in_room->area->area_flags, AREA_NO_SUMMON)
        || IS_SET(ch->in_room->area->area_flags, AREA_NO_SUMMON)
        || (IS_NPC(victim) && saves_spell(level, victim, DAM_OTHER)))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    act("$n disappears suddenly.", victim, NULL, NULL, TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, ch->in_room);
    act("$n arrives suddenly.", victim, NULL, NULL, TO_ROOM);
    act("$n has summoned you!", ch, NULL, victim, TO_VICT);
    do_function(victim, &do_look, "auto");
    return;
}

/*
 * Spell that will allow a player to transport themselves to a random point in
 * the world.
 */
void spell_teleport(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    ROOM_INDEX_DATA *pRoomIndex = NULL;
    bool found = FALSE;
    int counter = 0;

    if (IS_NPC(victim)
        || victim->in_room == NULL
        || IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
        || IS_SET(victim->in_room->area->area_flags, AREA_NO_GATE)
        || IS_SET(victim->in_room->room_flags, ROOM_NO_GATE)
        || IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
        || IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
        || IS_SET(victim->in_room->area->area_flags, AREA_NO_GATE)
        || (victim != ch && IS_SET(victim->imm_flags, IMM_SUMMON))
        || (!IS_NPC(ch) && victim->fighting != NULL)
        || (victim != ch && (saves_spell(level - 5, victim, DAM_OTHER))))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    while (!found)
    {
        counter++;

        // Get a random room
        pRoomIndex = get_random_room(victim);

        // Rooms must be not no gate and not no recall, hopefully ensuring
        // they don't end up somewhere that's impossible to get out of without
        // knowing how they'd have gotten there normally.
        if (!IS_SET(pRoomIndex->area->area_flags, AREA_NO_GATE)
            && !IS_SET(pRoomIndex->room_flags, ROOM_NO_GATE)
            && !IS_SET(pRoomIndex->room_flags, ROOM_NO_RECALL))
        {
            found = TRUE;
        }

        // This should never happen, but...
        if (counter > 200)
        {
            send_to_char("You failed.\r\n", ch);
            bug("spell_teleport: Didn't find a room in first 200 tries", sn);
            return;
        }

    }

    if (pRoomIndex != NULL)
    {
        if (victim != ch)
        {
            send_to_char("You have been teleported!\r\n", victim);
        }

        act("$n vanishes!", victim, NULL, NULL, TO_ROOM);
        char_from_room(victim);
        char_to_room(victim, pRoomIndex);
        act("$n slowly fades into existence.", victim, NULL, NULL, TO_ROOM);
        do_function(victim, &do_look, "auto");

        // Priest, agony spell check/processing
        agony_damage_check(ch);
    }
    else
    {
        send_to_char("You failed.\r\n", ch);
        bug("spell_teleport: pRoomIndex was null", sn);
    }

    return;
}

void spell_weaken(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
        send_to_char("They are already very weak.\r\n", ch);
        return;
    }

    if (saves_spell(level, victim, DAM_OTHER))
    {
        send_to_char("The spell failed.\r\n", ch);
        return;
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = level / 2;
    af.location = APPLY_STR;
    af.modifier = -1 * (level / 5);
    af.bitvector = AFF_WEAKEN;
    affect_to_char(victim, &af);
    send_to_char("You feel your strength slip away.\r\n", victim);
    act("$n looks tired and weak.", victim, NULL, NULL, TO_ROOM);
    return;
}

/*
 * Word of recall is much like the recall skill.  It will halve your movement
 * in order to send you back to the recall point.  If the character has the
 * enhanced recall skill and the check passes it will only half the movement
 * by 25%.
 */
void spell_word_of_recall(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    ROOM_INDEX_DATA *location;
    int recall_vnum = 0;

    if (IS_NPC(victim))
    {
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

    if ((location = get_room_index(recall_vnum)) == NULL)
    {
        send_to_char("You are completely lost.\r\n", victim);
        return;
    }

    if (IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
        || IS_AFFECTED(victim, AFF_CURSE)
        || IS_SET(victim->in_room->area->area_flags, AREA_NO_RECALL))
    {
        send_to_char("Spell failed.\r\n", victim);
        return;
    }

    if (victim->fighting != NULL)
    {
        stop_fighting(victim, TRUE);
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

    act("$n disappears.", victim, NULL, NULL, TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, location);
    act("$n appears in the room.", victim, NULL, NULL, TO_ROOM);
    do_function(victim, &do_look, "auto");

    // Priest, agony spell check/processing
    agony_damage_check(ch);

} // end spell_word_of_recall

/*
 * NPC spells.
 */
void spell_acid_breath(int sn, int level, CHAR_DATA * ch, void *vo,
    int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int dam, hp_dam, dice_dam, hpch;

    act("$n spits acid at $N.", ch, NULL, victim, TO_NOTVICT);
    act("$n spits a stream of corrosive acid at you.", ch, NULL, victim,
        TO_VICT);
    act("You spit acid at $N.", ch, NULL, victim, TO_CHAR);

    hpch = UMAX(12, ch->hit);
    hp_dam = number_range(hpch / 11 + 1, hpch / 6);
    dice_dam = dice(level, 16);

    dam = UMAX(hp_dam + dice_dam / 10, dice_dam + hp_dam / 10);

    if (saves_spell(level, victim, DAM_ACID))
    {
        acid_effect(victim, level / 2, dam / 4, TARGET_CHAR);
        damage(ch, victim, dam / 2, sn, DAM_ACID, TRUE);
    }
    else
    {
        acid_effect(victim, level, dam, TARGET_CHAR);
        damage(ch, victim, dam, sn, DAM_ACID, TRUE);
    }
}



void spell_fire_breath(int sn, int level, CHAR_DATA * ch, void *vo,
    int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    CHAR_DATA *vch, *vch_next;
    int dam, hp_dam, dice_dam;
    int hpch;

    act("$n breathes forth a cone of fire.", ch, NULL, victim, TO_NOTVICT);
    act("$n breathes a cone of hot fire over you!", ch, NULL, victim,
        TO_VICT);
    act("You breath forth a cone of fire.", ch, NULL, NULL, TO_CHAR);

    hpch = UMAX(10, ch->hit);
    hp_dam = number_range(hpch / 9 + 1, hpch / 5);
    dice_dam = dice(level, 20);

    dam = UMAX(hp_dam + dice_dam / 10, dice_dam + hp_dam / 10);
    fire_effect(victim->in_room, level, dam / 2, TARGET_ROOM);

    for (vch = victim->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if (is_safe_spell(ch, vch, TRUE)
            || (IS_NPC(vch) && IS_NPC(ch)
                && (ch->fighting != vch || vch->fighting != ch)))
            continue;

        if (vch == victim)
        {                        /* full damage */
            if (saves_spell(level, vch, DAM_FIRE))
            {
                fire_effect(vch, level / 2, dam / 4, TARGET_CHAR);
                damage(ch, vch, dam / 2, sn, DAM_FIRE, TRUE);
            }
            else
            {
                fire_effect(vch, level, dam, TARGET_CHAR);
                damage(ch, vch, dam, sn, DAM_FIRE, TRUE);
            }
        }
        else
        {                        /* partial damage */

            if (saves_spell(level - 2, vch, DAM_FIRE))
            {
                fire_effect(vch, level / 4, dam / 8, TARGET_CHAR);
                damage(ch, vch, dam / 4, sn, DAM_FIRE, TRUE);
            }
            else
            {
                fire_effect(vch, level / 2, dam / 4, TARGET_CHAR);
                damage(ch, vch, dam / 2, sn, DAM_FIRE, TRUE);
            }
        }
    }
}

void spell_frost_breath(int sn, int level, CHAR_DATA * ch, void *vo,
    int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    CHAR_DATA *vch, *vch_next;
    int dam, hp_dam, dice_dam, hpch;

    act("$n breathes out a freezing cone of frost!", ch, NULL, victim,
        TO_NOTVICT);
    act("$n breathes a freezing cone of frost over you!", ch, NULL, victim,
        TO_VICT);
    act("You breath out a cone of frost.", ch, NULL, NULL, TO_CHAR);

    hpch = UMAX(12, ch->hit);
    hp_dam = number_range(hpch / 11 + 1, hpch / 6);
    dice_dam = dice(level, 16);

    dam = UMAX(hp_dam + dice_dam / 10, dice_dam + hp_dam / 10);
    cold_effect(victim->in_room, level, dam / 2, TARGET_ROOM);

    for (vch = victim->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if (is_safe_spell(ch, vch, TRUE)
            || (IS_NPC(vch) && IS_NPC(ch)
                && (ch->fighting != vch || vch->fighting != ch)))
            continue;

        if (vch == victim)
        {                        /* full damage */
            if (saves_spell(level, vch, DAM_COLD))
            {
                cold_effect(vch, level / 2, dam / 4, TARGET_CHAR);
                damage(ch, vch, dam / 2, sn, DAM_COLD, TRUE);
            }
            else
            {
                cold_effect(vch, level, dam, TARGET_CHAR);
                damage(ch, vch, dam, sn, DAM_COLD, TRUE);
            }
        }
        else
        {
            if (saves_spell(level - 2, vch, DAM_COLD))
            {
                cold_effect(vch, level / 4, dam / 8, TARGET_CHAR);
                damage(ch, vch, dam / 4, sn, DAM_COLD, TRUE);
            }
            else
            {
                cold_effect(vch, level / 2, dam / 4, TARGET_CHAR);
                damage(ch, vch, dam / 2, sn, DAM_COLD, TRUE);
            }
        }
    }
}


void spell_gas_breath(int sn, int level, CHAR_DATA * ch, void *vo,
    int target)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam, hp_dam, dice_dam, hpch;

    act("$n breathes out a cloud of poisonous gas!", ch, NULL, NULL,
        TO_ROOM);
    act("You breath out a cloud of poisonous gas.", ch, NULL, NULL, TO_CHAR);

    hpch = UMAX(16, ch->hit);
    hp_dam = number_range(hpch / 15 + 1, 8);
    dice_dam = dice(level, 12);

    dam = UMAX(hp_dam + dice_dam / 10, dice_dam + hp_dam / 10);
    poison_effect(ch->in_room, level, dam, TARGET_ROOM);

    for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
        vch_next = vch->next_in_room;

        if (is_safe_spell(ch, vch, TRUE)
            || (IS_NPC(ch) && IS_NPC(vch)
                && (ch->fighting == vch || vch->fighting == ch)))
            continue;

        if (saves_spell(level, vch, DAM_POISON))
        {
            poison_effect(vch, level / 2, dam / 4, TARGET_CHAR);
            damage(ch, vch, dam / 2, sn, DAM_POISON, TRUE);
        }
        else
        {
            poison_effect(vch, level, dam, TARGET_CHAR);
            damage(ch, vch, dam, sn, DAM_POISON, TRUE);
        }
    }
}

void spell_lightning_breath(int sn, int level, CHAR_DATA * ch, void *vo,
    int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int dam, hp_dam, dice_dam, hpch;

    act("$n breathes a bolt of lightning at $N.", ch, NULL, victim,
        TO_NOTVICT);
    act("$n breathes a bolt of lightning at you!", ch, NULL, victim,
        TO_VICT);
    act("You breathe a bolt of lightning at $N.", ch, NULL, victim, TO_CHAR);

    hpch = UMAX(10, ch->hit);
    hp_dam = number_range(hpch / 9 + 1, hpch / 5);
    dice_dam = dice(level, 20);

    dam = UMAX(hp_dam + dice_dam / 10, dice_dam + hp_dam / 10);

    if (saves_spell(level, victim, DAM_LIGHTNING))
    {
        shock_effect(victim, level / 2, dam / 4, TARGET_CHAR);
        damage(ch, victim, dam / 2, sn, DAM_LIGHTNING, TRUE);
    }
    else
    {
        shock_effect(victim, level, dam, TARGET_CHAR);
        damage(ch, victim, dam, sn, DAM_LIGHTNING, TRUE);
    }
}

/*
 * Spells for mega1.are from Glop/Erkenbrand.
 */
void spell_general_purpose(int sn, int level, CHAR_DATA * ch, void *vo,
    int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int dam;

    dam = number_range(25, 100);
    if (saves_spell(level, victim, DAM_PIERCE))
        dam /= 2;
    damage(ch, victim, dam, sn, DAM_PIERCE, TRUE);
    return;
}

void spell_high_explosive(int sn, int level, CHAR_DATA * ch, void *vo,
    int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    int dam;

    dam = number_range(30, 120);
    if (saves_spell(level, victim, DAM_PIERCE))
        dam /= 2;
    damage(ch, victim, dam, sn, DAM_PIERCE, TRUE);
    return;
}

extern char *target_name;

/*
 * Farsight spell.  This spell will look at all areas connected to the current
 * area and then show the player all visible players they can see from those
 * areas.
 */
void spell_farsight(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    EXIT_DATA *exit;
    ROOM_INDEX_DATA *to_room;
    ROOM_INDEX_DATA *room;
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d;
    int vnum = 0;
    int index = 0;
    int i = 0;
    bool found = FALSE;
    char buf[MAX_STRING_LENGTH];
    int area_list[128];

    if (IS_AFFECTED(ch, AFF_BLIND))
    {
        send_to_char("Maybe it would help if you could see?\r\n", ch);
        return;
    }

    send_to_char("Your magic searches the surrounding areas.\r\n", ch);

    // Loop over all the rooms in this area, if any room links to an outside area
    // we will then do a where style command on that area.
    for (vnum = ch->in_room->area->min_vnum; vnum <= ch->in_room->area->max_vnum; vnum++)
    {
        if ((room = get_room_index(vnum)))
        {
            // Go over each direction in each room in this area and see if it links to an outside
            // area.  One way links are fine for our purpose.. if they other area doesn't have all
            // link back it won't work in that direction.  Multiple links to an area might be found
            // and we only want to show players from the destination area once so we'll collect the
            // areas we need, then process them.
            for (i = 0; i < MAX_DIR; i++)
            {
                // We're not saving more than 128 connected areas.  Ditch out here since it will
                // crash accessing the array outside of its bounds.
                if (index > 127)
                {
                    continue;
                }

                exit = room->exit[i];

                if (!exit)
                {
                    continue;
                }
                else
                {
                    to_room = exit->u1.to_room;
                }

                // If there is a non null room on the other side, see if it's in a different area.
                if (to_room)
                {
                    if (room->area != to_room->area)
                    {
                        // Check first to make sure it's not already in our list so we don't track
                        // duplicate areas that might be linked multiple times to multiple vnums (like
                        // Midgaard, oceans, etc.).
                        int x = 0;
                        found = FALSE;

                        for (x = 0; x < index; x++)
                        {
                            if (area_list[x] == to_room->area->vnum)
                            {
                                found = TRUE;
                                break;
                            }
                        }

                        if (!found)
                        {
                            area_list[index] = to_room->area->vnum;
                            index++;
                        }
                    }
                }
            }
        }
    }

    // Did we find anything to process?
    found = FALSE;

    if (index > 0)
    {
        // Loop over the vnums we saved
        for (i = 0; i < index; i++)
        {
            // This section was re-purposed from do_where
            for (d = descriptor_list; d; d = d->next)
            {
                if (d->connected == CON_PLAYING
                    && (victim = d->character) != NULL && !IS_NPC(victim)
                    && victim->in_room != NULL
                    && victim->in_room->area != NULL
                    && victim->in_room->area->vnum == area_list[i]
                    && !IS_SET(victim->in_room->room_flags, ROOM_NOWHERE)
                    && (is_room_owner(ch, victim->in_room)
                    || !room_is_private(victim->in_room))
                    && can_see(ch, victim))
                {
                    found = TRUE;
                    sprintf(buf, "%-20s %s\r\n", victim->name, victim->in_room->name);
                    send_to_char(buf, ch);
                }
            }
        }
    }

    if (!found)
    {
        send_to_char("There are no souls that you can sense.\r\n", ch);
    }

}

/*
 * Spell that can create a portal that players can walk through to go somewhere else.
 * Portal is one sided, you walk through it and there's no walking back.
 */
void spell_portal(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *portal, *stone;

    // Psionicist boost, can portal to higher levels mobs than normal at
    // level casters.
    if (ch->class == PSIONICIST_CLASS)
    {
        level += 3;
    }

    if ((victim = get_char_world(ch, target_name)) == NULL
        || victim == ch
        || victim->in_room == NULL
        || !can_see_room(ch, victim->in_room)
        || IS_SET(victim->in_room->room_flags, ROOM_SAFE)
        || IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
        || IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
        || IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
        || IS_SET(victim->in_room->room_flags, ROOM_NO_GATE)
        || IS_SET(ch->in_room->room_flags, ROOM_NO_GATE)
        || IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
        || victim->level >= level + 3
        || (!IS_NPC(victim) && victim->level >= LEVEL_HERO)    /* NOT trust */
        || (IS_NPC(victim) && IS_SET(victim->imm_flags, IMM_SUMMON))
        || (IS_NPC(victim) && saves_spell(level, victim, DAM_NONE))
        || (is_clan(victim) && !is_same_clan(ch, victim)))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    stone = get_eq_char(ch, WEAR_HOLD);

    if (!IS_IMMORTAL(ch)
        && (stone == NULL || stone->item_type != ITEM_WARP_STONE))
    {
        send_to_char("You lack the proper component for this spell.\r\n", ch);
        return;
    }

    if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
    {
        act("You draw upon the power of $p.", ch, stone, NULL, TO_CHAR);
        act("It flares brightly and vanishes!", ch, stone, NULL, TO_CHAR);
        separate_obj(stone);
        extract_obj(stone);
    }

    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL));
    portal->timer = 2 + level / 25;
    portal->value[3] = victim->in_room->vnum;

    obj_to_room(portal, ch->in_room);

    act("$p rises up from the ground.", ch, portal, NULL, TO_ROOM);
    act("$p rises up before you.", ch, portal, NULL, TO_CHAR);
}

/*
 * Nexus spell to create a two way portal to a location.
 */
void spell_nexus(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *portal, *stone;
    ROOM_INDEX_DATA *to_room, *from_room;

    from_room = ch->in_room;

    // Psionicist boost, can nexus to higher levels mobs than normal at
    // level casters.
    if (ch->class == PSIONICIST_CLASS)
    {
        level += 3;
    }

    if ((victim = get_char_world(ch, target_name)) == NULL
        || victim == ch
        || (to_room = victim->in_room) == NULL
        || !can_see_room(ch, to_room) || !can_see_room(ch, from_room)
        || IS_SET(to_room->room_flags, ROOM_NO_GATE)
        || IS_SET(from_room->room_flags, ROOM_NO_GATE)
        || IS_SET(to_room->room_flags, ROOM_SAFE)
        || IS_SET(from_room->room_flags, ROOM_SAFE)
        || IS_SET(to_room->room_flags, ROOM_PRIVATE)
        || IS_SET(to_room->room_flags, ROOM_SOLITARY)
        || IS_SET(to_room->room_flags, ROOM_NO_RECALL)
        || IS_SET(from_room->room_flags, ROOM_NO_RECALL)
        || victim->level >= level + 3 || (!IS_NPC(victim) && victim->level >= LEVEL_HERO)    /* NOT trust */
        || (IS_NPC(victim) && IS_SET(victim->imm_flags, IMM_SUMMON))
        || (IS_NPC(victim) && saves_spell(level, victim, DAM_NONE))
        || (is_clan(victim) && !is_same_clan(ch, victim)))
    {
        send_to_char("You failed.\r\n", ch);
        return;
    }

    stone = get_eq_char(ch, WEAR_HOLD);

    if (!IS_IMMORTAL(ch)
        && (stone == NULL || stone->item_type != ITEM_WARP_STONE))
    {
        send_to_char("You lack the proper component for this spell.\r\n", ch);
        return;
    }

    if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
    {
        act("You draw upon the power of $p.", ch, stone, NULL, TO_CHAR);
        act("It flares brightly and vanishes!", ch, stone, NULL, TO_CHAR);
        separate_obj(stone);
        extract_obj(stone);
    }

    /* portal one */
    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL));
    portal->timer = 1 + level / 10;
    portal->value[3] = to_room->vnum;

    obj_to_room(portal, from_room);

    act("$p rises up from the ground.", ch, portal, NULL, TO_ROOM);
    act("$p rises up before you.", ch, portal, NULL, TO_CHAR);

    /* no second portal if rooms are the same */
    if (to_room == from_room)
        return;

    /* portal two */
    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL));
    portal->timer = 1 + level / 10;
    portal->value[3] = from_room->vnum;

    obj_to_room(portal, to_room);

    if (to_room->people != NULL)
    {
        act("$p rises up from the ground.", to_room->people, portal, NULL, TO_ROOM);
        act("$p rises up from the ground.", to_room->people, portal, NULL, TO_CHAR);
    }
}

/*
 * Water breathing spell - This will allow a character to breath while underwater or in the
 * ocean.  It will stop them from drowning and losing HP from sucking down water.
 *
 * This spell can be refreshed on yourself but cannot be refreshed on others.
 */
void spell_water_breathing(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            act("$E can already breath water!", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = gsn_water_breathing;
    af.level = level;
    af.location = 0;
    af.modifier = 0;
    af.duration = level;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("Your lungs surge with a strange sensation.\r\n", victim);
    act("$n's lungs surge as $e appears to be breathing differently.", victim, NULL, NULL, TO_ROOM);
    return;
} // end spell_water_breathing

/*
 * Summons a fog into the room that will lower the chances that a player
 * can see. (9/24/2000)
 */
void spell_fog(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *fog;
    OBJ_DATA *obj;
    int density = 0;
    int duration = 0;

    for (obj = ch->in_room->contents; obj; obj = obj->next_content)
    {
        if (obj->item_type == ITEM_FOG)
        {
            send_to_char("There is already fog in the room, you cannot conjure anymore.\r\n", ch);
            return;
        }
    }

    // Quality of the fog
    density = number_range(level / 2, 100);
    duration = number_range(1, 3);

    // Bonus for those who cast at level or above
    if (level >= 51)
    {
        duration += 1;
        density += 10;
    }

    // Much lower density in the city
    if (ch->in_room->sector_type == SECT_CITY)
    {
        density -= 30;
    }

    // Slightly higher density in the forests
    if (ch->in_room->sector_type == SECT_FOREST)
    {
        density += 10;
    }

    // Much higher on the ocean where water is abundant to make the fog
    if (ch->in_room->sector_type == SECT_OCEAN)
    {
        density += 20;
    }

    density = URANGE(5, density, 100);

    // Immortals cast impenetrable fog
    if (IS_IMMORTAL(ch))
    {
        density = 100;
    }

    fog = create_object(get_obj_index(OBJ_VNUM_FOG));
    fog->value[0] = density; // thickness of the fog
    fog->timer = duration; // duration of the fog
    obj_to_room(fog, ch->in_room);

    if (density <= 50)
    {
        act("$n summons a thin fog that surrounds the area.", ch, NULL, NULL, TO_ROOM);
        act("You summon a thin fog that surrounds the area.", ch, NULL, NULL, TO_CHAR);
    }
    else if (density > 50 && density < 80)
    {
        act("$n summons a fog that surrounds the area.", ch, NULL, NULL, TO_ROOM);
        act("You summon a fog that surrounds the area.", ch, NULL, NULL, TO_CHAR);
    }
    else
    {
        act("$n summons a thick fog that surrounds the area.", ch, NULL, NULL, TO_ROOM);
        act("You summon a thick fog that surrounds the area.", ch, NULL, NULL, TO_CHAR);
    }

    return;

} // end fog

/*
 * Dissipate's any fog that is in the room. (9/24/2000)
 */
void spell_dispel_fog(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj;
    bool found = FALSE;

    for (obj = ch->in_room->contents; obj; obj = obj->next_content)
    {
        if (obj->item_type == ITEM_FOG)
        {
            act("$n dispels the fog from the area!", ch, NULL, NULL, TO_ROOM);
            send_to_char("You dispel the fog from the area!\r\n", ch);
            found = TRUE;
            extract_obj(obj);
        }
    }

    if (!found)
    {
        send_to_char("There is no fog to banish here.\r\n", ch);
    }

    return;
}

/*
 * Self projection spell (Illusion group) - Puts a shifted image of a person in
 * front of them that can disorient the attacker causing them to miss in their strike.
 * This should only be castable on one's self.
 */
void spell_self_projection(int sn, int level, CHAR_DATA * ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn))
    {
        if (victim == ch)
        {
            // Remove the affect so it can be re-added to yourself
            affect_strip(victim, sn);
        }
        else
        {
            // You cannot re-add this spell to someone else, this will stop people with lower
            // casting levels from replacing someone elses spells.
            act("$N's image is already projecting out in front of them.", ch, NULL, victim, TO_CHAR);
            return;
        }
    }

    af.where = TO_AFFECTS;
    af.type = sn;
    af.level = level;
    af.duration = 24;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = 0;
    affect_to_char(victim, &af);

    send_to_char("Your image projects out slightly in front of you.\r\n", victim);
    act("$n's image shifts out in front of $m.", victim, NULL, NULL, TO_ROOM);

    if (ch != victim)
    {
        act("$N's image projects out in front of $M.", ch, NULL, victim, TO_CHAR);
    }

    return;
}


/*
 * Returns the spell function for the specified name.  This will allow loading from
 * a file (although this has to be updated which isn't ideal but is necessary).  By
 * switching for the 7th character we can cut down the number of checks run.  All
 * spells under this construct must start with "spell_".  We will move the most used
 * spells in each case to the top.
 */
SPELL_FUN *spell_function_lookup(char *name)
{
    switch (name[6])
    {
        case 'a':
            if (!str_cmp(name, "spell_armor")) return spell_armor;
            if (!str_cmp(name, "spell_acid_blast")) return spell_acid_blast;
            if (!str_cmp(name, "spell_acid_breath")) return spell_acid_breath;
            if (!str_cmp(name, "spell_agony")) return spell_agony;
            break;
        case 'b':
            if (!str_cmp(name, "spell_bless")) return spell_bless;
            if (!str_cmp(name, "spell_blindness")) return spell_blindness;
            if (!str_cmp(name, "spell_burning_hands")) return spell_burning_hands;
            if (!str_cmp(name, "spell_bark_skin")) return spell_bark_skin;
            if (!str_cmp(name, "spell_boost")) return spell_boost;
            if (!str_cmp(name, "spell_blink")) return spell_blink;
            break;
        case 'c':
            if (!str_cmp(name, "spell_cancellation")) return spell_cancellation;
            if (!str_cmp(name, "spell_curse")) return spell_curse;
            if (!str_cmp(name, "spell_chain_lightning")) return spell_chain_lightning;
            if (!str_cmp(name, "spell_create_food")) return spell_create_food;
            if (!str_cmp(name, "spell_create_spring")) return spell_create_spring;
            if (!str_cmp(name, "spell_cure_blindness")) return spell_cure_blindness;
            if (!str_cmp(name, "spell_cure_critical")) return spell_cure_critical;
            if (!str_cmp(name, "spell_cure_disease")) return spell_cure_disease;
            if (!str_cmp(name, "spell_cure_poison")) return spell_cure_poison;
            if (!str_cmp(name, "spell_calm")) return spell_calm;
            if (!str_cmp(name, "spell_cause_critical")) return spell_cause_critical;
            if (!str_cmp(name, "spell_cause_light")) return spell_cause_light;
            if (!str_cmp(name, "spell_cause_serious")) return spell_cause_serious;
            if (!str_cmp(name, "spell_change_sex")) return spell_change_sex;
            if (!str_cmp(name, "spell_charm_person")) return spell_charm_person;
            if (!str_cmp(name, "spell_chill_touch")) return spell_chill_touch;
            if (!str_cmp(name, "spell_color_spray")) return spell_color_spray;
            if (!str_cmp(name, "spell_continual_light")) return spell_continual_light;
            if (!str_cmp(name, "spell_control_weather")) return spell_control_weather;
            if (!str_cmp(name, "spell_create_water")) return spell_create_water;
            if (!str_cmp(name, "spell_cure_light")) return spell_cure_light;
            if (!str_cmp(name, "spell_cure_serious")) return spell_cure_serious;
            if (!str_cmp(name, "spell_call_lightning")) return spell_call_lightning;
            if (!str_cmp(name, "spell_create_rose")) return spell_create_rose;
            if (!str_cmp(name, "spell_cure_weaken")) return spell_cure_weaken;
            if (!str_cmp(name, "spell_cure_slow")) return spell_cure_slow;
            if (!str_cmp(name, "spell_cure_deafness")) return spell_cure_deafness;
            if (!str_cmp(name, "spell_clairvoyance")) return spell_clairvoyance;
            break;
        case 'd':
            if (!str_cmp(name, "spell_detect_hidden")) return spell_detect_hidden;
            if (!str_cmp(name, "spell_detect_invis")) return spell_detect_invis;
            if (!str_cmp(name, "spell_dispel_magic")) return spell_dispel_magic;
            if (!str_cmp(name, "spell_detect_evil")) return spell_detect_evil;
            if (!str_cmp(name, "spell_detect_good")) return spell_detect_good;
            if (!str_cmp(name, "spell_detect_magic")) return spell_detect_magic;
            if (!str_cmp(name, "spell_detect_poison")) return spell_detect_poison;
            if (!str_cmp(name, "spell_detect_fireproof")) return spell_detect_fireproof;
            if (!str_cmp(name, "spell_dispel_evil")) return spell_dispel_evil;
            if (!str_cmp(name, "spell_dispel_good")) return spell_dispel_good;
            if (!str_cmp(name, "spell_disenchant")) return spell_disenchant;
            if (!str_cmp(name, "spell_demonfire")) return spell_demonfire;
            if (!str_cmp(name, "spell_dispel_fog")) return spell_dispel_fog;
            if (!str_cmp(name, "spell_displacement")) return spell_displacement;
            if (!str_cmp(name, "spell_divine_wisdom")) return spell_divine_wisdom;
            if (!str_cmp(name, "spell_dark_vision")) return spell_dark_vision;
            break;
        case 'e':
            if (!str_cmp(name, "spell_earthquake")) return spell_earthquake;
            if (!str_cmp(name, "spell_enchant_armor")) return spell_enchant_armor;
            if (!str_cmp(name, "spell_enchant_weapon")) return spell_enchant_weapon;
            if (!str_cmp(name, "spell_enchant_person")) return spell_enchant_person;
            if (!str_cmp(name, "spell_enchant_gem")) return spell_enchant_gem;
            if (!str_cmp(name, "spell_energy_drain")) return spell_energy_drain;
            if (!str_cmp(name, "spell_enhanced_recovery")) return spell_enhanced_recovery;
            if (!str_cmp(name, "spell_extend_portal")) return spell_extend_portal;
            break;
        case 'f':
            if (!str_cmp(name, "spell_fly")) return spell_fly;
            if (!str_cmp(name, "spell_frenzy")) return spell_frenzy;
            if (!str_cmp(name, "spell_fireball")) return spell_fireball;
            if (!str_cmp(name, "spell_faerie_fog")) return spell_faerie_fog;
            if (!str_cmp(name, "spell_faerie_fire")) return spell_faerie_fire;
            if (!str_cmp(name, "spell_farsight")) return spell_farsight;
            if (!str_cmp(name, "spell_fireproof")) return spell_fireproof;
            if (!str_cmp(name, "spell_flamestrike")) return spell_flamestrike;
            if (!str_cmp(name, "spell_floating_disc")) return spell_floating_disc;
            if (!str_cmp(name, "spell_fire_breath")) return spell_fire_breath;
            if (!str_cmp(name, "spell_frost_breath")) return spell_frost_breath;
            if (!str_cmp(name, "spell_fog")) return spell_fog;
            if (!str_cmp(name, "spell_forget")) return spell_forget;
            if (!str_cmp(name, "spell_feeble_mind")) return spell_feeble_mind;
            if (!str_cmp(name, "spell_false_life")) return spell_false_life;
            break;
        case 'g':
            if (!str_cmp(name, "spell_gate")) return spell_gate;
            if (!str_cmp(name, "spell_giant_strength")) return spell_giant_strength;
            if (!str_cmp(name, "spell_gas_breath")) return spell_gas_breath;
            if (!str_cmp(name, "spell_general_purpose")) return spell_general_purpose;
            if (!str_cmp(name, "spell_guardian_angel")) return spell_guardian_angel;
            break;
        case 'h':
            if (!str_cmp(name, "spell_haste")) return spell_haste;
            if (!str_cmp(name, "spell_heal")) return spell_heal;
            if (!str_cmp(name, "spell_heat_metal")) return spell_heat_metal;
            if (!str_cmp(name, "spell_harm")) return spell_harm;
            if (!str_cmp(name, "spell_holy_word")) return spell_holy_word;
            if (!str_cmp(name, "spell_high_explosive")) return spell_high_explosive;
            if (!str_cmp(name, "spell_healers_bind")) return spell_healers_bind;
            if (!str_cmp(name, "spell_healing_dream")) return spell_healing_dream;
            if (!str_cmp(name, "spell_holy_presence")) return spell_holy_presence;
            if (!str_cmp(name, "spell_holy_flame")) return spell_holy_flame;
            if (!str_cmp(name, "spell_heaviness")) return spell_heaviness;
            break;
        case 'i':
            if (!str_cmp(name, "spell_invis")) return spell_invis;
            if (!str_cmp(name, "spell_identify")) return spell_identify;
            if (!str_cmp(name, "spell_imbue")) return spell_imbue;
            if (!str_cmp(name, "spell_interlace_spirit")) return spell_interlace_spirit;
            if (!str_cmp(name, "spell_infravision")) return spell_infravision;
            if (!str_cmp(name, "spell_ice_blast")) return spell_ice_blast;
            if (!str_cmp(name, "spell_increase_armor")) return spell_increase_armor;
            break;
        case 'k':
            if (!str_cmp(name, "spell_know_alignment")) return spell_know_alignment;
            if (!str_cmp(name, "spell_know_religion")) return spell_know_religion;
            if (!str_cmp(name, "spell_knock")) return spell_knock;
            break;
        case 'l':
            if (!str_cmp(name, "spell_locate_object")) return spell_locate_object;
            if (!str_cmp(name, "spell_lightning_bolt")) return spell_lightning_bolt;
            if (!str_cmp(name, "spell_lightning_breath")) return spell_lightning_breath;
            if (!str_cmp(name, "spell_locate_wizard_mark")) return spell_locate_wizard_mark;
            if (!str_cmp(name, "spell_life_boost")) return spell_life_boost;
            if (!str_cmp(name, "spell_locate_bind")) return spell_locate_bind;
            break;
        case 'm':
            if (!str_cmp(name, "spell_magic_missile")) return spell_magic_missile;
            if (!str_cmp(name, "spell_mass_healing")) return spell_mass_healing;
            if (!str_cmp(name, "spell_mass_invis")) return spell_mass_invis;
            if (!str_cmp(name, "spell_mass_refresh")) return spell_mass_refresh;
            if (!str_cmp(name, "spell_magic_resistance")) return spell_magic_resistance;
            if (!str_cmp(name, "spell_mana_transfer")) return spell_mana_transfer;
            if (!str_cmp(name, "spell_mental_weight")) return spell_mental_weight;
            if (!str_cmp(name, "spell_mental_presence")) return spell_mental_presence;
            break;
        case 'n':
            if (!str_cmp(name, "spell_null")) return spell_null;
            if (!str_cmp(name, "spell_nexus")) return spell_nexus;
            if (!str_cmp(name, "spell_nourishment")) return spell_nourishment;
            break;
        case 'p':
            if (!str_cmp(name, "spell_poison")) return spell_poison;
            if (!str_cmp(name, "spell_pass_door")) return spell_pass_door;
            if (!str_cmp(name, "spell_plague")) return spell_plague;
            if (!str_cmp(name, "spell_portal")) return spell_portal;
            if (!str_cmp(name, "spell_protection_evil")) return spell_protection_evil;
            if (!str_cmp(name, "spell_protection_good")) return spell_protection_good;
            if (!str_cmp(name, "spell_protection_neutral")) return spell_protection_neutral;
            if (!str_cmp(name, "spell_psionic_blast")) return spell_psionic_blast;
            if (!str_cmp(name, "spell_preserve")) return spell_preserve;
            if (!str_cmp(name, "spell_psionic_focus")) return spell_psionic_focus;
            if (!str_cmp(name, "spell_psionic_shield")) return spell_psionic_shield;
            if (!str_cmp(name, "spell_poison_spray")) return spell_poison_spray;
            break;
        case 'r':
            if (!str_cmp(name, "spell_refresh")) return spell_refresh;
            if (!str_cmp(name, "spell_restore_weapon")) return spell_restore_weapon;
            if (!str_cmp(name, "spell_restore_armor")) return spell_restore_armor;
            if (!str_cmp(name, "spell_ray_of_truth")) return spell_ray_of_truth;
            if (!str_cmp(name, "spell_recharge")) return spell_recharge;
            if (!str_cmp(name, "spell_remove_curse")) return spell_remove_curse;
            if (!str_cmp(name, "spell_remove_faerie_fire")) return spell_remove_faerie_fire;
            if (!str_cmp(name, "spell_restore_mental_presence")) return spell_restore_mental_presence;
            break;
        case 's':
            if (!str_cmp(name, "spell_sanctuary")) return spell_sanctuary;
            if (!str_cmp(name, "spell_shield")) return spell_shield;
            if (!str_cmp(name, "spell_stone_skin")) return spell_stone_skin;
            if (!str_cmp(name, "spell_summon")) return spell_summon;
            if (!str_cmp(name, "spell_sleep")) return spell_sleep;
            if (!str_cmp(name, "spell_slow")) return spell_slow;
            if (!str_cmp(name, "spell_shocking_grasp")) return spell_shocking_grasp;
            if (!str_cmp(name, "spell_sequestor")) return spell_sequestor;
            if (!str_cmp(name, "spell_sacrificial_heal")) return spell_sacrificial_heal;
            if (!str_cmp(name, "spell_sense_affliction")) return spell_sense_affliction;
            if (!str_cmp(name, "spell_song_of_protection")) return spell_song_of_protection;
            if (!str_cmp(name, "spell_song_of_dissonance")) return spell_song_of_dissonance;
            if (!str_cmp(name, "spell_self_growth")) return spell_self_growth;
            if (!str_cmp(name, "spell_self_projection")) return spell_self_projection;
            break;
        case 't':
            if (!str_cmp(name, "spell_teleport")) return spell_teleport;
            break;
        case 'v':
            if (!str_cmp(name, "spell_ventriloquate")) return spell_ventriloquate;
            if (!str_cmp(name, "spell_vitalizing_presence")) return spell_vitalizing_presence;
            break;
        case 'w':
            if (!str_cmp(name, "spell_word_of_recall")) return spell_word_of_recall;
            if (!str_cmp(name, "spell_water_breathing")) return spell_water_breathing;
            if (!str_cmp(name, "spell_weaken")) return spell_weaken;
            if (!str_cmp(name, "spell_withering_enchant")) return spell_withering_enchant;
            if (!str_cmp(name, "spell_wizard_mark")) return spell_wizard_mark;
            if (!str_cmp(name, "spell_waves_of_weariness")) return spell_waves_of_weariness;
            if (!str_cmp(name, "spell_water_walk")) return spell_water_walk;
            break;
    }

    return spell_null;

} // end spell_function_lookup

/*
 * Returns the spell name for the given spell_ function.  This will be necessary to display
 * the friendly name in OLC that will be mapped back to the spell function.  Ideally these
 * functions wouldn't be necwssary
 */
char *spell_name_lookup(SPELL_FUN *spell)
{
    if (spell == spell_armor) return "spell_armor";
    if (spell == spell_acid_blast) return "spell_acid_blast";
    if (spell == spell_acid_breath) return "spell_acid_breath";
    if (spell == spell_bless) return "spell_bless";
    if (spell == spell_blindness) return "spell_blindness";
    if (spell == spell_burning_hands) return "spell_burning_hands";
    if (spell == spell_cancellation) return "spell_cancellation";
    if (spell == spell_curse) return "spell_curse";
    if (spell == spell_chain_lightning) return "spell_chain_lightning";
    if (spell == spell_create_food) return "spell_create_food";
    if (spell == spell_create_spring) return "spell_create_spring";
    if (spell == spell_cure_blindness) return "spell_cure_blindness";
    if (spell == spell_cure_critical) return "spell_cure_critical";
    if (spell == spell_cure_disease) return "spell_cure_disease";
    if (spell == spell_cure_poison) return "spell_cure_poison";
    if (spell == spell_calm) return "spell_calm";
    if (spell == spell_cause_critical) return "spell_cause_critical";
    if (spell == spell_cause_light) return "spell_cause_light";
    if (spell == spell_cause_serious) return "spell_cause_serious";
    if (spell == spell_change_sex) return "spell_change_sex";
    if (spell == spell_charm_person) return "spell_charm_person";
    if (spell == spell_chill_touch) return "spell_chill_touch";
    if (spell == spell_color_spray) return "spell_color_spray";
    if (spell == spell_continual_light) return "spell_continual_light";
    if (spell == spell_control_weather) return "spell_control_weather";
    if (spell == spell_create_water) return "spell_create_water";
    if (spell == spell_cure_light) return "spell_cure_light";
    if (spell == spell_cure_serious) return "spell_cure_serious";
    if (spell == spell_call_lightning) return "spell_call_lightning";
    if (spell == spell_create_rose) return "spell_create_rose";
    if (spell == spell_detect_hidden) return "spell_detect_hidden";
    if (spell == spell_detect_invis) return "spell_detect_invis";
    if (spell == spell_dispel_magic) return "spell_dispel_magic";
    if (spell == spell_detect_evil) return "spell_detect_evil";
    if (spell == spell_detect_good) return "spell_detect_good";
    if (spell == spell_detect_magic) return "spell_detect_magic";
    if (spell == spell_detect_poison) return "spell_detect_poison";
    if (spell == spell_dispel_evil) return "spell_dispel_evil";
    if (spell == spell_dispel_good) return "spell_dispel_good";
    if (spell == spell_disenchant) return "spell_disenchant";
    if (spell == spell_demonfire) return "spell_demonfire";
    if (spell == spell_earthquake) return "spell_earthquake";
    if (spell == spell_enchant_armor) return "spell_enchant_armor";
    if (spell == spell_enchant_weapon) return "spell_enchant_weapon";
    if (spell == spell_enchant_person) return "spell_enchant_person";
    if (spell == spell_enchant_gem) return "spell_enchant_gem";
    if (spell == spell_energy_drain) return "spell_energy_drain";
    if (spell == spell_fly) return "spell_fly";
    if (spell == spell_frenzy) return "spell_frenzy";
    if (spell == spell_fireball) return "spell_fireball";
    if (spell == spell_faerie_fog) return "spell_faerie_fog";
    if (spell == spell_faerie_fire) return "spell_faerie_fire";
    if (spell == spell_farsight) return "spell_farsight";
    if (spell == spell_fireproof) return "spell_fireproof";
    if (spell == spell_flamestrike) return "spell_flamestrike";
    if (spell == spell_floating_disc) return "spell_floating_disc";
    if (spell == spell_fire_breath) return "spell_fire_breath";
    if (spell == spell_frost_breath) return "spell_frost_breath";
    if (spell == spell_gate) return "spell_gate";
    if (spell == spell_giant_strength) return "spell_giant_strength";
    if (spell == spell_gas_breath) return "spell_gas_breath";
    if (spell == spell_general_purpose) return "spell_general_purpose";
    if (spell == spell_haste) return "spell_haste";
    if (spell == spell_heal) return "spell_heal";
    if (spell == spell_heat_metal) return "spell_heat_metal";
    if (spell == spell_harm) return "spell_harm";
    if (spell == spell_holy_word) return "spell_holy_word";
    if (spell == spell_high_explosive) return "spell_high_explosive";
    if (spell == spell_invis) return "spell_invis";
    if (spell == spell_identify) return "spell_identify";
    if (spell == spell_interlace_spirit) return "spell_interlace_spirit";
    if (spell == spell_infravision) return "spell_infravision";
    if (spell == spell_know_alignment) return "spell_know_alignment";
    if (spell == spell_locate_object) return "spell_locate_object";
    if (spell == spell_locate_bind) return "spell_locate_bind";
    if (spell == spell_lightning_bolt) return "spell_lightning_bolt";
    if (spell == spell_lightning_breath) return "spell_lightning_breath";
    if (spell == spell_locate_wizard_mark) return "spell_locate_wizard_mark";
    if (spell == spell_magic_missile) return "spell_magic_missile";
    if (spell == spell_mass_healing) return "spell_mass_healing";
    if (spell == spell_mass_invis) return "spell_mass_invis";
    if (spell == spell_null) return "spell_null";
    if (spell == spell_nexus) return "spell_nexus";
    if (spell == spell_poison) return "spell_poison";
    if (spell == spell_pass_door) return "spell_pass_door";
    if (spell == spell_plague) return "spell_plague";
    if (spell == spell_portal) return "spell_portal";
    if (spell == spell_protection_evil) return "spell_protection_evil";
    if (spell == spell_protection_good) return "spell_protection_good";
    if (spell == spell_protection_neutral) return "spell_protection_neutral";
    if (spell == spell_refresh) return "spell_refresh";
    if (spell == spell_restore_weapon) return "spell_restore_weapon";
    if (spell == spell_restore_armor) return "spell_restore_armor";
    if (spell == spell_ray_of_truth) return "spell_ray_of_truth";
    if (spell == spell_recharge) return "spell_recharge";
    if (spell == spell_remove_curse) return "spell_remove_curse";
    if (spell == spell_sanctuary) return "spell_sanctuary";
    if (spell == spell_shield) return "spell_shield";
    if (spell == spell_stone_skin) return "spell_stone_skin";
    if (spell == spell_summon) return "spell_summon";
    if (spell == spell_sleep) return "spell_sleep";
    if (spell == spell_slow) return "spell_slow";
    if (spell == spell_shocking_grasp) return "spell_shocking_grasp";
    if (spell == spell_sequestor) return "spell_sequestor";
    if (spell == spell_teleport) return "spell_teleport";
    if (spell == spell_ventriloquate) return "spell_ventriloquate";
    if (spell == spell_word_of_recall) return "spell_word_of_recall";
    if (spell == spell_water_breathing) return "spell_water_breathing";
    if (spell == spell_weaken) return "spell_weaken";
    if (spell == spell_withering_enchant) return "spell_withering_enchant";
    if (spell == spell_wizard_mark) return "spell_wizard_mark";
    if (spell == spell_waves_of_weariness) return "spell_waves_of_weariness";
    if (spell == spell_sacrificial_heal) return "spell_sacrificial_heal";
    if (spell == spell_mass_refresh) return "spell_mass_refresh";
    if (spell == spell_vitalizing_presence) return "spell_vitalizing_presence";
    if (spell == spell_life_boost) return "spell_life_boost";
    if (spell == spell_magic_resistance) return "spell_magic_resistance";
    if (spell == spell_mana_transfer) return "spell_mana_transfer";
    if (spell == spell_cure_weaken) return "spell_cure_weaken";
    if (spell == spell_restore_mental_presence) return "spell_restore_mental_presence";
    if (spell == spell_sense_affliction) return "spell_sense_affliction";
    if (spell == spell_cure_slow) return "spell_cure_slow";
    if (spell == spell_nourishment) return "spell_nourishment";
    if (spell == spell_enhanced_recovery) return "spell_enhanced_recovery";
    if (spell == spell_song_of_protection) return "spell_song_of_protection";
    if (spell == spell_song_of_dissonance) return "spell_song_of_dissonance";
    if (spell == spell_healers_bind) return "spell_healers_bind";
    if (spell == spell_cure_deafness) return "spell_cure_deafness";
    if (spell == spell_remove_faerie_fire) return "spell_remove_faerie_fire";
    if (spell == spell_bark_skin) return "spell_bark_skin";
    if (spell == spell_self_growth) return "spell_self_growth";
    if (spell == spell_fog) return "spell_fog";
    if (spell == spell_dispel_fog) return "spell_dispel_fog";
    if (spell == spell_imbue) return "spell_imbue";
    if (spell == spell_preserve) return "spell_preserve";
    if (spell == spell_ice_blast) return "spell_ice_blast";
    if (spell == spell_psionic_blast) return "spell_psionic_blast";
    if (spell == spell_healing_dream) return "spell_healing_dream";
    if (spell == spell_mental_weight) return "spell_mental_weight";
    if (spell == spell_forget) return "spell_forget";
    if (spell == spell_psionic_focus) return "spell_psionic_focus";
    if (spell == spell_clairvoyance) return "spell_clairvoyance";
    if (spell == spell_psionic_shield) return "spell_psionic_shield";
    if (spell == spell_boost) return "spell_boost";
    if (spell == spell_agony) return "spell_agony";
    if (spell == spell_holy_presence) return "spell_holy_presence";
    if (spell == spell_displacement) return "spell_displacement";
    if (spell == spell_holy_flame) return "spell_holy_flame";
    if (spell == spell_divine_wisdom) return "spell_divine_wisdom";
    if (spell == spell_know_religion) return "spell_know_religion";
    if (spell == spell_guardian_angel) return "spell_guardian_angel";
    if (spell == spell_water_walk) return "spell_water_walk";
    if (spell == spell_detect_fireproof) return "spell_detect_fireproof";
    if (spell == spell_self_projection) return "spell_self_projection";
    if (spell == spell_dark_vision) return "spell_dark_vision";
    if (spell == spell_increase_armor) return "spell_increase_armor";
    if (spell == spell_knock) return "spell_knock";
    if (spell == spell_extend_portal) return "spell_extend_portal";
    if (spell == spell_heaviness) return "spell_heaviness";
    if (spell == spell_feeble_mind) return "spell_feeble_mind";
    if (spell == spell_blink) return "spell_blink";
    if (spell == spell_mental_presence) return "spell_mental_presence";
    if (spell == spell_poison_spray) return "spell_poison_spray";
    if (spell == spell_false_life) return "spell_false_life";

    return "reserved";

} // end spell_name_lookup

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
 *  Portions of this code originated from a post made by Erwin Andreasen   *
 *  to the MERC/Envy mail list in January of 1998.  Additional code for    *
 *  assigning the GSN's originated from the Smaug code base and is needed  *
 *  to for setting the pointers in the skill table to the gsn where        *
 *  necessary.                                                             *
 *                                                                         *
 *  merc.h will have the global include for this file and then the         *
 *  IN_GSN_C flag will be set in db.c.  That should be all we need to set  *
 *  this up.                                                               *
 **************************************************************************/

/*
 * Declare the GSN in every file (via this being included in merc.h).  The
 * variable with be defined as an int in db.c and an extern in every
 * other file so they're accessible.
 */
#ifdef IN_GSN_C
    #define DECLARE_GSN(gsn) int gsn;
#else
    #define DECLARE_GSN(gsn) extern int gsn;
#endif

/*
 * Assign the GSN to the appropriate skill/spell.  Unfortunately this
 * will need to be called from a C file (in our case, you will need
 * to go update assign_gsn in db.c to map this gsn to the appripriate
 * entry in the skill table.
 */
#define ASSIGN_GSN(gsn, skill)                                          \
do                                                                      \
{                                                                       \
    if (((gsn) = skill_lookup((skill))) == -1)                          \
    {                                                                   \
        fprintf(stderr, "ASSIGN_GSN: Skill %s not found.\n", (skill) ); \
        global.last_boot_result = WARNING;                              \
    }                                                                   \
} while(0)

DECLARE_GSN(gsn_backstab)
DECLARE_GSN(gsn_dodge)
DECLARE_GSN(gsn_envenom)
DECLARE_GSN(gsn_hide)
DECLARE_GSN(gsn_peek)
DECLARE_GSN(gsn_pick_lock)
DECLARE_GSN(gsn_sneak)
DECLARE_GSN(gsn_steal)
DECLARE_GSN(gsn_disarm)
DECLARE_GSN(gsn_enhanced_damage)
DECLARE_GSN(gsn_kick)
DECLARE_GSN(gsn_parry)
DECLARE_GSN(gsn_rescue)
DECLARE_GSN(gsn_second_attack)
DECLARE_GSN(gsn_third_attack)
DECLARE_GSN(gsn_blindness)
DECLARE_GSN(gsn_charm_person)
DECLARE_GSN(gsn_change_sex)
DECLARE_GSN(gsn_curse)
DECLARE_GSN(gsn_chill_touch)
DECLARE_GSN(gsn_invisibility)
DECLARE_GSN(gsn_mass_invisibility)
DECLARE_GSN(gsn_plague)
DECLARE_GSN(gsn_poison)
DECLARE_GSN(gsn_sleep)
DECLARE_GSN(gsn_fly)
DECLARE_GSN(gsn_sanctuary)
DECLARE_GSN(gsn_axe)
DECLARE_GSN(gsn_dagger)
DECLARE_GSN(gsn_flail)
DECLARE_GSN(gsn_mace)
DECLARE_GSN(gsn_polearm)
DECLARE_GSN(gsn_shield_block)
DECLARE_GSN(gsn_spear)
DECLARE_GSN(gsn_sword)
DECLARE_GSN(gsn_whip)
DECLARE_GSN(gsn_bash)
DECLARE_GSN(gsn_berserk)
DECLARE_GSN(gsn_dirt)
DECLARE_GSN(gsn_hand_to_hand)
DECLARE_GSN(gsn_trip)
DECLARE_GSN(gsn_fast_healing)
DECLARE_GSN(gsn_haggle)
DECLARE_GSN(gsn_lore)
DECLARE_GSN(gsn_meditation)
DECLARE_GSN(gsn_scrolls)
DECLARE_GSN(gsn_staves)
DECLARE_GSN(gsn_wands)
DECLARE_GSN(gsn_recall)
DECLARE_GSN(gsn_dual_wield)
DECLARE_GSN(gsn_weaken)
DECLARE_GSN(gsn_water_breathing)
DECLARE_GSN(gsn_spellcraft)
DECLARE_GSN(gsn_swim)
DECLARE_GSN(gsn_vitalizing_presence)
DECLARE_GSN(gsn_sense_affliction)
DECLARE_GSN(gsn_slow)
DECLARE_GSN(gsn_immortal_blessing)
DECLARE_GSN(gsn_enhanced_recovery)
DECLARE_GSN(gsn_dirge)
DECLARE_GSN(gsn_stab)
DECLARE_GSN(gsn_disorientation)
DECLARE_GSN(gsn_blind_fighting)
DECLARE_GSN(gsn_bless)
DECLARE_GSN(gsn_song_of_dissonance)
DECLARE_GSN(gsn_song_of_protection)
DECLARE_GSN(gsn_enhanced_recall)
DECLARE_GSN(gsn_gore)
DECLARE_GSN(gsn_ghost)
DECLARE_GSN(gsn_enchant_person)
DECLARE_GSN(gsn_track)
DECLARE_GSN(gsn_acute_vision)
DECLARE_GSN(gsn_butcher)
DECLARE_GSN(gsn_bandage)
DECLARE_GSN(gsn_quiet_movement)
DECLARE_GSN(gsn_camping)
DECLARE_GSN(gsn_camouflage)
DECLARE_GSN(gsn_ambush)
DECLARE_GSN(gsn_find_water)
DECLARE_GSN(gsn_poison_prick)
DECLARE_GSN(gsn_shiv)
DECLARE_GSN(gsn_protection_good)
DECLARE_GSN(gsn_protection_neutral)
DECLARE_GSN(gsn_protection_evil)
DECLARE_GSN(gsn_escape)
DECLARE_GSN(gsn_peer)
DECLARE_GSN(gsn_bludgeon)
DECLARE_GSN(gsn_revolt)
DECLARE_GSN(gsn_imbue)
DECLARE_GSN(gsn_preserve)
DECLARE_GSN(gsn_haste)
DECLARE_GSN(gsn_giant_strength)
DECLARE_GSN(gsn_armor)
DECLARE_GSN(gsn_calm)
DECLARE_GSN(gsn_detect_evil)
DECLARE_GSN(gsn_detect_good)
DECLARE_GSN(gsn_detect_hidden)
DECLARE_GSN(gsn_detect_invis)
DECLARE_GSN(gsn_detect_magic)
DECLARE_GSN(gsn_faerie_fire)
DECLARE_GSN(gsn_frenzy)
DECLARE_GSN(gsn_infravision);
DECLARE_GSN(gsn_pass_door);
DECLARE_GSN(gsn_shield);
DECLARE_GSN(gsn_stone_skin);
DECLARE_GSN(gsn_life_boost);
DECLARE_GSN(gsn_bark_skin);
DECLARE_GSN(gsn_self_growth);
DECLARE_GSN(gsn_cure_blindness);
DECLARE_GSN(gsn_cure_light);
DECLARE_GSN(gsn_cure_serious);
DECLARE_GSN(gsn_cure_critical);
DECLARE_GSN(gsn_cure_poison);
DECLARE_GSN(gsn_refresh);
DECLARE_GSN(gsn_cure_disease);
DECLARE_GSN(gsn_gas_breath);
DECLARE_GSN(gsn_fire_breath);
DECLARE_GSN(gsn_high_explosive);
DECLARE_GSN(gsn_heal);
DECLARE_GSN(gsn_cancellation);
DECLARE_GSN(gsn_remove_curse);
DECLARE_GSN(gsn_healing_dream);
DECLARE_GSN(gsn_mental_weight);
DECLARE_GSN(gsn_forget);
DECLARE_GSN(gsn_psionic_focus);
DECLARE_GSN(gsn_clairvoyance);
DECLARE_GSN(gsn_psionic_shield);
DECLARE_GSN(gsn_boost);
DECLARE_GSN(gsn_magic_resistance);
DECLARE_GSN(gsn_prayer);
DECLARE_GSN(gsn_agony);
DECLARE_GSN(gsn_holy_presence);
DECLARE_GSN(gsn_displacement);
DECLARE_GSN(gsn_holy_flame);
DECLARE_GSN(gsn_divine_wisdom);
DECLARE_GSN(gsn_guardian_angel);
DECLARE_GSN(gsn_water_walk);
DECLARE_GSN(gsn_fourth_attack);
DECLARE_GSN(gsn_warcry);
DECLARE_GSN(gsn_cleanse);
DECLARE_GSN(gsn_natural_refresh);
DECLARE_GSN(gsn_power_swing);
DECLARE_GSN(gsn_second_wind);
DECLARE_GSN(gsn_magic_protection);
DECLARE_GSN(gsn_precision_striking);
DECLARE_GSN(gsn_detect_fireproof);
DECLARE_GSN(gsn_survey_terrain);
DECLARE_GSN(gsn_smokebomb);
DECLARE_GSN(gsn_self_projection);
DECLARE_GSN(gsn_curse_of_the_storm);
DECLARE_GSN(gsn_dark_vision);
DECLARE_GSN(gsn_increased_damage);
DECLARE_GSN(gsn_heaviness);
DECLARE_GSN(gsn_feeble_mind);
DECLARE_GSN(gsn_blink);
DECLARE_GSN(gsn_mental_presence);
DECLARE_GSN(gsn_poison_spray);
DECLARE_GSN(gsn_false_life);

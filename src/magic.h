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

/*
 * Spell functions.
 * Defined in magic.c.
 */
DECLARE_SPELL_FUN(spell_null);
DECLARE_SPELL_FUN(spell_acid_blast);
DECLARE_SPELL_FUN(spell_armor);
DECLARE_SPELL_FUN(spell_bless);
DECLARE_SPELL_FUN(spell_blindness);
DECLARE_SPELL_FUN(spell_burning_hands);
DECLARE_SPELL_FUN(spell_call_lightning);
DECLARE_SPELL_FUN(spell_calm);
DECLARE_SPELL_FUN(spell_cancellation);
DECLARE_SPELL_FUN(spell_cause_critical);
DECLARE_SPELL_FUN(spell_cause_light);
DECLARE_SPELL_FUN(spell_cause_serious);
DECLARE_SPELL_FUN(spell_change_sex);
DECLARE_SPELL_FUN(spell_chain_lightning);
DECLARE_SPELL_FUN(spell_charm_person);
DECLARE_SPELL_FUN(spell_chill_touch);
DECLARE_SPELL_FUN(spell_color_spray);
DECLARE_SPELL_FUN(spell_continual_light);
DECLARE_SPELL_FUN(spell_control_weather);
DECLARE_SPELL_FUN(spell_create_food);
DECLARE_SPELL_FUN(spell_create_rose);
DECLARE_SPELL_FUN(spell_create_spring);
DECLARE_SPELL_FUN(spell_create_water);
DECLARE_SPELL_FUN(spell_cure_blindness);
DECLARE_SPELL_FUN(spell_cure_critical);
DECLARE_SPELL_FUN(spell_cure_disease);
DECLARE_SPELL_FUN(spell_cure_light);
DECLARE_SPELL_FUN(spell_cure_poison);
DECLARE_SPELL_FUN(spell_cure_serious);
DECLARE_SPELL_FUN(spell_curse);
DECLARE_SPELL_FUN(spell_demonfire);
DECLARE_SPELL_FUN(spell_detect_evil);
DECLARE_SPELL_FUN(spell_detect_good);
DECLARE_SPELL_FUN(spell_detect_hidden);
DECLARE_SPELL_FUN(spell_detect_invis);
DECLARE_SPELL_FUN(spell_detect_magic);
DECLARE_SPELL_FUN(spell_detect_poison);
DECLARE_SPELL_FUN(spell_dispel_evil);
DECLARE_SPELL_FUN(spell_dispel_good);
DECLARE_SPELL_FUN(spell_dispel_magic);
DECLARE_SPELL_FUN(spell_earthquake);
DECLARE_SPELL_FUN(spell_enchant_armor);
DECLARE_SPELL_FUN(spell_enchant_weapon);
DECLARE_SPELL_FUN(spell_energy_drain);
DECLARE_SPELL_FUN(spell_faerie_fire);
DECLARE_SPELL_FUN(spell_faerie_fog);
DECLARE_SPELL_FUN(spell_farsight);
DECLARE_SPELL_FUN(spell_fireball);
DECLARE_SPELL_FUN(spell_fireproof);
DECLARE_SPELL_FUN(spell_flamestrike);
DECLARE_SPELL_FUN(spell_floating_disc);
DECLARE_SPELL_FUN(spell_fly);
DECLARE_SPELL_FUN(spell_frenzy);
DECLARE_SPELL_FUN(spell_gate);
DECLARE_SPELL_FUN(spell_giant_strength);
DECLARE_SPELL_FUN(spell_harm);
DECLARE_SPELL_FUN(spell_haste);
DECLARE_SPELL_FUN(spell_heal);
DECLARE_SPELL_FUN(spell_heat_metal);
DECLARE_SPELL_FUN(spell_holy_word);
DECLARE_SPELL_FUN(spell_identify);
DECLARE_SPELL_FUN(spell_infravision);
DECLARE_SPELL_FUN(spell_invis);
DECLARE_SPELL_FUN(spell_know_alignment);
DECLARE_SPELL_FUN(spell_lightning_bolt);
DECLARE_SPELL_FUN(spell_locate_object);
DECLARE_SPELL_FUN(spell_magic_missile);
DECLARE_SPELL_FUN(spell_mass_healing);
DECLARE_SPELL_FUN(spell_mass_invis);
DECLARE_SPELL_FUN(spell_nexus);
DECLARE_SPELL_FUN(spell_pass_door);
DECLARE_SPELL_FUN(spell_plague);
DECLARE_SPELL_FUN(spell_poison);
DECLARE_SPELL_FUN(spell_portal);
DECLARE_SPELL_FUN(spell_protection_evil);
DECLARE_SPELL_FUN(spell_protection_good);
DECLARE_SPELL_FUN(spell_ray_of_truth);
DECLARE_SPELL_FUN(spell_recharge);
DECLARE_SPELL_FUN(spell_refresh);
DECLARE_SPELL_FUN(spell_remove_curse);
DECLARE_SPELL_FUN(spell_sanctuary);
DECLARE_SPELL_FUN(spell_shocking_grasp);
DECLARE_SPELL_FUN(spell_shield);
DECLARE_SPELL_FUN(spell_sleep);
DECLARE_SPELL_FUN(spell_slow);
DECLARE_SPELL_FUN(spell_stone_skin);
DECLARE_SPELL_FUN(spell_summon);
DECLARE_SPELL_FUN(spell_teleport);
DECLARE_SPELL_FUN(spell_ventriloquate);
DECLARE_SPELL_FUN(spell_weaken);
DECLARE_SPELL_FUN(spell_word_of_recall);
DECLARE_SPELL_FUN(spell_acid_breath);
DECLARE_SPELL_FUN(spell_fire_breath);
DECLARE_SPELL_FUN(spell_frost_breath);
DECLARE_SPELL_FUN(spell_gas_breath);
DECLARE_SPELL_FUN(spell_lightning_breath);
DECLARE_SPELL_FUN(spell_general_purpose);
DECLARE_SPELL_FUN(spell_high_explosive);
DECLARE_SPELL_FUN(spell_withering_enchant);
DECLARE_SPELL_FUN(spell_enchant_person);
DECLARE_SPELL_FUN(spell_sequestor);
DECLARE_SPELL_FUN(spell_interlace_spirit);
DECLARE_SPELL_FUN(spell_wizard_mark);
DECLARE_SPELL_FUN(spell_enchant_gem);
DECLARE_SPELL_FUN(spell_restore_weapon);
DECLARE_SPELL_FUN(spell_restore_armor);
DECLARE_SPELL_FUN(spell_disenchant);
DECLARE_SPELL_FUN(spell_locate_wizard_mark);
DECLARE_SPELL_FUN(spell_water_breathing);
DECLARE_SPELL_FUN(spell_waves_of_weariness);
DECLARE_SPELL_FUN(spell_sacrificial_heal);
DECLARE_SPELL_FUN(spell_mass_refresh);
DECLARE_SPELL_FUN(spell_vitalizing_presence);
DECLARE_SPELL_FUN(spell_life_boost);
DECLARE_SPELL_FUN(spell_magic_resistance);
DECLARE_SPELL_FUN(spell_mana_transfer);
DECLARE_SPELL_FUN(spell_cure_weaken);
DECLARE_SPELL_FUN(spell_restore_mental_presence);
DECLARE_SPELL_FUN(spell_sense_affliction);
DECLARE_SPELL_FUN(spell_cure_slow);
DECLARE_SPELL_FUN(spell_nourishment);
DECLARE_SPELL_FUN(spell_enhanced_recovery);
DECLARE_SPELL_FUN(spell_song_of_protection);
DECLARE_SPELL_FUN(spell_song_of_dissonance);
DECLARE_SPELL_FUN(spell_healers_bind);
DECLARE_SPELL_FUN(spell_cure_deafness);
DECLARE_SPELL_FUN(spell_remove_faerie_fire);
DECLARE_SPELL_FUN(spell_detect_door);
DECLARE_SPELL_FUN(spell_bark_skin);
DECLARE_SPELL_FUN(spell_self_growth);
DECLARE_SPELL_FUN(spell_imbue);
DECLARE_SPELL_FUN(spell_preserve);
DECLARE_SPELL_FUN(spell_ice_blast);
DECLARE_SPELL_FUN(spell_psionic_blast);
DECLARE_SPELL_FUN(spell_healing_dream);
DECLARE_SPELL_FUN(spell_mental_weight);
DECLARE_SPELL_FUN(spell_forget);
DECLARE_SPELL_FUN(spell_psionic_focus);
DECLARE_SPELL_FUN(spell_clairvoyance);
DECLARE_SPELL_FUN(spell_psionic_shield);
DECLARE_SPELL_FUN(spell_boost);
DECLARE_SPELL_FUN(spell_agony);
DECLARE_SPELL_FUN(spell_holy_presence);
DECLARE_SPELL_FUN(spell_displacement);
DECLARE_SPELL_FUN(spell_holy_flame);
DECLARE_SPELL_FUN(spell_divine_wisdom);
DECLARE_SPELL_FUN(spell_know_religion);
DECLARE_SPELL_FUN(spell_guardian_angel);
DECLARE_SPELL_FUN(spell_water_walk);
DECLARE_SPELL_FUN(spell_detect_fireproof);
DECLARE_SPELL_FUN(spell_self_projection);
DECLARE_SPELL_FUN(spell_dark_vision);
DECLARE_SPELL_FUN(spell_increase_armor);
DECLARE_SPELL_FUN(spell_knock);
DECLARE_SPELL_FUN(spell_extend_portal);
DECLARE_SPELL_FUN(spell_heaviness);
DECLARE_SPELL_FUN(spell_feeble_mind);
DECLARE_SPELL_FUN(spell_blink);
DECLARE_SPELL_FUN(spell_mental_presence);
DECLARE_SPELL_FUN(spell_poison_spray);
DECLARE_SPELL_FUN(spell_false_life);

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

/* this is a listing of all the commands and command related data */

/* wrapper function for safe command execution */
void do_function(CHAR_DATA *ch, DO_FUN *do_fun, char *argument);

/* for command types */
#define ML 	MAX_LEVEL	/* implementor */
#define L1	MAX_LEVEL - 1  	/* admin */
#define L2	MAX_LEVEL - 2	/* coder */
#define L3	MAX_LEVEL - 3	/* builder */
#define L4 	MAX_LEVEL - 4	/* quest */
#define L5	MAX_LEVEL - 5	/* role-play */
#define L6	MAX_LEVEL - 6	/* story */
#define L7	MAX_LEVEL - 7	/* retired */
#define L8	MAX_LEVEL - 8	/* trial imm */
#define IM	LEVEL_IMMORTAL 	/* avatar */

/*
 * Structure for a command in the command lookup table.
 */
struct cmd_type
{
    char * const name;
    DO_FUN * do_fun;
    int position;
    int level;
    int log;
    int show;
};

/* the command table itself */
extern const struct   cmd_type   cmd_table[];

/*
 * Command functions.
 */
DECLARE_DO_FUN(do_advance);
DECLARE_DO_FUN(do_affects);
DECLARE_DO_FUN(do_afk);
DECLARE_DO_FUN(do_alia);
DECLARE_DO_FUN(do_alias);
DECLARE_DO_FUN(do_allow);
DECLARE_DO_FUN(do_answer);
DECLARE_DO_FUN(do_areas);
DECLARE_DO_FUN(do_at);
DECLARE_DO_FUN(do_auction);
DECLARE_DO_FUN(do_autoall);
DECLARE_DO_FUN(do_autoassist);
DECLARE_DO_FUN(do_autoexit);
DECLARE_DO_FUN(do_autogold);
DECLARE_DO_FUN(do_autolist);
DECLARE_DO_FUN(do_autoloot);
DECLARE_DO_FUN(do_autosac);
DECLARE_DO_FUN(do_autosplit);
DECLARE_DO_FUN(do_backstab);
DECLARE_DO_FUN(do_bamfin);
DECLARE_DO_FUN(do_bamfout);
DECLARE_DO_FUN(do_ban);
DECLARE_DO_FUN(do_bash);
DECLARE_DO_FUN(do_berserk);
DECLARE_DO_FUN(do_brandish);
DECLARE_DO_FUN(do_brief);
DECLARE_DO_FUN(do_bug);
DECLARE_DO_FUN(do_buy);
DECLARE_DO_FUN(do_cast);
DECLARE_DO_FUN(do_channels);
DECLARE_DO_FUN(do_clone);
DECLARE_DO_FUN(do_close);
DECLARE_DO_FUN(do_color);
DECLARE_DO_FUN(do_commands);
DECLARE_DO_FUN(do_combine);
DECLARE_DO_FUN(do_compact);
DECLARE_DO_FUN(do_compare);
DECLARE_DO_FUN(do_consider);
DECLARE_DO_FUN(do_copyover);
DECLARE_DO_FUN(do_count);
DECLARE_DO_FUN(do_credits);
DECLARE_DO_FUN(do_deaf);
DECLARE_DO_FUN(do_delet);
DECLARE_DO_FUN(do_delete);
DECLARE_DO_FUN(do_deny);
DECLARE_DO_FUN(do_description);
DECLARE_DO_FUN(do_dirt);
DECLARE_DO_FUN(do_disarm);
DECLARE_DO_FUN(do_disconnect);
DECLARE_DO_FUN(do_down);
DECLARE_DO_FUN(do_drink);
DECLARE_DO_FUN(do_drop);
DECLARE_DO_FUN(do_dump);
DECLARE_DO_FUN(do_east);
DECLARE_DO_FUN(do_eat);
DECLARE_DO_FUN(do_echo);
DECLARE_DO_FUN(do_emote);
DECLARE_DO_FUN(do_enter);
DECLARE_DO_FUN(do_envenom);
DECLARE_DO_FUN(do_equipment);
DECLARE_DO_FUN(do_examine);
DECLARE_DO_FUN(do_exits);
DECLARE_DO_FUN(do_fill);
DECLARE_DO_FUN(do_flag);
DECLARE_DO_FUN(do_flee);
DECLARE_DO_FUN(do_follow);
DECLARE_DO_FUN(do_force);
DECLARE_DO_FUN(do_freeze);
DECLARE_DO_FUN(do_gain);
DECLARE_DO_FUN(do_get);
DECLARE_DO_FUN(do_give);
DECLARE_DO_FUN(do_gossip);
DECLARE_DO_FUN(do_goto);
DECLARE_DO_FUN(do_grats);
DECLARE_DO_FUN(do_group);
DECLARE_DO_FUN(do_groups);
DECLARE_DO_FUN(do_gtell);
DECLARE_DO_FUN(do_guild);
DECLARE_DO_FUN(do_heal);
DECLARE_DO_FUN(do_help);
DECLARE_DO_FUN(do_hide);
DECLARE_DO_FUN(do_holylight);
DECLARE_DO_FUN(do_immtalk);
DECLARE_DO_FUN(do_incognito);
DECLARE_DO_FUN(do_clantalk);
DECLARE_DO_FUN(do_imotd);
DECLARE_DO_FUN(do_inventory);
DECLARE_DO_FUN(do_invis);
DECLARE_DO_FUN(do_kick);
DECLARE_DO_FUN(do_kill);
DECLARE_DO_FUN(do_list);
DECLARE_DO_FUN(do_load);
DECLARE_DO_FUN(do_lock);
DECLARE_DO_FUN(do_log);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_memory);
DECLARE_DO_FUN(do_mfind);
DECLARE_DO_FUN(do_mset);
DECLARE_DO_FUN(do_mstat);
DECLARE_DO_FUN(do_player_offline_stat);
DECLARE_DO_FUN(do_mwhere);
DECLARE_DO_FUN(do_mob);
DECLARE_DO_FUN(do_motd);
DECLARE_DO_FUN(do_mpstat);
DECLARE_DO_FUN(do_mpdump);
DECLARE_DO_FUN(do_murde);
DECLARE_DO_FUN(do_murder);
DECLARE_DO_FUN(do_newlock);
DECLARE_DO_FUN(do_nochannels);
DECLARE_DO_FUN(do_noemote);
DECLARE_DO_FUN(do_nofollow);
DECLARE_DO_FUN(do_noloot);
DECLARE_DO_FUN(do_north);
DECLARE_DO_FUN(do_noshout);
DECLARE_DO_FUN(do_nosummon);
DECLARE_DO_FUN(do_note);
DECLARE_DO_FUN(do_notell);
DECLARE_DO_FUN(do_ofind);
DECLARE_DO_FUN(do_open);
DECLARE_DO_FUN(do_order);
DECLARE_DO_FUN(do_oset);
DECLARE_DO_FUN(do_ostat);
DECLARE_DO_FUN(do_outfit);
DECLARE_DO_FUN(do_owhere);
DECLARE_DO_FUN(do_pardon);
DECLARE_DO_FUN(do_password);
DECLARE_DO_FUN(do_peace);
DECLARE_DO_FUN(do_pecho);
DECLARE_DO_FUN(do_permban);
DECLARE_DO_FUN(do_pick);
DECLARE_DO_FUN(do_pmote);
DECLARE_DO_FUN(do_pour);
DECLARE_DO_FUN(do_practice);
DECLARE_DO_FUN(do_prefi);
DECLARE_DO_FUN(do_prefix);
DECLARE_DO_FUN(do_prompt);
DECLARE_DO_FUN(do_protect);
DECLARE_DO_FUN(do_purge);
DECLARE_DO_FUN(do_put);
DECLARE_DO_FUN(do_quaff);
DECLARE_DO_FUN(do_question);
DECLARE_DO_FUN(do_qui);
DECLARE_DO_FUN(do_quiet);
DECLARE_DO_FUN(do_quit);
DECLARE_DO_FUN(do_reboo);
DECLARE_DO_FUN(do_reboot);
DECLARE_DO_FUN(do_recall);
DECLARE_DO_FUN(do_recho);
DECLARE_DO_FUN(do_recite);
DECLARE_DO_FUN(do_remove);
DECLARE_DO_FUN(do_replay);
DECLARE_DO_FUN(do_reply);
DECLARE_DO_FUN(do_report);
DECLARE_DO_FUN(do_rescue);
DECLARE_DO_FUN(do_rest);
DECLARE_DO_FUN(do_restore);
DECLARE_DO_FUN(do_return);
DECLARE_DO_FUN(do_rset);
DECLARE_DO_FUN(do_rstat);
DECLARE_DO_FUN(do_rules);
DECLARE_DO_FUN(do_sacrifice);
DECLARE_DO_FUN(do_save);
DECLARE_DO_FUN(do_say);
DECLARE_DO_FUN(do_scan);
DECLARE_DO_FUN(do_score);
DECLARE_DO_FUN(do_scroll);
DECLARE_DO_FUN(do_sell);
DECLARE_DO_FUN(do_set);
DECLARE_DO_FUN(do_show);
DECLARE_DO_FUN(do_shutdow);
DECLARE_DO_FUN(do_shutdown);
DECLARE_DO_FUN(do_sit);
DECLARE_DO_FUN(do_skills);
DECLARE_DO_FUN(do_sla);
DECLARE_DO_FUN(do_slay);
DECLARE_DO_FUN(do_sleep);
DECLARE_DO_FUN(do_slookup);
DECLARE_DO_FUN(do_smote);
DECLARE_DO_FUN(do_sneak);
DECLARE_DO_FUN(do_snoop);
DECLARE_DO_FUN(do_socials);
DECLARE_DO_FUN(do_south);
DECLARE_DO_FUN(do_sockets);
DECLARE_DO_FUN(do_spells);
DECLARE_DO_FUN(do_split);
DECLARE_DO_FUN(do_sset);
DECLARE_DO_FUN(do_stand);
DECLARE_DO_FUN(do_stat);
DECLARE_DO_FUN(do_steal);
DECLARE_DO_FUN(do_string);
DECLARE_DO_FUN(do_surrender);
DECLARE_DO_FUN(do_switch);
DECLARE_DO_FUN(do_tell);
DECLARE_DO_FUN(do_telnetga);
DECLARE_DO_FUN(do_time);
DECLARE_DO_FUN(do_title);
DECLARE_DO_FUN(do_train);
DECLARE_DO_FUN(do_transfer);
DECLARE_DO_FUN(do_trip);
DECLARE_DO_FUN(do_trust);
DECLARE_DO_FUN(do_typo);
DECLARE_DO_FUN(do_unalias);
DECLARE_DO_FUN(do_unlock);
DECLARE_DO_FUN(do_up);
DECLARE_DO_FUN(do_value);
DECLARE_DO_FUN(do_visible);
DECLARE_DO_FUN(do_violate);
DECLARE_DO_FUN(do_vnum);
DECLARE_DO_FUN(do_wake);
DECLARE_DO_FUN(do_wear);
DECLARE_DO_FUN(do_weather);
DECLARE_DO_FUN(do_west);
DECLARE_DO_FUN(do_where);
DECLARE_DO_FUN(do_who);
DECLARE_DO_FUN(do_whoclan);
DECLARE_DO_FUN(do_whois);
DECLARE_DO_FUN(do_whoami);
DECLARE_DO_FUN(do_wimpy);
DECLARE_DO_FUN(do_wizhelp);
DECLARE_DO_FUN(do_wizlock);
DECLARE_DO_FUN(do_wizlist);
DECLARE_DO_FUN(do_wiznet);
DECLARE_DO_FUN(do_worth);
DECLARE_DO_FUN(do_yell);
DECLARE_DO_FUN(do_zap);
DECLARE_DO_FUN(do_zecho);
DECLARE_DO_FUN(do_olc);
DECLARE_DO_FUN(do_asave);
DECLARE_DO_FUN(do_alist);
DECLARE_DO_FUN(do_resets);
DECLARE_DO_FUN(do_redit);
DECLARE_DO_FUN(do_aedit);
DECLARE_DO_FUN(do_medit);
DECLARE_DO_FUN(do_oedit);
DECLARE_DO_FUN(do_mpedit);
DECLARE_DO_FUN(do_hedit);
DECLARE_DO_FUN(do_qmread);
DECLARE_DO_FUN(do_debug);
DECLARE_DO_FUN(do_clear);
DECLARE_DO_FUN(do_forcetick);
DECLARE_DO_FUN(do_rename);
DECLARE_DO_FUN(do_vnumgap);
DECLARE_DO_FUN(do_exlist);
DECLARE_DO_FUN(do_broadcast);
DECLARE_DO_FUN(do_northeast);
DECLARE_DO_FUN(do_northwest);
DECLARE_DO_FUN(do_southeast);
DECLARE_DO_FUN(do_southwest);
DECLARE_DO_FUN(do_pray);
DECLARE_DO_FUN(do_reclass);
DECLARE_DO_FUN(do_pathfind);
DECLARE_DO_FUN(do_ooc_spool);
DECLARE_DO_FUN(do_news);
DECLARE_DO_FUN(do_changes);
DECLARE_DO_FUN(do_story_spool);
DECLARE_DO_FUN(do_history_spool);
DECLARE_DO_FUN(do_penalty);
DECLARE_DO_FUN(do_imm_spool);
DECLARE_DO_FUN(do_catchup);
DECLARE_DO_FUN(do_unread);
DECLARE_DO_FUN(do_stats);
DECLARE_DO_FUN(do_second);
DECLARE_DO_FUN(do_cecho);
DECLARE_DO_FUN(do_test);
DECLARE_DO_FUN(do_guildlist);
DECLARE_DO_FUN(do_land);
DECLARE_DO_FUN(do_wizcancel);
DECLARE_DO_FUN(do_wizbless);
DECLARE_DO_FUN(do_direct);
DECLARE_DO_FUN(do_class);
DECLARE_DO_FUN(do_dirge);
DECLARE_DO_FUN(do_disorient);
DECLARE_DO_FUN(do_stab);
DECLARE_DO_FUN(do_touch);
DECLARE_DO_FUN(do_cgossip);
DECLARE_DO_FUN(do_ooc);
DECLARE_DO_FUN(do_oclantalk);
DECLARE_DO_FUN(do_settings);
DECLARE_DO_FUN(do_gore);
DECLARE_DO_FUN(do_bury);
DECLARE_DO_FUN(do_dig);
DECLARE_DO_FUN(do_flipcoin);
DECLARE_DO_FUN(do_portal);
DECLARE_DO_FUN(do_confiscate);
DECLARE_DO_FUN(do_terrain);
DECLARE_DO_FUN(do_use);
DECLARE_DO_FUN(do_version);
DECLARE_DO_FUN(do_disable);
DECLARE_DO_FUN(do_butcher);
DECLARE_DO_FUN(do_bandage);
DECLARE_DO_FUN(do_quiet_movement);
DECLARE_DO_FUN(do_camp);
DECLARE_DO_FUN(do_camouflage);
DECLARE_DO_FUN(do_ambush);
DECLARE_DO_FUN(do_track);
DECLARE_DO_FUN(do_find_water);
DECLARE_DO_FUN(do_crypt);
DECLARE_DO_FUN(do_nocancel);
DECLARE_DO_FUN(do_random_names);
DECLARE_DO_FUN(do_stance);
DECLARE_DO_FUN(do_map);
DECLARE_DO_FUN(do_bind);
DECLARE_DO_FUN(do_pquest);
DECLARE_DO_FUN(do_poisonprick);
DECLARE_DO_FUN(do_shiv);
DECLARE_DO_FUN(do_escape);
DECLARE_DO_FUN(do_peer);
DECLARE_DO_FUN(do_snoopinfo);
DECLARE_DO_FUN(do_switchinfo);
DECLARE_DO_FUN(do_knock);
DECLARE_DO_FUN(do_skillstat);
DECLARE_DO_FUN(do_spellstat);
DECLARE_DO_FUN(do_clearreply);
DECLARE_DO_FUN(do_loner);
DECLARE_DO_FUN(do_bludgeon);
DECLARE_DO_FUN(do_revolt);
DECLARE_DO_FUN(do_linefeed);
DECLARE_DO_FUN(do_rfind);
DECLARE_DO_FUN(do_permanent);
DECLARE_DO_FUN(do_playerlist);
DECLARE_DO_FUN(do_nopray);
DECLARE_DO_FUN(do_write);
DECLARE_DO_FUN(do_read);
DECLARE_DO_FUN(do_duplicate);
DECLARE_DO_FUN(do_bank);
DECLARE_DO_FUN(do_clairvoyance);
DECLARE_DO_FUN(do_prayer);
DECLARE_DO_FUN(do_deity);
DECLARE_DO_FUN(do_warcry);
DECLARE_DO_FUN(do_cleanse);
DECLARE_DO_FUN(do_power_swing);
DECLARE_DO_FUN(do_meritlist);
DECLARE_DO_FUN(do_race);
DECLARE_DO_FUN(do_autoquit);
DECLARE_DO_FUN(do_draw);
DECLARE_DO_FUN(do_tnl);
DECLARE_DO_FUN(do_empty);
DECLARE_DO_FUN(do_pload);
DECLARE_DO_FUN(do_punload);
DECLARE_DO_FUN(do_smokebomb);
DECLARE_DO_FUN(do_glance);
DECLARE_DO_FUN(do_whisper);
DECLARE_DO_FUN(do_keyring);
DECLARE_DO_FUN(do_improve);
DECLARE_DO_FUN(do_rank);
DECLARE_DO_FUN(do_consent);
DECLARE_DO_FUN(do_renegade);
DECLARE_DO_FUN(do_keeps);
DECLARE_DO_FUN(do_setstorm);
DECLARE_DO_FUN(do_database);
DECLARE_DO_FUN(do_finger);
DECLARE_DO_FUN(do_mobprog_find);
DECLARE_DO_FUN(do_uninvisible);

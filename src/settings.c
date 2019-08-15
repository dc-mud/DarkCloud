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

 /**************************************************************************
 *  File: settings.c                                                       *
 *                                                                         *
 *  This file contains the code to load, save and manipulate game settings *
 *  that are persisted across boots.  It currently reads in writes in      *
 *  standard ROM fashion but is going to be changed to use the new INI     *
 *  parsing code the near future.                                          *
 *                                                                         *
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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "merc.h"
#include "interp.h"
#include "ini.h"
#include "tables.h"
#include "lookup.h"

// Local functions
void save_settings(void);

/*
 * Command to show a character the current game settings, locks, etc. setup by the mud
 * mud admin.  Potentially also show player based settings here as that list grows.
 */
void do_settings(CHAR_DATA* ch, char* argument)
{
	if (IS_NPC(ch))
	{
		send_to_char("NPC's are not allowed to use the settings command.\r\n", ch);
		return;
	}

	char buf[MAX_STRING_LENGTH];
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	// Show the game and player settings
	if (IS_NULLSTR(argument))
	{
		send_to_char("--------------------------------------------------------------------------------\r\n", ch);
		send_to_char("{WGame Settings{x\r\n", ch);
		send_to_char("--------------------------------------------------------------------------------\r\n", ch);

		sprintf(buf, "%-25s %-7s  %-25s %-7s\r\n",
			"Game Locked (Wizlock)", settings.wizlock ? "{GON{x" : "{ROFF{x",
			"New Lock", settings.newlock ? "{GON{x" : "{ROFF{x");
		send_to_char(buf, ch);

		sprintf(buf, "%-25s %-7s  %-25s %-7s\r\n",
			"Double Experience", settings.double_exp ? "{GON{x" : "{ROFF{x",
			"Double Gold", settings.double_gold ? "{GON{x" : "{ROFF{x");
		send_to_char(buf, ch);

		sprintf(buf, "%-25s %-7s  %-25s %-7s\r\n",
			"Test Mode", settings.test_mode ? "{GON{x" : "{ROFF{x",
			"Whitelist Lock", settings.whitelist_lock ? "{GON{x" : "{ROFF{x");
		send_to_char(buf, ch);

		sprintf(buf, "%-25s %-7s  %-25s %-7s\r\n",
			"Login Color Prompt", settings.login_color_prompt ? "{GON{x" : "{ROFF{x",
			"Login Who List Enabled", settings.login_who_list_enabled ? "{GON{x" : "{ROFF{x");
		send_to_char(buf, ch);

		sprintf(buf, "%-25s %-7s\r\n",
			"Database Logging", settings.db_logging ? "{GON{x" : "{ROFF{x");
		send_to_char(buf, ch);

		send_to_char("\r\n", ch);
		send_to_char("--------------------------------------------------------------------------------\r\n", ch);
		send_to_char("{WGame Mechanics{x\r\n", ch);
		send_to_char("--------------------------------------------------------------------------------\r\n", ch);

		sprintf(buf, "%-25s %-7s  %-25s %-7s\r\n",
			"Gain Convert", settings.gain_convert ? "{GON{x" : "{ROFF{x",
			"Shock Spread", settings.shock_spread ? "{GON{x" : "{ROFF{x");
		send_to_char(buf, ch);

		sprintf(buf, "%-25s %-4d %-25s %-7s\r\n",
			"Stat Surge", settings.stat_surge,
			"Hours Affect Experience", settings.hours_affect_exp ? "{GON{x" : "{ROFF{x");
		send_to_char(buf, ch);

		sprintf(buf, "%-25s %-7s\r\n",
			"Focused Improves", settings.focused_improves ? "{GON{x" : "{ROFF{x");
		send_to_char(buf, ch);

		send_to_char("\r\n", ch);
		send_to_char("--------------------------------------------------------------------------------\r\n", ch);
		send_to_char("{WGame Info{x\r\n", ch);
		send_to_char("--------------------------------------------------------------------------------\r\n", ch);

		sprintf(buf, "%-25s %s\r\n",
			"Mud Name", IS_NULLSTR(settings.mud_name) ? "N/A" : settings.mud_name);
		send_to_char(buf, ch);

		sprintf(buf, "%-25s %s\r\n",
			"Web Page URL", IS_NULLSTR(settings.web_page_url) ? "N/A" : settings.web_page_url);
		send_to_char(buf, ch);

		sprintf(buf, "%-25s %s\r\n",
			"Login Greeting", IS_NULLSTR(settings.login_greeting) ? "N/A" : settings.login_greeting);
		send_to_char(buf, ch);

	}

	send_to_char("\r\n", ch);

	// Since this is a dual mortal/immortal command, kick the mortals out here and lower level imms.
	if (ch->level < CODER)
	{
		return;
	}

	char syntax[MAX_STRING_LENGTH];
	syntax[0] = '\0';
	strcat(syntax, "\r\n{YProvide an argument to set or toggle a setting.{x\r\n\r\n");
	strcat(syntax, "Syntax: settings <wizlock|newlock|doublegold|doubleexperience|\r\n");
	strcat(syntax, "                  gainconvert|shockspread|testmode|logincolorprompt\n\r");
	strcat(syntax, "                  webpageurl|mudname|logingreeting|statsurge|hoursexp>\r\n");
	strcat(syntax, "                  loginwholist|improves|stormkeepowner|stormkeeptarget\r\n");
	strcat(syntax, "                  dblogging|save|load\r\n");

	// Get the arguments we need
	argument = one_argument(argument, arg1);
	argument = first_arg(argument, arg2, FALSE);

	if (IS_NULLSTR(arg1))
	{
		send_to_char(syntax, ch);
		return;
	}

	if (!str_prefix(arg1, "wizlock"))
	{
		do_wizlock(ch, "");
	}
	else if (!str_prefix(arg1, "save"))
	{
		save_settings();
		send_to_char("The settings have been saved.\r\n", ch);
		wiznet("$N has saved the settings.", ch, NULL, 0, 0, 0);
	}
	else if (!str_prefix(arg1, "load"))
	{
		load_settings();
		send_to_char("You have loaded the settings from the last saved file.\r\n", ch);
		wiznet("$N has loaded the settings from the last saved version.", ch, NULL, 0, 0, 0);
		return;
	}
	else if (!str_prefix(arg1, "newlock"))
	{
		do_newlock(ch, "");
	}
	else if (!str_prefix(arg1, "doubleexperience"))
	{
		settings.double_exp = !settings.double_exp;

		if (settings.double_exp)
		{
			wiznet("$N has enabled double experience.", ch, NULL, 0, 0, 0);
			send_to_char("Double experience enabled.\r\n", ch);
		}
		else
		{
			wiznet("$N has disabled double experience.", ch, NULL, 0, 0, 0);
			send_to_char("Double experience disabled.\r\n", ch);
		}

		// Save the settings out to file.
		save_settings();

	}
	else if (!str_prefix(arg1, "doublegold"))
	{
		settings.double_gold = !settings.double_gold;

		if (settings.double_gold)
		{
			wiznet("$N has enabled double gold.", ch, NULL, 0, 0, 0);
			send_to_char("Double gold enabled.\r\n", ch);
		}
		else
		{
			wiznet("$N has disabled double gold.", ch, NULL, 0, 0, 0);
			send_to_char("Double gold disabled.\r\n", ch);
		}

		// Save the settings out to file.
		save_settings();

	}
	else if (!str_prefix(arg1, "gainconvert"))
	{
		settings.gain_convert = !settings.gain_convert;

		if (settings.gain_convert)
		{
			wiznet("$N has enabled the gain convert feature.", ch, NULL, 0, 0, 0);
			send_to_char("The gain convert feature has been enabled.\r\n", ch);
		}
		else
		{
			wiznet("$N has disabled the gain convert feature.", ch, NULL, 0, 0, 0);
			send_to_char("The gain convert feature has been disabled.\r\n", ch);
		}

		// Save the settings out to file.
		save_settings();

	}
	else if (!str_prefix(arg1, "shockspread"))
	{
		settings.shock_spread = !settings.shock_spread;

		if (settings.shock_spread)
		{
			wiznet("$N has enabled the shock spread mechanic.", ch, NULL, 0, 0, 0);
			send_to_char("The shock spread mechanic has been enabled.\r\n", ch);
		}
		else
		{
			wiznet("$N has disabled the shock spread mechanic.", ch, NULL, 0, 0, 0);
			send_to_char("The shock spread mechanic has been disabled.\r\n", ch);
		}

		// Save the settings out to file.
		save_settings();

	}
	else if (!str_prefix(arg1, "dblogging"))
	{
		settings.db_logging = !settings.db_logging;

		if (settings.db_logging)
		{
			wiznet("$N has enabled the database logging feature.", ch, NULL, 0, 0, 0);
			send_to_char("Database logging has been enabled.\r\n", ch);
		}
		else
		{
			wiznet("$N has disabled database logging.", ch, NULL, 0, 0, 0);
			send_to_char("Database logging has been disabled.\r\n", ch);
		}

		// Save the settings out to file.
		save_settings();

	}
	else if (!str_prefix(arg1, "testmode"))
	{
		settings.test_mode = !settings.test_mode;

		if (settings.test_mode)
		{
			wiznet("$N has turned on game wide test mode.", ch, NULL, 0, 0, 0);
			send_to_char("Game wide test mode has been turned on.\r\n", ch);
		}
		else
		{
			wiznet("$N has turned off game wide test mode.", ch, NULL, 0, 0, 0);
			send_to_char("Game wide test mode has been turned off.\r\n", ch);
		}

		// Save the settings out to file.
		save_settings();

	}
	else if (!str_prefix(arg1, "loginwholist"))
	{
		settings.login_who_list_enabled = !settings.login_who_list_enabled;

		sendf(ch, "The login who list enabled flag has been set to: %s.\r\n", settings.login_who_list_enabled ? "ON" : "OFF");
		sprintf(buf, "$N has set the login who list enabled flag to: %s", settings.login_who_list_enabled ? "ON" : "OFF");
		wiznet(buf, ch, NULL, 0, 0, 0);

		// Save the settings out to file.
		save_settings();
	}
	else if (!str_prefix(arg1, "whitelistlock"))
	{
		settings.whitelist_lock = !settings.whitelist_lock;

		if (settings.whitelist_lock)
		{
			wiznet("$N has turned on the white list lock.", ch, NULL, 0, 0, 0);
			send_to_char("White list lock has been turned on.\r\n", ch);
		}
		else
		{
			wiznet("$N has turned off the white list lock.", ch, NULL, 0, 0, 0);
			send_to_char("White list lock has been turned off.\r\n", ch);
		}

		// Save the settings out to file.
		save_settings();

	}
	else if (!str_prefix(arg1, "logincolorprompt"))
	{
		settings.login_color_prompt = !settings.login_color_prompt;

		sprintf(buf, "$N has set the login color prompt to %s.", bool_onoff(settings.login_color_prompt));
		wiznet(buf, ch, NULL, 0, 0, 0);

		sendf(ch, "Login color prompt has been turned %s.\r\n", bool_onoff(settings.login_color_prompt));

		save_settings();
	}
	else if (!str_prefix(arg1, "hoursexp"))
	{
		settings.hours_affect_exp = !settings.hours_affect_exp;

		sprintf(buf, "$N has set the hours affect experience setting to %s.", bool_onoff(settings.hours_affect_exp));
		wiznet(buf, ch, NULL, 0, 0, 0);

		sendf(ch, "Hours affecting experience has been turned %s.\r\n", bool_onoff(settings.hours_affect_exp));

		save_settings();
	}
	else if (!str_prefix(arg1, "improves"))
	{
		settings.focused_improves = !settings.focused_improves;

		sprintf(buf, "$N has set the focused improves setting to %s.", bool_onoff(settings.focused_improves));
		wiznet(buf, ch, NULL, 0, 0, 0);

		sendf(ch, "Focused improves has been turned %s.\r\n", bool_onoff(settings.focused_improves));

		save_settings();
	}
	else if (!str_prefix(arg1, "webpageurl"))
	{
		if (!IS_NULLSTR(arg2))
		{
			free_string(settings.web_page_url);
			settings.web_page_url = str_dup(arg2);
		}
		else
		{
			send_to_char("Please enter the web page's url as the last argument.\r\n", ch);
			return;
		}

		sprintf(buf, "$N has set the web page URL to %s.", settings.web_page_url);
		wiznet(buf, ch, NULL, 0, 0, 0);

		sendf(ch, "Web page URL has been changed %s.\r\n", settings.web_page_url);

		save_settings();
	}
	else if (!str_prefix(arg1, "statsurge"))
	{
		if (!IS_NULLSTR(arg2))
		{
			settings.stat_surge = atoi(arg2);
			sendf(ch, "Stats can now be surged %d above their maximum.\r\n", settings.stat_surge);

			save_settings();
		}
		else
		{
			send_to_char("Please enter a numeric value for the stat surge.\r\n", ch);
			return;
		}
	}
	else if (!str_prefix(arg1, "mudname"))
	{
		if (!IS_NULLSTR(arg2))
		{
			free_string(settings.mud_name);
			settings.mud_name = str_dup(arg2);
		}
		else
		{
			send_to_char("Please enter the mud's name.\r\n", ch);
			return;
		}

		sprintf(buf, "$N has set the mud name to %s.", settings.mud_name);
		wiznet(buf, ch, NULL, 0, 0, 0);

		sendf(ch, "The mud's name has been set to %s.\r\n", settings.mud_name);

		save_settings();
	}
	else if (!str_prefix(arg1, "logingreeting"))
	{
		if (!IS_NULLSTR(arg2))
		{
			free_string(settings.login_greeting);
			settings.login_greeting = str_dup(arg2);
		}
		else
		{
			send_to_char("Please enter the login greeting.\r\n", ch);
			return;
		}

		sprintf(buf, "$N has set the login greeting to %s.", settings.login_greeting);
		wiznet(buf, ch, NULL, 0, 0, 0);

		sendf(ch, "The login greeting has been set to %s.\r\n", settings.login_greeting);

		save_settings();
	}
	else if (!str_prefix(arg1, "stormkeepowner"))
	{
		int storm_keep_owner;

		if (!IS_NULLSTR(arg2) && (storm_keep_owner = clan_lookup(arg2)) > 0)
		{
			settings.storm_keep_owner = storm_keep_owner;
		}
		else
		{
			send_to_char("Please enter the clan to set the storm keep owner to.\r\n", ch);
			return;
		}

		sprintf(buf, "$N has set the storm keep owner to %s.", clan_table[storm_keep_owner].friendly_name);
		wiznet(buf, ch, NULL, 0, 0, 0);

		sendf(ch, "The storm keep owner has been set to %s.\r\n", clan_table[storm_keep_owner].friendly_name);

		save_settings();
	}
	else if (!str_prefix(arg1, "stormkeeptarget"))
	{
		int storm_keep_target;

		if (!IS_NULLSTR(arg2) && (storm_keep_target = clan_lookup(arg2)) > 0)
		{
			settings.storm_keep_target = storm_keep_target;
		}
		else
		{
			send_to_char("Please enter the clan to set the storm keep target onto.\r\n", ch);
			return;
		}

		sprintf(buf, "$N has set the storm keep target to %s.", clan_table[storm_keep_target].friendly_name);
		wiznet(buf, ch, NULL, 0, 0, 0);

		sendf(ch, "The storm keep target has been set to %s.\r\n", clan_table[storm_keep_target].friendly_name);

		save_settings();
	}
	else
	{
		send_to_char(syntax, ch);
	}

} // end do_settings

 /*
  * Loads the game settings from the settings.ini file.  This is leveraging the INI parsing
  * utilities contained in "ini.c".  Any values not found will have default values set.  The
  * INI parser will read in 0, 1, True, False, Yes, No and convert it accordingly.  It will be
  * then re-saved out with a standard 'True' or 'False' for readability.
  */
void load_settings()
{
	bool save = FALSE;
	dictionary* ini;

	ini = iniparser_load(SETTINGS_FILE);

	// If it's null, log it, but continue on so the default are loaded, then save.
	if (ini == NULL)
	{
		log_f("WARNING: Settings file '%s' was not found or is inaccessible.", SETTINGS_FILE);
		save = TRUE;
	}

	settings.wizlock = iniparser_getboolean(ini, "Settings:WizLock", FALSE);
	settings.newlock = iniparser_getboolean(ini, "Settings:NewLock", FALSE);
	settings.double_exp = iniparser_getboolean(ini, "Settings:DoubleExp", FALSE);
	settings.double_gold = iniparser_getboolean(ini, "Settings:DoubleGold", FALSE);
	settings.shock_spread = iniparser_getboolean(ini, "Settings:ShockSpread", FALSE);
	settings.gain_convert = iniparser_getboolean(ini, "Settings:GainConvert", FALSE);
	settings.test_mode = iniparser_getboolean(ini, "Settings:TestMode", FALSE);
	settings.whitelist_lock = iniparser_getboolean(ini, "Settings:WhiteListLock", FALSE);
	settings.login_color_prompt = iniparser_getboolean(ini, "Settings:LoginColorPrompt", FALSE);
	settings.login_who_list_enabled = iniparser_getboolean(ini, "Settings:LoginWhoListEnabled", FALSE);
	settings.hours_affect_exp = iniparser_getboolean(ini, "Settings:HoursAffectExperience", TRUE);
	settings.focused_improves = iniparser_getboolean(ini, "Settings:FocusedImproves", TRUE);
	settings.db_logging = iniparser_getboolean(ini, "Settings:DbLogging", FALSE);

	settings.stat_surge = iniparser_getint(ini, "Settings:StatSurge", 0);
	settings.storm_keep_owner = clan_lookup(iniparser_getstring(ini, "Settings:StormKeepOwner", ""));
	settings.storm_keep_target = clan_lookup(iniparser_getstring(ini, "Settings:StormKeepTarget", ""));

	free_string(settings.web_page_url);
	settings.web_page_url = str_dup(iniparser_getstring(ini, "Settings:WebPageUrl", ""));

	free_string(settings.mud_name);
	settings.mud_name = str_dup(iniparser_getstring(ini, "Settings:MudName", "Multi-User-Dimension"));

	free_string(settings.login_greeting);
	settings.login_greeting = str_dup(iniparser_getstring(ini, "Settings:LoginGreeting", ""));

	free_string(settings.login_menu_light_color);
	settings.login_menu_light_color = str_dup(iniparser_getstring(ini, "Settings:LoginMenuLightColor", "{C"));

	free_string(settings.login_menu_dark_color);
	settings.login_menu_dark_color = str_dup(iniparser_getstring(ini, "Settings:LoginMenuDarkColor", "{c"));

	free_string(settings.pager_color);
	settings.pager_color = str_dup(iniparser_getstring(ini, "Settings:PagerColor", "{C"));

	iniparser_freedict(ini);

	if (save)
	{
		save_settings();
	}

	global.last_boot_result = SUCCESS;
	return;

} // end load_settings

/*
 * Saves all of the game settings to an INI file.
 */
void save_settings(void)
{
	FILE* fp;

	fclose(fpReserve);
	fp = fopen(SETTINGS_FILE, "w");

	if (!fp)
	{
		bug("Could not open SETTINGS_FILE for writing.", 0);
		return;
	}

	// Section
	fprintf(fp, "[Settings]\n");

	// Game Locks and Bonuses
	fprintf(fp, "WizLock = %s\n", settings.wizlock ? "True" : "False");
	fprintf(fp, "NewLock = %s\n", settings.newlock ? "True" : "False");
	fprintf(fp, "WhiteListLock = %s\n", settings.whitelist_lock ? "True" : "False");
	fprintf(fp, "DoubleExp = %s\n", settings.double_exp ? "True" : "False");
	fprintf(fp, "DoubleGold = %s\n", settings.double_gold ? "True" : "False");
	fprintf(fp, "TestMode = %s\n", settings.test_mode ? "True" : "False");

	// Game Mechanics Settings
	fprintf(fp, "ShockSpread = %s\n", settings.shock_spread ? "True" : "False");
	fprintf(fp, "GainConvert = %s\n", settings.gain_convert ? "True" : "False");
	fprintf(fp, "StatSurge = %d\n", settings.stat_surge);
	fprintf(fp, "HoursAffectExperience = %s\n", settings.hours_affect_exp ? "True" : "False");
	fprintf(fp, "FocusedImproves = %s\n", settings.focused_improves ? "True" : "False");
	fprintf(fp, "DbLogging = %s\n", settings.db_logging ? "True" : "False");

	// System Settings
	fprintf(fp, "LoginColorPrompt = %s\n", settings.login_color_prompt ? "True" : "False");

	// Info
	fprintf(fp, "WebPageUrl = %s\n", IS_NULLSTR(settings.web_page_url) ? "" : settings.web_page_url);
	fprintf(fp, "LoginGreeting = %s\n", IS_NULLSTR(settings.login_greeting) ? "" : settings.login_greeting);
	fprintf(fp, "LoginMenuLightColor = %s\n", IS_NULLSTR(settings.login_menu_light_color) ? "{C" : settings.login_menu_light_color);
	fprintf(fp, "LoginMenuDarkColor = %s\n", IS_NULLSTR(settings.login_menu_dark_color) ? "{c" : settings.login_menu_dark_color);
	fprintf(fp, "LoginWhoListEnabled = %s\n", settings.login_who_list_enabled ? "True" : "False");
	fprintf(fp, "MudName = %s\n", IS_NULLSTR(settings.mud_name) ? "" : settings.mud_name);
	fprintf(fp, "PagerColor = %s\n", IS_NULLSTR(settings.pager_color) ? "{C" : settings.pager_color);

	// Keep Ownership Info
	fprintf(fp, "StormKeepOwner = %s\n", clan_table[settings.storm_keep_owner].name);
	fprintf(fp, "StormKeepTarget = %s\n", clan_table[settings.storm_keep_target].name);

	fclose(fp);
	fpReserve = fopen(NULL_FILE, "r");

} // end save_settings

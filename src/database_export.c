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

/***************************************************************************
 *                                                                         *
 * Although we are still using flat files to store all of the game data,   *
 * we will use this module to export that data into a sqlite database      *
 * which can be easily queried and used in other locations such as a web   *
 * site to hopefully provide better tools to mine the game data for        *
 * players and game admins. Some of the data maybe redudant to to allow    *
 * for less joins when querying (e.g. the object row will include the area *
 * name as well as the area_vnum.  I don't expect that this will be an     *
 * issue.                                                                  *
 *                                                                         *
 ***************************************************************************/

// System Specific Includes
#if defined(_WIN32)
#include <sys/types.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#endif

#include "sqlite/sqlite3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merc.h"
#include "interp.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"

void export_continents(void);
void export_areas(void);
void export_objects(void);
void export_item_type(void);
void export_clans(void);
void export_bits(void);
void export_help(void);
void export_rooms(void);
void export_classes(void);
void export_stats(void);
void export_directions(void);
void export_liquids(void);
void export_mobiles(void);
void export_deity(void);
void export_pc_race(void);
void export_note_type(void);
void export_flags(char *table_name, const struct flag_type *flags);

char *flag_string(const struct flag_type *flag_table, int bits);

extern NOTE_DATA *note_list;
extern NOTE_DATA *penalty_list;
extern NOTE_DATA *news_list;
extern NOTE_DATA *changes_list;
extern NOTE_DATA *ooc_list;
extern NOTE_DATA *story_list;
extern NOTE_DATA *history_list;
extern NOTE_DATA *immnote_list;

/*
 * Command to initiate the exporting of all game data into a sqlite database that can then be used
 * elsewhere such as a web site.
 */
void export_game_data(CHAR_DATA * ch, char *argument)
{
    int x = 0;

    writef(ch->desc, "%-55sStatus\r\n", "Action");
    writef(ch->desc, HEADER);

    writef(ch->desc, "%-55s", "Exporting Bits");
    export_bits();
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    writef(ch->desc, "%-55s", "Exporting Continents");
    export_continents();
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    writef(ch->desc, "%-55s", "Exporting Item Types");
    export_item_type();
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    writef(ch->desc, "%-55s", "Exporting Areas");
    export_areas();
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    writef(ch->desc, "%-55s", "Exporting Objects");
    export_objects();
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    writef(ch->desc, "%-55s", "Exporting Clans");
    export_clans();
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    writef(ch->desc, "%-55s", "Exporting Rooms");
    export_rooms();
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    writef(ch->desc, "%-55s", "Exporting Mobiles");
    export_mobiles();
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    writef(ch->desc, "%-55s", "Exporting Help Files");
    export_help();
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    writef(ch->desc, "%-55s", "Exporting Classes");
    export_classes();
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    writef(ch->desc, "%-55s", "Exporting Stat Lookup");
    export_stats();
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    writef(ch->desc, "%-55s", "Exporting Directions");
    export_directions();
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    writef(ch->desc, "%-55s", "Exporting Liquids");
    export_liquids();
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    writef(ch->desc, "%-55s", "Exporting Deities");
    export_deity();
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    writef(ch->desc, "%-55s", "Exporting PC Races");
    export_pc_race();
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    writef(ch->desc, "%-55s", "Exporting Note Types");
    export_note_type();
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    // These are all of the tables that are flag_types (makes it easy to export generically).
    for (x = 0; export_flags_table[x].table_name != NULL; x++)
    {
        writef(ch->desc, "Exporting %-45s", export_flags_table[x].friendly_name);
        export_flags(export_flags_table[x].table_name, export_flags_table[x].flags);
        writef(ch->desc, "[ {GComplete{x ]\r\n");
    }

    writef(ch->desc, "%-55s", "Executing Vacuum");
    execute_non_query("VACUUM");
    writef(ch->desc, "[ {GComplete{x ]\r\n");

    writef(ch->desc, "\r\nExport of game data complete!\r\n");
}

/*
 * All of the tables to export that are from the "flag_type" struct type.
 */
const struct export_flags_type export_flags_table[] = {
    { "apply_flags", "Apply Flags", apply_flags },
    { "apply_types", "Apply Types", apply_types },
    { "extra_flags", "Extra Flags", extra_flags },
    { "room_flags", "Room Flags", room_flags },
    { "sector_flags", "Sector Flags", sector_flags },
    { "weapon_flags", "Weapon Flags", weapon_type2 },
    { "wear_flags", "Wear Flags", wear_flags },
    { "act_flags", "Act Flags", act_flags },
    { "plr_flags", "Player Flags", plr_flags },
    { "affect_flags", "Affect Flags", affect_flags},
    { "off_flags", "Offensive Flags", affect_flags},
    { "immune_flags", "Immunity Flags", affect_flags},
    { "res_flags", "Resistance Flags", res_flags },
    { "vuln_flags", "Vulneribility Flags", vuln_flags },
    { "form_flags", "Form Flags", form_flags},
    { "part_flags", "Part Flags", part_flags },
    { "comm_flags", "Communication Flags", comm_flags},
    { "mprog_flags", "Mob Prog Flags", mprog_flags},
    { "area_flags", "Area Flags", area_flags},
    { "sex_flags", "Gender Flags", sex_flags},
    { "exit_flags", "Exit Flags", exit_flags},
    { "door_resets", "Door Resets", door_resets},
    { "type_flags", "Item Type Flags", type_flags},
    { "wear_loc_strings", "Wear Location Strings", wear_loc_strings},
    { "wear_loc_flags", "Wear Location Flags", wear_loc_flags},
    { "container_flags", "Container Flags", container_flags},
    { "ac_type", "Armor Class Flags", ac_type},
    { "size_flags", "Size Flags", size_flags },
    { "weapon_class", "Weapon Class", weapon_class },
    { "position_flags", "Position Flags", position_flags },
    { "portal_flags", "Portal Floags", portal_flags },
    { "furniture_flags", "Furniture Flags", furniture_flags },
    { NULL, NULL, NULL }
};

/*
* Export a generic flags table (the name of the table and a pointer to the stucture is
* required.  This should be generic in that it can export anything that's a flag_type.  The
* flags table must have a final entry with a NULL or this will ya know, doing something not
* great.
*/
void export_flags(char *table_name, const struct flag_type *flags)
{
    int rc;
    sqlite3_stmt *stmt;
    int x;
    char sql[MSL];

    if (IS_NULLSTR(table_name) || flags == NULL)
    {
        bugf("Null table_name or flags_type in export_flags_table");
        return;
    }

    // Begin a transaction
    execute_non_query("BEGIN TRANSACTION");

    // Clear the table, we're going to reload it.
    execute_non_query("DELETE FROM %s;", table_name);

    // Prepare the insert statement that we'll re-use in the loop (one prepared, we'll have to finalize the stmt later)
    sprintf(sql, "INSERT INTO %s (id, name) VALUES (?1, ?2);", table_name);

    if (sqlite3_prepare(global.db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        bugf("export_flags_table(%s) -> Failed to prepare insert statement", table_name);
        goto out;
    }

    // Loop over all extra flags
    for (x = 0; flags[x].name != NULL; x++)
    {
        sqlite3_bind_int(stmt, 1, flags[x].bit);
        sqlite3_bind_text(stmt, 2, flags[x].name, -1, SQLITE_STATIC);

        rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE)
        {
            bugf("export_flags(%s) ERROR inserting data for %s: %s\n", table_name, flags[x].name, sqlite3_errmsg(global.db));
        }

        sqlite3_reset(stmt);
    }

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_flags_table(%s) -> Failed to commit transaction.", table_name);
    }

out:
    sqlite3_finalize(stmt);
    return;
}

void export_objects(void)
{
    int rc;
    sqlite3_stmt *stmt;
    sqlite3_stmt *stmt_affect;
    int vnum = 0;
    int nMatch = 0;
    OBJ_INDEX_DATA *pObjIndex;
    AFFECT_DATA *paf;

    // Begin a transaction
    sqlite3_exec(global.db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    // Clear the tables, we're going to reload it.
    execute_non_query("DELETE FROM object;");
    execute_non_query("DELETE FROM object_affect;");

    // Prepare the insert statement that we'll re-use in the loop
    if (sqlite3_prepare(global.db, "INSERT INTO object (vnum, name, short_description, long_description, material, item_type, extra_flags, wear_flags, level, condition, weight, cost, value1, value2, value3, value4, value5, area_name, area_vnum) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19);", -1, &stmt, NULL) != SQLITE_OK)
    {
        bugf("export_object -> Failed to prepare insert statement for object.");
        sqlite3_finalize(stmt);
        return;
    }

    if (sqlite3_prepare(global.db, "INSERT INTO object_affect (vnum, apply_id, name, modifier) VALUES (?1, ?2, ?3, ?4);", -1, &stmt_affect, NULL) != SQLITE_OK)
    {
        bugf("export_object -> Failed to prepare insert statement object_affect.");
        sqlite3_finalize(stmt);
        sqlite3_finalize(stmt_affect);
        return;
    }

    // Loop over all object index data's
    for (vnum = 0; nMatch < top_obj_index; vnum++)
    {
        if ((pObjIndex = get_obj_index(vnum)) != NULL)
        {
            nMatch++;

            // The basic object index data
            sqlite3_bind_int(stmt, 1, pObjIndex->vnum);
            sqlite3_bind_text(stmt, 2, pObjIndex->name, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, pObjIndex->short_descr, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, pObjIndex->description, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 5, pObjIndex->material, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 6, pObjIndex->item_type);
            sqlite3_bind_int(stmt, 7, pObjIndex->extra_flags);
            sqlite3_bind_int(stmt, 8, pObjIndex->wear_flags);
            sqlite3_bind_int(stmt, 9, pObjIndex->level);
            sqlite3_bind_int(stmt, 10, pObjIndex->condition);
            sqlite3_bind_int(stmt, 11, pObjIndex->weight);
            sqlite3_bind_int(stmt, 12, pObjIndex->cost);
            sqlite3_bind_int(stmt, 13, pObjIndex->value[0]);
            sqlite3_bind_int(stmt, 14, pObjIndex->value[1]);
            sqlite3_bind_int(stmt, 15, pObjIndex->value[2]);
            sqlite3_bind_int(stmt, 16, pObjIndex->value[3]);
            sqlite3_bind_int(stmt, 17, pObjIndex->value[4]);
            sqlite3_bind_text(stmt, 18, pObjIndex->area->name, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 19, pObjIndex->area->vnum);

            rc = sqlite3_step(stmt);

            if (rc != SQLITE_DONE)
            {
                bugf("ERROR inserting data for %d: %s\n", pObjIndex->vnum, sqlite3_errmsg(global.db));
            }

            sqlite3_reset(stmt);

            // Object affects for this item
            for (paf = pObjIndex->affected; paf; paf = paf->next)
            {
                sqlite3_bind_int(stmt_affect, 1, pObjIndex->vnum);
                sqlite3_bind_int(stmt_affect, 2, paf->location);
                sqlite3_bind_text(stmt_affect, 3, flag_string(apply_flags, paf->location), -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt_affect, 4, paf->modifier);

                rc = sqlite3_step(stmt_affect);

                if (rc != SQLITE_DONE)
                {
                    bugf("ERROR inserting data for %d: %s\n", pObjIndex->vnum, sqlite3_errmsg(global.db));
                }

                sqlite3_reset(stmt_affect);
            }

        }
    }

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_objects -> Failed to commit transaction.");
    }

    // Cleanup
    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_affect);

    return;
}

void export_mobiles(void)
{
    sqlite3_stmt *stmt;
    int rc;

    // Begin a transaction
    sqlite3_exec(global.db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    // Clear the tables, we're going to reload it.
    execute_non_query("DELETE FROM mobile;");

    // Prepare the insert statement that we'll re-use in the loop
    if (sqlite3_prepare(global.db, "INSERT INTO mobile (vnum, name, short_description, long_description, act, affected_by, alignment, level, hitroll, hit1, hit2, hit3, mana1, mana2, mana3, dam1, dam2, dam3, ac1, ac2, ac3, ac4, dam_type, off_flags, imm_flags, res_flags, vuln_flags, start_position, default_position, sex, race, wealth, form, parts, material, area_name, area_vnum) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16, ?17, ?18, ?19, ?20, ?21, ?22, ?23, ?24, ?25, ?26, ?27, ?28, ?29, ?30, ?31, ?32, ?33, ?34, ?35, ?36, ?37);", -1, &stmt, NULL) != SQLITE_OK)
    {
        bugf("export_mobiles -> Failed to prepare insert statement for mobile.");
        sqlite3_finalize(stmt);
        return;
    }

    // Loop over all of the mob index data's and insert them into the database.
    extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
    MOB_INDEX_DATA *pMobIndex;
    int hash;

    for (hash = 0; hash < MAX_KEY_HASH; hash++)
    {
        for (pMobIndex = mob_index_hash[hash]; pMobIndex != NULL; pMobIndex = pMobIndex->next)
        {
            sqlite3_bind_int(stmt, 1, pMobIndex->vnum);
            sqlite3_bind_text(stmt, 2, pMobIndex->player_name, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, pMobIndex->short_descr, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, pMobIndex->long_descr, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 5, pMobIndex->act);
            sqlite3_bind_int(stmt, 6, pMobIndex->affected_by);
            sqlite3_bind_int(stmt, 7, pMobIndex->alignment);
            sqlite3_bind_int(stmt, 8, pMobIndex->level);
            sqlite3_bind_int(stmt, 9, pMobIndex->hitroll);
            sqlite3_bind_int(stmt, 10, pMobIndex->hit[0]);
            sqlite3_bind_int(stmt, 11, pMobIndex->hit[1]);
            sqlite3_bind_int(stmt, 12, pMobIndex->hit[2]);
            sqlite3_bind_int(stmt, 13, pMobIndex->mana[0]);
            sqlite3_bind_int(stmt, 14, pMobIndex->mana[1]);
            sqlite3_bind_int(stmt, 15, pMobIndex->mana[2]);
            sqlite3_bind_int(stmt, 16, pMobIndex->damage[0]);
            sqlite3_bind_int(stmt, 17, pMobIndex->damage[1]);
            sqlite3_bind_int(stmt, 18, pMobIndex->damage[2]);
            sqlite3_bind_int(stmt, 19, pMobIndex->ac[0]);
            sqlite3_bind_int(stmt, 20, pMobIndex->ac[1]);
            sqlite3_bind_int(stmt, 21, pMobIndex->ac[2]);
            sqlite3_bind_int(stmt, 22, pMobIndex->ac[2]);
            sqlite3_bind_int(stmt, 23, pMobIndex->dam_type);
            sqlite3_bind_int(stmt, 24, pMobIndex->off_flags);
            sqlite3_bind_int(stmt, 25, pMobIndex->imm_flags);
            sqlite3_bind_int(stmt, 26, pMobIndex->res_flags);
            sqlite3_bind_int(stmt, 27, pMobIndex->vuln_flags);
            sqlite3_bind_int(stmt, 28, pMobIndex->start_pos);
            sqlite3_bind_int(stmt, 29, pMobIndex->default_pos);
            sqlite3_bind_int(stmt, 30, pMobIndex->sex);
            sqlite3_bind_int(stmt, 31, pMobIndex->race);
            sqlite3_bind_int(stmt, 32, pMobIndex->wealth);
            sqlite3_bind_int(stmt, 33, pMobIndex->form);
            sqlite3_bind_int(stmt, 34, pMobIndex->parts);
            sqlite3_bind_text(stmt, 35, pMobIndex->material, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 36, pMobIndex->area->name, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 37, pMobIndex->area->vnum);

            rc = sqlite3_step(stmt);

            if (rc != SQLITE_DONE)
            {
                bugf("ERROR inserting data for %d: %s\n", pMobIndex->vnum, sqlite3_errmsg(global.db));
            }

            sqlite3_reset(stmt);
        }
    }

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_mobiles -> Failed to commit transaction");
    }

    sqlite3_finalize(stmt);

    return;
}

void export_continents(void)
{
    int rc;
    sqlite3_stmt *stmt;
    int continent;

    // Begin a transaction
    sqlite3_exec(global.db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    // Clear the table
    execute_non_query("DELETE FROM continent;");

    // Prepare the insert statement that we'll re-use in the loop
    if (sqlite3_prepare(global.db, "INSERT INTO continent (id, name) VALUES (?1, ?2);", -1, &stmt, NULL) != SQLITE_OK)
    {
        bugf("export_continents -> Failed to prepare insert");
        sqlite3_finalize(stmt);
        return;
    }

    // Loop over all continents
    for (continent = 0; continent_table[continent].name != NULL; continent++)
    {
        sqlite3_bind_int(stmt, 1, continent_table[continent].type);
        sqlite3_bind_text(stmt, 2, continent_table[continent].name, -1, SQLITE_STATIC);

        rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE)
        {
            bugf("ERROR inserting data for %d: %s\n", continent_table[continent].type, sqlite3_errmsg(global.db));
        }

        sqlite3_reset(stmt);
    }

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_continents -> Failed to commit transaction");
    }

    sqlite3_finalize(stmt);

    return;
}

void export_item_type(void)
{
    int rc;
    sqlite3_stmt *stmt;
    int x;

    // Begin a transaction
    sqlite3_exec(global.db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    execute_non_query("DELETE FROM item_type");

    // Prepare the insert statement that we'll re-use in the loop
    if (sqlite3_prepare(global.db, "INSERT INTO item_type (id, name) VALUES (?1, ?2);", -1, &stmt, NULL) != SQLITE_OK)
    {
        bugf("export_item_type -> Failed to prepare insert statement");
        sqlite3_finalize(stmt);
        return;
    }

    // Loop over all continents
    for (x = 0; item_table[x].name != NULL; x++)
    {
        sqlite3_bind_int(stmt, 1, item_table[x].type);
        sqlite3_bind_text(stmt, 2, item_table[x].name, -1, SQLITE_STATIC);

        rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE)
        {
            bugf("ERROR inserting data for %d: %s\n", item_table[x].type, sqlite3_errmsg(global.db));
        }

        sqlite3_reset(stmt);
    }

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_item_type -> Failed to commit transaction.");
    }

    sqlite3_finalize(stmt);

    return;
}

void export_areas(void)
{
    int rc;
    sqlite3_stmt *stmt;
    AREA_DATA *pArea;

    // Begin a transaction
    sqlite3_exec(global.db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    // Clear the table
    execute_non_query("DELETE FROM area;");

    // Prepare the insert statement that we'll re-use in the loop
    if (sqlite3_prepare(global.db, "INSERT INTO area (vnum, name, min_level, max_level, builders, continent, area_flags) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7);", -1, &stmt, NULL) != SQLITE_OK)
    {
        bugf("export_areas -> Failed to prepare insert statement");
        sqlite3_finalize(stmt);
        return;
    }

    // Loop over all areas and save the area data for each entry.
    for (pArea = area_first; pArea; pArea = pArea->next)
    {
        sqlite3_bind_int(stmt, 1, pArea->vnum);
        sqlite3_bind_text(stmt, 2, pArea->name, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 3, pArea->min_level);
        sqlite3_bind_int(stmt, 4, pArea->max_level);
        sqlite3_bind_text(stmt, 5, pArea->builders, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 6, pArea->continent);
        sqlite3_bind_int(stmt, 7, pArea->area_flags);

        rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE)
        {
            bugf("ERROR inserting data for %s: %s\n", pArea->name, sqlite3_errmsg(global.db));
        }

        sqlite3_reset(stmt);
    }

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_areas -> Failed to commit transaction.");
    }

    sqlite3_finalize(stmt);

    return;
}

void export_clans(void)
{
    int rc;
    sqlite3_stmt *stmt;
    int x;

    // Begin a transaction
    sqlite3_exec(global.db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    // Clear table
    execute_non_query("DELETE FROM clan;");

    // Prepare the insert statement that we'll re-use in the loop
    if (sqlite3_prepare(global.db, "INSERT INTO clan (id, name, who_name, friendly_name, hall_vnum, independent, regen_room_directions) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7);", -1, &stmt, NULL) != SQLITE_OK)
    {
        bugf("export_clan -> Failed to prepare insert statement.");
        sqlite3_finalize(stmt);
        return;
    }

    // Loop over all continents
    for (x = 0; x < MAX_CLAN; x++)
    {
        sqlite3_bind_int(stmt, 1, x);
        sqlite3_bind_text(stmt, 2, clan_table[x].name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, clan_table[x].who_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, clan_table[x].friendly_name, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 5, clan_table[x].hall);
        sqlite3_bind_int(stmt, 6, clan_table[x].independent);
        sqlite3_bind_text(stmt, 7, clan_table[x].regen_room_directions, -1, SQLITE_STATIC);

        rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE)
        {
            bugf("ERROR inserting data for %d: %s\n", clan_table[x].name, sqlite3_errmsg(global.db));
        }

        sqlite3_reset(stmt);
    }

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_clan -> Failed to commit transaction.");
    }

    sqlite3_finalize(stmt);

    return;
}

void export_bits(void)
{
    // Begin a transaction
    execute_non_query("BEGIN TRANSACTION");

    // Clear the table
    execute_non_query("DELETE FROM bit;");

    execute_non_query("INSERT INTO bit(id, name) VALUES (1, 'A')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (2, 'B')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (4, 'C')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (8, 'D')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (16, 'E')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (32, 'F')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (64, 'G')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (128, 'H')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (256, 'I')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (512, 'J')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (1024, 'K')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (2048, 'L')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (4096, 'M')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (8192, 'N')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (16384, 'O')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (32768, 'P')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (65536, 'Q')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (131072, 'R')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (262144, 'S')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (524288, 'T')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (1048576, 'U')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (2097152, 'V')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (4194304, 'W')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (8388608, 'X')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (16777216, 'Y')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (33554432, 'Z')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (67108864, 'aa')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (134217728, 'bb')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (268435456, 'cc')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (536870912, 'dd')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (1073741824, 'ee')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (2147483648, 'ff')");
    execute_non_query("INSERT INTO bit(id, name) VALUES (4294967296, 'gg')");

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_bits -> Failed to commit transaction.");
    }

    return;
}

void export_help(void)
{
    int rc;
    sqlite3_stmt *stmt;
    HELP_DATA *pHelp;

    // Begin a transaction
    sqlite3_exec(global.db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    // Clear all data
    execute_non_query("DELETE FROM help;");

    // Prepare the insert statement that we'll re-use in the loop
    if (sqlite3_prepare(global.db, "INSERT INTO help (level, keyword, help_text) VALUES (?1, ?2, ?3);", -1, &stmt, NULL) != SQLITE_OK)
    {
        bugf("export_help -> Failed to prepare insert statement");
        sqlite3_finalize(stmt);
        return;
    }

    // Loop over all areas and save the area data for each entry.
    for (pHelp = help_first; pHelp != NULL; pHelp = pHelp->next)
    {
        sqlite3_bind_int(stmt, 1, pHelp->level);
        sqlite3_bind_text(stmt, 2, pHelp->keyword, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, pHelp->text, -1, SQLITE_STATIC);

        rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE)
        {
            bugf("ERROR inserting data for %s: %s\n", pHelp->keyword, sqlite3_errmsg(global.db));
        }

        sqlite3_reset(stmt);
    }

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_help -> Failed to commit transaction.");
    }

    sqlite3_finalize(stmt);

    return;
}

/*
 * TODO - Extra descriptions
 */
void export_rooms(void)
{
    int rc;
    sqlite3_stmt *stmt;
    sqlite3_stmt *stmt_exit;
    sqlite3_stmt *stmt_objects;
    sqlite3_stmt *stmt_mobs;
    EXIT_DATA *pexit;
    ROOM_INDEX_DATA *room;
    RESET_DATA *pReset;
    int x = 0;
    int door = 0;

    // Begin a transaction
    sqlite3_exec(global.db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    // Clear the old data
    execute_non_query("DELETE FROM room;");
    execute_non_query("DELETE FROM room_exits;");
    execute_non_query("DELETE FROM room_objects;");
    execute_non_query("DELETE FROM room_mobs;");

    // Prepare the insert statement that we'll re-use in the loop
    if (sqlite3_prepare(global.db, "INSERT INTO room(vnum, room_name, room_description, owner, room_flags, light, sector_type, heal_rate, mana_rate, clan, area_vnum, area_name) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12);", -1, &stmt, NULL) != SQLITE_OK)
    {
        bugf("export_rooms -> Failed to prepare insert statement");
        sqlite3_finalize(stmt);
        return;
    }

    if (sqlite3_prepare(global.db, "INSERT INTO room_exits(vnum, to_vnum, dir, dir_name, keyword, description, orig_door, rs_flags, exits_area) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9);", -1, &stmt_exit, NULL) != SQLITE_OK)
    {
        bugf("export_rooms -> Failed to prepare insert statement for room_exits");
        sqlite3_finalize(stmt);
        sqlite3_finalize(stmt_exit);
        return;
    }

    if (sqlite3_prepare(global.db, "INSERT INTO room_objects(room_vnum, object_vnum) VALUES (?1, ?2);", -1, &stmt_objects, NULL) != SQLITE_OK)
    {
        bugf("export_rooms -> Failed to prepare insert statement for room_objects");
        sqlite3_finalize(stmt);
        sqlite3_finalize(stmt_exit);
        sqlite3_finalize(stmt_objects);
        return;
    }

    if (sqlite3_prepare(global.db, "INSERT INTO room_mobs(room_vnum, mob_vnum) VALUES (?1, ?2);", -1, &stmt_mobs, NULL) != SQLITE_OK)
    {
        bugf("export_rooms -> Failed to prepare insert statement for room_mobs");
        sqlite3_finalize(stmt);
        sqlite3_finalize(stmt_exit);
        sqlite3_finalize(stmt_objects);
        sqlite3_finalize(stmt_mobs);
        return;
    }

    for (x = 0; x < MAX_KEY_HASH; x++) /* room index hash table */
    {
        for (room = room_index_hash[x]; room != NULL; room = room->next)
        {
            // First the room data
            sqlite3_bind_int(stmt, 1, room->vnum);
            sqlite3_bind_text(stmt, 2, room->name, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, room->description, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, room->owner, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 5, room->room_flags);
            sqlite3_bind_int(stmt, 6, room->light);
            sqlite3_bind_int(stmt, 7, room->sector_type);
            sqlite3_bind_int(stmt, 8, room->heal_rate);
            sqlite3_bind_int(stmt, 9, room->mana_rate);
            sqlite3_bind_int(stmt, 10, room->clan);
            sqlite3_bind_int(stmt, 11, room->area->vnum);
            sqlite3_bind_text(stmt, 12, room->area->name, -1, SQLITE_STATIC);

            rc = sqlite3_step(stmt);

            if (rc != SQLITE_DONE)
            {
                bugf("ERROR inserting data for %s (%d): %s\n", room->name, room->vnum, sqlite3_errmsg(global.db));
            }

            sqlite3_reset(stmt);

            // Now, insert the exits
            for (door = 0; door < MAX_DIR; door++)
            {
                if ((pexit = room->exit[door]) != NULL
                    && pexit->u1.to_room != NULL)
                {
                    sqlite3_bind_int(stmt_exit, 1, room->vnum);
                    sqlite3_bind_int(stmt_exit, 2, pexit->u1.to_room->vnum);
                    sqlite3_bind_int(stmt_exit, 3, door);
                    sqlite3_bind_text(stmt_exit, 4, dir_name[door], -1, SQLITE_STATIC);
                    sqlite3_bind_text(stmt_exit, 5, pexit->keyword, -1, SQLITE_STATIC);
                    sqlite3_bind_text(stmt_exit, 6, pexit->description, -1, SQLITE_STATIC);
                    sqlite3_bind_int(stmt_exit, 7, pexit->orig_door);
                    sqlite3_bind_int(stmt_exit, 8, pexit->rs_flags);
                    sqlite3_bind_int(stmt_exit, 9, (room->area->vnum != pexit->u1.to_room->area->vnum) ? TRUE : FALSE);

                    rc = sqlite3_step(stmt_exit);

                    if (rc != SQLITE_DONE)
                    {
                        bugf("ERROR inserting exit data for room %s (vnum=%d, exit=%d): %s\n", room->name, room->vnum, door, sqlite3_errmsg(global.db));
                    }

                    sqlite3_reset(stmt_exit);
                }
            }

            // Resets for what objects and mobs end up in this room in some capacity.  Due to the way objects are reset onto mobs, we
            // can't easily get what mob it's on (when resets are loaded it puts the object onto the last mob loaded in that room before
            // the reset... we don't have that here.  We *could* in the future do a search for the item on all mobs in the world but it
            // would require a pristine copy of the world (e.g. you're using real time data then.. a player could have taken the object or
            // it might not have reloaded, etc.).  For now we'll omit that.
            for (pReset = room->reset_first; pReset != NULL; pReset = pReset->next)
            {
                switch (pReset->command)
                {
                    case 'M':
                        sqlite3_bind_int(stmt_mobs, 1, room->vnum);
                        sqlite3_bind_int(stmt_mobs, 2, pReset->arg1);

                        rc = sqlite3_step(stmt_mobs);

                        if (rc != SQLITE_DONE)
                        {
                            bugf("ERROR inserting data for reset in %s (%d, object %d): %s\n", room->name, room->vnum, pReset->arg1, sqlite3_errmsg(global.db));
                        }

                        sqlite3_reset(stmt_mobs);

                        break;
                    case 'O':   // Object in the room
                    case 'P':   // Object in another object
                    case 'G':   // Object carried by a mob
                    case 'E':   // Object equiped onto a mob
                        sqlite3_bind_int(stmt_objects, 1, room->vnum);
                        sqlite3_bind_int(stmt_objects, 2, pReset->arg1);
                        sqlite3_bind_int(stmt_objects, 3, 0);

                        rc = sqlite3_step(stmt_objects);

                        if (rc != SQLITE_DONE)
                        {
                            bugf("ERROR inserting data for reset in %s (%d, object %d): %s\n", room->name, room->vnum, pReset->arg1, sqlite3_errmsg(global.db));
                        }

                        sqlite3_reset(stmt_objects);

                        break;
                }
            }

        }
    }

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_rooms -> Failed to commit transaction.");
    }

    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_exit);
    sqlite3_finalize(stmt_objects);
    sqlite3_finalize(stmt_mobs);

    return;
}

void export_classes(void)
{
    int rc;
    sqlite3_stmt *stmt;
    int x;

    // Begin a transaction
    sqlite3_exec(global.db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    // Clear data
    execute_non_query("DELETE FROM class;");

    // Prepare the insert statement that we'll re-use in the loop
    if (sqlite3_prepare(global.db, "INSERT INTO class(id, name, who_name, attr_prime, attr_second, weapon, skill_adept, thac0_00, thac0_32, hp_min, hp_max, mana, base_group, default_group, is_reclass, is_enabled) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, ?16);", -1, &stmt, NULL) != SQLITE_OK)
    {
        bugf("export_classes -> Failed to prepare insert statement");
        sqlite3_finalize(stmt);
        return;
    }

    // Loop over all the classes
    for (x = 0; x < top_class; x++)
    {
        sqlite3_bind_int(stmt, 1, x);
        sqlite3_bind_text(stmt, 2, capitalize(class_table[x]->name), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, class_table[x]->who_name, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 4, class_table[x]->attr_prime);
        sqlite3_bind_int(stmt, 5, class_table[x]->attr_second);
        sqlite3_bind_int(stmt, 6, class_table[x]->weapon);
        sqlite3_bind_int(stmt, 7, class_table[x]->skill_adept);
        sqlite3_bind_int(stmt, 8, class_table[x]->thac0_00);
        sqlite3_bind_int(stmt, 9, class_table[x]->thac0_32);
        sqlite3_bind_int(stmt, 10, class_table[x]->hp_min);
        sqlite3_bind_int(stmt, 11, class_table[x]->hp_max);
        sqlite3_bind_int(stmt, 12, class_table[x]->mana);
        sqlite3_bind_text(stmt, 13, class_table[x]->base_group, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 14, class_table[x]->default_group, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 15, class_table[x]->is_reclass);
        sqlite3_bind_int(stmt, 16, class_table[x]->is_enabled);

        rc = sqlite3_step(stmt);

        if (rc != SQLITE_DONE)
        {
            bugf("ERROR inserting data for %d: %s\n", capitalize(class_table[x]->name), sqlite3_errmsg(global.db));
        }

        sqlite3_reset(stmt);
    }

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_classes -> Failed to commit transaction.");
    }

    sqlite3_finalize(stmt);

    return;
}

void export_stats(void)
{
    // Begin a transaction
    sqlite3_exec(global.db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    // Clear table
    execute_non_query("DELETE FROM stat;");

    execute_non_query("INSERT INTO stat(id, name, short_name) VALUES(0, 'Strength', 'Str');");
    execute_non_query("INSERT INTO stat(id, name, short_name) VALUES(1, 'Intelligence', 'Int');");
    execute_non_query("INSERT INTO stat(id, name, short_name) VALUES(2, 'Wisdom', 'Wis');");
    execute_non_query("INSERT INTO stat(id, name, short_name) VALUES(3, 'Dexterity', 'Dex');");
    execute_non_query("INSERT INTO stat(id, name, short_name) VALUES(4, 'Constitution', 'Con');");

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_stats -> Failed to commit transaction.");
    }

    return;
}

void export_directions(void)
{
    // Begin a transaction
    sqlite3_exec(global.db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    // Clear the data
    execute_non_query("DELETE FROM directions;");

    execute_non_query("INSERT INTO directions(dir_id, name, reverse_dir_id, reverse_name) VALUES (0, 'north', 2, 'south');");
    execute_non_query("INSERT INTO directions(dir_id, name, reverse_dir_id, reverse_name) VALUES (1, 'east', 3, 'west');");
    execute_non_query("INSERT INTO directions(dir_id, name, reverse_dir_id, reverse_name) VALUES (2, 'south', 0, 'north');");
    execute_non_query("INSERT INTO directions(dir_id, name, reverse_dir_id, reverse_name) VALUES (3, 'west', 1, 'east');");
    execute_non_query("INSERT INTO directions(dir_id, name, reverse_dir_id, reverse_name) VALUES (4, 'up', 5, 'down');");
    execute_non_query("INSERT INTO directions(dir_id, name, reverse_dir_id, reverse_name) VALUES (5, 'down', 4, 'up');");
    execute_non_query("INSERT INTO directions(dir_id, name, reverse_dir_id, reverse_name) VALUES (6, 'northwest', 9, 'southeast');");
    execute_non_query("INSERT INTO directions(dir_id, name, reverse_dir_id, reverse_name) VALUES (7, 'northeast', 8, 'southwest');");
    execute_non_query("INSERT INTO directions(dir_id, name, reverse_dir_id, reverse_name) VALUES (8, 'southwest', 7, 'northeast');");
    execute_non_query("INSERT INTO directions(dir_id, name, reverse_dir_id, reverse_name) VALUES (9, 'southeast', 6, 'northwest');");

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_directions -> Failed to commit transaction.");
    }

    return;
}

void export_liquids(void)
{
    int x;

    execute_non_query("BEGIN TRANSACTION");
    execute_non_query("DELETE FROM liquids");

    // Loop over all liquids
    for (x = 0; liquid_table[x].liq_name != NULL; x++)
    {
        execute_non_query("INSERT INTO liquids(id, name, color, proof, full, thirst, food, size) VALUES (%d, '%q', '%q', %d, %d, %d, %d, %d)",
            x, liquid_table[x].liq_name, liquid_table[x].liq_color, liquid_table[x].liq_affect[0], liquid_table[x].liq_affect[1],
            liquid_table[x].liq_affect[2], liquid_table[x].liq_affect[3], liquid_table[x].liq_affect[4]);
    }

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_liquids() -> Failed to commit transaction.");
    }

}

void export_deity(void)
{
    int x;
    char align[128];

    execute_non_query("BEGIN TRANSACTION");
    execute_non_query("DELETE FROM deity");

    // Loop over all liquids
    for (x = 0; deity_table[x].name != NULL; x++)
    {
        if (deity_table[x].align == ALIGN_GOOD)
        {
            sprintf(align, "Good");
        }
        else if (deity_table[x].align == ALIGN_NEUTRAL)
        {
            sprintf(align, "Neutral");
        }
        else if (deity_table[x].align == ALIGN_EVIL)
        {
            sprintf(align, "Evil");
        }
        else if (deity_table[x].align == ALIGN_ALL)
        {
            sprintf(align, "All");
        }

        execute_non_query("INSERT INTO deity(id, name, description, align, enabled) VALUES (%d, '%q', '%q', '%q', %d)",
            x, deity_table[x].name, deity_table[x].description, align, deity_table[x].enabled);
    }

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_deity() -> Failed to commit transaction.");
    }

}

/*
 * TODO - class multiplier's in it's own table, additional racial skills in it's own table.
 */
void export_pc_race(void)
{
    int x;

    execute_non_query("BEGIN TRANSACTION");
    execute_non_query("DELETE FROM pc_race");

    // Loop over all liquids
    for (x = 1; x < MAX_PC_RACE; x++)
    {
        execute_non_query("INSERT INTO pc_race(id, name, who_name, article_name, points, size, strength, intelligence, wisdom, dexterity, constitution, player_selectable) VALUES (%d, '%q', '%q', '%q', %d, %d, %d, %d, %d, %d, %d, %d)",
            x, pc_race_table[x].name, pc_race_table[x].who_name, pc_race_table[x].article_name, pc_race_table[x].points, pc_race_table[x].size, 
            pc_race_table[x].max_stats[STAT_STR], pc_race_table[x].max_stats[STAT_INT], pc_race_table[x].max_stats[STAT_WIS], pc_race_table[x].max_stats[STAT_DEX], pc_race_table[x].max_stats[STAT_CON],
            pc_race_table[x].player_selectable);
    }

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_pc_race() -> Failed to commit transaction.");
    }

}

void export_note_type(void)
{
    // Begin a transaction
    sqlite3_exec(global.db, "BEGIN TRANSACTION", NULL, NULL, NULL);

    // Clear the data
    execute_non_query("DELETE FROM note_type;");

    execute_non_query("INSERT INTO note_type(id, name, description) VALUES (0, 'Note', 'In Character Note');");
    execute_non_query("INSERT INTO note_type(id, name, description) VALUES (1, 'News', 'News Note');");
    execute_non_query("INSERT INTO note_type(id, name, description) VALUES (2, 'Change', 'Game Change Note');");
    execute_non_query("INSERT INTO note_type(id, name, description) VALUES (3, 'Penalty', 'Penalty Note');");
    execute_non_query("INSERT INTO note_type(id, name, description) VALUES (4, 'OOC', 'Out of Character Note');");
    execute_non_query("INSERT INTO note_type(id, name, description) VALUES (5, 'Story', 'Story Note');");
    execute_non_query("INSERT INTO note_type(id, name, description) VALUES (6, 'History', 'History Note');");
    execute_non_query("INSERT INTO note_type(id, name, description) VALUES (7, 'Immortal', 'Immortal Note');");

    if (!execute_non_query("COMMIT TRANSACTION"))
    {
        bugf("export_note_type -> Failed to commit transaction.");
    }

    return;
}

/*
 * Logs a player kill along with information about these players at this time.
 */
void log_toast(CHAR_DATA *ch, CHAR_DATA *victim, char *message, char *verb, bool arena)
{
    // Check if logging is disabled to the sqlite database.
    if (!settings.db_logging)
    {
        return;
    }

    char verb_clean[256];
    char message_clean[256];

    sprintf(verb_clean, "%s", strip_color(verb));
    sprintf(message_clean, "%s", strip_color(message));

    buffer_non_query("INSERT INTO toasts(message, winner, winner_clan, winner_level, loser, loser_clan, loser_level, verb, arena, room_vnum, toast_time) VALUES ('%q', '%q', '%q', %d, '%q', '%q', %d, '%q', %d, %d, strftime('%%Y-%%m-%%d %%H:%%M:%%S', DATETIME('now', 'localtime')));",
        message_clean, ch->name, clan_table[ch->clan].friendly_name, ch->level, victim->name, clan_table[victim->clan].friendly_name, victim->level, verb_clean, arena, ch->in_room->vnum);

}

/*
 * Logs a players info into the database for use elsewhere like a web-site.  This should be called when the pfile
 * saves (or when a player logs out for less writes).
 */
void log_player(CHAR_DATA *ch)
{
    if (!settings.db_logging || ch == NULL || ch->desc == NULL || IS_NPC(ch))
    {
        return;
    }

    buffer_non_query("\
        REPLACE INTO player(name, last_ip, version, description, prompt, points, security, recall_vnum, quest_points,\
            pkills, pkilled, pkills_arena, bank_gold, priest_rank, deity, merit, clan_flags, clan_rank,\
            clan, sex, class, race, level, trust, played, last_save, hit, max_hit, perm_hit,\
            mana, max_mana, perm_mana, move, max_move, perm_move, gold, silver, exp, practice, train,\
            carry_weight, carry_number, saving_throw, hitroll, damroll, alignment, ac_pierce, ac_slash, ac_bash, ac_exotic,\
            wimpy, stance, strength, intelligence, wisdom, dexterity, constitution, password, title, email)\
        VALUES('%q', '%q', %d, '%q', '%q', %d, %d, %d, %d, %d, %d, %d, %d, '%q', '%q', %d, %d, '%q', '%q',\
            '%q', '%q', '%q', %d, %d, %d, strftime('%%Y-%%m-%%d %%H:%%M:%%S', DATETIME('now', 'localtime')), %d, %d,\
            %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,\
            %d, %d, %d, %d, %d, %d, '%q', '%q', '%q');",
        ch->name, ch->desc->host, ch->version, ch->description, ch->prompt, ch->pcdata->points, ch->pcdata->security, ch->pcdata->recall_vnum, ch->pcdata->quest_points,
        ch->pcdata->pkills, ch->pcdata->pkilled, ch->pcdata->pkills_arena, ch->pcdata->bank_gold, priest_rank_table[ch->pcdata->priest_rank].name, deity_table[ch->pcdata->deity].name, ch->pcdata->merit, ch->pcdata->clan_flags, clan_rank_table[ch->pcdata->clan_rank].name,
        clan_table[ch->clan].friendly_name, sex_table[ch->pcdata->true_sex].name, class_table[ch->class]->name, race_table[ch->race].name, ch->level, ch->trust, hours_played(ch), ch->hit, ch->max_hit, ch->pcdata->perm_hit,
        ch->mana, ch->max_mana, ch->pcdata->perm_mana, ch->move, ch->max_move, ch->pcdata->perm_move, ch->gold, ch->silver, ch->exp, ch->practice, ch->train,
        ch->carry_weight, ch->carry_number, ch->saving_throw, ch->hitroll, ch->damroll, ch->alignment, GET_AC(ch, AC_PIERCE), GET_AC(ch, AC_BASH), GET_AC(ch, AC_SLASH), GET_AC(ch, AC_EXOTIC),
        ch->wimpy, ch->stance, get_curr_stat(ch, STAT_STR), get_curr_stat(ch, STAT_INT), get_curr_stat(ch, STAT_WIS), get_curr_stat(ch, STAT_DEX), get_curr_stat(ch, STAT_CON), ch->pcdata->pwd, ch->pcdata->title, ch->pcdata->email);

}

/*
 * Loops through notes that are in the active note lists (some are filtered out when the notes are
 * loaded from the flat files.  This will replace a note if they already exist so there should not
 * be duplicates.  Notes should be logged when they are sent but say a crash happens, this could be
 * used to load ones that got missed because they were still in the buffer.
 */
int archive_active_notes(void)
{
    NOTE_DATA *pnote;
    int counter = 0;

    for (pnote = news_list; pnote != NULL; pnote = pnote->next)
    {
        log_note(pnote);
        counter++;
    }

    for (pnote = note_list; pnote != NULL; pnote = pnote->next)
    {
        log_note(pnote);
        counter++;
    }

    for (pnote = penalty_list; pnote != NULL; pnote = pnote->next)
    {
        log_note(pnote);
        counter++;
    }

    for (pnote = changes_list; pnote != NULL; pnote = pnote->next)
    {
        log_note(pnote);
        counter++;
    }

    for (pnote = ooc_list; pnote != NULL; pnote = pnote->next)
    {
        log_note(pnote);
        counter++;
    }

    for (pnote = story_list; pnote != NULL; pnote = pnote->next)
    {
        log_note(pnote);
        counter++;
    }

    for (pnote = history_list; pnote != NULL; pnote = pnote->next)
    {
        log_note(pnote);
        counter++;
    }

    for (pnote = immnote_list; pnote != NULL; pnote = pnote->next)
    {
        log_note(pnote);
        counter++;
    }

    return counter;
}

/*
 * Logs a note to the database.
 */
void log_note(NOTE_DATA *note)
{
    if (!settings.db_logging || note == NULL)
    {
        return;
    }

    // The time_t will overload an INTEGER in the database and cause memory issues so we have to store it as TEXT.
    char id[128];
    sprintf(id, "%ld", note->date_stamp);

    // These are a few convenience fields to find public and immortal notes.
    bool to_all = is_name("all", note->to_list);
    bool to_immortal = FALSE;

    if (is_name("immortal", note->to_list) || is_name("imm", note->to_list))
    {
        to_immortal = TRUE;
    }

    buffer_non_query("REPLACE INTO note(id, type, sender, str_date, to_list, subject, text, date_stamp, to_all, to_immortal) VALUES ('%q', %d, '%q', '%q', '%q', '%q', '%q', strftime('%%Y-%%m-%%d %%H:%%M:%%S', DATETIME('now', 'localtime')), %d, %d);",
        id, note->type, note->sender, note->date, note->to_list, note->subject, note->text, to_all, to_immortal);

    // Now, try to log the recipients so that we can easily query them individually.
    // strtok updates the original string in the pointer, so we need to make a copy of it and we will use that and then free it.
    char *tokenPtr;
    char *to_list_copy = strdup(note->to_list);

    // Initialize the string tokenizer and receive pointer to first token, split the string on spaces and commas.. extra commonly used characters
    // included which will get parsed out.  As an additional note, if ghost recipients (those that no longer exist) we can delete them from this
    // table on boot although I would suggest keeing the original unparsed to line.
    tokenPtr = strtok(to_list_copy, " ,();.");

    while (tokenPtr != NULL)
    {
        if (!IS_NULLSTR(tokenPtr))
        {
            // Capitalize the recipient to standarize it, this will also match the pfile structure
            buffer_non_query("REPLACE INTO note_recipient(id, name) VALUES ('%q', '%q');", id, capitalize(tokenPtr));
        }

        tokenPtr = strtok(NULL, " ,();.");
    }

    // Free the copy we created.
    free(to_list_copy);

}

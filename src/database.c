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
 * This file will contain all of the abstract/generic database methods     *
 * that other pieces of the game may call into.  This will generally be    *
 * utility methods and anything to create/maintain the database.           *
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

DB_BUFFER *db_buffer_list;
DB_BUFFER *db_buffer_last;

/*
 * Opens a connection to our SQLite game database
 */
void open_db()
{
    int rc;

    rc = sqlite3_open(EXPORT_DATABASE_FILE, &global.db);

    if (rc != SQLITE_OK)
    {
        bugf("open_db() -> Failed to open %s", EXPORT_DATABASE_FILE);
        global.last_boot_result = FAILURE;
        return;
    }

    // For ease of use and since we won't be calling open_db() all over the place we'll
    // call init_db() here which will ensure our tables are all created.
    init_db();

    global.last_boot_result = SUCCESS;
}

/*
* Closes the connection to our SQLite game database.
*/
void close_db()
{
    int rc = 0;

    // Flush the pending database operations to disk.
    flush_db();

    if (global.db != NULL)
    {
        rc = sqlite3_close(global.db);
    }

    if (rc != SQLITE_OK)
    {
        bugf("close_db() -> Failed to close %s", EXPORT_DATABASE_FILE);
        global.last_boot_result = FAILURE;
        return;
    }

    global.last_boot_result = SUCCESS;
}

/*
* This will create any tables that need to exist, doing this here will keep the clutter out of the
* load functions.
*/
void init_db()
{
    int x = 0;

    // These are all of the tables that are flag_types (makes it easy to export generically).
    for (x = 0; export_flags_table[x].table_name != NULL; x++)
    {
        execute_non_query("CREATE TABLE IF NOT EXISTS %s(id INTEGER PRIMARY KEY, name TEXT);", export_flags_table[x].table_name);
    }

    // Create the tables they do not exist
    execute_non_query("CREATE TABLE IF NOT EXISTS object(vnum INTEGER PRIMARY KEY, name TEXT, short_description TEXT, long_description TEXT, material TEXT, item_type INTEGER, extra_flags INTEGER, wear_flags INTEGER, level INTEGER, condition INTEGER, weight INTEGER, cost INTEGER, value1 INTEGER, value2 INTEGER, value3 INTEGER, value4 INTEGER, value5 INTEGER, area_name TEXT, area_vnum INTEGER);");
    execute_non_query("CREATE TABLE IF NOT EXISTS object_affect(vnum INTEGER, apply_id INTEGER, name TEXT, modifier INTEGER);");
    execute_non_query("CREATE TABLE IF NOT EXISTS continent(id INTEGER PRIMARY KEY, name TEXT);");
    execute_non_query("CREATE TABLE IF NOT EXISTS item_type(id INTEGER PRIMARY KEY, name TEXT);");
    execute_non_query("CREATE TABLE IF NOT EXISTS area(vnum INTEGER PRIMARY KEY, name TEXT, min_level INTEGER, max_level INTEGER, builders TEXT, continent INTEGER, area_flags INTEGER);");
    execute_non_query("CREATE TABLE IF NOT EXISTS clan(id INTEGER PRIMARY KEY, name TEXT, who_name TEXT, friendly_name TEXT, hall_vnum INTEGER, independent BOOLEAN, regen_room_directions TEXT);");
    execute_non_query("CREATE TABLE IF NOT EXISTS bit(id INTEGER PRIMARY KEY, name TEXT);");
    execute_non_query("CREATE TABLE IF NOT EXISTS help(id INTEGER PRIMARY KEY, level INTEGER, keyword TEXT, help_text TEXT);");
    execute_non_query("CREATE TABLE IF NOT EXISTS room(vnum INTEGER PRIMARY KEY, room_name TEXT, room_description TEXT, owner TEXT, room_flags INTEGER, light INTEGER, sector_type INTEGER, heal_rate INTEGER, mana_rate INTEGER, clan INTEGER, area_vnum INTEGER, area_name TEXT);");
    execute_non_query("CREATE TABLE IF NOT EXISTS room_exits(vnum INTEGER, to_vnum INTEGER, dir INTEGER, dir_name TEXT, keyword TEXT, description TEXT, orig_door INTEGER, rs_flags INTEGER, exits_area BOOLEAN);");
    execute_non_query("CREATE TABLE IF NOT EXISTS room_objects(room_vnum INTEGER, object_vnum INTEGER);");
    execute_non_query("CREATE TABLE IF NOT EXISTS room_mobs(room_vnum INTEGER, mob_vnum INTEGER);");
    execute_non_query("CREATE TABLE IF NOT EXISTS class(id INTEGER PRIMARY KEY, name TEXT, who_name TEXT, attr_prime INTEGER, attr_second INTEGER, weapon INTEGER, skill_adept INTEGER, thac0_00 INTEGER, thac0_32 INTEGER, hp_min INTEGER, hp_max INTEGER, mana BOOLEAN, base_group TEXT, default_group TEXT, is_reclass BOOLEAN, is_enabled BOOLEAN);");
    execute_non_query("CREATE TABLE IF NOT EXISTS stat(id INTEGER PRIMARY KEY, name TEXT, short_name TEXT);");
    execute_non_query("CREATE TABLE IF NOT EXISTS directions(dir_id INTEGER PRIMARY KEY, name TEXT, reverse_dir_id INTEGER, reverse_name TEXT);");
    execute_non_query("CREATE TABLE IF NOT EXISTS liquids(id INTEGER PRIMARY KEY, name TEXT, color TEXT, proof INTEGER, full INTEGER, thirst INTEGER, food INTEGER, size INTEGER);");
    execute_non_query("CREATE TABLE IF NOT EXISTS mobile(vnum INTEGER PRIMARY KEY, name TEXT, short_description TEXT, long_description TEXT, act INTEGER, affected_by INTEGER, alignment INTEGER, level INTEGER, hitroll INTEGER, hit1 INTEGER, hit2 INTEGER, hit3 INTEGER, mana1 INTEGER, mana2 INTEGER, mana3 INTEGER, dam1 INTEGER, dam2 INTEGER, dam3 INTEGER, ac1 INTEGER, ac2 INTEGER, ac3 INTEGER, ac4 INTEGER, dam_type INTEGER, off_flags INTEGER, imm_flags INTEGER, res_flags INTEGER, vuln_flags INTEGER, start_position INTEGER, default_position INTEGER, sex INTEGER, race INTEGER, wealth INTEGER, form INTEGER, parts INTEGER, material TEXT, area_name TEXT, area_vnum INTEGER); ");
    execute_non_query("CREATE TABLE IF NOT EXISTS deity(id INTEGER PRIMARY KEY, name TEXT, description TEXT, align TEXT, enabled INTEGER);");
    execute_non_query("CREATE TABLE IF NOT EXISTS pc_race(id INTEGER PRIMARY KEY, name TEXT, who_name TEXT, article_name TEXT, points INTEGER, size INTEGER, strength INTEGER, intelligence INTEGER, wisdom INTEGER, dexterity INTEGER, constitution INTEGER, player_selectable INTEGER)");
    execute_non_query("CREATE TABLE IF NOT EXISTS note_type(id INTEGER PRIMARY KEY, name TEXT, description TEXT);");
    execute_non_query("CREATE TABLE IF NOT EXISTS note(id TEXT PRIMARY KEY, type INTEGER, sender TEXT, str_date TEXT, to_list TEXT, subject TEXT, text TEXT, date_stamp DATETIME, to_all INTEGER, to_immortal INTEGER);");
    execute_non_query("CREATE TABLE IF NOT EXISTS note_recipient(id TEXT, name TEXT, PRIMARY KEY (id, name));");


    // These are tables that will track running game data and not be dropped.  The player table for instance does not replace the
    // pfile as the storage mechansim for a player at this point.
    execute_non_query("CREATE TABLE IF NOT EXISTS toasts(id INTEGER PRIMARY KEY AUTOINCREMENT, message TEXT(128), winner TEXT(24), winner_clan TEXT(24), winner_level INTEGER, loser TEXT(24), loser_clan TEXT(24), loser_level INTEGER, verb TEXT(16), arena INTEGER, room_vnum INTEGER, toast_time DATETIME);");
    execute_non_query("CREATE TABLE IF NOT EXISTS player(name TEXT PRIMARY KEY, last_ip TEXT, version INTEGER, description TEXT, prompt TEXT, points INTEGER, security INTEGER, recall_vnum INTEGER, quest_points INTEGER, pkills INTEGER, pkilled INTEGER, pkills_arena INTEGER, bank_gold INTEGER, priest_rank TEXT, deity TEXT, merit INTEGER, clan_flags INTEGER, clan_rank TEXT, clan TEXT, sex TEXT, class TEXT, race TEXT, level INTEGER, trust INTEGER, played INTEGER, last_save DATETIME, hit INTEGER, max_hit INTEGER, perm_hit INTEGER, mana INTEGER, max_mana INTEGER, perm_mana INTEGER, move INTEGER, max_move INTEGER, perm_move INTEGER, gold INTEGER, silver INTEGER, exp INTEGER, practice INTEGER, train INTEGER, carry_weight INTEGER, carry_number INTEGER, saving_throw INTEGER, hitroll INTEGER, damroll INTEGER, alignment INTEGER, ac_pierce INTEGER, ac_bash INTEGER, ac_slash INTEGER, ac_exotic INTEGER, wimpy INTEGER, stance INTEGER, strength INTEGER, intelligence INTEGER, wisdom INTEGER, dexterity INTEGER, constitution INTEGER, password TEXT, title TEXT, email TEXT);");

    // Finally, let's run vaccuum which will help with size but also to reorganize tables if we begin doing
    // lots of updates/deletes/inserts, etc.
    execute_non_query("VACUUM");
}

/*
* Executes a SQL statement that does not need a result, this will allow sprintf like
* place holders to be used.  This is IMMEDIATELY executed.
*/
bool execute_non_query(char *sql, ...)
{
    va_list args;
    va_start(args, sql);
    sqlite3_vsnprintf(MSL * 4, global.sql, sql, args);
    va_end(args);

    if (sqlite3_exec(global.db, global.sql, 0, 0, 0) != SQLITE_OK)
    {
        bugf("execute_non_query(%s) -> %s", global.buf, sqlite3_errmsg(global.db));
        bugf("SQL: %s", global.sql);
        return FALSE;
    }

    return TRUE;
}

/*
 * Executes a query verbatim.  Only safe queries should be passed here.
 */
bool execute_raw_query(char *sql)
{
    if (sqlite3_exec(global.db, sql, 0, 0, 0) != SQLITE_OK)
    {
        bugf("execute_raw_query -> %s", sqlite3_errmsg(global.db));
        bugf("SQL: %s", sql);
        return FALSE;
    }

    return TRUE;
}

/*
 * Adds a SQL statement to the buffer that will be executed in a bulk transaction at a
 * deferred time.  This query will be prepared by sqlite3_vsnprintf and thus should not
 * be re-processed by that again (it will corrupt the query if it contains special characters).
 * This will be run through the execute_raw_query which expects a safe query (which this will be).
 */
bool buffer_non_query(char *sql, ...)
{
    va_list args;
    va_start(args, sql);
    sqlite3_vsnprintf(MSL * 4, global.sql, sql, args);
    va_end(args);

    // Now that we've created on SQL, add it to the deferred buffer.
    DB_BUFFER * query;
    query = new_db_buffer();

    // Allocate the length of the strenght and one for the null terminator
    query->buffer = malloc(strlen(global.sql) + 1);
    sprintf(query->buffer, "%s", global.sql);

    return TRUE;
}

/*
* Returns the first value of the first column (expecting it to be a number)
*/
int execute_scalar_int(char *sql, ...)
{
    int rc;
    int return_value = 0;
    sqlite3_stmt *stmt;
    va_list args;
    va_start(args, sql);
    sqlite3_vsnprintf(MSL * 2, global.sql, sql, args);
    va_end(args);

    rc = sqlite3_prepare_v2(global.db, global.sql, -1, &stmt, 0);

    if (rc != SQLITE_OK)
    {
        bugf("execute_scalar_int(%s) -> %s", global.sql, sqlite3_errmsg(global.db));
        return -1;
    }

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        return_value = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return return_value;
}

/*
* Returns the first value of the first column (expecting it to be text).  TODO - Memory test this with valgrind (load test also)
*/
char *execute_scalar_text(char *sql, ...)
{
    static char buf[MSL];
    int rc;
    sqlite3_stmt *stmt;
    va_list args;
    va_start(args, sql);
    sqlite3_vsnprintf(MSL * 2, global.buf, sql, args);
    va_end(args);

    rc = sqlite3_prepare_v2(global.db, global.buf, -1, &stmt, 0);

    if (rc != SQLITE_OK)
    {
        bugf("execute_scalar_text(%s) -> %s", global.buf, sqlite3_errmsg(global.db));
        return "";
    }

    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW)
    {
        sprintf(buf, "%s", sqlite3_column_text(stmt, 0));
    }
    else
    {
        // This is necessary because it's a static char, if no value is found and this isn't
        // done then the last successful value would still be there.. so we have to clear it.
        buf[0] = '\0';
    }

    sqlite3_finalize(stmt);

    return buf;
}

/*
 * Create a new db deferred buffer.
 */
DB_BUFFER* new_db_buffer(void)
{
    DB_BUFFER* temp;

    // Allocate the memory needed
    temp = malloc(sizeof(*temp));

    if (!temp)
    {
        bugf("Unable to create, or allocate, a new timed buffer.");
        return NULL;
    }

    // Initialize the data
    temp->buffer = NULL;
    temp->timestamp = 0;
    temp->next = NULL;

    if (db_buffer_list == NULL)
    {
        db_buffer_list = temp;
        db_buffer_last = temp;
        db_buffer_list->next = NULL;
    }
    else
    {
        db_buffer_last->next = temp;
        db_buffer_last = temp;
        temp->next = NULL;
    }

    return temp;
}

/*
 *  Free's an allocated deferred buffer.
 */
void free_db_buffer(DB_BUFFER* buf)
{
    DB_BUFFER* temp;

    if (!buf)
    {
        return;
    }

    // The buffer itself may have and likely was allocated and if so it
    // needs to be free'd.
    if (buf->buffer)
    {
        free(buf->buffer);
        buf->buffer = NULL;
    }

    buf->timestamp = 0;

    /* Clean up the list. */
    if (buf == db_buffer_list)
    {
        temp = db_buffer_list->next;

        if (db_buffer_list->next)
        {
            db_buffer_list = temp;
        }
        else
        {
            db_buffer_list = NULL;
        }
    }
    else
    {
        for (temp = db_buffer_list; temp; temp = temp->next)
        {
            if (temp->next)
            {
                if (temp->next == buf)
                {
                    if (buf->next)
                    {
                        temp->next = buf->next; // Switch the list;
                    }
                    else
                    {
                        temp->next = NULL; // No next, end it.
                        break;
                    }
                }
            }
        }
    }

    buf->next = NULL;
    free(buf);
    return;
}

/*
 * Flush all of the existing deferred db queries and execute them in a transaction.
 */
int flush_db(void)
{
    DB_BUFFER* temp;
    DB_BUFFER* tnext;
    int counter = 0;

    if (!db_buffer_list)
    {
        return counter;
    }

    execute_non_query("BEGIN;");

    for (temp = db_buffer_list; temp; temp = tnext)
    {
        tnext = temp->next; // We'll free them up.
        global.db_operations++;
        counter++;

        // Execute the query (This buffer should already be safe)
        execute_raw_query(temp->buffer);

        // Free it up
        free_db_buffer(temp);
    }

    execute_non_query("COMMIT;");

    return counter;
}

/*
 * Return the number of deferred queries in the db buffer.
 */
int db_buffer_count(void)
{
    DB_BUFFER* temp;
    int counter = 0;

    if (!db_buffer_list)
    {
        return 0;
    }

    for (temp = db_buffer_list; temp != NULL; temp = temp->next)
    {
        counter++;
    }

    return counter;
}

/*
 * Command that will allow for interaction with the database.
 */
void do_database(CHAR_DATA *ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    int x = 0;

    if (IS_NULLSTR(argument))
    {
        send_to_char("Syntax: db <info|update-game-data|flush|archive-notes>\r\n", ch);
        return;
    }

    argument = one_argument(argument, arg);

    if (!str_cmp(arg, "update-game-data"))
    {
        export_game_data(ch, argument);
        return;
    }
    else if (!str_cmp(arg, "info"))
    {
        sendf(ch, "SQLite Version:        %s\r\n", SQLITE_VERSION);
        sendf(ch, "DB Logging:            %s\r\n", settings.db_logging ? "{GON{x" : "{ROFF{x");
        sendf(ch, "Operations this boot:  %d\r\n", global.db_operations);
        sendf(ch, "Pending Operations:    %d\r\n", db_buffer_count());
        return;
    }
    else if (!str_cmp(arg, "flush"))
    {
        start_timer();
        x = flush_db();
        double elapsed = end_timer();

        if (x == 0)
        {
            send_to_char("There were no database operations to flush.\r\n", ch);
        }
        else
        {
            sendf(ch, "%d database operation(s) flushed in %f seconds.\r\n", x, elapsed);
        }

        return;
    }
    else if (!str_cmp(arg, "archive-notes"))
    {
        // This will force go through and archive notes (it should replace previous versions
        // if they exist so there will be no dupliates.
        int count = archive_active_notes();
        sendf(ch, "%d notes have been queued to add to the database.\r\n", count);
    }
    else
    {
        send_to_char("That db command was not recognized.\r\n\r\n", ch);
        send_to_char("Syntax: db <info|update-game-data|flush|archive-notes>\r\n", ch);
        return;
    }

}

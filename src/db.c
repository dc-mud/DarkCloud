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
    #include <sys/resource.h>
    #include <time.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "merc.h"
#include "db.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "olc.h"

#if defined (_WIN32)
    #define random() rand()
    #define srandom( x ) srand( x )
#endif

#if !defined(linux)
    #if !defined(QMFIXES)
        long random();
    #endif
#endif

#if !defined(QMFIXES)
    void srandom(unsigned int);
#endif

int getpid();
time_t time(time_t * tloc);

/* externals for counting purposes */
extern OBJ_DATA *obj_free;
extern CHAR_DATA *char_free;
extern DESCRIPTOR_DATA *descriptor_free;
extern PC_DATA *pcdata_free;
extern AFFECT_DATA *affect_free;

/*
 * Globals.
 */
HELP_DATA *help_first;
HELP_DATA *help_last;
HELP_AREA *had_list;

SHOP_DATA *shop_first;
SHOP_DATA *shop_last;

MPROG_CODE *mprog_list;

CHAR_DATA *char_list;
char *help_greeting;
OBJ_DATA *object_list;
TIME_INFO_DATA time_info;
WEATHER_DATA weather_info;
SETTINGS_DATA settings;
STATISTICS_DATA statistics;
GLOBAL_DATA global;

extern const struct continent_type continent_table[];

/*
 * Locals.
 */
MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
OBJ_INDEX_DATA *obj_index_hash[MAX_KEY_HASH];
ROOM_INDEX_DATA *room_index_hash[MAX_KEY_HASH];
char *string_hash[MAX_KEY_HASH];

AREA_DATA *area_first;
AREA_DATA *area_last;
AREA_DATA *current_area;

char *string_space;
char *top_string;
char str_empty[1];

int top_affect;
int top_area;
int top_ed;
int top_exit;
int top_help;
int top_mob_index;
int top_obj_index;
int top_reset;
int top_room;
int top_shop;
int top_vnum_room;         /* OLC */
int top_vnum_mob;          /* OLC */
int top_vnum_obj;          /* OLC */
int top_mprog_index;       /* OLC */
int top_sn;
int top_group;
int top_class;
int mobile_count = 0;

/*
 * Memory management.
 * Increase MAX_STRING in case the mud reaches its limits, specifically when
 * adding new areas.  Bumped from the stock value of 1413120 to 2000000.
 * Tune the others only if you understand what you're doing.
 */
#define            MAX_STRING    2000000
#define            MAX_PERM_BLOCK    131072
#define            MAX_MEM_LIST    13

void * rgFreeList[MAX_MEM_LIST];
const size_t rgSizeList[MAX_MEM_LIST] =
{
    8, 16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768,  65536 - 64
};

int    nAllocString;
size_t sAllocString;
int    nAllocPerm;
size_t sAllocPerm;

/*
 * Semi-locals.
 */


// Whether the game is in the boot module. It will be true in the beginning and false in the end.
bool fBootDb;
FILE *fpArea;
char strArea[MAX_INPUT_LENGTH];

/*
 * Local booting procedures.
*/
void load_area(FILE * fp);    /* OLC */
void load_helps(FILE * fp, char *fname);
void load_mobiles(FILE * fp);
void load_objects(FILE * fp);
void load_resets(FILE * fp);
void load_rooms(FILE * fp);
void load_shops(FILE * fp);
void load_socials(FILE * fp);
void load_specials(FILE * fp);
void load_bans(void);
void load_mobprogs(FILE * fp);
void load_notes(void);
void fix_exits(void);
void fix_mobprogs(void);
void reset_area(AREA_DATA * pArea);
void load_classes(void);
void load_groups(void);
void load_settings(void);
void load_skills(void);
void load_game_objects(void);
void load_statistics(void);
void init_weather(void);
void assign_gsn(void);

SPELL_FUN  *spell_function_lookup(char *name);

/* For saving of PC corpses and donation pits across copyover and reboots */
void fwrite_obj(CHAR_DATA * ch, OBJ_DATA * obj, FILE * fp, int iNest);
void fread_obj (CHAR_DATA * ch, FILE * fp);
int  area_count(int continent, int level, bool recommend);

/*
 * Big mama top level function.
 */
void boot_db()
{

#if defined(_WIN32)
    // Windows memory leak debugging tools
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
#endif

    /*
     * Init some data space stuff.
     */
    {
        if ((string_space = calloc(1, MAX_STRING)) == NULL)
        {
            bug("Boot_db: can't alloc %d string space.", MAX_STRING);
            exit(1);
        }

        top_string = string_space;
        fBootDb = TRUE;
    }

    log_string("STATUS: Initializing Random Number Generator");
    init_random();

    log_string("STATUS: Initializing Weather");
    init_weather();

    log_string("STATUS: Loading Settings");
    if (global.copyover) copyover_broadcast("STATUS: Loading Settings.", FALSE);
    load_settings();

    log_string("STATUS: Loading Disabled Commands");
    if (global.copyover) copyover_broadcast("STATUS: Loading Disabled Commands.", TRUE);
    load_disabled();

    log_string("STATUS: Loading Skills");
    if (global.copyover) copyover_broadcast("STATUS: Loading Skills.", TRUE);
    load_skills();
    log_f("STATUS: %d Skills Loaded", top_sn);

    log_string("STATUS: Loading Groups");
    if (global.copyover) copyover_broadcast("STATUS: Loading Groups.", TRUE);
    load_groups();

    log_string("STATUS: Loading Classes");
    if (global.copyover) copyover_broadcast("STATUS: Loading Classes.", TRUE);
    load_classes();

    log_string("STATUS: Assigning GSNs (Global Skill Numbers)");
    if (global.copyover) copyover_broadcast("STATUS: Assigning GSNs (Global Skill Numbers).", TRUE);
    assign_gsn();

    if (global.copyover) copyover_broadcast("STATUS: Initializing Random Name Generator.", TRUE);
    init_name_parts();

    /*
     * Read in all the area files.
     */
    {
        log_string("STATUS: Loading Areas");
        if (global.copyover) copyover_broadcast("STATUS: Loading Areas.", TRUE);

        FILE *fpList;

        if ((fpList = fopen(AREA_LIST, "r")) == NULL)
        {
            global.last_boot_result = FAILURE;
            if (global.copyover) copyover_broadcast("STATUS: Shutting Down", TRUE);
            perror(AREA_LIST);
            exit(1);
        }

        for (;;)
        {
            strcpy(strArea, fread_word(fpList));
            if (strArea[0] == '$')
                break;

            if (strArea[0] == '-')
            {
                fpArea = stdin;
            }
            else
            {
                if ((fpArea = fopen(strArea, "r")) == NULL)
                {
                    global.last_boot_result = FAILURE;
                    if (global.copyover) copyover_broadcast("STATUS: Shutting Down", TRUE);
                    perror(strArea);
                    exit(1);
                }
            }

            current_area = NULL;

            for (;;)
            {
                char *word;

                if (fread_letter(fpArea) != '#')
                {
                    bug("Boot_db: # not found.", 0);
                    global.last_boot_result = FAILURE;
                    if (global.copyover) copyover_broadcast("STATUS: Shutting Down", TRUE);
                    exit(1);
                }

                word = fread_word(fpArea);

                if (word[0] == '$')
                    break;
                else if (!str_cmp(word, "AREADATA"))
                    load_area(fpArea);
                else if (!str_cmp(word, "HELPS"))
                    load_helps(fpArea, strArea);
                else if (!str_cmp(word, "MOBILES"))
                    load_mobiles(fpArea);
                else if (!str_cmp(word, "MOBPROGS"))
                    load_mobprogs(fpArea);
                else if (!str_cmp(word, "OBJECTS"))
                    load_objects(fpArea);
                else if (!str_cmp(word, "RESETS"))
                    load_resets(fpArea);
                else if (!str_cmp(word, "ROOMS"))
                    load_rooms(fpArea);
                else if (!str_cmp(word, "SHOPS"))
                    load_shops(fpArea);
                else if (!str_cmp(word, "SOCIALS"))
                    load_socials(fpArea);
                else if (!str_cmp(word, "SPECIALS"))
                    load_specials(fpArea);
                else
                {
                    global.last_boot_result = FAILURE;
                    bug("Boot_db: bad section name.", 0);
                    if (global.copyover) copyover_broadcast("STATUS: Shutting Down", TRUE);
                    exit(1);
                }
            }

            if (fpArea != stdin)
                fclose(fpArea);
            fpArea = NULL;
        }
        fclose(fpList);

        if (global.last_boot_result == UNKNOWN)
            global.last_boot_result = SUCCESS;

    }

    /*
     * Fix up exits.
     * Declare db booting over.
     * Reset all areas once.
     * Load up notes and ban files.
     */
    {
        log_string("STATUS: Fixing Exits");
        if (global.copyover) copyover_broadcast("STATUS: Fixing Exits.", TRUE);
        fix_exits();

        log_string("STATUS: Fixing MobProgs");
        if (global.copyover) copyover_broadcast("STATUS: Fixing MobProgs.", TRUE);
        fix_mobprogs();
        fBootDb = FALSE;

        // The loading of saved objects needs to happen before the
        // resetting otherwise we'll have duplicate objects
        log_string("STATUS: Loading Saved Objects (Pits/Corpses/Buried Items)");
        if (global.copyover) copyover_broadcast("STATUS: Loading Saved Objects.", TRUE);
        load_game_objects();

        log_string("STATUS: Resetting Areas");
        if (global.copyover) copyover_broadcast("STATUS: Resetting Areas.", TRUE);
        area_update();

        log_string("STATUS: Loading Banned Sites");
        if (global.copyover) copyover_broadcast("STATUS: Loading Banned Sites.", TRUE);
        load_bans();

        log_string("STATUS: Loading Notes");
        if (global.copyover) copyover_broadcast("STATUS: Loading Notes.", TRUE);
        load_notes();

        log_string("STATUS: Loading Statistics");
        if (global.copyover) copyover_broadcast("STATUS: Loading Statistics.", TRUE);
        load_statistics();

        log_string("STATUS: Initializing Database");
        if (global.copyover) copyover_broadcast("STATUS: Initializing Database.", TRUE);
        open_db();

    }

    return;
}

/*
 * OLC
 * Use these macros to load any new area formats that you choose to
 * support on your MUD.  See the load_area format below for
 * a short example.
 */
#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )                \
                if ( !str_cmp( word, literal ) )    \
                {                                   \
                    field  = value;                 \
                    fMatch = TRUE;                  \
                    break;                          \
                                }

#define SKEY( string, field )                       \
                if ( !str_cmp( word, string ) )     \
                {                                   \
                    free_string( field );           \
                    field = fread_string( fp );     \
                    fMatch = TRUE;                  \
                    break;                          \
                                }

/* OLC
 * Snarf an 'area' header line.   Check this format.  MUCH better.  Add fields
 * too.
 *
 * #AREAFILE
 * Name   { All } Locke    Newbie School~
 * Repop  A teacher pops in the room and says, 'Repop coming!'~
 * Recall 3001
 * End
 */
void load_area(FILE * fp)
{
    AREA_DATA *pArea;
    char *word;
    bool fMatch;

    pArea = alloc_perm(sizeof(*pArea));
    pArea->age = 15;
    pArea->nplayer = 0;
    pArea->file_name = str_dup(strArea);
    pArea->vnum = top_area;
    pArea->name = str_dup("New Area");
    pArea->builders = str_dup("");
    pArea->security = 9;
    pArea->min_vnum = 0;
    pArea->max_vnum = 0;
    pArea->min_level = 0;
    pArea->max_level = 0;
    pArea->area_flags = 0;
    pArea->continent = 0;

    log_f("STATUS: Loading Area %s", pArea->file_name);

    for (;;)
    {
        word = feof(fp) ? "End" : fread_word(fp);
        fMatch = FALSE;

        switch (UPPER(word[0]))
        {
            case 'L':
                if (!str_cmp(word, "LevelRange"))
                {
                    pArea->min_level = fread_number(fp);
                    pArea->max_level = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }

                break;
            case 'N':
                SKEY("Name", pArea->name);
                break;
            case 'S':
                KEY("Security", pArea->security, fread_number(fp));
                break;
            case 'V':
                if (!str_cmp(word, "VNUMs"))
                {
                    pArea->min_vnum = fread_number(fp);
                    pArea->max_vnum = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }
                break;
            case 'E':
                if (!str_cmp(word, "End"))
                {
                    fMatch = TRUE;
                    if (area_first == NULL)
                        area_first = pArea;
                    if (area_last != NULL)
                        area_last->next = pArea;
                    area_last = pArea;
                    pArea->next = NULL;
                    current_area = pArea;
                    top_area++;

                    return;
                }
                break;
            case 'A':
                KEY("AreaFlags", pArea->area_flags, fread_flag(fp));
                break;
            case 'B':
                SKEY("Builders", pArea->builders);
                break;
            case 'C':
                SKEY("Credits", pArea->credits);

                if (!str_cmp(word, "Continent"))
                {
                    pArea->continent = fread_number(fp);
                    fMatch = TRUE;
                }

                break;
        }

        if (!fMatch)
        {
            global.last_boot_result = WARNING;
            bugf("load_area key not found: %s", word);
        }

    }
} // end load_area

/*
 * Sets vnum range for area using OLC protection features.
 */
void assign_area_vnum(int vnum)
{
    if (area_last->min_vnum == 0 || area_last->max_vnum == 0)
        area_last->min_vnum = area_last->max_vnum = vnum;
    if (vnum != URANGE(area_last->min_vnum, vnum, area_last->max_vnum))
    {
        if (vnum < area_last->min_vnum)
            area_last->min_vnum = vnum;
        else
            area_last->max_vnum = vnum;
    }
    return;
}

/*
 * Initialize the time and weather
 */
void init_weather()
{
    long lhour, lday, lmonth;

    lhour = (current_time - 650336715) / (PULSE_TICK / PULSE_PER_SECOND);
    time_info.hour = lhour % 24;
    lday = lhour / 24;
    time_info.day = lday % 35;
    lmonth = lday / 35;
    time_info.month = lmonth % 17;
    time_info.year = lmonth / 17;

    if (time_info.hour < 5)
        weather_info.sunlight = SUN_DARK;
    else if (time_info.hour < 6)
        weather_info.sunlight = SUN_RISE;
    else if (time_info.hour < 19)
        weather_info.sunlight = SUN_LIGHT;
    else if (time_info.hour < 20)
        weather_info.sunlight = SUN_SET;
    else
        weather_info.sunlight = SUN_DARK;

    weather_info.change = 0;
    weather_info.mmhg = 960;

    if (time_info.month >= 7 && time_info.month <= 12)
        weather_info.mmhg += number_range(1, 50);
    else
        weather_info.mmhg += number_range(1, 80);

    if (weather_info.mmhg <= 980)
        weather_info.sky = SKY_LIGHTNING;
    else if (weather_info.mmhg <= 1000)
        weather_info.sky = SKY_RAINING;
    else if (weather_info.mmhg <= 1020)
        weather_info.sky = SKY_CLOUDY;
    else
        weather_info.sky = SKY_CLOUDLESS;

} // end init_weather

/*
 * Snarf a help section.
 */
void load_helps(FILE * fp, char *fname)
{
    HELP_DATA *pHelp;
    int level;
    char *keyword;

    for (;;)
    {
        HELP_AREA *had;

        level = fread_number(fp);
        keyword = fread_string(fp);

        if (keyword[0] == '$')
            break;

        if (!had_list)
        {
            had = new_had();
            had->filename = str_dup(fname);
            had->area = current_area;
            if (current_area)
                current_area->helps = had;
            had_list = had;
        }
        else if (str_cmp(fname, had_list->filename))
        {
            had = new_had();
            had->filename = str_dup(fname);
            had->area = current_area;
            if (current_area)
                current_area->helps = had;
            had->next = had_list;
            had_list = had;
        }
        else
            had = had_list;

        pHelp = new_help();
        pHelp->level = level;
        pHelp->keyword = keyword;
        pHelp->text = fread_string(fp);

        if (!str_cmp(pHelp->keyword, "greeting"))
            help_greeting = pHelp->text;

        if (help_first == NULL)
            help_first = pHelp;
        if (help_last != NULL)
            help_last->next = pHelp;

        help_last = pHelp;
        pHelp->next = NULL;

        if (!had->first)
            had->first = pHelp;
        if (!had->last)
            had->last = pHelp;

        had->last->next_area = pHelp;
        had->last = pHelp;
        pHelp->next_area = NULL;

        top_help++;
    }

    return;
}

/*
 * Adds a reset to a room.  OLC
 * Similar to add_reset in olc.c
 */
void new_reset(ROOM_INDEX_DATA * pR, RESET_DATA * pReset)
{
    RESET_DATA *pr;

    if (!pR)
        return;

    pr = pR->reset_last;

    if (!pr)
    {
        pR->reset_first = pReset;
        pR->reset_last = pReset;
    }
    else
    {
        pR->reset_last->next = pReset;
        pR->reset_last = pReset;
        pR->reset_last->next = NULL;
    }

/*    top_reset++; no estamos asignando memoria!!!! */

    return;
}

/*
 * Snarf a reset section.
 */
void load_resets(FILE * fp)
{
    RESET_DATA *pReset;
    EXIT_DATA *pexit;
    ROOM_INDEX_DATA *pRoomIndex;
    int rVnum = -1;

    if (!area_last)
    {
        bug("Load_resets: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;)
    {
        char letter;

        if ((letter = fread_letter(fp)) == 'S')
            break;

        if (letter == '*')
        {
            fread_to_eol(fp);
            continue;
        }

        pReset = new_reset_data();
        pReset->command = letter;
        /* if_flag */ fread_number(fp);
        pReset->arg1 = fread_number(fp);
        pReset->arg2 = fread_number(fp);
        pReset->arg3 = (letter == 'G' || letter == 'R')
            ? 0 : fread_number(fp);
        pReset->arg4 = (letter == 'P' || letter == 'M')
            ? fread_number(fp) : 0;
        fread_to_eol(fp);

        switch (pReset->command)
        {
            case 'M':
            case 'O':
                rVnum = pReset->arg3;
                break;

            case 'P':
            case 'G':
            case 'E':
                break;

            case 'D':
                pRoomIndex = get_room_index((rVnum = pReset->arg1));
                if (pReset->arg2 < 0
                    || pReset->arg2 >= MAX_DIR
                    || !pRoomIndex
                    || !(pexit = pRoomIndex->exit[pReset->arg2])
                    || !IS_SET(pexit->rs_flags, EX_ISDOOR))
                {
                    bugf("Load_resets: 'D': exit %d, room %d not door.",
                        pReset->arg2, pReset->arg1);
                    exit(1);
                }

                switch (pReset->arg3)
                {
                    default:
                        bug("Load_resets: 'D': bad 'locks': %d.",
                            pReset->arg3);
                        break;
                    case 0:
                        break;
                    case 1:
                        SET_BIT(pexit->rs_flags, EX_CLOSED);
                        SET_BIT(pexit->exit_info, EX_CLOSED);
                        break;
                    case 2:
                        SET_BIT(pexit->rs_flags, EX_CLOSED | EX_LOCKED);
                        SET_BIT(pexit->exit_info, EX_CLOSED | EX_LOCKED);
                        break;
                }
                break;

            case 'R':
                rVnum = pReset->arg1;
                break;
        }

        if (rVnum == -1)
        {
            bugf("load_resets : rVnum == -1");
            exit(1);
        }

        if (pReset->command != 'D')
            new_reset(get_room_index(rVnum), pReset);
        else
            free_reset_data(pReset);
    }

    return;
}

/*
 * Snarf a room section.
 */
void load_rooms(FILE * fp)
{
    ROOM_INDEX_DATA *pRoomIndex;

    if (area_last == NULL)
    {
        bug("Load_resets: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;)
    {
        int vnum;
        char letter;
        int door;
        int iHash;

        letter = fread_letter(fp);
        if (letter != '#')
        {
            bug("Load_rooms: # not found.", 0);
            exit(1);
        }

        vnum = fread_number(fp);
        if (vnum == 0)
            break;

        fBootDb = FALSE;
        if (get_room_index(vnum) != NULL)
        {
            bug("Load_rooms: vnum %d duplicated.", vnum);
            exit(1);
        }
        fBootDb = TRUE;

        pRoomIndex = alloc_perm(sizeof(*pRoomIndex));
        pRoomIndex->owner = str_dup("");
        pRoomIndex->people = NULL;
        pRoomIndex->contents = NULL;
        pRoomIndex->extra_descr = NULL;
        pRoomIndex->area = area_last;
        pRoomIndex->vnum = vnum;
        pRoomIndex->name = fread_string(fp);
        pRoomIndex->description = fread_string(fp);
        /* Area number */ fread_number(fp);
        pRoomIndex->room_flags = fread_flag(fp);
        pRoomIndex->sector_type = fread_number(fp);
        pRoomIndex->light = 0;
        for (door = 0; door < MAX_DIR; door++)
            pRoomIndex->exit[door] = NULL;

        /* defaults */
        pRoomIndex->heal_rate = 100;
        pRoomIndex->mana_rate = 100;

        for (;;)
        {
            letter = fread_letter(fp);

            if (letter == 'S')
                break;

            if (letter == 'H')    /* healing room */
                pRoomIndex->heal_rate = fread_number(fp);

            else if (letter == 'M')    /* mana room */
                pRoomIndex->mana_rate = fread_number(fp);

            else if (letter == 'C')
            {                    /* clan */
                if (pRoomIndex->clan)
                {
                    bug("Load_rooms: duplicate clan fields.", 0);
                    exit(1);
                }
                pRoomIndex->clan = clan_lookup(fread_string(fp));
            }


            else if (letter == 'D')
            {
                EXIT_DATA *pexit;
                int locks;

                door = fread_number(fp);
                if (door < 0 || door > 9)
                {
                    bug("Fread_rooms: vnum %d has bad door number.", vnum);
                    exit(1);
                }

                pexit = alloc_perm(sizeof(*pexit));
                pexit->description = fread_string(fp);
                pexit->keyword = fread_string(fp);
                pexit->exit_info = 0;
                pexit->rs_flags = 0;    /* OLC */
                locks = fread_number(fp);
                pexit->key = fread_number(fp);
                pexit->u1.vnum = fread_number(fp);
                pexit->orig_door = door;    /* OLC */

                switch (locks)
                {
                    case 1:
                        pexit->exit_info = EX_ISDOOR;
                        pexit->rs_flags = EX_ISDOOR;
                        break;
                    case 2:
                        pexit->exit_info = EX_ISDOOR | EX_PICKPROOF;
                        pexit->rs_flags = EX_ISDOOR | EX_PICKPROOF;
                        break;
                    case 3:
                        pexit->exit_info = EX_ISDOOR | EX_NOPASS;
                        pexit->rs_flags = EX_ISDOOR | EX_NOPASS;
                        break;
                    case 4:
                        pexit->exit_info =
                            EX_ISDOOR | EX_NOPASS | EX_PICKPROOF;
                        pexit->rs_flags =
                            EX_ISDOOR | EX_NOPASS | EX_PICKPROOF;
                        break;
                }

                pRoomIndex->exit[door] = pexit;
                top_exit++;
            }
            else if (letter == 'E')
            {
                EXTRA_DESCR_DATA *ed;

                ed = alloc_perm(sizeof(*ed));
                ed->keyword = fread_string(fp);
                ed->description = fread_string(fp);
                ed->next = pRoomIndex->extra_descr;
                pRoomIndex->extra_descr = ed;
                top_ed++;
            }

            else if (letter == 'O')
            {
                if (pRoomIndex->owner[0] != '\0')
                {
                    bug("Load_rooms: duplicate owner.", 0);
                    exit(1);
                }

                pRoomIndex->owner = fread_string(fp);
            }

            else
            {
                bug("Load_rooms: vnum %d has flag not 'DES'.", vnum);
                exit(1);
            }
        }

        iHash = vnum % MAX_KEY_HASH;
        pRoomIndex->next = room_index_hash[iHash];
        room_index_hash[iHash] = pRoomIndex;
        top_room++;
        top_vnum_room = top_vnum_room < vnum ? vnum : top_vnum_room;    /* OLC */
        assign_area_vnum(vnum);    /* OLC */
    }

    return;
}

/*
 * Snarf a shop section.
 */
void load_shops(FILE * fp)
{
    SHOP_DATA *pShop;
    int keeper = 0;

    for (;;)
    {
        MOB_INDEX_DATA *pMobIndex;
        int iTrade;

        // ROM mem leak fix, check the keeper before allocating the memory
        // to the SHOP_DATA variable.
        keeper = fread_number(fp);

        if (keeper == 0)
        {
            break;
        }

        // Now that we have a non zero keeper number we can allocate the memory
        pShop = alloc_perm(sizeof(*pShop));
        pShop->keeper = keeper;

        for (iTrade = 0; iTrade < MAX_TRADE; iTrade++)
        {
            pShop->buy_type[iTrade] = fread_number(fp);
        }

        pShop->profit_buy = fread_number(fp);
        pShop->profit_sell = fread_number(fp);
        pShop->open_hour = fread_number(fp);
        pShop->close_hour = fread_number(fp);
        fread_to_eol(fp);
        pMobIndex = get_mob_index(pShop->keeper);
        pMobIndex->pShop = pShop;

        if (shop_first == NULL)
        {
            shop_first = pShop;
        }

        if (shop_last != NULL)
        {
            shop_last->next = pShop;
        }

        shop_last = pShop;
        pShop->next = NULL;
        top_shop++;
    }

    return;
}

/*
 * Snarf spec proc declarations.
 */
void load_specials(FILE * fp)
{
    for (;;)
    {
        MOB_INDEX_DATA *pMobIndex;
        char letter;

        switch (letter = fread_letter(fp))
        {
            default:
                bug("Load_specials: letter '%c' not *MS.", letter);
                exit(1);

            case 'S':
                return;

            case '*':
                break;

            case 'M':
                pMobIndex = get_mob_index(fread_number(fp));
                pMobIndex->spec_fun = spec_lookup(fread_word(fp));
                if (pMobIndex->spec_fun == 0)
                {
                    bug("Load_specials: 'M': vnum %d.", pMobIndex->vnum);
                    exit(1);
                }
                break;
        }

        fread_to_eol(fp);
    }
}

/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits(void)
{
    ROOM_INDEX_DATA *pRoomIndex;
    EXIT_DATA *pexit;
    RESET_DATA *pReset;
    ROOM_INDEX_DATA *iLastRoom, *iLastObj;
    int iHash;
    int door;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
        for (pRoomIndex = room_index_hash[iHash];
        pRoomIndex != NULL; pRoomIndex = pRoomIndex->next)
        {
            bool fexit;

            iLastRoom = iLastObj = NULL;

            /* OLC : new check of resets */
            for (pReset = pRoomIndex->reset_first; pReset;
            pReset = pReset->next)
            {
                switch (pReset->command)
                {
                    default:
                        bugf("fix_exits : room %d with reset cmd %c", pRoomIndex->vnum, pReset->command);
                        global.last_boot_result = FAILURE;
                        if (global.copyover) copyover_broadcast("STATUS: Shutting Down", TRUE);
                        exit(1);
                        break;

                    case 'M':
                        get_mob_index(pReset->arg1);
                        iLastRoom = get_room_index(pReset->arg3);
                        break;

                    case 'O':
                        get_obj_index(pReset->arg1);
                        iLastObj = get_room_index(pReset->arg3);
                        break;

                    case 'P':
                        get_obj_index(pReset->arg1);
                        if (iLastObj == NULL)
                        {
                            global.last_boot_result = FAILURE;
                            bugf("fix_exits : reset in room %d with iLastObj NULL", pRoomIndex->vnum);
                            if (global.copyover) copyover_broadcast("STATUS: Shutting Down", TRUE);
                            exit(1);
                        }
                        break;

                    case 'G':
                    case 'E':
                        get_obj_index(pReset->arg1);
                        if (iLastRoom == NULL)
                        {
                            global.last_boot_result = FAILURE;
                            bugf("fix_exits : reset in room %d with iLastRoom NULL", pRoomIndex->vnum);
                            if (global.copyover) copyover_broadcast("STATUS: Shutting Down", TRUE);
                            exit(1);
                        }
                        iLastObj = iLastRoom;
                        break;

                    case 'D':
                        bugf("???");
                        break;

                    case 'R':
                        get_room_index(pReset->arg1);
                        if (pReset->arg2 < 0 || pReset->arg2 > MAX_DIR)
                        {
                            global.last_boot_result = FAILURE;
                            bugf("fix_exits : reset in room %d with arg2 %d >= MAX_DIR", pRoomIndex->vnum, pReset->arg2);
                            if (global.copyover) copyover_broadcast("STATUS: Shutting Down", TRUE);
                            exit(1);
                        }
                        break;
                }                /* switch */
            }                    /* for */

            fexit = FALSE;
            for (door = 0; door < MAX_DIR; door++)
            {
                if ((pexit = pRoomIndex->exit[door]) != NULL)
                {
                    if (pexit->u1.vnum <= 0
                        || get_room_index(pexit->u1.vnum) == NULL)
                        pexit->u1.to_room = NULL;
                    else
                    {
                        fexit = TRUE;
                        pexit->u1.to_room = get_room_index(pexit->u1.vnum);
                    }
                }
            }
            if (!fexit)
                SET_BIT(pRoomIndex->room_flags, ROOM_NO_MOB);
        }
    }

    // It appears this is just reporting the exists that need to be fixed... some areas have
    // exits that may not go back and forth (like hell).  Commeting this out for now, needless
    // processing.  Uncomment it if you want to check for exits that don't go back and forth in
    // the future or put it into it's own OLC wiz command.
    /*
    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
        for (pRoomIndex = room_index_hash[iHash];
             pRoomIndex != NULL; pRoomIndex = pRoomIndex->next)
        {
            for (door = 0; door < MAX_DIR; door++)
            {
                if ((pexit = pRoomIndex->exit[door]) != NULL
                    && (to_room = pexit->u1.to_room) != NULL
                    && (pexit_rev = to_room->exit[rev_dir[door]]) != NULL
                    && pexit_rev->u1.to_room != pRoomIndex
                    && (pRoomIndex->vnum < 1200 || pRoomIndex->vnum > 1299))
                {
                    sprintf (buf, "Fix_exits: %d:%d -> %d:%d -> %d.",
                             pRoomIndex->vnum, door,
                             to_room->vnum, rev_dir[door],
                             (pexit_rev->u1.to_room == NULL)
                             ? 0 : pexit_rev->u1.to_room->vnum);
                    bug (buf, 0);
                }
            }
        }
    }*/

    // If it gets here and it's unknown then it's a success
    if (global.last_boot_result == UNKNOWN)
        global.last_boot_result = SUCCESS;

    return;
}

/*
 * Load mobprogs section
 */
void load_mobprogs(FILE * fp)
{
    MPROG_CODE *pMprog;

    if (area_last == NULL)
    {
        bug("Load_mobprogs: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;)
    {
        int vnum;
        char letter;

        letter = fread_letter(fp);
        if (letter != '#')
        {
            bugf("Load_mobprogs: # not found. (%c)", letter);
            exit(1);
        }

        vnum = fread_number(fp);

        if (vnum == 0)
        {
            break;
        }

        fBootDb = FALSE;
        if (get_mprog_index(vnum) != NULL)
        {
            bug("Load_mobprogs: vnum %d duplicated.", vnum);
            exit(1);
        }
        fBootDb = TRUE;

        pMprog = alloc_perm(sizeof(*pMprog));
        pMprog->vnum = vnum;
        pMprog->name = fread_string(fp);
        pMprog->code = fread_string(fp);

        if (mprog_list == NULL)
        {
            mprog_list = pMprog;
        }
        else
        {
            pMprog->next = mprog_list;
            mprog_list = pMprog;
        }

        top_mprog_index++;
    }
    // marker (consider advanced logging config feature, commented out for
    // now as there aren't any mobprogs yet in the stock areas and it spams the
    // startup logs
    //log_f("Loaded %d mobprogs.", top_mprog_index);
    return;
}

/*
 *  Translate mobprog vnums pointers to real code
 */
void fix_mobprogs(void)
{
    MOB_INDEX_DATA *pMobIndex;
    MPROG_LIST *list;
    MPROG_CODE *prog;
    int iHash;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
    {
        for (pMobIndex = mob_index_hash[iHash];
        pMobIndex != NULL; pMobIndex = pMobIndex->next)
        {
            for (list = pMobIndex->mprogs; list != NULL; list = list->next)
            {
                if ((prog = get_mprog_index(list->vnum)) != NULL)
                {
                    list->code = prog->code;
                    list->name = prog->name;
                }
                else
                {
                    global.last_boot_result = FAILURE;
                    bug("Fix_mobprogs: code vnum %d not found.", list->vnum);
                    if (global.copyover) copyover_broadcast("STATUS: Shutting Down", TRUE);
                    exit(1);
                }
            }
        }
    }

    // If it's unknown when it gets here then it's a success.
    if (global.last_boot_result == UNKNOWN)
        global.last_boot_result = SUCCESS;

}

/*
 * Repopulate areas periodically.
 */
void area_update(void)
{
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];

    for (pArea = area_first; pArea != NULL; pArea = pArea->next)
    {

        if (++pArea->age < 3)
            continue;

        /*
         * Check age and reset.
         * Note: Mud School resets every 3 minutes (not 15).
         */
        if ((!pArea->empty && (pArea->nplayer == 0 || pArea->age >= 15))
            || pArea->age >= 31)
        {
            ROOM_INDEX_DATA *pRoomIndex;

            reset_area(pArea);
            sprintf(buf, "%s has just been reset.", pArea->name);
            wiznet(buf, NULL, NULL, WIZ_RESETS, 0, 0);

            pArea->age = number_range(0, 3);
            pRoomIndex = get_room_index(ROOM_VNUM_SCHOOL);
            if (pRoomIndex != NULL && pArea == pRoomIndex->area)
                pArea->age = 15 - 2;
            else if (pArea->nplayer == 0)
                pArea->empty = TRUE;
        }
    }

    return;
}

/* OLC
 * Reset one room.  Called by reset_area and olc.
 */
void reset_room(ROOM_INDEX_DATA * pRoom)
{
    RESET_DATA *pReset;
    CHAR_DATA *pMob;
    CHAR_DATA *mob;
    OBJ_DATA *pObj;
    CHAR_DATA *LastMob = NULL;
    OBJ_DATA *LastObj = NULL;
    int iExit;
    bool last;

    if (!pRoom)
        return;

    pMob = NULL;
    last = FALSE;

    for (iExit = 0; iExit < MAX_DIR; iExit++)
    {
        EXIT_DATA *pExit;
        if ((pExit = pRoom->exit[iExit])
            /*  && !IS_SET( pExit->exit_info, EX_BASHED )   ROM OLC */
            )
        {
            pExit->exit_info = pExit->rs_flags;
            if ((pExit->u1.to_room != NULL)
                && ((pExit = pExit->u1.to_room->exit[rev_dir[iExit]])))
            {
                /* nail the other side */
                pExit->exit_info = pExit->rs_flags;
            }
        }
    }

    for (pReset = pRoom->reset_first; pReset != NULL; pReset = pReset->next)
    {
        MOB_INDEX_DATA *pMobIndex;
        OBJ_INDEX_DATA *pObjIndex;
        OBJ_INDEX_DATA *pObjToIndex;
        ROOM_INDEX_DATA *pRoomIndex;
        char buf[MAX_STRING_LENGTH];
        int count, limit = 0;

        switch (pReset->command)
        {
            default:
                global.last_boot_result = WARNING;
                bug("Reset_room: bad command %c.", pReset->command);
                break;

            case 'M':
                if (!(pMobIndex = get_mob_index(pReset->arg1)))
                {
                    global.last_boot_result = WARNING;
                    bug("Reset_room: 'M': bad vnum %d.", pReset->arg1);
                    continue;
                }

                if ((pRoomIndex = get_room_index(pReset->arg3)) == NULL)
                {
                    global.last_boot_result = WARNING;
                    bug("Reset_area: 'R': bad vnum %d.", pReset->arg3);
                    continue;
                }

                // This checks to see if it's over the max allowed in the world, when a mob is created this
                // is incrimented and when it's extracted it should be decrimented.
                if (pMobIndex->count >= pReset->arg2)
                {
                    last = FALSE;
                    break;
                }

                // Count the mobs in the room and see if it's over the max
                count = 0;
                for (mob = pRoomIndex->people; mob != NULL; mob = mob->next_in_room) if (mob->pIndexData == pMobIndex)
                {
                    count++;
                    if (count >= pReset->arg4)
                    {
                        last = FALSE;
                        break;
                    }
                }

                if (count >= pReset->arg4)
                    break;

                pMob = create_mobile(pMobIndex);

                // Set the home zone for the mobile so we always know where it was reset from.
                pMob->zone = pRoomIndex->area;

                /*
                 * Some more hard coding.
                 */
                if (room_is_dark(pRoom))
                    SET_BIT(pMob->affected_by, AFF_INFRARED);

                /*
                 * Pet shop mobiles get ACT_PET set.
                 */
                {
                    ROOM_INDEX_DATA *pRoomIndexPrev;

                    pRoomIndexPrev = get_room_index(pRoom->vnum - 1);
                    if (pRoomIndexPrev
                        && IS_SET(pRoomIndexPrev->room_flags, ROOM_PET_SHOP))
                        SET_BIT(pMob->act, ACT_PET);
                }

                char_to_room(pMob, pRoom);

                LastMob = pMob;
                last = TRUE;
                break;

            case 'O':
                if (!(pObjIndex = get_obj_index(pReset->arg1)))
                {
                    global.last_boot_result = WARNING;
                    bug("Reset_room: 'O' 1 : bad vnum %d", pReset->arg1);
                    sprintf(buf, "%d %d %d %d", pReset->arg1, pReset->arg2,
                        pReset->arg3, pReset->arg4);
                    bug(buf, 1);
                    continue;
                }

                if (!(pRoomIndex = get_room_index(pReset->arg3)))
                {
                    global.last_boot_result = WARNING;
                    bug("Reset_room: 'O' 2 : bad vnum %d.", pReset->arg3);
                    sprintf(buf, "%d %d %d %d", pReset->arg1, pReset->arg2,
                        pReset->arg3, pReset->arg4);
                    bug(buf, 1);
                    continue;
                }

                if (pRoom->area->nplayer > 0
                    || count_obj_list(pObjIndex, pRoom->contents) > 0)
                {
                    last = FALSE;
                    break;
                }

                pObj = create_object(pObjIndex);
                pObj->cost = 0;
                obj_to_room(pObj, pRoom);
                last = TRUE;
                break;

            case 'P':
                if (!(pObjIndex = get_obj_index(pReset->arg1)))
                {
                    global.last_boot_result = WARNING;
                    bug("Reset_room: 'P': bad vnum %d.", pReset->arg1);
                    continue;
                }

                if (!(pObjToIndex = get_obj_index(pReset->arg3)))
                {
                    global.last_boot_result = WARNING;
                    bug("Reset_room: 'P': bad vnum %d.", pReset->arg3);
                    continue;
                }

                if (pReset->arg2 > 50)    /* old format */
                    limit = 6;
                else if (pReset->arg2 == -1)    /* no limit */
                    limit = 999;
                else
                    limit = pReset->arg2;

                if (pRoom->area->nplayer > 0
                    || (LastObj = get_obj_type(pObjToIndex)) == NULL
                    || (LastObj->in_room == NULL && !last)
                    || (pObjIndex->count >=
                        limit /* && number_range(0,4) != 0 */)
                    || (count =
                        count_obj_list(pObjIndex,
                            LastObj->contains)) > pReset->arg4)
                {
                    last = FALSE;
                    break;
                }
                /* lastObj->level  -  ROM */

                while (count < pReset->arg4)
                {
                    pObj = create_object(pObjIndex);
                    obj_to_obj(pObj, LastObj);
                    count++;
                    if (pObjIndex->count >= limit)
                        break;
                }

                /* fix object lock state! */
                LastObj->value[1] = LastObj->pIndexData->value[1];
                last = TRUE;
                break;

            case 'G':
            case 'E':
                if (!(pObjIndex = get_obj_index(pReset->arg1)))
                {
                    global.last_boot_result = WARNING;
                    bug("Reset_room: 'E' or 'G': bad vnum %d.",
                        pReset->arg1);
                    continue;
                }

                if (!last)
                    break;

                if (!LastMob)
                {
                    global.last_boot_result = WARNING;
                    bug("Reset_room: 'E' or 'G': null mob for vnum %d.",
                        pReset->arg1);
                    last = FALSE;
                    break;
                }

                if (LastMob->pIndexData->pShop)
                {
                    /* Shop-keeper? */
                    pObj = create_object(pObjIndex);
                    SET_BIT(pObj->extra_flags, ITEM_INVENTORY);    /* ROM OLC */
                }
                else
                {                /* ROM OLC else version */

                    int limit;
                    if (pReset->arg2 > 50)    /* old format */
                        limit = 6;
                    else if (pReset->arg2 == -1 || pReset->arg2 == 0)    /* no limit */
                        limit = 999;
                    else
                        limit = pReset->arg2;

                    if (pObjIndex->count < limit || number_range(0, 4) == 0)
                    {
                        pObj = create_object(pObjIndex);
                        /* error message if it is too high */
                        if (pObj->level > LastMob->level + 3)
                        {
                            log_f("Check Levels:");
                            log_f("  Object: (VNUM %5d)(Level %2d) %s",
                                pObj->pIndexData->vnum,
                                pObj->level,
                                pObj->short_descr);
                            log_f("     Mob: (VNUM %5d)(Level %2d) %s",
                                LastMob->pIndexData->vnum,
                                LastMob->level,
                                LastMob->short_descr);
                        }
                    }
                    else
                        break;
                }

                obj_to_char(pObj, LastMob);
                if (pReset->command == 'E')
                    equip_char(LastMob, pObj, pReset->arg3);
                last = TRUE;
                break;

            case 'D':
                break;

            case 'R':
                if (!(pRoomIndex = get_room_index(pReset->arg1)))
                {
                    global.last_boot_result = WARNING;
                    bug("Reset_room: 'R': bad vnum %d.", pReset->arg1);
                    continue;
                }

                {
                    EXIT_DATA *pExit;
                    int d0;
                    int d1;

                    for (d0 = 0; d0 < pReset->arg2 - 1; d0++)
                    {
                        d1 = number_range(d0, pReset->arg2 - 1);
                        pExit = pRoomIndex->exit[d0];
                        pRoomIndex->exit[d0] = pRoomIndex->exit[d1];
                        pRoomIndex->exit[d1] = pExit;
                    }
                }
                break;
        }
    }

    return;
}

/* OLC
 * Reset one area.
 */
void reset_area(AREA_DATA * pArea)
{
    ROOM_INDEX_DATA *pRoom;
    int vnum;

    for (vnum = pArea->min_vnum; vnum <= pArea->max_vnum; vnum++)
    {
        if ((pRoom = get_room_index(vnum)))
            reset_room(pRoom);
    }

    // If the result is unknown at this point then it's a success
    if (global.last_boot_result == UNKNOWN)
        global.last_boot_result = SUCCESS;

    return;
}

/*
 * Create an instance of a mobile.
 */
CHAR_DATA *create_mobile(MOB_INDEX_DATA * pMobIndex)
{
    CHAR_DATA *mob;
    int i;
    AFFECT_DATA af;

    mobile_count++;

    if (pMobIndex == NULL)
    {
        bug("Create_mobile: NULL pMobIndex.", 0);
        exit(1);
    }

    mob = new_char();

    mob->pIndexData = pMobIndex;

    mob->name = str_dup(pMobIndex->player_name);    /* OLC */
    mob->short_descr = str_dup(pMobIndex->short_descr);    /* OLC */
    mob->long_descr = str_dup(pMobIndex->long_descr);    /* OLC */
    mob->description = str_dup(pMobIndex->description);    /* OLC */
    mob->id = get_mob_id();
    mob->spec_fun = pMobIndex->spec_fun;
    mob->prompt = NULL;
    mob->mprog_target = NULL;

    if (pMobIndex->wealth == 0)
    {
        mob->silver = 0;
        mob->gold = 0;
    }
    else
    {
        long wealth;

        wealth =
            number_range(pMobIndex->wealth / 2, 3 * pMobIndex->wealth / 2);
        mob->gold = number_range(wealth / 200, wealth / 100);
        mob->silver = wealth - (mob->gold * 100);
    }

    /* read from prototype */
    mob->group = pMobIndex->group;
    mob->act = pMobIndex->act;
    mob->comm = COMM_NOCHANNELS | COMM_NOSHOUT | COMM_NOTELL;
    mob->affected_by = pMobIndex->affected_by;
    mob->alignment = pMobIndex->alignment;
    mob->level = pMobIndex->level;
    mob->hitroll = pMobIndex->hitroll;
    mob->damroll = pMobIndex->damage[DICE_BONUS];
    mob->max_hit = dice(pMobIndex->hit[DICE_NUMBER],
        pMobIndex->hit[DICE_TYPE])
        + pMobIndex->hit[DICE_BONUS];
    mob->hit = mob->max_hit;
    mob->max_mana = dice(pMobIndex->mana[DICE_NUMBER],
        pMobIndex->mana[DICE_TYPE])
        + pMobIndex->mana[DICE_BONUS];
    mob->mana = mob->max_mana;
    mob->damage[DICE_NUMBER] = pMobIndex->damage[DICE_NUMBER];
    mob->damage[DICE_TYPE] = pMobIndex->damage[DICE_TYPE];
    mob->dam_type = pMobIndex->dam_type;
    if (mob->dam_type == 0)
        switch (number_range(1, 3))
        {
            case (1) :
                mob->dam_type = 3;
                break;        /* slash */
            case (2) :
                mob->dam_type = 7;
                break;        /* pound */
            case (3) :
                mob->dam_type = 11;
                break;        /* pierce */
        }
    for (i = 0; i < 4; i++)
        mob->armor[i] = pMobIndex->ac[i];
    mob->off_flags = pMobIndex->off_flags;
    mob->imm_flags = pMobIndex->imm_flags;
    mob->res_flags = pMobIndex->res_flags;
    mob->vuln_flags = pMobIndex->vuln_flags;
    mob->start_pos = pMobIndex->start_pos;
    mob->default_pos = pMobIndex->default_pos;
    mob->sex = pMobIndex->sex;
    if (mob->sex == 3)        /* random sex */
        mob->sex = number_range(1, 2);
    mob->race = pMobIndex->race;
    mob->form = pMobIndex->form;
    mob->parts = pMobIndex->parts;
    mob->size = pMobIndex->size;
    mob->material = str_dup(pMobIndex->material);

    /* computed on the spot */

    for (i = 0; i < MAX_STATS; i++)
        mob->perm_stat[i] = UMIN(25, 11 + mob->level / 4);

    if (IS_SET(mob->act, ACT_WARRIOR))
    {
        mob->perm_stat[STAT_STR] += 3;
        mob->perm_stat[STAT_INT] -= 1;
        mob->perm_stat[STAT_CON] += 2;
    }

    if (IS_SET(mob->act, ACT_THIEF))
    {
        mob->perm_stat[STAT_DEX] += 3;
        mob->perm_stat[STAT_INT] += 1;
        mob->perm_stat[STAT_WIS] -= 1;
    }

    if (IS_SET(mob->act, ACT_CLERIC))
    {
        mob->perm_stat[STAT_WIS] += 3;
        mob->perm_stat[STAT_DEX] -= 1;
        mob->perm_stat[STAT_STR] += 1;
    }

    if (IS_SET(mob->act, ACT_MAGE))
    {
        mob->perm_stat[STAT_INT] += 3;
        mob->perm_stat[STAT_STR] -= 1;
        mob->perm_stat[STAT_DEX] += 1;
    }

    if (IS_SET(mob->off_flags, OFF_FAST))
        mob->perm_stat[STAT_DEX] += 2;

    mob->perm_stat[STAT_STR] += mob->size - SIZE_MEDIUM;
    mob->perm_stat[STAT_CON] += (mob->size - SIZE_MEDIUM) / 2;

    /* let's get some spell action */
    if (IS_AFFECTED(mob, AFF_SANCTUARY))
    {
        af.where = TO_AFFECTS;
        af.type = gsn_sanctuary;
        af.level = mob->level;
        af.duration = -1;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector = AFF_SANCTUARY;
        affect_to_char(mob, &af);
    }

    if (IS_AFFECTED(mob, AFF_HASTE))
    {
        af.where = TO_AFFECTS;
        af.type = gsn_haste;
        af.level = mob->level;
        af.duration = -1;
        af.location = APPLY_DEX;
        af.modifier = 1 + (mob->level >= 18) + (mob->level >= 25) +
            (mob->level >= 32);
        af.bitvector = AFF_HASTE;
        affect_to_char(mob, &af);
    }

    if (IS_AFFECTED(mob, AFF_PROTECT_EVIL))
    {
        af.where = TO_AFFECTS;
        af.type = gsn_protection_evil;
        af.level = mob->level;
        af.duration = -1;
        af.location = APPLY_SAVES;
        af.modifier = -1;
        af.bitvector = AFF_PROTECT_EVIL;
        affect_to_char(mob, &af);
    }

    if (IS_AFFECTED(mob, AFF_PROTECT_GOOD))
    {
        af.where = TO_AFFECTS;
        af.type = gsn_protection_good;
        af.level = mob->level;
        af.duration = -1;
        af.location = APPLY_SAVES;
        af.modifier = -1;
        af.bitvector = AFF_PROTECT_GOOD;
        affect_to_char(mob, &af);
    }

    mob->position = mob->start_pos;


    /* link the mob to the world list */
    mob->next = char_list;
    char_list = mob;
    pMobIndex->count++;
    return mob;
}

/* duplicate a mobile exactly -- except inventory */
void clone_mobile(CHAR_DATA * parent, CHAR_DATA * clone)
{
    int i;
    AFFECT_DATA *paf;

    if (parent == NULL || clone == NULL || !IS_NPC(parent))
        return;

    /* start fixing values */
    clone->name = str_dup(parent->name);
    clone->version = parent->version;
    clone->short_descr = str_dup(parent->short_descr);
    clone->long_descr = str_dup(parent->long_descr);
    clone->description = str_dup(parent->description);
    clone->group = parent->group;
    clone->sex = parent->sex;
    clone->class = parent->class;
    clone->race = parent->race;
    clone->level = parent->level;
    clone->trust = 0;
    clone->timer = parent->timer;
    clone->wait = parent->wait;
    clone->hit = parent->hit;
    clone->max_hit = parent->max_hit;
    clone->mana = parent->mana;
    clone->max_mana = parent->max_mana;
    clone->move = parent->move;
    clone->max_move = parent->max_move;
    clone->gold = parent->gold;
    clone->silver = parent->silver;
    clone->exp = parent->exp;
    clone->act = parent->act;
    clone->comm = parent->comm;
    clone->imm_flags = parent->imm_flags;
    clone->res_flags = parent->res_flags;
    clone->vuln_flags = parent->vuln_flags;
    clone->invis_level = parent->invis_level;
    clone->affected_by = parent->affected_by;
    clone->position = parent->position;
    clone->practice = parent->practice;
    clone->train = parent->train;
    clone->saving_throw = parent->saving_throw;
    clone->alignment = parent->alignment;
    clone->hitroll = parent->hitroll;
    clone->damroll = parent->damroll;
    clone->wimpy = parent->wimpy;
    clone->form = parent->form;
    clone->parts = parent->parts;
    clone->size = parent->size;
    clone->material = str_dup(parent->material);
    clone->off_flags = parent->off_flags;
    clone->dam_type = parent->dam_type;
    clone->start_pos = parent->start_pos;
    clone->default_pos = parent->default_pos;
    clone->spec_fun = parent->spec_fun;

    for (i = 0; i < 4; i++)
        clone->armor[i] = parent->armor[i];

    for (i = 0; i < MAX_STATS; i++)
    {
        clone->perm_stat[i] = parent->perm_stat[i];
        clone->mod_stat[i] = parent->mod_stat[i];
    }

    for (i = 0; i < 3; i++)
        clone->damage[i] = parent->damage[i];

    /* now add the affects */
    for (paf = parent->affected; paf != NULL; paf = paf->next)
        affect_to_char(clone, paf);

}

/*
 * Create an instance of an object.
 */
OBJ_DATA *create_object(OBJ_INDEX_DATA * pObjIndex)
{
    AFFECT_DATA *paf;
    OBJ_DATA *obj;

    if (pObjIndex == NULL)
    {
        bug("Create_object: NULL pObjIndex.", 0);
        exit(1);
    }

    obj = new_obj();

    obj->pIndexData = pObjIndex;
    obj->in_room = NULL;
    obj->enchanted = FALSE;
    obj->level = pObjIndex->level;
    obj->wear_loc = -1;
    obj->count = 1;
    obj->name = str_dup(pObjIndex->name);    /* OLC */
    obj->short_descr = str_dup(pObjIndex->short_descr);    /* OLC */
    obj->description = str_dup(pObjIndex->description);    /* OLC */
    obj->material = str_dup(pObjIndex->material);
    obj->item_type = pObjIndex->item_type;
    obj->extra_flags = pObjIndex->extra_flags;
    obj->wear_flags = pObjIndex->wear_flags;
    obj->value[0] = pObjIndex->value[0];
    obj->value[1] = pObjIndex->value[1];
    obj->value[2] = pObjIndex->value[2];
    obj->value[3] = pObjIndex->value[3];
    obj->value[4] = pObjIndex->value[4];
    obj->weight = pObjIndex->weight;
    obj->cost = pObjIndex->cost;
    obj->condition = pObjIndex->condition;

    /*
     * Mess with object properties.
     */
    switch (obj->item_type)
    {
        default:
            bug("Read_object: vnum %d bad type.", pObjIndex->vnum);
            break;

        case ITEM_LIGHT:
            if (obj->value[2] == 999)
                obj->value[2] = -1;
            break;

        case ITEM_FURNITURE:
        case ITEM_TRASH:
        case ITEM_CONTAINER:
        case ITEM_DRINK_CON:
        case ITEM_KEY:
        case ITEM_FOOD:
        case ITEM_BOAT:
        case ITEM_CORPSE_NPC:
        case ITEM_CORPSE_PC:
        case ITEM_FOUNTAIN:
        case ITEM_MAP:
        case ITEM_CLOTHING:
        case ITEM_PORTAL:
        case ITEM_TREASURE:
        case ITEM_WARP_STONE:
        case ITEM_ROOM_KEY:
        case ITEM_GEM:
        case ITEM_JEWELRY:
        case ITEM_SCROLL:
        case ITEM_WAND:
        case ITEM_STAFF:
        case ITEM_WEAPON:
        case ITEM_ARMOR:
        case ITEM_POTION:
        case ITEM_PILL:
        case ITEM_MONEY:
        case ITEM_SHOVEL:
        case ITEM_FOG:
        case ITEM_PARCHMENT:
        case ITEM_SEED:
            break;
    }

    for (paf = pObjIndex->affected; paf != NULL; paf = paf->next)
    {
        if (paf->location == APPLY_SPELL_AFFECT)
        {
            affect_to_obj(obj, paf);
        }
    }

    obj->next = object_list;
    object_list = obj;
    pObjIndex->count++;

    return obj;
}

/* duplicate an object exactly -- except contents */
void clone_object(OBJ_DATA * parent, OBJ_DATA * clone)
{
    int i;
    AFFECT_DATA *paf;
    EXTRA_DESCR_DATA *ed, *ed_new;

    if (parent == NULL || clone == NULL)
        return;

    /* start fixing the object */
    clone->name = str_dup(parent->name);
    clone->short_descr = str_dup(parent->short_descr);
    clone->description = str_dup(parent->description);
    clone->item_type = parent->item_type;
    clone->extra_flags = parent->extra_flags;
    clone->wear_flags = parent->wear_flags;
    clone->weight = parent->weight;
    clone->cost = parent->cost;
    clone->level = parent->level;
    clone->condition = parent->condition;
    clone->material = str_dup(parent->material);
    clone->timer = parent->timer;
    clone->count = 1;

    for (i = 0; i < 5; i++)
        clone->value[i] = parent->value[i];

    /* affects */
    clone->enchanted = parent->enchanted;

    for (paf = parent->affected; paf != NULL; paf = paf->next)
        affect_to_obj(clone, paf);

    /* extended desc */
    for (ed = parent->extra_descr; ed != NULL; ed = ed->next)
    {
        ed_new = new_extra_descr();
        ed_new->keyword = str_dup(ed->keyword);
        ed_new->description = str_dup(ed->description);
        ed_new->next = clone->extra_descr;
        clone->extra_descr = ed_new;
    }

}

/*
 * Get an extra description from a list.
 */
char *get_extra_descr(const char *name, EXTRA_DESCR_DATA * ed)
{
    for (; ed != NULL; ed = ed->next)
    {
        if (is_name((char *)name, ed->keyword))
            return ed->description;
    }
    return NULL;
}

/*
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
MOB_INDEX_DATA *get_mob_index(int vnum)
{
    MOB_INDEX_DATA *pMobIndex;

    for (pMobIndex = mob_index_hash[vnum % MAX_KEY_HASH];
    pMobIndex != NULL; pMobIndex = pMobIndex->next)
    {
        if (pMobIndex->vnum == vnum)
            return pMobIndex;
    }

    if (fBootDb)
    {
        bug("Get_mob_index: bad vnum %d.", vnum);
        exit(1);
    }

    return NULL;
}

/*
 * Translates mob virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJ_INDEX_DATA *get_obj_index(int vnum)
{
    OBJ_INDEX_DATA *pObjIndex;

    for (pObjIndex = obj_index_hash[vnum % MAX_KEY_HASH];
    pObjIndex != NULL; pObjIndex = pObjIndex->next)
    {
        if (pObjIndex->vnum == vnum)
            return pObjIndex;
    }

    if (fBootDb)
    {
        bug("Get_obj_index: bad vnum %d.", vnum);
        exit(1);
    }

    return NULL;
}

/*
 * Translates mob virtual number to its room index struct.
 * Hash table lookup.  This will exit/crash the game on boot
 * if a requested room isn't found, otherwise it will return
 * a null.
 */
ROOM_INDEX_DATA *get_room_index(int vnum)
{
    ROOM_INDEX_DATA *pRoomIndex;

    for (pRoomIndex = room_index_hash[vnum % MAX_KEY_HASH];
    pRoomIndex != NULL; pRoomIndex = pRoomIndex->next)
    {
        if (pRoomIndex->vnum == vnum)
            return pRoomIndex;
    }

    if (fBootDb)
    {
        bug("Get_room_index: bad vnum %d.", vnum);
        exit(1);
    }

    return NULL;
}

MPROG_CODE *get_mprog_index(int vnum)
{
    MPROG_CODE *prg;
    for (prg = mprog_list; prg; prg = prg->next)
    {
        if (prg->vnum == vnum)
            return (prg);
    }
    return NULL;
}

/*
 * Read a letter from a file.
 */
char fread_letter(FILE * fp)
{
    char c;

    do
    {
        c = getc(fp);
    } while (isspace(c));

    return c;
}

/*
 * Read a number from a file.
 */
int fread_number(FILE * fp)
{
    int number;
    bool sign;
    char c;

    do
    {
        c = getc(fp);
    } while (isspace(c));

    number = 0;

    sign = FALSE;
    if (c == '+')
    {
        c = getc(fp);
    }
    else if (c == '-')
    {
        sign = TRUE;
        c = getc(fp);
    }

    if (!isdigit(c))
    {
        bug("Fread_number: bad format.", 0);
        exit(1);
    }

    while (isdigit(c))
    {
        number = number * 10 + c - '0';
        c = getc(fp);
    }

    if (sign)
        number = 0 - number;

    if (c == '|')
        number += fread_number(fp);
    else if (c != ' ')
        ungetc(c, fp);

    return number;
}

long fread_flag(FILE * fp)
{
    int number;
    char c;
    bool negative = FALSE;

    do
    {
        c = getc(fp);
    } while (isspace(c));

    if (c == '-')
    {
        negative = TRUE;
        c = getc(fp);
    }

    number = 0;

    if (!isdigit(c))
    {
        while (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
        {
            number += flag_convert(c);
            c = getc(fp);
        }
    }

    while (isdigit(c))
    {
        number = number * 10 + c - '0';
        c = getc(fp);
    }

    if (c == '|')
        number += fread_flag(fp);

    else if (c != ' ')
        ungetc(c, fp);

    if (negative)
        return -1 * number;

    return number;
}

long flag_convert(char letter)
{
    long bitsum = 0;
    char i;

    if ('A' <= letter && letter <= 'Z')
    {
        bitsum = 1;
        for (i = letter; i > 'A'; i--)
            bitsum *= 2;
    }
    else if ('a' <= letter && letter <= 'z')
    {
        bitsum = 67108864;        /* 2^26 */
        for (i = letter; i > 'a'; i--)
            bitsum *= 2;
    }

    return bitsum;
}

/*
 * Read and allocate space for a string from a file.
 * These strings are read-only and shared.
 * Strings are hashed:
 *   each string prepended with hash pointer to prev string,
 *   hash code is simply the string length.
 *   this function takes 40% to 50% of boot-up time.
 */
char *fread_string(FILE * fp)
{
    char *plast;
    char c;

    plast = top_string + sizeof(char *);
    if (plast > &string_space[MAX_STRING - MAX_STRING_LENGTH])
    {
        bug("Fread_string: MAX_STRING %d exceeded.", MAX_STRING);
        exit(1);
    }

    /*
     * Skip blanks.
     * Read first char.
     */
    do
    {
        c = getc(fp);
    } while (isspace(c));

    if ((*plast++ = c) == '~')
        return &str_empty[0];

    for (;;)
    {
        /*
         * Back off the char type lookup,
         *   it was too dirty for portability.
         *   -- Furey
         */

        switch (*plast = getc(fp))
        {
            default:
                plast++;
                break;

            case EOF:
                /* temp fix */
                bug("Fread_string: EOF", 0);
                return NULL;
                /* exit( 1 ); */
                break;

            case '\n':
                plast++;
                *plast++ = '\r';
                break;

            case '\r':
                break;

            case '~':
                plast++;
                {
                    union {
                        char *pc;
                        char rgc[sizeof(char *)];
                    } u1;
                    int ic;
                    int iHash;
                    char *pHash;
                    char *pHashPrev;
                    char *pString;

                    plast[-1] = '\0';
                    iHash = UMIN(MAX_KEY_HASH - 1, plast - 1 - top_string);
                    for (pHash = string_hash[iHash]; pHash; pHash = pHashPrev)
                    {
                        for (ic = 0; ic < sizeof(char *); ic++)
                            u1.rgc[ic] = pHash[ic];
                        pHashPrev = u1.pc;
                        pHash += sizeof(char *);

                        if (top_string[sizeof(char *)] == pHash[0]
                            && !strcmp(top_string + sizeof(char *) + 1,
                                pHash + 1))
                            return pHash;
                    }

                    if (fBootDb)
                    {
                        pString = top_string;
                        top_string = plast;
                        u1.pc = string_hash[iHash];
                        for (ic = 0; ic < sizeof(char *); ic++)
                            pString[ic] = u1.rgc[ic];
                        string_hash[iHash] = pString;

                        nAllocString += 1;
                        sAllocString += top_string - pString;
                        return pString + sizeof(char *);
                    }
                    else
                    {
                        return str_dup(top_string + sizeof(char *));
                    }
                }
        }
    }
}

char *fread_string_eol(FILE * fp)
{
    static bool char_special[256 - EOF];
    char *plast;
    char c;

    if (char_special[EOF - EOF] != TRUE)
    {
        char_special[EOF - EOF] = TRUE;
        char_special['\n' - EOF] = TRUE;
        char_special['\r' - EOF] = TRUE;
    }

    plast = top_string + sizeof(char *);
    if (plast > &string_space[MAX_STRING - MAX_STRING_LENGTH])
    {
        bug("Fread_string: MAX_STRING %d exceeded.", MAX_STRING);
        exit(1);
    }

    /*
     * Skip blanks.
     * Read first char.
     */
    do
    {
        c = getc(fp);
    } while (isspace(c));

    if ((*plast++ = c) == '\n')
        return &str_empty[0];

    for (;;)
    {
        if (!char_special[(*plast++ = getc(fp)) - EOF])
            continue;

        switch (plast[-1])
        {
            default:
                break;

            case EOF:
                bug("Fread_string_eol  EOF", 0);
                exit(1);
                break;

            case '\n':
            case '\r':
            {
                union {
                    char *pc;
                    char rgc[sizeof(char *)];
                } u1;
                int ic;
                int iHash;
                char *pHash;
                char *pHashPrev;
                char *pString;

                plast[-1] = '\0';
                iHash = UMIN(MAX_KEY_HASH - 1, plast - 1 - top_string);
                for (pHash = string_hash[iHash]; pHash; pHash = pHashPrev)
                {
                    for (ic = 0; ic < sizeof(char *); ic++)
                        u1.rgc[ic] = pHash[ic];
                    pHashPrev = u1.pc;
                    pHash += sizeof(char *);

                    if (top_string[sizeof(char *)] == pHash[0]
                        && !strcmp(top_string + sizeof(char *) + 1,
                            pHash + 1))
                        return pHash;
                }

                if (fBootDb)
                {
                    pString = top_string;
                    top_string = plast;
                    u1.pc = string_hash[iHash];
                    for (ic = 0; ic < sizeof(char *); ic++)
                        pString[ic] = u1.rgc[ic];
                    string_hash[iHash] = pString;

                    nAllocString += 1;
                    sAllocString += top_string - pString;
                    return pString + sizeof(char *);
                }
                else
                {
                    return str_dup(top_string + sizeof(char *));
                }
            }
        }
    }
}

/*
 * Read to end of line (for comments).
 */
void fread_to_eol(FILE * fp)
{
    char c;

    do
    {
        c = getc(fp);
    } while (c != '\n' && c != '\r');

    do
    {
        c = getc(fp);
    } while (c == '\n' || c == '\r');

    ungetc(c, fp);
    return;
}

/*
 * Read one word (into static buffer).
 */
char *fread_word(FILE * fp)
{
    static char word[MAX_INPUT_LENGTH];
    char *pword;
    char cEnd;

    do
    {
        cEnd = getc(fp);
    } while (isspace(cEnd));

    if (cEnd == '\'' || cEnd == '"')
    {
        pword = word;
    }
    else
    {
        word[0] = cEnd;
        pword = word + 1;
        cEnd = ' ';
    }

    for (; pword < word + MAX_INPUT_LENGTH; pword++)
    {
        *pword = getc(fp);
        if (cEnd == ' ' ? isspace(*pword) : *pword == cEnd)
        {
            if (cEnd == ' ')
                ungetc(*pword, fp);
            *pword = '\0';
            return word;
        }
    }

    bug("Fread_word: word too long.", 0);
    exit(1);
    return NULL;
}

/*
 * Allocate some ordinary memory,
 *   with the expectation of freeing it someday.
 */
void *alloc_mem(size_t sMem)
{
    void *pMem;
    size_t *magic;
    int iList;

    sMem += sizeof(*magic);

    for (iList = 0; iList < MAX_MEM_LIST; iList++)
    {
        if (sMem <= rgSizeList[iList])
            break;
    }

    if (iList == MAX_MEM_LIST)
    {
        bug("Alloc_mem: size %d too large.", sMem);
        exit(1);
    }

    if (rgFreeList[iList] == NULL)
    {
        pMem = alloc_perm(rgSizeList[iList]);
    }
    else
    {
        pMem = rgFreeList[iList];
        rgFreeList[iList] = *((void **)rgFreeList[iList]);
    }

#if !defined(_WIN32)
    magic = (size_t *)pMem;
    *magic = MAGIC_NUM;
    pMem = (void *)((size_t)pMem + (size_t)(sizeof(*magic)));
#endif

    return pMem;
}

/*
 * Free some memory.
 * Recycle it back onto the free list for blocks of that size.
 */
void free_mem(void *pMem, size_t sMem)
{
    int iList;

#if !defined(_WIN32)
    size_t *magic;

    pMem = (void *)((size_t)pMem - (size_t) sizeof(*magic));
    magic = (size_t *)pMem;

    if (*magic != MAGIC_NUM)
    {
        bug("Attempt to recyle invalid memory of size %d.", sMem);
        bug((char *)pMem + sizeof(*magic), 0);
        return;
    }

    *magic = 0;
    sMem += sizeof(*magic);
#endif

    for (iList = 0; iList < MAX_MEM_LIST; iList++)
    {
        if (sMem <= rgSizeList[iList])
            break;
    }

    if (iList == MAX_MEM_LIST)
    {
        bug("Free_mem: size %d too large.", sMem);
        exit(1);
    }

    *((void **)pMem) = rgFreeList[iList];
    rgFreeList[iList] = pMem;

    return;
}

/*
 * Allocate some permanent memory.
 * Permanent memory is never freed,
 *   pointers into it may be copied safely.
 */
void *alloc_perm(size_t sMem)
{
    static char *pMemPerm;
    static size_t iMemPerm;
    void *pMem;

    while (sMem % sizeof(long) != 0)
        sMem++;
    if (sMem > MAX_PERM_BLOCK)
    {
        bug("Alloc_perm: %d too large.", sMem);
        exit(1);
    }

    if (pMemPerm == NULL || iMemPerm + sMem > MAX_PERM_BLOCK)
    {
        iMemPerm = 0;
        if ((pMemPerm = (char *)calloc(1, MAX_PERM_BLOCK)) == NULL)
        {
            perror("Alloc_perm");
            exit(1);
        }
    }

    pMem = pMemPerm + iMemPerm;
    iMemPerm += sMem;
    nAllocPerm += 1;
    sAllocPerm += sMem;
    return pMem;
}

/*
 * Duplicate a string into dynamic memory.
 * Fread_strings are read-only and shared.
 */
char *str_dup(const char *str)
{
    char *str_new;

    if (str == NULL)
        return &str_empty[0];

    if (str[0] == '\0')
        return &str_empty[0];

    if (str >= string_space && str < top_string)
        return (char *)str;

    str_new = alloc_mem(strlen(str) + 1);
    strcpy(str_new, str);
    return str_new;
}

/*
 * Free a string.
 * Null is legal here to simplify callers.
 * Read-only shared strings are not touched.
 */
void free_string(char *pstr)
{
    if (pstr == NULL || pstr == &str_empty[0]
        || (pstr >= string_space && pstr < top_string))
        return;

    free_mem(pstr, strlen(pstr) + 1);
    return;
}

/*
 * List the areas in the game with the credits.  The area command by default will show you
 * all of the areas.  If you provide the optional 'recommend' flag it will only show you
 * areas in your level range.
 */
void do_areas(CHAR_DATA * ch, char *argument)
{
    if (IS_NPC(ch))
        return;

    BUFFER *output;
    char buf[MAX_STRING_LENGTH];
    char border[MAX_STRING_LENGTH];
    AREA_DATA *pArea;
    int continent;
    bool recommend = FALSE;

    if (!IS_NULLSTR(argument) && !str_cmp(argument, "recommend"))
    {
        recommend = TRUE;
    }

    output = new_buf();

    sprintf(border, "---------------------------------------------------------------------------\r\n");

    for (continent = 0; continent_table[continent].name != NULL; continent++)
    {
        // Send the continent
        if (!recommend)
        {
            // All levels
            sprintf(buf, "{CContinent: %s (%d Areas){x\r\n", capitalize(continent_table[continent].name), area_count(continent, -1, FALSE));
        }
        else
        {
            // Recommended only the players levels shown.
            sprintf(buf, "{CContinent: %s (%d Areas){x\r\n", capitalize(continent_table[continent].name), area_count(continent, ch->level, TRUE));
        }

        add_buf(output, buf);
        add_buf(output, border);

        // Send the header
        sprintf(buf, "[{G%-5s{x][{W%-39s{x][{C%-25s{x]\r\n", "Level", "Area Name", "Builders");
        add_buf(output, buf);
        add_buf(output, border);

        // Show the areas for the current continent, skip the rest, we'll hit them on a later iteration.
        for (pArea = area_first; pArea; pArea = pArea->next)
        {
            if (pArea->continent != continent)
                continue;

            if (!recommend)
            {
                // This is the general area command with now parameters, it will show you all of the
                // areas.
                sprintf(buf, "[{G%2d %2d{x] {W%-39.39s{x [{C%-25.25s{x]\r\n",
                    pArea->min_level, pArea->max_level, pArea->name, pArea->builders);
                add_buf(output, buf);
            }
            else if (recommend && (ch->level >= pArea->min_level && ch->level <= pArea->max_level) && (pArea->max_level - pArea->min_level < 30))
            {
                // This is the area command witht he 'recommend' flag.  It will show you only areas
                // that fall within your level range and don't have a huge range (like level 1 to level 51)
                sprintf(buf, "[{G%2d %2d{x] {W%-39.39s{x [{C%-25.25s{x]\r\n",
                    pArea->min_level, pArea->max_level, pArea->name, pArea->builders);
                add_buf(output, buf);
            }

        }

        // Bottom Border
        add_buf(output, border);
        sprintf(buf, "\r\n");
        add_buf(output, buf);

    }

    if (!recommend)
    {
        sprintf(buf, "{CTotal Areas: %d (Use 'area recommend' to see areas in your level range){x\r\n\r\n", area_count(-1, -1, FALSE));
    }
    else
    {
        sprintf(buf, "{CTotal Recommend Areas: %d{x\r\n\r\n", area_count(-1, ch->level, TRUE));
    }

    add_buf(output, buf);

    page_to_char(buf_string(output), ch);
    free_buf(output);
} // end void do_areas

/*
 * Returns the number of areas on a specified continent.  If -1 is supplied for
 * the continent number then all areas will be counted.  The level will indicate
 * whether the area falls in the level range.  If it is -1 it will be ignored.  The
 * recommend bool can be used to filter down the level ranges further to get rid of
 * areas that might be 1-51.
 */
int area_count(int continent, int level, bool recommend)
{
    AREA_DATA *pArea;
    int counter = 0;

    for (pArea = area_first; pArea; pArea = pArea->next)
    {
        if (pArea == NULL)
            break;

        if (level == -1)
        {
            if (pArea->continent == continent || continent == -1)
            {
                counter++;
            }
        }
        else
        {
            if (pArea->continent == continent || continent == -1)
            {
                if ((pArea->min_level <= level && pArea->max_level >= level) && (recommend && pArea->max_level - pArea->min_level < 30))
                {
                    counter++;
                }
            }
        }
    }

    return counter;

} // end area_count

/*
 * Shows some basic memory usage.  This really should be updated because it doesn't show
 * updated counts on a lot of the values for changes in memory (it just shows at boot time
 * where static ints were kept).  Showing the boot stats along with current values would
 * be more helpful. (TODO item).
 */
void do_memory(CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    OBJ_DATA * obj_pit_item;
    OBJ_DATA * obj_pit_item_next;
    int base_objects_purged = 0;
    int all_objects_purged = 0;
    int pits_purged = 0;
    int obj_count = 0;
    int total_obj_count = 0;

    // Count all the objects in memory, and then get their additional counts if
    // they've been consolidated.
    for (obj = object_list; obj != NULL; obj = obj_next)
    {
        obj_next = obj->next;
        obj_count++;
        total_obj_count += obj->count;
    }

    sprintf(buf, "Affects %5d\r\n", top_affect);
    send_to_char(buf, ch);
    sprintf(buf, "Areas   %5d\r\n", top_area);
    send_to_char(buf, ch);
    sprintf(buf, "ExDes   %5d\r\n", top_ed);
    send_to_char(buf, ch);
    sprintf(buf, "Exits   %5d\r\n", top_exit);
    send_to_char(buf, ch);
    sprintf(buf, "Helps   %5d\r\n", top_help);
    send_to_char(buf, ch);
    sprintf(buf, "Socials %5d\r\n", social_count);
    send_to_char(buf, ch);
    sprintf(buf, "Mobs    %5d (%d in use)\r\n", top_mob_index, mobile_count);
    send_to_char(buf, ch);
    sprintf(buf, "Objs    %5d (%d loaded with unconsolidated count of %d)\r\n", top_obj_index, obj_count, total_obj_count);
    send_to_char(buf, ch);
    sprintf(buf, "Resets  %5d\r\n", top_reset);
    send_to_char(buf, ch);
    sprintf(buf, "Rooms   %5d\r\n", top_room);
    send_to_char(buf, ch);
    sprintf(buf, "Shops   %5d\r\n", top_shop);
    send_to_char(buf, ch);

    sprintf(buf, "Strings %5d strings of %7zd bytes (max %d).\r\n",
        nAllocString, sAllocString, MAX_STRING);
    send_to_char(buf, ch);

    sprintf(buf, "Perms   %5d blocks  of %7zd bytes.\r\n",
        nAllocPerm, sAllocPerm);
    send_to_char(buf, ch);

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
            }

            pits_purged++;
        }
    }

    sprintf(buf, "Pits    %5d (%d objects in the pits, %d after consolidation)\r\n", pits_purged, all_objects_purged, base_objects_purged);
    send_to_char(buf, ch);

#if !defined(_WIN32)
    // Show the processes memory and cpu usage as per the ps command in POSIX.  I'm making
    // the assumption that anything not _WIN32 is probably POSIX (maybe that's a bad assumption
    // if it doesn't work for you, fix it and put a pull request in).  It should at least gracefully
    // fail if it does.. if ps isn't found or it doesn't return a result it will simply output nothing.
    FILE *fp;
    int line_count = 0;

    // Open the command for reading, assumes 'game' is the name of the executable to grep for
    fp = popen("ps aux|head -n 1&&ps aux|grep game|grep -v grep", "r");

    if (fp == NULL)
    {
        send_to_char("Error executing system call for this processes memory.\r\n", ch);
    }
    else
    {
        send_to_char("\r\n", ch);

        // Read the output a line at a time and send it to the caller.
        while (fgets(buf, sizeof(buf) - 1, fp) != NULL)
        {
            sendf(ch, "%s", buf);

            // This is a little hacky, but the input from the system call only has a newline
            // where some clients need a newline and a carriage return in order to consistently
            // space the lines correctly.
            if (line_count == 0)
            {
                line_count++;
                send_to_char("\r", ch);
            }
        }
    }

    // Close the file handle
    pclose(fp);
#endif

    return;
}

void do_dump(CHAR_DATA * ch, char *argument)
{
    int count, count2, num_pcs, aff_count;
    CHAR_DATA *fch;
    MOB_INDEX_DATA *pMobIndex;
    PC_DATA *pc;
    OBJ_DATA *obj;
    OBJ_INDEX_DATA *pObjIndex;
    ROOM_INDEX_DATA *room;
    EXIT_DATA *exit;
    DESCRIPTOR_DATA *d;
    AFFECT_DATA *af;
    FILE *fp;
    int vnum, nMatch = 0;

    /* open file */
    fclose(fpReserve);
    fp = fopen("mem.dmp", "w");

    /* report use of data structures */

    num_pcs = 0;
    aff_count = 0;

    /* mobile prototypes */
    fprintf(fp, "MobProt    %4d (%8ld bytes)\n",
        top_mob_index, top_mob_index * (sizeof(*pMobIndex)));

/* mobs */
    count = 0;
    count2 = 0;
    for (fch = char_list; fch != NULL; fch = fch->next)
    {
        count++;
        if (fch->pcdata != NULL)
            num_pcs++;
        for (af = fch->affected; af != NULL; af = af->next)
            aff_count++;
    }
    for (fch = char_free; fch != NULL; fch = fch->next)
        count2++;

    fprintf(fp, "Mobs    %4d (%8ld bytes), %2d free (%ld bytes)\n",
        count, count * (sizeof(*fch)), count2,
        count2 * (sizeof(*fch)));

/* pcdata */
    count = 0;
    for (pc = pcdata_free; pc != NULL; pc = pc->next)
        count++;

    fprintf(fp, "Pcdata    %4d (%8ld bytes), %2d free (%ld bytes)\n",
        num_pcs, num_pcs * (sizeof(*pc)), count,
        count * (sizeof(*pc)));

/* descriptors */
    count = 0;
    count2 = 0;
    for (d = descriptor_list; d != NULL; d = d->next)
        count++;
    for (d = descriptor_free; d != NULL; d = d->next)
        count2++;

    fprintf(fp, "Descs    %4d (%8ld bytes), %2d free (%ld bytes)\n",
        count, count * (sizeof(*d)), count2, count2 * (sizeof(*d)));

/* object prototypes */
    for (vnum = 0; nMatch < top_obj_index; vnum++)
        if ((pObjIndex = get_obj_index(vnum)) != NULL)
        {
            for (af = pObjIndex->affected; af != NULL; af = af->next)
                aff_count++;
            nMatch++;
        }

    fprintf(fp, "ObjProt    %4d (%8ld bytes)\n",
        top_obj_index, top_obj_index * (sizeof(*pObjIndex)));


/* objects */
    count = 0;
    count2 = 0;
    for (obj = object_list; obj != NULL; obj = obj->next)
    {
        count++;
        for (af = obj->affected; af != NULL; af = af->next)
            aff_count++;
    }
    for (obj = obj_free; obj != NULL; obj = obj->next)
        count2++;

    fprintf(fp, "Objs    %4d (%8ld bytes), %2d free (%ld bytes)\n",
        count, count * (sizeof(*obj)), count2,
        count2 * (sizeof(*obj)));

/* affects */
    count = 0;
    for (af = affect_free; af != NULL; af = af->next)
        count++;

    fprintf(fp, "Affects    %4d (%8ld bytes), %2d free (%ld bytes)\n",
        aff_count, aff_count * (sizeof(*af)), count,
        count * (sizeof(*af)));

/* rooms */
    fprintf(fp, "Rooms    %4d (%8ld bytes)\n",
        top_room, top_room * (sizeof(*room)));

/* exits */
    fprintf(fp, "Exits    %4d (%8ld bytes)\n",
        top_exit, top_exit * (sizeof(*exit)));

    fclose(fp);

    /* start printing out mobile data */
    fp = fopen("mob.dmp", "w");

    fprintf(fp, "\nMobile Analysis\n");
    fprintf(fp, "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_mob_index; vnum++)
        if ((pMobIndex = get_mob_index(vnum)) != NULL)
        {
            nMatch++;
            fprintf(fp, "#%-4d %3d active %3d killed     %s\n",
                pMobIndex->vnum, pMobIndex->count,
                pMobIndex->killed, pMobIndex->short_descr);
        }
    fclose(fp);

    /* start printing out object data */
    fp = fopen("obj.dmp", "w");

    fprintf(fp, "\nObject Analysis\n");
    fprintf(fp, "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_obj_index; vnum++)
        if ((pObjIndex = get_obj_index(vnum)) != NULL)
        {
            nMatch++;
            fprintf(fp, "#%-4d %3d active %3d reset      %s\n",
                pObjIndex->vnum, pObjIndex->count,
                pObjIndex->reset_num, pObjIndex->short_descr);
        }

    /* close file */
    fclose(fp);
    fpReserve = fopen(NULL_FILE, "r");
}

/*
 * Simple linear interpolation.
 */
int interpolate(int level, int value_00, int value_32)
{
    return value_00 + level * (value_32 - value_00) / 32;
}

/*
 * Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 */
void smash_tilde(char *str)
{
    for (; *str != '\0'; str++)
    {
        if (*str == '~')
            *str = '-';
    }

    return;
}

/*
 * Removes dollar signs to keep snerts from crashing us.
 * Posted to ROM list by Kyndig. JR -- 10/15/00
 */
void smash_dollar(char *str)
{
    for (; *str != '\0'; str++)
    {
        if (*str == '$')
            *str = 'S';
    }
    return;
}

/*
 * Compare strings, case insensitive.
 * Return TRUE if different
 *   (compatibility with historical functions).
 */
bool str_cmp(const char *astr, const char *bstr)
{
    if (astr == NULL)
    {
        bug("Str_cmp: null astr.", 0);
        return TRUE;
    }

    if (bstr == NULL)
    {
        bug("Str_cmp: null bstr.", 0);
        return TRUE;
    }

    for (; *astr || *bstr; astr++, bstr++)
    {
        if (LOWER(*astr) != LOWER(*bstr))
            return TRUE;
    }

    return FALSE;
}

/*
 * Compare strings, case insensitive, for prefix matching.
 * Return TRUE if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool str_prefix(const char *astr, const char *bstr)
{
    if (astr == NULL)
    {
        bug("Strn_cmp: null astr.", 0);
        return TRUE;
    }

    if (bstr == NULL)
    {
        bug("Strn_cmp: null bstr.", 0);
        return TRUE;
    }

    for (; *astr; astr++, bstr++)
    {
        if (LOWER(*astr) != LOWER(*bstr))
            return TRUE;
    }

    return FALSE;
}

/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns TRUE is astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool str_infix(const char *astr, const char *bstr)
{
    int sstr1;
    int sstr2;
    int ichar;
    char c0;

    if ((c0 = LOWER(astr[0])) == '\0')
        return FALSE;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);

    for (ichar = 0; ichar <= sstr2 - sstr1; ichar++)
    {
        if (c0 == LOWER(bstr[ichar]) && !str_prefix(astr, bstr + ichar))
            return FALSE;
    }

    return TRUE;
}

/*
 * Compare strings, case insensitive, for suffix matching.
 * Return TRUE if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool str_suffix(const char *astr, const char *bstr)
{
    int sstr1;
    int sstr2;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);
    if (sstr1 <= sstr2 && !str_cmp(astr, bstr + sstr2 - sstr1))
        return FALSE;
    else
        return TRUE;
}

/*
 * Returns an initial-capped string.
 */
char *capitalize(const char *str)
{
    static char strcap[MAX_STRING_LENGTH];
    int i;

    for (i = 0; str[i] != '\0'; i++)
        strcap[i] = LOWER(str[i]);
    strcap[i] = '\0';
    strcap[0] = UPPER(strcap[0]);
    return strcap;
}

/*
 * Append a string to a file.
 */
void append_file(CHAR_DATA * ch, char *file, char *str)
{
    FILE *fp;

    if (IS_NPC(ch) || str[0] == '\0')
        return;

    fclose(fpReserve);
    if ((fp = fopen(file, "a")) == NULL)
    {
        perror(file);
        send_to_char("Could not open the file!\r\n", ch);
    }
    else
    {
        fprintf(fp, "[%5d] %s: %s\n",
            ch->in_room ? ch->in_room->vnum : 0, ch->name, str);
        fclose(fp);
    }

    fpReserve = fopen(NULL_FILE, "r");
    return;
}

/*
 * This function is here to aid in debugging.
 * If the last expression in a function is another function call,
 *   gcc likes to generate a JMP instead of a CALL.
 * This is called "tail chaining."
 * It hoses the debugger call stack for that call.
 * So I make this the last call in certain critical functions,
 *   where I really need the call stack to be right for debugging!
 *
 * If you don't understand this, then LEAVE IT ALONE.
 * Don't remove any calls to tail_chain anywhere.
 *
 * -- Furey
 */
void tail_chain(void)
{
    return;
}

/* Added this as part of a bugfix suggested by Chris Litchfield (of
 * "The Mage's Lair" fame). Pets were being loaded with any saved
 * affects, then having those affects loaded again. -- JR 2002/01/31
 */
bool check_pet_affected(int vnum, AFFECT_DATA *paf)
{
    MOB_INDEX_DATA *petIndex;

    petIndex = get_mob_index(vnum);
    if (petIndex == NULL)
        return FALSE;

    if (paf->where == TO_AFFECTS)
        if (IS_AFFECTED(petIndex, paf->bitvector))
            return TRUE;

    return FALSE;
}

extern int flag_lookup(const char *name, const struct flag_type * flag_table);

/* values for db2.c */
struct social_type social_table[MAX_SOCIALS];
int social_count;

/* snarf a socials file */
void load_socials(FILE * fp)
{
    for (;;)
    {
        struct social_type social;
        char *temp;
        /* clear social */
        social.char_no_arg = NULL;
        social.others_no_arg = NULL;
        social.char_found = NULL;
        social.others_found = NULL;
        social.vict_found = NULL;
        social.char_not_found = NULL;
        social.char_auto = NULL;
        social.others_auto = NULL;

        temp = fread_word(fp);
        if (!strcmp(temp, "#0"))
            return;                /* done */
#if defined(social_debug)
        else
            fprintf(stderr, "%s\r\n", temp);
#endif

        strcpy(social.name, temp);
        fread_to_eol(fp);

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_no_arg = NULL;
        else if (!strcmp(temp, "#"))
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_no_arg = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.others_no_arg = NULL;
        else if (!strcmp(temp, "#"))
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.others_no_arg = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_found = NULL;
        else if (!strcmp(temp, "#"))
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.others_found = NULL;
        else if (!strcmp(temp, "#"))
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.others_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.vict_found = NULL;
        else if (!strcmp(temp, "#"))
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.vict_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_not_found = NULL;
        else if (!strcmp(temp, "#"))
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_not_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_auto = NULL;
        else if (!strcmp(temp, "#"))
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.char_auto = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.others_auto = NULL;
        else if (!strcmp(temp, "#"))
        {
            social_table[social_count] = social;
            social_count++;
            continue;
        }
        else
            social.others_auto = temp;

        social_table[social_count] = social;
        social_count++;
    }
    return;
}

/*
* Snarf a mob section.  new style
*/
void load_mobiles(FILE * fp)
{
    MOB_INDEX_DATA *pMobIndex;

    if (!area_last)
    {                            /* OLC */
        bug("Load_mobiles: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;)
    {
        int vnum;
        char letter;
        int iHash;

        letter = fread_letter(fp);
        if (letter != '#')
        {
            bug("Load_mobiles: # not found.", 0);
            exit(1);
        }

        vnum = fread_number(fp);
        if (vnum == 0)
            break;

        fBootDb = FALSE;
        if (get_mob_index(vnum) != NULL)
        {
            bug("Load_mobiles: vnum %d duplicated.", vnum);
            exit(1);
        }
        fBootDb = TRUE;

        pMobIndex = alloc_perm(sizeof(*pMobIndex));
        pMobIndex->vnum = vnum;
        pMobIndex->area = area_last;    /* OLC */
        pMobIndex->player_name = fread_string(fp);
        pMobIndex->short_descr = fread_string(fp);
        pMobIndex->long_descr = fread_string(fp);
        pMobIndex->description = fread_string(fp);
        pMobIndex->race = race_lookup(fread_string(fp));

        pMobIndex->long_descr[0] = UPPER(pMobIndex->long_descr[0]);
        pMobIndex->description[0] = UPPER(pMobIndex->description[0]);

        pMobIndex->act = fread_flag(fp) | ACT_IS_NPC
            | race_table[pMobIndex->race].act;
        pMobIndex->affected_by = fread_flag(fp)
            | race_table[pMobIndex->race].aff;
        pMobIndex->pShop = NULL;
        pMobIndex->alignment = fread_number(fp);

        // Alignment sanity check and convert.  Since we deviated from ROM's alignments this check
        // wil be necessary in the short term.
        if (pMobIndex->alignment < 1 || pMobIndex->alignment > 3)
        {
            bugf("%s with alignment of %d - converting but will need to be fixed properly.", pMobIndex->player_name, pMobIndex->alignment);

            if (pMobIndex->alignment < -350)
            {
                pMobIndex->alignment = ALIGN_EVIL;
            }
            else if (pMobIndex->alignment > 350)
            {
                pMobIndex->alignment = ALIGN_GOOD;
            }
            else
            {
                pMobIndex->alignment = ALIGN_NEUTRAL;
            }
        }

        pMobIndex->group = fread_number(fp);

        pMobIndex->level = fread_number(fp);
        pMobIndex->hitroll = fread_number(fp);

        /* read hit dice */
        pMobIndex->hit[DICE_NUMBER] = fread_number(fp);
        /* 'd'          */ fread_letter(fp);
        pMobIndex->hit[DICE_TYPE] = fread_number(fp);
        /* '+'          */ fread_letter(fp);
        pMobIndex->hit[DICE_BONUS] = fread_number(fp);

        /* read mana dice */
        pMobIndex->mana[DICE_NUMBER] = fread_number(fp);
        fread_letter(fp);
        pMobIndex->mana[DICE_TYPE] = fread_number(fp);
        fread_letter(fp);
        pMobIndex->mana[DICE_BONUS] = fread_number(fp);

        /* read damage dice */
        pMobIndex->damage[DICE_NUMBER] = fread_number(fp);
        fread_letter(fp);
        pMobIndex->damage[DICE_TYPE] = fread_number(fp);
        fread_letter(fp);
        pMobIndex->damage[DICE_BONUS] = fread_number(fp);
        pMobIndex->dam_type = attack_lookup(fread_word(fp));

        /* read armor class */
        pMobIndex->ac[AC_PIERCE] = fread_number(fp) * 10;
        pMobIndex->ac[AC_BASH] = fread_number(fp) * 10;
        pMobIndex->ac[AC_SLASH] = fread_number(fp) * 10;
        pMobIndex->ac[AC_EXOTIC] = fread_number(fp) * 10;

        /* read flags and add in data from the race table */
        pMobIndex->off_flags = fread_flag(fp)
            | race_table[pMobIndex->race].off;
        pMobIndex->imm_flags = fread_flag(fp)
            | race_table[pMobIndex->race].imm;
        pMobIndex->res_flags = fread_flag(fp)
            | race_table[pMobIndex->race].res;
        pMobIndex->vuln_flags = fread_flag(fp)
            | race_table[pMobIndex->race].vuln;

        /* vital statistics */
        pMobIndex->start_pos = position_lookup(fread_word(fp));
        pMobIndex->default_pos = position_lookup(fread_word(fp));
        pMobIndex->sex = sex_lookup(fread_word(fp));

        pMobIndex->wealth = fread_number(fp);

        pMobIndex->form = fread_flag(fp) | race_table[pMobIndex->race].form;
        pMobIndex->parts = fread_flag(fp)
            | race_table[pMobIndex->race].parts;
        /* size */
        CHECK_POS(pMobIndex->size, size_lookup(fread_word(fp)), "size");
        /*    pMobIndex->size            = size_lookup(fread_word(fp)); */
        pMobIndex->material = str_dup(fread_word(fp));

        for (;;)
        {
            letter = fread_letter(fp);

            if (letter == 'F')
            {
                char *word;
                long vector;

                word = fread_word(fp);
                vector = fread_flag(fp);

                if (!str_prefix(word, "act"))
                    REMOVE_BIT(pMobIndex->act, vector);
                else if (!str_prefix(word, "aff"))
                    REMOVE_BIT(pMobIndex->affected_by, vector);
                else if (!str_prefix(word, "off"))
                    REMOVE_BIT(pMobIndex->off_flags, vector);
                else if (!str_prefix(word, "imm"))
                    REMOVE_BIT(pMobIndex->imm_flags, vector);
                else if (!str_prefix(word, "res"))
                    REMOVE_BIT(pMobIndex->res_flags, vector);
                else if (!str_prefix(word, "vul"))
                    REMOVE_BIT(pMobIndex->vuln_flags, vector);
                else if (!str_prefix(word, "for"))
                    REMOVE_BIT(pMobIndex->form, vector);
                else if (!str_prefix(word, "par"))
                    REMOVE_BIT(pMobIndex->parts, vector);
                else
                {
                    bug("Flag remove: flag not found.", 0);
                    exit(1);
                }
            }
            else if (letter == 'M')
            {
                MPROG_LIST *pMprog;
                char *word;
                int trigger = 0;

                pMprog = alloc_perm(sizeof(*pMprog));
                word = fread_word(fp);
                if ((trigger = flag_lookup(word, mprog_flags)) == NO_FLAG)
                {
                    bug("MOBprogs: invalid trigger.", 0);
                    exit(1);
                }
                SET_BIT(pMobIndex->mprog_flags, trigger);
                pMprog->trig_type = trigger;
                pMprog->vnum = fread_number(fp);
                pMprog->trig_phrase = fread_string(fp);
                pMprog->next = pMobIndex->mprogs;
                pMobIndex->mprogs = pMprog;
            }
            else
            {
                ungetc(letter, fp);
                break;
            }
        }

        iHash = vnum % MAX_KEY_HASH;
        pMobIndex->next = mob_index_hash[iHash];
        mob_index_hash[iHash] = pMobIndex;
        top_mob_index++;
        top_vnum_mob = top_vnum_mob < vnum ? vnum : top_vnum_mob;    /* OLC */
        assign_area_vnum(vnum);    /* OLC */
    }

    return;
}

/*
* Snarf an obj section. new style
*/
void load_objects(FILE * fp)
{
    OBJ_INDEX_DATA *pObjIndex;

    if (!area_last)
    {                            /* OLC */
        bug("Load_objects: no #AREA seen yet.", 0);
        exit(1);
    }

    for (;;)
    {
        int vnum;
        char letter;
        int iHash;

        letter = fread_letter(fp);
        if (letter != '#')
        {
            bug("Load_objects: # not found.", 0);
            exit(1);
        }

        vnum = fread_number(fp);
        if (vnum == 0)
            break;

        fBootDb = FALSE;
        if (get_obj_index(vnum) != NULL)
        {
            bug("Load_objects: vnum %d duplicated.", vnum);
            exit(1);
        }
        fBootDb = TRUE;

        pObjIndex = alloc_perm(sizeof(*pObjIndex));
        pObjIndex->vnum = vnum;
        pObjIndex->area = area_last;    /* OLC */
        pObjIndex->reset_num = 0;
        pObjIndex->name = fread_string(fp);
        pObjIndex->short_descr = fread_string(fp);
        pObjIndex->description = fread_string(fp);
        pObjIndex->material = fread_string(fp);

        CHECK_POS(pObjIndex->item_type, item_lookup(fread_word(fp)),
            "item_type");
        pObjIndex->extra_flags = fread_flag(fp);
        pObjIndex->wear_flags = fread_flag(fp);
        switch (pObjIndex->item_type)
        {
            case ITEM_WEAPON:
                pObjIndex->value[0] = weapon_type_lookup(fread_word(fp));
                pObjIndex->value[1] = fread_number(fp);
                pObjIndex->value[2] = fread_number(fp);
                pObjIndex->value[3] = attack_lookup(fread_word(fp));
                pObjIndex->value[4] = fread_flag(fp);
                break;
            case ITEM_CONTAINER:
                pObjIndex->value[0] = fread_number(fp);
                pObjIndex->value[1] = fread_flag(fp);
                pObjIndex->value[2] = fread_number(fp);
                pObjIndex->value[3] = fread_number(fp);
                pObjIndex->value[4] = fread_number(fp);
                break;
            case ITEM_DRINK_CON:
            case ITEM_FOUNTAIN:
                pObjIndex->value[0] = fread_number(fp);
                pObjIndex->value[1] = fread_number(fp);
                CHECK_POS(pObjIndex->value[2], liq_lookup(fread_word(fp)),
                    "liq_lookup");
                pObjIndex->value[3] = fread_number(fp);
                pObjIndex->value[4] = fread_number(fp);
                break;
            case ITEM_WAND:
            case ITEM_STAFF:
                pObjIndex->value[0] = fread_number(fp);
                pObjIndex->value[1] = fread_number(fp);
                pObjIndex->value[2] = fread_number(fp);
                pObjIndex->value[3] = skill_lookup(fread_word(fp));
                pObjIndex->value[4] = fread_number(fp);
                break;
            case ITEM_POTION:
            case ITEM_PILL:
            case ITEM_SCROLL:
                pObjIndex->value[0] = fread_number(fp);
                pObjIndex->value[1] = skill_lookup(fread_word(fp));
                pObjIndex->value[2] = skill_lookup(fread_word(fp));
                pObjIndex->value[3] = skill_lookup(fread_word(fp));
                pObjIndex->value[4] = skill_lookup(fread_word(fp));
                break;
            default:
                pObjIndex->value[0] = fread_flag(fp);
                pObjIndex->value[1] = fread_flag(fp);
                pObjIndex->value[2] = fread_flag(fp);
                pObjIndex->value[3] = fread_flag(fp);
                pObjIndex->value[4] = fread_flag(fp);
                break;
        }
        pObjIndex->level = fread_number(fp);
        pObjIndex->weight = fread_number(fp);
        pObjIndex->cost = fread_number(fp);

        /* condition */
        letter = fread_letter(fp);
        switch (letter)
        {
            case ('P') :
                pObjIndex->condition = 100;
                break;
            case ('G') :
                pObjIndex->condition = 90;
                break;
            case ('A') :
                pObjIndex->condition = 75;
                break;
            case ('W') :
                pObjIndex->condition = 50;
                break;
            case ('D') :
                pObjIndex->condition = 25;
                break;
            case ('B') :
                pObjIndex->condition = 10;
                break;
            case ('R') :
                pObjIndex->condition = 0;
                break;
            default:
                pObjIndex->condition = 100;
                break;
        }

        for (;;)
        {
            char letter;

            letter = fread_letter(fp);

            if (letter == 'A')
            {
                AFFECT_DATA *paf;

                paf = alloc_perm(sizeof(*paf));
                paf->where = TO_OBJECT;
                paf->type = -1;
                paf->level = pObjIndex->level;
                paf->duration = -1;
                paf->location = fread_number(fp);
                paf->modifier = fread_number(fp);
                paf->bitvector = 0;
                paf->next = pObjIndex->affected;
                pObjIndex->affected = paf;
                top_affect++;
            }

            else if (letter == 'F')
            {
                AFFECT_DATA *paf;

                paf = alloc_perm(sizeof(*paf));
                letter = fread_letter(fp);
                switch (letter)
                {
                    case 'A':
                        paf->where = TO_AFFECTS;
                        break;
                    case 'I':
                        paf->where = TO_IMMUNE;
                        break;
                    case 'R':
                        paf->where = TO_RESIST;
                        break;
                    case 'V':
                        paf->where = TO_VULN;
                        break;
                    default:
                        bug("Load_objects: Bad where on flag set.", 0);
                        exit(1);
                }
                paf->type = -1;
                paf->level = pObjIndex->level;
                paf->duration = -1;
                paf->location = fread_number(fp);
                paf->modifier = fread_number(fp);
                paf->bitvector = fread_flag(fp);
                paf->next = pObjIndex->affected;
                pObjIndex->affected = paf;
                top_affect++;
            }

            else if (letter == 'E')
            {
                EXTRA_DESCR_DATA *ed;

                ed = alloc_perm(sizeof(*ed));
                ed->keyword = fread_string(fp);
                ed->description = fread_string(fp);
                ed->next = pObjIndex->extra_descr;
                pObjIndex->extra_descr = ed;
                top_ed++;
            }

            else
            {
                ungetc(letter, fp);
                break;
            }
        }

        iHash = vnum % MAX_KEY_HASH;
        pObjIndex->next = obj_index_hash[iHash];
        obj_index_hash[iHash] = pObjIndex;
        top_obj_index++;
        top_vnum_obj = top_vnum_obj < vnum ? vnum : top_vnum_obj;    /* OLC */
        assign_area_vnum(vnum);    /* OLC */
    }

    return;
}

/*
 * Load's a class into the class type.
 */
bool load_class(char *fname)
{
    char buf[MAX_STRING_LENGTH];
    int rating, guild = 0;
    CLASSTYPE *class;
    FILE *fp;
    char *word;
    bool fMatch;
    int cl = -1;
    char class_name[MAX_STRING_LENGTH];

    sprintf(buf, "%s%s", CLASS_DIR, fname);
    sprintf(class_name, "%s", fname);

    log_f("STATUS: Loading Class %s", fname);

    if (!(fp = fopen(buf, "r")))
    {
        global.last_boot_result = FAILURE;
        sprintf(buf, "Could not open file in order to load class %s%s.", CLASS_DIR, fname);
        log_string(buf);
        return FALSE;
    }

    class = alloc_perm(sizeof(*class));
    class->is_enabled = TRUE; // Default, will need to be set to false in the load file

    for (; ; )
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
                // Stat lookup will lookup the stat based on the word value to make the file easier to read.
                KEY("Attrprime", class->attr_prime, flag_value(stat_flags, fread_word(fp)));
                KEY("AttrSecond", class->attr_second, flag_value(stat_flags, fread_word(fp)));
                break;
            case 'B':
                KEY("BaseGroup", class->base_group, str_dup(fread_word(fp)));
                break;
            case 'C':
                // For note, this needs to be up at the top of the class file since we're reading
                // in the cl variable which is used in other keys.
                if (!str_cmp(word, "Class"))
                {
                    cl = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }

                KEY("Clan", class->clan, clan_lookup(fread_string(fp)));

                break;
            case 'D':
                KEY("DefGroup", class->default_group, str_dup(fread_word(fp)));
                break;
            case 'G':
                if (!str_cmp(word, "Gr") || !str_cmp(word, "Group"))
                {
                    int group;

                    word = fread_word(fp);
                    rating = fread_number(fp);
                    group = group_lookup(word);
                    if (group == -1)
                    {
                        global.last_boot_result = WARNING;
                        sprintf(buf, "load_class_file: Group %s unknown", word);
                        bug(buf, 0);
                    }
                    else
                    {
                        group_table[group]->rating[cl] = rating;
                    }
                    fMatch = TRUE;
                    break;
                }
                if (!str_cmp(word, "Guild"))
                {
                    class->guild[guild++] = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }
            case 'E':
                if (!str_cmp(word, "End"))
                {
                    fclose(fp);
                    if (cl < 0 || cl >= MAX_CLASS)
                    {
                        global.last_boot_result = FAILURE;
                        sprintf(buf, "Load_class_file: Class (%s) bad/not found (%d)",
                        class->who_name ? class->who_name: "name not found", cl);
                        bug(buf, 0);
                        if (class->name)
                            free_string(class->who_name);
                        return FALSE;
                    }
                    class_table[cl] = class;
                    top_class++;

                    log_f("STATUS: Loading Class %s Complete.", class_name);

                    return TRUE;
                }
            case 'H':
                KEY("Hpmin", class->hp_min, fread_number(fp));
                KEY("Hpmax", class->hp_max, fread_number(fp));
                break;
            case 'I':
                KEY("IsReclass", class->is_reclass, fread_number(fp));
                KEY("IsEnabled", class->is_enabled, fread_number(fp));
                break;
            case 'M':
                if (!str_cmp(word, "Mult"))
                {
                    int race;
                    race = fread_number(fp);
                            // marker todo, file-size this one, save the table out to disk
                            // to create the pc_race_table files from the hard coded tables
                    pc_race_table[race].class_mult[cl] = fread_number(fp);
                    //pc_race_table[race].can_take[cl] = fread_number(fp);
                    fMatch = TRUE;
                    break;
                }
                KEY("Mana", class->mana, fread_number(fp));
                break;
            case 'N':
                KEY("Name", class->name, fread_string(fp));
                break;
            case 'S':
                if (!str_cmp(word, "Sk") || !str_cmp(word, "Skill"))
                {
                    int sn, lev;

                    word = fread_word(fp);
                    lev = fread_number(fp);
                    rating = fread_number(fp);
                    sn = skill_lookup(word);
                    if (sn == -1)
                    {
                        global.last_boot_result = WARNING;
                        sprintf(buf, "load_class_file: Skill %s unknown", word);
                        bug(buf, 0);
                    }
                    else
                    {
                        skill_table[sn]->skill_level[cl] = lev;
                        skill_table[sn]->rating[cl] = rating;
                    }
                    fMatch = TRUE;
                    break;
                }
                KEY("Skilladept", class->skill_adept, fread_number(fp));
                break;
            case 'T':
                KEY("Thac0_00", class->thac0_00, fread_number(fp));
                KEY("Thac0_32", class->thac0_32, fread_number(fp));
                break;
            case 'W':
                if (!str_cmp(word, "WhoName"))
                {
                    strcpy(class->who_name, fread_string(fp));
                    fMatch = TRUE;
                    break;
                }
                KEY("Weapon", class->weapon, fread_number(fp));
                break;
        }

        if (!fMatch)
        {
            bugf("load_class key not found: %s", word);
        }
    }

} // end bool load_class

/*
 *  Loads each class listed in the CLASS_FILE
 */
void load_classes()
{
    FILE *fpList;
    char *filename;
    char classlist[256];
    char buf[MAX_STRING_LENGTH];
    extern int top_class;

    sprintf(classlist, "%s%s", CLASS_DIR, CLASS_FILE);
    if ((fpList = fopen(classlist, "r")) == NULL)
    {
        global.last_boot_result = FAILURE;
        log_string(classlist);
        exit(1);
    }

    for (; ; )
    {
        filename = feof(fpList) ? "$" : fread_word(fpList);
        if (filename[0] == '$')
            break;

        if (!load_class(filename))
        {
            global.last_boot_result = FAILURE;
            sprintf(buf, "Cannot load class file: %s", filename);
            bug(buf, 0);
        }
    }
    fclose(fpList);

    log_f("STATUS: %d of a maximum %d Classes Loaded", top_class, MAX_CLASS);

    // It may have been set to a failure or warning, but if it's unknown and we
    // get here then set it to a success.
    if (global.last_boot_result == UNKNOWN)
        global.last_boot_result = SUCCESS;

    return;

} // end void load_classes

/*
 * Loads all of the skills and spells into the groups they belong to.
 */
void load_groups()
{
    FILE *fp;
    if ((fp = fopen(GROUP_FILE, "r")) != NULL)
    {
        top_group = 0;
        for (;; )
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
                bug("load_groups: # not found.", 0);
                global.last_boot_result = WARNING;
                break;
            }

            word = fread_word(fp);
            if (!str_cmp(word, "GROUP"))
            {
                if (top_group >= MAX_GROUP)
                {
                    bug("load_groups: more skills than top_group %d", top_group);
                    global.last_boot_result = FAILURE;
                    fclose(fp);
                    return;
                }
                group_table[top_group++] = fread_group(fp);
                continue;
            }
            else
                if (!str_cmp(word, "END"))
                    break;
                else
                {
                    global.last_boot_result = WARNING;
                    bug("load_groups: bad section.", 0);
                    continue;
                }
        }
        fclose(fp);
    }
    else
    {
        global.last_boot_result = FAILURE;
        bug("load_groups: Cannot open groups.dat", 0);
        if (global.copyover) copyover_broadcast("STATUS: Shutting Down", TRUE);
        exit(0);
    }

    log_f("STATUS: %d of a maximum %d Groups Loaded", top_group, MAX_GROUP);
    global.last_boot_result = SUCCESS;

} // end void load_groups

/*
 * Reads a group in from the file and returns a GROUPTYPE.
 */
GROUPTYPE *fread_group(FILE *fp)
{
    char *word;
    bool fMatch;
    GROUPTYPE *gr;
    int x, i;
    i = 0;
    extern int top_class;

    gr = alloc_perm(sizeof(*gr));
    for (x = 0; x < top_class; x++)
    {
        gr->rating[x] = -1;
    }

    for (; ; )
    {
        word = feof(fp) ? "End" : fread_word(fp);
        fMatch = FALSE;

        switch (UPPER(word[0]))
        {
            case '*':
                fMatch = TRUE;
                fread_to_eol(fp);
                break;
            case 'S':
                KEY("Sk", gr->spells[i++], str_dup(fread_word(fp)));
                break;
            case 'E':
                if (!str_cmp(word, "End"))
                    return gr;
                break;
            case 'N':
                KEY("Name", gr->name, str_dup(fread_word(fp)));
                break;
        }

        if (!fMatch)
        {
            bug("fread_group: no match.", 0);
            fread_to_eol(fp);
        }
    }
    return gr;
} // end GROUPTYPE *fread_group

/*
 * Loads the skills and spells in from the skills.dat file.
 */
void load_skills()
{
    FILE *fp;
    if ((fp = fopen(SKILLS_FILE, "r")) != NULL)
    {
        top_sn = 0;
        for (;; )
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
                bug("Load_skill_table: # not found.", 0);
                break;
            }

            word = fread_word(fp);
            if (!str_cmp(word, "SKILL"))
            {
                if (top_sn >= MAX_SKILL)
                {
                    global.last_boot_result = FAILURE;
                    bug("load_skill_table: more skills than top_sn %d.  Increase MAX_SKILL in merc.h.", top_sn);
                    fclose(fp);
                    return;
                }
                skill_table[top_sn++] = fread_skill(fp);
                continue;
            }
            else
                if (!str_cmp(word, "END"))
                    break;
                else
                {
                    global.last_boot_result = WARNING;
                    bug("Load_skill_table: bad section.", 0);
                    continue;
                }
        }
        fclose(fp);
    }
    else
    {
        global.last_boot_result = FAILURE;
        bug("Cannot open SKILLS_FILE", 0);
        if (global.copyover) copyover_broadcast("STATUS: Shutting Down", TRUE);
        exit(0);
    }

    global.last_boot_result = SUCCESS;
    return;

} // end load_skills

/*
 * Reads in one skill/spell entry.
 */
SKILLTYPE *fread_skill(FILE *fp)
{
    char *word;
    bool fMatch;
    SKILLTYPE *skill;
    int x;

    skill = alloc_perm(sizeof(*skill));

    // Set all of the ratings to 52, -1 by default.  These values will later be
    // loaded when the classes are loaded (which is where they are stored).
    for (x = 0; x < MAX_CLASS; x++)
    {
        skill->skill_level[x] = LEVEL_IMMORTAL;
        skill->rating[x] = -1;
    }

    // Default all skills to ranged false.
    skill->ranged = FALSE;

    for (; ; )
    {
        word = feof(fp) ? "End" : fread_word(fp);
        fMatch = FALSE;

        switch (UPPER(word[0]))
        {
            case '*':
                fMatch = TRUE;
                fread_to_eol(fp);
                break;

            case 'B':
                KEY("Beats", skill->beats, fread_number(fp));
                break;

            case 'D':
                KEY("Damage", skill->noun_damage, fread_string(fp));
                break;

            case 'E':
                if (!str_cmp(word, "End"))
                    return skill;
                break;

            case 'M':
                KEY("MinMana", skill->min_mana, fread_number(fp));
                KEY("MinPos", skill->minimum_position, fread_number(fp));
                KEY("MsgObj", skill->msg_obj, fread_string(fp));
                KEY("MsgOff", skill->msg_off, fread_string(fp));
                break;
            case 'N':
                KEY("Name", skill->name, fread_string(fp));
                break;
            case 'R':
                KEY("Race", skill->race, fread_number(fp));
                KEY("Ranged", skill->ranged, fread_number(fp));
                break;
            case 'S':
                KEY("SpellFun", skill->spell_fun, spell_function_lookup(fread_string(fp)));
                break;
            case 'T':
                KEY("Target", skill->target, fread_number(fp));
                break;
        }

        if (!fMatch)
        {
            bug("fread_skill: no match.", 0);
            log_f("KEY missing: %s", word);
            fread_to_eol(fp);
        }
    }
    return skill;
}

/*
 * Saves certain items types that will persist across copyovers and
 * reboots.  Player corpses and pits will be the initial items.  We
 * may need to initiate pit or shelf purging or a maximum capacity if
 * this gets over bloated.  These are in play objects put in pits or
 * in PC corpses by players.  This is not OLC.
 */
void save_game_objects(void)
{
    FILE * fp;
    OBJ_DATA * obj;
    OBJ_DATA * obj_next;
    int objects_saved = 0;

    // Exit this loop if the game hasn't loaded, we don't want to accidently
    // save anything.
    if (!global.game_loaded)
    {
        return;
    }

    fclose(fpReserve);

    if ((fp = fopen(TEMP_FILE, "w")) == NULL)
    {
        bug("save_game_objects: fopen", 0);
        perror("failed open of TEMP_FILE in save_game_objects");
    }
    else
    {
        for (obj = object_list; obj != NULL; obj = obj_next)
        {
            obj_next = obj->next;

            // We are going to write out player corpses and pits (e.g. containers that are
            // no purge and that can't be worn.  We will also write out owned items like
            // disarmed weapons and buried items.
            if ((obj->owner != NULL
                || obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC
                || IS_OBJ_STAT(obj, ITEM_BURIED)
                || (obj->item_type == ITEM_CONTAINER && IS_OBJ_STAT(obj, ITEM_NOPURGE)))
                && obj->wear_loc == WEAR_NONE
                && obj->in_room != NULL)
            {
                fwrite_obj(NULL, obj, fp, 0);
                objects_saved++;
            }
        }

        fprintf(fp, "#END\n\n");

        fflush(fp);
        fclose(fp);
    }

#if defined(_WIN32)
    // In MS C rename will fail if the file exists (not so on POSIX).  In Windows, it will never
    // save past the first pfile save if this isn't done.
    _unlink(SAVED_OBJECT_FILE);
#endif

    // Rename the temp file to the saved object file
    rename(TEMP_FILE, SAVED_OBJECT_FILE);
    fpReserve = fopen(NULL_FILE, "r");

    return;

} // end save_game_objects

/*
 * This will load player corpses and pits that have been saved so certain
 * items can be persisted over time.  This has to run before the resets
 * initially otherwise it will duplicate the pit (it saves the pit object as
 * well as the items in it and loads them.  This also means that a shutdown
 * is required to manually take items out of the pit (as they are saved on
 * reboot/copyover/shutdown.  We will want to add an immortal command to purge
 * the contents of a pit if needed so then they can be cleared and pruned if
 * need in game.
 */
void load_game_objects(void)
{
    FILE *fp;
    fclose(fpReserve);

    if ((fp = fopen(SAVED_OBJECT_FILE, "r")) == NULL)
    {
       global.last_boot_result = MISSING;
       bug("load_game_objects: fopen of SAVED_OBJECT_FILE", 0);
    }
    else
    {
        for (; ; )
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
                bug("load_game_objects: # not found.", 0);
                break;
            }

            word = fread_word(fp);

            if (!str_cmp(word, "O"))
            {
                fread_obj(NULL, fp);
            }
            else if (!str_cmp(word, "END"))
            {
                break;
            }
            else
            {
                global.last_boot_result = WARNING;
                bug("Load_objects: bad section.", 0);
                break;
            }
        }
    }

    // If the file pointer existed, close it, otherwise log that the saved
    // object file wasn't there (and the game will put a new one there so this
    // should only happen once if it doesn't exist.
    if (fp)
    {
        fclose(fp);
    }
    else
    {
        log_f("WARNING: %s was not found", SAVED_OBJECT_FILE);
    }

    fpReserve = fopen(NULL_FILE, "r");

    // If it's unknown at this point then it's a success.
    if (global.last_boot_result == UNKNOWN)
        global.last_boot_result = SUCCESS;

} // end load_game_objects

/*
 * Saves all of the game statistics to file.
 */
void save_statistics(void)
{
    FILE *fp;

    fclose(fpReserve);
    fp = fopen(STATISTICS_FILE, "w");

    if (!fp)
    {
        bug("Could not open STATISTICS_FILE for writing.", 0);
        return;
    }

    // Game statistics
    fprintf(fp, "MaxPlayersOnline     %d\n", statistics.max_players_online);
    fprintf(fp, "MobsKilled           %ld\n", statistics.mobs_killed);
    fprintf(fp, "PlayersKilled        %ld\n", statistics.players_killed);
    fprintf(fp, "PKills               %ld\n", statistics.pkills);
    fprintf(fp, "Logins               %ld\n", statistics.logins);
    fprintf(fp, "TotalCharacters      %ld\n", statistics.total_characters);

    fprintf(fp, "#END\n");
    fclose(fp);
    fpReserve = fopen(NULL_FILE, "r");

} // end save_statistics

/*
 * Loads the game statistics from the statistics.dat file.  I skipped using the KEY macro in this
 * case an opt'd for an if ladder.  If an invalid statistic is found in the file it will attempt to
 * log it.  I don't read ahead on that value in case it's corrupt, I want to hit the #END marker
 * eventually and get out without crashing (if an unfound setting is logged, it will then read in
 * its value if it has one and do the if/else on it.
 */
void load_statistics()
{
    FILE *fp;
    char *word;

    fclose(fpReserve);
    fp = fopen(STATISTICS_FILE, "r");

    if (!fp)
    {
        global.last_boot_result = MISSING;
        log_f("WARNING: Statistics file '%s' was not found or is inaccessible.", STATISTICS_FILE);
        fpReserve = fopen(NULL_FILE, "r");
        return;
    }

    for (;;)
    {
        word = feof(fp) ? "#END" : fread_word(fp);

        // End marker?  Exit cleanly
        if (!str_cmp(word, "#END"))
        {
            fclose(fp);
            fpReserve = fopen(NULL_FILE, "r");
            global.last_boot_result = SUCCESS;
            return;
        }

        if (!str_cmp(word, "MaxPlayersOnline"))
        {
            statistics.max_players_online = fread_number(fp);
        }
        else if (!str_cmp(word, "MobsKilled"))
        {
            statistics.mobs_killed = fread_number(fp);
        }
        else if (!str_cmp(word, "PlayersKilled"))
        {
            statistics.players_killed = fread_number(fp);
        }
        else if (!str_cmp(word, "PKills"))
        {
            statistics.pkills = fread_number(fp);
        }
        else if (!str_cmp(word, "Logins"))
        {
            statistics.logins = fread_number(fp);
        }
        else if (!str_cmp(word, "TotalCharacters"))
        {
            statistics.total_characters = fread_number(fp);
        }
        else
        {
            log_f("Invalid statistic '%s' found.", word);
        }

    }

    // It should have closed on the end tag
    global.last_boot_result = WARNING;
    bugf("load_statistics: No #END found");

    fclose(fp);
    fpReserve = fopen(NULL_FILE, "r");

    return;

} // end load_statistics

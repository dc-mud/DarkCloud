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

// We're going to use this to indicate the version of this release which
// is arbitrary to the person implementing the game.
#define VERSION "2018.08.13"

#define args(list) list
#define DECLARE_DO_FUN(fun)       DO_FUN    fun
#define DECLARE_SPEC_FUN(fun)     SPEC_FUN  fun
#define DECLARE_SPELL_FUN(fun)    SPELL_FUN fun

/* system calls */
#if (!_WIN32)
    int unlink();
    int system();
#endif

#if !defined(FALSE)
    #define FALSE    0
#endif

#if !defined(TRUE)
    #define TRUE     1
#endif

typedef unsigned char bool;

/* Global Include for gsn.h */
#include "gsn.h"
#include "sqlite/sqlite3.h"

/*
 * Structure types.
 */
typedef struct    affect_data        AFFECT_DATA;
typedef struct    area_data          AREA_DATA;
typedef struct    ban_data           BAN_DATA;
typedef struct    buf_type           BUFFER;
typedef struct    char_data          CHAR_DATA;
typedef struct    descriptor_data    DESCRIPTOR_DATA;
typedef struct    exit_data          EXIT_DATA;
typedef struct    extra_descr_data   EXTRA_DESCR_DATA;
typedef struct    help_data          HELP_DATA;
typedef struct    help_area_data     HELP_AREA;
typedef struct    mem_data           MEM_DATA;
typedef struct    mob_index_data     MOB_INDEX_DATA;
typedef struct    note_data          NOTE_DATA;
typedef struct    obj_data           OBJ_DATA;
typedef struct    obj_index_data     OBJ_INDEX_DATA;
typedef struct    pc_data            PC_DATA;
typedef struct    gen_data           GEN_DATA;
typedef struct    reset_data         RESET_DATA;
typedef struct    room_index_data    ROOM_INDEX_DATA;
typedef struct    shop_data          SHOP_DATA;
typedef struct    time_info_data     TIME_INFO_DATA;
typedef struct    weather_data       WEATHER_DATA;
typedef struct    mprog_list         MPROG_LIST;
typedef struct    mprog_code         MPROG_CODE;
typedef struct    group_type         GROUPTYPE;
typedef struct    class_type         CLASSTYPE;
typedef struct    skill_type         SKILLTYPE;
typedef struct    settings_data      SETTINGS_DATA;
typedef struct    statistics_data    STATISTICS_DATA;
typedef struct    extended_bitvector EXT_BV;
typedef struct    timer_data         TIMER;
typedef struct    disabled_data      DISABLED_DATA;
typedef struct    global_data        GLOBAL_DATA;
typedef struct    db_buffer          DB_BUFFER;

/*
 * Function types.
 */
typedef void DO_FUN    (CHAR_DATA *ch, char *argument);
typedef bool SPEC_FUN  (CHAR_DATA *ch);
typedef void SPELL_FUN (int sn, int level, CHAR_DATA *ch, void *vo, int target);

/*
 * String and memory management parameters / shorthand defs.
 */
#define MAX_KEY_HASH          1024
#define MAX_STRING_LENGTH     4608
#define MAX_INPUT_LENGTH       256
#define PAGELEN                 22
#define MSL MAX_STRING_LENGTH
#define MIL MAX_INPUT_LENGTH

/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Group and Class maxes have been changed to initialize the table at a larger
 * value and then a top_ variable should be used in loops that indicate when the
 * loop ends, this will allow us to add groups and classes without having to up
 * this max everytime.  The top value is set to the exact amount on boot when the
 * items are loaded.
 */
#define MAX_SOCIALS        256
#define MAX_SKILL          250
#define MAX_GROUP          100  // top_group
#define MAX_IN_GROUP       20
#define MAX_ALIAS          10
#define MAX_CLASS          13   // top_class
#define MAX_PC_RACE        17
#define MAX_CLAN           3
#define MAX_DAMAGE_MESSAGE 41
#define MAX_LEVEL          60
#define LEVEL_HERO         51
#define LEVEL_IMMORTAL     52
#define CODER              MAX_LEVEL - 2
#define ADMIN              MAX_LEVEL - 1
#define IMPLEMENTOR        MAX_LEVEL

/* Added this for "orphaned help" code. Check do_help() -- JR */
#define MAX_CMD_LEN         50

/*
 * Timing pulses.  These are how long certain game events take, events
 * are typically fired off for each type.
 */
#define PULSE_PER_SECOND    4
#define PULSE_VIOLENCE      (  3 * PULSE_PER_SECOND)
#define PULSE_MOBILE        (  4 * PULSE_PER_SECOND)
#define PULSE_TICK          ( 40 * PULSE_PER_SECOND)
#define PULSE_MINUTE        ( 60 * PULSE_PER_SECOND)
#define PULSE_AREA          (120 * PULSE_PER_SECOND)
#define PULSE_HALF_TICK     (PULSE_TICK / 2)

/*
 * Color stuff by Lope.
 */
#define CLEAR       "\x1B[0m"       /* Resets Color */
#define C_BLACK	    "\x1B[0;30m"
#define C_RED       "\x1B[0;31m"    /* Normal Colors */
#define C_GREEN     "\x1B[0;32m"
#define C_YELLOW    "\x1B[0;33m"
#define C_BLUE      "\x1B[0;34m"
#define C_MAGENTA   "\x1B[0;35m"
#define C_CYAN      "\x1B[0;36m"
#define C_WHITE     "\x1B[0;37m"
#define C_D_GREY    "\x1B[1;30m"    /* Light Colors */
#define C_B_RED     "\x1B[1;31m"
#define C_B_GREEN   "\x1B[1;32m"
#define C_B_YELLOW  "\x1B[1;33m"
#define C_B_BLUE    "\x1B[1;34m"
#define C_B_MAGENTA "\x1B[1;35m"
#define C_B_CYAN    "\x1B[1;36m"
#define C_B_WHITE   "\x1B[1;37m"
#define BLINK       "\x1B[5m"      /* Special Codes */
#define BACK        "\x1B[1D"
#define UNDERLINE   "\x1B[4m"
#define REVERSE     "\x1B[7m"
#define UP          "\x1B[1A"      /* Cursor Movement */
#define DOWN	    "\x1B[1B"
#define RIGHT	    "\x1B[1C"
#define LEFT        "\x1B[1D"

#define HEADER "--------------------------------------------------------------------------------\r\n"

/*
 * Class lookup values that correspond to the class table.
 */
#define MAGE_CLASS                       0
#define CLERIC_CLASS                     1
#define THIEF_CLASS                      2
#define WARRIOR_CLASS                    3
#define ENCHANTER_CLASS                  4
#define HEALER_CLASS                     5
#define DIRGE_CLASS                      6
#define RANGER_CLASS                     7
#define ROGUE_CLASS                      8
#define PSIONICIST_CLASS                 9
#define PRIEST_CLASS                     10
#define BARBARIAN_CLASS                  11
#define WIZARD_CLASS                     12

/*
 * PC Race Lookup
 */
#define NONE_RACE                        0
#define HUMAN_RACE                       1
#define ELF_RACE                         2
#define HALF_ELF_RACE                    3
#define WILD_ELF_RACE                    4
#define SEA_ELF_RACE                     5
#define DARK_ELF_RACE                    6
#define DWARF_RACE                       7
#define HILL_DWARF_RACE                  8
#define MOUNTAIN_DWARF_RACE              9
#define DARK_DWARF_RACE                  10
#define OGRE_RACE                        11
#define GIANT_OGRE_RACE                  12
#define KENDER_RACE                      13
#define MINOTAUR_RACE                    14
#define YINN_RACE                        15
#define GULLY_DWARF_RACE                 16

/*
 * Priest ranks
 */
#define PRIEST_RANK_NOVITIATE   0
#define PRIEST_RANK_DEACON      1
#define PRIEST_RANK_PRIEST      2
#define PRIEST_RANK_BISHOP      3
#define PRIEST_RANK_ARCHBISHOP  4
#define PRIEST_RANK_CARDINAL    5
#define PRIEST_RANK_HIGH_PRIEST 6

/*
 * Thanks Dingo for making life a bit easier ;)
 */
#define CH(d)((d)->original ? (d)->original : (d)->character)

/*
 * Site ban structure.
 */
#define BAN_SUFFIX     A
#define BAN_PREFIX     B
#define BAN_NEWBIES    C
#define BAN_ALL        D
#define BAN_PERMANENT  F
#define BAN_WHITELIST  G

struct ban_data
{
    BAN_DATA  * next;
    bool        valid;
    int         ban_flags;
    int         level;
    char      * name;
};

struct buf_type
{
    BUFFER    * next;
    bool        valid;
    int         state;  /* error state of the buffer */
    long        size;   /* size in k */
    char *      string; /* buffer's string */
};

/*
 * Time and weather stuff.
 */
#define SUN_DARK       0
#define SUN_RISE       1
#define SUN_LIGHT      2
#define SUN_SET        3

#define SKY_CLOUDLESS  0
#define SKY_CLOUDY     1
#define SKY_RAINING    2
#define SKY_LIGHTNING  3

struct time_info_data
{
    int        hour;
    int        day;
    int        month;
    int        year;
};

struct weather_data
{
    int        mmhg;
    int        change;
    int        sky;
    int        sunlight;
};

/*
 * Connected state for a channel.
 */
#define CON_CHOOSE_MERIT            -21
#define CON_ASK_MERIT               -20
#define CON_GET_DEITY               -19
#define CON_NEW_CHARACTER           -18
#define CON_LOGIN_RETURN            -17
#define CON_LOGIN_MENU              -16
#define CON_GET_EMAIL               -15
#define CON_GET_NAME			    -14
#define CON_GET_OLD_PASSWORD		-13
#define CON_CONFIRM_NEW_NAME		-12
#define CON_GET_NEW_PASSWORD		-11
#define CON_CONFIRM_NEW_PASSWORD	-10
#define CON_COLOR                    -9
#define CON_GET_TELNETGA             -8
#define CON_GET_NEW_RACE             -7
#define CON_GET_NEW_SEX              -6
#define CON_GET_NEW_CLASS            -5
#define CON_GET_ALIGNMENT            -4
#define CON_DEFAULT_CHOICE           -3
#define CON_GEN_GROUPS               -2
#define CON_PICK_WEAPON              -1
#define CON_PLAYING                   0
#define CON_READ_IMOTD                1
#define CON_READ_MOTD                 2
#define CON_BREAK_CONNECT             3
#define CON_COPYOVER_RECOVER          4

/*
 * Descriptor (channel) structure.
 */
struct descriptor_data
{
    DESCRIPTOR_DATA *  next;
    DESCRIPTOR_DATA *  snoop_by;
    CHAR_DATA       *  character;
    CHAR_DATA       *  original;
    bool               valid;
    bool               ansi;
    char            *  host; // host or the dotted ip_address
    char            *  ip_address; // always the dotted ip_address
    int                descriptor;
    int                connected;
    bool               fcommand;
    char               inbuf[MAX_INPUT_LENGTH * 4];
    char               incomm[MAX_INPUT_LENGTH];
    char               inlast[MAX_INPUT_LENGTH];
    int                repeat;
    char            *  outbuf;
    int                outsize;
    int                outtop;
    char            *  showstr_head;
    char            *  showstr_point;
    void            *  pEdit;         /* OLC */
    char           **  pString;       /* OLC */
    int                editor;        /* OLC */
    char            *  name;          /* COPYOVER */
};

/*
 * Attribute bonus structures.
 */
struct str_app_type
{
    int todam;
    int carry;
    int wield;
};

struct int_app_type
{
    int learn;            // How fast individuals learn from using skills, higher is better
    int improve_minutes;  // How many online minutes between improve a skill, lower is better
};

struct wis_app_type
{
    int practice;
    int saves_bonus;    // Bonus the players saves
};

struct dex_app_type
{
    int defensive;      // AC Bonus
    int hitroll_bonus;  // Hit Roll Bonus
};

struct con_app_type
{
    int hitp;
};


/*
 * The default standard for what values a mob of a certain level should be
 * set at which will be used with the balance command in OLC.  This will
 * simplify the creation of new mobs.
 */
struct standard_mob_values_type
{
    int level;
    int ac_pierce;
    int ac_bash;
    int ac_slash;
    int ac_exotic;
    int hit_number;
    int hit_type;
    int hit_bonus;
    int dam_number;
    int dam_type;
    int dam_bonus;
    int mana_number;
    int mana_type;
    int mana_bonus;
};

/*
 * TO types for act.
 */
#define TO_ROOM           0
#define TO_NOTVICT        1
#define TO_VICT           2
#define TO_CHAR           3
#define TO_ALL            4

/*
 * Help table types.
 */
struct help_data
{
    HELP_DATA *  next;
    HELP_DATA *  next_area;
    int          level;
    char *       keyword;
    char *       text;
};

struct help_area_data
{
    HELP_AREA *    next;
    HELP_DATA *    first;
    HELP_DATA *    last;
    AREA_DATA *    area;
    char *         filename;
    bool           changed;
};


/*
 * Shop types.
 */
#define MAX_TRADE     5

struct    shop_data
{
    SHOP_DATA * next;                  /* Next shop in list              */
    int         keeper;                /* Vnum of shop keeper mob        */
    int         buy_type[MAX_TRADE];  /* Item types shop will buy       */
    int         profit_buy;            /* Cost multiplier for buying     */
    int         profit_sell;           /* Cost multiplier for selling    */
    int         open_hour;             /* First opening hour             */
    int         close_hour;            /* First closing hour             */
};

/*
 * Per-class stuff.
 */
#define MAX_GUILD  3
#define MAX_STATS  5
#define STAT_STR   0
#define STAT_INT   1
#define STAT_WIS   2
#define STAT_DEX   3
#define STAT_CON   4

// reclass
struct class_type
{
    char *  name;              /* the full name of the class  */
    char    who_name[4];       /* Three-letter name for 'who' */
    int     attr_prime;        /* Prime attribute             */
    int     attr_second;       /* Secondary attribute         */
    int     weapon;            /* First weapon                */
    int     guild[MAX_GUILD];  /* Vnum of guild rooms         */
    int     skill_adept;       /* Maximum skill level         */
    int     thac0_00;          /* Thac0 for level  0          */
    int     thac0_32;          /* Thac0 for level 32          */
    int     hp_min;            /* Min hp gained on leveling   */
    int     hp_max;            /* Max hp gained on leveling   */
    bool    mana;              /* Class gains mana on level   */
    char *  base_group;        /* base skills gained          */
    char *  default_group;     /* default skills gained       */
    bool    is_reclass;        /* Whether the class is a reclass (or a base class) */
    bool    is_enabled;        /* Whether the class is enabled for public use */
    int     clan;              /* The clan number if this is a clan specific reclass */
};

/*
 * These are spells (and their message off) that are shared and used in both
 * spell_cancel and spell_dispel.  The gsn is the global skill number of the
 * spell and the room message is the act message the room should display.
 */
struct dispel_type
{
    int  *    gsn;
    char *    room_msg;
};

/*
 * Priest ranks.  The integer rank for comparison, the name to be show in who and
 * in score and the minimum hours required for a given rank.
 */
struct priest_rank_type
{
    int rank;
    char * name;
    int hours;
};

struct item_type
{
    int       type;
    char *    name;
};


/*
 * Tables that can be exported via db_export and the flag_type that table maps to.
 */
struct export_flags_type
{
    char  *table_name;
    char  *friendly_name;
    const struct flag_type *flags;
};

/*
 * This will hold the continent int lookup value and name.
 */
struct continent_type
{
    int       type;
    char *    name;
};

/*
 * Structure for a merit including the bit, the name, a description and
 * whether it can be choosen from the login menu.
 */
struct merit_type
{
    long   merit;
    char * name;
    bool   chooseable;
};

struct weapon_type
{
    char *    name;
    int       vnum;
    int       type;
    int      *gsn;
};

struct wiznet_type
{
    char *   name;
    long     flag;
    int      level;
};

struct attack_type
{
    char *    name;            /* name         */
    char *    noun;            /* message      */
    int       damage;          /* damage class */
};

struct race_type
{
    char *  name;     /* call name of the race          */
    bool    pc_race;  /* can be chosen by pcs           */
    long    act;      /* act bits for the race          */
    long    aff;      /* aff bits for the race          */
    long    off;      /* off bits for the race          */
    long    imm;      /* imm bits for the race          */
    long    res;      /* res bits for the race          */
    long    vuln;     /* vuln bits for the race         */
    long    form;     /* default form flag for the race */
    long    parts;    /* default parts for the race     */
};

struct pc_race_type                 /* additional data for pc races    */
{
    char *  name;                   /* MUST be in race_type            */
    char    who_name[7];
    char *  article_name;           /* Name plus article, an elf, a dwarf, etc.*/
    int     points;                 /* cost in points of the race      */
    int     class_mult[MAX_CLASS];  /* exp multiplier for class, * 100 */
    char *  skills[5];              /* bonus skills for the race       */
    int     stats[MAX_STATS];       /* starting stats                  */
    int     max_stats[MAX_STATS];   /* maximum stats                   */
    int     size;                   /* aff bits for the race           */
    bool    player_selectable;      /* Whether or not a player can select this race at creation */
};

struct spec_type
{
    char *      name;      /* special function name */
    SPEC_FUN *  function;  /* the function          */
};

/*
 * One disabled command
 */
struct disabled_data
{
    DISABLED_DATA         *next;        /* pointer to next node */
    struct cmd_type const *command;     /* pointer to the command struct*/
    char                  *disabled_by; /* name of disabler */
    int                   level;        /* level of disabler */
};

/*
 * Buffer to hold SQL to be executed in batches.
 */
struct db_buffer
{
    DB_BUFFER*  next;       // For linked list.
    char*       buffer;     // Buffer to hold
    time_t      timestamp;  // When was this buffer added.
};

/*
 * Data structure for notes.
 */
#define NOTE_NOTE       0
#define NOTE_NEWS       1
#define NOTE_CHANGES    2
#define NOTE_PENALTY    3
#define NOTE_OOC        4
#define NOTE_STORY      5
#define NOTE_HISTORY    6
#define NOTE_IMM	    7

/*
 * Data structure for notes.
 */
struct note_data
{
    NOTE_DATA *  next;
    bool         valid;
    int          type;
    char *       sender;
    char *       date;
    char *       to_list;
    char *       subject;
    char *       text;
    time_t       date_stamp;
};

/*
 * An affect.
 */
struct affect_data
{
    AFFECT_DATA *  next;
    CHAR_DATA *    caster;    // Included when the caster is needed later on an effect.
    bool           valid;
    int            where;
    int            type;
    int            level;
    int            duration;
    int            location;
    int            modifier;
    int            bitvector;
};

/* where definitions */
#define TO_AFFECTS   0
#define TO_OBJECT    1
#define TO_IMMUNE    2
#define TO_RESIST    3
#define TO_VULN      4
#define TO_WEAPON    5

/*
 * Game settings - This will contain reward bonuses, game locks and
 * game mechanic toggles to allow some functionality to be enabled and
 * disabled on the fly.  This table will be savable via the setgings
 * comand and loaded on game boot.
 */
struct settings_data
{
    // Game Bonuses
    bool double_exp;         // Double Experience
    bool double_gold;        // Double Gold
    // Game Locks / System Behavior
    bool newlock;            // New lock, no new characters can create
    bool wizlock;            // Only immortals can login
    bool whitelist_lock;     // Whether a white list is active, see ban.c for info.
    bool test_mode;          // Whether the entire game is put into test mode
    bool login_color_prompt; // Whether or not the Do you want color? prompt will appear on login
    bool db_logging;         // Logging of some game data to the sqlite database.
    // Game Mechanics
    bool shock_spread;       // Shocking effect spreads under water.
    bool gain_convert;       // Whether or not gain convert is enabled.
    int stat_surge;          // How far above the players natural stat a spell can take it.
    bool hours_affect_exp;   // Whether hours affect experience
    bool focused_improves;   // Whether or not you improve skills via the improvement system.
    // Info
    char *web_page_url;
    char *mud_name;
    char *login_greeting;
    char *login_menu_light_color;
    char *login_menu_dark_color;
    bool login_who_list_enabled;
    char *pager_color;       // The color to use on the pager that splits up buffered text
    // Persisted Game Data
    int storm_keep_owner;    // The clan who owns the storm keep (A value of 0 is none)
    int storm_keep_target;   // The clan who the storm keep owner has set the target onto
};

/*
 * Game statistics - This will be used to save information about in game
 * stats, such as the maximum amount of players ever online, mobs killed,
 * players killed, etc.  The stats will be saved on some interval in the
 * update module and re/loaded on boot.
 */
struct statistics_data
{
    int  max_players_online;     // Most players ever online
    long mobs_killed;            // Total number of mobs killed
    long players_killed;         // Total number of times players killed
    long pkills;                 // Players killed by other players
    long logins;                 // Total number of logins
    long total_characters;       // Total number of characters created
};

/*
 * Structure for holding first and last parts of names for use in the
 * random name generator.
 */
struct name_part_type
{
    char *first_part;
    char *last_part;
};

extern const struct name_part_type name_part_table[];

/*
 * Results for a load_* function when the mud is booting.  This will allow us to show
 * the last status in copyover (whether it failed, etc.).  We'll store the last status
 * in the global data so we don't have to pass it around.
 */
typedef enum {UNKNOWN, SUCCESS, FAILURE, WARNING, DISABLE, DEFAULT, MISSING} boot_result;

/*
 * Global data - This is going to store global data that is not persisted
 * between boots.  This will help us reign in and organize various global
 * variables throughout the game (and in a modern IDE will help us quickly
 * find/use them).
 */
struct global_data
{
    bool copyover;                          // Whether this boot is a copyover or not
    CHAR_DATA *copyover_ch;                 // Link back to the person who executed the copyover
    char copyover_reason[MAX_INPUT_LENGTH]; // The reason for the copyover so we can show it on each tick.
    bool is_copyover;                       // Whether a copyover is running or not
    int  copyover_timer;                    // How many ticks are left until the copyover executes
    bool shutdown;                          // Whether a shutdown is in progress (formerly merc_down)
    bool game_loaded;                       // Whether the game has been fully booted.
    char boot_time[MAX_INPUT_LENGTH];       // String of when the mud booted, formerly str_boot_time
    boot_result last_boot_result;           // The status of the last boot function.
    char last_command[MAX_STRING_LENGTH];   // The last command and argument that was run.
    int max_on_boot;                        // The max people online for this boot.
    sqlite3 *db;                            // Global SQLite DB
    char buf[MAX_STRING_LENGTH];            // This is similiar to the old log_buf, I've come full circle on it.
    char sql[MAX_STRING_LENGTH * 4];        // Buffer to hold SQL
    int db_operations;                      // The number of DB operations executed this boot
};

/*
 * Drunk struct
 */
struct struckdrunk
{
    int     min_drunk_level;
    int     number_of_rep;
    char    *replacement[11];
};

/*
 * Clan ranks
 */
struct clan_rank_type
{
    char *name;       // The name of the rank as should appear in whoc
    int clan;         // The clan the rank belongs to
    bool is_default;  // If the rank is the default rank for the clan
};

/*
 * Character Sub States / Timers (from Smaug)
 */
typedef enum
{
    SUB_NONE, SUB_PAUSE,
    /* timer types ONLY below this point */
    SUB_TIMER_DO_ABORT = 128, SUB_TIMER_CANT_ABORT
} char_substates;

/*
 * Types of timers available
 */
typedef enum
{
    TIMER_NONE, TIMER_DO_FUN
} timer_types;


/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (Start of section ... start here)                     *
 *                                                                         *
 ***************************************************************************/

/*
 * Well known mob virtual numbers that are used somewhere in code (like
 * in special.c or for loading).
 * Defined in #MOBILES.
 */
#define MOB_VNUM_FIDO            3090
#define MOB_VNUM_PATROLMAN       2106
#define VNUM_GUARDIAN_ANGEL        10
#define GROUP_VNUM_TROLLS        2100
#define GROUP_VNUM_OGRES         2101

#define STORM_KEEP_GUARD         5500
#define STORM_KEEP_LORD          5501
#define STORM_KEEP_PORTAL        5533  // The room the clan's portal takes them to

/* RT ASCII conversions -- used so we can have letters in this file */
#define A            1
#define B            2
#define C            4
#define D            8
#define E           16
#define F           32
#define G           64
#define H          128
#define I          256
#define J          512
#define K         1024
#define L         2048
#define M         4096
#define N         8192
#define O        16384
#define P        32768
#define Q        65536
#define R       131072
#define S       262144
#define T       524288
#define U      1048576
#define V      2097152
#define W      4194304
#define X      8388608
#define Y     16777216
#define Z     33554432
#define aa    67108864  /* doubled due to conflicts */
#define bb   134217728
#define cc   268435456
#define dd   536870912
#define ee  1073741824
#define ff  2147483648
#define gg  4294967296

/*
 * ACT bits for mobs.
 * Used in #MOBILES.
 */
#define ACT_IS_NPC             (A)   /* Auto set for mobs  */
#define ACT_SENTINEL           (B)   /* Stays in one room  */
#define ACT_SCAVENGER          (C)   /* Picks up objects   */
#define ACT_IS_PORTAL_MERCHANT (D)   /* Mob that sells portals to places */
#define ACT_TRACKER            (E)   /* A mob that can track other mobs or players */
#define ACT_AGGRESSIVE         (F)   /* Attacks PC's       */
#define ACT_STAY_AREA          (G)   /* Won't leave area   */
#define ACT_WIMPY              (H)   /* Mob will flee when it has little life left */
#define ACT_PET                (I)   /* Auto set for pets  */
#define ACT_TRAIN              (J)   /* Can train PC's     */
#define ACT_PRACTICE           (K)   /* Can practice PC's  */
#define ACT_BANKER             (L)
#define ACT_SAFE               (M)   /* Cannot be attacked */
//                             (N)
#define ACT_UNDEAD             (O)
//                             (P)
#define ACT_CLERIC             (Q)   /* Mob can cast cleric spells in battle */
#define ACT_MAGE               (R)   /* Mob can cast mage spells in battle */
#define ACT_THIEF              (S)
#define ACT_WARRIOR            (T)
#define ACT_NOALIGN            (U)
#define ACT_NOPURGE            (V)   /* Mob can't be purged from a room */
#define ACT_OUTDOORS           (W)
#define ACT_INDOORS            (Y)
#define ACT_SCRIBE             (Z)
#define ACT_IS_HEALER          (aa)  /* Mob sells healing services */
#define ACT_GAIN               (bb)
#define ACT_UPDATE_ALWAYS      (cc)
#define ACT_IS_CHANGER         (dd)

/* damage classes */
#define DAM_NONE            0
#define DAM_BASH            1
#define DAM_PIERCE          2
#define DAM_SLASH           3
#define DAM_FIRE            4
#define DAM_COLD            5
#define DAM_LIGHTNING       6
#define DAM_ACID            7
#define DAM_POISON          8
#define DAM_NEGATIVE        9
#define DAM_HOLY           10
#define DAM_ENERGY         11
#define DAM_MENTAL         12
#define DAM_DISEASE        13
#define DAM_DROWNING       14
#define DAM_LIGHT          15
#define DAM_OTHER          16
#define DAM_HARM           17
#define DAM_CHARM          18
#define DAM_SOUND          19

/* OFF bits for mobiles */
#define OFF_AREA_ATTACK    (A)
#define OFF_BACKSTAB       (B)
#define OFF_BASH           (C)
#define OFF_BERSERK        (D)
#define OFF_DISARM         (E)
#define OFF_DODGE          (F)
#define OFF_FADE           (G)
#define OFF_FAST           (H)
#define OFF_KICK           (I)
#define OFF_KICK_DIRT      (J)
#define OFF_PARRY          (K)
#define OFF_RESCUE         (L)
#define OFF_TAIL           (M)
#define OFF_TRIP           (N)
#define OFF_CRUSH          (O)
#define ASSIST_ALL         (P)
#define ASSIST_ALIGN       (Q)
#define ASSIST_RACE        (R)
#define ASSIST_PLAYERS     (S)
#define ASSIST_GUARD       (T)
#define ASSIST_VNUM        (U)

/* return values for check_imm */
#define IS_NORMAL          0
#define IS_IMMUNE          1
#define IS_RESISTANT       2
#define IS_VULNERABLE      3

/* IMM bits for mobs */
#define IMM_SUMMON         (A)
#define IMM_CHARM          (B)
#define IMM_MAGIC          (C)
#define IMM_WEAPON         (D)
#define IMM_BASH           (E)
#define IMM_PIERCE         (F)
#define IMM_SLASH          (G)
#define IMM_FIRE           (H)
#define IMM_COLD           (I)
#define IMM_LIGHTNING      (J)
#define IMM_ACID           (K)
#define IMM_POISON         (L)
#define IMM_NEGATIVE       (M)
#define IMM_HOLY           (N)
#define IMM_ENERGY         (O)
#define IMM_MENTAL         (P)
#define IMM_DISEASE        (Q)
#define IMM_DROWNING       (R)
#define IMM_LIGHT          (S)
#define IMM_SOUND          (T)
//                         (U)
//                         (V)
//                         (W)
#define IMM_WOOD           (X)
#define IMM_SILVER         (Y)
#define IMM_IRON           (Z)

/* RES bits for mobs */
#define RES_SUMMON         (A)
#define RES_CHARM          (B)
#define RES_MAGIC          (C)
#define RES_WEAPON         (D)
#define RES_BASH           (E)
#define RES_PIERCE         (F)
#define RES_SLASH          (G)
#define RES_FIRE           (H)
#define RES_COLD           (I)
#define RES_LIGHTNING      (J)
#define RES_ACID           (K)
#define RES_POISON         (L)
#define RES_NEGATIVE       (M)
#define RES_HOLY           (N)
#define RES_ENERGY         (O)
#define RES_MENTAL         (P)
#define RES_DISEASE        (Q)
#define RES_DROWNING       (R)
#define RES_LIGHT          (S)
#define RES_SOUND          (T)
//                         (U)
//                         (V)
//                         (W)
#define RES_WOOD           (X)
#define RES_SILVER         (Y)
#define RES_IRON           (Z)

/* VULN bits for mobs */
#define VULN_SUMMON        (A)
#define VULN_CHARM         (B)
#define VULN_MAGIC         (C)
#define VULN_WEAPON        (D)
#define VULN_BASH          (E)
#define VULN_PIERCE        (F)
#define VULN_SLASH         (G)
#define VULN_FIRE          (H)
#define VULN_COLD          (I)
#define VULN_LIGHTNING     (J)
#define VULN_ACID          (K)
#define VULN_POISON        (L)
#define VULN_NEGATIVE      (M)
#define VULN_HOLY          (N)
#define VULN_ENERGY        (O)
#define VULN_MENTAL        (P)
#define VULN_DISEASE       (Q)
#define VULN_DROWNING      (R)
#define VULN_LIGHT         (S)
#define VULN_SOUND         (T)
//                         (U)
//                         (V)
//                         (W)
#define VULN_WOOD          (X)
#define VULN_SILVER        (Y)
#define VULN_IRON          (Z)

/* body form */
#define FORM_EDIBLE        (A)
#define FORM_POISON        (B)
#define FORM_MAGICAL       (C)
#define FORM_INSTANT_DECAY (D)
#define FORM_OTHER         (E)  /* defined by material bit */
//                         (F)

/* actual form */
#define FORM_ANIMAL        (G)
#define FORM_SENTIENT      (H)
#define FORM_UNDEAD        (I)
#define FORM_CONSTRUCT     (J)
#define FORM_MIST          (K)
#define FORM_INTANGIBLE    (L)

#define FORM_BIPED         (M)
#define FORM_CENTAUR       (N)
#define FORM_INSECT        (O)
#define FORM_SPIDER        (P)
#define FORM_CRUSTACEAN    (Q)
#define FORM_WORM          (R)
#define FORM_BLOB          (S)

//                         (T)
//                         (U)
#define FORM_MAMMAL        (V)
#define FORM_BIRD          (W)
#define FORM_REPTILE       (X)
#define FORM_SNAKE         (Y)
#define FORM_DRAGON        (Z)
#define FORM_AMPHIBIAN     (aa)
#define FORM_FISH          (bb)
#define FORM_COLD_BLOOD    (cc)

/* body parts */
#define PART_HEAD          (A)
#define PART_ARMS          (B)
#define PART_LEGS          (C)
#define PART_HEART         (D)
#define PART_BRAINS        (E)
#define PART_GUTS          (F)
#define PART_HANDS         (G)
#define PART_FEET          (H)
#define PART_FINGERS       (I)
#define PART_EAR           (J)
#define PART_EYE           (K)
#define PART_LONG_TONGUE   (L)
#define PART_EYESTALKS     (M)
#define PART_TENTACLES     (N)
#define PART_FINS          (O)
#define PART_WINGS         (P)
#define PART_TAIL          (Q)
//                         (R)
//                         (S)
//                         (T)
/* for combat */
#define PART_CLAWS         (U)
#define PART_FANGS         (V)
#define PART_HORNS         (W)
#define PART_SCALES        (X)
#define PART_TUSKS         (Y)

/*
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 */
#define AFF_BLIND          (A)
#define AFF_INVISIBLE      (B)
#define AFF_DETECT_EVIL    (C)
#define AFF_DETECT_INVIS   (D)
#define AFF_DETECT_MAGIC   (E)
#define AFF_DETECT_HIDDEN  (F)
#define AFF_DETECT_GOOD    (G)
#define AFF_SANCTUARY      (H)
#define AFF_FAERIE_FIRE    (I)
#define AFF_INFRARED       (J)
#define AFF_CURSE          (K)
#define AFF_DEAFEN         (L)
#define AFF_POISON         (M)
#define AFF_PROTECT_EVIL   (N)
#define AFF_PROTECT_GOOD   (O)
#define AFF_SNEAK          (P)
#define AFF_HIDE           (Q)
#define AFF_SLEEP          (R)
#define AFF_CHARM          (S)
#define AFF_FLYING         (T)
#define AFF_PASS_DOOR      (U)
#define AFF_HASTE          (V)
#define AFF_CALM           (W)
#define AFF_PLAGUE         (X)
#define AFF_WEAKEN         (Y)
#define AFF_DARK_VISION    (Z)
#define AFF_BERSERK        (aa)
#define AFF_SWIM           (bb)
#define AFF_REGENERATION   (cc)
#define AFF_SLOW           (dd)

/*
 * Sex.
 * Used in #MOBILES.
 */
#define SEX_NEUTRAL  0
#define SEX_MALE     1
#define SEX_FEMALE   2

/* AC types */
#define AC_PIERCE    0
#define AC_BASH      1
#define AC_SLASH     2
#define AC_EXOTIC    3

/* dice */
#define DICE_NUMBER  0
#define DICE_TYPE    1
#define DICE_BONUS   2

/* size */
#define SIZE_TINY    0
#define SIZE_SMALL   1
#define SIZE_MEDIUM  2
#define SIZE_LARGE   3
#define SIZE_HUGE    4
#define SIZE_GIANT   5
#define SIZE_DRAGON  6

/* alignments */
#define ALIGN_EVIL      1   // Evil folks
#define ALIGN_NEUTRAL   2   // Neutral folks
#define ALIGN_GOOD		3   // Good folks
#define ALIGN_ALL       4   // This is going to be used for gods who accept anyone (not a true alignment)

/*
 * Well known object virtual numbers.
 * Defined in #OBJECTS.
 */
#define OBJ_VNUM_SILVER_ONE         1
#define OBJ_VNUM_GOLD_ONE           2
#define OBJ_VNUM_GOLD_SOME          3
#define OBJ_VNUM_SILVER_SOME        4
#define OBJ_VNUM_COINS              5
#define OBJ_VNUM_EGG                6
#define OBJ_VNUM_BLUE_DIAMOND       7

#define OBJ_VNUM_CORPSE_NPC        10
#define OBJ_VNUM_CORPSE_PC         11
#define OBJ_VNUM_SEVERED_HEAD      12
#define OBJ_VNUM_TORN_HEART        13
#define OBJ_VNUM_SLICED_ARM        14
#define OBJ_VNUM_SLICED_LEG        15
#define OBJ_VNUM_GUTS              16
#define OBJ_VNUM_BRAINS            17

#define OBJ_VNUM_FOG               18

#define OBJ_VNUM_MUSHROOM          20
#define OBJ_VNUM_LIGHT_BALL        21
#define OBJ_VNUM_SPRING            22
#define OBJ_VNUM_DISC              23
#define OBJ_VNUM_WARPSTONE         24
#define OBJ_VNUM_PORTAL            25
#define OBJ_VNUM_HEALERS_BIND      26
#define OBJ_VNUM_STEAK             27
#define OBJ_VNUM_CAMPFIRE          28
#define OBJ_VNUM_SPRING_2          29
#define OBJ_VNUM_BIND_STONE        31

#define OBJ_VNUM_ROSE              1001

#define OBJ_VNUM_PIT               3010

#define OBJ_VNUM_SCHOOL_MACE       3700
#define OBJ_VNUM_SCHOOL_DAGGER     3701
#define OBJ_VNUM_SCHOOL_SWORD      3702
#define OBJ_VNUM_SCHOOL_SPEAR      3717
#define OBJ_VNUM_SCHOOL_STAFF      3718
#define OBJ_VNUM_SCHOOL_AXE        3719
#define OBJ_VNUM_SCHOOL_FLAIL      3720
#define OBJ_VNUM_SCHOOL_WHIP       3721
#define OBJ_VNUM_SCHOOL_POLEARM    3722

#define OBJ_VNUM_SCHOOL_VEST       3703
#define OBJ_VNUM_SCHOOL_SHIELD     3704
#define OBJ_VNUM_SCHOOL_BANNER     3716
#define OBJ_VNUM_MAP               3162

#define OBJ_VNUM_WHISTLE           2116

/*
 * Item types.
 * Used in #OBJECTS.
 */
#define ITEM_LIGHT       1
#define ITEM_SCROLL      2
#define ITEM_WAND        3
#define ITEM_STAFF       4
#define ITEM_WEAPON      5
#define ITEM_TREASURE    8
#define ITEM_ARMOR       9
#define ITEM_POTION      10
#define ITEM_CLOTHING    11
#define ITEM_FURNITURE   12
#define ITEM_TRASH       13
#define ITEM_CONTAINER   15
#define ITEM_DRINK_CON   17
#define ITEM_KEY         18
#define ITEM_FOOD        19
#define ITEM_MONEY       20
#define ITEM_BOAT        22
#define ITEM_CORPSE_NPC  23
#define ITEM_CORPSE_PC   24
#define ITEM_FOUNTAIN    25
#define ITEM_PILL        26
#define ITEM_PROTECT     27
#define ITEM_MAP         28
#define ITEM_PORTAL      29
#define ITEM_WARP_STONE  30
#define ITEM_ROOM_KEY    31
#define ITEM_GEM         32
#define ITEM_JEWELRY     33
#define ITEM_SHOVEL      34
#define ITEM_FOG         35
#define ITEM_PARCHMENT   36
#define ITEM_SEED        37

/*
 * Extra flags.
 * Used in #OBJECTS.
 */
#define ITEM_GLOW          (A)
#define ITEM_HUM           (B)
#define ITEM_DARK          (C)
#define ITEM_LOCK          (D)
#define ITEM_EVIL          (E)
#define ITEM_INVIS         (F)
#define ITEM_MAGIC         (G)
#define ITEM_NODROP        (H)
#define ITEM_BLESS         (I)
#define ITEM_ANTI_GOOD     (J)
#define ITEM_ANTI_EVIL     (K)
#define ITEM_ANTI_NEUTRAL  (L)
#define ITEM_NOREMOVE      (M)
#define ITEM_INVENTORY     (N)
#define ITEM_NOPURGE       (O)
#define ITEM_ROT_DEATH     (P)
#define ITEM_VIS_DEATH     (Q)
#define ITEM_BURIED        (R)
#define ITEM_NONMETAL      (S)
#define ITEM_NOLOCATE      (T)
#define ITEM_MELT_DROP     (U)
#define ITEM_HAD_TIMER     (V)
#define ITEM_SELL_EXTRACT  (W)
//                         (X)
#define ITEM_BURN_PROOF    (Y)
#define ITEM_NOUNCURSE     (Z)

/*
 * Wear flags.
 * Used in #OBJECTS.
 */
#define ITEM_TAKE         (A)
#define ITEM_WEAR_FINGER  (B)
#define ITEM_WEAR_NECK    (C)
#define ITEM_WEAR_BODY    (D)
#define ITEM_WEAR_HEAD    (E)
#define ITEM_WEAR_LEGS    (F)
#define ITEM_WEAR_FEET    (G)
#define ITEM_WEAR_HANDS   (H)
#define ITEM_WEAR_ARMS    (I)
#define ITEM_WEAR_SHIELD  (J)
#define ITEM_WEAR_ABOUT   (K)
#define ITEM_WEAR_WAIST   (L)
#define ITEM_WEAR_WRIST   (M)
#define ITEM_WIELD        (N)
#define ITEM_HOLD         (O)
#define ITEM_NO_SAC       (P)
#define ITEM_WEAR_FLOAT   (Q)

/* weapon class */
#define WEAPON_EXOTIC   0
#define WEAPON_SWORD    1
#define WEAPON_DAGGER   2
#define WEAPON_SPEAR    3
#define WEAPON_MACE     4
#define WEAPON_AXE      5
#define WEAPON_FLAIL    6
#define WEAPON_WHIP     7
#define WEAPON_POLEARM  8

/* weapon types */
#define WEAPON_FLAMING    (A)
#define WEAPON_FROST      (B)
#define WEAPON_VAMPIRIC   (C)
#define WEAPON_SHARP      (D)
#define WEAPON_VORPAL     (E)
#define WEAPON_TWO_HANDS  (F)
#define WEAPON_SHOCKING   (G)
#define WEAPON_POISON     (H)
#define WEAPON_LEECH      (I)
#define WEAPON_STUN       (J)

/* gate flags */
#define GATE_NORMAL_EXIT  (A)
#define GATE_NOCURSE      (B)
#define GATE_GOWITH       (C)
#define GATE_BUGGY        (D)
#define GATE_RANDOM       (E)

/* furniture flags */
#define STAND_AT          (A)
#define STAND_ON          (B)
#define STAND_IN          (C)
#define SIT_AT            (D)
#define SIT_ON            (E)
#define SIT_IN            (F)
#define REST_AT           (G)
#define REST_ON           (H)
#define REST_IN           (I)
#define SLEEP_AT          (J)
#define SLEEP_ON          (K)
#define SLEEP_IN          (L)
#define PUT_AT            (M)
#define PUT_ON            (N)
#define PUT_IN            (O)
#define PUT_INSIDE        (P)

/*
 * Apply types (for affects).
 * Used in #OBJECTS.
 */
#define APPLY_NONE            0
#define APPLY_STR             1
#define APPLY_DEX             2
#define APPLY_INT             3
#define APPLY_WIS             4
#define APPLY_CON             5
#define APPLY_SEX             6
#define APPLY_CLASS           7
#define APPLY_LEVEL           8
#define APPLY_AGE             9
#define APPLY_HEIGHT         10
#define APPLY_WEIGHT         11
#define APPLY_MANA           12
#define APPLY_HIT            13
#define APPLY_MOVE           14
#define APPLY_GOLD           15
#define APPLY_EXP            16
#define APPLY_AC             17
#define APPLY_HITROLL        18
#define APPLY_DAMROLL        19
#define APPLY_SAVES          20
#define APPLY_SPELL_AFFECT   25

/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
#define CONT_CLOSEABLE   1
#define CONT_PICKPROOF   2
#define CONT_CLOSED      4
#define CONT_LOCKED      8
#define CONT_PUT_ON     16

/*
 * Well known room virtual numbers.
 * Defined in #ROOMS.
 */
#define ROOM_VNUM_LIMBO             2
#define ROOM_VNUM_ETHEREAL_PLANE    3
#define ROOM_VNUM_CHAT           1200
#define ROOM_VNUM_TEMPLE         3001
#define ROOM_VNUM_ALTAR          3054
#define ROOM_VNUM_SCHOOL         3700

/*
 * Clan specifc altar's and recalls
 */
#define ROOM_VNUM_SYLVAN_ALTAR     10560
#define ROOM_VNUM_SYLVAN_RECALL     1815
#define ROOM_VNUM_WARHAMMER_ALTAR  10607
#define ROOM_VNUM_WARHAMMER_RECALL   903
#define ROOM_VNUM_REDOAK_ALTAR       704

/*
 * Clan specfic flags for players
 */
#define CLAN_LEADER       (A)
#define CLAN_RECRUITER    (B)
#define CLAN_EXILE        (C)

/*
 * Room flags.
 * Used in #ROOMS.
 */
#define ROOM_DARK         (A)
#define ROOM_ARENA        (B) // Arena where player can die and not lose items
#define ROOM_NO_MOB       (C)
#define ROOM_INDOORS      (D)
#define ROOM_NO_GATE      (E)
#define ROOM_GUILD        (F) // A guild room for a class (the actually guild vnums are defined in the class).
#define ROOM_PRIVATE      (J)
#define ROOM_SAFE         (K)
#define ROOM_SOLITARY     (L)
#define ROOM_PET_SHOP     (M)
#define ROOM_NO_RECALL    (N)
#define ROOM_IMP_ONLY     (O)
#define ROOM_GODS_ONLY    (P)
#define ROOM_HEROES_ONLY  (Q)
#define ROOM_NEWBIES_ONLY (R)
#define ROOM_LAW          (S)
#define ROOM_NOWHERE      (T)

/*
 * Directions.
 * Used in #ROOMS.  (if direction is added update MAX_DIR with 1 plus the max number below)
 */
#define DIR_NORTH     0
#define DIR_EAST      1
#define DIR_SOUTH     2
#define DIR_WEST      3
#define DIR_UP        4
#define DIR_DOWN      5
#define DIR_NORTHWEST 6
#define DIR_NORTHEAST 7
#define DIR_SOUTHWEST 8
#define DIR_SOUTHEAST 9

/*
 * Exit flags.
 * Used in #ROOMS.
 */
#define EX_ISDOOR         (A)
#define EX_CLOSED         (B)
#define EX_LOCKED         (C)
#define EX_PICKPROOF      (F)
#define EX_NOPASS         (G)
#define EX_EASY           (H)
#define EX_HARD           (I)
#define EX_INFURIATING    (J)
#define EX_NOCLOSE        (K)
#define EX_NOLOCK         (L)

/*
 * Sector types.
 * Used in #ROOMS.
 */
#define SECT_INSIDE         0
#define SECT_CITY           1
#define SECT_FIELD          2
#define SECT_FOREST         3
#define SECT_HILLS          4
#define SECT_MOUNTAIN       5
#define SECT_WATER_SWIM     6
#define SECT_WATER_NOSWIM   7
#define SECT_UNUSED         8
#define SECT_AIR            9
#define SECT_DESERT        10
#define SECT_OCEAN         11
#define SECT_BEACH         12
#define SECT_UNDERWATER    13
#define SECT_UNDERGROUND   14
#define SECT_MAX           15

/*
 * Equpiment wear locations.
 * Used in #RESETS.
 */
#define WEAR_NONE            -1
#define WEAR_LIGHT            0
#define WEAR_FINGER_L         1
#define WEAR_FINGER_R         2
#define WEAR_NECK_1           3
#define WEAR_NECK_2           4
#define WEAR_BODY             5
#define WEAR_HEAD             6
#define WEAR_LEGS             7
#define WEAR_FEET             8
#define WEAR_HANDS            9
#define WEAR_ARMS            10
#define WEAR_SHIELD          11
#define WEAR_ABOUT           12
#define WEAR_WAIST           13
#define WEAR_WRIST_L         14
#define WEAR_WRIST_R         15
#define WEAR_WIELD           16
#define WEAR_HOLD            17
#define WEAR_FLOAT           18
#define WEAR_SECONDARY_WIELD 19
#define MAX_WEAR             20

#define CONTINENT_ETHEREAL_PLANE       0
#define CONTINENT_MIDGAARD    1
#define CONTINENT_ARCANIS     2
#define CONTINENT_OCEANS      3

/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (End of this section ... stop here)                   *
 *                                                                         *
 ***************************************************************************/

/*
 * Conditions.
 */
#define COND_DRUNK   0
#define COND_FULL    1
#define COND_THIRST  2
#define COND_HUNGER  3

/*
 * Positions.
 */
#define POS_DEAD      0
#define POS_MORTAL    1
#define POS_INCAP     2
#define POS_STUNNED   3
#define POS_SLEEPING  4
#define POS_RESTING   5
#define POS_SITTING   6
#define POS_FIGHTING  7
#define POS_STANDING  8

// Battle Stances
#define STANCE_NORMAL       0
#define STANCE_DEFENSIVE    1
#define STANCE_OFFENSIVE    2

/*
 * ACT bits for players.
 */
#define PLR_IS_NPC          (A)        /* Don't EVER set.    */

/* WANTED combines and replaces KILLER and THIEF */
#define PLR_WANTED          (B)

/* RT auto flags */
#define PLR_AUTOASSIST      (C)
#define PLR_AUTOEXIT        (D)
#define PLR_AUTOLOOT        (E)
#define PLR_AUTOSAC         (F)
#define PLR_AUTOGOLD        (G)
#define PLR_AUTOSPLIT       (H)
#define PLR_AUTOQUIT        (I)
#define PLR_QUESTOR         (J)

/* Special Flags */
#define PLR_TESTER          (M)

/* RT personal flags */
#define PLR_HOLYLIGHT       (N)
#define PLR_CANLOOT         (P)
#define PLR_NOSUMMON        (Q)
#define PLR_NOFOLLOW        (R)
#define PLR_NOCANCEL        (S)
#define PLR_COLOR           (T)
/* 1 bit reserved, S */

/* penalty flags */
#define PLR_PERMIT          (U)
#define PLR_LOG             (W)
#define PLR_DENY            (X)
#define PLR_FREEZE          (Y)

/* RT comm flags -- may be used on both mobs and chars */
#define COMM_QUIET          (A)
#define COMM_DEAF           (B)
#define COMM_NOWIZ          (C)
#define COMM_NOAUCTION      (D)
#define COMM_NOGOSSIP       (E)
#define COMM_NOQUESTION     (F)
#define COMM_NOOOC          (G)
#define COMM_NOCLAN         (H)
#define COMM_NOCGOSSIP      (I)
#define COMM_NOOCLAN        (J)
#define COMM_NOPRAY         (K)

/* display flags */
#define COMM_COMPACT        (L)
#define COMM_BRIEF          (M)
#define COMM_PROMPT         (N)
#define COMM_COMBINE        (O)
#define COMM_TELNET_GA      (P)
#define COMM_SHOW_AFFECTS   (Q)
#define COMM_NOGRATS        (R)
#define COMM_LINEFEED_TICK  (S)

/* penalties */
#define COMM_NOEMOTE        (T)
#define COMM_NOSHOUT        (U)
#define COMM_NOTELL         (V)
#define COMM_NOCHANNELS     (W)
#define COMM_SNOOP_PROOF    (Y)
#define COMM_AFK            (Z)

/* WIZnet flags */
#define WIZ_ON              (A)
#define WIZ_TICKS           (B)
#define WIZ_LOGINS          (C)
#define WIZ_SITES           (D)
#define WIZ_LINKS           (E)
#define WIZ_DEATHS          (F)
#define WIZ_RESETS          (G)
#define WIZ_MOBDEATHS       (H)
#define WIZ_FLAGS           (I)
#define WIZ_PENALTIES       (J)
#define WIZ_SACCING         (K)
#define WIZ_LEVELS          (L)
#define WIZ_SECURE          (M)
#define WIZ_SWITCHES        (N)
#define WIZ_SNOOPS          (O)
#define WIZ_RESTORE         (P)
#define WIZ_LOAD            (Q)
#define WIZ_NEWBIE          (R)
#define WIZ_PREFIX          (S)
#define WIZ_SPAM            (T)
#define WIZ_GENERAL         (U)
#define WIZ_BANK            (V)
#define WIZ_BUGS            (W)
#define WIZ_DEBUG           (X)

// Merits
#define MERIT_MAGIC_AFFINITY     (A)
#define MERIT_MAGIC_PROTECTION   (B)
#define MERIT_DAMAGE_REDUCTION   (C)
#define MERIT_LIGHT_FOOTED       (D)
#define MERIT_PERCEPTION         (E)
#define MERIT_PRECISION_STRIKING (F)
#define MERIT_FAST_LEARNER       (G)
#define MERIT_HEALTHY            (H)
#define MERIT_CONSTITUTION       (I)
#define MERIT_INTELLIGENCE       (J)
#define MERIT_WISDOM             (K)
#define MERIT_STRENGTH           (L)
#define MERIT_DEXTERITY          (M)
#define MERIT_INCREASED_DAMAGE   (N)

// Clans (these correspond to the entries in the clan_table)
#define CLAN_NONE          0
#define CLAN_LONER         1
#define CLAN_RENEGADE      2
#define CLAN_KNIGHTS       3
#define CLAN_MALICE        4
#define CLAN_VALOR         5
#define CLAN_CULT          6
#define CLAN_SYLVAN        7
#define CLAN_WAR_HAMMER    8
#define CLAN_CONCLAVE      9

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct    mob_index_data
{
    MOB_INDEX_DATA *  next;
    SPEC_FUN *        spec_fun;
    SHOP_DATA *       pShop;
    MPROG_LIST *      mprogs;
    AREA_DATA *       area;        /* OLC */
    int               vnum;
    int               group;
    int               count;
    int               killed;
    char *            player_name;
    char *            short_descr;
    char *            long_descr;
    char *            description;
    long              act;
    long              affected_by;
    int               alignment;
    int               level;
    int               hitroll;
    int               hit[3];
    int               mana[3];
    int               damage[3];
    int               ac[4];
    int               dam_type;
    long              off_flags;
    long              imm_flags;
    long              res_flags;
    long              vuln_flags;
    int               start_pos;
    int               default_pos;
    int               sex;
    int               race;
    long              wealth;
    long              form;
    long              parts;
    int               size;
    char *            material;
    long              mprog_flags;
};

/* memory settings */
#define MEM_CUSTOMER  A
#define MEM_SELLER    B
#define MEM_HOSTILE   C
#define MEM_AFRAID    D

/* memory for mobs */
struct mem_data
{
    MEM_DATA     *next;
    bool          valid;
    int           id;
    int           reaction;
    time_t        when;
};

/*
 * One character (PC or NPC).
 */
struct    char_data
{
    CHAR_DATA *        next;
    CHAR_DATA *        next_in_room;
    CHAR_DATA *        master;
    CHAR_DATA *        leader;
    CHAR_DATA *        fighting;
    CHAR_DATA *        reply;
    CHAR_DATA *        pet;
    CHAR_DATA *        mprog_target;
    MEM_DATA *         memory;
    SPEC_FUN *         spec_fun;
    MOB_INDEX_DATA *   pIndexData;
    DESCRIPTOR_DATA *  desc;
    AFFECT_DATA *      affected;
    OBJ_DATA *         carrying;
    OBJ_DATA *         on;
    ROOM_INDEX_DATA *  in_room;
    ROOM_INDEX_DATA *  was_in_room;
    AREA_DATA *        zone;
    PC_DATA *          pcdata;
    GEN_DATA *         gen_data;
    NOTE_DATA *        pnote;
    bool               valid;
    char *             name;
    long               id;
    int                version;
    char *             short_descr;
    char *             long_descr;
    char *             description;
    char *             prompt;
    char *             prefix;
    int                group;
    int                clan;
    int                sex;
    int                class;
    int                race;
    int                level;
    int                trust;
    int                played;
    int                lines;  /* for the pager */
    time_t             logon;
    int                timer;
    int                wait;
    int                daze;
    int                hit;
    int                max_hit;
    int                mana;
    int                max_mana;
    int                move;
    int                max_move;
    long               gold;
    long               silver;
    int                exp;
    long               act;
    long               comm;   /* RT added to pad the vector */
    long               wiznet; /* wiz stuff */
    long               imm_flags;
    long               res_flags;
    long               vuln_flags;
    int                invis_level;
    int                incog_level;
    long               affected_by;
    int                position;
    int                practice;
    int                train;
    int                carry_weight;
    int                carry_number;
    int                saving_throw;
    int                alignment;
    int                hitroll;
    int                damroll;
    int                armor[4];
    int                wimpy;
    int                stance;
    /* stats */
    int                perm_stat[MAX_STATS];
    int                mod_stat[MAX_STATS];
    /* parts stuff */
    long               form;
    long               parts;
    int                size;
    char*              material;
    /* mobile stuff */
    long               off_flags;
    int                damage[3];
    int                dam_type;
    int                start_pos;
    int                default_pos;
    int                mprog_delay;
    /* Smaug style timer */
    TIMER *            first_timer;
    TIMER *            last_timer;
    int                substate;
    char *             timer_buf;    // Reserved for use with timer callbacks
};

/*
 * Data which only PC's have.
 */
struct pc_data
{
    PC_DATA *       next;
    BUFFER *        buffer;
    bool            valid;
    char *          pwd;
    char *          bamfin;
    char *          bamfout;
    char *          title;
    char *          email;
    int             perm_hit;
    int             perm_mana;
    int             perm_move;
    int             true_sex;
    int	            last_level;
    int             condition[4];
    int             learned[MAX_SKILL];
    bool            group_known[MAX_GROUP];
    int             points;
    bool            confirm_delete;
    char *          alias[MAX_ALIAS];
    char *          alias_sub[MAX_ALIAS];
    int	            security;               /* OLC - Builder security */
    bool            is_reclassing;          /* Whether or not the user is currently reclassing */
    time_t          last_note;
    time_t          last_penalty;
    time_t          last_news;
    time_t          last_change;
    time_t          last_ooc;
    time_t          last_story;
    time_t          last_history;
    time_t          last_immnote;
    int             pk_timer;          // How many ticks the player has to wait to quit after an event like pk.
    char *          last_ip;           // Saves the last IP address used, see save.c for notes.
    int             recall_vnum;       // Custom recall point that can be set by the user to any bind stone
    CHAR_DATA *     quest_giver;       // Vassago - Questing
    int             quest_points;      // Vassago - Questing
    int             next_quest;        // Vassago - Questing
    int             countdown;         // Vassago - Questing
    int             quest_obj;         // Vassago - Questing
    int             quest_mob;         // Vassago - Questing
    int             pkills;            // The number of player kills a character has.
    int             pkills_arena;      // The number of player kills a character has in the arena.
    int             pkilled;           // The number of times a player has been killed.
    long            bank_gold;         // The amount of gold a player has in the bank.
    int             vnum_clairvoyance; // If the user has set a clairvoyance vnum, this is it
    int             priest_rank;       // If the user is a priest, this is the integer value of their rank.
    int             deity;             // The deity if any that the user follows
    long            merit;
    int             key_ring[10];      // The keys that can be stored on a key ring.
    int             improve_focus_gsn; // GSN of the skill the player is focusing on to improve.
    int             improve_minutes;   // Number of minutes until the next improve for the player.
    int             clan_flags;        // Attributes that only a PC clanner can have
    int             clan_rank;         // The clan rank the player has if they are in a clan.
    CHAR_DATA *     consent;           // A player (online) who has been given consent to execute a command (such as guilding you)
};

/* Data for generating characters -- only used during generation */
struct gen_data
{
    GEN_DATA *  next;
    bool        valid;
    bool        skill_chosen[MAX_SKILL];
    bool        group_chosen[MAX_GROUP];
    int         points_chosen;
};

/*
 * Liquids.
 */
#define LIQ_WATER        0

struct    liquid_type
{
    char *    liq_name;
    char *    liq_color;
    int       liq_affect[5];
};

/*
 * Extra description data for a room or object.
 */
struct    extra_descr_data
{
    EXTRA_DESCR_DATA *  next;         /* Next in list            */
    bool                valid;
    char *              keyword;      /* Keyword in look/examine */
    char *              description;  /* What to see             */
};

/*
 * Prototype for an object.
 */
struct    obj_index_data
{
    OBJ_INDEX_DATA *    next;
    EXTRA_DESCR_DATA *  extra_descr;
    AFFECT_DATA *       affected;
    AREA_DATA *         area;        /* OLC */
    char *              name;
    char *              short_descr;
    char *              description;
    int                 vnum;
    int                 reset_num;
    char *              material;
    int                 item_type;
    int                 extra_flags;
    int                 wear_flags;
    int                 level;
    int                 condition;
    int                 count;
    int                 weight;
    int                 cost;
    int                 value[5];
};

/*
 * One object.
 */
struct    obj_data
{
    OBJ_DATA *          next;
    OBJ_DATA *          next_content;
    OBJ_DATA *          contains;
    OBJ_DATA *          in_obj;
    OBJ_DATA *          on;
    CHAR_DATA *         carried_by;
    EXTRA_DESCR_DATA *  extra_descr;
    AFFECT_DATA *       affected;
    OBJ_INDEX_DATA *    pIndexData;
    ROOM_INDEX_DATA *   in_room;
    bool                valid;
    bool                enchanted;
    char *              owner;
    char *              name;
    char *              short_descr;
    char *              description;
    char *              enchanted_by; /* The last enchantor that enchanted the item */
    char *              wizard_mark;  /* In case the item is wizard marked, it can always be located by the enchantor */
    int                 item_type;
    int                 extra_flags;
    int                 wear_flags;
    int                 wear_loc;
    int                 weight;
    int                 cost;
    int                 level;
    int                 condition;
    char *              material;
    int                 timer;
    int                 value[5];
    int                 count;
};

/*
 * Exit data.
 */
struct    exit_data
{
    union
    {
        ROOM_INDEX_DATA *  to_room;
        int             vnum;
    } u1;
    int          exit_info;
    int          key;
    char *       keyword;
    char *       description;
    EXIT_DATA *  next;        /* OLC */
    int          rs_flags;    /* OLC */
    int          orig_door;   /* OLC */
    bool         color;       /* Path Find */
};

/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'D': set state of door
 *   'R': randomize room exits
 *   'S': stop (end of list)
 */

/*
 * Area-reset definition.
 */
struct    reset_data
{
    RESET_DATA *  next;
    char          command;
    int           arg1;
    int           arg2;
    int           arg3;
    int           arg4;
};

/*
 * Area definition.
 */
struct    area_data
{
    AREA_DATA *   next;
    HELP_AREA *   helps;
    char *        file_name;
    char *        name;
    char *        credits;
    int           age;
    int           nplayer;
    int           min_level;
    int           max_level;
    int           min_vnum;
    int           max_vnum;
    bool          empty;
    char *        builders;    /* OLC */ /* Listing of */
    int           vnum;        /* OLC */ /* Area vnum  */
    int           area_flags;  /* OLC */
    int           security;    /* OLC */ /* Value 1-9  */
    int           continent;
};

/*
 * Room type.
 */
struct    room_index_data
{
    ROOM_INDEX_DATA *    next;
    CHAR_DATA *          people;
    OBJ_DATA *           contents;
    EXTRA_DESCR_DATA *   extra_descr;
    AREA_DATA *          area;
    EXIT_DATA *          exit[10];
    RESET_DATA *         reset_first;    /* OLC */
    RESET_DATA *         reset_last;     /* OLC */
    char *               name;
    char *               description;
    char *               owner;
    int                  vnum;
    int                  room_flags;
    int                  light;
    int                  sector_type;
    int                  heal_rate;
    int                  mana_rate;
    int                  clan;
    int                  heap_index;     /* Path Find */
    int                  steps;          /* Path Find */
    bool                 visited;        /* Path Find */

};

/*
 * Timer data - from Smaug
 */
struct timer_data
{
    TIMER  *    prev;
    TIMER  *    next;
    DO_FUN *    do_fun;
    int         value;
    int         type;
    int         count;
    char *      cmd;
};

/*
 * Types of attacks.
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
#define TYPE_UNDEFINED               -1
#define TYPE_HIT                     1000

/*
 *  Target types.
 */
#define TAR_IGNORE             0
#define TAR_CHAR_OFFENSIVE     1
#define TAR_CHAR_DEFENSIVE     2
#define TAR_CHAR_SELF          3
#define TAR_OBJ_INV            4
#define TAR_OBJ_CHAR_DEF       5
#define TAR_OBJ_CHAR_OFF       6

#define TARGET_CHAR            0
#define TARGET_OBJ             1
#define TARGET_ROOM            2
#define TARGET_NONE            3

/*
 * Skills include spells as a particular case.
 */
struct skill_type
{
    char *      name;                      /* Name of skill                */
    int         skill_level[MAX_CLASS];    /* Level needed by class        */
    int         rating[MAX_CLASS];         /* How hard it is to learn      */
    SPELL_FUN * spell_fun;                 /* Spell pointer (for spells)   */
    int         target;                    /* Legal targets                */
    int         minimum_position;          /* Position for caster / user   */
    int         min_mana;                  /* Minimum mana used            */
    int         beats;                     /* Waiting time after use       */
    char *      noun_damage;               /* Damage message               */
    char *      msg_off;                   /* Wear off message             */
    char *      msg_obj;                   /* Wear off message for obects  */
    int         race;                      /* Specific race if the skill is only for one race */
    bool        ranged;                    /* Whether or not this skill/spell is ranged */
};

struct  group_type
{
    char *    name;
    int       rating[MAX_CLASS];
    char *    spells[MAX_IN_GROUP];
};

/*
 * MOBprog definitions
 */
#define TRIG_ACT      (A)
#define TRIG_BRIBE    (B)
#define TRIG_DEATH    (C)
#define TRIG_ENTRY    (D)
#define TRIG_FIGHT    (E)
#define TRIG_GIVE     (F)
#define TRIG_GREET    (G)
#define TRIG_GRALL    (H)
#define TRIG_KILL     (I)
#define TRIG_HPCNT    (J)
#define TRIG_RANDOM   (K)
#define TRIG_SPEECH   (L)
#define TRIG_EXIT     (M)
#define TRIG_EXALL    (N)
#define TRIG_DELAY    (O)
#define TRIG_SURR     (P)

struct mprog_list
{
    int           trig_type;
    char *        trig_phrase;
    int           vnum;
    char *        code;
    char *        name;
    MPROG_LIST *  next;
    bool          valid;
};

struct mprog_code
{
    int           vnum;  // The unique vnum of the mob program
    char *        name;  // The name identifier to display
    char *        code;  // The actual code of the mob prog
    MPROG_CODE *  next;
};

/*
 * Utility macros.
 */
#define IS_VALID(data)      ((data) != NULL && (data)->valid)
#define VALIDATE(data)      ((data)->valid = TRUE)
#define INVALIDATE(data)    ((data)->valid = FALSE)
#define UMIN(a, b)          ((a) < (b) ? (a) : (b))
#define UMAX(a, b)          ((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)     ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define LOWER(c)            ((c) >= 'A' && (c) <= 'Z' ? (c)+'a'-'A' : (c))
#define UPPER(c)            ((c) >= 'a' && (c) <= 'z' ? (c)+'A'-'a' : (c))
#define IS_SET(flag, bit)   ((flag) & (bit))
#define SET_BIT(var, bit)   ((var) |= (bit))
#define REMOVE_BIT(var, bit)((var) &= ~(bit))
#define IS_NULLSTR(str)     ((str) == NULL || (str)[0] == '\0')
#define ENTRE(min,num,max)  (((min) < (num)) && ((num) < (max)) )
#define CHANCE(percent)     (number_range(1,100) < percent)
#define CHANCE_SKILL(ch, sn)(number_percent() < get_skill(ch, sn))
#define CHECK_POS(a, b, c)    {                            \
                    (a) = (b);                    \
                    if ( (a) < 0 )                    \
                    bug( "CHECK_POS : " c " == %d < 0", a );    \
                }                            \

#define LINK(link, first, last, next, prev)                     \
do                                                              \
{                                                               \
    if ( !(first) )                                             \
      (first)                   = (link);                       \
    else                                                        \
      (last)->next              = (link);                       \
    (link)->next                = NULL;                         \
    (link)->prev                = (last);                       \
    (last)                      = (link);                       \
} while(0)


#define UNLINK(link, first, last, next, prev)                   \
do                                                              \
{                                                               \
    if ( !(link)->prev )                                        \
      (first)                   = (link)->next;                 \
    else                                                        \
      (link)->prev->next        = (link)->next;                 \
    if ( !(link)->next )                                        \
      (last)                    = (link)->prev;                 \
    else                                                        \
      (link)->next->prev        = (link)->prev;                 \
} while(0)


/*
 * Defines for extended bitvectors
 */
#ifndef INTBITS
#define INTBITS       32
#endif
#define XBM             31      /* extended bitmask   ( INTBITS - 1 )   */
#define RSV             5       /* right-shift value  ( sqrt(XBM+1) )   */
#define XBI             4       /* integers in an extended bitvector    */
#define MAX_BITS        XBI * INTBITS

/*
 * Structure for extended bitvectors -- Thoric (Smaug)
 */
struct extended_bitvector
{
    int         bits[XBI];
};

/*
 * The functions for these prototypes can be found in misc.c
 * They are up here because they are used by the macros below
 */
bool    ext_is_empty            (EXT_BV *bits);
void    ext_clear_bits          (EXT_BV *bits);
int     ext_has_bits            (EXT_BV *var, EXT_BV *bits);
bool    ext_same_bits           (EXT_BV *var, EXT_BV *bits);
void    ext_set_bits            (EXT_BV *var, EXT_BV *bits);
void    ext_remove_bits         (EXT_BV *var, EXT_BV *bits);
void    ext_toggle_bits         (EXT_BV *var, EXT_BV *bits);
char    *stristr                (const char *str1, const char *str2);
/*
 * Here are the extended bitvector macros:
 */
#define xIS_SET(var, bit)       ((var).bits[(bit) >> RSV] & 1 << ((bit) & XBM))
#define xSET_BIT(var, bit)      ((var).bits[(bit) >> RSV] |= 1 << ((bit) & XBM))
#define xSET_BITS(var, bit)     (ext_set_bits(&(var), &(bit)))
#define xREMOVE_BIT(var, bit)   ((var).bits[(bit) >> RSV] &= ~(1 << ((bit) & XBM)))
#define xREMOVE_BITS(var, bit)  (ext_remove_bits(&(var), &(bit)))
#define xTOGGLE_BIT(var, bit)   ((var).bits[(bit) >> RSV] ^= 1 << ((bit) & XBM))
#define xTOGGLE_BITS(var, bit)  (ext_toggle_bits(&(var), &(bit)))
#define xCLEAR_BITS(var)        (ext_clear_bits(&(var)))
#define xIS_EMPTY(var)          (ext_is_empty(&(var)))
#define xHAS_BITS(var, bit)     (ext_has_bits(&(var), &(bit)))
#define xSAME_BITS(var, bit)    (ext_same_bits(&(var), &(bit)))

/*
 * Character macros.
 */
#define IS_NPC(ch)           (IS_SET((ch)->act, ACT_IS_NPC))
#define IS_IMMORTAL(ch)      (get_trust(ch) >= LEVEL_IMMORTAL)
#define IS_HERO(ch)          (get_trust(ch) >= LEVEL_HERO)
#define IS_TRUSTED(ch,level) (get_trust((ch)) >= (level))
#define IS_AFFECTED(ch, sn)  (IS_SET((ch)->affected_by, (sn)))
#define GET_AGE(ch)          ((int) (17 + ((ch)->played + current_time - (ch)->logon )/72000))
#define IS_GOOD(ch)          (ch->alignment == ALIGN_GOOD)
#define IS_EVIL(ch)          (ch->alignment == ALIGN_EVIL)
#define IS_NEUTRAL(ch)       (!IS_GOOD(ch) && !IS_EVIL(ch))
#define IS_AWAKE(ch)         (ch->position > POS_SLEEPING)
#define GET_AC(ch,type)        ((ch)->armor[type]                \
                + ( IS_AWAKE(ch)                \
            ? dex_app[get_curr_stat(ch,STAT_DEX)].defensive : 0 ))
#define GET_HITROLL(ch, obj)    \
    ((ch)->hitroll + hit_adjustment(ch) + obj_affect_modifier(obj, APPLY_HITROLL))
#define GET_DAMROLL(ch, obj) \
    ((ch)->damroll + damage_adjustment(ch) + obj_affect_modifier(obj, APPLY_DAMROLL ))

#define IS_OUTSIDE(ch)        (!IS_SET(                    \
                    (ch)->in_room->room_flags,            \
                    ROOM_INDOORS))

#define WAIT_STATE(ch, npulse)    ((ch)->wait = UMAX((ch)->wait, (npulse)))
#define DAZE_STATE(ch, npulse)  ((ch)->daze = UMAX((ch)->daze, (npulse)))
#define get_carry_weight(ch)    ((ch)->carry_weight + (ch)->silver/10 +  \
                              (ch)->gold * 2 / 5)

#define act(format,ch,arg1,arg2,type)\
    act_new((format),(ch),(arg1),(arg2),(type),POS_RESTING)

#define HAS_TRIGGER(ch,trig)    (IS_SET((ch)->pIndexData->mprog_flags,(trig)))
#define IS_SWITCHED( ch )       ( ch->desc && ch->desc->original )
#define IS_BUILDER(ch, Area)    ( !IS_NPC(ch) && !IS_SWITCHED( ch ) &&      \
                ( ch->pcdata->security >= Area->security  \
                || strstr( Area->builders, ch->name )      \
                || strstr( Area->builders, "All" ) ) )
#define IS_WANTED(ch) (IS_SET(ch->act, PLR_WANTED))
#define IS_TESTER(ch) (IS_SET(ch->act, PLR_TESTER) || settings.test_mode)
#define IS_GHOST(ch)  (is_affected(ch,gsn_ghost))
#define IS_DAY()      (time_info.hour > 5 && time_info.hour < 20)
#define IS_NIGHT()    (!IS_DAY())
#define IS_FIGHTING(ch) (ch->fighting != NULL)

// Sub races
// All elves minus dark elves
#define IS_ELF(ch) (ch->race == ELF_RACE || ch->race == HALF_ELF_RACE || ch->race == WILD_ELF_RACE || ch->race == SEA_ELF_RACE)
#define IS_DWARF(ch) (ch->race == DWARF_RACE || ch->race == MOUNTAIN_DWARF_RACE || ch->race == HILL_DWARF_RACE || ch->race == DARK_DWARF_RACE || ch->race == GULLY_DWARF_RACE)
#define IS_OGRE(ch) (ch->race == OGRE_RACE || ch->race == GIANT_OGRE_RACE)

/*
 * Object macros.
 */
#define CAN_WEAR(obj, part)    (IS_SET((obj)->wear_flags,  (part)))
#define IS_OBJ_STAT(obj, stat)    (IS_SET((obj)->extra_flags, (stat)))
#define IS_WEAPON_STAT(obj,stat)(IS_SET((obj)->value[4],(stat)))
#define WEIGHT_MULT(obj)    ((obj)->item_type == ITEM_CONTAINER ? \
    (obj)->value[4] : 100)
#define IS_PIT(obj) (obj->item_type == ITEM_CONTAINER && IS_OBJ_STAT(obj,ITEM_NOPURGE))

/*
 * Description macros.
 */
#define PERS(ch, looker)    ( can_see( looker, (ch) ) ?        \
                ( IS_NPC(ch) ? (ch)->short_descr               \
                : pers(ch, looker)) : (ch)->level >= LEVEL_IMMORTAL  && !IS_NPC(ch) ? "(An Imm)" : "someone")

/*
 * Structure for a social in the socials table.
 */
struct    social_type
{
    char      name[20];
    char *    char_no_arg;
    char *    others_no_arg;
    char *    char_found;
    char *    others_found;
    char *    vict_found;
    char *    char_not_found;
    char *    char_auto;
    char *    others_auto;
};

/*
 * Global constants.
 */
extern    const    struct    str_app_type      str_app[26];
extern    const    struct    int_app_type      int_app[26];
extern    const    struct    wis_app_type      wis_app[26];
extern    const    struct    dex_app_type      dex_app[26];
extern    const    struct    con_app_type      con_app[26];

extern    const    struct    weapon_type       weapon_table[];
extern    const    struct    item_type         item_table[];
extern    const    struct    wiznet_type       wiznet_table[];
extern    const    struct    attack_type       attack_table[];
extern    const    struct    race_type         race_table[];
extern             struct    pc_race_type      pc_race_table[];
extern    const    struct    spec_type         spec_table[];
extern    const    struct    liquid_type       liquid_table[];
extern    const    struct    dispel_type       dispel_table[];
extern    const    struct    priest_rank_type  priest_rank_table[];
extern    const    struct    export_flags_type export_flags_table[];
extern             struct    social_type       social_table[MAX_SOCIALS];
extern    const    struct    clan_rank_type    clan_rank_table[];
extern    const    struct    standard_mob_values_type standard_mob_values[];

//extern             struct    skill_type      skill_table    [MAX_SKILL];
//extern    const    struct    class_type      class_table    [MAX_CLASS];
//extern    const    struct    group_type      group_table    [MAX_GROUP];

/*
 * Global variables.
 */
extern HELP_DATA               * help_first;
extern SHOP_DATA               * shop_first;
extern CHAR_DATA               * char_list;
extern DESCRIPTOR_DATA         * descriptor_list;
extern OBJ_DATA                * object_list;
extern MPROG_CODE              * mprog_list;
extern time_t                  current_time;
extern bool                    fLogAll;
extern FILE                    * fpReserve;
extern TIME_INFO_DATA          time_info;
extern WEATHER_DATA            weather_info;
extern SETTINGS_DATA           settings;
extern STATISTICS_DATA         statistics;
extern NOTE_DATA               * note_free;
extern OBJ_DATA                * obj_free;
extern bool                    MOBtrigger;
extern GROUPTYPE               * group_table[MAX_GROUP];
extern CLASSTYPE               * class_table[MAX_CLASS];
extern SKILLTYPE               * skill_table[MAX_SKILL];
extern DISABLED_DATA           * disabled_first;
extern GLOBAL_DATA             global;
extern DB_BUFFER               * db_buffer_list;
extern DB_BUFFER               * db_buffer_last;

/*
 * OS-dependent declarations.
 */
#if defined(_WIN32)
    // Support for snprintf and vsnprintf in WIN32
    #define vsnprintf _vsnprintf
    #define snprintf _snprintf

    #if defined(unix)
        #undef unix
    #endif
#endif

/*
 * Data files used by the server.
 *
 * AREA_LIST contains a list of areas to boot.
 * All files are read in completely at bootup.
 * Most output files (bug, idea, typo, shutdown) are append-only.
 *
 * The NULL_FILE is held open so that we have a stream handle in reserve,
 *   so players can go ahead and telnet to all the other descriptors.
 * Then we close it whenever we need to open a file (e.g. a save file).
 */
#if defined(_WIN32)
    #define PLAYER_DIR	     "../player/"        /* Player files */
    #define TEMP_FILE	     "../player/romtmp"
    #define NULL_FILE        "nul"               /* To reserve one stream  */
#endif

#if defined(unix)
    #define PLAYER_DIR       "../player/"         /* Player files           */
    #define GOD_DIR          "../gods/"           /* list of gods           */
    #define TEMP_FILE        "../player/romtmp"
    #define NULL_FILE        "/dev/null"          /* To reserve one stream  */
#endif

#define AREA_LIST            "area.lst"                   /* List of areas */
#define BUG_FILE             "../log/bugs.txt"            /* For 'bug' and bug() */
#define TYPO_FILE            "../log/typos.txt"           /* For 'typo' */
#define OHELPS_FILE	         "../log/orphaned_helps.txt"  /* Unmet 'help' requests */
#define SHUTDOWN_FILE        "shutdown.txt"               /* For 'shutdown'        */

#define GROUP_FILE           "../classes/groups.dat"      /* groups file */
#define CLASS_DIR            "../classes/"                /* classes directory */
#define CLASS_FILE           "class.lst"                  /* classes file */
#define SKILLS_FILE          "../classes/skills.dat"      /* skills and spells file */

#define NOTE_FILE            "../notes/note.note"
#define PENALTY_FILE         "../notes/penalty.note"
#define NEWS_FILE            "../notes/news.note"
#define CHANGES_FILE         "../notes/change.note"
#define OOC_FILE             "../notes/ooc.note"
#define STORY_FILE           "../notes/story.note"
#define HISTORY_FILE         "../notes/history.note"
#define IMMNOTE_FILE	     "../notes/imm.note"

#define BAN_FILE             "../system/ban.dat"
#define SAVED_OBJECT_FILE    "../system/saved_objects.dat"
#define SETTINGS_FILE        "../system/settings.ini"
#define STATISTICS_FILE      "../system/statistics.dat"
#define DISABLED_FILE        "../system/disabled.dat"
#define EXPORT_DATABASE_FILE "../system/game-data.db"

// Exit codes
#define MUD_EXIT_REBOOT          0                        /* Normal exit */
#define MUD_EXIT_HALT            1                        /* Exit due to error or signal */

/*
 * Our function prototypes.
 * One big lump ... this is every function in Merc.
 */
#define CD   CHAR_DATA
#define MID  MOB_INDEX_DATA
#define OD   OBJ_DATA
#define OID  OBJ_INDEX_DATA
#define RID  ROOM_INDEX_DATA
#define SF   SPEC_FUN
#define AD   AFFECT_DATA
#define MPC  MPROG_CODE

/* act_comm.c */
void     add_follower        (CHAR_DATA *ch, CHAR_DATA *master);
void     stop_follower       (CHAR_DATA *ch);
void     nuke_pets           (CHAR_DATA *ch);
void     die_follower        (CHAR_DATA *ch);
bool     is_same_group       (CHAR_DATA *ach, CHAR_DATA *bch);
char     *obj_short          (OBJ_DATA *obj);
void     shutdown_request    (int a);
void     linefeed_update     ();
int      color_strlen        (const char *src);
void     clear_consents      (CHAR_DATA *ch);

/* act_enter.c */
RID      *get_random_room    (CHAR_DATA *ch);

/* act_info.c */
void     set_title           (CHAR_DATA *ch, char *title);
bool     char_in_list        (CHAR_DATA *ch);
char     *pers               (CHAR_DATA *ch, CHAR_DATA *looker);
char     *who_list_formatter (CHAR_DATA *wch);
int      player_online_count (void);
bool     check_fog           (CHAR_DATA * ch);


/* act_move.c */
void move_char(CHAR_DATA *ch, int door, bool follow);
int find_door(CHAR_DATA * ch, char *arg, bool silent);
int direction(char *arg);

/* act_obj.c */
bool     can_loot            (CHAR_DATA *ch, OBJ_DATA *obj);
void     wear_obj            (CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace);
void     get_obj             (CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container);
void     show_lore           (CHAR_DATA *ch, OBJ_DATA *obj);
void     remove_all_obj      (CHAR_DATA *ch);
bool     remove_obj          (CHAR_DATA * ch, int iWear, bool fReplace);
void     wear_obj            (CHAR_DATA * ch, OBJ_DATA * obj, bool fReplace);
bool     empty_obj           (OBJ_DATA *obj, OBJ_DATA *dest_obj, ROOM_INDEX_DATA *dest_room);

/* act_wiz.c */
void     wiznet                    (char *string, CHAR_DATA *ch, OBJ_DATA *obj, long flag, long flag_skip, int min_level);
void     wiznetf                   (int flag, const char *format, ...);
void     copyover_recover          (void);
void     copyover_load_descriptors (void);
void     auto_train                (CHAR_DATA *ch);


/* alias.c */
void     substitute_alias    (DESCRIPTOR_DATA *d, char *input);

/* ban.c */
bool     check_ban           (char *site, int type);

/* comm.c */
void     show_string         (struct descriptor_data *d, char *input);
void     close_socket        (DESCRIPTOR_DATA *dclose);
void     write_to_buffer     (DESCRIPTOR_DATA *d, const char *txt);
void     send_to_desc        (const char *txt, DESCRIPTOR_DATA *d);
void     send_to_char        (const char *txt, CHAR_DATA *ch);
void     page_to_char        (const char *txt, CHAR_DATA *ch);
//void     act                 (const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type);
void     act_new             (const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type, int min_pos);
void     sendf               (CHAR_DATA *, char *, ...);
bool     write_to_descriptor (int desc, char *txt, DESCRIPTOR_DATA *d);
bool     write_to_desc       (char *str, DESCRIPTOR_DATA *d);
void     write_to_all_desc   (char *txt);
void     send_to_all_char    (char *txt);
void     copyover_broadcast  (char *txt, bool show_last_result);
void     writef              (DESCRIPTOR_DATA * d, char *fmt, ...);

/* db.c */
void     reset_area          (AREA_DATA * pArea);        /* OLC */
void     reset_room          (ROOM_INDEX_DATA *pRoom);   /* OLC */
char *   print_flags         (int flag);
void     boot_db             (void);
void     area_update         (void);
CD *     create_mobile       (MOB_INDEX_DATA *pMobIndex);
void     clone_mobile        (CHAR_DATA *parent, CHAR_DATA *clone);
OD *     create_object       (OBJ_INDEX_DATA *pObjIndex);
void     clone_object        (OBJ_DATA *parent, OBJ_DATA *clone);
char *   get_extra_descr     (const char *name, EXTRA_DESCR_DATA *ed);
MID *    get_mob_index       (int vnum);
OID *    get_obj_index       (int vnum);
RID *    get_room_index      (int vnum);
MPC *    get_mprog_index     (int vnum);
char     fread_letter        (FILE *fp);
int      fread_number        (FILE *fp);
GROUPTYPE * fread_group      (FILE *fp);
SKILLTYPE *fread_skill       (FILE *fp);
long     fread_flag          (FILE *fp);
char *   fread_string        (FILE *fp);
char *   fread_string_eol    (FILE *fp);
void     fread_to_eol        (FILE *fp);
char *   fread_word          (FILE *fp);
long     flag_convert        (char letter);
void *   alloc_mem           (size_t sMem);
void *   alloc_perm          (size_t sMem);
void     free_mem            (void *pMem, size_t sMem);
char *   str_dup             (const char *str);
void     free_string         (char *pstr);
int      dice                (int number, int size);
int      interpolate         (int level, int value_00, int value_32);
void     smash_tilde         (char *str);
void     smash_dollar        (char *str);
bool     str_cmp             (const char *astr, const char *bstr);
bool     str_prefix          (const char *astr, const char *bstr);
bool     str_infix           (const char *astr, const char *bstr);
bool     str_suffix          (const char *astr, const char *bstr);
char *   capitalize          (const char *str);
void     append_file         (CHAR_DATA *ch, char *file, char *str);
void     tail_chain          (void);
bool     check_pet_affected  (int vnum, AFFECT_DATA *paf);
void     save_statistics     (void);
void     save_game_objects   (void);

/*  Color stuff by Lope */
char  *strip_color    (char *string);

/* disable.c */
bool  check_disabled  (const struct cmd_type *command);
void  load_disabled   (void);
void  save_disabled   (void);

/* grid.c */
int   count_color     (const char *str);

/* effect.c */
void    acid_effect    (void *vo, int level, int dam, int target);
void    cold_effect    (void *vo, int level, int dam, int target);
void    fire_effect    (void *vo, int level, int dam, int target);
void    poison_effect  (void *vo, int level, int dam, int target);
void    shock_effect   (void *vo, int level, int dam, int target);
bool    stun_effect    (CHAR_DATA *ch, CHAR_DATA *victim);

/* login_menu.c */
void     show_greeting       (DESCRIPTOR_DATA *d);
void     show_login_menu     (DESCRIPTOR_DATA *d);
void     show_menu_header    (char *caption, DESCRIPTOR_DATA *d);
void     show_random_names   (DESCRIPTOR_DATA *d);
void     show_login_who      (DESCRIPTOR_DATA *d);
void     show_login_credits  (DESCRIPTOR_DATA *d);
void     show_menu_top       (DESCRIPTOR_DATA *d);
void     show_menu_bottom    (DESCRIPTOR_DATA *d);

/* random.c */
void     init_random         (void);
int      number_fuzzy        (int number);
int      number_range        (int from, int to);
int      number_percent      (void);
int      number_door         (void);
int      number_bits         (int width);
long     number_mm           (void);
int      dice                (int number, int size);

/* fight.c */
bool    is_safe         (CHAR_DATA *ch, CHAR_DATA *victim);
bool    is_safe_spell   (CHAR_DATA *ch, CHAR_DATA *victim, bool area);
void    violence_update (void);
void    multi_hit       (CHAR_DATA *ch, CHAR_DATA *victim, int dt);
void    one_hit         (CHAR_DATA * ch, CHAR_DATA * victim, int dt, bool dual);
bool    damage          (CHAR_DATA *ch, CHAR_DATA *victim, int dam, int dt, int class, bool show);
void    update_pos      (CHAR_DATA *victim);
void    stop_fighting   (CHAR_DATA *ch, bool fBoth);
void    check_wanted    (CHAR_DATA *ch, CHAR_DATA *victim);
void    check_death     (CHAR_DATA *victim, int dt);
char    *get_stance_name(CHAR_DATA *ch);
int     stance_defensive_modifier (CHAR_DATA *ch);
void    disarm(CHAR_DATA * ch, CHAR_DATA * victim);
void    set_fighting(CHAR_DATA * ch, CHAR_DATA * victim);
int     standard_modifier(CHAR_DATA *ch, CHAR_DATA *victim);
int     damage_adjustment(CHAR_DATA *ch);
int     hit_adjustment(CHAR_DATA *ch);

/* clan.c */
bool   is_clan            (CHAR_DATA *ch);
bool   is_same_clan       (CHAR_DATA *ch, CHAR_DATA *victim);
int    clan_rank_lookup   (const char *name, CHAR_DATA *ch);
void   set_default_rank   (CHAR_DATA *ch);
void   guild_clan         (CHAR_DATA *ch, char *argument);

/* handler.c */
AD    *affect_find        (AFFECT_DATA *paf, int sn);
void   affect_check       (CHAR_DATA *ch, int where, int vector);
int    count_users        (OBJ_DATA *obj);
void   deduct_cost        (CHAR_DATA *ch, int cost);
void   affect_enchant     (OBJ_DATA *obj);
int    check_immune       (CHAR_DATA *ch, int dam_type);
int    material_lookup    (const char *name);
int    weapon_lookup      (const char *name);
int    weapon_type_lookup (const char *name);
char  *weapon_name        (int weapon_Type);
char  *item_name          (int item_type);
int    attack_lookup      (const char *name);
long   wiznet_lookup      (const char *name);
int    class_lookup       (const char *name);
int    get_skill          (CHAR_DATA *ch, int sn);
int    get_weapon_sn      (CHAR_DATA *ch, bool dual);
int    get_weapon_skill   (CHAR_DATA *ch, int sn);
int    get_age            (CHAR_DATA *ch);
void   reset_char         (CHAR_DATA *ch);
int    get_trust          (CHAR_DATA *ch);
int    get_curr_stat      (CHAR_DATA *ch, int stat);
int    get_max_train      (CHAR_DATA *ch, int stat);
int    can_carry_n        (CHAR_DATA *ch);
int    can_carry_w        (CHAR_DATA *ch);
bool   is_name            (char *str, char *namelist);
bool   is_exact_name      (char *str, char *namelist);
void   affect_to_char     (CHAR_DATA *ch, AFFECT_DATA *paf);
void   affect_to_obj      (OBJ_DATA *obj, AFFECT_DATA *paf);
void   affect_remove      (CHAR_DATA *ch, AFFECT_DATA *paf);
void   affect_remove_obj  (OBJ_DATA *obj, AFFECT_DATA *paf);
void   affect_strip       (CHAR_DATA *ch, int sn);
void   affect_strip_all   (CHAR_DATA *ch);
bool   is_affected        (CHAR_DATA *ch, int sn);
void   affect_join        (CHAR_DATA *ch, AFFECT_DATA *paf);
void   char_from_room     (CHAR_DATA *ch);
void   char_to_room       (CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex);
OD *   obj_to_char        (OBJ_DATA *obj, CHAR_DATA *ch);
void   obj_from_char      (OBJ_DATA *obj);
int    apply_ac           (OBJ_DATA *obj, int iWear, int type);
OD *   get_eq_char        (CHAR_DATA *ch, int iWear);
void   equip_char         (CHAR_DATA *ch, OBJ_DATA *obj, int iWear);
void   unequip_char       (CHAR_DATA *ch, OBJ_DATA *obj);
int    count_obj_list     (OBJ_INDEX_DATA *obj, OBJ_DATA *list);
void   obj_from_room      (OBJ_DATA *obj);
OBJ_DATA *obj_to_room     (OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex);
OBJ_DATA *obj_to_obj      (OBJ_DATA *obj, OBJ_DATA *obj_to);
void   obj_from_obj       (OBJ_DATA *obj);
void   extract_obj        (OBJ_DATA *obj);
void   extract_char       (CHAR_DATA *ch, bool fPull);
void   separate_obj       (OBJ_DATA *obj);
void   split_obj          (OBJ_DATA *obj, int num);
void   empty_pits         (void);
CD *   get_char_room      (CHAR_DATA *ch, char *argument);
CD *   get_char_area      (CHAR_DATA *ch, char *argument);
CD *   get_char_world     (CHAR_DATA *ch, char *argument);
OD *   get_obj_type       (OBJ_INDEX_DATA *pObjIndexData);
OD *   get_obj_list       (CHAR_DATA *ch, char *argument, OBJ_DATA *list);
OD *   get_obj_carry      (CHAR_DATA *ch, char *argument, CHAR_DATA *viewer);
OD *   get_obj_wear       (CHAR_DATA *ch, char *argument);
OD *   get_obj_here       (CHAR_DATA *ch, char *argument);
OD *   get_obj_world      (CHAR_DATA *ch, char *argument);
OD *   create_money       (int gold, int silver);
int    get_obj_number     (OBJ_DATA *obj);
int    obj_count_by_type  (OBJ_DATA *obj, int item_type);
int    get_obj_weight     (OBJ_DATA *obj);
int    get_true_weight    (OBJ_DATA *obj);
bool   room_is_dark       (ROOM_INDEX_DATA *pRoomIndex);
bool   is_room_owner      (CHAR_DATA *ch, ROOM_INDEX_DATA *room);
bool   room_is_private    (ROOM_INDEX_DATA *pRoomIndex);
bool   can_see            (CHAR_DATA *ch, CHAR_DATA *victim);
bool   can_see_obj        (CHAR_DATA *ch, OBJ_DATA *obj);
bool   can_see_room       (CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex);
bool   can_drop_obj       (CHAR_DATA *ch, OBJ_DATA *obj);
char * affect_loc_name    (int location);
char * affect_bit_name    (int vector);
char * extra_bit_name     (int extra_flags);
char * wear_bit_name      (int wear_flags);
char * act_bit_name       (int act_flags);
char * off_bit_name       (int off_flags);
char * imm_bit_name       (int imm_flags);
char * form_bit_name      (int form_flags);
char * part_bit_name      (int part_flags);
char * weapon_bit_name    (int weapon_flags);
char * comm_bit_name      (int comm_flags);
char * cont_bit_name      (int cont_flags);
bool   has_item_type      (CHAR_DATA *ch, int item_type);
void   make_ghost         (CHAR_DATA *ch);
char * get_room_name      (int vnum);
char * get_area_name      (int vnum);
bool   same_continent     (int vnum_one, int vnum_two);
bool  same_continent_char (CHAR_DATA *ch, CHAR_DATA *victim);
int    hours_played       (CHAR_DATA *ch);
bool   obj_in_room        (CHAR_DATA *ch, int vnum);
int    obj_affect_modifier(OBJ_DATA *obj, int location);
bool   in_same_room       (CHAR_DATA *ch, CHAR_DATA *victim);
bool   leads_grouped_mob  (CHAR_DATA *ch, int mob_vnum);
char * deity_formatted_name (CHAR_DATA *ch);
char *health_description  (CHAR_DATA *ch, CHAR_DATA *victim);
char *mana_description    (CHAR_DATA *ch, CHAR_DATA *victim);
char *pc_clan_flags_lookup(int flags);
int   percent_health      (CHAR_DATA *ch);

/* recycle.c */
TIMER *new_timer          (void);
void   free_timer         (TIMER *timer);

/* interp.c */
void    interpret          (CHAR_DATA *ch, char *argument);
bool    is_number          (char *arg);
int     number_argument    (char *argument, char *arg);
int     mult_argument      (char *argument, char *arg);
char *  one_argument       (char *argument, char *arg_first);

/* magic.c */
int       find_spell     (CHAR_DATA *ch, const char *name);
bool      saves_spell    (int level, CHAR_DATA *victim, int dam_type);
void      obj_cast_spell (int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj);
bool      check_dispel   (int dis_level, CHAR_DATA * victim, int sn);
CHAR_DATA *get_target    (CHAR_DATA *ch, char *argument, bool ranged);
bool      saves_dispel   (int dis_level, int spell_level, int duration);
int       casting_level  (CHAR_DATA *ch);
int       get_saves      (CHAR_DATA *ch);

/* merit.c */
void add_merit(CHAR_DATA *ch, long merit);
void remove_merit(CHAR_DATA *ch, long merit);
void apply_merit_affects(CHAR_DATA *ch);
char *merit_list(CHAR_DATA *ch);

/* mob_prog.c */
void    program_flow       (int vnum, char *source, CHAR_DATA *mob, CHAR_DATA *ch, const void *arg1, const void *arg2);
void    mp_act_trigger     (char *argument, CHAR_DATA *mob, CHAR_DATA *ch, const void *arg1, const void *arg2, int type);
bool    mp_percent_trigger (CHAR_DATA *mob, CHAR_DATA *ch, const void *arg1, const void *arg2, int type);
void    mp_bribe_trigger   (CHAR_DATA *mob, CHAR_DATA *ch, int amount);
bool    mp_exit_trigger    (CHAR_DATA *ch, int dir);
void    mp_give_trigger    (CHAR_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj);
void    mp_greet_trigger   (CHAR_DATA *ch);
void    mp_hprct_trigger   (CHAR_DATA *mob, CHAR_DATA *ch);

/* mob_cmds.c */
void    mob_interpret      (CHAR_DATA *ch, char *argument);

/* quest.c */
CHAR_DATA *get_quest_giver (int vnum);
CHAR_DATA *find_quest_master(CHAR_DATA * ch);
void       quest_update    (void);

/* settings.c */
void    save_settings      (void);

/* save.c */
void    save_char_obj      (CHAR_DATA *ch);
bool    load_char_obj      (DESCRIPTOR_DATA *d, char *name);

/* skills.c */
int     skill_lookup     (const char *name);
bool    parse_gen_groups (CHAR_DATA *ch, char *argument);
void    list_group_costs (CHAR_DATA *ch);
int     exp_per_level    (CHAR_DATA *ch, int points);
void    check_improve    (CHAR_DATA *ch, int sn, bool success, int multiplier);
bool    check_skill_improve (CHAR_DATA * ch, int sn, int success_multiplier, int failure_multiplier);
void    check_time_improve(CHAR_DATA * ch);
int     group_lookup     (const char *name);
void    gn_add           (CHAR_DATA *ch, int gn);
void    gn_remove        (CHAR_DATA *ch, int gn);
void    group_add        (CHAR_DATA *ch, const char *name, bool deduct);
void    group_remove     (CHAR_DATA *ch, const char *name);
void    show_skill_list  (CHAR_DATA * ch, CHAR_DATA * ch_show, char *argument);
void    show_spell_list  (CHAR_DATA * ch, CHAR_DATA * ch_show, char *argument);

/* nature.c */
void    seed_grow_check  (OBJ_DATA *obj);

/* special.c */
SF *    spec_lookup      (const char *name);
char *  spec_name        (SPEC_FUN *function);

/* timer.c */
void    start_timer();
double  end_timer();
void    subtract_times     (struct timeval *etime, struct timeval *stime);
void    add_timer          (CHAR_DATA *ch, int type, int count, DO_FUN *fun, int value, char *argument);
TIMER  *get_timerptr       (CHAR_DATA *ch, int type);
int  get_timer          (CHAR_DATA *ch, int type);
void    extract_timer      (CHAR_DATA *ch, TIMER *timer);
void    remove_timer       (CHAR_DATA *ch, int type);

/* update.c */
void    gain_condition   (CHAR_DATA *ch, int iCond, int value);
void    update_handler   (bool forced);
void    timer_update     (CHAR_DATA *ch);

/* string.c */
void    string_edit    (CHAR_DATA *ch, char **pString);
void    string_append  (CHAR_DATA *ch, char **pString);
char *  string_replace (char * orig, char * old, char * new);
void    string_add     (CHAR_DATA *ch, char *argument);
char *  format_string  (char *oldstring /*, bool fSpace */);
char *  first_arg      (char *argument, char *arg_first, bool fCase);
char *  string_unpad   (char * argument);
char *  string_proper  (char * argument);
char *  num_punct      (int foo);
char *  num_punct_long (long foo);

/* olc.c */
bool      run_olc_editor    (DESCRIPTOR_DATA *d);
char      *olc_ed_name      (CHAR_DATA *ch);
char      *olc_ed_vnum      (CHAR_DATA *ch);
AREA_DATA *get_area_data    (int vnum);

/* lookup.c */
int    race_lookup          (const char *name);
int    item_lookup          (const char *name);
int    liq_lookup           (const char *name);
int    deity_lookup         (const char *name);
int    merit_lookup         (const char *name);

/* log.c */
void   bug                  (const char *str, int param);
void   log_string           (const char *str);
void   log_obj              (OBJ_DATA * obj);
void   log_f                (char * fmt, ...);
void   bugf                 (char *, ...);

/* name_generator.c */
char   *generate_random_name();
void    init_name_parts();

/* experience.c */
int     xp_compute    (CHAR_DATA * gch, CHAR_DATA * victim, int total_levels);
void    group_gain    (CHAR_DATA * ch, CHAR_DATA * victim);
void    gain_exp      (CHAR_DATA *ch, int gain);
void    advance_level (CHAR_DATA *ch, bool hide);

/* misc.c */
bool    file_exists           (const char *fname);
char    *center_string_padded (const char *str, int width);
char    *bool_truefalse       (bool value);
char    *bool_yesno           (bool value);
char    *bool_onoff           (bool value);
bool    player_exists         (const char *player);
char    *player_file_location (const char *player);
char    *file_last_modified   (const char *filename);

/* act_mob.c */
void          process_portal_merchant (CHAR_DATA * ch, char *argument);
CHAR_DATA *   find_mob_by_act(CHAR_DATA * ch, long act_flag);

/* class_priest.c */
void          agony_damage_check(CHAR_DATA *ch);
void          calculate_priest_rank(CHAR_DATA * ch);

/* class_barbarian.c */
void          second_wind(CHAR_DATA * ch);

/* class_wizard.c */
bool blink_check(CHAR_DATA * ch, bool remove_affect);

/* drunk.c */
char *make_drunk(char *string, CHAR_DATA *ch);

/* keeps.c */
void keep_lord_notify(CHAR_DATA *ch);
void apply_keep_affects();
void apply_keep_affects_to_char(CHAR_DATA * ch);

/* database.c, database_export.c */
void open_db(void);
void close_db(void);
void init_db(void);
bool execute_non_query(char *sql, ...);
bool buffer_non_query(char *sql, ...);
int execute_scalar_int(char *sql, ...);
char *execute_scalar_text(char *sql, ...);
void log_toast(CHAR_DATA *ch, CHAR_DATA *victim, char *message, char *verb, bool arena);
void log_player(CHAR_DATA *ch);
void log_note(NOTE_DATA *note);
int archive_active_notes(void);
void export_game_data(CHAR_DATA * ch, char *argument);
int flush_db(void);
void free_db_buffer(DB_BUFFER* buf);
DB_BUFFER* new_db_buffer(void);
int db_buffer_count(void);
bool execute_raw_query(char *sql);

#undef    CD
#undef    MID
#undef    OD
#undef    OID
#undef    RID
#undef    SF
#undef    AD

/*****************************************************************************
 *                                    OLC                                    *
 *****************************************************************************/

/*
 * Object defined in limbo.are
 * Used in save.c to load objects that don't exist.
 */
#define         OBJ_VNUM_DUMMY 30

/*
 * Area flags.
 */
#define         AREA_NONE        0
#define         AREA_CHANGED    (A)    /* Area has been modified. */
#define         AREA_ADDED      (B)    /* Area has been added to. */
#define         AREA_LOADING    (C)    /* Used for counting in db.c */
#define         AREA_NO_RECALL  (D)    /* Entire area is no recall */
#define         AREA_NO_SUMMON  (E)    /* Cannot summon or be summoned from */
#define         AREA_NO_GATE    (F)    /* Area cannot be gated into or from */
#define         AREA_KEEP       (H)    /* Area is a keep/seige */
#define         AREA_NO_WHERE   (I)    /* Disable the use of 'where' in an entire area */

#define         MAX_DIR        10    /* Maximum direction (0-9 are used making 10) */
#define         NO_FLAG       -99    /* Must not be used in flags or stats. */

/*
 * Global Constants
 */
extern    char *   const     dir_name[];
extern    const    int    rev_dir[];          /* int - ROM OLC */
extern    const    struct    spec_type  spec_table[];

/*
 * Global variables
 */
extern    AREA_DATA *  area_first;
extern    AREA_DATA *  area_last;
extern    SHOP_DATA *  shop_last;

extern    int   top_affect;
extern    int   top_area;
extern    int   top_ed;
extern    int   top_exit;
extern    int   top_help;
extern    int   top_mob_index;
extern    int   top_obj_index;
extern    int   top_reset;
extern    int   top_room;
extern    int   top_shop;
extern    int   top_vnum_mob;
extern    int   top_vnum_obj;
extern    int   top_vnum_room;
extern    int   top_group;
extern    int   top_class;
extern    int   top_sn;
extern    char  str_empty[1];

extern    MOB_INDEX_DATA  *    mob_index_hash[MAX_KEY_HASH];
extern    OBJ_INDEX_DATA  *    obj_index_hash[MAX_KEY_HASH];
extern    ROOM_INDEX_DATA *    room_index_hash[MAX_KEY_HASH];

/* Path Find - This structure is used for the pathfinding algorithm. */
typedef struct heap_data
{
    int             iVertice;
    ROOM_INDEX_DATA ** knude;
    int                size;
} HEAP;

/*  Path Find - Add the prototype for the pathfinding function. */
char *pathfind  (ROOM_INDEX_DATA *from, ROOM_INDEX_DATA *to);

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
    #include <time.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "recycle.h"

/* globals from db.c for load_notes */
extern FILE * fpArea;
extern char strArea[MAX_INPUT_LENGTH];

// Level to post a note
#define NOTE_LEVEL 1

/* local procedures */
void load_thread(char *name, NOTE_DATA **list, int type, time_t free_time);
void parse_note(CHAR_DATA *ch, char *argument, int type);
bool hide_note(CHAR_DATA *ch, NOTE_DATA *pnote);
bool str_list(char *str, char *namelist);
void remote_notify(NOTE_DATA *pnote);
bool notify(CHAR_DATA *ch, NOTE_DATA *pnote);
void forward_note(CHAR_DATA *ch, NOTE_DATA *spoolfrom, char *argument, char *list_name);

NOTE_DATA *note_list;
NOTE_DATA *penalty_list;
NOTE_DATA *news_list;
NOTE_DATA *changes_list;
NOTE_DATA *ooc_list;
NOTE_DATA *story_list;
NOTE_DATA *history_list;
NOTE_DATA *immnote_list;

/*
 * Counts the number of non hidden notes in a specified notes spool.
 */
int count_spool(CHAR_DATA *ch, NOTE_DATA *spool)
{
    int count = 0;
    NOTE_DATA *pnote;

    for (pnote = spool; pnote != NULL; pnote = pnote->next)
    {
        if (!hide_note(ch, pnote))
            count++;
    }

    return count;
} // end int count_spool

/*
 * Shows the number of unread notes and what spools they belong to.
 */
void do_unread(CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    int count;
    bool found = FALSE;

    if (IS_NPC(ch))
        return;

    if ((count = count_spool(ch, news_list)) > 0)
    {
        found = TRUE;
        sprintf(buf, "There %s %d new news article%s waiting.\r\n",
            count > 1 ? "are" : "is", count, count > 1 ? "s" : "");
        send_to_char(buf, ch);
    }

    if ((count = count_spool(ch, changes_list)) > 0)
    {
        found = TRUE;
        sprintf(buf, "There %s %d change%s waiting to be read.\r\n",
            count > 1 ? "are" : "is", count, count > 1 ? "s" : "");
        send_to_char(buf, ch);
    }

    if ((count = count_spool(ch, note_list)) > 0)
    {
        found = TRUE;
        sprintf(buf, "You have %d new note%s waiting.\r\n",
            count, count > 1 ? "s" : "");
        send_to_char(buf, ch);
    }

    if ((count = count_spool(ch, ooc_list)) > 0)
    {
        found = TRUE;
        sprintf(buf, "You have %d new ooc note%s waiting.\r\n",
            count, count > 1 ? "s" : "");
        send_to_char(buf, ch);
    }

    if ((count = count_spool(ch, story_list)) > 0)
    {
        found = TRUE;
        sprintf(buf, "%d %s been added.\r\n",
            count, count > 1 ? "story notes have" : "story note has");
        send_to_char(buf, ch);
    }

    if ((count = count_spool(ch, history_list)) > 0)
    {
        found = TRUE;
        sprintf(buf, "%d %s been added.\r\n",
            count, count > 1 ? "history notes have" : "history note has");
        send_to_char(buf, ch);
    }

    if (IS_IMMORTAL(ch) && (count = count_spool(ch, penalty_list)) > 0)
    {
        found = TRUE;
        sprintf(buf, "%d %s been added.\r\n",
            count, count > 1 ? "penalties have" : "penalty has");
        send_to_char(buf, ch);
    }

    if (IS_IMMORTAL(ch) && (count = count_spool(ch, immnote_list)) > 0)
    {
        found = TRUE;
        sprintf(buf, "%d %s been added.\r\n",
            count, count > 1 ? "imm notes have" : "imm note has");
        send_to_char(buf, ch);
    }

    if (!found)
    {
        send_to_char("You have no unread notes.\r\n", ch);
    }

} // end do_unread

void do_ooc_spool(CHAR_DATA *ch, char *argument)
{
    parse_note(ch, argument, NOTE_OOC);
} // end do_ooc_spool

void do_note(CHAR_DATA *ch, char *argument)
{
    parse_note(ch, argument, NOTE_NOTE);
} // end do_note

void do_story_spool(CHAR_DATA *ch, char *argument)
{
    parse_note(ch, argument, NOTE_STORY);
} // end do_story_pool

void do_news(CHAR_DATA *ch, char *argument)
{
    parse_note(ch, argument, NOTE_NEWS);
} // end do_news

void do_changes(CHAR_DATA *ch, char *argument)
{
    parse_note(ch, argument, NOTE_CHANGES);
} // end do_news

void do_history_spool(CHAR_DATA *ch, char *argument)
{
    parse_note(ch, argument, NOTE_HISTORY);
} // end do_history_spool

void do_penalty(CHAR_DATA *ch, char *argument)
{
    parse_note(ch, argument, NOTE_PENALTY);
} // end do_penalty

void do_imm_spool(CHAR_DATA *ch, char *argument)
{
    parse_note(ch, argument, NOTE_IMM);
} // end do_imm_spool


/*
 * Saves all of the note spools to their files.
 */
void save_notes(int type)
{
    FILE *fp;
    char *name;
    NOTE_DATA *pnote;

    switch (type)
    {
        default:
            return;
        case NOTE_NOTE:
            name = NOTE_FILE;
            pnote = note_list;
            break;
        case NOTE_PENALTY:
            name = PENALTY_FILE;
            pnote = penalty_list;
            break;
        case NOTE_NEWS:
            name = NEWS_FILE;
            pnote = news_list;
            break;
        case NOTE_CHANGES:
            name = CHANGES_FILE;
            pnote = changes_list;
            break;
        case NOTE_OOC:
            name = OOC_FILE;
            pnote = ooc_list;
            break;
        case NOTE_STORY:
            name = STORY_FILE;
            pnote = story_list;
            break;
        case NOTE_IMM:
            name = IMMNOTE_FILE;
            pnote = immnote_list;
            break;
        case NOTE_HISTORY:
            name = HISTORY_FILE;
            pnote = history_list;
            break;
    }

    fclose(fpReserve);
    if ((fp = fopen(name, "w")) == NULL)
    {
        perror(name);
    }
    else
    {
        for (; pnote != NULL; pnote = pnote->next)
        {
            fprintf(fp, "Sender  %s~\n", pnote->sender);
            fprintf(fp, "Date    %s~\n", pnote->date);
            fprintf(fp, "Stamp   %ld\n", pnote->date_stamp);
            fprintf(fp, "To      %s~\n", pnote->to_list);
            fprintf(fp, "Subject %s~\n", pnote->subject);
            fprintf(fp, "Text\n%s~\n", pnote->text);
        }
        fclose(fp);
        fpReserve = fopen(NULL_FILE, "r");
        return;
    }
} // end void save_notes

/*
 * Loads each of the note spools.
 */
void load_notes(void)
{
    // 0 for the first number to keep forever, otherwise it's the amount of days
    // you want to keep entries in the spool
    load_thread(NOTE_FILE, &note_list, NOTE_NOTE, 28 * 24 * 60 * 60);
    load_thread(PENALTY_FILE, &penalty_list, NOTE_PENALTY, 365 * 24 * 60 * 60);
    load_thread(NEWS_FILE, &news_list, NOTE_NEWS, 0);
    load_thread(CHANGES_FILE, &changes_list, NOTE_CHANGES, 0);
    load_thread(OOC_FILE, &ooc_list, NOTE_OOC, 7 * 24 * 60 * 60);
    load_thread(STORY_FILE, &story_list, NOTE_STORY, 60 * 24 * 60 * 60);
    load_thread(HISTORY_FILE, &history_list, NOTE_HISTORY, 0);
    load_thread(IMMNOTE_FILE, &immnote_list, NOTE_IMM, 30 * 24 * 60 * 60);

    // If it's unknown at this point then it's a success
    if (global.last_boot_result == UNKNOWN)
        global.last_boot_result = SUCCESS;

} // end void load_notes

/*
 * Loads an individual note spool.
 */
void load_thread(char *name, NOTE_DATA **list, int type, time_t free_time)
{
    FILE *fp;
    NOTE_DATA *pnotelast;

    if ((fp = fopen(name, "r")) == NULL)
        return;

    log_f("STATUS: Loading Note Spool %s", name);

    pnotelast = NULL;
    for (; ; )
    {
        NOTE_DATA *pnote;
        char letter;

        do
        {
            letter = getc(fp);
            if (feof(fp))
            {
                fclose(fp);
                return;
            }
        } while (isspace(letter));
        ungetc(letter, fp);

        pnote = alloc_perm(sizeof(*pnote));

        if (str_cmp(fread_word(fp), "sender"))
            break;

        pnote->sender = fread_string(fp);

        if (str_cmp(fread_word(fp), "date"))
            break;

        pnote->date = fread_string(fp);

        if (str_cmp(fread_word(fp), "stamp"))
            break;

        pnote->date_stamp = fread_number(fp);

        if (str_cmp(fread_word(fp), "to"))
            break;

        pnote->to_list = fread_string(fp);

        if (str_cmp(fread_word(fp), "subject"))
            break;

        pnote->subject = fread_string(fp);

        if (str_cmp(fread_word(fp), "text"))
            break;

        pnote->text = fread_string(fp);

        if (free_time && pnote->date_stamp < current_time - free_time)
        {
            free_note(pnote);
            continue;
        }

        pnote->type = type;

        if (*list == NULL)
            *list = pnote;
        else
            pnotelast->next = pnote;

        pnotelast = pnote;
    }

    global.last_boot_result = FAILURE;
    strcpy(strArea, NOTE_FILE);
    fpArea = fp;
    bug("Load_notes: bad key word.", 0);
    exit(1);
    return;
} // end void load_thread

/*
 * Writes a note out to the note file.  All notes are not saved over and over with each post, this file
 * is appended to and then it's only totally saved when a note is removed.
 */
void append_note(NOTE_DATA *pnote)
{
    FILE *fp;
    char *name;
    NOTE_DATA **list;
    NOTE_DATA *last;

    switch (pnote->type)
    {
        default:
            return;
        case NOTE_NOTE:
            name = NOTE_FILE;
            list = &note_list;
            break;
        case NOTE_PENALTY:
            name = PENALTY_FILE;
            list = &penalty_list;
            break;
        case NOTE_NEWS:
            name = NEWS_FILE;
            list = &news_list;
            break;
        case NOTE_OOC:
            name = OOC_FILE;
            list = &ooc_list;
            break;
        case NOTE_STORY:
            name = STORY_FILE;
            list = &story_list;
            break;
        case NOTE_CHANGES:
            name = CHANGES_FILE;
            list = &changes_list;
            break;
        case NOTE_HISTORY:
            name = HISTORY_FILE;
            list = &history_list;
            break;
        case NOTE_IMM:
            name = IMMNOTE_FILE;
            list = &immnote_list;
            break;
    }

    if (*list == NULL)
    {
        *list = pnote;
    }
    else
    {
        for (last = *list; last->next != NULL; last = last->next);
        last->next = pnote;
    }

    fclose(fpReserve);

    if ((fp = fopen(name, "a")) == NULL)
    {
        perror(name);
    }
    else
    {
        fprintf(fp, "Sender  %s~\n", pnote->sender);
        fprintf(fp, "Date    %s~\n", pnote->date);
        fprintf(fp, "Stamp   %ld\n", pnote->date_stamp);
        fprintf(fp, "To      %s~\n", pnote->to_list);
        fprintf(fp, "Subject %s~\n", pnote->subject);
        fprintf(fp, "Text\n%s~\n", pnote->text);
        fclose(fp);
    }

    fpReserve = fopen(NULL_FILE, "r");

    // Log the note to the database for posterity.  This will log only if db logging
    // is enabled.
    log_note(pnote);

} // end void append_note

/*
 * Notifies a player that they have received a note is a given spool.
 */
void remote_notify(NOTE_DATA *pnote)
{
    char buf[MAX_STRING_LENGTH];
    char *list_name;
    DESCRIPTOR_DATA *d;

    switch (pnote->type)
    {
        default:
            return;
        case NOTE_NOTE:
            list_name = "notes";
            break;
        case NOTE_STORY:
            list_name = "storys";
            break;
        case NOTE_OOC:
            list_name = "oocs";
            break;
        case NOTE_PENALTY:
            list_name = "penalties";
            break;
        case NOTE_NEWS:
            list_name = "news";
            break;
        case NOTE_CHANGES:
            list_name = "changes";
            break;
        case NOTE_HISTORY:
            list_name = "history";
            break;
        case NOTE_IMM:
            list_name = "immnote";
            break;

    }

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        //CHAR_DATA *victim;
        //victim = d->original ? d->original : d->character;

        if (d->connected == CON_PLAYING && notify(d->character, pnote))
        {
            sprintf(buf, "%s left you a note in the %s spool!{!\r\n", pnote->sender, list_name);
            send_to_char(buf, d->character);
        }
    }

} // end void remote_notify

/*
 * Whether a note is to a specified player or group.
 */
bool is_note_to(CHAR_DATA *ch, NOTE_DATA *pnote)
{
    // Implementor can see all notes, this isn't really an issue because the imp
    // has access to the raw files anyway.
    if (get_trust(ch) >= IMPLEMENTOR)
        return TRUE;

    // The note was sent to the sender	
#if !defined( _WIN32 )
    if (!strcasecmp(ch->name, pnote->sender))
        return TRUE;
#else
    if (!_stricmp(ch->name, pnote->sender))
        return TRUE;
#endif

    // The note was sent to everyone
    if (is_exact_name("all", pnote->to_list))
        return TRUE;

    // The note was sent to all clanners
    if ((is_exact_name("clan", pnote->to_list)
        || is_exact_name("clans", pnote->to_list))
        && ch->clan)
        return TRUE;

    //  The note was sent to the immortal staff
    if (IS_IMMORTAL(ch)
        && (is_exact_name("immortal", pnote->to_list)
            || is_exact_name("immortals", pnote->to_list)
            || is_exact_name("imms", pnote->to_list)
            || is_exact_name("imm", pnote->to_list)
            || is_exact_name("gods", pnote->to_list)
            || is_exact_name("god", pnote->to_list)))
        return TRUE;


    // The note was sent specifically to the coders
    if ((get_trust(ch) == CODER
        || ch->level == CODER
        || (ch->pcdata && ch->pcdata->security >= 10))
        && (is_exact_name("coder", pnote->to_list) || is_exact_name("coders", pnote->to_list)))
        return TRUE;

    // the note was specifically sent to the admin team
    if (get_trust(ch) >= ADMIN
        && (is_exact_name("admin", pnote->to_list) || is_exact_name("admins", pnote->to_list)))
        return TRUE;


    // The player was on the note in question
    if (is_exact_name(ch->name, pnote->to_list))
        return TRUE;

    return FALSE;
} // end bool is_note_to

/*
 * Whether not not a player should be notified of a note.
 */
bool notify(CHAR_DATA *ch, NOTE_DATA *pnote)
{
    // This note was specifically to this person
    if (is_exact_name(ch->name, pnote->to_list))
    {
        return TRUE;
    }

    // The note was sent to all clans
    if ((is_exact_name("clans", pnote->to_list)
        || is_exact_name("clan", pnote->to_list))
        && ch->clan)
    {
        return TRUE;
    }

    //  The note was sent to the immortal staff
    if (IS_IMMORTAL(ch)
        && (is_exact_name("immortal", pnote->to_list)
            || is_exact_name("immortals", pnote->to_list)
            || is_exact_name("imms", pnote->to_list)
            || is_exact_name("imm", pnote->to_list)
            || is_exact_name("gods", pnote->to_list)
            || is_exact_name("god", pnote->to_list)))
    {
        return TRUE;
    }

    // The note was sent specifically to the coders
    if ((get_trust(ch) == CODER
        || ch->level == CODER
        || (ch->pcdata && ch->pcdata->security >= 10))
        && (is_exact_name("coder", pnote->to_list) || is_exact_name("coders", pnote->to_list)))
    {
        return TRUE;
    }

    // the note was specifically sent to the admin team
    if (get_trust(ch) >= ADMIN
        && (is_exact_name("admin", pnote->to_list) || is_exact_name("admins", pnote->to_list)))
    {
        return TRUE;
    }

    // The player was on the note in question
    if (is_exact_name(ch->name, pnote->to_list))
    {
        return TRUE;
    }

    return FALSE;
} // end bool notify

/*
 * Creates a new note and attachs it to the player if they don't already
 * have a note stated.
 */
void note_attach(CHAR_DATA *ch, int type)
{
    NOTE_DATA *pnote;

    if (ch->pnote != NULL)
        return;

    pnote = new_note();

    pnote->next = NULL;
    pnote->sender = str_dup(ch->name);
    pnote->date = str_dup("");
    pnote->to_list = str_dup("");
    pnote->subject = str_dup("");
    pnote->text = str_dup("");
    pnote->type = type;
    ch->pnote = pnote;

    return;
} // end void note_attach

/*
 * Remove a note from the specified spool.
 */
void note_remove(CHAR_DATA *ch, NOTE_DATA *pnote, bool delete)
{
    char to_new[MAX_INPUT_LENGTH];
    char to_one[MAX_INPUT_LENGTH];
    NOTE_DATA *prev;
    NOTE_DATA **list;
    char *to_list;

    if (!delete)
    {
        /* make a new list */
        to_new[0] = '\0';
        to_list = pnote->to_list;
        while (*to_list != '\0')
        {
            to_list = one_argument(to_list, to_one);
            if (to_one[0] != '\0' && str_cmp(ch->name, to_one))
            {
                strcat(to_new, " ");
                strcat(to_new, to_one);
            }
        }
        /* Just a simple recipient removal? */
        if (str_cmp(ch->name, pnote->sender) && to_new[0] != '\0')
        {
            free_string(pnote->to_list);
            pnote->to_list = str_dup(to_new + 1);
            return;
        }
    }

    /* nuke the whole note */
    switch (pnote->type)
    {
        default:
            return;
        case NOTE_NOTE:
            list = &note_list;
            break;
        case NOTE_OOC:
            list = &ooc_list;
            break;
        case NOTE_STORY:
            list = &story_list;
            break;
        case NOTE_PENALTY:
            list = &penalty_list;
            break;
        case NOTE_NEWS:
            list = &news_list;
            break;
        case NOTE_CHANGES:
            list = &changes_list;
            break;
        case NOTE_HISTORY:
            list = &history_list;
            break;
        case NOTE_IMM:
            list = &immnote_list;
            break;
    }

    /*
     * Remove note from linked list.
     */
    if (pnote == *list)
    {
        *list = pnote->next;
    }
    else
    {
        for (prev = *list; prev != NULL; prev = prev->next)
        {
            if (prev->next == pnote)
                break;
        }

        if (prev == NULL)
        {
            bug("Note_remove: pnote not found.", 0);
            return;
        }

        prev->next = pnote->next;
    }

    save_notes(pnote->type);
    free_note(pnote);
    return;
} // end void note_remove

/*
 * Whether or not a specified note is hidden from the player.
 */
bool hide_note(CHAR_DATA *ch, NOTE_DATA *pnote)
{
    time_t last_read;

    if (IS_NPC(ch))
        return TRUE;

    switch (pnote->type)
    {
        default:
            return TRUE;
        case NOTE_NOTE:
            last_read = ch->pcdata->last_note;
            break;
        case NOTE_STORY:
            last_read = ch->pcdata->last_story;
            break;
        case NOTE_OOC:
            last_read = ch->pcdata->last_ooc;
            break;
        case NOTE_PENALTY:
            last_read = ch->pcdata->last_penalty;
            break;
        case NOTE_NEWS:
            last_read = ch->pcdata->last_news;
            break;
        case NOTE_CHANGES:
            last_read = ch->pcdata->last_change;
            break;
        case NOTE_HISTORY:
            last_read = ch->pcdata->last_history;
            break;
        case NOTE_IMM:
            last_read = ch->pcdata->last_immnote;
            break;
    }

    if (pnote->date_stamp <= last_read)
        return TRUE;

    if (!str_cmp(ch->name, pnote->sender))
        return TRUE;

    if (!is_note_to(ch, pnote))
        return TRUE;

    return FALSE;
} // end bool hide_note

/*
 * Update the last timestamp marker a note was read on.
 */
void update_read(CHAR_DATA *ch, NOTE_DATA *pnote)
{
    time_t stamp;

    if (IS_NPC(ch))
        return;

    stamp = pnote->date_stamp;

    switch (pnote->type)
    {
        default:
            return;
        case NOTE_NOTE:
            ch->pcdata->last_note = UMAX(ch->pcdata->last_note, stamp);
            break;
        case NOTE_STORY:
            ch->pcdata->last_story = UMAX(ch->pcdata->last_story, stamp);
            break;
        case NOTE_OOC:
            ch->pcdata->last_ooc = UMAX(ch->pcdata->last_ooc, stamp);
            break;
        case NOTE_PENALTY:
            ch->pcdata->last_penalty = UMAX(ch->pcdata->last_penalty, stamp);
            break;
        case NOTE_NEWS:
            ch->pcdata->last_news = UMAX(ch->pcdata->last_news, stamp);
            break;
        case NOTE_CHANGES:
            ch->pcdata->last_change = UMAX(ch->pcdata->last_change, stamp);
            break;
        case NOTE_HISTORY:
            ch->pcdata->last_history = UMAX(ch->pcdata->last_history, stamp);
            break;
        case NOTE_IMM:
            ch->pcdata->last_immnote = UMAX(ch->pcdata->last_immnote, stamp);
            break;
    }
} // end void update_read

/*
 * Parses the various note commands (search, read, forward, send, etc.).  This is where a bulk
 * of the note logic resides.
 */
void parse_note(CHAR_DATA *ch, char *argument, int type)
{
    BUFFER *buffer;
    BUFFER *output;
    char buf[MAX_STRING_LENGTH * 15];
    char arg[MAX_INPUT_LENGTH];
    NOTE_DATA *pnote;
    NOTE_DATA **list;
    char *list_name;
    int vnum;
    int anum;
    int count;
    int search;

    // Note notes for NPC's through this mechansim
    if (IS_NPC(ch))
        return;

    switch (type)
    {
        default:
            return;
        case NOTE_NOTE:
            list = &note_list;
            list_name = "notes";
            break;
        case NOTE_STORY:
            list = &story_list;
            list_name = "storys";
            break;
        case NOTE_OOC:
            list = &ooc_list;
            list_name = "oocs";
            break;
        case NOTE_PENALTY:
            list = &penalty_list;
            list_name = "penalties";
            break;
        case NOTE_NEWS:
            list = &news_list;
            list_name = "news";
            break;
        case NOTE_CHANGES:
            list = &changes_list;
            list_name = "changes";
            break;
        case NOTE_HISTORY:
            list = &history_list;
            list_name = "history";
            break;
        case NOTE_IMM:
            list = &immnote_list;
            list_name = "imm";
            break;
    }

    argument = one_argument(argument, arg);
    smash_tilde(argument);

    if (arg[0] == '\0' || !str_prefix(arg, "read"))
    {
        bool fAll;

        if (!str_cmp(argument, "all"))
        {
            fAll = TRUE;
            anum = 0;
        }

        else if (argument[0] == '\0' || !str_prefix(argument, "next"))
        /* read next unread note */
        {
            vnum = 0;
            for (pnote = *list; pnote != NULL; pnote = pnote->next)
            {
                if (!hide_note(ch, pnote))
                {
                    sprintf(buf, "[%3d] %s: %s\r\n%s\r\nTo: %s\r\n",
                        vnum,
                        pnote->sender,
                        pnote->subject,
                        pnote->date,
                        pnote->to_list);
                    output = new_buf();
                    add_buf(output, HEADER);
                    add_buf(output, buf);
                    add_buf(output, HEADER);
                    add_buf(output, pnote->text);
                    page_to_char(buf_string(output), ch);
                    free_buf(output);
                    update_read(ch, pnote);
                    return;
                }
                else if (is_note_to(ch, pnote))
                {
                    vnum++;
                }
            }
            sprintf(buf, "You have no unread %s.\r\n", list_name);
            send_to_char(buf, ch);
            return;
        }
        else if (is_number(argument))
        {
            fAll = FALSE;
            anum = atoi(argument);
        }
        else
        {
            send_to_char("Read which number?\r\n", ch);
            return;
        }

        vnum = 0;
        for (pnote = *list; pnote != NULL; pnote = pnote->next)
        {
            if (is_note_to(ch, pnote) && (vnum++ == anum || fAll))
            {
                sprintf(buf, "[%3d] %s: %s\r\n%s\r\nTo: %s\r\n",
                    vnum - 1,
                    pnote->sender,
                    pnote->subject,
                    pnote->date,
                    pnote->to_list);
                output = new_buf();
                add_buf(output, HEADER);
                add_buf(output, buf);
                add_buf(output, HEADER);
                add_buf(output, pnote->text);
                page_to_char(buf_string(output), ch);
                free_buf(output);
                update_read(ch, pnote);
                return;
            }
        }

        sprintf(buf, "There aren't that many %s.\r\n", list_name);
        send_to_char(buf, ch);
        return;
    }

    if (!str_prefix(arg, "forward"))
    {
        forward_note(ch, *list, argument, list_name);
        return;
    }

    if (!str_prefix(arg, "personal"))
    {
        /* read next unread note */
        vnum = 0;
        for (pnote = *list; pnote != NULL; pnote = pnote->next)
        {
            if (!hide_note(ch, pnote) && !str_cmp(ch->name, pnote->to_list))
            {
                sprintf(buf, "[%3d] %s: %s\r\n%s\r\nTo: %s\r\n",
                    vnum,
                    pnote->sender,
                    pnote->subject,
                    pnote->date,
                    pnote->to_list);
                output = new_buf();
                add_buf(output, HEADER);
                add_buf(output, buf);
                add_buf(output, HEADER);
                add_buf(output, pnote->text);
                page_to_char(buf_string(output), ch);
                free_buf(output);
                update_read(ch, pnote);
                return;
            }
            else if (is_note_to(ch, pnote))
            {
                vnum++;
            }
        }
        sprintf(buf, "You have no unread %s.\r\n", list_name);
        send_to_char(buf, ch);
        return;
    }

    if (!str_prefix(arg, "search"))
    {
        output = new_buf();
        count = 0;
        vnum = 0;
        argument = one_argument(argument, arg);
        if (!str_prefix(arg, "old") || !str_prefix(arg, "read"))
        {
            argument = one_argument(argument, arg);
            search = 1;
        }
        else if (!str_prefix(arg, "new") || !str_prefix(arg, "unread"))
        {
            argument = one_argument(argument, arg);
            search = 2;
        }
        else if (!str_prefix(arg, "all"))
        {
            argument = one_argument(argument, arg);
            search = 3;
        }
        else
        {
            search = 2;
        }

        if (!str_prefix(arg, "to"))
        {
            for (pnote = *list; pnote != NULL; pnote = pnote->next)
            {
                switch (search)
                {
                    case 1:
                        if (str_list(pnote->to_list, argument) && hide_note(ch, pnote) && is_note_to(ch, pnote))
                        {
                            sprintf(buf, "[{G%3d{X%s{x] {W%s{x: {C%s{x\r\n",
                                vnum, hide_note(ch, pnote) ? " " : "N",
                                pnote->sender, pnote->subject);
                            count++;
                            add_buf(output, buf);
                        }
                        break;
                    case 2:
                        if (str_list(pnote->to_list, argument) && !hide_note(ch, pnote) && is_note_to(ch, pnote))
                        {
                            sprintf(buf, "[{G%3d{X%s{x] {W%s{x: {C%s{x\r\n",
                                vnum, hide_note(ch, pnote) ? " " : "N",
                                pnote->sender, pnote->subject);
                            count++;
                            add_buf(output, buf);
                        }
                        break;
                    case 3:
                        if (str_list(pnote->to_list, argument) && is_note_to(ch, pnote))
                        {
                            sprintf(buf, "[{G%3d{X%s{x] {W%s{x: {C%s{x\r\n",
                                vnum, hide_note(ch, pnote) ? " " : "N",
                                pnote->sender, pnote->subject);
                            count++;
                            add_buf(output, buf);
                        }
                        break;
                }
                if (is_note_to(ch, pnote))
                    vnum++;
            }
        }
        else if (!str_prefix(arg, "subject"))
        {
            for (pnote = *list; pnote != NULL; pnote = pnote->next)
            {
                switch (search)
                {
                    case 1:
                        if (str_list(pnote->subject, argument) && hide_note(ch, pnote) && is_note_to(ch, pnote))
                        {
                            sprintf(buf, "[{G%3d{X%s{x] {W%s{x: {C%s{x\r\n",
                                vnum, hide_note(ch, pnote) ? " " : "N",
                                pnote->sender, pnote->subject);
                            count++;
                            add_buf(output, buf);
                        }
                        break;
                    case 2:
                        if (str_list(pnote->subject, argument) && !hide_note(ch, pnote) && is_note_to(ch, pnote))
                        {
                            sprintf(buf, "[{G%3d{X%s{x] {W%s{x: {C%s{x\r\n",
                                vnum, hide_note(ch, pnote) ? " " : "N",
                                pnote->sender, pnote->subject);
                            count++;
                            add_buf(output, buf);
                        }
                        break;
                    case 3:
                        if (str_list(pnote->subject, argument) && is_note_to(ch, pnote))
                        {
                            sprintf(buf, "[{G%3d{X%s{x] {W%s{x: {C%s{x\r\n",
                                vnum, hide_note(ch, pnote) ? " " : "N",
                                pnote->sender, pnote->subject);
                            count++;
                            add_buf(output, buf);
                        }
                        break;
                }
                if (is_note_to(ch, pnote))
                {
                    vnum++;
                }
            }
        }
        else if (!str_prefix(arg, "from"))
        {
            for (pnote = *list; pnote != NULL; pnote = pnote->next)
            {
                switch (search)
                {
                    case 1:
                        if (!str_cmp(pnote->sender, argument) && hide_note(ch, pnote) && is_note_to(ch, pnote))
                        {
                            sprintf(buf, "[{G%3d{X%s{x] {W%s{x: {C%s{x\r\n",
                                vnum, hide_note(ch, pnote) ? " " : "N",
                                pnote->sender, pnote->subject);
                            count++;
                            add_buf(output, buf);
                        }
                        break;
                    case 2:
                        if (!str_cmp(pnote->sender, argument) && !hide_note(ch, pnote) && is_note_to(ch, pnote))
                        {
                            sprintf(buf, "[{G%3d{X%s{x] {W%s{x: {C%s{x\r\n",
                                vnum, hide_note(ch, pnote) ? " " : "N",
                                pnote->sender, pnote->subject);
                            count++;
                            add_buf(output, buf);
                        }
                        break;
                    case 3:
                        if (!str_cmp(pnote->sender, argument) && is_note_to(ch, pnote))
                        {
                            sprintf(buf, "[{G%3d{X%s{x] {W%s{x: {C%s{x\r\n",
                                vnum, hide_note(ch, pnote) ? " " : "N",
                                pnote->sender, pnote->subject);
                            count++;
                            add_buf(output, buf);
                        }
                        break;
                }
                if (is_note_to(ch, pnote))
                {
                    vnum++;
                }
            }
        }

        sprintf(buf, "Your search returned a total of %d matches in the %s spool using:%s\r\n\r\n", count, list_name, argument);
        send_to_char(buf, ch);
        page_to_char(buf_string(output), ch);

        if (count == 0)
        {
            switch (type)
            {
                case NOTE_NOTE:
                    send_to_char("There are no notes for you.\r\n", ch);
                    break;
                case NOTE_STORY:
                    send_to_char("There are no story notes for you.\r\n", ch);
                    break;
                case NOTE_OOC:
                    send_to_char("There are no ooc notes for you.\r\n", ch);
                    break;
                case NOTE_PENALTY:
                    send_to_char("There are no penalties for you.\r\n", ch);
                    break;
                case NOTE_NEWS:
                    send_to_char("There is no news for you.\r\n", ch);
                    break;
                case NOTE_CHANGES:
                    send_to_char("There are no changes for you.\r\n", ch);
                    break;
                case NOTE_HISTORY:
                    send_to_char("There are no history notes for you.\r\n", ch);
                    break;
                case NOTE_IMM:
                    send_to_char("There are no imm notes for you.\r\n", ch);
                    break;
            }
        }

        // cleanup
        free_buf(output);
        return;
    }

    if (!str_prefix(arg, "list"))
    {
        output = new_buf();
        vnum = 0;
        for (pnote = *list; pnote != NULL; pnote = pnote->next)
        {
            if (is_note_to(ch, pnote))
            {
                sprintf(buf, "[{G%3d{X%s{x] {W%s{x: {C%s{x\r\n",
                    vnum, hide_note(ch, pnote) ? " " : "N",
                    pnote->sender, pnote->subject);
                add_buf(output, buf);
                vnum++;
            }
        }

        page_to_char(buf_string(output), ch);

        // cleanup
        free_buf(output);

        if (!vnum)
        {
            switch (type)
            {
                case NOTE_NOTE:
                    send_to_char("There are no notes for you.\r\n", ch);
                    break;
                case NOTE_STORY:
                    send_to_char("There are no story notes for you.\r\n", ch);
                    break;
                case NOTE_OOC:
                    send_to_char("There are no ooc notes for you.\r\n", ch);
                    break;
                case NOTE_PENALTY:
                    send_to_char("There are no penalties for you.\r\n", ch);
                    break;
                case NOTE_NEWS:
                    send_to_char("There is no news for you.\r\n", ch);
                    break;
                case NOTE_CHANGES:
                    send_to_char("There are no changes for you.\r\n", ch);
                    break;
                case NOTE_HISTORY:
                    send_to_char("There are no history notes for you.\r\n", ch);
                    break;
                case NOTE_IMM:
                    send_to_char("There are no imm notes for you.\r\n", ch);
                    break;
            }
        }
        return;
    }

    if (!str_prefix(arg, "ulist") || !str_prefix(arg, "unread"))
    {
        output = new_buf();
        vnum = 0;
        count = 0;

        for (pnote = *list; pnote != NULL; pnote = pnote->next)
        {
            if (is_note_to(ch, pnote) && !hide_note(ch, pnote))
            {
                sprintf(buf, "[{G%3d{X%s{x] {W%s{x: {C%s{x\r\n",
                    vnum, hide_note(ch, pnote) ? " " : "N",
                    pnote->sender, pnote->subject);
                add_buf(output, buf);
                count++;
            }
            if (is_note_to(ch, pnote))
            {
                vnum++;
            }
        }
        page_to_char(buf_string(output), ch);

        // cleanup
        free_buf(output);

        if (!count)
        {
            send_to_char("There are no notes of that spool for you.\r\n", ch);
        }

        return;
    }

    if (!str_prefix(arg, "remove"))
    {
        if (!is_number(argument))
        {
            send_to_char("Note remove which number?\r\n", ch);
            return;
        }

        anum = atoi(argument);
        vnum = 0;
        for (pnote = *list; pnote != NULL; pnote = pnote->next)
        {
            if (is_note_to(ch, pnote) && vnum++ == anum)
            {
                note_remove(ch, pnote, FALSE);
                send_to_char("Ok.\r\n", ch);
                return;
            }
        }

        sprintf(buf, "There aren't that many %s.\r\n", list_name);
        send_to_char(buf, ch);
        return;
    }

    if (!str_prefix(arg, "delete") && get_trust(ch) >= MAX_LEVEL - 10)
    {
        if (!is_number(argument))
        {
            send_to_char("Note delete which number?\r\n", ch);
            return;
        }

        anum = atoi(argument);
        vnum = 0;
        for (pnote = *list; pnote != NULL; pnote = pnote->next)
        {
            if (is_note_to(ch, pnote) && vnum++ == anum)
            {
                note_remove(ch, pnote, TRUE);
                send_to_char("Ok.\r\n", ch);
                return;
            }
        }

        sprintf(buf, "There aren't that many %s.\r\n", list_name);
        send_to_char(buf, ch);
        return;
    }

    if (!str_prefix(arg, "catchup"))
    {
        switch (type)
        {
            case NOTE_NOTE:
                ch->pcdata->last_note = current_time;
                break;
            case NOTE_OOC:
                ch->pcdata->last_ooc = current_time;
                break;
            case NOTE_STORY:
                ch->pcdata->last_story = current_time;
                break;
            case NOTE_PENALTY:
                ch->pcdata->last_penalty = current_time;
                break;
            case NOTE_NEWS:
                ch->pcdata->last_news = current_time;
                break;
            case NOTE_CHANGES:
                ch->pcdata->last_change = current_time;
                break;
            case NOTE_HISTORY:
                ch->pcdata->last_history = current_time;
                break;
            case NOTE_IMM:
                ch->pcdata->last_immnote = current_time;
                break;
        }
        return;
    }

    /* below this point only certain people can edit notes */
    if ((type == NOTE_NEWS && !IS_IMMORTAL(ch))
        || (type == NOTE_HISTORY && !IS_IMMORTAL(ch))
        || (type == NOTE_CHANGES && !IS_IMMORTAL(ch))
        || (type == NOTE_IMM && !IS_IMMORTAL(ch)))
    {
        sprintf(buf, "You must be an immortal to write %s.\r\n", list_name);
        send_to_char(buf, ch);
        return;
    }

    if (!str_cmp(arg, "+"))
    {
        note_attach(ch, type);
        if (ch->pnote->type != type)
        {
            send_to_char("You already have a different note in progress.\r\n", ch);
            return;
        }

        if (strlen(ch->pnote->text) + strlen(argument) >= 4096)
        {
            send_to_char("Note too long.\r\n", ch);
            return;
        }

        buffer = new_buf();
        add_buf(buffer, ch->pnote->text);
        add_buf(buffer, argument);
        add_buf(buffer, "\r\n");
        free_string(ch->pnote->text);
        ch->pnote->text = str_dup(buf_string(buffer));
        free_buf(buffer);
        send_to_char("Ok.\r\n", ch);
        return;
    }

    if (!str_cmp(arg, "++"))
    {
        note_attach(ch, type);
        if (ch->pnote->type != type)
        {
            send_to_char("You already have a different note in progress.\r\n", ch);
            return;
        }

        string_append(ch, &ch->pnote->text);
        return;
    }

    if (!str_cmp(arg, "-"))
    {
        int len;
        bool found = FALSE;

        note_attach(ch, type);
        if (ch->pnote->type != type)
        {
            send_to_char("You already have a different note in progress.\r\n", ch);
            return;
        }

        if (ch->pnote->text == NULL || ch->pnote->text[0] == '\0')
        {
            send_to_char("No lines left to remove.\r\n", ch);
            return;
        }

        strcpy(buf, ch->pnote->text);

        for (len = strlen(buf); len > 0; len--)
        {
            if (buf[len] == '\r')
            {
                if (!found)  /* back it up */
                {
                    if (len > 0)
                        len--;
                    found = TRUE;
                }
                else /* found the second one */
                {
                    buf[len + 1] = '\0';
                    free_string(ch->pnote->text);
                    ch->pnote->text = str_dup(buf);
                    return;
                }
            }
        }
        buf[0] = '\0';
        free_string(ch->pnote->text);
        ch->pnote->text = str_dup(buf);
        return;
    }

    if (!str_prefix(arg, "subject"))
    {
        note_attach(ch, type);
        if (ch->pnote->type != type)
        {
            send_to_char("You already have a different note in progress.\r\n", ch);
            return;
        }

        free_string(ch->pnote->subject);
        ch->pnote->subject = str_dup(argument);
        send_to_char("Ok.\r\n", ch);
        return;
    }

    if (!str_prefix(arg, "to"))
    {
        note_attach(ch, type);
        if (ch->pnote->type != type)
        {
            send_to_char("You already have a different note in progress.\r\n", ch);
            return;
        }
        free_string(ch->pnote->to_list);
        ch->pnote->to_list = str_dup(argument);
        send_to_char("Ok.\r\n", ch);
        return;
    }

    if (!str_prefix(arg, "transfer"))
    {
        if (!ch->pnote)
        {
            send_to_char("You don't have a note in progress.\r\n", ch);
            return;
        }
        argument = one_argument(argument, arg);

        if (!str_prefix(arg, "note"))
            type = NOTE_NOTE;
        else if (!str_prefix(arg, "storyn"))
            type = NOTE_STORY;
        else if (!str_prefix(arg, "oocn"))
            type = NOTE_OOC;
        else if (!str_prefix(arg, "penalty"))
            type = NOTE_PENALTY;
        else if (!str_prefix(arg, "news"))
            type = NOTE_NEWS;
        else if (!str_prefix(arg, "change"))
            type = NOTE_CHANGES;
        else if (!str_prefix(arg, "imm"))
            type = NOTE_IMM;
        else if (!str_prefix(arg, "history"))
            type = NOTE_HISTORY;
        else
        {
            send_to_char("That spool doesn't exist!\r\n", ch);
            return;
        }

        ch->pnote->type = type;
        send_to_char("Ok.\r\n", ch);
        return;
    }

    if (!str_prefix(arg, "clear"))
    {
        if (ch->pnote != NULL)
        {
            free_note(ch->pnote);
            ch->pnote = NULL;
        }

        send_to_char("Ok.\r\n", ch);
        return;
    }

    if (!str_prefix(arg, "show"))
    {
        if (ch->pnote == NULL)
        {
            send_to_char("You have no note in progress.\r\n", ch);
            return;
        }

        if (ch->pnote->type != type)
        {
            send_to_char("You aren't working on that kind of note.\r\n", ch);
            return;
        }

        sprintf(buf, "%s: %s\r\nTo: %s\r\n", ch->pnote->sender, ch->pnote->subject, ch->pnote->to_list);
        output = new_buf();
        add_buf(output, HEADER);
        add_buf(output, buf);
        add_buf(output, HEADER);
        add_buf(output, ch->pnote->text);
        page_to_char(buf_string(output), ch);
        free_buf(output);
        return;
    }

    if (!str_prefix(arg, "format"))
    {
        if (ch->pnote == NULL)
        {
            send_to_char("You have no note in progress.\r\n", ch);
            return;
        }

        if (ch->pnote->type != type)
        {
            send_to_char("You aren't working on that kind of note.\r\n", ch);
            return;
        }

        ch->pnote->text = format_string(ch->pnote->text);
        send_to_char("Your note has now been formatted.\r\n", ch);
        return;
    }

    if (!str_prefix(arg, "post") || !str_prefix(arg, "send"))
    {
        char *strtime;

        if (ch->pnote == NULL)
        {
            send_to_char("You have no note in progress.\r\n", ch);
            return;
        }

        if (ch->level < NOTE_LEVEL)
        {
            sprintf(buf, "You cannot post public notes until you are level %d.\r\n", NOTE_LEVEL);
            send_to_char(buf, ch);
            return;
        }

        if (ch->pnote->type != type)
        {
            send_to_char("You aren't working on that kind of note.\r\n", ch);
            return;
        }

        if (!str_cmp(ch->pnote->to_list, ""))
        {
            send_to_char("You need to provide a recipient (name, all, or immortal).\r\n", ch);
            return;
        }

        if (!str_cmp(ch->pnote->subject, ""))
        {
            send_to_char("You need to provide a subject.\r\n", ch);
            return;
        }

        ch->pnote->next = NULL;
        strtime = ctime(&current_time);
        strtime[strlen(strtime) - 1] = '\0';
        ch->pnote->date = str_dup(strtime);
        ch->pnote->date_stamp = current_time;

        append_note(ch->pnote);
        send_to_char("Thank you for contributing.\r\n", ch);

        // TODO - notifications
        remote_notify(ch->pnote);

        ch->pnote = NULL;

        return;
    }

    send_to_char("You can't do that.\r\n", ch);
    return;
} // end void parse_note

/*
 * Determines if a fully matched name is found a list of names.
 */
bool str_list(char *str, char *namelist)
{
    char name[MAX_INPUT_LENGTH];
    char *list, *string;

    /* fix crash on NULL namelist */
    if (namelist == NULL || namelist[0] == '\0')
        return FALSE;

    /* fixed to prevent is_name on "" returning TRUE */
    if (str == NULL || str[0] == '\0')
        return FALSE;

    string = str;
    /* we need ALL parts of string to match part of namelist */
    for (; ;)  /* start parsing string */
    {
        /* check to see if this is part of namelist */
        list = namelist;
        for (; ; )  /* start parsing namelist */
        {
            list = one_argument(list, name);

            if (is_name(name, string))
                return TRUE; /* full pattern match */

            if (name[0] == '\0' || list[0] == '\0') /*  this name was not found  */
                return FALSE;

        }
    }
    return FALSE;
} // end bool str_list

/*
 * Forwards a note from one player to another.
 */
void forward_note(CHAR_DATA *ch, NOTE_DATA *spoolfrom, char *argument, char *list_name)
{
    NOTE_DATA *pNoteFrom, *pNoteTo;
    char buf[MAX_STRING_LENGTH * 15];
    char arg[MAX_INPUT_LENGTH];
    char *strtime;
    bool found = FALSE;
    int anum, vnum = 0, type;
    BUFFER *buffer;

    smash_tilde(argument);
    argument = one_argument(argument, arg);

    if (is_number(arg))
    {
        anum = atoi(arg);
    }
    else
    {
        send_to_char("Forward which note?\r\n", ch);
        return;
    }

    if (ch->level < NOTE_LEVEL)
    {
        sprintf(buf, "You cannot forward notes until you are level %d.\r\n", NOTE_LEVEL);
        send_to_char(buf, ch);
        return;
    }

    argument = one_argument(argument, arg);

    if (!str_prefix(arg, "note"))
    {
        type = NOTE_NOTE;
    }
    else if (!str_prefix(arg, "storyn"))
    {
        type = NOTE_STORY;
    }
    else if (!str_prefix(arg, "oocn"))
    {
        type = NOTE_OOC;
    }
    else if (!str_prefix(arg, "penalty") && ch->level >= LEVEL_IMMORTAL)
    {
        type = NOTE_PENALTY;
    }
    else if (!str_prefix(arg, "news") && ch->level >= LEVEL_IMMORTAL)
    {
        type = NOTE_NEWS;
    }
    else if (!str_prefix(arg, "change") && ch->level >= LEVEL_IMMORTAL)
    {
        type = NOTE_CHANGES;
    }
    else if (!str_prefix(arg, "immnote") && ch->level >= LEVEL_IMMORTAL)
    {
        type = NOTE_IMM;
    }
    else if (!str_prefix(arg, "history") && ch->level >= LEVEL_IMMORTAL)
    {
        type = NOTE_HISTORY;
    }
    else
    {
        sprintf(buf, "%s number spool to_list\r\n", list_name);
        send_to_char(buf, ch);
        return;
    }

    for (pNoteFrom = spoolfrom; pNoteFrom != NULL; pNoteFrom = pNoteFrom->next)
    {
        if (is_note_to(ch, pNoteFrom) && (vnum++ == anum))
        {
            found = TRUE;
            break;
        }
    }

    if (found)
    {
        buffer = new_buf();
        pNoteTo = new_note();
        pNoteTo->next = NULL;
        pNoteTo->sender = str_dup(ch->name);
        strtime = ctime(&current_time);
        strtime[strlen(strtime) - 1] = '\0';
        pNoteTo->date = str_dup(strtime);
        pNoteTo->date_stamp = current_time;
        pNoteTo->to_list = str_dup(argument);
        sprintf(buf, "Fw: %s", pNoteFrom->subject);
        pNoteTo->subject = str_dup(buf);
        sprintf(buf, "\r\n| -----Original Message-----\r\n| From: %s\r\n| To:  %s\r\n| Subject: %s\r\n| Date: %s\r\n\r\n", pNoteFrom->sender, pNoteFrom->to_list, pNoteFrom->subject, pNoteFrom->date);
        add_buf(buffer, buf);
        add_buf(buffer, pNoteFrom->text);
        pNoteTo->text = str_dup(buf_string(buffer));
        pNoteTo->type = type;
        free_buf(buffer);
        append_note(pNoteTo);
        send_to_char("Thank you for contributing.\r\n", ch);

        remote_notify(pNoteTo);
        // TODO - add notification
    }
    else
    {
        sprintf(buf, "There aren't that many %s.\r\n", list_name);
        send_to_char(buf, ch);
    }
    return;
} // end void forward_note

/*
 * Command to catchup all note spools, will be handy for players who start new
 * characters and don't want to read through all the news or changes they may
 * have already read.
 */
void do_catchup(CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
        return;

    ch->pcdata->last_note = current_time;
    ch->pcdata->last_ooc = current_time;
    ch->pcdata->last_story = current_time;
    ch->pcdata->last_news = current_time;
    ch->pcdata->last_change = current_time;
    ch->pcdata->last_history = current_time;
    ch->pcdata->last_immnote = current_time;
    ch->pcdata->last_penalty = current_time;

    send_to_char("Your note spools have been caught up.\r\n", ch);

} // end do_catchup

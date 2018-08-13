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

/*
 * Handles writing to a piece of parchement.  We will writing line by line
 * or via the string editor like with notes.  We are going to use a standard
 * object and utilize the extra description to store the parchments content
 * so that we don't have to create a specialized object like we might/will
 * have to with a full book that has multiple pages.  We will keep parchments
 * simple.
 */
void do_write(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    argument = one_argument(argument, arg);

    if (IS_NULLSTR(arg))
    {
        send_to_char("What parchment do you wish to write on?\r\n", ch);
        return;
    }

    if ((obj = get_obj_carry(ch, arg, ch)) == NULL)
    {
        send_to_char("You do not appear to have that item.\r\n", ch);
        return;
    }

    if (obj->item_type != ITEM_PARCHMENT)
    {
        send_to_char("That is not parchment.\r\n", ch);
        return;
    }

    // We will handle the title here, the parchment can be titled/retitled
    // even if it has already been written on (which is why it comes before
    // that check).  We will enforce that the parchment always keeps that it
    // is a piece of parchment in the title so players can't create pieces of
    // parchment that look like other objects.
    if (!str_prefix("title", argument))
    {
        // Pluck title off, keep the rest.
        argument = one_argument(argument, arg);

        if (IS_NULLSTR(argument))
        {
            // They must provide text for the title.
            send_to_char("You must provide a title for the parchment.\r\n", ch);
            return;
        }
        else if (strlen(argument) > 30)
        {
            // The title can't be that long, will enforce a 30 character limit.
            send_to_char("Titles for parchment are limited to 30 characters.\r\n", ch);
            return;
        }

        // Separate the object from the others like it if the character has more than
        // one (otherwise it will write this over all of them).
        separate_obj(obj);

        // Construct the title, free the memory for the short description and copy the new one in.
        sprintf(buf, "a piece of parchment titled \"%s\"", argument);
        free_string(obj->short_descr);
        obj->short_descr = str_dup(buf);

        // In case a player has lots of parchments, we don't want them to have to try to get the
        // right one by "read 13.parchment".  Remove the color codes from here.  Add the title
        // into the keywords (we should consider removing articles and prepositions).
        sprintf(buf, "parchment %s", strip_color(argument));
        free_string(obj->name);
        obj->name = str_dup(buf);

        return;
    }

    // We only allow paper to be written on one time
    if (obj->value[1] == TRUE)
    {
        send_to_char("That parchment has already been written on.\r\n", ch);
        return;
    }

    // They either need to enter the text for the parchment or indicate they want to enter
    // the string editor.
    if (IS_NULLSTR(argument))
    {
        send_to_char("What do you wish to write on the parchment?\r\n", ch);
        return;
    }

    // Separate out this specific piece of paper from the rest if the character
    // holds more than one.
    separate_obj(obj);

    // Remove any tilde's from the input string as they'll bork the object from being read
    // back in when the pfile is loaded.
    smash_tilde(argument);

    // Go ahead and show the room the character is writing.
    send_to_char("You write to a piece of parchment.\r\n",ch);
    act("$n writes to a piece of parchment.", ch, NULL, NULL, TO_ROOM);

    // Write the data onto the parchment, doing it this way allows us to not
    // have to create a more in depth object type.  One extra description is
    // allowed per parchment and it will be flagged then as being written on
    // already to stop re-writing on it.  Although that maybe annoying for people
    // that want to edit their text, it seems more realistic.
    obj->extra_descr = new_extra_descr();
    obj->extra_descr->keyword = str_dup("parchment");
    obj->extra_descr->next = NULL;
    obj->value[1] = TRUE;

    // What the user see's in their inventory.
    free_string(obj->short_descr);
    obj->short_descr = str_dup("a piece of parchment");

    // If it gets to this point is has been used, tailor the description to
    // indicate that characters see this when it's on the ground.
    free_string(obj->description);
    obj->description = str_dup("A piece of used parchment lies here.");

    // Check to see if we're going to edit the string editor or if this is going to simply
    // write the one line of content the user provides
    if (!str_cmp(argument, "++") || !str_cmp(argument, "edit"))
    {
        string_append(ch, &obj->extra_descr->description);
        return;
    }
    else
    {
        // They did not indicate they wanted the string editor so write their argument
        // verbatim into the field.  In this instance, we will auto format it for them.
        obj->extra_descr->description = str_dup(argument);
        obj->extra_descr->description = format_string(obj->extra_descr->description);
    }

    return;
}

/*
 * Reads from a piece of parchment that has been written to.
 */
void do_read(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;

    if (IS_NULLSTR(argument))
    {
        send_to_char("What do you wish to read?\r\n", ch);
        return;
    }

    // This may need to change later if we allow for the reading of signs as
    // objects for instance or posted pieces of parchments that the character
    // doesn't have in their inventory.
    if ((obj = get_obj_carry(ch, argument, ch)) == NULL)
    {
        send_to_char("You do not appear to have that item.\r\n", ch);
        return;
    }

    // Is it parchment?
    if (obj->item_type != ITEM_PARCHMENT)
    {
        send_to_char("That is not a piece of parchment.\r\n",ch);
        return;
    }

    // Checks to make sure the parchment is ready to be read.
    if (obj->value[1] == FALSE || obj->extra_descr == NULL || IS_NULLSTR(obj->extra_descr->description))
    {
        send_to_char("That piece of parchment is blank.\r\n", ch);
        return;
    }

    // Show the character what they have read.
    sendf(ch, "%s\r\n", obj->extra_descr->description);

    // Show the room
    act("$n reads a piece of parchment.", ch, NULL, NULL, TO_ROOM);
}

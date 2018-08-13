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
 *                                                                         *
 *  Grid Creation Code by Davion from mudbytes.net                         *
 *                                                                         *
 ***************************************************************************/

#ifndef GRID_H
#define GRID_H
#ifndef MSL
#define MSL MAX_STRING_LENGTH
#endif

#define DEFAULT_GRID_SIZE = 120 //characters

typedef struct grid_data GRID_DATA;
typedef struct grid_row  GRID_ROW;
typedef struct grid_cell GRID_CELL;

struct grid_data
{
    GRID_ROW *first_row;
    GRID_ROW *last_row;

    int width;

    char border_corner;
    char border_top;
    char border_left;
    char border_right;
    char border_internal;
    char border_bottom;
};

struct grid_row
{
    GRID_DATA *grid;

    GRID_ROW *next;
    GRID_ROW *prev;

    GRID_CELL *first_cell;
    GRID_CELL *last_cell;

    int curr_width;
    int max_height;
    int columns;

    int padding_top;
    int padding_left;
    int padding_right;
    int padding_bottom;
};

struct grid_cell
{
    GRID_ROW *row;
    GRID_CELL *next;
    GRID_CELL *prev;

    char contents[MSL];

    int width;
    int lines;
};

// Prototypes
void grid_to_char(GRID_DATA *grid, CHAR_DATA *ch, bool destroy);
void row_to_char(GRID_ROW *row, CHAR_DATA *ch);
void cell_set_linecount(GRID_CELL *cell);
void cell_append_contents(GRID_CELL *cell, char *fmt, ...);
void cell_set_contents(GRID_CELL *cell, char *fmt, ...);
GRID_CELL * row_append_cell(GRID_ROW *row, int width, char *fmt, ...);
void row_remove_cell(GRID_CELL *cell);
void row_add_cell(GRID_ROW *row, GRID_CELL *cell);
void grid_remove_row(GRID_ROW *row);
void grid_add_row(GRID_DATA *grid, GRID_ROW *row);
GRID_CELL * destroy_cell(GRID_CELL *cell);
GRID_CELL *create_cell(GRID_ROW *row, int width);
GRID_ROW *destroy_row(GRID_ROW *row);
GRID_ROW *create_row(GRID_DATA *grid);
GRID_ROW *create_row_padded(GRID_DATA *grid, int top, int bottom, int left, int right);
GRID_DATA * destroy_grid(GRID_DATA *grid);
GRID_DATA * create_grid(int width);
#endif


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
 *  Grid Creation Code by Davion from mudbytes.net                         *
 *                                                                         *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  NOTE - When creating a grid only use \n otherwise the spacing at the   *
 *         end of the line will be off by one.                             *
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "merc.h"
#include "grid.h"

/*
 * Creates a grid.
 */
GRID_DATA * create_grid(int width)
{
    GRID_DATA *new_grid;

    new_grid = calloc(1, sizeof(*new_grid));
    new_grid->border_corner = '+';
    new_grid->border_left = '|';
    new_grid->border_right = '|';
    new_grid->border_internal = '+';
    new_grid->border_top = '-';
    new_grid->border_bottom = '-';
    new_grid->width = width;
    return new_grid;
}

/*
 * Destroy's and free's the memory of a grid
 */
GRID_DATA * destroy_grid(GRID_DATA *grid)
{
    GRID_ROW *row, *row_next = NULL;
    GRID_CELL *cell, *cell_next;

    if (grid->first_row)
    {
        for (row = grid->first_row; row; row = row_next)
        {
            row_next = row->next;

            if (row->first_cell)
            {
                for (cell = row->first_cell; cell; cell = cell_next)
                {
                    cell_next = cell->next;
                    destroy_cell(cell);
                }
            }

            destroy_row(row);
        }
    }

    free(grid);
    return NULL;
}

/*
 * Creates a new grid row with the default padding (1 top, 1 bottom, 2 left, 2 right).
 */
GRID_ROW *create_row(GRID_DATA *grid)
{
    GRID_ROW *new_row;

    new_row = calloc(1, sizeof(*new_row));
    new_row->grid = grid;

    if (grid)
    {
        grid_add_row(grid, new_row);
    }

    new_row->padding_top = 1;
    new_row->padding_bottom = 1;
    new_row->padding_left = 2;
    new_row->padding_right = 2;
    return new_row;

}

/*
 * Create's a new grid row with the padding specified by the caller.
 */
GRID_ROW *create_row_padded(GRID_DATA *grid, int top, int bottom, int left, int right)
{
    GRID_ROW *new_row;

    new_row = calloc(1, sizeof(*new_row));
    new_row->grid = grid;

    if (grid)
    {
        grid_add_row(grid, new_row);
    }

    new_row->padding_top = top;
    new_row->padding_bottom = bottom;
    new_row->padding_left = left;
    new_row->padding_right = right;

    return new_row;
}


/*
 * Destroy's a row
 */
GRID_ROW * destroy_row(GRID_ROW *row)
{
    if (row->grid)
    {
        grid_remove_row(row);
    }

    free(row);
    return NULL;
}

/*
 * Creates a cell for the grid
 */
GRID_CELL *create_cell(GRID_ROW *row, int width)
{
    GRID_CELL *new_cell;
    new_cell = calloc(1, sizeof(*new_cell));
    new_cell->contents[0] = '\0';
    new_cell->width = width;

    new_cell->row = row;

    if (row)
    {
        row_add_cell(row, new_cell);
    }

    return new_cell;
}

/*
 * Destroy's a cell.
 */
GRID_CELL * destroy_cell(GRID_CELL *cell)
{
    if (cell->row)
    {
        row_remove_cell(cell);
    }

    free(cell);
    return NULL;
}

/*
 * List Management
 */
void grid_add_row(GRID_DATA *grid, GRID_ROW *row)
{
    if (!grid->last_row)
    {
        grid->first_row = row;
        grid->last_row = row;
        row->grid = grid;
        return;
    }

    row->prev = grid->last_row;
    grid->last_row->next = row;
    grid->last_row = row;
    row->grid = grid;
    return;
}

/*
 * Removes a grid row.
 */
void grid_remove_row(GRID_ROW *row)
{
    GRID_DATA *grid = row->grid;

    if (!grid)
    {
        return;
    }

    if (row == grid->first_row)
    {
        grid->first_row = grid->first_row->next;
    }

    if (row == grid->last_row)
    {
        grid->last_row = row->prev;
    }

    if (row->prev)
    {
        row->prev->next = row->next;
    }

    if (row->next)
    {
        row->next->prev = row->prev;
    }

    row->grid = NULL;
    row->next = NULL;
    row->prev = NULL;
}

/*
 * Adds a row cell
 */
void row_add_cell(GRID_ROW *row, GRID_CELL *cell)
{
    if (row->curr_width + cell->width > row->grid->width)
    {
        log_string("Warning: Added Cell Width Overflows Grid");
    }

    if (!row->last_cell)
    {
        row->first_cell = cell;
        row->last_cell = cell;
        row->columns++;
        return;
    }

    cell->prev = row->last_cell;
    row->last_cell->next = cell;
    row->last_cell = cell;
    cell->row = row;
    row->curr_width += cell->width;
    row->columns++;
    return;
}

/*
 * Removes a row cell.
 */
void row_remove_cell(GRID_CELL *cell)
{
    GRID_ROW *row = cell->row;

    if (!row)
    {
        return;
    }

    if (cell == row->first_cell)
    {
        row->first_cell = row->first_cell->next;
    }

    if (cell == row->last_cell)
    {
        row->last_cell = cell->prev;
    }

    if (cell->prev)
    {
        cell->prev->next = cell->next;
    }

    if (cell->next)
    {
        cell->next->prev = cell->prev;
    }

    row->columns--;
    cell->row = NULL;
    cell->next = NULL;
    cell->prev = NULL;
}

/*
 * Meat
 * IF You alaready have a cell use this to set the contents. If it has contents already, they are overwritten.
 */
void cell_set_contents(GRID_CELL *cell, char *fmt, ...)
{
    char buf[MSL];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    sprintf(cell->contents, "%s", buf);
    cell_set_linecount(cell);
}

/*
 * If you already have a cell use this to append to the cells content.
 */
void cell_append_contents(GRID_CELL *cell, char *fmt, ...)
{
    char buf[MSL];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);
    strncat(cell->contents, buf, MSL - 1);
    cell_set_linecount(cell);
}

/*
 * IF you don't already have a cell, but you have a row, use this to create a cell with the contents of fmt.
 */
GRID_CELL * row_append_cell(GRID_ROW *row, int width, char *fmt, ...)
{
    char buf[MSL];
    GRID_CELL *cell;

    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    cell = create_cell(row, width);
    cell_set_contents(cell, buf);
    return cell;
}

/*
 * Low level line counter
 */
void cell_set_linecount(GRID_CELL *cell)
{
    int count = 1;
    char *pos = cell->contents;

    while (*pos++)
    {
        if (*pos == '\n')
        {
            count++;
        }
    }

    cell->lines = count;

    if (cell->row->max_height < count)
    {
        cell->row->max_height = count;
    }
}

/*
 * Counts color codes to display offsets properly
 */
int count_color(const char *str)
{
    char c;
    int count = 0;

    while ((c = *str++) != '\0')
    {
        if (c == '{')
        {
            count++;
        }
    }

    return count * 2;
}

/*
 * Displaying of the Grid
 */
void row_to_char(GRID_ROW *row, CHAR_DATA *ch)
{
    GRID_CELL *cell;
    char *** ptrs;
    char *tok;
    char buf[MSL], pad_buf[MSL];
    int i = 0, n = 0;
    int filler = UMIN(0, row->grid->width - row->curr_width - 1);
    int actual_height = row->max_height + (row->padding_top + row->padding_bottom);
    int alloced = 0;

    ptrs = calloc(row->columns, sizeof(*ptrs));

    //Explode columns into individual lines.
    for (i = 0, cell = row->first_cell; cell; cell = cell->next, ++i)
    {
        int colour_offset = 0, actual_width = cell->width - row->padding_right - row->padding_left - 1;

        sprintf(pad_buf, "%*s%-*.*s%*s", row->padding_left, " ", actual_width, actual_width, " ", row->padding_right, " ");

        ptrs[i] = calloc(actual_height, sizeof(*ptrs[i]));

        tok = strtok(cell->contents, "\n");

        //Append Padding to Top.
        for (n = 0; n < row->padding_top; ++n)
        {
            #if defined(_WIN32)
            ptrs[i][n] = _strdup(pad_buf); alloced++;
            #else
            ptrs[i][n] = strdup(pad_buf); alloced++;
            #endif
        }

        while (tok)
        {
            colour_offset = count_color(tok);

            sprintf(buf, "%*s%-*.*s%*s", row->padding_left, " ", actual_width + colour_offset, actual_width + colour_offset, tok, row->padding_right, " ");

            #if defined(_WIN32)
            ptrs[i][n] = _strdup(buf);
            #else
            ptrs[i][n] = strdup(buf);
            #endif

            tok = strtok(NULL, "\n");
            ++n;
        }

        //Add padding to bottom. This will also fill in any empty rows.
        for (; n < actual_height; ++n)
        {
            #if defined(_WIN32)
            ptrs[i][n] = _strdup(pad_buf);
            #else
            ptrs[i][n] = strdup(pad_buf);
            #endif
        }
    }

    //Go through the exploded row, and send a line from each column at a time.
    for (n = 0; n < actual_height; ++n)
    {
        for (i = 0; i < row->columns; ++i)
        {
            if (i == 0)
            {
                sendf(ch, "%c", row->grid->border_left);
            }
            else
            {
                sendf(ch, "%c", row->grid->border_internal);
            }

            sendf(ch, "%s", ptrs[i][n]);
            free(ptrs[i][n]);
        }

        sendf(ch, "%-*c\r\n", filler, row->grid->border_right);
    }

    for (i = 0; i < row->columns; ++i)
    {
        free(ptrs[i]);
    }

    free(ptrs);
}

/*
 * Display the whole grid, row by row, and destroy if necessary. Should only
 * not destroy if you intend to display it multiple times in a single function.
 */
void grid_to_char(GRID_DATA *grid, CHAR_DATA *ch, bool destroy)
{
    GRID_ROW *row;
    int i;

    sendf(ch, "%c", grid->border_corner);

    for (i = 0; i < grid->width - 1; ++i)
    {
        sendf(ch, "%c", grid->border_top);
    }

    sendf(ch, "%c\r\n", grid->border_corner);

    for (row = grid->first_row; row; row = row->next)
    {
        row_to_char(row, ch);
        sendf(ch, "%c", grid->border_corner);

        for (i = 0; i < grid->width - 1; ++i)
        {
            sendf(ch, "%c", grid->border_bottom);
        }

        sendf(ch, "%c\r\n", grid->border_corner);
    }

    if (destroy)
    {
        destroy_grid(grid);
    }
}


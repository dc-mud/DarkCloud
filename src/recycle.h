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

/* externs */
extern char str_empty[1];
extern int mobile_count;

/* stuff for providing a crash-proof buffer */

#define MAX_BUF		16384
#define MAX_BUF_LIST 	13
#define BASE_BUF 	1024

/* valid states */
#define BUFFER_SAFE	0
#define BUFFER_OVERFLOW	1
#define BUFFER_FREED 	2

/* ban data recycling */
#define BD BAN_DATA
BD	*new_ban (void);
void	free_ban (BAN_DATA *ban);
#undef BD

/* note recycling */
#define ND NOTE_DATA
ND	*new_note (void);
void	free_note (NOTE_DATA *note);
#undef ND

/* descriptor recycling */
#define DD DESCRIPTOR_DATA
DD	*new_descriptor (void);
void	free_descriptor (DESCRIPTOR_DATA *d);
#undef DD

/* char gen data recycling */
#define GD GEN_DATA
GD 	*new_gen_data (void);
void	free_gen_data (GEN_DATA * gen);
#undef GD

/* extra descr recycling */
#define ED EXTRA_DESCR_DATA
ED	*new_extra_descr (void);
void	free_extra_descr (EXTRA_DESCR_DATA *ed);
#undef ED

/* affect recycling */
#define AD AFFECT_DATA
AD	*new_affect (void);
void	free_affect (AFFECT_DATA *af);
#undef AD

/* object recycling */
#define OD OBJ_DATA
OD	*new_obj (void);
void	free_obj (OBJ_DATA *obj);
#undef OD

/* character recyling */
#define CD CHAR_DATA
#define PD PC_DATA
CD	*new_char (void);
void	free_char (CHAR_DATA *ch);
PD	*new_pcdata (void);
void	free_pcdata (PC_DATA *pcdata);
#undef PD
#undef CD


/* mob id and memory procedures */
#define MD MEM_DATA
long 	get_pc_id (void);
long	get_mob_id (void);
MD	*new_mem_data (void);
void	free_mem_data (MEM_DATA *memory);
#undef MD

/* buffer procedures */
BUFFER	*new_buf (void);
BUFFER  *new_buf_size (int size);
void	free_buf (BUFFER *buffer);
bool	add_buf (BUFFER *buffer, char *string);
void	clear_buf (BUFFER *buffer);
char	*buf_string (BUFFER *buffer);

HELP_AREA *	new_had		(void);
HELP_DATA *	new_help	(void);
void		free_help	(HELP_DATA *);

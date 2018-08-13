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

/*
 * This file contains all of the OS-dependent stuff:
 *   startup, signals, BSD sockets for tcp/ip, i/o, timing.
 *
 * The data flow for input is:
 *    Game_loop ---> Read_from_descriptor ---> Read
 *    Game_loop ---> Read_from_buffer
 *
 * The data flow for output is:
 *    Game_loop ---> Process_Output ---> Write_to_descriptor -> Write
 *
 * The OS-dependent functions are Read_from_descriptor and Write_to_descriptor.
 * -- Furey  26 Jan 1993
 */

// System Specific Includes
#if defined(_WIN32)
#include <sys/types.h>
#include <time.h>
#include <io.h>
//#include <winsock.h>
#include <WinSock2.h>
#include "telnet.h"
#include <sys/timeb.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>  //  OLC -- for close read write etc
#include <time.h>
#endif

// General Includes
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>  // for sendf
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"

/*
 * Malloc debugging stuff.
 */

#if defined(MALLOC_DEBUG)
    #include <malloc.h>
    extern int malloc_debug(int);
    extern int malloc_verify(void);
#endif

int line_count(char *orig);

#if defined(unix) || defined(_WIN32)
    #include <signal.h>
#endif

/*
 * Socket and TCP/IP stuff.
 */
#if defined(_WIN32)
    const	char	echo_off_str[] = {IAC, WILL, TELOPT_ECHO, '\0'};
    const	char	echo_on_str[] = {IAC, WONT, TELOPT_ECHO, '\0'};
    const	char 	go_ahead_str[] = {IAC, GA, '\0'};
    void  gettimeofday(struct timeval *tp, void *tzp);
#endif

#if    defined(unix)
    #include <fcntl.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include "telnet.h"
    const char echo_off_str[] = {IAC, WILL, TELOPT_ECHO, '\0'};
    const char echo_on_str[] = {IAC, WONT, TELOPT_ECHO, '\0'};
    const char go_ahead_str[] = {IAC, GA, '\0'};
#endif

/*
 * OS-dependent declarations.
 */
#if    defined(interactive)
    #include <net/errno.h>
    #include <sys/fnctl.h>
#endif

#if    defined(linux)
    /*
        Linux shouldn't need these. If you have a problem compiling, try
        uncommenting these functions.
    */
    /*
    int    accept        ( int s, struct sockaddr *addr, int *addrlen );
    int    bind        ( int s, struct sockaddr *name, int namelen );
    int    getpeername    ( int s, struct sockaddr *name, int *namelen );
    int    getsockname    ( int s, struct sockaddr *name, int *namelen );
    int    listen        ( int s, int backlog );
    int    read        ( int fd, char *buf, int nbyte );
    int    write        ( int fd, char *buf, int nbyte );
    */

    int close(int fd);
    int gettimeofday(struct timeval * tp, struct timezone * tzp);
    int select(int width, fd_set * readfds, fd_set * writefds, fd_set * exceptfds, struct timeval * timeout);
    int socket(int domain, int type, int protocol);
#endif

/*
 * Global variables.
 */
DESCRIPTOR_DATA *descriptor_list;    /* All open descriptors     */
DESCRIPTOR_DATA *d_next;             /* Next descriptor in loop  */
FILE *fpReserve;                     /* Reserved file handle     */
time_t current_time;                 /* time of this pulse */
bool MOBtrigger = TRUE;              /* act() switch                 */

/*
 * OS-dependent local functions.
 */
#if defined(unix) || defined(_WIN32)
    void game_loop(int control);
    int init_socket(int port);
    void init_descriptor(int control);
    bool read_from_descriptor(DESCRIPTOR_DATA * d);
#endif

/*
 * Other local functions (OS-independent).
 */
bool check_parse_name(char *name);
bool check_reconnect(DESCRIPTOR_DATA * d, char *name, bool fConn);
bool check_playing(DESCRIPTOR_DATA * d, char *name);
int main(int argc, char **argv);
void nanny(DESCRIPTOR_DATA * d, char *argument);
bool process_output(DESCRIPTOR_DATA * d, bool fPrompt);
void read_from_buffer(DESCRIPTOR_DATA * d);
void stop_idling(CHAR_DATA * ch);
void bust_a_prompt(CHAR_DATA * ch);

/* Needs to be global because of do_copyover */
int port, control;

/* Set this to the IP address you want to listen on (127.0.0.1 is good for    */
/* paranoid types who don't want the 'net at large peeking at their MUD)      */
char *mud_ipaddress = "0.0.0.0";

/*
 * Entry function - Every new beginning comes from some other beginning's end
 */
int main(int argc, char **argv)
{
    struct timeval now_time;

    /*
     * Memory debugging if needed.
     */
#if defined(MALLOC_DEBUG)
    malloc_debug(2);
#endif

    /*
     * Init time.
     */
    gettimeofday(&now_time, NULL);
    current_time = (time_t)now_time.tv_sec;
    strcpy(global.boot_time, ctime(&current_time));

    // Remove the new line that's put onto the end of current_time
    global.boot_time[strlen(global.boot_time) - 1] = '\0';

    global.copyover = FALSE;    // This will be set later if true
    global.game_loaded = FALSE;
    global.is_copyover = FALSE;
    global.copyover_timer = 0;
    global.max_on_boot = 0;

    log_string("-------------------------------------------------");
    log_string("STATUS: Initializing Game");
    log_string("        Last compile " __DATE__ " at " __TIME__);
    log_string("-------------------------------------------------");

    /*
     * Reserve one channel for our use.
     */
    if ((fpReserve = fopen(NULL_FILE, "r")) == NULL)
    {
        perror(NULL_FILE);
        exit(1);
    }

    /*
     * Get the port number.
     */
    port = 4000;
    if (argc > 1)
    {
        if (!is_number(argv[1]))
        {
            fprintf(stderr, "Usage: %s [port #]\n", argv[0]);
            exit(1);
        }
        else if ((port = atoi(argv[1])) <= 1024)
        {
            fprintf(stderr, "Port number must be above 1024.\n");
            exit(1);
        }

        /* Are we recovering from a copyover? */
        if (argv[2] && argv[2][0])
        {
            global.copyover = TRUE;
            control = atoi(argv[3]);
        }
        else
        {
            global.copyover = FALSE;
        }

    }

    /*
     * Run the game.
     */
#if defined(unix) || defined(_WIN32)

    if (!global.copyover)
        control = init_socket(port);

    if (global.copyover)
        copyover_load_descriptors();

    boot_db();

    if (global.copyover)
        copyover_recover();

    // Throw a log warning out if a whitelist is enabled.  This should be disabled by
    // default but if for some reason it gets enabled we need to give the op a clue as
    // to why people are being denied access.
    if (settings.whitelist_lock)
    {
        log_f("WARNING: A whitelist lock is enabled in the game settings.  Only");
        log_f("         hosts in the whitelist will be allowed to connect.");
    }

    if (!IS_NULLSTR(settings.mud_name))
    {
        log_f("%s is ready to rock on port %d.", strip_color(settings.mud_name), port);
    }
    else
    {
        log_f("The game is ready to rock on port %d.", port);
    }

    global.game_loaded = TRUE;

    game_loop(control);

#if defined(_WIN32)
    closesocket(control);
    WSACleanup();
#else
    close(control);
#endif

#endif

    // Alas, all good things must come to and end.
    log_string("Normal termination of game.");
    exit(0);
    return 0;
}



#if defined(unix) || defined(_WIN32)
int init_socket(int port)
{
    static struct sockaddr_in sa_zero;
    struct sockaddr_in sa;
    int x = 1;
    int fd;

#if !defined(_WIN32)
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Init_socket: socket");
        exit(1);
    }
#else
    WORD    wVersionRequested = MAKEWORD(1, 1);
    WSADATA wsaData;
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
    {
        perror("No useable WINSOCK.DLL");
        exit(1);
    }

    if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Init_socket: socket");
        exit(1);
    }
#endif

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
        (char *)&x, sizeof(x)) < 0)
    {
        perror("Init_socket: SO_REUSEADDR");

#if defined(_WIN32)
        closesocket(fd);
#else
        close(fd);
#endif

        exit(1);
    }

    sa = sa_zero;

#if !defined(_WIN32)
    sa.sin_family = AF_INET;
#else
    sa.sin_family = PF_INET;
#endif

    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = inet_addr(mud_ipaddress);
    log_f("Set IP address to %s", mud_ipaddress);

    if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
    {
        perror("Init socket: bind");

#if defined(_WIN32)
        closesocket(fd);
#else
        close(fd);
#endif

        exit(1);
    }

    if (listen(fd, 3) < 0)
    {
        perror("Init socket: listen");

#if defined(_WIN32)
        closesocket(fd);
#else
        close(fd);
#endif

        exit(1);
    }

    return fd;
}
#endif


#if defined(unix) || defined(_WIN32)
void game_loop(int control)
{
    static struct timeval null_time;
    struct timeval last_time;

#if !defined(_WIN32)
    log_string("STATUS: Setting up signal handling");

    // Route SIGPIPE to SIG_IGN
    signal(SIGPIPE, SIG_IGN);          // Broken pipe: write to pipe with no readers

    // This will keep the game running even when someone logs out of
    // the shell that might have started it.
    signal(SIGHUP, SIG_IGN);           // Hangup detected on controlling terminal or death of controlling process

    // SIGINT and SIGTERM caught and handled gracefully, these are typically
    // shutdown requests from outside the program.  This occurs in the game
    // loop after the game has loaded (or in the case of a copyover after the
    // players have loaded, it's main purpose is to shutdown gracefully and
    // save state, if it doesn't make it that far there's really nothing to
    // save or process).
    signal(SIGINT, shutdown_request);   // Interrupt from keyboard
    signal(SIGTERM, shutdown_request);  // Termination signal
    signal(SIGQUIT, shutdown_request);  // Quit from keyboard
    signal(SIGTSTP, shutdown_request);  // Stop typed at terminal
    signal(SIGPWR, shutdown_request);   // Power Failure

    // Crashes
    signal(SIGSEGV, shutdown_request);  // Invalid memory reference
    signal(SIGFPE, shutdown_request);   // Floating point exception
    signal(SIGILL, shutdown_request);   // Illegal Instruction
    signal(SIGBUS, shutdown_request);   // Bus error (bad memory access)
    signal(SIGABRT, shutdown_request);  // Abort signal from abort(3)
    signal(SIGXCPU, shutdown_request);  // CPU time limit exceeded
    signal(SIGXFSZ, shutdown_request);  // File size limit exceeded

#endif

    gettimeofday(&last_time, NULL);
    current_time = (time_t)last_time.tv_sec;

    /* Main loop */
    while (!global.shutdown)
    {
        fd_set in_set;
        fd_set out_set;
        fd_set exc_set;
        DESCRIPTOR_DATA *d;
        int maxdesc;

#if defined(MALLOC_DEBUG)
        if (malloc_verify() != 1)
            abort();
#endif

        /*
         * Poll all active descriptors.
         */
        FD_ZERO(&in_set);
        FD_ZERO(&out_set);
        FD_ZERO(&exc_set);
        FD_SET(control, &in_set);
        maxdesc = control;

        for (d = descriptor_list; d; d = d->next)
        {
            maxdesc = UMAX(maxdesc, d->descriptor);
            FD_SET(d->descriptor, &in_set);
            FD_SET(d->descriptor, &out_set);
            FD_SET(d->descriptor, &exc_set);
        }

        if (select(maxdesc + 1, &in_set, &out_set, &exc_set, &null_time) < 0)
        {
            perror("Game_loop: select: poll");
            exit(1);
        }

        /*
         * New connection?
         */
        if (FD_ISSET(control, &in_set))
            init_descriptor(control);

        /*
         * Kick out the freaky folks.
         */
        for (d = descriptor_list; d != NULL; d = d_next)
        {
            d_next = d->next;
            if (FD_ISSET(d->descriptor, &exc_set))
            {
                FD_CLR(d->descriptor, &in_set);
                FD_CLR(d->descriptor, &out_set);
                if (d->character && d->connected == CON_PLAYING)
                    save_char_obj(d->character);
                d->outtop = 0;
                close_socket(d);
            }
        }

        /*
         * Process input.
         */
        for (d = descriptor_list; d != NULL; d = d_next)
        {
            d_next = d->next;
            d->fcommand = FALSE;

            if (FD_ISSET(d->descriptor, &in_set))
            {
                if (d->character != NULL)
                    d->character->timer = 0;
                if (!read_from_descriptor(d))
                {
                    FD_CLR(d->descriptor, &out_set);
                    if (d->character != NULL && d->connected == CON_PLAYING)
                        save_char_obj(d->character);
                    d->outtop = 0;
                    close_socket(d);
                    continue;
                }
            }

            if (d->character != NULL && d->character->daze > 0)
                --d->character->daze;

            // Update any timers for the character if they're not null
            if (d->character != NULL)
            {
                timer_update(d->character);
            }

            if (d->character != NULL && d->character->wait > 0)
            {
                --d->character->wait;
                continue;
            }

            read_from_buffer(d);
            if (d->incomm[0] != '\0')
            {
                d->fcommand = TRUE;
                stop_idling(d->character);

                /* OLC */
                if (d->showstr_point)
                {
                    show_string(d, d->incomm);
                }
                else if (d->pString)
                {
                    string_add(d->character, d->incomm);
                }
                else
                {
                    switch (d->connected)
                    {
                        case CON_PLAYING:
                            if (!run_olc_editor(d))
                                substitute_alias(d, d->incomm);
                            break;
                        default:
                            nanny(d, d->incomm);
                            break;
                    }
                }

                d->incomm[0] = '\0';
            }
        }

        /*
         * Autonomous game motion.
         */
        update_handler(FALSE);

        /*
         * Output.
         */
        for (d = descriptor_list; d != NULL; d = d_next)
        {
            d_next = d->next;

            if ((d->fcommand || d->outtop > 0)
                && FD_ISSET(d->descriptor, &out_set))
            {
                if (!process_output(d, TRUE))
                {
                    if (d->character != NULL && d->connected == CON_PLAYING)
                        save_char_obj(d->character);
                    d->outtop = 0;
                    close_socket(d);
                }
            }
        }

        /*
         * Synchronize to a clock.
         * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
         * Careful here of signed versus unsigned arithmetic.
         */
#if !defined(_WIN32)
        {
            struct timeval now_time;
            long secDelta;
            long usecDelta;

            gettimeofday(&now_time, NULL);
            usecDelta = ((int)last_time.tv_usec) - ((int)now_time.tv_usec) + 1000000 / PULSE_PER_SECOND;
            secDelta = ((int)last_time.tv_sec) - ((int)now_time.tv_sec);

            while (usecDelta < 0)
            {
                usecDelta += 1000000;
                secDelta -= 1;
            }

            while (usecDelta >= 1000000)
            {
                usecDelta -= 1000000;
                secDelta += 1;
            }

            if (secDelta > 0 || (secDelta == 0 && usecDelta > 0))
            {
                struct timeval stall_time;

                stall_time.tv_usec = usecDelta;
                stall_time.tv_sec = secDelta;
                if (select(0, NULL, NULL, NULL, &stall_time) < 0)
                {
                    perror("Game_loop: select: stall");
                    exit(1);
                }
            }
        }
#else
        {
            int times_up;
            int nappy_time;
            struct _timeb start_time;
            struct _timeb end_time;
            _ftime(&start_time);
            times_up = 0;

            while (times_up == 0)
            {
                _ftime(&end_time);
                if ((nappy_time =
                    (int)(1000 *
                        (double)((end_time.time - start_time.time) +
                            ((double)(end_time.millitm -
                                start_time.millitm) /
                                1000.0)))) >=
                    (double)(1000 / PULSE_PER_SECOND))
                {
                    times_up = 1;
                }
                else
                {
                    Sleep((int)((double)(1000 / PULSE_PER_SECOND) -
                        (double)nappy_time));
                    times_up = 1;
                }
            }
        }

#endif
        gettimeofday(&last_time, NULL);
        current_time = (time_t)last_time.tv_sec;
    }

    return;
}
#endif

#if defined(unix) || defined(_WIN32)

void init_descriptor(int control)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *dnew;
    struct sockaddr_in sock;
    struct hostent *from;

#if defined(_WIN32)
    int size;
    int desc;
#else
    size_t desc;
    socklen_t size;
#endif

    size = sizeof(sock);

    getsockname(control, (struct sockaddr *) &sock, &size);
    if ((desc = accept(control, (struct sockaddr *) &sock, &size)) < 0)
    {
        perror("New_descriptor: accept");
        return;
    }

#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

#if !defined(_WIN32)
    if (fcntl(desc, F_SETFL, FNDELAY) == -1)
    {
        perror("New_descriptor: fcntl: FNDELAY");
        return;
    }
#endif

    /*
     * Cons a new descriptor.
     */
    dnew = new_descriptor();

    dnew->descriptor = desc;

    // Do they get prompted on whether they want color or do they go straight
    // to the login menu with color defaulted to true.
    if (settings.login_color_prompt)
    {
        dnew->connected = CON_COLOR;
    }
    else
    {
        dnew->connected = CON_LOGIN_MENU;
    }

    dnew->ansi = TRUE;
    dnew->showstr_head = NULL;
    dnew->showstr_point = NULL;
    dnew->outsize = 2000;
    dnew->pEdit = NULL;            /* OLC */
    dnew->pString = NULL;        /* OLC */
    dnew->editor = 0;            /* OLC */
    dnew->outbuf = alloc_mem(dnew->outsize);

    size = sizeof(sock);
    if (getpeername(desc, (struct sockaddr *) &sock, &size) < 0)
    {
        perror("New_descriptor: getpeername");

        free_string(dnew->host);
        dnew->host = str_dup("(unknown)");

        free_string(dnew->ip_address);
        dnew->ip_address = str_dup("(unknown)");
    }
    else
    {
        /*
         * Would be nice to use inet_ntoa here but it takes a struct arg,
         * which ain't very compatible between gcc and system libraries.
         */
        int addr;

        addr = ntohl(sock.sin_addr.s_addr);
        sprintf(buf, "%d.%d.%d.%d",
            (addr >> 24) & 0xFF, (addr >> 16) & 0xFF,
            (addr >> 8) & 0xFF, (addr)& 0xFF);

#if !defined(_WIN32)
        from = gethostbyaddr((char *)&sock.sin_addr, sizeof(sock.sin_addr), AF_INET);
#else
        from = gethostbyaddr((char *)&sock.sin_addr, sizeof(sock.sin_addr), PF_INET);
#endif
        // Keep both the resolved host (if it exists) and the dotted ip_address.  This
        // will make banning easier since the bans are based off of string comparisons (e.g.
        // we can ban the dotted IP always and not have to also ban the host string as exists
        // today).
        dnew->host = str_dup(from ? from->h_name : buf);
        dnew->ip_address = str_dup(buf);

        // Make a wiznet log message under sites when a new IP connects.
        char wiz_msg[MAX_STRING_LENGTH];

        if (!str_cmp(buf, dnew->host))
        {
            sprintf(wiz_msg, "New connection: %s", buf);
        }
        else
        {
            sprintf(wiz_msg, "New connection: %s (%s)", dnew->host, buf);
        }

        log_f(wiz_msg);
        wiznet(wiz_msg, NULL, NULL, WIZ_SITES, 0, 0);
    }

    if (settings.whitelist_lock && check_ban(dnew->host, BAN_WHITELIST))
    {
        char wiz_msg[MAX_STRING_LENGTH];

        write_to_descriptor(desc, "\n\rThis service is not available to the public.\r\n", dnew);

        sprintf(wiz_msg, "Banned Site: Denying access via whitelist to %s", dnew->host);
        wiznet(wiz_msg, NULL, NULL, WIZ_SITES, 0, 0);
        log_f(wiz_msg);

#if defined(_WIN32)
        closesocket(desc);
#else
        close(desc);
#endif

        free_descriptor(dnew);

        return;
    }

    /*
     * Swiftest: I added the following to ban sites.  I don't
     * endorse banning of sites, but Copper has few descriptors now
     * and some people from certain sites keep abusing access by
     * using automated 'autodialers' and leaving connections hanging.
     *
     * Furey: added suffix check by request of Nickel of HiddenWorlds.
     */
    if (check_ban(dnew->host, BAN_ALL))
    {
        write_to_descriptor(desc, "\r\nYour site has been banned from this mud.\r\n", dnew);

        char wiz_msg[MAX_STRING_LENGTH];

        sprintf(wiz_msg, "Banned Site: Denying access to %s.", dnew->host);
        log_f(wiz_msg);
        wiznet(wiz_msg, NULL, NULL, WIZ_SITES, 0, 0);

#if defined(_WIN32)
        closesocket(desc);
#else
        close(desc);
#endif

        free_descriptor(dnew);

        return;
    }
    /*
     * Init descriptor data.
     */
    dnew->next = descriptor_list;
    descriptor_list = dnew;

    /*
     * First Contact!
     */
    if (settings.login_color_prompt)
    {
        write_to_descriptor(desc, "\r\nDo you want color? (Y/N) -> ", dnew);
    }
    else
    {
        // No color prompt, show them the greeting and the menu.
        show_greeting(dnew);
        show_login_menu(dnew);
    }

    return;
}
#endif


/*
 * Closes the socket and handles notifying wiznet and free'ing info or extracting the char
 * under some circumstances like reclass.
 */
void close_socket(DESCRIPTOR_DATA * dclose)
{
    CHAR_DATA *ch;

    if (dclose->outtop > 0)
    {
        process_output(dclose, FALSE);
    }

    if (dclose->snoop_by != NULL)
    {
        write_to_buffer(dclose->snoop_by, "Your victim has left the game.\r\n");
    }

    {
        DESCRIPTOR_DATA *d;

        for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->snoop_by == dclose)
                d->snoop_by = NULL;
        }
    }

    if ((ch = dclose->character) != NULL)
    {
        log_f("Closing link to %s.", ch->name);
        /* cut down on wiznet spam when rebooting */

        /* If ch is writing note or playing, just lose link otherwise clear char */
        if ((dclose->connected == CON_PLAYING && !global.shutdown))
        {
            act("$n has lost $s link.", ch, NULL, NULL, TO_ROOM);
            wiznet("Net death has claimed $N.", ch, NULL, WIZ_LINKS, 0, 0);
            ch->desc = NULL;
        }
        else if (ch->pcdata->is_reclassing == TRUE)
        {
            // They are reclassing, disconnect them, extract them, don't save them.  Since we re-use the creation connection
            // state we must check the is_reclassing boolean which never gets saved to the pfile.
            wiznet("Net death has claimed $N while reclassing.  They have been extracted from the world and not saved.", ch, NULL, WIZ_LINKS, 0, 0);

            if (ch != NULL)
            {
                log_f("Net death has claimed %s while reclassing.  They have been extracted from the world and not saved.", ch->name);
            }

            extract_char(ch, TRUE);
        }
        else
        {
            free_char(dclose->original ? dclose->original : dclose->character);
        }
    }
    else if (dclose != NULL && dclose->host != NULL)
    {
        // Log that we've closed the link.
        log_f("Closing link to %s", dclose->host);
    }

    if (d_next == dclose)
    {
        d_next = d_next->next;
    }

    if (dclose == descriptor_list)
    {
        descriptor_list = descriptor_list->next;
    }
    else
    {
        DESCRIPTOR_DATA *d;

        for (d = descriptor_list; d && d->next != dclose; d = d->next);
        
        if (d != NULL)
        {
            d->next = dclose->next;
        }
        else
        {
            bug("close_socket: Descriptor at end of descriptor_list is NULL.", 0);
        }
        
    }

#if defined(_WIN32)
    closesocket(dclose->descriptor);
#else
    close(dclose->descriptor);
#endif

    free_descriptor(dclose);
    return;
}

/*
 * Reads incoming data from the player's connection.  If the link has dropped an end of file (EOF)
 * will be reported and false will be returned.
 */
bool read_from_descriptor(DESCRIPTOR_DATA * d)
{
    int iStart;

    /* Hold horses if pending command already. */
    if (d->incomm[0] != '\0')
        return TRUE;

    /* Check for overflow. */
    iStart = strlen(d->inbuf);
    if (iStart >= sizeof(d->inbuf) - 10)
    {
        log_f("%s input overflow!", d->host);
        write_to_descriptor(d->descriptor, "\r\n*** PUT A LID ON IT!!! ***\r\n", d);
        return FALSE;
    }

    /* Snarf input. */
#if defined(unix) || defined(_WIN32)
    for (;;)
    {
        int nRead;

#if !defined(_WIN32)
        nRead = read(d->descriptor, d->inbuf + iStart, sizeof(d->inbuf) - 10 - iStart);
#else
        nRead = recv(d->descriptor, d->inbuf + iStart, sizeof(d->inbuf) - 10 - iStart, 0);
#endif

        if (nRead > 0)
        {
            iStart += nRead;
            if (d->inbuf[iStart - 1] == '\n' || d->inbuf[iStart - 1] == '\r')
                break;
        }
        else if (nRead == 0)
        {
            if (d == NULL || d->host == NULL)
            {
                log_string("read_from_descriptor: EOF encountered on read (null descriptor or host).");
            }
            else
            {
                char wiz_msg[MAX_STRING_LENGTH];
                sprintf(wiz_msg, "Disconnected: %s", d->host);
                log_f(wiz_msg);
                wiznet(wiz_msg, NULL, NULL, WIZ_SITES, 0, 0);
            }
            return FALSE;
        }
#if defined( WIN32 )
        else if (WSAGetLastError() == WSAEWOULDBLOCK || errno == EAGAIN)
        {
            break;
        }
#endif
        else if (errno == EWOULDBLOCK)
        {
            break;
        }
        else
        {
            perror("Read_from_descriptor");
            return FALSE;
        }
    }
#endif

    d->inbuf[iStart] = '\0';
    return TRUE;
}

/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer(DESCRIPTOR_DATA * d)
{
    int i = 0;
    int j = 0;
    int k = 0;

    /*
     * Shift the input buffer if there is a tilde in it.. essentially this will in
     * effect cancel any pending commands that have been entered in (say you spam bash
     * in 3 times and are waiting for the lag to end, this will allow you to cancel the
     * 2nd and 3rd ones, perhaps to react to a disarm that you need to re-arm.  Etc.
     * Know, that it gimps the tilde.  You may choose another character here OR provide
     * another way for a user to enter the tilde, perhaps in the colors codes code.
     */
    while (d->inbuf[i] != '~' && d->inbuf[i] != '\0')
    {
        i++;
    }

    if (d->inbuf[i] == '~')
    {
        i++;

        for (j = 0; (d->inbuf[j] = d->inbuf[i + j]) != '\0'; j++);

        return;
    }

    /*
     * Hold horses if pending command already.
     */
    if (d->incomm[0] != '\0')
        return;

    /*
     * Look for at least one new line.
     */
    for (i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++)
    {
        if (d->inbuf[i] == '\0')
            return;
    }

    /*
     * Canonical input processing.
     */
    for (i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++)
    {
        if (k >= MAX_INPUT_LENGTH - 2)
        {
            write_to_descriptor(d->descriptor, "Line too long.\r\n", d);

            /* skip the rest of the line */
            for (; d->inbuf[i] != '\0'; i++)
            {
                if (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
                    break;
            }
            d->inbuf[i] = '\n';
            d->inbuf[i + 1] = '\0';
            break;
        }

        if (d->inbuf[i] == '\b' && k > 0)
        {
            --k;
        }
        else if (isascii(d->inbuf[i]) && isprint(d->inbuf[i]))
        {
            d->incomm[k++] = d->inbuf[i];
        }
    }

    /*
     * Finish off the line.
     */
    if (k == 0)
    {
        d->incomm[k++] = ' ';
    }

    d->incomm[k] = '\0';

    /*
     * Deal with bozos with #repeat 1000 ...
     */
    if (k > 1 || d->incomm[0] == '!')
    {
        if (d->incomm[0] != '!' && strcmp(d->incomm, d->inlast))
        {
            d->repeat = 0;
        }
        else
        {
            if (++d->repeat >= 25 && d->character
                && d->connected == CON_PLAYING)
            {
                log_f("%s@%s input spamming!", d->character->name, d->host);
                wiznet("Spam spam spam $N spam spam spam spam spam!", d->character, NULL, WIZ_SPAM, 0, get_trust(d->character));

                if (d->incomm[0] == '!')
                {
                    wiznet(d->inlast, d->character, NULL, WIZ_SPAM, 0, get_trust(d->character));
                }
                else
                {
                    wiznet(d->incomm, d->character, NULL, WIZ_SPAM, 0, get_trust(d->character));
                }

                d->repeat = 0;
            }
        }
    }

    /*
     * Do '!' substitution.
     */
    if (d->incomm[0] == '!')
    {
        strcpy(d->incomm, d->inlast);
    }
    else
    {
        strcpy(d->inlast, d->incomm);
    }

    /*
     * Shift the input buffer.
     */
    while (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
    {
        i++;
    }

    for (j = 0; (d->inbuf[j] = d->inbuf[i + j]) != '\0'; j++);

    return;
}

/*
 * Low level output function.
 */
bool process_output(DESCRIPTOR_DATA * d, bool fPrompt)
{
    /*
     * Bust a prompt.
     */
    if (!global.shutdown)
    {
        if (d->showstr_point)
        {
            // The color was hard coded before, now we're going to read it in from the game settings.
            sprintf(global.buf,
                    "\r\n{x[ (%sC{x)ontinue, (%sR{x)efresh, (%sB{x)ack, (%sH{x)elp, (%sE{x)nd, (%sT{x)op, (%sQ{x)uit, or %sRETURN{x ]: ",
                    settings.pager_color, settings.pager_color, settings.pager_color, settings.pager_color,
                    settings.pager_color, settings.pager_color, settings.pager_color, settings.pager_color);

            write_to_buffer(d, global.buf);

            // The paging prompt was not showing on mudlet without the telnet go ahead being sent, the
            // issue arrising because there is no line terminator on the line above as a design choice,
            // without that, mudlet waits for an extended period before rendering the line.
            if (d->character && IS_SET(d->character->comm, COMM_TELNET_GA))
            {
                write_to_buffer(d, go_ahead_str);
            }
        }
        else if (fPrompt && d->pString && d->connected == CON_PLAYING)
        {
            char buf[32];
            sprintf(buf,"{C%3.3d{x> ", line_count(*d->pString));
            write_to_buffer(d, buf);
        }
        else if (fPrompt && d->connected == CON_PLAYING)
        {
            CHAR_DATA *ch;
            CHAR_DATA *victim;

            ch = d->character;

            /* battle prompt */
            if ((victim = ch->fighting) != NULL && can_see(ch, victim))
            {
                char buf[MSL];
                sprintf(buf, "%s", health_description(ch, victim));
                write_to_buffer(d, buf);
            }

            ch = d->original ? d->original : d->character;
            if (!IS_SET(ch->comm, COMM_COMPACT))
                write_to_buffer(d, "\r\n");

            if (IS_SET(ch->comm, COMM_PROMPT))
                bust_a_prompt(d->character);

            if (IS_SET(ch->comm, COMM_TELNET_GA))
                write_to_buffer(d, go_ahead_str);
        }
    }

    /*
     * Short-circuit if nothing to write.
     */
    if (d->outtop == 0)
        return TRUE;

    /*
     * Snoop-o-rama.
     */
    if (d->snoop_by != NULL)
    {
        if (d->character != NULL)
        {
            write_to_buffer(d->snoop_by, d->character->name);
        }

        write_to_buffer(d->snoop_by, "> ");
        write_to_buffer(d->snoop_by, d->outbuf);
    }

    /*
     * OS-dependent output.
     */
    if (!write_to_descriptor(d->descriptor, d->outbuf, d))
    {
        d->outtop = 0;
        return FALSE;
    }
    else
    {
        d->outtop = 0;
        return TRUE;
    }
}

/*
 * Bust a prompt (player settable prompt)
 * coded by Morgenes for Aldara Mud
 */
void bust_a_prompt(CHAR_DATA * ch)
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    const char *str;
    const char *i;
    char *point;
    char doors[MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    bool found;
    const char *dir_name[] = {"N", "E", "S", "W", "U", "D", "-NW", "-NE", "-SW", "-SE"};
    int door;

    point = buf;
    str = ch->prompt;

    // Blake, 04/10/2015 If the prompt is null, give them the default prompt
    if (str == NULL || str[0] == '\0')
    {
        ch->prompt = str_dup("<%hhp %mm %vmv {g%r {x({C%e{x)>{x  ");
    }

    if (IS_SET(ch->comm, COMM_QUIET))
    {
        send_to_char("[{cQuiet{x] ", ch);
    }

    if (IS_SET(ch->comm, COMM_AFK))
    {
        send_to_char("<{cAFK{x> ", ch);
        return;
    }

    while (*str != '\0')
    {
        if (*str != '%')
        {
            *point++ = *str++;
            continue;
        }
        ++str;
        switch (*str)
        {
            default:
                i = " ";
                break;
            case 'e':
                // The exits that are visible from the current room.
                found = FALSE;
                doors[0] = '\0';

                for (door = 0; door < MAX_DIR; door++)
                {
                    if ((pexit = ch->in_room->exit[door]) != NULL
                        && pexit->u1.to_room != NULL
                        && (can_see_room(ch, pexit->u1.to_room)
                            || (IS_AFFECTED(ch, AFF_INFRARED)
                                && !IS_AFFECTED(ch, AFF_BLIND)))
                        && !IS_SET(pexit->exit_info, EX_CLOSED))
                    {
                        found = TRUE;
                        strcat(doors, dir_name[door]);
                    }
                }

                if (!found)
                {
                    strcat(doors, "none");
                }

                sprintf(buf2, "%s", doors);
                i = buf2;
                break;
            case 'c':
                // Ability to add a carriage return to the prompt.
                sprintf(buf2, "%s", "\r\n");
                i = buf2;
                break;
            case 'h':
            {
                // 4/10/2015, color indicators for vitals (Changes at 75%, 33%)
                sprintf(buf2, "{%s%d{x",
                    ch->hit < ch->max_hit / 3 ? "R" :
                    ch->hit < ch->max_hit * 3 / 4 ? "Y" : "W", ch->hit);

                i = buf2;
                break;
            }
            case 'H':
                // Maximum HP a player currently is able of having
                sprintf(buf2, "%d", ch->max_hit);
                i = buf2;
                break;
            case 'm':
            {
                // 4/10/2015, color indicators for vitals (Changes at 75%, 33%)
                sprintf(buf2, "{%s%d{x",
                    ch->mana < ch->max_mana / 3 ? "R" :
                    ch->mana < ch->max_mana * 3 / 4 ? "Y" : "W", ch->mana);

                i = buf2;
                break;
            }
            case 'M':
                // Maximum mana a player currently is able of having
                sprintf(buf2, "%d", ch->max_mana);
                i = buf2;
                break;
            case 'v':
            {
                // 4/10/2015, color indicators for vitals (Changes at 75%, 33%)
                sprintf(buf2, "{%s%d{x",
                    ch->move < ch->max_move / 3 ? "R" :
                    ch->move < ch->max_move * 3 / 4 ? "Y" : "W", ch->move);
                i = buf2;
                break;
            }
            case 'V':
                // Maximum movement a player currently is able of having
                sprintf(buf2, "%d", ch->max_move);
                i = buf2;
                break;
            case 'x':
                // The amount of experience in total a player has
                sprintf(buf2, "%d", ch->exp);
                i = buf2;
                break;
            case 'X':
                // The experience a user has until their next level.
                sprintf(buf2, "%d", IS_NPC(ch) ? 0 :
                    (ch->level + 1) * exp_per_level(ch, ch->pcdata->points) - ch->exp);
                i = buf2;
                break;
            case 'g':
                // Gold the character has on them
                sprintf(buf2, "%ld", ch->gold);
                i = buf2;
                break;
            case 's':
                // Silver the character has on them
                sprintf(buf2, "%ld", ch->silver);
                i = buf2;
                break;
            case 'S':
                // Battle stance
                sprintf(buf2, "%s", capitalize(get_stance_name(ch)));
                i = buf2;
                break;
            case 't':
                // Time of the day
                sprintf(buf2, "%d%s", (time_info.hour % 12 == 0) ? 12 : time_info.hour % 12, time_info.hour >= 12 ? "pm" : "am");
                i = buf2;
                break;
            case 'a':
                // Alignment
                sprintf(buf2, "%s", IS_GOOD(ch) ? "good" : IS_EVIL(ch) ? "evil" : "neutral");
                i = buf2;
                break;
            case 'r':
                // The name of the room a player is in.
                if (ch->in_room != NULL)
                {
                    sprintf(buf2, "%s",
                        ((!IS_NPC
                        (ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
                            || (!IS_AFFECTED(ch, AFF_BLIND)
                                && !room_is_dark(ch->in_room))) ? ch->in_room->name : "darkness");
                }
                else
                {
                    sprintf(buf2, " ");
                }

                i = buf2;
                break;
            case 'R':
                // The vnum of the room the player is in (Immortal only)
                if (IS_IMMORTAL(ch) && ch->in_room != NULL)
                {
                    sprintf(buf2, "%d", ch->in_room->vnum);
                }
                else
                {
                    sprintf(buf2, " ");
                }

                i = buf2;
                break;
            case 'w':
                // The level an immortal is wizi (hidden) at
                if (IS_IMMORTAL(ch) && ch->invis_level > 0)
                {
                    sprintf(buf2, "(Wizi@%d)", ch->invis_level);
                }
                else
                {
                    buf2[0] = '\0';
                }

                i = buf2;
                break;
            case 'i':
                // The level an immortal is incog (hidden except to in room) at
                if (IS_IMMORTAL(ch) && ch->incog_level > 0)
                {
                    sprintf(buf2, "(Incog@%d)", ch->incog_level);
                }
                else
                {
                    buf2[0] = '\0';
                }
                i = buf2;
                break;
            case 'z':
                // The name of the area a player is in (immortal only)
                if (IS_IMMORTAL(ch) && ch->in_room != NULL)
                {
                    sprintf(buf2, "%s", ch->in_room->area->name);
                }
                else
                {
                    sprintf(buf2, " ");
                }
                i = buf2;
                break;
            case 'y':
                // What the players current wimpy is set at (the HP at which they auto flee)
                sprintf(buf2, "%d", ch->wimpy);
                i = buf2;
                break;
            case 'q':
                // Quest Points (Which are only for players, not for NPC's)
                if (!IS_NPC(ch) && ch->pcdata != NULL)
                {
                    sprintf(buf2, "%d", ch->pcdata->quest_points);
                }
                else
                {
                    sprintf(buf2, "%d", 0);
                }

                i = buf2;
                break;
            case '%':
                // The ability to show a percent sign.
                sprintf(buf2, "%%");
                i = buf2;
                break;
            case 'o':
                // The name of the OLC editor the player is currently in.
                sprintf(buf2, "%s", olc_ed_name(ch));
                i = buf2;
                break;
            case 'O':
                // The vnum from OLC the player is currently editing.
                sprintf(buf2, "%s", olc_ed_vnum(ch));
                i = buf2;
                break;
        }

        ++str;

        while ((*point = *i) != '\0')
        {
            ++point, ++i;
        }
    }

    *point = '\0';
    write_to_buffer(ch->desc, buf);
    write_to_buffer(ch->desc, "{x");

    // Do we send the prefix to the line also?
    if (ch->prefix[0] != '\0')
    {
        write_to_buffer(ch->desc, ch->prefix);
    }

    return;
}

/*
 * Append onto an output buffer.
 */
void write_to_buffer(DESCRIPTOR_DATA *d, const char *txt)
{
    int length = 0;

    // Find the length
    length = strlen(txt);

    // can't update null descriptor
    if (d == NULL)
        return;

    /*
     * Initial \r\n if needed.
     */
    if (d->outtop == 0 && !d->fcommand)
    {
        d->outbuf[0] = '\r';
        d->outbuf[1] = '\n';
        d->outtop = 2;
    }

    /*
     * Expand the buffer as needed.
     */
    while (d->outtop + length >= d->outsize)
    {
        char *outbuf;

        if (d->outsize >= 32000)
        {
            char abuf[MAX_STRING_LENGTH];
            char* pOverflowMsg = "There is too much for you to take in. Your senses shut down.\r\n";

            if (d->character != NULL)
            {
                sprintf(abuf, "Write to buffer - Buffer overflow from %s. Check what this char is doing.\r\n",
                    d->character->name);
                bug(abuf, 0);
            }
            else
            {
                bug("write_to_buffer - Buffer overflow from NULL character.\r\n", 0);
            }

            free_mem(d->outbuf, d->outsize);
            outbuf = alloc_mem(2048);
            strncpy(outbuf, pOverflowMsg, strlen(pOverflowMsg));
            d->outbuf = outbuf;
            d->outsize = 2048;
            d->outtop = strlen(pOverflowMsg);
            return;
        }
        outbuf = alloc_mem(2 * d->outsize);
        strncpy(outbuf, d->outbuf, d->outtop);
        free_mem(d->outbuf, d->outsize);
        d->outbuf = outbuf;
        d->outsize *= 2;
    }

    /*
     * Copy.
     */
    strcpy(d->outbuf + d->outtop, txt);
    d->outtop += length;
    return;
} // end void write_to_buffer

/*
 * Writes a message to all descriptors using write_to_descriptor which is
 * low level with no color.  This can be used to send a message to every
 * connected socket.  This is useful where no other logic in the loop is
 * needed.
 */
void write_to_all_desc(char *txt)
{
    DESCRIPTOR_DATA *d;

    for (d = descriptor_list; d != NULL; d = d->next)
    {
        write_to_descriptor(d->descriptor, txt, d);
    }
}

/*
 * A method for handling sending messages about a copyover to all players, this
 * will handle some formatting for us.
 */
void copyover_broadcast(char *txt, bool show_last_result)
{
    char buf[MAX_STRING_LENGTH];

    if (show_last_result)
    {
        if (global.last_boot_result == UNKNOWN)
            sprintf(buf, "[ {yUnknown{x ]\r\n%-55s", txt);
        else if (global.last_boot_result == SUCCESS)
            sprintf(buf, "[ {GSuccess{x ]\r\n%-55s", txt);
        else if (global.last_boot_result == FAILURE)
            sprintf(buf, "[ {RFailure{x ]\r\n%-55s", txt);
        else if (global.last_boot_result == WARNING)
            sprintf(buf, "[ {YWarning{x ]\r\n%-55s", txt);
        else if (global.last_boot_result == MISSING)
            sprintf(buf, "[ {gMissing{x ]\r\n%-55s", txt);
        else if (global.last_boot_result == DISABLE)
            sprintf(buf, "[ {BDisable{x ]\r\n%-55s", txt);
        else if (global.last_boot_result == DEFAULT)
            sprintf(buf, "[ {GDefault{x ]\r\n%-55s", txt);
        else
        {
            sprintf(buf, "[ {yUnknown{x ]\r\n%-55s", txt);
        }
    }
    else
    {
        sprintf(buf, "%-55s", txt);
    }

    write_to_all_desc(buf);

    // Reset the last boot result to UNKNOWN so it indicates on the copyover
    // if one of the boot functions doesn't set it's result when it's finished.
    global.last_boot_result = UNKNOWN;

}

/*
 * Writes a message to all characters through the send_to_char mechansim.
 */
void send_to_all_char(char *txt)
{
    DESCRIPTOR_DATA *d;

    for (d = descriptor_list; d; d = d->next)
    {
        if (d->connected == CON_PLAYING && d->character != NULL)
        {
            send_to_char(txt, d->character);
        }
    }

    return;
}

/*
 * Lowest level output function.  Shorthand for write_to_descriptor if the DESCRIPTOR_DATA
 * isn't null.
 */
bool write_to_desc(char *str, DESCRIPTOR_DATA *d)
{
    if (d != NULL)
    {
        return write_to_descriptor(d->descriptor, str, d);
    }

    return FALSE;
}

/*
 * printf support for low level write to descriptor.
 */
void writef(DESCRIPTOR_DATA * d, char *fmt, ...)
{
    char buf[MAX_STRING_LENGTH];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    if (d != NULL)
    {
        write_to_descriptor(d->descriptor, buf, d);
    }
}

/*
 * - Lowest level output function.
 * - Write a block of text to the file descriptor.
 * - If this gives errors on very long blocks (like 'ofind all') then
 *   try lowering the max block size.
 * - Lope's color code moved into here so color can be used from the lowest level
 */
bool write_to_descriptor(int desc, char *str, DESCRIPTOR_DATA *d)
{
    int length;
    register int iStart;
    register int nWrite;
    int nBlock;
    BUFFER *txt;
    char *point;
    char buf[2];
    char buf2[MAX_STRING_LENGTH];

    txt = new_buf();
    buf[1] = '\0';
    for (point = str; *point; point++)
    {
        if (*point == '{' && (point + 1 != NULL))
        {
            ++point;
            if (d->ansi & 1 || *point == '!' || *point == '-')
            {
                switch (*point)
                {
                    case 'x':
                        sprintf(buf2, CLEAR);
                        break;
                    case 'b':
                        sprintf(buf2, C_BLUE);
                        break;
                    case 'c':
                        sprintf(buf2, C_CYAN);
                        break;
                    case 'g':
                        sprintf(buf2, C_GREEN);
                        break;
                    case 'm':
                        sprintf(buf2, C_MAGENTA);
                        break;
                    case 'r':
                        sprintf(buf2, C_RED);
                        break;
                    case 'w':
                        sprintf(buf2, C_WHITE);
                        break;
                    case 'y':
                        sprintf(buf2, C_YELLOW);
                        break;
                    case 'B':
                        sprintf(buf2, C_B_BLUE);
                        break;
                    case 'C':
                        sprintf(buf2, C_B_CYAN);
                        break;
                    case 'G':
                        sprintf(buf2, C_B_GREEN);
                        break;
                    case 'M':
                        sprintf(buf2, C_B_MAGENTA);
                        break;
                    case 'R':
                        sprintf(buf2, C_B_RED);
                        break;
                    case 'W':
                        sprintf(buf2, C_B_WHITE);
                        break;
                    case 'Y':
                        sprintf(buf2, C_B_YELLOW);
                        break;
                    case 'Z':
                        sprintf(buf2, C_BLACK);
                        break;
                    case 'D':
                        sprintf(buf2, C_D_GREY);
                        break;
                    case '!':
                        sprintf(buf2, "%c", '\a');
                        break;
                    case '{':
                        sprintf(buf2, "%c", '{');
                        break;
                    case '*':
                        sprintf(buf2, BLINK);
                        break;
                    case '_':
                        sprintf(buf2, UNDERLINE);
                        break;
                    case '&':
                        sprintf(buf2, REVERSE);
                        break;
                    case '-':
                        sprintf(buf2, "%c", '~');
                        break;
                    default:
                        sprintf(buf2, CLEAR);
                        break;
                }
                add_buf(txt, buf2);
                continue;
            }
            continue;
        }

        buf[0] = *point;
        add_buf(txt, buf);
    }

    length = strlen(buf_string(txt));

    for (iStart = 0; iStart < length; iStart += nWrite)
    {
        nBlock = UMIN(length - iStart, 4096);

#if !defined( _WIN32 )
        // POSIX
        if ((nWrite = write(desc, buf_string(txt) + iStart, nBlock)) < 0)
        {
            free_buf(txt); // Add this here, seemed like it needed to be free'd success or fail
            return FALSE;
        }
#else
        // Windows
        if ((nWrite = send(desc, buf_string(txt) + iStart, nBlock, 0)) < 0)
        {
            free_buf(txt);
            return FALSE;
        }
#endif

    }

    free_buf(txt);
    return TRUE;
} // end bool write_to_descriptor

/*
 * Parse a name for acceptability.
 */
bool check_parse_name(char *name)
{
    int x;

    // Reserved words.
    if (is_exact_name(name, "all auto immortal self someone something the you loner none"))
    {
        return FALSE;
    }

    // Check against command table.
    for (x = 0; cmd_table[x].name[0] != '\0'; x++)
    {
        if (!str_cmp(name, cmd_table[x].name))
        {
            return FALSE;
        }
    }

    // Check clans
    for (x = 0; x < MAX_CLAN; x++)
    {
        if (LOWER(name[0]) == LOWER(clan_table[x].name[0])
            && !str_cmp(name, clan_table[x].name))
            return FALSE;
    }

    // Length restrictions
    if (strlen(name) < 2)
        return FALSE;

    if (strlen(name) > 12)
        return FALSE;

    // Alphanumerics only.  Lock out IllIll twits.
    {
        char *pc;
        bool fIll, adjcaps = FALSE, cleancaps = FALSE;
        int total_caps = 0;

        fIll = TRUE;
        for (pc = name; *pc != '\0'; pc++)
        {
            if (!isalpha(*pc) && *pc != '\'')
                return FALSE;

            if (isupper(*pc))
            {                    /* ugly anti-caps hack */
                if (adjcaps)
                {
                    cleancaps = TRUE;
                }
                total_caps++;
                adjcaps = TRUE;
            }
            else
            {
                adjcaps = FALSE;
            }

            if (LOWER(*pc) != 'i' && LOWER(*pc) != 'l')
            {
                fIll = FALSE;
            }
        }

        if (fIll)
            return FALSE;

        if (cleancaps || (total_caps > (strlen(name)) / 2 && strlen(name) < 3))
            return FALSE;
    }

    // Prevent players from naming themselves after mobs.
    {
        extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
        MOB_INDEX_DATA *pMobIndex;
        int iHash;

        for (iHash = 0; iHash < MAX_KEY_HASH; iHash++)
        {
            for (pMobIndex = mob_index_hash[iHash];
            pMobIndex != NULL; pMobIndex = pMobIndex->next)
            {
                if (is_name(name, pMobIndex->player_name))
                    return FALSE;
            }
        }
    }

    /*
     * Edwin's been here too. JR -- 10/15/00
     *
     * Check names of people playing. Yes, this is necessary for multiple
     * newbies with the same name (thanks Saro)
     */
    if (descriptor_list)
    {
        int count = 0;
        DESCRIPTOR_DATA *d, *dnext;

        for (d = descriptor_list; d != NULL; d = dnext)
        {
            dnext = d->next;
            if (d->connected != CON_PLAYING&&d->character&&d->character->name
                && d->character->name[0] && !str_cmp(d->character->name, name))
            {
                count++;
                close_socket(d);
            }
        }
        if (count)
        {
            char buf[MAX_STRING_LENGTH];
            sprintf(buf, "Double newbie alert (%s)", name);
            wiznet(buf, NULL, NULL, WIZ_LOGINS, 0, 0);

            return FALSE;
        }
    }

    return TRUE;
}

/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect(DESCRIPTOR_DATA * d, char *name, bool fConn)
{
    CHAR_DATA *ch;

    for (ch = char_list; ch != NULL; ch = ch->next)
    {
        if (!IS_NPC(ch)
            && (!fConn || ch->desc == NULL)
            && !str_cmp(d->character->name, ch->name))
        {
            if (fConn == FALSE)
            {
                free_string(d->character->pcdata->pwd);
                d->character->pcdata->pwd = str_dup(ch->pcdata->pwd);
            }
            else
            {
                free_char(d->character);
                d->character = ch;
                ch->desc = d;
                ch->timer = 0;
                send_to_char("Reconnecting. Type replay to see missed tells.\r\n", ch);
                act("$n has reconnected.", ch, NULL, NULL, TO_ROOM);

                log_f("%s@%s reconnected.", ch->name, d->host);
                wiznet("$N groks the fullness of $S link.", ch, NULL, WIZ_LINKS, 0, 0);
                d->connected = CON_PLAYING;
            }
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * Check if already playing.
 */
bool check_playing(DESCRIPTOR_DATA * d, char *name)
{
    DESCRIPTOR_DATA *dold;

    for (dold = descriptor_list; dold; dold = dold->next)
    {
        if (dold != d
            && dold->character != NULL
            && dold->connected != CON_GET_NAME
            && dold->connected != CON_GET_OLD_PASSWORD
            && !str_cmp(name, dold->original
                ? dold->original->name : dold->character->name))
        {
            write_to_buffer(d, "That character is already playing.\r\n");
            write_to_buffer(d, "Do you wish to connect anyway (Y/N)?");
            d->connected = CON_BREAK_CONNECT;
            return TRUE;
        }
    }

    return FALSE;
}

void stop_idling(CHAR_DATA * ch)
{
    if (ch == NULL
        || ch->desc == NULL
        || ch->desc->connected != CON_PLAYING
        || ch->was_in_room == NULL
        || ch->in_room != get_room_index(ROOM_VNUM_LIMBO)) return;

    ch->timer = 0;
    char_from_room(ch);
    char_to_room(ch, ch->was_in_room);
    ch->was_in_room = NULL;
    act("$n has returned from the void.", ch, NULL, NULL, TO_ROOM);
    return;
}

/*
 * Write to one char.
 */
void send_to_char(const char *txt, CHAR_DATA *ch)
{
    if (txt != NULL)
    {
        if (ch->desc != NULL)
        {
            write_to_buffer(ch->desc, txt);
        }
    }

    return;
} // end send_to_char

/*
 * Page to one descriptor using Lope's color (wraps write_to_buffer)
 */
void send_to_desc(const char *txt, DESCRIPTOR_DATA * d)
{
    write_to_buffer(d, txt);
    return;
}

/*
 * Send a page to one char.
 */
void page_to_char(const char *txt, CHAR_DATA *ch)
{
    if (txt == NULL || ch->desc == NULL)
    {
        return;
    }

    if (ch->lines == 0 && strlen(txt) <= 16384)
    {
        send_to_char(txt, ch);
        return;
    }

    if (ch->desc->showstr_head &&
        (strlen(txt) + strlen(ch->desc->showstr_head) + 1) < 32000)
    {
        char *temp = alloc_mem(strlen(txt) + strlen(ch->desc->showstr_head) + 1);
        strcpy(temp, ch->desc->showstr_head);
        strcat(temp, txt);
        ch->desc->showstr_point = temp + (ch->desc->showstr_point - ch->desc->showstr_head);
        free_mem(ch->desc->showstr_head, strlen(ch->desc->showstr_head) + 1);
        ch->desc->showstr_head = temp;
    }
    else
    {
        if (ch->desc->showstr_head)
        {
            free_mem(ch->desc->showstr_head, strlen(ch->desc->showstr_head) + 1);
        }
        ch->desc->showstr_head = alloc_mem(strlen(txt) + 1);
        strcpy(ch->desc->showstr_head, txt);
        ch->desc->showstr_point = ch->desc->showstr_head;
        show_string(ch->desc, "");
    }

} // end void page_to_char

/*
 * String Pager
 *
 * Some logic updated from ACK mud and the ROM ICE project.
 */
void show_string(struct descriptor_data *d, char *input)
{
    char buffer[MAX_STRING_LENGTH * 4];
    char buf[MAX_INPUT_LENGTH];
    register char *scan, *chk;
    int lines = 0, toggle = 1;
    int space;
    int show_lines;
    int max_linecount;
    bool newline = FALSE;

    if (d->character)
    {
        show_lines = d->character->lines;
    }
    else
    {
        show_lines = 0;
    }

    if (strlen(d->showstr_point) >= 16384)
    {
        show_lines = PAGELEN;
    }

    if (show_lines == 0)
    {
        bugf("show_string - show_lines == 0, returning without processing further");
        return;
    }

    one_argument(input, buf);

    if (strlen(buf) > 2)
    {
        if (d->showstr_head)
        {
            free_mem(d->showstr_head, strlen(d->showstr_head) + 1);
            d->showstr_head = 0;
        }

        d->showstr_point = 0;

        if (d->connected == CON_PLAYING)
        {
            substitute_alias(d, d->incomm);
        }

        return;
    }

    max_linecount = line_count(d->showstr_head);

    switch (UPPER(buf[0]))
    {
        case '\0':
        case 'C': /* show next page of text */
            lines = 0;
            break;
        case 'R': /* refresh current page of text */
            lines = -(d->character->lines);
            if ((max_linecount - line_count(d->showstr_point)) <= d->character->lines)
            {
                lines--;
            }
            break;
        case 'B': /* scroll back a page of text */
            lines = -(2 * d->character->lines);
            break;
        case 'E': /* Go to the end of the buffer - page */
            lines = max_linecount - (d->character->lines) - (d->character->lines) / 2;
            break;
        case 'T': /* Go to the top of the buffer - page */
            lines = 0;
            d->showstr_point = d->showstr_head;
            break;
        case 'H': /* Show some help */
            write_to_buffer(d, "\r\nC, or Return = continue, R = redraw this page, B = back one page\r\n");
            write_to_buffer(d, "H = this help, E = End of document, T = Top of document, Q or other keys = exit.\r\n");
            return;
            break;
        case 'Q': /*otherwise, stop the text viewing */
            if (d->showstr_head)
            {
                free_mem(d->showstr_head, strlen(d->showstr_head) + 1);
                d->showstr_head = 0;
            }

            d->showstr_point = 0;
            return;
        default:
            if (d->showstr_head)
            {
                free_mem(d->showstr_head, strlen(d->showstr_head) + 1);
                d->showstr_head = 0;
            }
            d->showstr_point = 0;

            if (d->connected == CON_PLAYING)
            {
                substitute_alias(d, d->incomm);
            }
            return;
    }

    if (max_linecount > d->character->lines)
    {
        write_to_buffer(d, "\r\n");
    }

    /* do any backing up necessary */
    if (lines < 0)
    {
        // Back option which takes you back one page
        for (scan = d->showstr_point; scan > d->showstr_head; scan--)
        {
            if ((*scan == '\n') || (*scan == '\r'))
            {
                toggle = -toggle;
                if (toggle < 0)
                {
                    if (!(++lines))
                        break;
                }
            }
        }
        d->showstr_point = scan;
    }
    else if (lines > 0)
    {
        // End option that takes you to the last page
        for (scan = d->showstr_head; *scan; scan++)
        {
            if ((*scan == '\n') || (*scan == '\r'))
            {
                toggle = -toggle;
                if (toggle < 0)
                {
                    if (!(--lines))
                        break;
                }
            }
        }
        d->showstr_point = scan;
    }

    /* show a chunk */
    lines = 0;
    space = (MAX_STRING_LENGTH * 4) - 100;

    for (scan = buffer;; scan++, d->showstr_point++)
    {
        space--;

        if (((*scan = *d->showstr_point) == '\r' || *scan == '\n') && space > 0)
        {
            if (newline == FALSE)
            {
                newline = TRUE;
                lines++;
            }
            else
            {
                newline = FALSE;
            }
        }
        else if (!*scan || ( show_lines > 0 && lines >= show_lines) || space <= 0)
        {
            *scan = '\0';
            write_to_buffer(d, buffer);
            for (chk = d->showstr_point; isspace(*chk); chk++);
            
            if (!*chk)
            {
                if (d->showstr_head)
                {
                    free_mem(d->showstr_head, strlen( d->showstr_head ) + 1);
                    d->showstr_head = 0;
                }
                d->showstr_point = 0;
            }
            
            return;
        }
        else
        {
            newline = FALSE;
        }
    }
    return;

} // end void show_string

void act_new(const char *format, CHAR_DATA * ch, const void *arg1, const void *arg2, int type, int min_pos)
{
    static char *const he_she[] = {"it", "he", "she"};
    static char *const him_her[] = {"it", "him", "her"};
    static char *const his_her[] = {"its", "his", "her"};

    char buf[MAX_STRING_LENGTH];
    char fname[MAX_INPUT_LENGTH];
    CHAR_DATA *to;
    CHAR_DATA *vch = (CHAR_DATA *)arg2;
    OBJ_DATA *obj1 = (OBJ_DATA *)arg1;
    OBJ_DATA *obj2 = (OBJ_DATA *)arg2;
    const char *str;
    const char *i;
    char *point;

    /*
     * Discard null and zero-length messages.
     */
    if (format == NULL || format[0] == '\0')
        return;

    /* discard null rooms and chars */
    if (ch == NULL || ch->in_room == NULL)
        return;

    to = ch->in_room->people;
    if (type == TO_VICT)
    {
        if (vch == NULL)
        {
            bug("Act: null vch with TO_VICT.", 0);
            return;
        }

        if (vch->in_room == NULL)
            return;

        to = vch->in_room->people;
    }

    for (; to != NULL; to = to->next_in_room)
    {
        if ((!IS_NPC(to) && to->desc == NULL)
            || (IS_NPC(to) && !HAS_TRIGGER(to, TRIG_ACT))
            || to->position < min_pos)
            continue;

        if ((type == TO_CHAR) && to != ch)
            continue;
        if (type == TO_VICT && (to != vch || to == ch))
            continue;
        if (type == TO_ROOM && to == ch)
            continue;
        if (type == TO_NOTVICT && (to == ch || to == vch))
            continue;

        point = buf;
        str = format;
        while (*str != '\0')
        {
            if (*str != '$')
            {
                *point++ = *str++;
                continue;
            }
            ++str;
            i = " <@@@> ";

            if (arg2 == NULL && *str >= 'A' && *str <= 'Z')
            {
                bug("Act: missing arg2 for code %d.", *str);
                i = " <@@@> ";
            }
            else
            {
                switch (*str)
                {
                    /* Added checking of pointers to each case after
                     * reading about the bug on Edwin's page.
                     * JR -- 10/15/00
                     */
                    default:
                        bug("Act: bad code %d.", *str);
                        i = " <@@@> ";
                        break;
                        /* Thx alex for 't' idea */
                    case 't':
                        if (arg1)
                            i = (char *)arg1;
                        else
                            bug("Act: bad code $t for 'arg1'", 0);
                        break;
                    case 'T':
                        if (arg2)
                            i = (char *)arg2;
                        else
                            bug("Act: bad code $T for 'arg2'", 0);
                        break;
                    case 'n':
                        if (ch && to)
                            i = PERS(ch, to);
                        else
                            bug("Act: bad code $n for 'ch' or 'to'", 0);
                        break;
                    case 'N':
                        if (vch && to)
                            i = PERS(vch, to);
                        else
                            bug("Act: bad code $N for 'vch' or 'to'", 0);
                        break;
                    case 'e':
                        if (ch)
                            i = he_she[URANGE(0, ch->sex, 2)];
                        else
                            bug("Act: bad code $e for 'ch'", 0);
                        break;
                    case 'E':
                        if (vch)
                            i = he_she[URANGE(0, vch->sex, 2)];
                        else
                            bug("Act: bad code $E for 'vch'", 0);
                        break;
                    case 'm':
                        if (ch)
                            i = him_her[URANGE(0, ch->sex, 2)];
                        else
                            bug("Act: bad code $m for 'ch'", 0);
                        break;
                    case 'M':
                        if (vch)
                            i = him_her[URANGE(0, vch->sex, 2)];
                        else
                            bug("Act: bad code $M for 'vch'", 0);
                        break;
                    case 's':
                        if (ch)
                            i = his_her[URANGE(0, ch->sex, 2)];
                        else
                            bug("Act: bad code $s for 'ch'", 0);
                        break;
                    case 'S':
                        if (vch)
                            i = his_her[URANGE(0, vch->sex, 2)];
                        else
                            bug("Act: bad code $S for 'vch'", 0);
                        break;
                    case 'o':
                        if (to && obj1)
                            i = can_see_obj(to, obj1) ? obj_short(obj1) : "something";
                        else
                            bug("Act: bad code $o for 'to' or 'obj1'", 0);
                        break;

                    case 'O':
                        if (to && obj2)
                            i = can_see_obj(to, obj2) ? obj_short(obj2) : "something";
                        else
                            bug("Act: bad code $O for 'to' or 'obj2'", 0);
                        break;

                    case 'p':
                        if (to && obj1)
                            i = can_see_obj(to, obj1)
                            ? obj1->short_descr : "something";
                        else
                            bug("Act: bad code $p for 'to' or 'obj1'", 0);
                        break;
                    case 'P':
                        if (to && obj2)
                            i = can_see_obj(to, obj2)
                            ? obj2->short_descr : "something";
                        else
                            bug("Act: bad code $P for 'to' or 'obj2'", 0);
                        break;
                    case 'd':
                        if (arg2 == NULL || ((char *)arg2)[0] == '\0')
                        {
                            i = "door";
                        }
                        else
                        {
                            one_argument((char *)arg2, fname);
                            i = fname;
                        }
                        break;
                }
            }

            ++str;
            while ((*point = *i) != '\0')
                ++point, ++i;
        }

        *point++ = '\r';
        *point++ = '\n';
        *point = '\0';

        /*
         * Kludge to capitalize first letter of buffer, trying
         * to account for { color codes. -- JR 09/09/00
         */
        if (buf[0] == 123)
            buf[2] = UPPER(buf[2]);
        else
            buf[0] = UPPER(buf[0]);

        if (to->desc && (to->desc->connected == CON_PLAYING))
            write_to_buffer(to->desc, buf); /* changed to buffer to reflect prev. fix */
        else if (MOBtrigger)
            mp_act_trigger(buf, to, ch, arg1, arg2, TRIG_ACT);
    }
    return;
}

int obj_short_item_cnt;

/*
 * Gives a short description of an object with additional handling for counts
 * where a player might be using multiple of an item.
 */
char *obj_short(OBJ_DATA *obj)
{
    static char buf[MAX_STRING_LENGTH];
    int count;

    if (obj->count > 1)
    {
        count = obj->count;
    }
    else if (obj_short_item_cnt > 1)
    {
        count = obj_short_item_cnt;
    }
    else
    {
        count = 1;
    }

    if (count > 1)
    {
        if (!str_prefix("a ", obj->short_descr))
        {
            sprintf(buf, "%d %ss", count, &obj->short_descr[2]);
        }
        else if (!str_prefix("an ", obj->short_descr))
        {
            sprintf(buf, "%d %ss", count, &obj->short_descr[3]);
        }
        else if (!str_prefix("the ", obj->short_descr))
        {
            sprintf(buf, "%d %ss", count, &obj->short_descr[4]);
        }
        else
        {
            sprintf(buf, "%d %ss", count, obj->short_descr);
        }
        return buf;
    }
    return obj->short_descr;
} // end obj_short

/*
 * Function to remove color codes from a string.  Added to help remove color codes
 * from strings that were colored with Lope's color (e.g. if we want to remove color
 * codes from a log message, etc).
 */
char *strip_color(char *string)
{
    static char buf[MAX_STRING_LENGTH];
    char *point;
    int count = 0;

    buf[0] = '\0';

    for (point = string; *point; point++)
    {
        if (*point == '{' && (point + 1 != NULL))
        {
            ++point;

            if (*point == '{')
            {
                --point;
                buf[count] = *point;
                ++point;
                ++count;
            }
        }
        else
        {
            buf[count] = *point;
            ++count;
        }
    }

    buf[count] = '\0';
    return buf;
} // end strip_color

/*
 * Returns the display length of a string as it would be on the screen by
 * skipping any color codes that are found.  This is a modified version of
 * Quixadhal's code from SmaugFUSS.
 */
int color_strlen(const char *text)
{
    register unsigned int i = 0;
    int len = 0;

    // Ditch out of it's a null
    if( !text || !*text )
    {
        return 0;
    }

    for(i = 0; i < strlen(text);)
    {
        switch (text[i])
        {
            case '{':
                // It's a bracket which we will skip because it's not displayed, so increment
                // the string counter, but not the display length.
                i++;

                // There is another character after the color code bracket, skip it also as it
                // will not be displayed but only if it doesn't go out of the array bounds.
                if (i < strlen(text))
                {
                    i++;
                }

                break;
             default:
                // This is a displayed character, both the display length and the string counter
                // will be incremented.
                len++;
                i++;
                break;
        }
    }
    return len;
}

/*
 * Written by John Booth, EOD (as printf_to_char), renamed to sendf for easier
 * coding as we make this a standard.  This assumes nothing will be larger
 * than MAX_STRING_LENGTH and in the end passes the call down to send_to_char.
 */
void sendf(CHAR_DATA * ch, char *fmt, ...)
{
    char buf[MAX_STRING_LENGTH];
    va_list args;
    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    send_to_char(buf, ch);
}

/*
 * Signal handler for game shutdown.  This will be executed when SIGINT and/or SIGTERM
 * are signaled (typically from something killing the process).  It will attempt to safely
 * shutdown the game and save characters and other objects that are persisted across boots.
 * This does not handle other crashing scenarios currently as it would interrupt the process
 * that makes a core file (which is needed to debug crashes).  This will go through the
 * a similiar type process as do_shutdown.
 */
void shutdown_request(int a)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d, *d_next;
    CHAR_DATA *vch;

    // Log the message as a bug for review
    bugf("Emergency System Shutdown - Signal Received %d", a);

    // If the last command exists, which it should, log it out so we have an idea of what
    // crashed us if it was a crash.
    if (!IS_NULLSTR(global.last_command))
    {
        log_f("Last Command: %s", global.last_command);
    }
    else
    {
        log_string("Last Command: (null)");
    }
    // Send a message to the players
    sprintf(buf, "\r\n{RWARNING{x: emergency-{R{*shutdown{x by {BSystem{x.\r\n{WReason{x: System received shutdown request or crash signal (%d)\r\n\r\n", a);
    send_to_all_char(buf);

    // Save the characters gear, close their sockets
    global.shutdown = TRUE;
    for (d = descriptor_list; d != NULL; d = d_next)
    {
        d_next = d->next;
        vch = d->original ? d->original : d->character;

        if (vch != NULL)
        {
            save_char_obj(vch);
        }

        close_socket(d);
    }

    // Save special items like donation pits and corpses
    save_game_objects();

    // Save the current statistics to file
    save_statistics();

    // Close the database
    close_db();

    // This trick will allow the core to still be dumped even after we've handled the signal.
    // Alas, all good things must come to an end.
    signal(a, SIG_DFL);

    // Force core?
    abort();

#if defined( _WIN32 )
    global.shutdown = TRUE;
#else
    kill(getpid(), SIGSEGV);
#endif

    return;
}

/*
 * Windows support functions
 * (copied from Envy)
 */
#if defined( _WIN32 )
void gettimeofday(struct timeval *tp, void *tzp)
{
    tp->tv_sec = time(NULL);
    tp->tv_usec = 0;
}
#endif

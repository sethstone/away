/* Away - terminal locking program
 * Copyright (C) 1999-2000 Cameron Moore
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * Or try here: http://www.fsf.org/copyleft/gpl.html
 */

#ifndef _AWAY_H
#define _AWAY_H

/* ./configure help */
#include "../config.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>

#ifdef    STDC_HEADERS
# include <stdlib.h>
# include <string.h>
#else
# ifndef  HAVE_STRDUP
#  define strdup(x) calc_strdup((CONST char *)(x))
# endif //HAVE_STRDUP
# ifndef  HAVE_STRSPN
//#  define strspn(x) ????
# endif
#endif  //STDC_HEADERS

#ifdef    HAVE_UNISTD_H
# include <unistd.h>
#endif  //HAVE_UNISTD_H

#ifdef    TM_IN_SYS_TIME
# include <sys/time.h>
#else
# include <time.h>
#endif  //TIM_IN_SYS_TIME

#ifdef    HAVE_PATHS_H
# include <paths.h>
#endif  //HAVE_PATHS_H

#include <pwd.h>
#include <ctype.h>
#include <signal.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/stat.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>


/*
 * DEFINES
 */

/* contacts */
#define CONTACT "cameron@unbeatenpath.net"
#define MLIST   "away@unbeatenpath.net"

/* Separator for conf file */
#define WHITESPACE " \t\n"

/* macro */
#define APPEND_MAILBOX(root,path,desc) root=snocString((root),(path),(desc))

/* Max filename length */
#define FILE_NAME_LEN 1024

/* ENV variable names */
#define AWAY_RCFILE       "AWAY_RCFILE"
#define AWAY_NORCFILE     "AWAY_NORCFILE"
#define AWAY_TIME         "AWAY_TIME"
#define AWAY_NOTIME       "AWAY_NOTIME"
#define AWAY_MAIL         "AWAY_MAIL"
#define AWAY_PERSIST      "AWAY_PERSIST"

/* Default conf file name */
char *rcfile = ".awayrc";

/* min mail check interval */
const int MIN_TIME = 10;

/* Default settings */
int TIME = 300;
int PERSIST = 1;
int CHECK_MAIL = 1;

/* flags to allow command line options to override conf file options */
int TIME_OP = 0;
int PERSIST_OP = 0;
int RCFILE_OP = 0;
int MAIL_OP = 0;
int NORCFILE_OP = 0;

/* turned on while trying to authenticate the user */
int pam_active = 0;
/* mail was found */
int mail_found = 0;
/* the user has already been notified */
int notified  = 0;
/* name of mailbox new mail was found in */
char *found_in = NULL;
/* saving argv[0] for stderr reporting */
char *argv0;

/* PAM conversation struct */
static struct pam_conv conv = { misc_conv, NULL };

/* rc file commands */
typedef enum {
  oMail,
  oMaildir,
  oMailbox,
  oPersist,
  oTime,
  oBadCmd
} CmdCodes;
static struct {
  const char *name;
  CmdCodes opcode;
} commands[] = {
  { "mail", oMail },
  { "maildir", oMaildir },
  { "mailbox", oMailbox },
  { "persist", oPersist },
  { "time", oTime },
  { NULL, 0 }
};

/* mailbox linked list */
typedef struct mailbox {
  char *path;             /* path to mailbox */
  char *desc;             /* description of mailbox */
  time_t mtime;           /* last modified time (new mail) */
  time_t atime;           /* last accessed time (read mail) */
  struct mailbox *next;
} Mailbox;

/* command line options for getopt */
static struct option long_options[] =
{
  {"help", 0, 0, 'h'},
  {"mail", 0, 0, 'c'},
  {"message", 0, 0, 'm'},
  {"nomail", 0, 0, 'C'},
  {"nopersist", 0, 0, 'P'},
  {"norcfile", 0, 0, 'F'},
  {"notime", 0, 0, 'T'},
  {"persist", 0, 0, 'p'},
  {"rcfile", 1, 0, 'f'},
  {"time", 1, 0, 't'},
  {"version", 0, 0, 'v'},
  {0, 0, 0, 0}
};

/* prototypes */
void master(void);
int authenticate(char *);
int authenticate_root(void);
char *make_path(char *, char *);
void print_version(void);
void short_help(void);
void ext_help(void);
void stall(void);
char *make_time(void);
void salutations(void);
int new_mail(Mailbox *);
void mail_thread_f(Mailbox **);
static CmdCodes get_opcode(const char *, const char *, int);
static void *my_malloc(int);
static Mailbox *make_cell(void);
static Mailbox *snocString(Mailbox *, char *, char *);
static void add_mailbox(Mailbox **, char *, char *);
void set_defaults(Mailbox **, char *);
void read_config(Mailbox **, char *, char *);
void re_exec(int, char *[], int, int);
void check_env(void);
void free_mailboxes(Mailbox *);

#endif   /* _AWAY_H */

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

/* Away version number */
#define VERSION "0.9.1"

/* Separator for conf file */
#define WHITESPACE " \t\n"

/* macro */
#define APPEND_MAILBOX(root,path,desc) root=snocString((root),(path),(desc))

/* Max filename length */
#define FILE_NAME_LEN 1024

/* ENV variable names */
#define AWAY_CONF_FILE    "AWAY_CONF_FILE"
#define AWAY_WAIT_SECS    "AWAY_WAIT_SECS"
#define AWAY_NO_WAIT      "AWAY_NO_WAIT"
#define AWAY_PERSIST      "AWAY_PERSIST"

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <paths.h>
#include <time.h>
#include <pwd.h>
#include <ctype.h>
#include <signal.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/stat.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>

/* Default conf file name */
char *conf_file = ".awayrc";

/* min mail check interval */
const int MIN_WAIT_SECS = 10;

/* ... */
int WAIT_SECS = 300;
int PERSIST = 1;

/* flags to allow command line options to override conf file options */
short WAIT_OP = 0;
short PERSIST_OP = 0;
short CONF_OP = 0;

/* global variables */
char *foundIn = NULL, *awayTime = 0;
short pamActive = 0, mailFound = 0, notified  = 0;

/* PAM conversation struct */
static struct pam_conv conv = { misc_conv, NULL };

/* rc file commands */
typedef enum {
  oMaildir,
  oMailbox,
  oPersist,
  oWait,
  oBadCmd
} CmdCodes;
static struct {
  const char *name;
  CmdCodes opcode;
} commands[] = {
  { "maildir", oMaildir },
  { "mailbox", oMailbox },
  { "persist", oPersist },
  { "wait", oWait },
  { NULL, 0 }
};

/* mailbox linked list */
typedef struct mailbox {
  char *path;             /* path to mailbox */
  char *desc;             /* description of mailbox */
  struct mailbox *next;
} Mailbox;

/* command line options for getopt */
static struct option long_options[] =
{
  {"conf", 1, 0, 'c'},
  {"message", 0, 0, 'm'},
  {"help", 0, 0, 'h'},
  {"nopersist", 0, 0, 'P'},
  {"nowait", 0, 0, 'W'},
  {"persist", 0, 0, 'p'},
  {"wait", 1, 0, 'w'},
  {"version", 0, 0, 'v'},
  {0, 0, 0, 0}
};

/* prototypes */
void master(void);
short authenticate(char *username);
char *make_path(char *dirs, char *filename);
void print_version(void);
void short_help(char *argv0);
void ext_help(char *argv0);
void stall(void);
char *make_time(void);
void salutations(void);
short new_mail(char *path);
void mail_thread_f(Mailbox **root);
static CmdCodes get_opcode(const char *cp, const char *filename, int linenum);
static void *my_malloc(int n);
static Mailbox *make_cell(void);
static Mailbox *snocString(Mailbox *root, char *path, char *desc);
static void add_mailbox(Mailbox **list, char *path, char *desc);
void set_defaults(Mailbox **root, char *name);
void read_config(Mailbox **root, char *homedir, char *username);
void re_exec(int argc, char *argv[], int opt_cnt, short as_mesg);
void check_env(void);

#endif   /* _AWAY_H */

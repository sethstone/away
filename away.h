/* Away - Let other users know that you are away from your terminal
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

#ifndef _AWAY_H_
#define _AWAY_H_

#define AWAY_CONF_FILE ".awayrc"
#define WHITESPACE " \t\n"
#define APPEND_MAILBOX(root,path,desc) root=snocString((root),(path),(desc))
#define FILE_NAME_LEN 1024

const char *AWAY_VERSION = "0.9";

/* number of secs to wait between checking mailboxes for new mail */
const int MIN_WAIT_SECS = 30;
int WAIT_SECS = 300;

/* global variables */
char *foundIn = NULL, *awayTime = 0;
short pamActive = 0, mailFound = 0, notified  = 0;

/* PAM conversation struct */
static struct pam_conv conv = {
  misc_conv, NULL
};

/* options */
typedef enum { oMaildir, oMailbox, oWait, oBadOp } OpCodes;
static struct {
  const char *name;
  OpCodes opcode;
} options[] = {
  { "maildir", oMaildir },
  { "mailbox", oMailbox },
  { "wait", oWait },
  { NULL, 0 }
};

/* mailbox linked list */
typedef struct mailbox {
  char *path;             /* path to mailbox */
  char *desc;             /* description of mailbox */
  struct mailbox *next;
} Mailbox;

/* * * * * * * */

/* Authenticate Password */
short authenticate(char *username) {
  short error;
  pam_handle_t *pamh = NULL;

  /* start PAM session for 'away'.          *
   * the session name is what PAM looks for *
   * in /etc/pam.d/ or /etc/pam.config      */
  error = pam_start("away", username, &conv, &pamh);

  /* authenticate user */
  if (!error) error = pam_authenticate(pamh, 0);

  /* close PAM session */
  if (pam_end(pamh,error) != PAM_SUCCESS) {
    pamh = NULL;
    fprintf(stderr,"away: ** failed to release PAM authenticator\n");
    error = 1;
  }
  return error;
}

/* Make a Path */
char *make_path(char *dirs, char *filename) {
  unsigned int size = 8;
  char *buffer = (char *) malloc (size),
    *last = strrchr(dirs, '/'), *format = NULL;

  if (last != 0 && strlen(last) == 1) format = "%s%s";
  else format = "%s/%s";

  while (1) {
    /* try to print in the allocated space. */
    int nchars = snprintf (buffer, size, format, dirs, filename);
    /* if that worked, return the string. */
    if (nchars < size) return buffer;
    /* else try again with twice as much space. */
    size *= 2;
    buffer = (char *) realloc (buffer,size);
  }
}

/* Short Help */
void short_help() { fprintf(stderr,"Usage: away [--help] [-m] <REASON>\n"); }

/* Extended Help */
void ext_help() {
  printf("Away v%.20s (c) Cameron Moore <cameron@unbeatenpath.net>\n", AWAY_VERSION);
  printf("Usage: away [--help] [-m] <REASON>\n\n");
  printf("   ex: away playing games\\!\\!\n");
  printf("       away -m Zzz...  :\\)\n\n");
  printf("   Use -m to have away appear to execute as the first word\n");
  printf("   in <REASON>.\n\n");
  printf("   If you plan on using special characters, make sure you\n");
  printf("   escape each character like the examples above.\n\n");
  printf("   Compiled to check mail every %d seconds.\n\n", WAIT_SECS);
}

/* Stall */
void stall() {
  printf("\n       You went away at %.20s",awayTime);
  printf("\n\n -- Press [Enter] to come back online --\n");
  getchar();
}

/* Get Current Time */
char *make_time() {
  unsigned int size = 8;
  char *buffer = (char *) malloc (size);
  time_t t = time(NULL);
  struct tm *thetime = localtime(&t);

  while (1) {
    /* try to print in the allocated space. */
    int nchars = snprintf (buffer, size, "%.02d:%.02d:%.02d",
                           thetime->tm_hour, thetime->tm_min, thetime->tm_sec);
    /* if that worked, return the string. */
    if (nchars < size) return buffer;
    /* else try again with twice as much space. */
    size *= 2;
    buffer = (char *) realloc (buffer,size);
  }
}

/* Salutations */
void salutations() {
  printf("\n     Welcome back. It's %.20s\n\n", make_time());
}

/* Check for New Mail */
short new_mail(char *path) {
  struct stat status;
  time_t mailread = 0, mailrecv = 0;

  if (stat(path, &status) < 0) {
    printf("Could not get status of \"%.200s\"\n", path);
    /* if we can't get a status on the mail file, *
     * then our thread is useless.  so kill it.   */
    mailFound = 0;
    pthread_exit(NULL);
  }
  else if (status.st_size != 0) {
    mailrecv = status.st_mtime;
    mailread = status.st_atime;
  }

  if (mailread < mailrecv)
    return mailFound = 1;
  else
    return mailFound = 0;
}

/* Mail Thread Function */
void mail_thread_f(Mailbox *root) {
  Mailbox *mb = NULL;
  short found_mail = 0;

  while (!found_mail) {
    for (mb = root; !found_mail && mb != NULL; mb = mb->next) {
      if (new_mail(mb->path)) {
        found_mail = 1;
        foundIn = mb->desc;
      }
    }
    if (!found_mail) sleep(WAIT_SECS);
  }
  /* sleep for 1 second to make sure the *
   * main process outputs its stuff...   */
  sleep(1);

  /* don't interrupt user authentication */
  if (!pamActive) {
    printf("\a\n       You have new mail in %.200s.\n", foundIn);
    notified = 1;
  }
  /* exit the thread */
  pthread_exit(NULL);
}

/* Get OpCode */
static OpCodes get_opcode(const char *cp, const char *filename, int linenum) {
  unsigned int i;
  for (i = 0; options[i].name; i++)
    if (strcasecmp(cp, options[i].name) == 0)
      return options[i].opcode;
  fprintf(stderr, "%.200s: line %d: Bad configuration option: %.200s\n",
    filename, linenum, cp);
  return oBadOp;
}

/* Allocate New Link Node */
static void *my_malloc(int n) {
  void *p = malloc( (size_t)n );
  if (p == NULL) {
    fprintf(stderr, "Out of Memory");
    exit(1);
  } return p;
}

/* Setup New Mailbox Container */
static Mailbox *make_cell(void) {
  Mailbox *m = (Mailbox *) my_malloc(sizeof(Mailbox));
  m->path = m->desc = NULL;
  m->next = NULL;
  return m;
}

/* Find New Node and Set String Values */
static Mailbox *snocString(Mailbox *root, char *path, char *desc) {
  if (root == NULL) {
    Mailbox *tmp = make_cell();
    tmp->path = (char *) my_malloc(5 + strlen(path));
    tmp->desc = (char *) my_malloc(5 + strlen(desc));
    strcpy(tmp->path, path);
    strcpy(tmp->desc, desc);
    return tmp;
  } else {
    Mailbox *tmp = root;
    while (tmp->next != NULL) tmp = tmp->next;
    tmp->next = snocString(tmp->next, path, desc);
    return root;
  }
}

/* Add New Mailbox to List */
static void add_mailbox(Mailbox **list, char *path, char *desc) {
  if (path != NULL) {
    int i, j, k, gotPath = 0;
    char tmpPath[FILE_NAME_LEN], tmpDesc[FILE_NAME_LEN];

    i = 0;
    while (1) {
      if (path[i] == 0) break;
      path += i;
      i = 0;
      while (isspace((int)(path[0]))) path++;
      while (path[i] != 0 && !isspace((int)(path[i]))) i++;
      if (i > 0) {
        k = i; if (k > FILE_NAME_LEN-10) k = FILE_NAME_LEN-10;
        for (j = 0; j < k; j++) tmpPath[j] = path[j];
        tmpPath[k] = 0;
        gotPath = 1;
      }
    }
    i = 0;
    while (gotPath) {
      if (desc[i] == 0) break;
      desc += i;
      i = 0;
      while (isspace((int)(desc[0]))) desc++;
      while (desc[i] != 0) i++;
      if (i > 0) {
        k = i; if (k > FILE_NAME_LEN-10) k = FILE_NAME_LEN-10;
        for (j = 0; j < k; j++) tmpDesc[j] = desc[j];
        tmpDesc[k] = 0;
        APPEND_MAILBOX(*list, tmpPath, tmpDesc);
      }
    }
  }
}

/* Set Defaults */
void set_defaults(Mailbox **root, char *name) {
  add_mailbox(root, make_path(_PATH_MAILDIR, name), "INBOX");
}

/* Read Configuration File */
void read_config(Mailbox **root, char *homedir, char *username) {
  FILE *f = NULL;
  char line[1024], filename[256];
  char *cp = NULL, *maildir = "";
  unsigned int linenum = 0, opcode;
  short changed_wait_secs = 0;

  snprintf(filename, sizeof filename, "%.100s/%.100s", homedir, AWAY_CONF_FILE);
  /* open rc file */
  f = fopen(filename, "r");
  if (!f) {
    /* if there is no rc file, assume they have a /var/spool/mail file */
    set_defaults(root, username);
    return;
  }

  /* go line by line */
  while (fgets(line, sizeof(line), f)) {
    linenum++;
    /* skip whitespace */
     cp = line + strspn(line, WHITESPACE);
    if (!*cp || *cp == '\n' || *cp == '#');
    else {
      /* get opcode */
      cp = strtok(cp, WHITESPACE);
      opcode = get_opcode(cp, filename, linenum);

      switch (opcode) {
      case oMaildir:
        /* save the maildir for building paths to bailboxes */
        cp = strtok(NULL, WHITESPACE);
        if (!cp)
          fprintf(stderr,"%.200s line %d: missing argument.\n",
                  filename, linenum);
        else if (strcmp(cp,maildir) != 0) 
          if (cp[0] == '~') {
            /* handle references to home dir as ~, ~foo, etc. */
            char *save = NULL;
            cp = strtok_r(cp,"~",&save);
            if (strstr(cp, "/") != NULL)
              maildir = strtok_r(cp, "/",&save);
            else maildir = cp;
            if (maildir != NULL && strcmp(maildir,username) == 0)
              cp = strcat(cp=strdup("/"), save);
            if (cp == NULL) cp = homedir;
            else cp = strcat(homedir, cp);
          }
          maildir = make_path(cp,"");
        break;

      case oMailbox:
        /* append mailbox to maildir and save */
        cp = strtok(NULL, WHITESPACE);
        if (!cp)
          fprintf(stderr,"%.200s line %d: missing argument.\n",
                  filename, linenum);
        else if (strcmp(maildir,"") != 0) {
          char *desc = strtok(NULL, "\n");
          if (!desc) desc = strdup(cp);
          else if (strstr(desc,"{") && strstr(desc,"}")) desc = strtok(desc,"{}");
          else {
            fprintf(stderr,"%.200s line %d: garbage at end of line.\n", filename, linenum);
            desc = strdup(cp);
          }
          add_mailbox(root, make_path(maildir,cp), desc);
        } else
          fprintf(stderr,"%.200s line %d: Mailbox option needs a Maildir parent.\n",
          filename, linenum);
        break;

      case oWait:
        /* change WAIT_SECS */
        if (changed_wait_secs) {
          cp = strtok(NULL, WHITESPACE);
          fprintf(stderr,"%.200s line %d: multiple Wait options.\n",
                  filename, linenum);
        } else {
          cp = strtok(NULL, WHITESPACE);
          if (!cp)
            fprintf(stderr,"%.200s line %d: missing argument.\n",
                    filename, linenum);
          else if (atoi(cp) >= MIN_WAIT_SECS) WAIT_SECS = atoi(cp);
          else {
            fprintf(stderr,"%.200s line %d: Wait value less than minimum (%d).\n",
                    filename, linenum, MIN_WAIT_SECS);
            WAIT_SECS = MIN_WAIT_SECS;
          }
          changed_wait_secs = 1;
        }
        break;

      default:
        break;
      } /* switch */

      /* check for garbage at EOL */
      if (strtok(NULL, WHITESPACE) != NULL)
        fprintf(stderr,"%.200s line %d: garbage at end of line.\n", filename, linenum);
    } /* else */
  } /* while */
  fclose(f);
}

/* Exec as Message */
void exec_as_mesg(int argc, char *argv[])
{
  /* preserve location of binary */
  char *original = argv[0]; 
  /* create new argv */
  char *newargv[argc];

  int i=0;
  for (; i<=argc-2; i++){ newargv[i] = argv[i+2]; }

  /* pad with space to make use of one word arguments to -m */
  newargv[argc-2] = " ";
  newargv[argc-1] = NULL;

  /* exec THIS! */
  execvp(original, newargv);
} /* execAsMessage */

#endif   /* _AWAY_H_ */

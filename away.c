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

#include "away.h"

int main(int argc, char **argv) {
  int option_count = 0;
  int option_index = 0;
  int c = 0;
  short mesg_exec = 0;
  short restart = 0;

  /*
  signal(SIGINT , SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);
  */
 
  if (argc == 1) { short_help(argv[0]); }
  while ((c = getopt_long(argc, argv, "c:mhpPw:Wv", long_options, &option_index)) && c != -1) {
    restart = 1;
    switch (c) {
      case 'c':
        setenv(AWAY_CONF_FILE, (char *)strdup(optarg), 1);
        option_count += strchr(argv[option_count],'=') ? 1 : 2;
        break;
      case 'm':
        mesg_exec = 1;
        option_count++;
        break;
      case 'h':
        ext_help(argv[0]);
        break;
      case 'p':
        setenv(AWAY_PERSIST, "1", 1);
        option_count++;
        break;
      case 'P':
        setenv(AWAY_PERSIST, "0", 1);
        option_count++;
        break;
      case 'w':
        if (atoi(optarg) >= MIN_WAIT_SECS)
          setenv(AWAY_WAIT_SECS, (char *)strdup(optarg), 1);
        option_count += strchr(argv[option_count],'=') ? 1 : 2;
        break;
      case 'W':
        setenv(AWAY_NO_WAIT, "1", 1);
        option_count++;
        break;
      case 'v':
        print_version();
        break;
      default:
        break;
    }
  }

  /* if passed an option, we need to re-exec */
  if (restart) {
    /* make sure we have enough args for a message */
    if (option_count+1 == argc)
      short_help(argv[0]);
    else
      re_exec(argc, argv, option_count, mesg_exec);
  }

  check_env();
  master();
  return 0;
}

/* Master function */
void master(void) {
  pthread_t mail_thread;
  short error = 1;
  struct passwd *pw = NULL;
  Mailbox *mailboxRoot = NULL, *mb = NULL;

  /* get home dir */
  pw = getpwuid(getuid());
  if (!pw) { fprintf(stderr, "You don't exist!\n"); exit(1); }
  read_config(&mailboxRoot, pw->pw_dir, pw->pw_name);

  /* build time string */
  awayTime = make_time();
  /* start mail checking thread */
  pthread_create(&mail_thread, NULL, (void*)&mail_thread_f, &mailboxRoot);

  /* lock this sucka up */
  while (error) {
    stall();
    /* authenticate the user */
    pamActive = 1;
    error = authenticate(pw->pw_name);
    pamActive = 0;
    /* on invalid passwd */
    if (error) {
      printf("\a** Invalid password **\n");
      /* mail was found during authentication... */
      if ((!notified) && (mailFound)) {
        printf("\a\n       You have new mail in %.200s.\n", foundIn);
        notified = 1;
      }
    }
  } /* while */

  /* mail was found during authentication... */
  if ((!notified) && (mailFound))
    printf("\a\n       You have new mail in %.200s.\n", foundIn);
  salutations();
  /* clean up LL */
  mb = mailboxRoot;
  while (mb != NULL) {
    Mailbox *next = mb->next;
    if (mb->path != NULL) free(mb->path);
    free(mb);
    mb = next;
  };
}

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

/* Version */
void print_version(void) {
  fprintf(stderr, "away v"
    VERSION " (c) Cameron Moore <cameron@unbeatenpath.net>\n");
  exit (0);
}

/* Short Help */
void short_help(char *argv0) {
  fprintf(stderr,"Usage: %s [OPTIONS] message [...]\n", argv0);
  exit (0);
}

/* Extended Help */
void ext_help(char *argv0) {
  printf("Usage: %s [OPTIONS] message [...]\n", argv0);
  printf("A terminal locking program.\n\n");
  printf("  -c, --conf=FILE         specifies an alternative configuration file\n");
  printf("  -h, --help              display this help information\n");
  printf("  -m, --message           execute showing only the message passed\n");
  printf("                          to the program\n");
  printf("  -p, --persist           continue checking for new mail as long as\n");
  printf("                          there is at least one mailbox that has not\n");
  printf("                          recieved new mail\n");
  printf("  -P, --nopersist         stop checking mail if any mailbox is found\n");
  printf("                          to have new mail\n");
  printf("  -w, --wait=SECS         sets how long the program waits between\n");
  printf("                          mail checks\n");
  printf("  -W, --nowait            use the default wait time of %d seconds\n",
         WAIT_SECS);
  printf("  -v, --version           display version information\n");
  printf("\naway v" VERSION " (c) Cameron Moore <cameron@unbeatenpath.net>\n");
  exit (0);
}

/* Stall */
void stall(void) {
  printf("\n       You went away at %.20s",awayTime);
  printf("\n\n -- Press [Enter] to come back online --\n");
  getchar();
}

/* Get Current Time */
char *make_time(void) {
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
void salutations(void) {
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
void mail_thread_f(Mailbox **root) {
  Mailbox *mb = NULL;
  Mailbox *last = NULL;
  short slept = 0;

  while (1) {
    mb = *root;

    while ((!mailFound || PERSIST) && mb != NULL) {
      short deleted_root = 0;
      short incr = 1;

      if (new_mail(mb->path)) {
        foundIn = mb->desc;

        /* make sure the main process has *
         * time to outputs its stuff...   */
        if (!slept) { sleep(1); slept++; }
        if (!pamActive) {
          printf("\a\n       You have new mail in %.200s.\n", foundIn);
          notified = 1;
        }

        if (!PERSIST) break;
        else {
          /* delete node */
          Mailbox *tmp = mb;
          incr = 0;
          /* reset place keepers */
          notified = mailFound = 0;

          if (mb == *root) {
            deleted_root = 1;
            *root = mb = mb->next;
          } else { last->next = mb = mb->next; }

          /* free */
          if (tmp->path != NULL) free(tmp->path);
          free(tmp);
        }
      } else last = mb;

      /* don't go to the next mb if we deleted one */
      if (incr) mb = mb->next;
    } /* while */

    if (mailFound && !PERSIST) break;
    last = NULL;
    sleep(WAIT_SECS);
  }

  /* exit the thread */
  pthread_exit(NULL);
}

/* Get CmdCode */
static CmdCodes get_opcode(const char *cp, const char *filename, int linenum) {
  unsigned int i;
  for (i = 0; commands[i].name; i++)
    if (strcasecmp(cp, commands[i].name) == 0)
      return commands[i].opcode;
  fprintf(stderr, "%.200s: line %d: Bad configuration option: %.200s\n",
    filename, linenum, cp);
  return oBadCmd;
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
  add_mailbox(root, make_path(_PATH_MAILDIR, name), "INBOX"); }

/* Read Configuration File */
void read_config(Mailbox **root, char *homedir, char *username) {
  FILE *f = NULL;
  char line[1024], filename[256];
  char *cp = NULL, *maildir = "";
  unsigned int linenum = 0, opcode;
  short changed_wait_secs = 0;
  short changed_persist = 0;

  if (CONF_OP)
    snprintf(filename, sizeof filename, "%.200s", conf_file);
  else
    snprintf(filename, sizeof filename, "%.100s/%.100s", homedir,conf_file);

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
          else if (strstr(desc,"{") && strstr(desc,"}"))
            desc = strtok(desc,"{}");
          else {
            fprintf(stderr,"%.200s line %d: garbage at end of line.\n",
                    filename, linenum);
            desc = strdup(cp);
          }
          add_mailbox(root, make_path(maildir,cp), desc);
        } else {
          fprintf(stderr,
                  "%.200s line %d: Mailbox option needs a Maildir parent.\n",
                  filename, linenum);
        }
        break;

      case oPersist:
        /* if changed from command line */
        if (PERSIST_OP) {
          cp = strtok(NULL, WHITESPACE);
          break;
        }
        if (changed_persist) {
          cp = strtok(NULL, WHITESPACE);
          fprintf(stderr,"%.200s line %d: multiple Persist commands.\n",
                  filename, linenum);
        } else {
          cp = strtok(NULL, WHITESPACE);
          if (!cp)
            fprintf(stderr,"%.200s line %d: missing argument.\n",
                    filename, linenum);
          else if (strcasecmp(cp,"yes") == 0 || atoi(cp) == 1)
            PERSIST = 1;
          else if (strcasecmp(cp,"no") == 0 || atoi(cp) == 0)
            PERSIST = 0;
          else {
            fprintf(stderr,
                    "%.200s line %d: Invalid Persist argument (%.200s).\n",
                    filename, linenum, cp);
          }
          changed_persist = 1;
        }
        break;

      case oWait:
        /* if changed from command line */
        if (WAIT_OP) {
          cp = strtok(NULL, WHITESPACE);
          break;
        }
        /* change WAIT_SECS */
        if (changed_wait_secs) {
          cp = strtok(NULL, WHITESPACE);
          fprintf(stderr,"%.200s line %d: multiple Wait commands.\n",
                  filename, linenum);
        } else {
          cp = strtok(NULL, WHITESPACE);
          if (!cp)
            fprintf(stderr,"%.200s line %d: missing argument.\n",
                    filename, linenum);
          else if (atoi(cp) >= MIN_WAIT_SECS) WAIT_SECS = atoi(cp);
          else {
            fprintf(stderr,
                    "%.200s line %d: Wait value less than minimum (%d).\n",
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
        fprintf(stderr,"%.200s line %d: garbage at end of line.\n",
                filename, linenum);
    } /* else */
  } /* while */
  fclose(f);
}

/* Re-execute */
void re_exec(int argc, char *argv[], int opt_cnt, short as_mesg) {
  /* preserve location of binary */
  char *original = argv[0]; 
  /* create new argv */
  char *newargv[argc];

  int i = (as_mesg ? 0 : 1);
  for (; i < argc-(opt_cnt+as_mesg); i++)
    newargv[i] = argv[i+(opt_cnt+as_mesg)];

  /* keep original exec name */
  if (!as_mesg) { newargv[0] = argv[0]; }

  /* pad with space to make use of one word arguments to -m */
  newargv[argc-(opt_cnt+as_mesg)] = " ";
  i = opt_cnt;// - (as_mesg ? 1 : 0);
  for(; i > 0; i--)
    newargv[argc-i] = NULL;

  /* exec THIS! */
  execvp(original, newargv);
}

/* check ENV for options */
void check_env(void) {
  if (getenv(AWAY_CONF_FILE)) {
    conf_file = getenv(AWAY_CONF_FILE);
    CONF_OP = 1;
    unsetenv(AWAY_CONF_FILE);
  }

  if (getenv(AWAY_NO_WAIT)) {
    WAIT_OP = 1;
    unsetenv(AWAY_NO_WAIT);
  } else if (getenv(AWAY_WAIT_SECS)) {
    if (atoi(getenv(AWAY_WAIT_SECS)) >= MIN_WAIT_SECS) {
      WAIT_SECS = atoi(getenv(AWAY_WAIT_SECS));
      WAIT_OP = 1;
    }
    unsetenv(AWAY_WAIT_SECS);
  }

  if (getenv(AWAY_PERSIST)) {
    PERSIST = atoi(getenv(AWAY_PERSIST));
    PERSIST_OP = 1;
    unsetenv(AWAY_PERSIST);
  }
}


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

#include "away.h"

int main(argc, argv)
  int argc;
  char **argv;
{
  int option_count = 0;
  int option_index = 0;
  int c = 0;
  int mesg_exec = 0;
  int restart = 0;

  signal(SIGINT , SIG_IGN);
  signal(SIGHUP , SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGUSR1, SIG_IGN);
  signal(SIGUSR2, SIG_IGN);
 
  if (argc == 1) { short_help(argv[0]); }
  while ((c = getopt_long(argc, argv, "cCf:FhmpPt:Tv", long_options, &option_index)) && c != -1) {
    restart = 1;
    switch (c) {
      case 'c':
        setenv(AWAY_MAIL, "1", 1);
        option_count++;
        break;
      case 'C':
        setenv(AWAY_MAIL, "0", 1);
        option_count++;
        break;
      case 'f':
        setenv(AWAY_RCFILE, (char *)strdup(optarg), 1);
        option_count += strchr(argv[option_count],'=') ? 1 : 2;
        break;
      case 'F':
        setenv(AWAY_NORCFILE, "1", 1);
        option_count++;
        break;
      case 'h':
        ext_help(argv[0]);
        break;
      case 'm':
        mesg_exec = 1;
        option_count++;
        break;
      case 'p':
        setenv(AWAY_PERSIST, "1", 1);
        option_count++;
        break;
      case 'P':
        setenv(AWAY_PERSIST, "0", 1);
        option_count++;
        break;
      case 't':
        if (atoi(optarg) >= MIN_TIME)
          setenv(AWAY_TIME, (char *)strdup(optarg), 1);
        option_count += strchr(argv[option_count],'=') ? 1 : 2;
        break;
      case 'T':
        setenv(AWAY_NOTIME, "1", 1);
        option_count++;
        break;
      case 'v':
        print_version();
        break;
      case '?':
        fprintf(stderr, "Try `%s --help` for more information.\n", argv[0]);
	exit(0);
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
void master(void)
{
  pthread_t mail_thread;
  int error = 1;
  struct passwd *pw = NULL;
  Mailbox *mailboxRoot = NULL;

  /* get home dir */
  pw = getpwuid(getuid());
  if (!pw) { fprintf(stderr, "You don't exist!\n"); exit(1); }
  read_config(&mailboxRoot, pw->pw_dir, pw->pw_name);

  /* start mail checking thread */
  if (CHECK_MAIL != 0)
    pthread_create(&mail_thread, NULL, (void*)&mail_thread_f, &mailboxRoot);

  /* lock this sucka up */
  while (error) {
    stall();
    /* authenticate the user */
    pam_active = 1;
    error = authenticate(pw->pw_name);
    pam_active = 0;
    /* on invalid passwd */
    if (error) {
      printf("\a** Invalid password **\n");
      /* mail was found during authentication... */
      if ((!notified) && (mail_found)) {
        printf("\a\n       You have new mail in %.200s.\n", found_in);
        notified = 1;
      }
    }
  } /* while */

  /* mail was found during authentication... */
  if ((!notified) && (mail_found))
    printf("\a\n       You have new mail in %.200s.\n", found_in);
  salutations();
  free_mailboxes(mailboxRoot);
}

/* Authenticate Password */
int authenticate(username)
  char *username;
{
  int error;
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
char *make_path(dirs, filename)
  char *dirs, *filename;
{
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
void print_version(void)
{
  printf(PACKAGE " " VERSION "\n");
  printf("Copyright (c) 1999, 2000 Cameron Moore <" CONTACT ">\n");
  printf("This software comes with NO WARRANTY, to the extent permitted by law.\n");
  printf("You may redistribute copies of this software under the terms of the\n");
  printf("GNU General Public License.  For more information about these matters,\n");
  printf("see the file named COPYING distributed with this software.\n\n");
  printf("To contact the developers, please send mail to <" MLIST ">.\n");
  exit (0);
}

/* Short Help */
void short_help(argv0)
  char *argv0;
{
  fprintf(stderr,"Usage: %s [OPTIONS] message [...]\n", argv0);
  exit (0);
}

/* Extended Help */
void ext_help(argv0)
  char *argv0;
{
  printf("Usage: %s [OPTIONS] message [...]\n", argv0);
  printf("A terminal locking program.\n\n");
  printf("  -c, --mail              enable checking of mail\n");
  printf("  -C, --nomail            disable checking of mail\n");
  printf("  -f, --rcfile=FILE       specifies an alternative configuration file\n");
  printf("  -F, --norcfile          ignore user configuration file\n");
  printf("  -h, --help              display this help information\n");
  printf("  -m, --message           execute showing only the message passed\n");
  printf("                          to the program\n");
  printf("  -p, --persist           continue checking for new mail as long as\n");
  printf("                          there is at least one mailbox that has not\n");
  printf("                          recieved new mail\n");
  printf("  -P, --nopersist         stop checking mail if any mailbox is found\n");
  printf("                          to have new mail\n");
  printf("  -t, --time=SECONDS      sets the time interval by which the program\n");
  printf("                          performs its background tasks\n");
  printf("  -T, --notime            ignore any options to set the time interval\n");
  printf("                          and use the default of %d seconds\n",
         TIME);
  printf("  -v, --version           display version copyright information\n\n");
  printf("Report bugs to " MLIST ".\n");
  exit (0);
}

/* Stall */
void stall(void)
{
  printf("\n       You went away at %.20s", make_time());
  printf("\n\n -- Press [Enter] to come back online --\n");
  getchar();
}

/* Get Current Time */
char *make_time(void)
{
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
void salutations(void)
{
  printf("\n     Welcome back. It's %.20s\n\n", make_time());
}

/* Check for New Mail */
int new_mail(mb)
  Mailbox *mb;
{
  struct stat status;
  time_t mailread = 0, mailrecv = 0;

  if (stat(mb->path, &status) < 0) {
    /* couldn't stat */
    if (access(mb->path, F_OK) == -1)
      fprintf(stderr, "Could not open %s: file does not exist\n", mb->path);
    else if (access(mb->path, R_OK) == -1)
      fprintf(stderr, "Could not open %s: access denied\n", mb->path);
    else
      fprintf(stderr, "Could not open %s\n", mb->path);
    mail_found = 0;
  } else if (status.st_size != 0) {
    mailrecv = status.st_mtime;
    mailread = status.st_atime;
  }

  if ((mailread < mailrecv) && (mb->mtime != mailrecv)) {
    mb->mtime = mailrecv;
    return mail_found = 1;
  } else {
    return mail_found = 0;
  }
}

/* Mail Thread Function */
void mail_thread_f(root)
  Mailbox **root;
{
  Mailbox *mb = *root;
  int slept = 0;

  while (1) {
    while (!mail_found || PERSIST) {
      if (new_mail(mb)) {
        found_in = mb->desc;
        /* make sure the main process has *
         * time to outputs its stuff...   */
        if (!slept) { sleep(1); slept++; }
        if (!pam_active) {
          printf("\a\n       You have new mail in %.200s.\n", found_in);
          notified = 1;
        }
      }
    } /* while */
    if (mail_found && !PERSIST) break;
    sleep(TIME);
  }

  /* exit the thread */
  pthread_exit(NULL);
}

/* Get CmdCode */
static CmdCodes get_opcode(cp, filename, linenum)
  const char *cp, *filename;
  int linenum;
{
  unsigned int i;
  for (i = 0; commands[i].name; i++)
    if (strcasecmp(cp, commands[i].name) == 0)
      return commands[i].opcode;
  fprintf(stderr, "%.200s: line %d: Bad configuration option: %.200s\n",
    filename, linenum, cp);
  return oBadCmd;
}

/* Allocate New Link Node */
static void *my_malloc(n)
  int n;
{
  void *p = malloc( (size_t)n );
  if (p == NULL) {
    fprintf(stderr, "Out of Memory");
    exit(1);
  } return p;
}

/* Setup New Mailbox Container */
static Mailbox *make_cell(void)
{
  Mailbox *m = (Mailbox *) my_malloc(sizeof(Mailbox));
  m->path = m->desc = NULL;
  m->next = NULL;
  return m;
}

/* Find New Node and Set String Values */
static Mailbox *snocString(root, path, desc)
  Mailbox *root;
  char *path, *desc;
{
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
static void add_mailbox(list, path, desc)
  Mailbox **list;
  char *path, *desc;
{
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
void set_defaults(root, name)
  Mailbox **root;
  char *name;
{
  add_mailbox(root, make_path(_PATH_MAILDIR, name), "INBOX"); }

/* Read Configuration File */
void read_config(root, homedir, username)
  Mailbox **root;
  char *homedir, *username;
{
  FILE *f = NULL;
  char line[1024], filename[256];
  char *cp = NULL, *maildir = "";
  unsigned int linenum = 0, opcode = 0;
  int changed_mail = 0, changed_persist = 0, changed_time = 0;

  if (RCFILE_OP)
    snprintf(filename, sizeof filename, "%.200s", rcfile);
  else
    snprintf(filename, sizeof filename, "%.100s/%.100s", homedir,rcfile);

  /* check for existance of conf file */
  if (access(filename, F_OK) == -1) {
    if (RCFILE_OP) {
      fprintf(stderr, "Could not open %s: file does not exist\n", filename);
    }
    set_defaults(root, username);
    return;
  }

  /* check for existance of conf file */
  if (access(filename, R_OK) == -1) {
    fprintf(stderr, "Could not open %s: access denied\n", filename);
    set_defaults(root, username);
    return;
  }

  /* open rc file */
  f = fopen(filename, "r");
  if (!f) {
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
      case oMail:
        /* if changed from command line */
        if (MAIL_OP) {
          cp = strtok(NULL, "\n");
          break;
        }
        if (changed_mail) {
          cp = strtok(NULL, WHITESPACE);
          fprintf(stderr,"%s line %d: multiple Mail commands.\n",
                  filename, linenum);
        } else {
          cp = strtok(NULL, WHITESPACE);
          if (!cp)
            fprintf(stderr,"%s line %d: missing argument.\n",
                    filename, linenum);
          else if (strcasecmp(cp,"yes") == 0 || atoi(cp) == 1)
            CHECK_MAIL = 1;
          else if (strcasecmp(cp,"no") == 0 || atoi(cp) == 0)
            CHECK_MAIL = 0;
          else {
            fprintf(stderr,
                    "%s line %d: Invalid Mail argument (%.200s).\n",
                    filename, linenum, cp);
          }
          changed_mail = 1;
        }
        break;

      case oMaildir:
        if (!CHECK_MAIL) {
          cp = strtok(NULL, "\n");
          break;
        }
        /* save the maildir for building paths to bailboxes */
        cp = strtok(NULL, WHITESPACE);
        if (!cp)
          fprintf(stderr,"%s line %d: missing argument.\n",
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
        if (!CHECK_MAIL) {
          cp = strtok(NULL, "\n");
          break;
        }
        /* append mailbox to maildir and save */
        cp = strtok(NULL, WHITESPACE);
        if (!cp)
          fprintf(stderr,"%s line %d: missing argument.\n",
                  filename, linenum);
        else if (strcmp(maildir,"") != 0) {
          char *desc = strtok(NULL, "\n");
          if (!desc) desc = strdup(cp);
          else if (strstr(desc,"{") && strstr(desc,"}"))
            desc = strtok(desc,"{}");
          else {
            fprintf(stderr,"%s line %d: garbage at end of line.\n",
                    filename, linenum);
            desc = strdup(cp);
          }
          add_mailbox(root, make_path(maildir,cp), desc);
        } else {
          fprintf(stderr,
                  "%s line %d: Mailbox option needs a Maildir parent.\n",
                  filename, linenum);
        }
        break;

      case oPersist:
        /* if changed from command line */
        if (PERSIST_OP) {
          cp = strtok(NULL, "\n");
          break;
        }
        if (changed_persist) {
          cp = strtok(NULL, WHITESPACE);
          fprintf(stderr,"%s line %d: multiple Persist commands.\n",
                  filename, linenum);
        } else {
          cp = strtok(NULL, WHITESPACE);
          if (!cp)
            fprintf(stderr,"%s line %d: missing argument.\n",
                    filename, linenum);
          else if (strcasecmp(cp,"yes") == 0 || atoi(cp) == 1)
            PERSIST = 1;
          else if (strcasecmp(cp,"no") == 0 || atoi(cp) == 0)
            PERSIST = 0;
          else {
            fprintf(stderr,
                    "%s line %d: Invalid Persist argument (%.200s).\n",
                    filename, linenum, cp);
          }
          changed_persist = 1;
        }
        break;

      case oTime:
        /* if changed from command line */
        if (TIME_OP) {
          cp = strtok(NULL, "\n");
          break;
        }
        /* change TIME */
        if (changed_time) {
          cp = strtok(NULL, WHITESPACE);
          fprintf(stderr,"%s line %d: multiple Time commands.\n",
                  filename, linenum);
        } else {
          cp = strtok(NULL, WHITESPACE);
          if (!cp)
            fprintf(stderr,"%s line %d: missing argument.\n",
                    filename, linenum);
          else if (atoi(cp) >= MIN_TIME) TIME = atoi(cp);
          else {
            fprintf(stderr,
                    "%s line %d: Time value less than minimum (%d).\n",
                    filename, linenum, MIN_TIME);
            TIME = MIN_TIME;
          }
          changed_time = 1;
        }
        break;

      default:
        break;
      } /* switch */

      /* check for garbage at EOL */
      if (strtok(NULL, WHITESPACE) != NULL)
        fprintf(stderr,"%s line %d: garbage at end of line.\n",
                filename, linenum);
    } /* else */
  } /* while */
  fclose(f);

  /* if no mailboxes where setup, add system inbox */
  if (CHECK_MAIL && root == NULL) { set_defaults(root, username); }
  else if (!CHECK_MAIL) { free_mailboxes(*root); }
}

/* Re-execute */
void re_exec(argc, argv, opt_cnt, as_mesg)
  int argc, opt_cnt, as_mesg;
  char *argv[];
{
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
void check_env(void)
{
  if (getenv(AWAY_RCFILE)) {
    rcfile = getenv(AWAY_RCFILE);
    RCFILE_OP = 1;
    unsetenv(AWAY_RCFILE);
  }

  if (getenv(AWAY_NOTIME)) {
    TIME_OP = 1;
    unsetenv(AWAY_NOTIME);
  } else if (getenv(AWAY_TIME)) {
    if (atoi(getenv(AWAY_TIME)) >= MIN_TIME) {
      TIME = atoi(getenv(AWAY_TIME));
      TIME_OP = 1;
    }
    unsetenv(AWAY_TIME);
  }

  if (getenv(AWAY_PERSIST)) {
    PERSIST = atoi(getenv(AWAY_PERSIST));
    PERSIST_OP = 1;
    unsetenv(AWAY_PERSIST);
  }

  if (getenv(AWAY_MAIL)) {
    CHECK_MAIL = atoi(getenv(AWAY_MAIL));
    MAIL_OP = 1;
    unsetenv(AWAY_MAIL);
  }
}

/* free mailboxes linked list */
void free_mailboxes(root)
  Mailbox *root;
{
  while (root != NULL) {
    Mailbox *next = root->next;
    if (root->path != NULL) free(root->path);
    free(root);
    root = next;
  };
}

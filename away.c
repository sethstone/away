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

int main(int argc, char *argv[]) {
  if ((argc > 2) && (strcmp(argv[1],"-m") == 0)) { exec_as_mesg(argc, argv); }
  else if ((argc == 2) && (strcmp(argv[1],"--help") == 0)) { ext_help(); }
  else if ((argc == 2) && (strcmp(argv[1],"-m") == 0)) { short_help(); }
  else if (argc == 1) { short_help(); }
  else {
    pthread_t mail_thread;
    short error = 1;
    struct passwd *pw = NULL;
    Mailbox *mailboxRoot = NULL, *mb = NULL;

    signal(SIGINT , SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGUSR1, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);
 
    /* get home dir */
    pw = getpwuid(getuid());
    if (!pw) { fprintf(stderr, "You don't exist!\n"); exit(1); }
    read_config(&mailboxRoot, pw->pw_dir, pw->pw_name);

    /* build time string */
    awayTime = make_time();
    /* start mail checking thread */
    pthread_create(&mail_thread, NULL, (void*)&mail_thread_f, mailboxRoot);

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
  return 0;
}

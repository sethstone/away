/* strstr - from linux/lib/string.c
 * Copyright (C) 1991, 1992  Linus Torvalds
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

#include <string.h>

char *strstr(const char * s1,const char * s2)
{
  int l1, l2;

  l2 = strlen(s2);
  if (!l2)
    return (char *) s1;
  l1 = strlen(s1);
  while (l1 >= l2) {
    l1--;
    if (!memcmp(s1,s2,l2))
      return (char *) s1;
    s1++;
  }
  return NULL;
}


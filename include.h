/*(C) Copyright 2012 Tyrell Keene
  This file is part of Cash.
  
  Cash is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  Cash is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Cash.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef INCLUDE_H
#define INCLUDE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <sys/wait.h>
#include <syslog.h>
#include <limits.h>
#include <pwd.h>
#include <readline/readline.h>

/* Prototypes */
void open_log(void);
void print_usage(FILE*, int, const char *);
void get_options(int, char **);
void init_env(void);
void parse(char *, char **);
void parse_rc(void);
void rm_nl(char *, int);
void exit_clean(int);
int built_ins(char **);
int write_history_file(char *);
int execute(char **);
/* End Prototypes */

#endif

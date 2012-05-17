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

/*Informational stuff*/
const char *version_string = "cash-0.1";
const char *help_string = "Cash, version 0.1, Linux\n--help\t\t-h\tshow this help screen\n--version\t-v\tshow version info\n--restricted\t-r\trun restricted shell (no cd)\n--no-history\t-n\tdont write history to file for this session\n--logging\t-l\tthe shell will keep a log in /var/log/messages for this session\n--verbose\t-V\tthe shell will write the log to both /var/log/messages and stderr.\n\t\t\tlogging will be turned on with verbose\n--alt-hist\t-f\t<filename>\tThe shell will write history to the file you specify\n\t\t\tinstead of the default ~/.cash_history.\n";

const struct option long_options[] = {
  {"restricted", 0, NULL, 'r'},
  {"help",       0, NULL, 'h'},
  {"no-history", 0, NULL, 'n'},
  {"version",    0, NULL, 'v'},
  {"verbose",    0, NULL, 'V'},
  {"logging",    0, NULL, 'l'},
  {"alt-hist",   1, NULL, 'f'},
  {NULL,         0, NULL,  0 }
};

/*for input*/
char line[4096];        
char *argv[4096] = {NULL};  

/*History file, filename, and open flag*/
const char *history_filename = "/.cash_history";    
char *alt_history_filepath;
const char *rc_filename = "/.cashrc";
char *home_path;
FILE *history_file;
FILE *rc_file;
char *PS1;

/*Shell stuff*/
pid_t cash_pid;
int cash_interactive, cash_terminal;

/*flags*/
_Bool restricted;         /*For restricted shell, 1 is restricted, 0 is not*/
_Bool open_history_file;  /*history file open. 1 is open, 0 is not*/
_Bool no_history;         /*no history option, 1 is no history, 0 history*/
_Bool logging;            /*logging option, 1 is on, 0 is off.*/
_Bool verbose;            /*verbose option, 1 is on, 0 is off.*/
_Bool alt_hist;           /*alt history file specified. 1 is yes, 0 is no*/

/*Environment stuff*/
typedef struct{
  char *home,
    *logname,
    *display,
    *path,
    *term;
  char cur_dir[PATH_MAX];
}ENVIRONMENT;

/* This is our structure to hold environment variables */
ENVIRONMENT *env;

/* Custom shell option strings */
const char* shell_user = "\\u";
const char* shell_cwd = "\\w";
const char* shell_host = "\\h";


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

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

#include "include.h"
#include "cash.h"

extern _Bool restricted;
extern _Bool open_history_file;
extern _Bool no_history;
extern _Bool logging;
extern _Bool verbose;

extern ENVIRONMENT *env;

extern const struct option long_options[];
extern const char* version_string;
extern const char* help_string;

int built_ins(char *in[]){
  if(strcmp(in[0], "#") == 0)
    return 1;
  if(strcmp(in[0], "exit") == 0){
    exit_clean(0);
  }
  if(restricted && strcmp(in[0], "cd") == 0){
    fprintf(stderr,"Can't cd: restricted shell\n");
    if(logging)
      syslog(LOG_NOTICE,"restricted shell attempted to cd");
    else
      return 1;
  }
  else
    /*
     * Everything beyond this point is restricted and can't be executed if the 
     * shell is run with the -r/--restrcited flag. Make sure not to put anything
     * critical here, or there will be issues
     */
    if(strcmp(in[0], "cd") == 0 && in[1] != NULL) {
    if(chdir(in[1]) == -1){
      perror("cd");
      return 1;
    }
    else
      return 1;
  }
  if(strncmp(in[0], "cd", 2) == 0 && in[1] == NULL){
    if(chdir(env->home) == -1){
      perror("cd");
      return 1;
    }
    else
      return 1;
  }
  else
    return 0;
} /*End restricted*/

void print_usage(FILE* stream, int exit_code, const char *string){
  fprintf(stream, "%s", string);
  exit(exit_code);
}

void get_options(int arg_count, char **arg_value){
  int next_option;
  const char* const short_options = "hrnvlV";
  /*Here we check for startup flags, and act accordingly*/
  do {
    next_option = getopt_long(arg_count, arg_value, short_options, long_options, NULL);
    switch(next_option){
    case 'h':
      print_usage(stdout, 0, help_string);
      break;
    case 'r':
      restricted = 1;
      break;
    case 'n':
      fprintf(stderr,"history file will not be opened for this session\n");
      no_history = 1;
      break;
    case 'v':
      print_usage(stdout, 0, version_string);
      break;
    case 'l':
      logging = 1;
      break;
    case 'V':
      fprintf(stderr,"log will be written to both /var/log/messages and stderr\n");
      if(!logging)
	logging = 1;
      verbose = 1;
    case -1:
      break;
    case '?':
      print_usage(stderr, 1, help_string);
    default:
      abort();
    }
  }
  while(next_option != -1);
  return;
}

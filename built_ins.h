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

int built_ins(char *in[]){
  if(strncmp(in[0], "#", 1) == 0)
    return 1;
  if(strcmp(in[0], "exit") == 0){
    exit_clean(0);
  }
  if(restricted && strncmp(in[0], "cd", 2) == 0){
    fprintf(stderr,"Can't cd: restricted shell\n");
    if(logging)
      syslog(LOG_NOTICE,"user %s attempted to cd in restricted shell", env->logname);
    else
      return 1;
  }
  else
    /*
     * Everything beyond this point is restricted and can't be executed if the 
     * shell is run with the -r/--restrcited flag. Make sure not to put anything
     * critical here, or there will be issues
     */
    if(strncmp(in[0], "cd", 2) == 0 && in[1] != NULL) {
    if(chdir(in[1]) == -1){
      perror("cd");
      return 1;
    }
    else
      return 1;
  }
  if(strcmp(in[0], "cd") == 0 && in[1] == NULL){
    if(chdir(env->home) == -1){
      perror("cd");
      return 1;
    }
    else
      return 1;
  }
  else
    return 0;
} /*End restricted built-ins*/

void print_usage(FILE* stream, int exit_code, const char *string){
  fprintf(stream, "%s", string);
  exit(exit_code);
}

void get_options(int arg_count, char **arg_value){
  int next_option;
  const char* const short_options = "hrnvlVf:";
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
      if(!logging)
	logging = 1;
      verbose = 1;
      fprintf(stderr,"log will be written to both /var/log/messages and stderr\n");
      break;
    case 'f':
      if(logging)
      	syslog(LOG_DEBUG,"history will be written to %s instead of %s", optarg, history_filename);
      alt_history_filepath = optarg;
      alt_hist = 1;
      break;
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

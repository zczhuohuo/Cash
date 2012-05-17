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

#include "include.h"
#include "built_ins.h"

extern int built_ins(char **);
extern void print_usage(FILE*, int, const char *);
extern void get_options(int, char **);

void open_log(void){
  if(verbose){
    if(!logging)
      logging = 1;
    openlog(version_string, LOG_CONS|LOG_PID|LOG_PERROR, LOG_USER);
  }
  else
    openlog(version_string, LOG_CONS|LOG_PID, LOG_USER);  
}

/*
 * Here's our exit function. Just make sure everything
 * is freed, and any open files are closed.
 */
void exit_clean(int ret_no){
  if(open_history_file)
    if(fclose(history_file) != 0){
      perror("fclose");
      if(logging)
	syslog(LOG_ERR,"failed to close history file on exit");
    }
  if(env)
    free(env);
  if(rc_file)
    fclose(rc_file);
  if(PS1)
    free(PS1);
  if(logging){
    syslog(LOG_DEBUG, "shell exited");
    closelog();
  }
  exit(ret_no);
}

void init_env(void){
  cash_interactive = isatty(cash_terminal);
  if(cash_interactive != 0){
    env = malloc(sizeof(ENVIRONMENT));
    if(!env){
      perror("Couldn't allocate memory to environemnt structure\n");
      if(logging){
	syslog(LOG_CRIT,"couldnt allocate memory to environment structure");
	exit_clean(1);
      }
    }else{
      signal(SIGINT, SIG_IGN);
      /*Here is where we setup out environment structure. Much of the shell
       * is based on this part working. after a call to init_env you'll end up
       * with env full of all the environment variables that are needed. This 
       * will go until exit where env is freed*/
      env->home    = getenv("HOME");    
      env->logname = getenv("LOGNAME");
      env->path    = getenv("PATH");
      env->display = getenv("DISPLAY");
      env->term    = getenv("TERM");

      setenv("HOME",env->home,1);
      setenv("LOGNAME",env->logname,1);
      setenv("PATH",env->path,1);
      setenv("DISPLAY",env->display,1);
      setenv("TERM", env->term, 1);

      cash_pid = getpid();
      if(logging)
	syslog(LOG_DEBUG, "shell spawned");
    }
  }
}

/* Replace the first instance of sub in dst with src, and return a pointer to that instance */
char* strrplc(char* dst, const char* sub, char* src){
  char* loc;
  char* buf;
  if((dst && sub && src) == 0)
    return NULL;
  loc = strstr(dst, sub);
  if(loc != NULL){
    buf = malloc(sizeof(char) * 8192);
    strcpy(buf, src);    
    strcat(buf, loc+strlen(sub));
    memcpy(loc, buf, strlen(buf));
    free(buf);
    return loc;
  } else
    return NULL;
}

void parse_rc(void){
  char* buf = malloc(sizeof(char) * 4096);
  int i = 0;
  char* p;
  if(env){
    strcpy(buf, env->home);
    strcat(buf, rc_filename);
  } else
      return;
  if(!(rc_file = fopen(buf, "r"))){
    if(logging)
      syslog(LOG_DEBUG, "rc file wasnt found or could not be opened");
    return;
  }
  memset(buf, 0, 4096);
  while(fgets(buf, 4096, rc_file)){
    if( (p = strstr(buf, "PS1 = \"")) ){
      p += strlen("PS1 = \"");
      while(p[i++] != '\"')
	continue;
      i--;
      PS1 = malloc(sizeof(char) * (i+1));
      strncpy(PS1, p, i);
      PS1[i] = '\0';
    }
  }
  free(buf);
}

void format_prompt(char* dst, int len){
  char buf[4096];
  strcpy(dst, PS1);
  strrplc(dst, shell_user, env->logname);
  memset(buf, 0, sizeof(buf));
  gethostname(buf, 4096);
  strrplc(dst, shell_host, buf);
  strrplc(dst, shell_cwd, env->cur_dir);
}

void parse(char *line, char **argv){
  while (*line != '\0') {      
    while (*line == ' ' || *line == '\t' || *line == '\n')
      *line++ = '\0';   
    *argv++ = line;        
    while (*line != '\0' && *line != ' ' && 
	   *line != '\t' && *line != '\n') 
      line++;             
  }
  *argv = '\0'; 
}

void rm_nl(char *sp, int len){
  volatile int i;
  for(i = 0; i < len; i++){
    if(sp[i] == '\n'){
      sp[i] = '\0';
      break;
    }
  }
  return;
}

/*
 * This function is actually entirely of my own doing
 * first it checks to see if the history file has been
 * opened yet, if not it makes the path using some 
 * string magic and opens the file, then writes to it.
 * If it's already open, it simply writes to the file 
 * and returns. Returns non-zero on error zero otherwise.
 * open_history_file is boolean. So open file is 1, otherwise 0.
 */

int write_history_file(char *input){
  if(!open_history_file){
    if(alt_hist) {
      if(!(history_file = fopen(alt_history_filepath, "a+"))) {
	if(logging)
	  syslog(LOG_ERR,"unable to open alternative history file %s", alt_history_filepath);
      }
      else
	open_history_file = 1;
      fprintf(history_file,"%s", input);
      return 0;
    }
    else
      if(home_path == NULL)
	home_path = malloc(sizeof(char) * 4096);
    strcpy(home_path, env->home);
    strcat(home_path, history_filename);
    if( (history_file = fopen(home_path, "a+")) == NULL){
      if(logging)
	syslog(LOG_ERR,"unable to open history file");
      free(home_path);
      perror("history");
      return -1;
    }
    else {
      if(home_path != NULL)
	free(home_path);
      open_history_file = 1;
    }
  }  
  fprintf(history_file, "%s", input);
  return 0;
}

int execute(char **argv){
  pid_t pid;
  int status;
  if ((pid = fork()) < 0) {     
    perror("fork");
    if(logging)
      syslog(LOG_ERR,"failed to fork");
    return -1;
  }
  else if (pid == 0) {          
    if((execvp(*argv, argv)) < 0) {    
      if(logging)
	syslog(LOG_ERR,"failed to execute %s", *argv);
      perror("exec");
      abort();
    }
  }
  else {                               
    while (wait(&status) != pid)       
      ;
  }
  return 0;
}

int main(int argc, char* arg[]){
  char fmt_PS1[4096];
  logging    = 0;
  restricted = 0;
  no_history = 0;
  alt_hist   = 0;
  verbose    = 0;
  alt_hist   = 0;

  get_options(argc,arg);

  if(logging){
    logging = 1;
    open_log();
  }
  init_env();
  parse_rc();

  /*
   * Here's the main loop.
   * First we go ahead and get the current working directory
   * Next we see if the we're in a restricted shell, and change the
   * prompt accordingly. Then we get the string, and if all goes well 
   * with that, write the history. Then we remove the \n terminator and
   * feed the string to the parser. After everything is parsed we pass the
   * string to check for builtins. If it was a built in, there's no need 
   * to execute it, so we skip the execute function, otherwise we go ahead 
   * and execute it.
   */
   
  while(1){  
    if( (getcwd(env->cur_dir, sizeof(env->cur_dir)) == NULL)) {
      if(logging)
	syslog(LOG_ERR, "could not get current working directory");
    }
    else
      if(restricted)
	fprintf(stdout,"%s$ ", version_string);
    else
      if(PS1){
	memset(fmt_PS1, 0, 4096);
	format_prompt(fmt_PS1, 4096);
	fprintf(stdout, "%s", fmt_PS1);
      }
      else
	fprintf(stdout,"%s$ ", version_string);
    if(fgets(line, 4096, stdin) == NULL)
	  continue;
    if(line[1] == '\0'){
      continue;
    }
    if(!no_history){
      if(write_history_file(line) == -1){
	fprintf(stderr,"history disabled, if logging is enabled check /var/log/messages for more info\n");
	syslog(LOG_ERR,"history file could not be opened or written to, history disabled");
	no_history = 1;
      }
    }
    rm_nl(line, strlen(line));
    parse(line, argv);      
    if(built_ins(argv) == 1)
      continue;
    else
      execute(argv);
  }
}

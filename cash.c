/*(C) Copyright 2012 Tyrell Keene, Max Rose
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
#include "cash.h"

/*Informational stuff*/
const char *version_string = "cash-0.1";
const char *help_string = "Cash, version 0.1, Linux\n--help        -h    show this help screen\n--version     -v    show version info\n--restricted  -r    run restricted shell (no cd)\n--no-history  -n    dont write history to file for this session\n--logging     -l    the shell will keep a log in /var/log/messages for this session\n--verbose     -V    the shell will write the log to both /var/log/messages and stderr.                                        logging will be turned on with verbose\n";

const struct option long_options[] = {
  {"restricted", 0, NULL, 'r'},
  {"help",       0, NULL, 'h'},
  {"no-history", 0, NULL, 'n'},
  {"version",    0, NULL, 'v'},
  {"verbose",    0, NULL, 'V'},
  {"logging",    0, NULL, 'l'},
  {NULL,         0, NULL,  0 }
};

/*for input*/
char line[4096];        
char *argv[4096] = {NULL};  

/*History file, filename, and open flag*/
const char *history_filename = "/.cash_history";    
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
_Bool no_history;         /*no history option, 1 is no history, 0 no history*/
_Bool logging;            /*logging option, 1 is on, 0 is off.*/
_Bool verbose;            /*verbose option, 1 is on, 0 is off.*/
_Bool specified_PS1;

/* This is our structure to hold environment variables */
ENVIRONMENT *env;

/* Custom shell option strings */
const char* shell_user = "\\u";
const char* shell_cwd = "\\w";
const char* shell_host = "\\h";
const char* shell_version = "\\v";

const char* default_prompt = "\\v:\\w$ ";

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
  if(PS1 && specified_PS1 == 1)
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
      open_history_file = 0;
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
  } else{
    free(buf);
    return;
  }
  if(!(rc_file = fopen(buf, "r"))){
    syslog(LOG_DEBUG, "rc file wasnt found or couldnt be opened");
    free(buf);
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
      specified_PS1 = 1;
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
  strrplc(dst, shell_version, version_string);
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

void add_nl(char *sp, int len){
  volatile int i;
  for(i = 0; i < len+1; i++){
    if(sp[i] == '\0'){
      sp[i] = '\n';
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
  if(open_history_file == 0){
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
  add_nl(input, strlen(input));
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
  verbose    = 0;
  char* input = 0;
  get_options(argc,arg);

  if(logging){
    logging = 1;
    open_log();
  }
  init_env();
  parse_rc();

  if(!PS1){
    PS1 = default_prompt; 
    specified_PS1 = 0;
  }

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
	syslog(LOG_ERR, "couldnt get current working directory");
    }    
    
    if(!no_history){
      if(write_history_file(line) == -1){
	fprintf(stderr,"history disabled, if logging is enabled check /var/log/messages for more info\n");
	syslog(LOG_ERR,"history file couldn't be opened or written to, history disabled");
	no_history = 1;
      }
    }
    
    memset(fmt_PS1, 0, 4096);
    format_prompt(fmt_PS1, 4096);
    
    input = readline(fmt_PS1);
    if(!input)
      exit_clean(0);
    
    if(strlen(input) < 1)
      continue;

    strcpy(line, input);
    parse(line, argv);      
    
    if(built_ins(argv) == 1)
      continue;
    else
      execute(argv);
  }
}

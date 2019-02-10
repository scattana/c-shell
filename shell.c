// Assignment 1: C Shell Programming
// // CSE-34341 Operating Systems (SP19)
// Seth Cattanach (scattana)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// enable "true and false" to be used as boolean types
typedef enum {false, true} bool;

// global variable to contain command history (max 100):
char history[100][BUFSIZ];
int hist_count = 0;

// global variables to track whether a file stream needs to be closed
bool input_flag = false, output_flag = false;

// Function Prototypes:
char ** parse_options(char[]);
void free_mem(char ***);
bool execute(char ***, int, bool);
bool isBackground(char **);
void clear_history();
void view_history(int);
char * find_rec_cmd(char*);
bool starts_with(const char*, const char*);
bool out_redir(char **);
bool in_redir(char **);
char * handle_output(char **);
char * handle_input(char **);
bool handle_io(char **);
char * strip_space_lead_trail(char *);
int count_pipes(char *);

// takes in a single command returns a dynamically-allocated array of strings
// to pass to execvp()
char ** parse_options(char input[]){
	// count number of spaces to determine how many strings to allocate
	int spaces = 0;
	for(int i=0; i<strlen(input); i++) if(isspace(input[i])) spaces++;

	// allocate space for the array of strings itself
	char **args;
	args = malloc(sizeof(char*) * (spaces+2));			// allocate space for N strings
	// note: +1 b/c # of words = spaces+1, and another +1 for NULL terminator

	// tokenize input string with strtok, allocate mem, and copy into array of strings
	int i=0;
	for(char *temp = strtok(input," "); temp != NULL; temp = strtok(NULL," ")){
		args[i] = malloc(BUFSIZ * sizeof(char));
		strcpy(args[i], temp);
		i++;

	}
	args[i] = NULL;	// NULL terminator for the last item in array of strings

	return args;
}

// releases dynamically allocated memory for an array of strings
void free_mem(char *** ptr){
	int len = -1;
	while(ptr[++len] != NULL) continue;	// continues until "len" is the # of commands
	for(int i=0; i<len; i++){
		int args=0;									// to track number of args
		while(ptr[i][args] != NULL) args++;			// get length of args
		for(int j=0; j<args; j++) free(ptr[i][j]);	// deallocate args
		free(ptr[i]);								// deallocate command
	} free(ptr);									// deallocate array of commands
}

// tries to create child process and execute specified command (parameter)
// returns "false" if unable to fork() or execvp()
bool execute(char ***commands, int cmd_count, bool background_flag){
	int cmd_index = 0;
	int p[2];								// set up pipe work
	int fd_in = 0;
	char **command = *commands;				// get first command

	while(cmd_index != cmd_count){			// continuous loop for all pipes (if any)
		if(cmd_count > 1){
			if(pipe(p) != 0){
				fprintf(stderr,"%s\n","Error: could not pipe");
				return false;
			}
		}

		pid_t pid;							// track process ID
		int status;							// track process status

		// prevent incorrect exit
		if(strcmp(command[0],"exit") == 0) return false;

		if((pid = fork()) < 0){				// start FORKING here!
			fprintf(stderr, "%s\n", "Error: Could not fork child process");
			return false;					// could not fork child process
		}
		else if(pid == 0){					// this is the newly-created child process
			if(cmd_count > 1){				// handle pipe if needed (>1 command)
				dup2(fd_in, 0);
				if(commands[cmd_index+1] != NULL) dup2(p[1], 1);
				close(p[0]);
			}

			// handle any input or output redirection
			if(!handle_io(command)){
				exit(1);					// kill forked process before returning
				return false;
			}
			// try to execute "execvp" and return false if error
			if(execvp(*command, command) < 0) {
				fprintf(stderr, "%s\n", "Error: could not exec the specified command");
				if(input_flag) fclose(stdin);
				if(output_flag) fclose(stdout);
				exit(1);		// terminate the forked process (w/exit failure status)
				return false;
			}
		}
		else{
			if(!background_flag){			// ignore this and continue to exec in background
				while(wait(&status) != pid) continue;	// wait for child to finish
			}
			if(cmd_count > 1){
				close(p[1]);
				fd_in = p[0];				// save input for next command
			}
		}
		if(input_flag){						// close file streams, as necessary
			if(fclose(stdin)!=0) fprintf(stderr,"Error: could not close infile\n");
		}
		if(output_flag){
			if(fclose(stdout)!=0) fprintf(stderr,"Error: could not close outfile\n");
		}
		cmd_index++;
		if(commands[cmd_index] != NULL) command = commands[cmd_index];
	}	// end of pipe WHILE
	return true;
}

// returns true/false based on whether the last arg is &
// in which case, the command should be executed in the background
bool isBackground(char ** command){
	int count = 0;
	while(command[count] != NULL) count++;
	if(strcmp(command[count-1],"&")==0) return true;
	else return false;
}

// called when "history -c" is indicated
void clear_history(){
	for(int i=0; i<100; i++) strcpy(history[i],"");
	hist_count = 0;
}

// view history with specified number of entries (oldest first)
void view_history(int hist_num){
	int start;
	// prevent re-printing entries if demand exceeds number of commands in history
	if(hist_num > hist_count) hist_num = hist_count;
	if(hist_num > 100) hist_num = 100;
	if(hist_count >= 100) start = hist_count % 100;
	else start = 0;
	
	// print off commands in history:
	for(int i=start; i<start+hist_num; i++){
		if(strcmp(history[i%100],"") != 0){
			fprintf(stdout,"%d\t%s\n",i,history[i%100]);
		}
	}
}

// returns most recent command in history with given prefix, or NULL if not found
char * find_rec_cmd(char *prefix){
	int most_recent_index = (hist_count-1) % 100;
	for(int i=most_recent_index; i != hist_count; i--){
		if(i==0) i = 99;
		if(starts_with(prefix, history[i])) return history[i];
	}
	return NULL;		// if command with given prefix not found in history
}

// utility function that checks whether "string" starts with "prefix"
bool starts_with(const char *prefix, const char *string){
	size_t pre_length = strlen(prefix);
	size_t str_length = strlen(string);
	return str_length < pre_length ? false : strncmp(prefix, string, pre_length) == 0;
}

// out_redir returns true/false based on presence of output redirection
bool out_redir(char ** cmd){
	int i=0;
	while(cmd[i] != NULL){
		if(strcmp(cmd[i],">")==0) return true;
		i++;
	}
	return false;
}

// in_redir returns true/false based on presence of input redirection
bool in_redir(char ** cmd){
	int i=0;
	while(cmd[i] != NULL){
		if(strcmp(cmd[i],"<")==0) return true;
		i++;
	}
	return false;
}

// handle_output returns modified command and opens output file stream
// (or returns NULL if there's an error in the command or while opening stream)
char * handle_output(char ** cmd){
	int i=0;
	char *filename;
	while(cmd[i] != NULL){
		if(strcmp(cmd[i],">")==0){
			if(cmd[i+1]==NULL) return NULL;
			filename = cmd[i+1];
			cmd[i+1] = NULL;
			cmd[i] = NULL;
			break;
		}
		i++;							// continue searching
	}
	return filename;
}

// handle_input returns modified command and opens input file stream
// (or returns NULL if there's an error in the command or while opening stream)
char * handle_input(char ** cmd){
	int i=0;
	char *filename;
	while(cmd[i] != NULL){
		if(strcmp(cmd[i],"<")==0){
			if(i==0) return NULL;		// this would mean "<" is the first command
			if(cmd[i+1]==NULL) return NULL;
			filename = cmd[i+1];
			for(int j=i; cmd[j+2] != NULL; j++) cmd[j] = cmd[j+2];
			break;
		}
		i++;							// continue searching
	}
	// move "i" to the index of the first NULL element of "cmd"
	for(i=0; cmd[i]!=NULL; i++) continue;
	cmd[i-1] = NULL;
	cmd[i-2] = NULL;
	return filename;
}

// handle_io is one level of abstraction above "handle_input" and "handle_output"
// it takes the modified command and conducts stream redirection as necessary
bool handle_io(char ** command){
	FILE *fin, *fout;
	if(in_redir(command)){
		char *infile = handle_input(command);
		fin = freopen(infile,"r",stdin);
		if(fin==NULL){
			fprintf(stderr,"Error: failed to open file for reading\n");
			return false;
		}
		input_flag = true;
	}
	if(out_redir(command)){
		char *outfile = handle_output(command);
		fout = freopen(outfile,"w",stdout);
		if(fout==NULL){
			fprintf(stderr,"Error: failed to open file for writing\n");
			return false;
		}
		output_flag = true;
	}	
	return true;						// successfully handled any I/O if present
}

// strip leading and trailing spaces from an input string
char * strip_space_lead_trail(char * temp){
	if(temp[0]==' ') temp++;
	if(isspace(temp[strlen(temp)-1])) temp[strlen(temp)-1] = '\0';
	return temp;
}

// count and return number of pipes in a command
int count_pipes(char * cmd){
	int count = 0;
	for(int i=0; i<strlen(cmd); i++) if(cmd[i]=='|') count++;
	return count;
}

// MAIN function
int main(int argc, char *argv[]){
	fprintf(stdout,"\n%s\n\n","C Shell Programming (Seth Cattanach)");

	char hold[BUFSIZ] = "";
	clear_history();
	bool false_exit_flag = false;

	// get and pre-process user command(s)
	while(strcmp(hold,"exit") != 0 || (strcmp(hold,"exit")==0 && false_exit_flag)){
		fprintf(stdout, "%s", "--> ");
		fgets(hold, BUFSIZ, stdin);
		if(strlen(hold) > 0) hold[strlen(hold)-1] = '\0';
		if(strcmp(hold,"exit") == 0) return 0;

		// tokenize semicolons and parse/execute each command appropriately:
		bool exit_flag = false;
		char* arr = hold;

		for(char *temp = strtok_r(arr,";",&arr); temp != NULL; temp = strtok_r(NULL,";",&arr)){
			// strip leading space and trailing space if present
			temp = strip_space_lead_trail(temp);

			if((strstr(temp,"exit") != NULL) && (strlen(temp) > 4)){
				false_exit_flag = true;
				fprintf(stderr,"%s\n","Error: unrecognized command");
			}

			// check for "!!" command
			if(strcmp(temp,"!!")==0) temp = history[(hist_count-1)%100];

			// check for and set exit condition if present
			if(strcmp(temp,"exit")==0){
				exit_flag = true;
				continue;
			}

			// check for and handle commands of the form "!string"
			if(strlen(temp) > 1){
				if(temp[0]=='!' && temp[1]!='!'){
					char *prefix =  temp;
					prefix++;		// increment past initial '!' and set pointer
					temp = find_rec_cmd(prefix);
					if(temp == NULL){
						fprintf(stderr,"%s\n","Error: matching prefix not found");
						continue;		// skip remainder of execution
					} else fprintf(stdout,"Executing command: %s\n",temp);
				}
			}

			// add the non-parsed command to history (before executing, regardless of success)
			strcpy(history[hist_count%100],temp);
			hist_count++;

			// modify behavior if "history" command is found
			if((strstr(temp,"history") != NULL) && (temp[0] == 'h')){
				if(strstr(temp," -c") != NULL) clear_history();
				else{
					if((strstr(temp," ") == NULL) && (strlen(temp)==7)) view_history(100);
					else{
						if((strstr(temp," ") == NULL) && (strlen(temp)!=7)){	// error!
							fprintf(stderr,"%s\n","Error: invalid command");
							continue;
						}
						char **t = parse_options(temp);
						int hist_num = atoi(t[1]);		// get number of history commands
						view_history(hist_num);		// view history with N commands
						// edge case: free memory
						int x=0;
						while(t[x] != NULL) x++;
						for(int i=0; i<x; i++) free(t[i]);
						free(t);
					}
				}
				continue;		// skip normal execution after handling "history" command
			}

			// count and parse for PIPES
			int num_pipes = count_pipes(temp);			// get number of pipes in the string
			char *** commands;
			commands = malloc(sizeof(char**) * (num_pipes+2));
			int cmd_count = 0;
			char **command;
			bool background_flag = false;
			for(char *t = strtok_r(temp,"|",&temp); t != NULL; t = strtok_r(NULL,"|",&temp)){
				command = parse_options(t);
				if(isBackground(command)){
					background_flag = true;				// trim & symbol and free its memory
					int x=0;
					while(command[x] != NULL) x++;		// increment until end
					free(command[x-1]);					// dealloc mem before overwriting
					command[x-1] = NULL;				// overwrite "&" character
				}

				commands[cmd_count] = command;			// push command to "commands" list
				cmd_count++;							// track how many commands
			}
			commands[cmd_count] = NULL;					// sentinel for final command

			execute(commands, cmd_count, background_flag);
			
			free_mem(commands);							// free heap memory

		}	// end of FOR loop (strtok ; )
		if(exit_flag) exit(0);	// if "exit" was specified during last series of cmds

	}	// end of "while" loop
	return 0;
}

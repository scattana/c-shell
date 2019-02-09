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

// Function Prototypes:
char ** parse_options(char[]);
void free_mem(char **);
bool execute(char **);
bool isBackground(char **);
void clear_history();
void view_history(int);
char * find_rec_cmd(char*);
bool starts_with(const char*, const char*);
bool out_redir(char **);
bool in_redir(char **);
char * handle_output(char **);
char * handle_input(char **);


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
void free_mem(char ** ptr){
	int len = -1;
	while(ptr[++len] != NULL) continue;	// continues until "len" is the # of strings
	for(int i=0; i<len; i++){
		free(ptr[i]);
	}
	free(ptr);
}

// tries to create child process and execute specified command (parameter)
// returns "false" if unable to fork() or execvp()
bool execute(char **command){
	// if background process, set flag and get rid of & from args:
	bool background_flag = false;
	bool input_flag = false, output_flag = false;
	FILE *fin, *fout;

	if(isBackground(command)){
		background_flag = true;
		int tmp = 0;
		while(command[tmp] != NULL) tmp++;	// count # of elements, including last NULL
		free(command[tmp-1]);	// about to be set to NULL, so first, free dynamic mem
		command[tmp-1] = NULL;	// set & to be NULL
	}

	pid_t pid;
	int status;

	// DEBUG: test print the command
	for(int i=0; command[i] != NULL; i++) fprintf(stderr,"111 %s\n",command[i]);

	// prevent incorrect exit
	if(strcmp(command[0],"exit") == 0) return false;

	if((pid = fork()) < 0){
		fprintf(stderr, "%s\n", "Error: Could not fork child process");
		return false;		// could not fork child process
	}
	else if(pid == 0){	// this is the newly-created child process
		// handle I/O redirection if necessary:
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
	
		// try to execute "execvp" and return false if error
		if(execvp(*command, command) < 0) {
			fprintf(stderr, "%s\n", "Error: could not exec the specified command");
			if(input_flag) fclose(stdin);
			if(output_flag) fclose(stdout);
			exit(1);		// terminate the forked process (w/exit failure status)
			return false;
		}
		for(int i=0; command[i] != NULL; i++) fprintf(stderr,"222 %s\n",command[i]);
	}
	else{
		if(!background_flag){	// ignore this and continue to exec in background
			while(wait(&status) != pid) continue;	// wait for child to finish
		}
	}
	for(int i=0; command[i] != NULL; i++) fprintf(stderr,"333 %s\n",command[i]);
	if(input_flag){
		fprintf(stderr,"inflag is true\n");
		if(fclose(stdin)!=0) fprintf(stderr,"ERROR 1\n");
	}
	if(output_flag){
		fprintf(stderr,"outflag is true\n");
		if(fclose(stdout)!=0) fprintf(stderr,"ERROR 2\n");
	}
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

void clear_history(){
	for(int i=0; i<100; i++) strcpy(history[i],"");
	hist_count = 0;
}

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
		i++;	// continue searching
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
			// STILL NEED TO DELETE THE TWO ITEMS IN CMD
			for(int j=i; cmd[j+2] != NULL; j++) cmd[j] = cmd[j+2];
			break;
		}
		i++;		// continue searching
	}
	cmd[i-1] = NULL;
	cmd[i-2] = NULL;
	return filename;
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
		if((strstr(hold,"exit") != NULL) && (strlen(hold) > 4)){
			false_exit_flag = true;
			fprintf(stderr,"%s\n","Error: unrecognized command");
		}
		char* arr = hold;
		for(char *temp = strtok_r(arr,";",&arr); temp != NULL; temp = strtok_r(NULL,";",&arr)){
			// strip leading space and trailing space if present
			if(temp[0]==' ') temp++;
			if(isspace(temp[strlen(temp)-1])) temp[strlen(temp)-1] = '\0';

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
			if(strstr(temp,"history") != NULL && temp[0] == 'h'){
				if(strstr(temp," -c") != NULL) clear_history();
				else{
					if(strstr(temp," ") == NULL) view_history(100);
					else{
						char **t = parse_options(temp);
						int hist_num = atoi(t[1]);		// get number of history commands
						view_history(hist_num);		// view history with specified number of commands
						free_mem(t);
					}
				}
				continue;		// skip normal execution after handling "history" command
			}


			char **command = parse_options(temp);
			execute(command);
			free_mem(command);

		}	// end of FOR loop
		if(exit_flag) exit(0);	// "exit" was specified during last series of cmds

		// test print parsed command
		//const char** ptr = (const char**)command;
		//while(*ptr != 0){
		//	fprintf(stdout, "%s\n", *ptr);
		//	ptr++;
		//}

	}	// end of "while" loop
	return 0;
}

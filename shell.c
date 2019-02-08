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
	if(isBackground(command)){
		background_flag = true;
		int tmp = 0;
		while(command[tmp] != NULL) tmp++;	// count # of elements, including last NULL
		free(command[tmp-1]);	// about to be set to NULL, so first, free dynamic mem
		command[tmp-1] = NULL;	// set & to be NULL
	}

	pid_t pid;
	int status;

	if((pid = fork()) < 0){
		fprintf(stderr, "%s\n", "Error: Could not fork child process");
		return false;		// could not fork child process
	}
	else if(pid == 0){	// this is the newly-created child process
		// try to execute "execvp" and return false if error
		if(execvp(*command, command) < 0) {
			fprintf(stderr, "%s\n", "Error: could not exec the specified command");
			exit(1);		// terminate the forked process (w/exit failure status)
			return false;
		}
	}
	else{
		if(!background_flag){	// ignore this and continue to exec in background
			while(wait(&status) != pid) continue;	// wait for child to finish
		}
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
	int temp = hist_num % 100;
	int start;
	if(hist_num >= 100) start = temp;
	else start = 0;
	for(int i=start; i<start+hist_num; i++){
		if(strcmp(history[i%100],"") != 0) fprintf(stdout,"%s\n",history[i%100]);
	}
}

// MAIN function
int main(int argc, char *argv[]){
	fprintf(stdout,"\n%s\n\n","C Shell Programming (Seth Cattanach)");

	char hold[BUFSIZ] = "";
	clear_history();

	// get and pre-process user command(s)
	while(strcmp(hold,"exit") != 0){
		fprintf(stdout, "%s", "--> ");
		fgets(hold, BUFSIZ, stdin);
		if(strlen(hold) > 0) hold[strlen(hold)-1] = '\0';
		if(strcmp(hold,"exit") == 0) return 0;

		// tokenize semicolons and parse/execute each command appropriately:
		bool exit_flag = false;
		char* arr = hold;
		for(char *temp = strtok_r(arr,";",&arr); temp != NULL; temp = strtok_r(NULL,";",&arr)){
			// strip leading space and trailing space if present
			if(temp[0]==' ') temp++;
			if(isspace(temp[strlen(temp)-1])) temp[strlen(temp)-1] = '\0';

			if(strcmp(temp,"exit")==0){
				exit_flag = true;
				continue;
			}

			// add the non-parsed command to history (before executing, regardless of success)
			strcpy(history[hist_count%100],temp);
			hist_count++;

			// modify behavior if "history" command is found
			if(strstr(temp,"history") != NULL){
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

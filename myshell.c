/********************************************************************************************
This is a template for assignment on writing a custom Shell. 

Students may change the return types and arguments of the functions given in this template,
but do not change the names of these functions.

Though use of any extra functions is not recommended, students may use new functions if they need to, 
but that should not make code unnecessorily complex to read.

Students should keep names of declared variable (and any new functions) self explanatory,
and add proper comments for every logical step.

Students need to be careful while forking a new process (no unnecessory process creations) 
or while inserting the single handler code (should be added at the correct places).

Finally, keep your filename as myshell.c, do not change this name (not even myshell.cpp, 
as you not need to use any features for this assignment that are supported by C++ but not by C).
*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()


/*This function will parse the input string into multiple commands or a single 
command with arguments depending on the delimiter (&&, ##, >, or spaces).*/
int parseInput(char* input_str, char** command_list)
{	
	// input_str    = the input string we got using getline
	// command_list = array of strings containing list of single commands seperated 
	//                by the detected delimeter

	int retval;
	
	// get newstring without \n, i.e without the last character
	int len = strlen(input_str);
	char* str_without_newline = (char *)malloc(len-1);   

	for(int j = 0; j < len-1; j++)
		str_without_newline[j] = input_str[j];
	strcat(str_without_newline," ");

	// get a copy of the new string without "\n"
	char* duplicate = strdup(str_without_newline);
	
	// check for the presence and type of delimiters
	if(strstr(duplicate, "&&") != NULL) 
	{
		// isolate each command seperated by delimeter && 
		int i = 0;
		char* sep_str;

		while((sep_str = strsep(&duplicate,"&&")) != NULL )
		{
			int count = 0;

			// store the isolated commands in command_list
			strcpy(command_list[i],sep_str);
			i++;
		}
		command_list[i]=NULL;

		// set retval=1 for parallel execution of multiple commands
		retval = 1;  				
	}
	else if(strstr(duplicate, "##") != NULL)
	{
		// isolate each command seperated by delimeter ## 
		int i = 0;
		char* sep_str;

		while((sep_str = strsep(&duplicate,"##")) != NULL )
		{
			int count = 0;

			// store the isolated commands in command_list
			strcpy(command_list[i],sep_str);
			i++;
		}
		command_list[i]=NULL;

		// set retval=2 for sequential execution of multiple commands
		retval = 2;					
	}
	else if(strstr(duplicate, ">") != NULL)
	{
		// isolate each command seperated by delimeter > 
		int i = 0;
		char* sep_str;
		while((sep_str = strsep(&duplicate,">")) != NULL )
		{
			int count = 0;


			strcpy(command_list[i],sep_str);
			i++;
		}
		command_list[i]=NULL;

		// set retval=3 for command redirection
		retval = 3;					
	}
	else
	{
		// pass the entire command to command list in case of no delimiter
		char* token = duplicate;

		command_list[0]=token;
		command_list[1]=NULL;

		//set retval=4 for  single command execution
		retval = 4;					
	}

	return retval;
}

/* This function will break the isolated command strings into their constituent command
and command arguments, and return the array of strings */
char** get_commandArgs(char* command)
{
	//remove new line character from the string
	command = strsep(&command, "\n");		

	// remove whitespaces before a command if there are present
	while(*command == ' ')					
		command++;
	
	// create duplicate command string
	char* dup_command = strdup(command);

	// get the command in a string, to check for "cd"
	char* first_sep = strsep(&command, " ");

	// change directory is "cd" is encountered
	if (strcmp(first_sep,"cd")==0)
	{
		chdir(strsep(&command, "\n "));
	}
	else
	{
		// allocate memory to the array of strings, to store the command its arguments
		char** command_args;
		command_args = (char**)malloc(sizeof(char*)*10);
		for(int i=0; i<10; i++)
			command_args[i] = (char*)malloc(50*sizeof(char));

		char* sep_str;
		int i = 0;

		//if there's a string with spaces, else directly assign the string to command_args[i]
		if (strstr(dup_command, " ")!=NULL)
		{	
			// iterate over the space seperated arguments
			while ((sep_str = strsep(&dup_command," ")) != NULL)					
			{
				// remove whitespaces before a command if there are present
				while(*sep_str == ' ')					
					sep_str++;
				command_args[i] = strsep(&sep_str, " ");	
				i++;
			}
			
			// if current string is an empty string, assign NULL
			if(strlen(command_args[i-1])==0)
				command_args[i-1] = NULL;
			else
				command_args[i] = NULL;

			i=0;
			return command_args;
		}
		else
		{
			// assign the entire command in case of no spaces
			strcpy(command_args[0], dup_command);
			command_args[1] = NULL;
			return command_args;
		}
		
	}
	return NULL;
}

// This function will fork a new process to execute a command
void executeCommand(char** command_list)
{

	// get the command and arg list from given command
	char** command_args = get_commandArgs(command_list[0]);

	// fork process
	pid_t pid = fork();

	//child Process
	if (pid==0)          
	{
		// execute the command and it's args in child
		int retval = execvp(command_args[0], command_args);

		 //execvp error code
		if (retval < 0)     
		{
			printf("Shell: Incorrect command\n");
			exit(1);
		}
	} 

	// Parent Process
	else if(pid)
	{
		//wait for the child to terminate
		wait(NULL);
	}

	// if fork fails, then exit
	else
		exit(0);
}

// This function will run a single command with output redirected to an output file specificed by user
void executeCommandRedirection(char** command_list)
{
	// get the command and arg list from given command
	char* command = command_list[0];

	// getting the string that contains filename
	char* file_name = command_list[1]; 

	// remove whitespaces before a filename if there are present
	while(*file_name == ' ')			
		file_name++;

	// Close stdout
	

	// open given file, or create if not found one
	int filefd = open(file_name, O_WRONLY|O_CREAT|O_APPEND, 0666);	
	
	// fork process
	int pid = fork();

	// child Process
	if (pid ==0) 
	{
		close(STDOUT_FILENO);
		dup(filefd);
		// get command args to execute
		char** command_args = get_commandArgs(command);
		int retval = execvp(command_args[0], command_args);

		//execvp error code
		if (retval < 0)      
		{
			printf("Shell: Incorrect command\n");
			exit(1);
		}
	} 

	// Parent Process
	else if(pid)
	{
		close(filefd);
		int t=wait(NULL);
	}

	// if fork fails, then exit
	else
		exit(0);
}

// This function will run multiple commands in parallel
void executeParallelCommands(char** command_list)
{
	int i = 0;
	while(command_list[i]!=NULL)
	{
		char* dup_command = strdup(command_list[i]);
		if (get_commandArgs(dup_command) == NULL)
		{
			i++;
			continue;
		}
		else if(strlen(command_list[i])>0)
		{
			// fork process
			int pid = fork();
			if (pid == 0)	
			{
				
				//we have to enable signals again for child processes	
				signal(SIGINT, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);	

				// get the command and arg list from given command
				char** command_args = get_commandArgs(command_list[i]);

				int return_val = execvp(command_args[0], command_args);

				//execvp error code
				if (return_val < 0)					
				{	 
					printf("Shell: Incorrect command\n");
					exit(1);
				}
				exit(0);
			}
			// Parent Process
			else if (pid)
			{
				//no wait statement here as it needs to run parallely
				//parent process
				i++;			
				if(command_list[i+1]!=NULL)
					wait(NULL);
				else
					continue;	
			}
			// if fork fails, then exit
			else
			{ 
				exit(1);	
			}
		}
		else
		{
			i++;
		}
					
	}
}

// This function will run multiple commands in parallel
void executeSequentialCommands(char** command_list)
{	

	int i = 0;
	while(command_list[i]!=NULL)
	{	
		// get the command and arg list from given command
		char* dup_command = strdup(command_list[i]);

		// special case for "cd"
		if(strstr(dup_command,"cd")!=NULL)
		{
			get_commandArgs(command_list[i]);
			i++;
		}
		// case where there's empty string
		else if(strlen(command_list[i])>0)
		{

			int pid = fork(); //making child process

			// child process
			if (pid == 0)		
			{
				//we have to enable signals again for child processes
				signal(SIGINT, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);		

				// parsing each command
				char** command_args = get_commandArgs(command_list[i]); 
				int return_val = execvp(command_args[0], command_args);
			
				//execvp error code
				if (return_val < 0)					
				{	 
					printf("Shell: Incorrect command\n");
					exit(1);
				}	
	
			}

			// Parent Process
			else if (pid)
			{
				// we have to enable signals again for child processes
				wait(NULL);
				i++;
			}	
			else
			{ 
				exit(1);
			}
		}
		else
		{
			i++;
		}
		
	}
}

// Signal Handler for SIGINT (CTRL + C)
void sigintHandler(int sig_num)
{
    /* Reset handler to catch SIGINT next time. */
    signal(SIGINT, sigintHandler);
    printf("\n Cannot be terminated using Ctrl+C, enter 'exit' \n");
}

// Signal Handler for SIGTSTP (CTRL + Z)
void sighandler(int sig_num)
{
    // Reset handler to catch SIGTSTP next time
    signal(SIGTSTP, sighandler);
    printf("\n Cannot be terminated using Ctrl+Z, enter 'exit'\n");      
}

int main()
{
	// Initial declarations
	char* str=NULL;
	size_t str_len = 64;

	signal(SIGINT, sigintHandler);
	signal(SIGTSTP, sighandler);
	
	// This loop will keep your shell running until user exits.
	while(1)	
	{
		char curr_dir[64];

		// Print the prompt in format - currentWorkingDirectory$
		getcwd(curr_dir, 64);
		
		printf("%s$",curr_dir);          

		// accept input with 'getline()'
		getline(&str,&str_len,stdin);   

		// allocate memory for the list of strings to strore isolated
		// commands, seperated by delimiters
		char** command_list;
		command_list = (char**)malloc(sizeof(char*)*10);
		for(int i=0; i<10; i++)
			command_list[i] = (char*)malloc(50*sizeof(char));

		// When user uses exit command		
		if(strcmp(str, "exit\n")==0)
		{
			printf("Exiting shell...\n");
			break;
		}

		// Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
		int ret = parseInput(str, command_list); 
	
		if(ret==1)
			executeParallelCommands(command_list);		// This function is invoked when user wants to run multiple commands in parallel (commands separated by &&)
		else if(ret==2)
			executeSequentialCommands(command_list);	// This function is invoked when user wants to run multiple commands sequentially (commands separated by ##)
		else if(ret==3)
			executeCommandRedirection(command_list);	// This function is invoked when user wants redirect output of a single command to and output file specificed by user
		else
			executeCommand(command_list);		// This function is invoked when user wants to run a single commands
				
	}
	
	return 0;
}
// including necessary libraries
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>

char last_executed_command[1024] = "";
void execute_command(char *command);
#define TEMP_FILE "temp_aliases.txt" // used in handle_alias function
int is_command_available(const char *command);
#define ALIAS_FILE "aliases.txt" // to keep new aliases and save old ones

// function that trims leading and trailing whitespaces (spaces and tabs) from a given string
char *strtrim(char *str) {
    // Trim leading whitespaces
    while (*str && (*str == ' ' || *str == '\t')) {
        str++;
    }
    // Trim trailing whitespaces
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t')) {
        end--;
    }
    // Null-terminate the trimmed string
    *(end + 1) = '\0';
    return str;
}

/* when the command is in "alias sth = command" form, I call the below function. 
It takes sth and if it exist in the aliases.txt file, then it overwrites the command.
if it does not exist in aliases.txt file, then it create a new line and put this new alias inside aliases.txt file
*/
void handle_alias(char *command) {
    // Check if the input starts with "alias"
    if (strncmp(command, "alias", 5) == 0) {
        // If it does, remove "alias" and trim the command
        command += 5;
        command = strtrim(command);
    }

    // Make a copy of the command string to avoid modification
    char command_copy[1024];
    strcpy(command_copy, command);

    // Find the position of the equal sign in the command
    char *equals_sign = strchr(command_copy, '=');

    // Check if there is an equal sign in the command
    if (equals_sign == NULL) {
        fprintf(stderr, "Invalid alias syntax: %s\n", command);
        return;
    }

    // Null-terminate the alias at the equal sign to get the alias name
    *equals_sign = '\0';
    char *alias = strtrim(command_copy);

    // Parse the rest of the line as the alias command
   char *alias_command = equals_sign + 1;

	// Remove leading and trailing whitespaces from the alias command
	alias_command = strtrim(alias_command);

	// Remove quotes from the right part of the alias command if they exist
	char *quote_start = strchr(alias_command, '"');
	if (quote_start != NULL) {
		char *quote_end = strrchr(quote_start + 1, '"');
		if (quote_end != NULL) {
		    // Remove the first and last quotation marks
		    memmove(quote_start, quote_start + 1, quote_end - quote_start);
		    quote_end[-1] = '\0'; // Null-terminate the string at the closing quote
		}
	}

	// Open the alias file in read-write mode
	FILE *alias_file_rw = fopen(ALIAS_FILE, "r");
	if (alias_file_rw == NULL) {
		perror("Error opening alias file for read");
		exit(EXIT_FAILURE);
	}

    // Open a temporary file for writing
    FILE *temp_file = fopen(TEMP_FILE, "w");
    if (temp_file == NULL) {
        perror("Error opening temporary file for write");
        exit(EXIT_FAILURE);
    }

    // Check if the alias already exists in the file
    bool alias_exists = false;
    char line[1024];

    while (fgets(line, sizeof(line), alias_file_rw) != NULL) {
        // Check if the line starts with the alias
        if (strncmp(line, alias, strlen(alias)) == 0 && line[strlen(alias)] == '=') {
            alias_exists = true;
            // Overwrite the existing alias with the new command
            fprintf(temp_file, "%s=%s\n", alias, alias_command);
        } else {
            // Copy the line as it is to the temporary file
            fprintf(temp_file, "%s", line);
        }
    }

    if (!alias_exists) {
        // If the alias doesn't exist, append the new alias to the file
        fprintf(temp_file, "%s=%s\n", alias, alias_command);
    }
    fclose(alias_file_rw);
    fclose(temp_file);

    // Replace the original file with the temporary one
    if (rename(TEMP_FILE, ALIAS_FILE) != 0) {
        perror("Error replacing alias file");
        exit(EXIT_FAILURE);
    }
}


/* this is called when command starts with bello
it  gathers and displays various system and user-related information.
*/
void bello(char* last) {
    char username[256];
    char hostname[256];
    char last_executed_command[1024];
    char tty[256];
    char current_shell_name[256];
    char home_location[1024];
    time_t current_time;
    struct tm *tm_info;
    int num_processes = 0;
    char *shell_path;
    // Get the value of the SHELL environment variable
    shell_path = getenv("SHELL");
    char *shell_name = basename(shell_path);
    // 1. Username
    if (getlogin_r(username, sizeof(username)) != 0) {
        perror("Error getting username");
        exit(EXIT_FAILURE);
    }
    // 2. Hostname
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("Error getting hostname");
        exit(EXIT_FAILURE);
    }
    // 4. TTY
    if (ttyname_r(STDIN_FILENO, tty, sizeof(tty)) != 0) {
        perror("Error getting TTY");
        exit(EXIT_FAILURE);
   }
    // 5. Current Shell Name
    strcpy(current_shell_name, shell_name);
    // 6. Home Location
    if (getenv("HOME") != NULL) {
        strcpy(home_location, getenv("HOME"));
    } else {
        perror("Error getting home location");
        exit(EXIT_FAILURE);
    }
    // 7. Current Time and Date
    time(&current_time);
    tm_info = localtime(&current_time);
    // 8. Current number of processes being executed
    FILE *processes = popen("ps | wc -l", "r");
    if (processes != NULL) {
        fscanf(processes, "%d", &num_processes);
        pclose(processes);
    } else {
        perror("Error counting processes");
        exit(EXIT_FAILURE);
    }
    // Display the information
    printf("1. Username: %s\n", username);
    printf("2. Hostname: %s\n", hostname);
    
    // 3. Last Executed Command (I prefer to apply 3rd one here)
    char command[1024];
    strncpy(command, last, sizeof(command) - 1);
    command[sizeof(command) - 1] = '\0';

    // Extract the command without arguments
    char *token = strtok(command, " ");
    if (token != NULL) {
        printf("3. Last Executed Command: %s\n", token);
    } else {
        printf("3. Last Executed Command: (empty)\n");
    }
    printf("4. TTY: %s\n", tty);
    printf("5. Current Shell Name: %s\n", current_shell_name);
    printf("6. Home Location: %s\n", home_location);
    printf("7. Current Time and Date: %s", asctime(tm_info));
    printf("8. Current number of processes: %d\n", num_processes);
}

/*this is used for the triple redirection case. 
It takes input and parse the left part of >>>, and put the left part into left_part*/
void extract_left_part(const char *input, char *left_part) {
    const char *redirect_operator = " >>> ";
    const char *operator_position = strstr(input, redirect_operator);

    if (operator_position != NULL) {
        size_t operator_length = strlen(redirect_operator);
        size_t left_part_length = operator_position - input;

        if (left_part_length > 0) {
            strncpy(left_part, input, left_part_length);
            left_part[left_part_length] = '\0';
        } else {
            // Handle the case where the left part is empty
            left_part[0] = '\0';
        }
    } else {
        // Handle the case where the redirect operator is not found
        strcpy(left_part, input);
    }
}


// this helps to execute whole command. In here I handle fork exec mechanism
void execute_command(char *command) {
    // Check if the command should run in the background
    bool run_in_background = false;
    if (strlen(command) > 0 && command[strlen(command) - 1] == '&') {
        run_in_background = true;
        command[strlen(command) - 1] = '\0'; // Remove the '&' from the command
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("Error forking process");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Child process
        // Parse the command and arguments
        
        char command_copy1[1024];
    	strcpy(command_copy1, command);
        char *args[1024];
        char left_part[1024];
        char *token = strtok(command, " ");
        int i = 0;
        
        // tokenization
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        
        
        args[i] = NULL; // Null-terminate the argument array
        // Check for redirection and handle it
		for (int j = 0; args[j] != NULL; ++j) {
			if (strcmp(args[j], ">") == 0 || strcmp(args[j], ">>") == 0 || strcmp(args[j], ">>>") == 0) {
			// Handle redirection based on the operator
				if (strcmp(args[j], ">>>") == 0) {
					// Handle ">>>" re-redirection operator
					if (args[j + 1] == NULL) {
						fprintf(stderr, "Error: No output file specified after '>>>'\n");
						exit(EXIT_FAILURE);
					}

					// Open the file for writing
					int fd = open(args[j + 1], O_WRONLY | O_CREAT | O_APPEND, 0666);
					if (fd == -1) {
						perror("Error opening output file");
						exit(EXIT_FAILURE);
					}

					// Redirect standard output to the output file
					dup2(fd, STDOUT_FILENO);
					close(fd);

					// Execute the left part of ">>>" using popen
					extract_left_part(command_copy1, left_part);
					FILE *command_output = popen(left_part, "r");
					if (command_output == NULL) {
						perror("Error executing command");
						exit(EXIT_FAILURE);
					}

					// Read lines from the command output and store them in an array
					char *lines[1024];
					int line_count = 0;
					char line[1024];

					while (fgets(line, sizeof(line), command_output) != NULL) {
						// Reverse the line before storing it in the array
						size_t len = strlen(line);
						for (size_t i = 0, j = len - 1; i < j; ++i, --j) {
							char temp = line[i];
							line[i] = line[j];
							line[j] = temp;
						}
						lines[line_count++] = strdup(line);
					}
					// Close the command output
					pclose(command_output);
					// Write lines to the output file
					for (int k = line_count - 1; k >= 0; --k) {
						dprintf(STDOUT_FILENO, "%s", lines[k]); // Write directly to stdout
						free(lines[k]);  // Free memory allocated by strdup
					}
					// Exit the child process
					exit(EXIT_SUCCESS);
			}

				else {
				    if (strcmp(args[j], ">") == 0 || strcmp(args[j], ">>") == 0) {
						int fd;
						if (strcmp(args[j], ">") == 0) {
						    fd = open(args[j + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
						} else {
						    fd = open(args[j + 1], O_WRONLY | O_CREAT | O_APPEND, 0666);
						}
						if (fd == -1) {
						    perror("Error opening output file");
						    exit(EXIT_FAILURE);
						}
						dup2(fd, STDOUT_FILENO);
						close(fd);
						// Remove the redirection symbols and the filename from the arguments
						for (int k = j; args[k] != NULL; ++k) {
						    args[k] = args[k + 2];
						}
						break;
					}
				}
			}
		}
		// since bello may have background process, I put it here
		if(strcmp(command, "bello") == 0){
        	bello(last_executed_command);
        	exit(EXIT_SUCCESS);
        }
        // Execute the command
        if (execvp(args[0], args) == -1) {
            perror("Execution failed");
            exit(EXIT_FAILURE);
        }
     }else { // Parent process
        if (!run_in_background) {
            int status;
            waitpid(pid, &status, 0);
            if (WIFEXITED(status)) {
                int exit_status = WEXITSTATUS(status);
                if (exit_status == 0) {
                    // Update the last executed command for successful execution
                    strncpy(last_executed_command, command, sizeof(last_executed_command) - 1);
                    last_executed_command[sizeof(last_executed_command) - 1] = '\0';
                }
                // to determine the error
                printf("\nChild process exited with status %d\n", exit_status);
            }
        }
    }
}
// Function to display the shell prompt that you want in description
void display_prompt() {
    char username[256];
    char hostname[256];
    char cwd[1024];
    // Get the machine (hostname) name
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("Error getting hostname");
        exit(EXIT_FAILURE);
    }
    // Get the current user name
    if (getlogin_r(username, sizeof(username)) != 0) {
        perror("Error getting username");
        exit(EXIT_FAILURE);
    }
    // Get the current working directory
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Error getting current directory");
        exit(EXIT_FAILURE);
    }
    // Get the current time and date
    time_t t;
    struct tm *tm_info;
    time(&t);
    tm_info = localtime(&t);
    // Display the prompt
    printf("%s@%s %s --- ", username, hostname, cwd);
}

// split PATH from : and iterate whole path parts to check whether the command inside one of them or not
int is_command_available(const char *command) {
    // Extract the first word from the command
    char first_word[1024];
    sscanf(command, "%s", first_word);

    // Check if the first word is available in the system's PATH
    char which_command[2 * PATH_MAX];  // Using a larger buffer
    snprintf(which_command, sizeof(which_command), "which %s", first_word);

    FILE *which_pipe = popen(which_command, "r");

    if (which_pipe != NULL) {
        char path[1024];
        if (fgets(path, sizeof(path), which_pipe) != NULL) {
            pclose(which_pipe);
            // Tokenize the path using ':' as the delimiter
            char *token = strtok(path, ":");
            while (token != NULL) {
                // Trim leading and trailing whitespaces from the token
                char trimmed_token[1024];
                sscanf(token, " %1023[^\n\t ] ", trimmed_token);
                // Check if the command exists in the current path
                if (access(trimmed_token, X_OK) == 0) {
                    return 1; // Command found in one of the paths
                }
                token = strtok(NULL, ":");
            }
            return 0; // Command not found in any of the paths
        }
        pclose(which_pipe);
    }
    return 0; // Command not found
}

// determine which function should be executed for different type of commands
void process_user_input(char *input) {
	char command[1024];
    sscanf(input, "%s", command);
	bool flag1 = false;
	// Search for the alias in the file and execute the corresponding command
    FILE *alias_file = fopen(ALIAS_FILE, "r");
    if (alias_file == NULL) {
        perror("Error opening alias file");
        exit(EXIT_FAILURE);
    }

    char line[1024];
    while (fgets(line, sizeof(line), alias_file) != NULL) {
        // Check if the line has the correct format
        if (strstr(line, "=") != NULL) {
            char *alias = strtok(line, "=");
            char *alias_command = strtok(NULL, "=");

            // Remove newline characters
            alias_command[strcspn(alias_command, "\n")] = '\0';

            // Check if the entered command matches the alias
            if (strcmp(command, alias) == 0) {
                // If the input contains additional arguments, append them to the alias command
                if (strlen(input) > strlen(alias)) {
                    strcat(alias_command, input + strlen(alias));
                }
                // ifit exist in aliases file, convert flag to true and execute command
                execute_command(alias_command);
                flag1 = true;
                break;
            }
        }
    }
    fclose(alias_file);
	
	// code enters here if the command does not exis in aliases.txt file
	if(!flag1){
		if (strcmp(input, "exit") == 0) {
		    // Exit the shell
		    printf("Shell terminated.\n");
		    exit(EXIT_SUCCESS);
		} else if (strncmp(input, "bello", 5) == 0) {
		    // Execute bello command
		    execute_command(input);
		} else if (strncmp(input, "alias", 5) == 0) {
		    // Handle alias command
		    handle_alias(input);
		} else if (is_command_available(input)) { // if it is not bello, exit or alias then I check whether it is inside one of the PATH parts
		    // Execute the command directly
		    execute_command(input);
		} else { // if it is not inside one of the PATH parts, then command does not exist inside one of PATH parts
		    printf("Command not found\n");
		}
    }
}

// program starts here
int main() {
	// I accept that the length of the command should be at most 1024 character
    char input[1024];
    FILE *alias_file = fopen(ALIAS_FILE, "a");
    if (alias_file == NULL) {
        perror("Error creating alias file");
        exit(EXIT_FAILURE);
    }
    fclose(alias_file);
    // until get an exception or facing exit command, the shell runs
    while (1) {
         display_prompt();
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        // Remove trailing newline character
        input[strcspn(input, "\n")] = '\0';
        // Trim leading and trailing whitespaces
        char *trimmed_input = strtrim(input);
        // Process user input based on the provided steps
        process_user_input(trimmed_input);
    }
    return 0;
}

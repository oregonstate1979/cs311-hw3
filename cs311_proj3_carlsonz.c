#define _POSIX_C_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>

#define SORTERS	5

/*
 * Reads from standard in and writes to a pipe, read by the suppressor
 */
void read_and_parse(int sorters, int **sorter_input, int cur_stdin);

/* 
 * Suppresses duplicates and writes to standard output
 */
void suppressor(int sorters,int **sorter_output);

/*
 * converts string to lowercase
 */
static char *to_lower (char *str);

int main(int argc, char *argv[]){
	int sorters;		// stores no of sorters given from input. By default, set to 5.
	int **sorter_input;		// fd array btw parser's output and sorter's input
	int **sorter_output;		// fd array btw sorter's output and suppressor's input
	int i;					
	int sorter_input_status;	// variable to hold the status when pipes are created
	int sorter_output_status;	// variable to hold the status when pipes are created

	int cur_stdin = dup(STDIN_FILENO);
	int cur_stdout = dup(STDOUT_FILENO);
	
	if(argc > 1 &&  strcmp(argv[1], "--help") == 0){
		printf("Usage: %s [number of sorters <0-13>] < <fileName>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	else if(argc >= 2){
		sorters = atoi(argv[1]);
	}
	else{
		sorters = SORTERS;
	}
	/* 
	 * sorter_input is for the parent to write parser output to and child sorter to read from 
	 * sorter_output is for the child sorter to write to and suppressor to read from
	 */
	sorter_input = (int**)malloc((sorters)*sizeof(int*));
	sorter_output = (int**)malloc((sorters)*sizeof(int*));
	
	if(sorter_input == NULL){
		fputs("Not able to allocate memory\n", stderr);
		exit(EXIT_FAILURE);
	}
	if(sorter_output == NULL){
		fputs("Not able to allocate memory\n", stderr);
		exit(EXIT_FAILURE);
	}

	for(i = 0; i < sorters ; i++){
		sorter_input[i] = malloc(2 * sizeof(int));
		sorter_output[i] = malloc(2 * sizeof(int)); 
		if(sorter_input[i] == NULL){
			fputs("Not able to allocate memory\n", stderr);
			exit(EXIT_FAILURE);
		}
		if(sorter_output[i] == NULL){
			fputs("Not able to allocate memory\n", stderr);
			exit(EXIT_FAILURE);
		}
	}
	//create sorter children
	for (i = 0; i < sorters; i++){
		/*
		 * create pipes to enable communication between parent(parser) and the children(sorter)
		 */
		sorter_input_status = pipe(sorter_input[i]);
		sorter_output_status = pipe(sorter_output[i]);

		if(sorter_input_status == -1){
			fputs("Error occured while creating pipes\n",stderr);
			exit(EXIT_FAILURE);
		}
		if(sorter_output_status == -1){
			fputs("Error occured while creating pipes\n",stderr);
			exit(EXIT_FAILURE);
		}

		switch(fork()){
			case -1:
				fputs("Error while trying create child sort processes\n",stderr);
				exit(EXIT_FAILURE);
			case 0:
				/*
				 * child sorter processes are just going to read from the pipe. so 
				 * close the write fd;
				 */ 
				if(close(sorter_input[i][1]) == -1){
					fputs("Error while closing sorter_input's write end in child\n",stderr);
					printf("errno: %d",errno);
					exit(EXIT_FAILURE);
				}
				/* 
				 * child sorters are not going to read from sorter_output's in. so close it
				 */
				if(close(sorter_output[i][0]) == -1){
					fputs("Error while closing sorter_output's read end in child\n",stderr);
					exit(EXIT_FAILURE);
				}
				/*
				 * child processes are going to	read from the parent. so set pipe out as sort
				 * program's input. after duping, close the original fd
				 */
				if(sorter_input[i][0] != STDIN_FILENO){
					if(dup2(sorter_input[i][0],STDIN_FILENO) == -1){
						fputs("Error while trying to set sort's stdin to the pipe\n",stderr);
						exit(EXIT_FAILURE);
					}
					if(close(sorter_input[i][0]) == -1){
						fputs("Error while trying to close sort's in\n",stderr);
						exit(EXIT_FAILURE);
					}
				}
				/*
				 * child process will be writing to sorter_output in, so set sorter_output fd as sort program's
				 *  stdout. after duping, close the original fd
				 */
				if(sorter_output[i][1] != STDOUT_FILENO){
					if(dup2(sorter_output[i][1],STDOUT_FILENO) == -1){
						fputs("Error while trying to set sort's stdout to the pipe\n",stderr);
						exit(EXIT_FAILURE); 
					}
					if(close(sorter_output[i][1]) == -1){
						fputs("Error while trying to close sort's out\n",stderr);
						exit(EXIT_FAILURE);
					}
				}
			execlp("sort","sorting begins", (char *)NULL);
			default:
				break;
		}
		/*
		* parent process is going to write into the pipe so close the read fd
		*/
		if(close(sorter_input[i][0]) == -1){
			fputs("Error while closing sorter_input's read end in parent\n",stderr);
			exit(EXIT_FAILURE);
		}
		if(close(sorter_output[i][1]) == -1){
			fputs("Error while closing sorter_output's write end in parent\n",stderr);
			exit(EXIT_FAILURE);
		}
	}

	//fork to create the suppressor
	switch(fork()){
	case -1:
		exit(EXIT_FAILURE);
	case 0:
		for(i = 0; i < sorters; i++){
			if(close(sorter_input[i][1]) == -1){
				fputs("Error while closing file descriptor in parent\n",stderr);
				exit(EXIT_FAILURE);
			}
		}
		suppressor(sorters,sorter_output);
		//fputs("suprressor is exiting now\n",stdout);
		exit(EXIT_SUCCESS);
	default:
		break;
	}

	for(i = 0; i < sorters; i++){
		if(close(sorter_output[i][0]) == -1){
			fputs("Error while closing file descriptor in parent\n",stderr);
			exit(EXIT_FAILURE);
		}
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	dup2(cur_stdin,STDIN_FILENO);
	 dup2(cur_stdout,STDOUT_FILENO);

	read_and_parse(sorters, sorter_input, cur_stdin);

	 for (i = 0; i < sorters; i++){
		wait(0);
	 }

	 free(sorter_input);
	 free(sorter_output);
	 exit(EXIT_SUCCESS);
}

void read_and_parse(int sorters, int **sorter_input, int cur_stdin){
	char buffer[200];		
	int i = 0;				
	
	FILE *stdin_stream;		//stream for reading standard input
	
	stdin_stream = fdopen(cur_stdin, "r");
	if(stdin_stream == NULL){
		fputs("Error while opeing standard input to read \n",stderr);
		exit(EXIT_FAILURE);
	}

	FILE **suppressor_in_streams;		//stream array to read the output of the sorters
	
	suppressor_in_streams = (FILE**)malloc((sorters)*sizeof(FILE*));
	if( suppressor_in_streams == NULL){
		fputs("Not able to allocate memory\n", stderr);
		exit(EXIT_FAILURE);
	}
	//create streams to read from sorter's output
	for (i = 0; i < sorters; i++){
		suppressor_in_streams[i] = fdopen(sorter_input[i][1],"w");
		if(suppressor_in_streams[i] == NULL){
			fputs("Error while opening sorter_input write end\n",stderr);
			exit(EXIT_FAILURE);
		}
	}
	/*
	 * read one word at a time. words are delimited by any non-alphabetic character.
	 */
	while(fscanf(stdin_stream, "%*[^A-Za-z]"), fscanf(stdin_stream, "%198[a-zA-Z]", buffer) > 0) {
		//reset if counter has reached the last of the round robin distribution
		if(i > sorters-1){
			i = 0;
		}
		strcat(buffer,"\n");
		fputs(to_lower(buffer), suppressor_in_streams[i]);
		i++;
	}
	fclose(stdin_stream);
	for(i = 0; i < sorters; i++){
		if(fclose(suppressor_in_streams[i]) < 0){
			fputs("Error while closing suppressor input streams\n",stderr);
		}
	}
	  
	free(suppressor_in_streams);
	return;
}

void suppressor(int sorters, int **sorter_output){
	int numbers_string = 4096;
	int index = 0;
	int i = 0;						
	int j = 0;						
	int k = 0;						
	int updateIndex = 0;
	int exists = 0;					// flag to set when match is present
	char temp[200];
	char next_word[200];			// holds the current word read
	char **words;					// final array that holds all the sorted and unique words
	
	words = malloc(numbers_string*sizeof(char*));
	if (words == NULL){
		fputs("Not able to allocate memory\n", stderr);
		exit(EXIT_FAILURE);
	}

	FILE **suppressor_in_streams = (FILE**)malloc((sorters)*sizeof(FILE*));
	if(suppressor_in_streams == NULL){
		fputs("Not able to allocate memory\n", stderr);
		exit(EXIT_FAILURE);
	}
	// open the stream for sorter output in read mode
	for(i = 0; i < sorters; i++){
		suppressor_in_streams[i] = fdopen(sorter_output[i][0],"r");
	}

	for (i = 0; i < sorters; i++){
		while(fgets(next_word,199,suppressor_in_streams[i]) != 0){
			updateIndex = 0;
			exists = 0;
			/* 
			 * words array is full. reallocate and expand its
			 * capacity
			 */
			if(index >= numbers_string){
				numbers_string = numbers_string + 4096;
				words = realloc(words, numbers_string);
			}
			// allocate memory for a new word
			words[index] = (char*)malloc(199*sizeof(char));
			if (words[index] == NULL){
				fputs("Not able to allocate memory\n", stderr);
				exit(EXIT_FAILURE);
			}
			//copy the first word from the first stream to the array.
			if(index == 0){
				strcpy(words[index++], next_word);
			}
			else{
				/* 
				 * loop through the words array. if the word already exists
				 * set exists to 1.
				 */
				for( j = 0; j < index; j++){
					if(strcmp(next_word, words[j]) == 0){
						exists = 1;
					}
				}
				// only if exists is equal to zero, proceed further
				if(exists == 0 && strcmp(next_word,words[index-1]) != 0){
					strcpy(words[index],next_word);
					if(strcmp(words[index], words[index-1]) < 0){
						for(k = index; k > 0; k--){
							if(strcmp(words[k], words[k-1]) < 0){
								strcpy(temp,words[k-1]);
								strcpy(words[k-1],words[k]);
								strcpy(words[k],temp);
								updateIndex = 1;
							}else if(strcmp(words[k],words[k-1]) == 0){
								k=0;
								updateIndex = 0;
							}
						}
					}else if(exists == 0 && strcmp(next_word,words[index-1]) > 0){
						strcpy(words[index++],next_word);
					}
					if(updateIndex == 1){
						index++;
						updateIndex = 0;
					}
				}
			}
		}
		
	}
	//all data from all sorters are read. close the streams
	for(i = 0; i < sorters; i++){
		fclose(suppressor_in_streams[i]);
	}
	//print words array which is the output to standard out
	for(i = 0; i < index; i++){
		printf("%s",words[i]);
	}
	free(words);
	return;
}

static char *to_lower (char *str){
	char *s = str;
	while (*s){
		if (isupper (*s))
		*s = tolower (*s);
		s++;
	}
	return str;
}

/*
 *reads and parses.
 * **sorter_in will be file descriptor array between parse output and sort input
 * cur_stdin allows duplication of a file descriptor
 */

void parse(int no_of_sorters, int **sorter_in, int cur_stdin){
	char buffer[200];	//For each word
	int i = 0;				

	FILE *stdin_stream;		//stream for reading standard input

	stdin_stream = fdopen(cur_stdin, "r");

	FILE **suppressor_in_streams;		//stream array to read the output of the sorters

	suppressor_in_streams = (FILE**)malloc((no_of_sorters)*sizeof(FILE*));

	//create streams to read from sorter's output
	for (i = 0; i < no_of_sorters; i++){
		suppressor_in_streams[i] = fdopen(sorter_in[i][1],"w");
	}
	/*
	 * read one word at a time. Words are delimited by any non-alphabetic character and fed to the suppressor in a round
	 * robin fashion.
	 */
	while(fscanf(stdin_stream, "%*[^A-Za-z]"), fscanf(stdin_stream, "%198[a-zA-Z]", buffer) > 0) {
		//reset if counter has reached the last of the round robin distribution
		if(i > no_of_sorters - 1)
			i = 0;
		strcat(buffer,"\n");
		fputs(to_lower(buffer), suppressor_in_streams[i]);
		i++;
	}
	fclose(stdin_stream);1
  free(suppressor_in_streams);
	return;
}

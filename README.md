cs311-hw3
=========
This program will satisfy the requirement given in class of the following (copied and pasted from the class website):

For this project, you will be implementing a text mining system. Specifically, you will be using the term frequency-inverse
document frequency (tf-idf) statistic. You will be reading a corpus of documents, and building a sytem which can be used to find documents with a given term or terms.

There are 2 main pieces of this: the scoring system and the searching system.

Internally, the scoring program would be organized into 3 types of processes. A single process reads the input parsing the 
lines into words, another group of process does the scoring, and a single process combines the scores from the middle stage
and writes the output. You should organize the processes so that every parent waits for its children.

Your program must arrange to start the processes and plumb the pipes (this means no popen). The number of scoring processes
is a command line argument to the program, with the parser distributing the words round robin to the scoring processes 
(just like dealing cards).

The middle set of processes is where the actual scoring will occur. Each of these processes will take the words fed it, and
keep a running total of how often it has seen the word. When complete, it will output each word with its count to the 
combine process, which will sum all common word totals, as well as keep track of the total number of words. It will then 
output all words with a total count from that document.

Finally, you will write a program that searches for a given term in a corpus of files. This program will make use of the 
tfidf program as described above to generate term frequency scores for every file it is given -- one tfidf process per 
file, up to a total of 8 processes (more files will simply have to wait). It will then weight the term frequency by the 
inverse document frequency by dividing the total number of documents by the number of documents containing the term, and 
then taking the logarithm of that quotient. In this fashion, you will weight common terms very lightly and rare terms very
heavily, meaning rare terms will be easier to find.

The output of this program is the list of files which contain the term being searched for. Again, there is a frontend 
searching program, which makes use of a scoring program, which itself makes use of multiple children to do the actual 
scoring.

The I/O to and from the pipes should be done using the stream functions fgets and fputs, with fdopen for attaching to the
pipes, and fclose for flushing the streams. After each scoring process is done, work must still be done to merge the 
scores of the words prior to printing the list from the suppresser.

In this assignment words are all alphabetic and case insensitive, with the parser converting all alphabetic characters to
lower case. Any non-alphabetic characters other than apostraphes and hyphens delimit words and are discarded.

All processes should terminate cleanly on INTR, QUIT, and HANGUP signals. This requires the installation of a signal 
handler for each of these three signals. Ensure you do this via sigaction, rather than signal. Also, ensure you issue 
QUIT signals to all children, as well.

You are responsible for cleaning up any mess you make on the server. I do not want to see anyone with 1000 processes, all
of which are hanging.

You must use getopt and reasonable options for setting the number of sorting processes, the name of the input file, and 
the name of the output file. If no input or output files are specified, read and write via standard output.

Please provide timings based on the total size and number of input files. Ensure that you test it with multiple types and
sizes of files, including word lists and free form prose. These timings should be plotted in a fashion that makes sense. 
I recommend using one of matlab, gnuplot, R, or octave to generate the plot. Please ensure to save the plot as a 
postscript file.

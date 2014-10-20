#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* For char array initialization */
#define MAX_LINE_SIZE 4096

/* Used for simple double array cache */
static const int LBYTES = 2;
static const int TBYTES = 4;
static const int LGETS = 8;
static const int TGETS = 16;
static const int FGETS = 32;
static const int LFGETS = 64;

/* Used to manage fields on parsing */
static const int ADDRESS_FIELD = 0;
static const int DATE_FIELD = 3;
static const int HTTP_STATUS_CODE_FIELD = 8;
static const int BYTES_DOWNLOADED_FIELD = 9;

/* Used as delimeter for line parsing */
static const char *DELIMETER = " []\"";

/* Struct to hold webstats data */
struct stats {
	double local_bytes;
	double total_bytes;
	double local_gets;
	double total_gets;
	double failed_gets;
	double local_failed_gets;
	pthread_mutex_t mutex;
} webstats;

/*
 * parse_line(): Parse one line into string tokens separated by the given delimiters.
 * Beware: The input argument line is consumed in the process.
 *
 *	input:
 * 		line: A character array containing the current server log entry line
 * 		delim: A character array containing the delimiters to be used to separate fields in the line
 *      cache: A double array initalized to at least LFGETS+1 to cache critical data
 *      date: An empty character array to write a date to
 *
 * 	output:
 * 		cache: An double array with updated webstats data
 *      date: A character array with the extracted date written to it
 */
static void parse_line(char *line, const char *delim, double *cache, char *date)
{
	/* Setup Data */
	char *next;
	char *save;
	char *address;
	char *httpStatusCode;
	int cnt = 0;
	int bytes_downloaded = 0;

	/* Increment the total gets */
	cache[TGETS]++;

	/* Begin tokenization of the line */
	next = strtok_r(line, delim, &save);
	while(next) {
		/* Get our address */
		if(cnt == ADDRESS_FIELD) {
			address = (char *) malloc(strlen(next) + 1);
			strcpy(address, next);
		}

		/* Get and write the date */
		else if(cnt == DATE_FIELD)
			strcpy(date, next);

		/* Get our status code */
		else if(cnt == HTTP_STATUS_CODE_FIELD) {
			httpStatusCode = (char *) malloc(strlen(next) + 1);
			strcpy(httpStatusCode, next);

			/* If failure, we need to update our failed gets */
			if(atoi(httpStatusCode) == 404)
				cache[FGETS]++;
		}

		/* Get the bytes downloaded */
		else if(cnt == BYTES_DOWNLOADED_FIELD) {
			bytes_downloaded = atoi(next);

			/* Update the total bytes */
			cache[TBYTES] += bytes_downloaded;

			/* See if it's ours */
			if((strstr(address, "boisestate.edu") != NULL) || (strstr(address, "132.178") != NULL)) {
				/* Update our local gets and bytes downloaded */
				cache[LGETS]++;
				cache[LBYTES] += bytes_downloaded;

				/* Update our local failures if needed */
				if(atoi(httpStatusCode) == 404)
					cache[LFGETS]++;
			}

			/* Keep it clean */
			free(address);
			free(httpStatusCode);
			return;
		}
		next = strtok_r(NULL, delim, &save);
		cnt++;
	}

	/* Keep it clean in case we break out or miss the return above */
	free(address);
	free(httpStatusCode);
}

/**
 * initialize_webstats(): Initialize the webstats structure.
 */
static void initialize_webstats()
{
	webstats.total_bytes = 0;
	webstats.total_gets = 0;
	webstats.local_bytes = 0;
	webstats.local_gets = 0;
	webstats.failed_gets = 0;
	webstats.local_failed_gets = 0;
	pthread_mutex_init(&(webstats.mutex), NULL);
}


/**
 *	update_webstats(): Updates the webstats structure based on the input fields of current line.
 *
 *	input:
 *		num: The number of fields in the current webserver log entry
 *		field: An array of num strings representing the log entry
 */
static void update_webstats(double *cache)
{
	/* Lock out other threads */
	pthread_mutex_lock(&(webstats.mutex));

	/* Update our critical data */
	webstats.local_bytes += cache[LBYTES];
	webstats.total_bytes += cache[TBYTES];
	webstats.local_gets += cache[LGETS];
	webstats.total_gets += cache[TGETS];
	webstats.failed_gets += cache[FGETS];
	webstats.local_failed_gets += cache[LFGETS];

	/* Unlock so other threads get a turn */
	pthread_mutex_unlock(&(webstats.mutex));
}


/**
 * 	print_webstats(): Print out webstats on standard output.
 */
static void print_webstats()
{
	printf("%10s %15s   %15s  %15s\n", "TYPE", "gets", "failed gets", "MB transferred");
	printf("%10s  %15.0f  %15.0f  %15.0f\n", "local", webstats.local_gets,
	       webstats.local_failed_gets, (double) webstats.local_bytes / (1024 * 1024));
	printf("%10s  %15.0f  %15.0f  %15.0f\n", "total", webstats.total_gets,
	       webstats.failed_gets, (double) webstats.total_bytes / (1024 * 1024));
}


/**
 * process_file(): The main function that processes one log file
 *
 *	input:
 * 		ptr: Pointer to log file name.
 */
void *process_file(void *ptr)
{
	/* Setup our data */
	char *filename = (char *) ptr;
	char *linebuffer = (char *) malloc(sizeof(char) * MAX_LINE_SIZE);
	char *date = (char *) malloc(sizeof(char) * MAX_LINE_SIZE);
	double *cache = (double *) calloc(LFGETS + 1, sizeof(double));
	FILE *fin = fopen(filename, "r");

	/* Handle error of opening file */
	if(fin == NULL) {
		fprintf(stderr, "Cannot open file %s\n", filename);
		exit(1);
	}

	/* Get us that file data */
	char *s = fgets(linebuffer, MAX_LINE_SIZE, fin);

	/* Process the line if file had data */
	if(s != NULL) {
		/* Strip the first line for start date */
		parse_line(linebuffer, DELIMETER, cache, date);
		printf("Starting date: %s\n", date);

		/* Nab the rest of it all */
		while(fgets(linebuffer, MAX_LINE_SIZE, fin) != NULL) {
			parse_line(linebuffer, DELIMETER, cache, date);
			strcpy(linebuffer, "");
		}
		printf("Ending date: %s\n", date);

		/* Write to webstats (only ONCE to reduce critical code access) */
		update_webstats(cache);
	}

	/* Keep it clean */
	fclose(fin);
	free(date);
	free(linebuffer);
	free(cache);

	return NULL;
}

/**
 * Main insertion
 */
int main(int argc, char **argv)
{
	/* Check args */
	if(argc <  2) {
		fprintf(stderr, "Usage: %s <access_log_file> {<access_log_file>} \n", argv[0]);
		exit(1);
	}

	/* Setup data */
	int i;
	int numThreads = argc - 1;
	char *program_name;
	pthread_t *threads;

	/* Set the program name (pretty useless in version 1) */
	program_name = (char *) malloc(strlen(argv[0]) + 1);
	strcpy(program_name, argv[0]);

	/* Initialize our data */
	initialize_webstats();
	threads = (pthread_t *) malloc(sizeof(pthread_t) * numThreads);

	/* And we're off with our threads */
	for(i = 0; i < numThreads; i++)
		pthread_create(&threads[i], NULL, process_file, (void*) argv[i + 1]);

	/* Wait on our threads to finish up */
	for(i = 0; i < numThreads; i++)
		pthread_join(threads[i], NULL);

	/* Throw out the data */
	print_webstats();

	/* Keep it clean */
	free(program_name);
	free(threads);
	exit(0);
}

/* vim: set ts=4: */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define MAX_LINE_SIZE 4096
#define MAX_NUM_FIELDS 40

static const int LBYTES = 0;
static const int TBYTES = 1;
static const int LGETS = 2;
static const int TGETS = 3;
static const int FGETS = 4;
static const int LFGETS = 5;
static const int HTTP_STATUS_CODE_FIELD = 8;
static const int BYTES_DOWNLOADED_FIELD = 9;

struct stats {
	double local_bytes;
	double total_bytes;
	double local_gets;
	double total_gets;
	double failed_gets;
	double local_failed_gets;
	HANDLE mutex;
} webstats;

HANDLE *threads;

/*
* parse_line(): Parse one line into string tokens separated by the given delimiters.
* Beware: The input argument line is consumed in the process.
*
*	input:
* 		line: A character array containing the current server log entry line
* 		delim: A character array containing the delimiters to be used to separate fields in the line
*
* 	output:
* 		field: An array of strings that represent the tokens found in the line.
*
* 	return:
* 		num: The number of tokens found in the line.
*/

static void parse_line(char *line, const char *delim, double *cache, char *date)
{
	char *next;
	char *save;
	char *address = NULL;
	char *httpStatusCode = NULL;
	int cnt = 0;
	int bytes_downloaded = 0;

	cache[TGETS]++;

	next = strtok_s(line, delim, &save);
	while (next) {
		if (cnt == 0) {
			address = (char *)malloc(strlen(next) + 1);
			strcpy(address, next);
		}

		else if (cnt == 3) {
			strcpy(date, next);
		}

		else if (cnt == HTTP_STATUS_CODE_FIELD) {
			httpStatusCode = (char *)malloc(strlen(next) + 1);
			strcpy(httpStatusCode, next);

			if (atoi(httpStatusCode) == 404)
				cache[FGETS]++;
		}

		else if (cnt == BYTES_DOWNLOADED_FIELD) {
			bytes_downloaded = atoi(next);

			cache[TBYTES] += bytes_downloaded;

			if ((strstr(address, "boisestate.edu") != NULL) || (strstr(address, "132.178") != NULL)) {
				cache[LGETS]++;
				cache[LBYTES] += bytes_downloaded;

				if (atoi(httpStatusCode) == 404)
					cache[LFGETS]++;
			}
			free(address);
			free(httpStatusCode);
			return;
		}

		next = strtok_s(NULL, delim, &save);
		cnt++;
	}
}

/*
* initialize_webstats(): Initialize the webstats structure.
*
* 	input:
* 		none
*	output
* 		none
* 	return
* 		none
*/

static void initialize_webstats()
{
	webstats.total_bytes = 0;
	webstats.total_gets = 0;
	webstats.local_bytes = 0;
	webstats.local_gets = 0;
	webstats.failed_gets = 0;
	webstats.local_failed_gets = 0;
	webstats.mutex = CreateMutex(NULL, FALSE, NULL);
}


/*
*	update_webstats(): Updates the webstats structure based on the input fields of current line.
*
*	input:
*		num: The number of fields in the current webserver log entry
*		field: An array of num strings representing the log entry
*
*	output:
*		none
*/

static void update_webstats(double *cache)
{
	DWORD lock = WaitForSingleObject(webstats.mutex, INFINITE);

	if (lock == WAIT_OBJECT_0){
		__try{
			webstats.local_bytes += cache[LBYTES];
			webstats.total_bytes += cache[TBYTES];
			webstats.local_gets += cache[LGETS];
			webstats.total_gets += cache[TGETS];
			webstats.failed_gets += cache[FGETS];
			webstats.local_failed_gets += cache[LFGETS];
		}

		__finally{
			ReleaseMutex(webstats.mutex);
		}
	}
}


/*
* 	print_webstats(): Print out webstats on standard output.
*
*	input:
*		none
*	output:
*		none
*
*	return
*		none
*/

static void print_webstats()
{
	printf("%10s %15s   %15s  %15s\n", "TYPE", "gets", "failed gets",
		"MB transferred");
	printf("%10s  %15.0f  %15.0f  %15.0f\n", "local", webstats.local_gets,
		webstats.local_failed_gets, (double)webstats.local_bytes / (1024 * 1024));
	printf("%10s  %15.0f  %15.0f  %15.0f\n", "total", webstats.total_gets,
		webstats.failed_gets, (double)webstats.total_bytes / (1024 * 1024));
}


/*
* process_file(): The main function that processes one log file
*
*	input:
* 		ptr: Pointer to log file name.
*
*	output:
*		none
*	return
*		none
*/
DWORD WINAPI process_file(LPVOID ptr)
{
	char *filename = (char *)ptr;
	char *linebuffer = (char *)malloc(sizeof(char)* MAX_LINE_SIZE);
	char *date = (char *)malloc(sizeof(char)* MAX_LINE_SIZE);
	double *cache = (double *)calloc(LFGETS + 1, sizeof(double));

	FILE *fin = fopen(filename, "r");

	if (fin == NULL) {
		fprintf(stderr, "Cannot open file %s\n", filename);
		exit(1);
	}

	char *s = fgets(linebuffer, MAX_LINE_SIZE, fin);

	if (s != NULL) {
		parse_line(linebuffer, " []\"", cache, date);
		printf("Starting date: %s\n", date);

		while (fgets(linebuffer, MAX_LINE_SIZE, fin) != NULL) {
			parse_line(linebuffer, " []\"", cache, date);
			strcpy(linebuffer, "");
		}
		printf("Ending date: %s\n", date);

		update_webstats(cache);
	}

	fclose(fin);

	free(date);
	free(linebuffer);
	free(cache);

	return NULL;
}

int main(int argc, char **argv)
{
	if (argc <  2) {
		fprintf(stderr, "Usage: %s <access_log_file> {<access_log_file>} \n", argv[0]);
		exit(1);
	}

	int i;
	int numThreads = argc - 1;
	char *program_name;
	DWORD ThreadId;

	program_name = (char *)malloc(strlen(argv[0]) + 1);
	strcpy(program_name, argv[0]);

	initialize_webstats();

	threads = (HANDLE *)malloc(sizeof(HANDLE)* numThreads);

	for (i = 0; i < numThreads; i++)
		threads[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)process_file, argv[i + 1], 0, &ThreadId);

	WaitForMultipleObjects(numThreads, threads, TRUE, INFINITE);

	for (i = 0; i < numThreads; i++)
		CloseHandle(threads[i]);

	CloseHandle(webstats.mutex);

	print_webstats();

	free(program_name);
	free(threads);
	exit(0);
}

/* vim: set ts=4: */

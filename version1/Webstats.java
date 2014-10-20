import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.StringTokenizer;

/**
 * Original Author: Jeff Cope
 * Maintaining Author: Amit Jain
 * Modified by: Stephen Porter
 * Class: CS 453
 */
public class Webstats {
	private static final int LBYTES = 2;
	private static final int TBYTES = 4;
	private static final int LGETS = 8;
	private static final int TGETS = 16;
	private static final int FGETS = 32;
	private static final int LFGETS = 64;
	
	private static final int ADDRESS_FIELD = 0;
	private static final int DATE_FIELD = 3;
	private static final int HTTP_STATUS_CODE_FIELD = 8;
	private static final int BYTES_DOWNLOADED_FIELD = 9;
	
	private static final String DELIMETER = " []\"";

	private double local_bytes;
	private double total_bytes;
	private double local_gets;
	private double total_gets;
	private double failed_gets;
	private double local_failed_gets;

	/**
	 * Webstat(): Initializes the Webstat structure.
	 * @param new_program_name This programs name.
	 * @return none
	 */
	public Webstats(String new_program_name) {
		local_bytes = 0;
		total_bytes = 0;
		local_gets = 0;
		total_gets = 0;
		failed_gets = 0;
		local_failed_gets = 0;
	}

	/**
	 * parse_line(): Parse one line into String tokens separated by the given
	 * delimiters.
	 * @param line A character array containing the current server log entry line
	 * @param delim A character array containing the delimiters to be used to separate fields in the line
	 * @return The number of tokens found in the line.
	 */
	private String parse_line(String line, String delim, double[] cache) {
		StringTokenizer strtok = new StringTokenizer(line, delim);
		int cnt, bytes_downloaded;
		String next, address, httpStatusCode, date;
		
		cnt = 0;
		address = httpStatusCode = date = "";

		cache[TGETS]++;

		while (strtok.hasMoreTokens()) {
			next = strtok.nextToken();
			bytes_downloaded = 0;

			if (cnt == ADDRESS_FIELD) 
				address = next;

			else if (cnt == DATE_FIELD) 
				date = next;

			else if (cnt == HTTP_STATUS_CODE_FIELD) {
				httpStatusCode = next;

				if (httpStatusCode.equals("404"))
					cache[FGETS]++;
			}

			else if (cnt == BYTES_DOWNLOADED_FIELD) {
				try {
					bytes_downloaded = Integer.parseInt(next);
				}
				catch (NumberFormatException e) {
					/* skip lines without downloaded bytes field */
				}

				cache[TBYTES] += bytes_downloaded;

				if ((address.indexOf("boisestate.edu") != -1) || (address.indexOf("132.178") != -1)) {
					cache[LGETS]++;
					cache[LBYTES] += bytes_downloaded;

					if (httpStatusCode.equals("404"))
						cache[LFGETS]++;
				}
				
				return date;
			}
			
			cnt++;
		}

		return date;
	}

	/**
	 * update_Webstat(): Updates the Webstat structure based on the input fields
	 * of current line.
	 * @param num The number of fields in the current webserver log entry
	 * @param field An array of num Strings representing the log entry
	 * @return none
	 */
	private synchronized void update_webstats(double[] cache) {
		local_bytes += cache[LBYTES];
		total_bytes += cache[TBYTES];
		local_gets += cache[LGETS];
		total_gets += cache[TGETS];
		failed_gets += cache[FGETS];
		local_failed_gets += cache[LFGETS];
	}

	/**
	 * print_webstats(): Print out webstats on standard output.
	 * @return none
	 */
	void print_webstats() {
		System.out.printf("%10s %15s   %15s  %15s\n", "TYPE", "gets", "failed gets", "MB transferred");
		System.out.printf("%10s  %15.0f  %15.0f  %15.0f\n", "local", local_gets, local_failed_gets, (double) local_bytes / (1024 * 1024));
		System.out.printf("%10s  %15.0f  %15.0f  %15.0f\n", "total", total_gets, failed_gets, (double) total_bytes / (1024 * 1024));
	}

	/**
	 * process_file(): The main function that processes one log file
	 * @param ptr Pointer to log file name.
	 * @return none output: none
	 */
	public void process_file(Object ptr) {
		double[] cache = new double[LFGETS + 1];
		String filename = (String) ptr;
		String end_date = "";

		try {
			BufferedReader fin = new BufferedReader(new FileReader(new File(filename)));
			String linebuffer = fin.readLine();
			
			if (linebuffer != null) {
				System.out.println("Starting date: " + parse_line(linebuffer, DELIMETER, cache));

				while ((linebuffer = fin.readLine()) != null)
					end_date = parse_line(linebuffer, DELIMETER, cache);
				
				System.out.println("Ending date: " + end_date);
			}

			update_webstats(cache);
			fin.close();
		}
		catch (FileNotFoundException e) {
			System.err.println("Cannot read from file " + filename);
			System.exit(1);
		}
		catch (IOException e) {
			System.err.println("Cannot open file " + filename);
			System.exit(1);
		}
	}

	/**
	 * Main insertion (Such bad Java coding)
	 */
	public static void main(String argv[]) throws InterruptedException {
		if (argv.length < 1) {
			System.err.println("Usage: java Webstats <access_log_file> {<access_log_file>}");
			System.exit(1);
		}

		Thread[] threads = new Thread[argv.length];
		Webstats ws = new Webstats("Webstats.java");

		for (int i = 0; i < argv.length; i++)
			threads[i] = createThread(ws, argv[i]);

		for (int i = 0; i < argv.length; i++)
			threads[i].join();

		ws.print_webstats();
		System.exit(0);
	}

	/**
	 * I hate doing threads this way in Java, but I figured I would keep
	 * it as close to the C solution as possible in case you didn't want
	 * any extra classes.
	 * @param ws The webstats object
	 * @param ptr The file path to kick off the process file command
	 * @return The thread
	 */
	private static Thread createThread(Webstats ws, Object ptr) {
		Thread t = new Thread(new Runnable() {
			public void run() {
				ws.process_file(ptr);
			}
		});

		t.start();
		return t;
	}
}

/* vim: set ts=4: */

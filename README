Name: Stephen Porter
Updt: 10/19/2014
Clss: CS 453
Asmt: P2

============
Build / Run
============
To Build:
	make

To Run:
	   C: ./webstats <access_log>
	Java: java Webstats <access_log>
	
To Clean:
	make clean

======
Files
======
ms-windows // Windows version of webstats
	|
	webstats
		|
		Debug
			|
			(empty)
		Webstats
			|
			Debug
				|
				Webstats.Build.CppClean.log
				Webstats.log
				Webstats.tlog
					|
					(empty)
			Webstats.cpp // Webstats code
			Webstats.vcxproj
			Webstats.vcxproj.filters
			Webstats.vcxproj.user
		Webstats.opensdf
		Webstats.sln // Webstats solution file
		Webstats.v12.suo
version1
	|
	Makefile // Makefile to build project
	webstats.c // C version
	webstats.java // Java version
Makefile // Top level Makefile
README // This README

=====
Data
=====
Program 		Linux C 		Java 		Windows
-------------------------------------------------------------------------
Sequential 		3.68s 			5.53s 		48.83s
Threaded 		0.28s 			0.85s 		8.43s
Speedup 		1338.91% 		650.59% 	579.00%

===========
Discussion
===========
This project was very straight-forward. This is most likely because I've done multi-threading in Java before, so it wasn't really that difficult. I actually spent a lot more time trying to optimize it for better speedups and making it look pretty. I had it done prior to the extension, but when I'm given extra time to do something, I use it (I don't like turning things in early).

Anyhow, the first thing I did was the Java version since I have experience multi-threading Java, so I figured it would be the easiest. Immediately I noticed that the update_webstats method accessed shared data and would need to be synchronized. I did that and setup my threads and sure enough, it worked as expected. I then moved on to reduce the critical section of code. I had to brainstorm for a bit, but I came up with a simple cache system which would allow me to only ever have to call update_webstats once. So I setup process_file so that each thread would create a double array with indexes specified by private static final integer data types to represent which field we are updating. Since each thread is operating on it's own simple cache, all of them can parse the lines and update their own data at the same time. Then after all the parsing is done, I call update_webstats once with the cache from each thread which is synchronized.

Once I had the Java version done, I knew it would basically be the same process, but in C. I practically did the exact same thing except I setup a mutex to synchronize the update_webstats method (No thank you semaphores!).

Once the C version was done, I moved on to the Windows version. I did literally the exact same thing except using Windows API for threads and mutexes.

I did hit one roadblock, but it took me like 5 minutes to figure it out. In the Windows version, I was getting some weird data. I initially thought that maybe the mutex didn't work properly, but then realized that couldn't be the case. I looked at my process_file method and instantly saw that I was using malloc for my data cache and not initalizing the data so I was getting some nice garbage. I fixed that with a calloc, went back to my Linux C version and switched that malloc out with a calloc, and sure enough, all was well. It was funny though, because in the C version, even though I was using malloc and not initializing my data, the output was correct, so somehow I wasn't getting any garbage.
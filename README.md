# c-shell
Custom C Shell implemented for CSE-34341 Operating Systems (Spring 2019)

### Report

Author: Seth Cattanach (scattana)

#### Compiling the shell and system requirements

`shell.c` requires the `gcc` compiler with `gnu99` extention enabled. You can compile the executable with the provided Makefile, or run the following command:

`gcc -std=gnu99 -g -Wall shell.c -o shell`

The following flags are enabled in this statement:
* `-std=gnu99` (to specify the `gnu99` extention)
* `-Wall` (to enable warnings)
* `-g` (to enable debugging tools such as valgrind to operate properly)
* `-o` (to specify executable name)

To check memory usage, make sure the `valgrind` utility is installed on your system, and run the following command: `valgrind --leak-check=full -v ./shell` (after compiling the `shell` executable)
* Note: the `-v` flag here is optional; it provides verbose (very detailed) output

To run the executable, simply type `./shell` (if you get a permission denied error, try `chmod +x ./shell` and try executing `./shell` again)

#### Features and Usage:

`shell.c` includes the following features:
	
| Feature												|	Example 					|
|-------------------------------------------------------|-------------------------------|
| A command to exit										| --> exit                      |
| A command with no arguments							| --> ls						|
| A command with arguments								| --> ls -l						|
| A command executed in the background (&)				| --> cat myFile.txt &			|
| A command whose output is redirected					| --> ls -l > myFile.txt		|
| A command whose input is redirected					| --> sort < myFile.txt			|
| Commands connected by piping							| --> ls -l | more				|
| A line with multiple semicolon-separated commands		| --> ls -l ; pwd ; cat in.txt	|
| A command to search previous commands					| --> history [-c] [number]		|
| A command to execute the last command					| --> !!						|
| A command to search/execute history (prefix-matching) | --> !prefix					|



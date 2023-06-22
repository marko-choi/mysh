# mysh - my shell

---

Mysh is a fully functional shell that provides a vast range of functionality matching, and even outperforming that of other builtin shells.

#### Makefile

The Makefile produces an executable called
mysh

The command "make run" starts mysh.

The Makefile uses the following flags to compile mysh files:
<br>-g -Wall -Werror -fsanitize=address

### General Overview

Mysh will support the following commands as builtins:

- echo
- ls
- cd
- cat
- wc
- pipes (|)
- bg (&)
- kill
- ps
- exit
- start-server
- close-server
- send
- start-client

### Functionalities

---

- User prompt: mysh$ (this is shown and waiting for user input)

- Command: echo \[string\]

  - e.g., mysh$ echo hello world
    - Outputs the \[string\] component (i.e., hello world) to stdout
    - In contrast to bash, echo does not handle quotes in a unique way. Specifically, echo "hello world" should display "hello world", not hello world.
    - Similar to bash, extra spaces are ignored. So echo "hello&nbsp; &nbsp; &nbsp;world" displays "hello world"

- Command: exit

  - e.g., mysh$ exit
  - Terminates the shell program with exit code 0.

- Limitations and Notes:

  - The maximum input line length, including the command itself, is 64 characters.
  - If more than 64 characters are entered, output an error to stderr: `ERROR: input line too long` followed by a new line.
  - Upon receiving any command other than echo or exit, your code should output:
    `ERROR: Unrecognized command: [the unrecognized command]` followed by a new line.
  - Your shell should NEVER crash, no matter what the input is.

- Setting and using environment variables:

  - e.g.: \
    mysh$ myvar=hello \
    mysh$ myvar2=world \
    mysh$ echo $myvar $myvar2 \
    hello world
  - The program must support storing an arbitrary number of environment variables.
  - Your programs memory footprint must grow proportionally and reasonably with the number of environment variables.
  - Hint: You are required to dynamically allocate memory for environment variables.

- Error Handling: Whenever a builtin command fails, you must report an appropriate error messages. Some required descriptions are outlined below. For any other error scenarios, choose a corresponding descriptive message.
  - ERROR: Description of error message (for errors not covered in the next two cases)
  - ERROR: Builtin failed: builtin name
  - For all commands not specified in the previous milestones or those not yet implemented (e.g., true, yes), check whether they are builtin (to your shell), and if so, run the builtin commands, otherwise display the error `ERROR: Unrecognized command: [the unrecognized command]`
- Command: ls
  - ls \[path\]
    - path can be absolute or relative
    - --f substring. Display all directory contents that contain the substring
    - --rec \[path\] Indicates a recursive ls traversal starting from path
    - --d \<depth value\>. The search depth of the file traversal tree
    - --rec and --d must be provided together. Report an error if the arguments are not provided together.
    - Report `ERROR: Invalid path` if the provided path is invalid
- Command: cd
  - cd \[path\]
  - path can be absolute or relative
  - Some additional features:
    - cd ... (change directory 2 directories up: ../.. must also work)
    - cd .... (change directory 3 directories up: ../../.. must also work)
    - cd .. (zero or more copies of "/.." may be appended, and cd must change directory the correct amount of times)
    - Report `ERROR: Invalid path` if the provided path is invalid
- Command: cat
  - cat \[file\] (displays the contents of the file to stdout)
    - If there is standard input, execute the command by reading standard input.
  - If there is no standard input, display `ERROR: No input source provided`.
  - Report `ERROR: Cannot open file` if the provided file path is invalid
- Command: wc

  - wc \[file\]
  - e.g., Assume the file foobar has 1234 characters in it, each separated by any whitespace (e.g., \n, \t, \r). These characters form 251 words and span 25 lines. Then ...\
    mysh$ wc foobar.txt \
    word count 251 \
    character count 1234 \
    newline count 25
  - The implementation for this function may not use string methods
  - If there is standard input, execute the command by reading standard input.
  - If there is no standard input, display `ERROR: No input source provided`.
  - Report `ERROR: Cannot open file` if the provided file path is invalid

- Pipes work for all built-in commands.

  - E.g., `cat some_file | wc`
    - This should pipe the contents of some_file as input to the wc command.
    - Note: Input via pipe is not subject to the 64 character command line limit.

- If a shell command does not exist, use the default bash implementation

  - If a shell command does not exist, use the implementation from /bin or /usr/bin
  - The shell must not terminate when pressing ctrl+c
  - Backgrounding a process via &
    - Starts the program as a background process
    - Display `[number] pid` when the process is launched. e.g., `[1] 25787`. The behaviour of this is identical to bash.
    - Display `[number]+  Done command` when the process completes. The behavioural is identical to bash, except that we will only display Done messages. If a process terminates because of a signal, a Done is still shown.
  - Command: kill \[pid\] \[signum\]
    - Send signal number signum to the process with id pid.
    - If signum is not provided, send a SIGTERM
    - If the process id is invalid, report `ERROR: The process does not exist`
    - If the signal is invalid report `ERROR: Invalid signal specified`
  - Command: ps
    - List process names and ids for all processes launched by this shell. For example: \
      mysh$ sleep 50 & \
      [1] 8574 \
      mysh$ sleep 20 & \
      [2] 8575 \
      mysh$ ps \
      sleep 8575 \
      sleep 8574 \
      mysh$

- Command: start-server \<port number>
  - Initiates a background server on the provided port number. The server must be non-blocking and must allow multiple connections at the same time.
  - Report `ERROR: No port provided` if no port is provided
- Command: close-server
  - Terminates the current server.
- Commmand: send \<port number> \<hostname> \<message>
  - Send a message to the hostname at the port number.
- Command: start-client \<port number> \<hostname>
  - Starts a client that can send multiple messages. The messages are taken as standard input. The client is connected once and can continue sending messages until pressing a CTRL+D or CTRL+C.
  - Report `ERROR: No port provided` if no port provided
  - Report `ERROR: No hostname provided` if no hostname provided

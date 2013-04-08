Readme for cvsh v0.1
----------------------

cvsh 0.1 is a simple command interpreter for a unix like environment. Has been developed by S.Chandravadan (Hence the 'cv' in the name :P ) over a period of ~10 days :)

Features:
----------

-Internal commands
  -"cd" is fully supported in all its formats (.. ,NULL, -, ~, PATH)
  -variables can be exported using the "export NAME=VALUE" format. They aren't written to (though read from) the .cvshrc file.
  -"pwd" is supported.

-I/O Redirection
  - The >,>> and < operators are supported in their full capacity, and there shouldn't be a problem with multiple redirections in a single command. (The heredoc, << has been reserved for future versions :)

-Piping
  - The "|" operator is fully supported, and the limit on the number of pipes is restricted only by the creativity of the user :P
  - Piping with internal redirections is also supported. Ex: cat<afile|grep abc>found.txt

-History
  -All the commands (literally) are logged in the 'histfile', located in the same folder. It is read into the main memory when the shell starts to minimise the disk accesses, making cvsh 'buttery smooth' when reverse searching ;)
  -The "!" operator is supported. It looks for the most recent command that started with the specified characters.

-Job Control
  -A limited amount of job control facilities are supported. 
    -The "&" operator at the end of a command starts a back ground process. It also gives the list of jobs in the background along with their pids, so the user can choose which one to kill.
    -The "fg %n" format can be used to bring the nth background process into the foreground. Beware of bringing infinite processes into the fg though, that might be the last thing you ever do! :P
    -For situations where cvsh starts misbehaving, a Ctrl-C would kill it and all its children, as opposed to making them 'defunct'.

Limitations:
-------------
- Signal handling is present to a very limited extent.
- heredoc operator (<<) yet to be implemented.
- "kill %n" format not yet supported.


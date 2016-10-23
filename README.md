# thuongdang-shell

Thuong Dang Shell - #TD$ v. 0.01a

Implemented by Tuan Dao

Functions:
- Most basic shell functions are implemented:
	+ Background execution (&)
	+ Input and output redirection to files (>, <)
	+ Input and output redirection to processes (|)
- Extra functions:
	+ Change directory. 
		Usage: Use "cd" command. Example: cd ../testdir
	+ Environment variables. 
		Usage: Use "export" to set environment variables. Example: export VAR=VALUE
		Use "lsenv" to list all current environment variables.
	+ Notifications upon background process completion. Currently only support one background process. If more than one process is working in the background, the newest process is prioritized.
	+ Clear console output. 
		Usage: Use "clear"

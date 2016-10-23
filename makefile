all: shell

shell: readme
	cat readme
	gcc shell.c -Wall -o tdsh

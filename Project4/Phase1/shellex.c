/* $begin shellmain */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);

int main()
{
	char cmdline[MAXLINE]; /* Command line */

	while (1) {
		/* Read */
		printf("cse4100-SP-P#4> ");
		fgets(cmdline, MAXLINE, stdin);
		if (feof(stdin))
			exit(0);

		/* Evaluate */
		eval(cmdline);
	}
}
/* $end shellmain */

/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline)
{
	char *argv[MAXARGS]; /* Argument list execve() */
	char buf[MAXLINE];   /* Holds modified command line */
	int bg;              /* Should the job run in bg or fg? */
	pid_t pid;           /* Process id */
	strcpy(buf, cmdline);
	bg = parseline(buf, argv);
	if (argv[0] == NULL)
		return;   /* Ignore empty lines */
	if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
		if ((pid = Fork()) == 0) {
			if (execvp(argv[0], argv) < 0) {	//ex) /bin/ls ls -al &
				printf("%s: Command not found.\n", argv[0]);
				exit(0);
			}
		}

		/* Parent waits for foreground job to terminate */
		if (!bg) {
			int status;
			if (waitpid(pid, &status, 0) < 0)
				unix_error("waitfg: waitpid error");
		}
		else//when there is backgrount process!
			printf("%d %s", pid, cmdline);
	}
	return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv)
{
	char msg[1024] = "-bash: cd: ";
	if (!strcmp(argv[0], "quit") || !strcmp(argv[0], "exit"))	/* quit command or exit command*/
		exit(0);
	if (!strcmp(argv[0], "&"))		/* Ignore singleton & */
		return 1;
	if (!strcmp(argv[0], "cd")) {	/* change directory & */
		if (argv[1] == NULL) chdir(getenv("HOME"));
		else
			if (chdir(argv[1])) {
				strcat(msg, argv[1]);
				perror(msg);
			}
		return 1;
	}
	return 0;                     /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv)
{
	char *delim;		/* delimiter */
	int argc;			/* Number of args */
	int bg;				/* Background job? */
	char *space;		/* Points to first space delimiter */
	char *dquotes1;		/* Points to first double quotes delimiter */
	char *dquotes2;		/* Points to second double quotes delimiter */
	char *squotes1;		/* Points to first single quotes delimiter */
	char *squotes2;		/* Points to second single quotes delimiter */
	char *backslash;	/* Points to second backslash delimiter */

	buf[strlen(buf) - 1] = ' ';  /* Replace trailing '\n' with space */
	while (*buf && (*buf == ' ')) /* Ignore leading spaces */
		buf++;

	/* Build the argv list */
	argc = 0;

	while ((space = strchr(buf, ' '))) {

		delim = space;
		dquotes1 = strchr(buf, '\"');
		squotes1 = strchr(buf, '\'');
		if (dquotes1 != 0 && dquotes1 < delim) delim = dquotes1;
		if (squotes1 != 0 && squotes1 < delim) delim = squotes1;
		if (delim == dquotes1) {			/* double quotes*/
			dquotes2 = strchr(delim + 1, '\"');
			argv[argc++] = delim + 1;
			*dquotes2 = '\0';
			buf = dquotes2 + 1;
			while (*buf && (*buf == ' '))	/* Ignore spaces */
				buf++;
		}
		else if (delim == squotes1) {		/* single quotes*/
			squotes2 = strchr(delim + 1, '\'');
			argv[argc++] = buf + 1;
			*squotes2 = '\0';
			buf = squotes2 + 1;
			while (*buf && (*buf == ' ')) /* Ignore spaces */
				buf++;
		}
		else {
			argv[argc++] = buf;
			*delim = '\0';
			buf = delim + 1;
			while (*buf && (*buf == ' ')) /* Ignore spaces */
				buf++;
		}
	}
	argv[argc] = NULL;

	if (argc == 0)  /* Ignore blank line */
		return 1;


	/* Should the job run in the background? */
	if ((bg = (*argv[argc - 1] == '&')) != 0)
		argv[--argc] = NULL;
	else if ((bg = (argv[argc - 1][strlen(argv[argc - 1]) - 1] == '&')) != 0)
		argv[argc - 1][strlen(argv[argc - 1]) - 1] = '\0';
	return bg;
}
/* $end parseline */
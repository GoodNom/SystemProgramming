/* $begin shellmain */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128
#define MAXJOBS   128

/*job states*/
#define DEFAULT	0		/* default (undefined) */
#define FG		1		/* running in foreground */
#define BG		2		/* running in background */
#define ST		3		/* stopped */
#define DONE	4		/* done */
#define KILL	5		/* killed */


/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
void run_command(char **argv, char *cmdline, int bg);
void run_pipe_command(char *argv[][MAXARGS], char *cmdline, int pos, int in_fd, int bg);

void sigint_handler(int sig);
void sigtstp_handler(int sig);
void sigchld_handler(int sig);

typedef struct job {		/* job struct */
	pid_t pid;				/* job pid */
	int id;					/* job id */
	int state;				/* job state ()*/
	char cmdline[MAXLINE];	/* command line */
}job;

job jobs[MAXJOBS];	/* job list */

void initjobs(job *jobs);
int addjob(job *jobs, pid_t pid, int state, char *cmdline);
void donejob(job *jobs, pid_t pid);
void clearjob(job *jobs);
void printjobs(job *jobs);
void delNprint_done(job *jobs);
void waitfg(pid_t pid);
void checkflag(char **argv);
int cur_jobid = 0;	/* current job id*/
int flag = 0;

int main()
{
	char cmdline[MAXLINE]; /* Command line */

	initjobs(jobs);

	while (1) {
		Signal(SIGINT, sigint_handler);
		Signal(SIGTSTP, sigtstp_handler);
		Signal(SIGCHLD, sigchld_handler);
		/* Read */
		printf("cse4100-SP-P#4> ");
		fgets(cmdline, MAXLINE, stdin);
		if (feof(stdin))
			exit(0);

		/* Evaluate */
		eval(cmdline);
		delNprint_done(jobs);
	}
}
/* $end shellmain */

/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline)
{
	char *argv[MAXARGS][MAXARGS]; /* Argument list execvp() */
	char buf[MAXLINE];	/* Holds modified command line */
	int bg;				/* Should the job run in bg or fg? */
	char *delim;		/* Points to first space delimiter */
	char cmd[MAXARGS][MAXLINE];	/* Argument list execvp() */
	char *ptr;			/* Points to buf */
	int cmd_n = 0;		/* Number of cmd */
	pid_t pid;			/* Process id */
	int state;

	strcpy(buf, cmdline);

	if ((delim = strchr(buf, '|'))) {
		ptr = buf;
		ptr[strlen(ptr) - 1] = '|';
		while ((delim = strchr(ptr, '|'))) {
			*delim = '\0';
			strcpy(cmd[cmd_n], ptr);
			strcat(cmd[cmd_n++], "\n");
			ptr = delim + 1;
		}
		cmd[cmd_n][0] = '\0';

		for (int i = 0; cmd[i][0] != '\0'; i++) {
			bg = parseline(cmd[i], argv[i]);
			state = bg ? BG : FG;
		}
		argv[cmd_n][0] = NULL;
		if ((pid = Fork()) == 0) {
			if (!bg) {
				signal(SIGINT, SIG_DFL);
			}
			else {//when there is background process!
				signal(SIGINT, SIG_IGN);
			}
			run_pipe_command(argv, cmdline, 0, STDIN_FILENO, bg);
		}
		/* Parent waits for foreground job to terminate */
		addjob(jobs, pid, state, cmdline);	/* Add the child to the job list */

		/* Parent waits for foreground job to terminate */
		if (!bg) {
			int status;
			if (waitpid(pid, &status, WUNTRACED) < 0) {
				unix_error("waitfg: waitpid error");
			}
			else {
				for (int i = 0; i < MAXJOBS; i++) {
					if (jobs[i].state == FG) {
						clearjob(&jobs[i]);
					}
				}
			}
		}
		else {//when there is background process!
			int id;

			for (int i = 0; i < MAXJOBS; i++) {
				if (jobs[i].pid == pid) {
					id = jobs[i].id;
					break;
				}
			}
			printf("[%d] %d\t\t\t%s", id, pid, cmdline);
		}

	}
	else {
		bg = parseline(buf, argv[0]);

		if (argv[0][0] == NULL)
			return;   /* Ignore empty lines */
		run_command(argv[0], cmdline, bg);
	}
	return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv)
{
	char msg[1024] = "-bash: cd: ";
	if (!strcmp(argv[0], "quit"))	/* quit command*/
		exit(0);
	if (!strcmp(argv[0], "exit"))	/* exit command*/
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
	if (!strcmp(argv[0], "jobs")) {		/* jobs command */
		printjobs(jobs);
		return 1;
	}
	if (!strcmp(argv[0], "fg")) {		/* fg command */

		int jid;
		int status;
		int len;
		jid = atoi(&argv[1][1]);
		if (jobs[jid - 1].state == DEFAULT) {
			printf("-bash: fg: %s: no such job\n", argv[1]);
		}
		else {

			len = strlen(jobs[jid - 1].cmdline);
			if (jobs[jid - 1].cmdline[len - 2] == '&') {
				jobs[jid - 1].cmdline[len - 2] = '\n';
				jobs[jid - 1].cmdline[len - 1] = '\0';
			}
			printf("%s", jobs[jid - 1].cmdline);
			jobs[jid - 1].state = FG;
			kill(jobs[jid - 1].pid, SIGCONT);
			waitfg(jobs[jid - 1].pid);
		}
		return 1;
	}
	if (!strcmp(argv[0], "bg")) {		/* bg command */

		int jid;
		int status;
		int len;
		jid = atoi(&argv[1][1]);
		if (jobs[jid - 1].state == DEFAULT) {
			printf("-bash: bg: %s: no such job\n", argv[1]);
		}
		else if (jobs[jid - 1].state == BG) {
			printf("-bash: bg: job %d already in background\n", jid);
		}
		else {
			len = strlen(jobs[jid - 1].cmdline);
			jobs[jid - 1].cmdline[len - 1] = ' ';
			jobs[jid - 1].cmdline[len] = '&';
			jobs[jid - 1].cmdline[len + 1] = '\n';
			jobs[jid - 1].cmdline[len + 2] = '\0';

			printf("[%d]   %s", jobs[jid - 1].id, jobs[jid - 1].cmdline);

			jobs[jid - 1].state = BG;
			kill(jobs[jid - 1].pid, SIGCONT);
		}
		return 1;
	}
	if (!strcmp(argv[0], "kill")) {
		int jid;
		int status;
		int len;
		jid = atoi(&argv[1][1]);
		if (jobs[jid - 1].state == DEFAULT) {
			printf("-bash: kill: %s: no such job\n", argv[1]);
		}
		else {
			kill(-jobs[jid - 1].pid, SIGKILL);
			jobs[jid - 1].state = KILL;
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
	else if ((bg = (argv[argc-1][strlen(argv[argc - 1]) - 1] == '&')) != 0)
		argv[argc-1][strlen(argv[argc - 1]) - 1] = '\0';

	checkflag(argv);
	return bg;
}
/* $end parseline */

void run_command(char **argv, char *cmdline, int bg) {
	pid_t pid;           /* Process id */
	int state = bg ? BG : FG;
	if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
		if ((pid = Fork()) == 0) {
			if (!flag) {
				setpgrp();
				flag = 0;
			}
			if (!bg) {
				signal(SIGINT, SIG_DFL);
			}
			else {//when there is background process!
				signal(SIGINT, SIG_IGN);
			}

			if (execvp(argv[0], argv) < 0) {	//ex) /bin/ls ls -al &
				printf("%s: Command not found.\n", argv[0]);
				exit(0);
			}
		}

		addjob(jobs, pid, state, cmdline);	/* Add the child to the job list */

		/* Parent waits for foreground job to terminate */
		if (!bg) {
			int status;
			if (waitpid(pid, &status, WUNTRACED) < 0) {
				unix_error("waitfg: waitpid error");
			}
			else {
				if (WIFEXITED(status)) {
					for (int i = 0; i < MAXJOBS; i++) {
						if (jobs[i].state == FG) {
							clearjob(&jobs[i]);
						}
					}
				}
			}
		}
		else {//when there is background process!
			int id;

			for (int i = 0; i < MAXJOBS; i++) {
				if (jobs[i].pid == pid) {
					id = jobs[i].id;
					break;
				}
			}
			printf("[%d] %d\t\t\t%s", id, pid, cmdline);
		}
	}
}

void run_pipe_command(char *argv[][MAXARGS], char *cmdline, int pos, int in_fd, int bg) {
	int fd[2];
	pid_t pid;           /* Process id */

	if (argv[pos + 1][0] == NULL) {	//last
		Dup2(in_fd, STDIN_FILENO);
		Close(in_fd);
		if (!builtin_command(argv[pos])) { //quit -> exit(0), & -> ignore, other -> run
			if (execvp(argv[pos][0], argv[pos]) < 0) {	//ex) /bin/ls ls -al &
				printf("%s: Command not found.\n", argv[pos][0]);
				exit(0);
			}
		}
		exit(0);
	}
	else {
		if (pipe(fd) < 0) {
			perror("pipe");
			exit(0);
		}
		if ((pid = Fork()) == 0) {
			Close(fd[0]);		//unused
			Dup2(in_fd, STDIN_FILENO);
			Close(in_fd);
			Dup2(fd[1], STDOUT_FILENO);
			Close(fd[1]);
			if (!builtin_command(argv[pos])) { //quit -> exit(0), & -> ignore, other -> run

				if (execvp(argv[pos][0], argv[pos]) < 0) {	//ex) /bin/ls ls -al &
					printf("%s: Command not found.\n", argv[pos][0]);
					exit(1);
				}
			}
			exit(0);
		}

		Close(fd[1]);
		run_pipe_command(argv, cmdline, pos + 1, fd[0], bg);
		int status;
		if (waitpid(pid, &status, 0) < 0)
			unix_error("waitfg: waitpid error");
	}
}

void sigint_handler(int sig) {
	pid_t pid;
	Sio_puts("\n");
	for (int i = 0; i < MAXJOBS; i++) {
		if (jobs[i].state == FG) {
			pid = jobs[i].pid;
			kill(-pid, SIGKILL);
			clearjob(&jobs[i]);
		}
	}
	return;
}
void sigtstp_handler(int sig) {
	pid_t pid;
	Sio_puts("\n");
	for (int i = 0; i < MAXJOBS; i++) {
		if (jobs[i].state == FG) {
			pid = jobs[i].pid;
			jobs[i].state = ST;
			kill(-pid, SIGTSTP);
			printf("[%d]+  Stopped\t\t\t%s", jobs[i].id, jobs[i].cmdline);
		}
	}

	return;
}

void sigchld_handler(int sig) {
	pid_t pid;
	int status, id;
	job *job;
	char line[MAXLINE];
	while ((pid = waitpid(-1, &status, WNOHANG | WCONTINUED)) > 0) {
		if (WIFEXITED(status)) {
			donejob(jobs, pid);
		}
		else if (WIFSIGNALED(status)) {
			donejob(jobs, pid);
		}

	}
	return;
}

void initjobs(job *jobs) {
	for (int i = 0; i < MAXJOBS; i++) {
		clearjob(&jobs[i]);
	}
}
int addjob(job *jobs, pid_t pid, int state, char *cmdline) {
	if (pid < 1)
		return 0;
	if (jobs[cur_jobid].state == DEFAULT) {
		jobs[cur_jobid].pid = pid;
		jobs[cur_jobid].state = state;
		strcpy(jobs[cur_jobid].cmdline, cmdline);
		jobs[cur_jobid].id = cur_jobid+1;
		cur_jobid++;
		return 1;	/* success addjob */
	}

	return 0;
}
void donejob(job *jobs, pid_t pid) {
	if (pid < 1)
		return;

	for (int i = 0; i < MAXJOBS; i++) {
		if (jobs[i].pid == pid) {
			jobs[i].state = DONE;
			break;
		}
	}
}

void clearjob(job *job) {
	job->pid = 0;
	job->id = 0;
	job->state = DEFAULT;
	job->cmdline[0] = '\0';
}

void printjobs(job *jobs) {
	for (int i = 0; i < MAXJOBS; i++) {
		if (jobs[i].state == BG) {
			printf("[%d]   Running\t\t\t%s", jobs[i].id, jobs[i].cmdline);
		}
		else if (jobs[i].state == ST) {
			printf("[%d]   Stopped\t\t\t%s", jobs[i].id, jobs[i].cmdline);
		}
		else if (jobs[i].state == FG) {
			printf("[%d]   FG\t\t\t%s", jobs[i].id, jobs[i].cmdline);
		}

	}
}

void delNprint_done(job *jobs) {
	char *delim;
	for (int i = 0; i < MAXJOBS; i++) {
		if (jobs[i].state == DONE) {
			if (delim = strchr(jobs[i].cmdline, '&')) {
				*delim = '\n';
				delim += 1;
				*delim = '\0';
			}
			printf("[%d]   Done\t\t\t%s", jobs[i].id, jobs[i].cmdline);
			clearjob(&jobs[i]);
		}
		else if (jobs[i].state == KILL) {
			if (delim = strchr(jobs[i].cmdline, '&')) {
				*delim = '\n';
				delim += 1;
				*delim= '\0';
			}
			printf("[%d]   Terminated\t\t\t%s", jobs[i].id, jobs[i].cmdline);
			clearjob(&jobs[i]);
		}
	}
	for (int i = MAXJOBS - 1; i >= 0; i--) {
		if (jobs[i].state != DEFAULT) {
			cur_jobid = i + 1;
			return;
		}
	}
	cur_jobid = 0;
}

void waitfg(pid_t pid)
{
	pid_t f_pid;
	int id;
	for (int i = 0; i < MAXJOBS; i++) {
		if (jobs[i].state == FG) {
			f_pid = jobs[i].pid;
			id = jobs[i].id;
			break;
		}
	}


	while (1) {
		if (jobs[id - 1].state != FG)
			break;

		sleep(1);
	}
	return;

}
void checkflag(char **argv) {
	
	if (!strcmp(argv[0], "cat") && (argv[1] == NULL))
		flag =  1;
	if (!strcmp(argv[0], "less"))
		flag =  1;

	return;
}
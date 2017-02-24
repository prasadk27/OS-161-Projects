/*
 * sty1.c
 *
 * 	The original sty is changed by making to run only one hog.
 *
 * This test should itself run correctly when the basic system calls
 * are complete. It may be helpful for scheduler performance analysis.
 */

#include <unistd.h>
#include <err.h>

static char *hargv[2] = { (char *)"hog", NULL };

#define MAXPROCS  1
static int pids[MAXPROCS], npids;

static
void
hog(void)
{
	int pid = fork();
	switch (pid) {
	    case -1:
		err(1, "fork");
	    case 0:
		/* child */
		execv("/testbin/hog", hargv);
		err(1, "/testbin/hog");
	    default:
		/* parent */
		pids[npids++] = pid;
		break;
	}
}

static
void
waitall(void)
{
	int i, status;
	for (i=0; i<npids; i++) {
		if (waitpid(pids[i], &status, 0)<0) {
			warn("waitpid for %d", pids[i]);
		}
		else if (status != 0) {
			warnx("pid %d: exit %d", pids[i], status);
		}
	}
}

int
main()
{
	hog();


	waitall();
	return 0;
}

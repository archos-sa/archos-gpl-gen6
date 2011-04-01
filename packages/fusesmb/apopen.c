#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>
#include <syslog.h>

#define LOCK_CHILD_LIST		pthread_mutex_lock(&apopen_mutex)
#define UNLOCK_CHILD_LIST	pthread_mutex_unlock(&apopen_mutex)

#define DBG if (0)

pthread_mutex_t apopen_mutex = PTHREAD_MUTEX_INITIALIZER;

// ripped from uclibc popen.c
struct popen_list_item {
	struct popen_list_item *next;
	FILE *f;
	pid_t pid;
};

static struct popen_list_item *popen_list /* = NULL (bss initialized) */;

FILE *apopen(const char *command, const char *modes)
{
	FILE *fp;
	struct popen_list_item *pi;
	struct popen_list_item *po;
	int pipe_fd[2];
	int parent_fd;
	int child_fd;
	int child_writing;			/* Doubles as the desired child fildes. */
	pid_t pid;

	child_writing = 0;			/* Assume child is writing. */
	if (modes[0] != 'w') {			/* Parent not writing... */
		++child_writing;		/* so child must be writing. */
		if (modes[0] != 'r') {		/* Oops!  Parent not reading either! */
			goto RET_NULL;
		}
	}

	if (!(pi = malloc(sizeof(struct popen_list_item)))) {
		goto RET_NULL;
	}

	if (pipe(pipe_fd)) {
		goto FREE_PI;
	}

	child_fd = pipe_fd[child_writing];
	parent_fd = pipe_fd[1-child_writing];

	if (!(fp = fdopen(parent_fd, modes))) {
		close(parent_fd);
		close(child_fd);
		goto FREE_PI;
	}

	LOCK_CHILD_LIST;
	if ((pid = fork()) == 0) {	/* Child of fork... */
		close(parent_fd);
		if (child_fd != child_writing) {
			dup2(child_fd, child_writing);
			close(child_fd);
		}

		// become a process group leaded
		printf("settting pg\n");
		if ( setpgid(0, 0) ) {
			printf("setpgid failed: %s\n", strerror(errno));
		}

		/* SUSv3 requires that any previously popen()'d streams in the
		 * parent shall be closed in the child. */
		for (po = popen_list ; po ; po = po->next) {
			close(fileno(po->f));
		}
		execl("/bin/sh", "sh", "-c", command, (char *)0);

		/* SUSv3 mandates an exit code of 127 for the child if the
		 * command interpreter can not be invoked. */
		_exit(127);
	}

	/* We need to close the child filedes whether fork failed or
	 * it succeeded and we're in the parent. */
	close(child_fd);

	if (pid > 0) {				/* Parent of vfork... */
		DBG syslog(LOG_DEBUG, "putting %i into the child list\n", pid);
		pi->pid = pid;
		pi->f = fp;
		//LOCK;
		pi->next = popen_list;
		popen_list = pi;
		UNLOCK_CHILD_LIST;
		return fp;
	}
        UNLOCK_CHILD_LIST;

	/* If we get here, fork failed. */
	fclose(fp);					/* Will close parent_fd. */

 FREE_PI:
	free(pi);

 RET_NULL:
	return NULL;
}

int apclose(FILE *stream)
{
	struct popen_list_item *p;
	int stat;
	pid_t pid;

	/* First, find the list entry corresponding to stream and remove it
	 * from the list.  Set p to the list item (NULL if not found). */
	LOCK_CHILD_LIST;
	if ((p = popen_list) != NULL) {
		if (p->f == stream) {
			popen_list = p->next;
		} else {
			struct popen_list_item *t;
			do {
				t = p;
				if (!(p = t->next)) {
					//__set_errno(EINVAL); /* Not required by SUSv3. */
					break;
				}
				if (p->f == stream) {
					t->next = p->next;
					break;
				}
			} while (1);
		}
	}
	//UNLOCK;

	if (p) {
		pid = p->pid;			/* Save the pid we need */
		free(p);				/* and free the list item. */

		fclose(stream);	/* The SUSv3 example code ignores the return. */

		/* SUSv3 specificly requires that pclose not return before the child
		 * terminates, in order to disallow pclose from returning on EINTR. */
		do {
			if (waitpid(pid, &stat, 0) >= 0) {
				UNLOCK_CHILD_LIST;
				DBG syslog(LOG_DEBUG, "removed %i from the child list\n", pid);
				return stat;
			}
			if (errno != EINTR) {
				break;
			}
		} while (1);
	}
        UNLOCK_CHILD_LIST;
	DBG syslog(LOG_DEBUG, "something was wrong\n");

	return -1;
}

void apopen_shutdown(void)
{
    LOCK_CHILD_LIST;
    // walk the child list and kill'em all!
    struct popen_list_item *cursor = popen_list;
    while ( cursor ) {
        DBG syslog(LOG_DEBUG, "about to kill %i\n", cursor->pid);
        if ( kill( -(cursor->pid), SIGKILL) ) { // send a signal to every process inside a group
            DBG syslog(LOG_DEBUG, "kill(%i, SIGKILL) failed: %s\n", cursor->pid, strerror(errno));
        }
        cursor = cursor->next;
    }
    DBG syslog(LOG_DEBUG, "all processes got their kill\n");

    // afterwards pick up the remains
    cursor = popen_list;
    while ( cursor ) {
        DBG syslog(LOG_DEBUG, "waiting %i\n", cursor->pid);
        if ( waitpid(cursor->pid, NULL, 0) == -1 ) {
            DBG syslog(LOG_DEBUG, "waitpid for %i failed: %s\n", cursor->pid, strerror(errno));
        }
        cursor = cursor->next;
    }
    UNLOCK_CHILD_LIST;
}

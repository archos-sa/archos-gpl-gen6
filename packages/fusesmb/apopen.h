#include <stdio.h>
#include <pthread.h>

#define LOCK_CHILD_LIST		pthread_mutex_lock(&mylock)
#define UNLOCK_CHILD_LIST	pthread_mutex_unlock(&mylock)

FILE *apopen(const char *command, const char *modes);
int apclose(FILE *stream);
void apopen_shutdown(void);

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <libsmbclient.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/param.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <syslog.h>
#include <dirent.h>

#include "stringlist.h"
#include "smbctx.h"
#include "hash.h"
#include "configfile.h"
#include "apopen.h"
#include "debug.h"

#  define index(s,c) strchr((s), (c))
#  define rindex(s,c) strrchr((s), (c))


#define MAX_SERVERLEN 255
#define MAX_WGLEN 255

#define DBG if (0)

static void *wg_scanner(void *arg);

char pidfile[1024];

struct fusesmb_cache_opt {
    stringlist_t *ignore_servers;
    stringlist_t *ignore_workgroups;
};


config_t cfg;
struct fusesmb_cache_opt opts;

static void options_read(config_t *cfg, struct fusesmb_cache_opt *opt)
{
    opt->ignore_servers = NULL;
    if (-1 == config_read_stringlist(cfg, "ignore", "servers", &(opt->ignore_servers), ','))
    {
        opt->ignore_servers = NULL;
    }
    opt->ignore_workgroups = NULL;
    if (-1 == config_read_stringlist(cfg, "ignore", "workgroups", &(opt->ignore_workgroups), ','))
    {
        opt->ignore_workgroups = NULL;
    }
}

static void options_free(struct fusesmb_cache_opt *opt)
{
    if (NULL != opt->ignore_servers)
    {
        sl_free(opt->ignore_servers);
    }
    if (NULL != opt->ignore_workgroups)
    {
        sl_free(opt->ignore_workgroups);
    }
}

static int is_process_alive(const char* pidfile)
{
	FILE *fp = fopen(pidfile, "r");
	if (fp == NULL)
		return -1;

	int pid;
	if (fscanf(fp, "%i", &pid) != 1) {
		printf("Failed to read pid file (%s:%i).\n",
			__FILE__, __LINE__);
		fclose(fp);
		return -1;
	}
	fclose(fp);

	// make sure it's a potentially good
	if (pid <= 0) {
		printf("Something is fishy with this pid %i (%s:%i)",
			pid, __FILE__, __LINE__);
		return -1;
	}

	int ret = kill(pid, 0);
	if (ret == -1 && errno == ESRCH) {
		return 0;
	}
	else if ( ret == -1) {
		return -1;
	}
	else {
		return 1;
	}
}

/*
 * Some servers refuse to return a server list using libsmbclient, so using
 *  broadcast lookup through nmblookup
 */
static int nmblookup(const char *wg, stringlist_t *sl, hash_t *ipcache)
{
DBG syslog(LOG_DEBUG, "tid: %i # %s start", (int)pthread_self(), __FUNCTION__);
    /* Find all ips for the workgroup by running :
    $ nmblookup 'workgroup_name'
    */
    char wg_cmd[512];
    snprintf(wg_cmd, 512, "nmblookup '%s'", wg);

    DBG syslog(LOG_DEBUG, "calling \"%s\"\n", wg_cmd);
    FILE *pipe = apopen(wg_cmd, "r");
DBG syslog(LOG_DEBUG, "tid: %i # %s: %i", (int)pthread_self(), __FUNCTION__, __LINE__);
    if (pipe == NULL)
        return -1;

    int ip_cmd_size = 8192;
    char *ip_cmd = (char *)malloc(ip_cmd_size * sizeof(char));
    if (ip_cmd == NULL)
        return -1;
    strcpy(ip_cmd, "nmblookup -A ");
    int ip_cmd_len = strlen(ip_cmd);
    while ( !ferror(pipe) && !feof(pipe))
    {
        /* Parse output that looks like this:
        querying boerderie on 172.20.91.255
        172.20.89.134 boerderie<00>
        172.20.89.191 boerderie<00>
        172.20.88.213 boerderie<00>
        */
        char buf[4096];
        if (NULL == fgets(buf, 4096, pipe)) {
            break;
        }

        char *pip = buf;
        /* Yes also include the space */
        while (isdigit(*pip) || *pip == '.' || *pip == ' ')
        {
            pip++;
        }
        *pip = '\0';
        int len = strlen(buf);
        if (len == 0) continue;
        ip_cmd_len += len;
        if (ip_cmd_len >= (ip_cmd_size -1))
        {
            ip_cmd_size *= 2;
            char *tmp = realloc(ip_cmd, ip_cmd_size *sizeof(char));
            if (tmp == NULL)
            {
                ip_cmd_size /= 2;
                ip_cmd_len -= len;
                continue;
            }
            ip_cmd = tmp;
        }
        /* Append the ip to the command:
        $ nmblookup -A ip1 ... ipn
        */
        strcat(ip_cmd, buf);
    }
    apclose(pipe);
DBG syslog(LOG_DEBUG, "tid: %i # %s: %i", (int)pthread_self(), __FUNCTION__, __LINE__);

    if (strlen(ip_cmd) == 13)
    {
        free(ip_cmd);
        return 0;
    }
    debug("%s\n", ip_cmd);

    DBG syslog(LOG_DEBUG, "calling \"%s\"\n", ip_cmd);
DBG syslog(LOG_DEBUG, "tid: %i # %s: %i", (int)pthread_self(), __FUNCTION__, __LINE__);
    pipe = apopen(ip_cmd, "r");
DBG syslog(LOG_DEBUG, "tid: %i # %s: %i", (int)pthread_self(), __FUNCTION__, __LINE__);
    if (pipe == NULL)
    {
        free(ip_cmd);
        return -1;
    }

    while ( !ferror(pipe) && !feof(pipe) )
    {
        char buf2[4096];
        char buf[4096];
        char ip[32];

        char *start = buf;
        if (NULL == fgets(buf2, 4096, pipe)) {
            break;
        }
        /* Parse following input:
            Looking up status of 123.123.123.123
                    SERVER          <00> -         B <ACTIVE>
                    SERVER          <03> -         B <ACTIVE>
                    SERVER          <20> -         B <ACTIVE>
                    ..__MSBROWSE__. <01> - <GROUP> B <ACTIVE>
                    WORKGROUP       <00> - <GROUP> B <ACTIVE>
                    WORKGROUP       <1d> -         B <ACTIVE>
                    WORKGROUP       <1e> - <GROUP> B <ACTIVE>
        */
        if (strncmp(buf2, "Looking up status of ", strlen("Looking up status of ")) == 0)
        {
            char *tmp = rindex(buf2, ' ');
            tmp++;
            char *end = index(tmp, '\n');
            *end = '\0';
            strcpy(ip, tmp);
            debug("%s", ip);
        }
        else
        {
            continue;
        }

        while ( !ferror(pipe) && !feof(pipe) )
        {
            if (NULL == fgets(buf, 4096, pipe))
                break;
            char *sep = buf;

            if (*buf != '\t')
                break;
            if (NULL != strstr(buf, "<GROUP>"))
                break;
            if (NULL == (sep = strstr(buf, "<00>")))
                break;
            *sep = '\0';

            start++;

            while (*sep == '\t' || *sep == ' ' || *sep == '\0')
            {
                *sep = '\0';
                sep--;
            }
            sl_add(sl, start, 1);
            if (NULL == hash_lookup(ipcache, start))
                hash_alloc_insert(ipcache, strdup(start), strdup(ip));
            debug("%s : %s", ip, start);
        }

    }
    apclose(pipe);
    free(ip_cmd);
DBG syslog(LOG_DEBUG, "tid: %i # %s: finished", (int)pthread_self(), __FUNCTION__);
    return 0;
}

static int server_listing(SMBCCTX *ctx, FILE *tmp_file, const char *wg, const char *sv, const char *ip)
{
    char tmp_path[MAXPATHLEN] = "smb://";
    if (ip != NULL)
    {
        strcat(tmp_path, ip);
    }
    else
    {
        strcat(tmp_path, sv);
    }
DBG syslog(LOG_DEBUG, "%s working on server: '%s' ", __FUNCTION__, tmp_path);

    struct smbc_dirent *share_dirent;
    SMBCFILE *dir;
    dir = ctx->opendir(ctx, tmp_path);
    if (dir == NULL)
    {
DBG syslog(LOG_DEBUG, "ctx->opendir failed (%s)\n", strerror(errno));
        ctx->closedir(ctx, dir);
        fprintf(tmp_file, "/%s/%s\n", wg, sv);
DBG syslog(LOG_DEBUG, "%s: we arent allowd to browse the shares", __FUNCTION__);
        return -1;
    }

    while ( NULL != (share_dirent = ctx->readdir(ctx, dir)) )
    {
DBG syslog(LOG_DEBUG, "%s: %i", __FUNCTION__, __LINE__);
        if (//share_dirent->name[strlen(share_dirent->name)-1] == '$' ||
            share_dirent->smbc_type != SMBC_FILE_SHARE ||
            share_dirent->namelen == 0)
        {
            continue;
        }

        if (0 == strcmp("ADMIN$", share_dirent->name) ||
            0 == strcmp("print$", share_dirent->name))
        {
            continue;
        }

        fprintf(tmp_file, "/%s/%s/%s\n", wg, sv, share_dirent->name);
    }
    ctx->closedir(ctx, dir);
DBG syslog(LOG_DEBUG, "%s finished", __FUNCTION__);
    return 0;
}

static int create_tmp_file(FILE **tmp, pid_t pid)
{
    char buf[128];
    snprintf(buf, 128, "/tmp/fusesmbcache.tmp%i", pid);
    *tmp = fopen(buf, "w");
    return *tmp == NULL;
}

// might be left if a previous scanner run was killed
static void cleanup_stale_tmp_files(void)
{
	char buf[128];
	DIR *tmpd = opendir("/tmp/");
	if ( !tmpd ) {
		return;
	}

	struct dirent *dentry;
	while ( (dentry = readdir(tmpd)) ) {
		if ( !strncmp(dentry->d_name, "fusesmbcache.tmp", 16) ) {
			snprintf(buf, 128, "/tmp/%s", dentry->d_name);
			unlink(buf);
		}
	}
	closedir(tmpd);
}

pid_t scanner_pid = 0;
static void *workgroup_listing(char *wg)
{
    sigset_t blocked;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGUSR1);
    sigaddset(&blocked, SIGUSR2);
    sigaddset(&blocked, SIGPIPE);
    sigaddset(&blocked, SIGHUP);
    sigaddset(&blocked, SIGQUIT);
    sigaddset(&blocked, SIGTERM);

    if ( pthread_sigmask(SIG_SETMASK, &blocked, NULL) ) {
        DBG syslog(LOG_DEBUG, "sigmask failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    pthread_t wg_scanner_shutdown_thread;
    scanner_pid = getpid();
    int ret = pthread_create(&wg_scanner_shutdown_thread, NULL, wg_scanner, wg);
    if ( ret ) {
        DBG syslog(LOG_DEBUG, "failed to start the wg_scanner_shutdown_thread thread\n");
        exit(EXIT_FAILURE);
    }

    int signum;
    sigwait(&blocked, &signum);
    DBG syslog(LOG_DEBUG, "received signal %i", signum);
    apopen_shutdown();
    exit(EXIT_FAILURE);
}

static void *wg_scanner(void *arg)
{
DBG syslog(LOG_DEBUG, "workgroup_listing starting to run");
    char *wg = (char*)arg;

    hash_t *ip_cache = hash_create(HASHCOUNT_T_MAX, NULL, NULL);
    if (NULL == ip_cache)
        exit(EXIT_FAILURE);

    stringlist_t *servers = sl_init();
    if (NULL == servers)
    {
        DBG syslog(LOG_DEBUG, "Malloc failed\n");
        exit(EXIT_FAILURE);
    }

    FILE *tmp_file;
    if ( create_tmp_file(&tmp_file, scanner_pid) ) {
        exit(EXIT_FAILURE);
    }

    SMBCCTX *ctx = fusesmb_cache_new_context(&cfg);
    SMBCFILE *dir;
    char temp_path[MAXPATHLEN] = "smb://";
    strcat(temp_path, wg);
DBG syslog(LOG_DEBUG, "%s: Looking up Workgroup: %s", __FUNCTION__, wg);
    struct smbc_dirent *server_dirent;
DBG syslog(LOG_DEBUG, "%s: opendir %s", __FUNCTION__, temp_path);
    dir = ctx->opendir(ctx, temp_path);
DBG syslog(LOG_DEBUG, "%s: ready", __FUNCTION__);
    if (dir == NULL)
    {
DBG syslog(LOG_DEBUG, "%s: Using the nmblookup method for '%s'", __FUNCTION__, wg);
        ctx->closedir(ctx, dir);
        nmblookup(wg, servers, ip_cache);
DBG syslog(LOG_DEBUG, "%s: nmblookup ready", __FUNCTION__);
    }
    else {
DBG syslog(LOG_DEBUG, "%s: Using the ctx->readdir method for '%s'", __FUNCTION__, wg);
        while (NULL != (server_dirent = ctx->readdir(ctx, dir)))
        {
            if (server_dirent->namelen == 0 ||
                server_dirent->smbc_type != SMBC_SERVER)
            {
                continue;
            }

            if (-1 == sl_add(servers, server_dirent->name, 1))
                continue;
DBG syslog(LOG_DEBUG, "%s: added server: '%s' to our todo list", __FUNCTION__, server_dirent->name);
        }
        ctx->closedir(ctx, dir);
DBG syslog(LOG_DEBUG, "%s ready", __FUNCTION__);
    }
    sl_casesort(servers);

    size_t i;
    for (i=0; i < sl_count(servers); i++)
    {
        /* Skip duplicates */
        if (i > 0 && strcmp(sl_item(servers, i), sl_item(servers, i-1)) == 0)
            continue;

        /* Check if this server is in the ignore list in fusesmb.conf */
        if (NULL != opts.ignore_servers)
        {
            if (NULL != sl_find(opts.ignore_servers, sl_item(servers, i)))
            {
                debug("Ignoring %s", sl_item(servers, i));
                continue;
            }
        }
        char sv[1024] = "/";
        strcat(sv, sl_item(servers, i));
        int ignore = 0;

        /* Check if server specific option says ignore */
        if (0 == config_read_bool(&cfg, sv, "ignore", &ignore))
        {
            if (ignore == 1)
                continue;
        }
        hnode_t *node = hash_lookup(ip_cache, sl_item(servers, i));
        if (node == NULL)
            server_listing(ctx, tmp_file, wg, sl_item(servers, i), NULL);
        else
            server_listing(ctx, tmp_file, wg, sl_item(servers, i), hnode_get(node));
    }
DBG syslog(LOG_DEBUG, "%s: finished listing servers", __FUNCTION__);
    hscan_t sc;
    hnode_t *n;
    hash_scan_begin(&sc, ip_cache);
    while (NULL != (n = hash_scan_next(&sc)))
    {
        void *data = hnode_get(n);
        const void *key = hnode_getkey(n);
        hash_scan_delfree(ip_cache, n);
        free((void *)key);
        free(data);

    }
    hash_destroy(ip_cache);
    sl_free(servers);
    smbc_free_context(ctx, 1);
    fclose(tmp_file);
DBG syslog(LOG_DEBUG, "%s: finished", __FUNCTION__);
    exit(EXIT_SUCCESS);
}

static int read_tmp_file(pid_t pid, stringlist_t *cache)
{
    FILE *tmp_cache;
    char path[128];
    snprintf(path, 128, "/tmp/fusesmbcache.tmp%i", pid);
    tmp_cache = fopen(path, "r");
    if ( !tmp_cache ) {
DBG syslog(LOG_DEBUG, "%s: failed to open tmp file '%s'", __FUNCTION__, path);
        return -1;
    }

    char buf[1024];
    int len;
    while ( fgets(buf, 1024, tmp_cache ) ) {
        len = strlen(buf);
        if ( len >= 6 ) { // "/a/b\n" is the minimum
            if ( buf[len - 1] == '\n' ) {
                buf[len - 1] = '\0';
            }
            sl_add(cache, buf, 1);
        }
    }
    fclose(tmp_cache);
    if (unlink(path)) {
DBG syslog(LOG_DEBUG, "%s: removing tmp file failed. (%s)", __FUNCTION__, strerror(errno));
    }
}

pthread_mutex_t processe_ctrl_lock = PTHREAD_MUTEX_INITIALIZER;
unsigned int num_processes = 0;
pid_t *processes = NULL;
stringlist_t *cache;

static int write_cache_to_disk(void)
{
    sl_casesort(cache);
    char cachefile[1024];
    char tmp_cachefile[1024];
#ifndef ARCHOS
    snprintf(tmp_cachefile, 1024, "%s/.smb/fusesmb.cache.XXXXXX", getenv("HOME"));
#else
    snprintf(tmp_cachefile, 1024, "/var/cache/fusesmb.cache.XXXXXX");
#endif
    mkstemp(tmp_cachefile);
#ifndef ARCHOS
    snprintf(cachefile, 1024, "%s/.smb/fusesmb.cache", getenv("HOME"));
#else
    snprintf(cachefile, 1024, "/var/cache/fusesmb.cache");
#endif
    mode_t oldmask;
    oldmask = umask(022);
    FILE *fp = fopen(tmp_cachefile, "w");
    umask(oldmask);
    if (fp == NULL)
    {
        sl_free(cache);
        return -1;
    }

DBG syslog(LOG_DEBUG, "dumping cache\n");
    unsigned int i;
    for ( i= 0; i < sl_count(cache); i++)
    {
        fprintf(fp, "%s\n", sl_item(cache, i));
DBG syslog(LOG_DEBUG, "%s\n", sl_item(cache, i));
    }
    fclose(fp);
    /* Make refreshing cache file atomic */
    rename(tmp_cachefile, cachefile);
    sl_free(cache);
}

int cache_servers(SMBCCTX *ctx)
{
DBG syslog(LOG_DEBUG, "tid: %i # %s: start", (int)pthread_self(), __FUNCTION__);

    SMBCFILE *dir;
    struct smbc_dirent *workgroup_dirent;

DBG syslog(LOG_DEBUG, "ctx->opendir(ctx, \"smb://\");");

    dir = ctx->opendir(ctx, "smb://");

    if (dir == NULL)
    {
        ctx->closedir(ctx, dir);
        return -1;
    }

DBG syslog(LOG_DEBUG, "ctx->opendir(ctx, \"smb://\"); READY!");

    processes = (pid_t*)malloc(sizeof(pid_t));
    if (NULL == processes) {
        return -1;
    }

    while (NULL != (workgroup_dirent = ctx->readdir(ctx, dir)) )
    {
        if (workgroup_dirent->namelen == 0 ||
            workgroup_dirent->smbc_type != SMBC_WORKGROUP)
        {
            continue;
DBG syslog(LOG_DEBUG, "tid: %i # %s: %i", (int)pthread_self(), __FUNCTION__, __LINE__);
        }

        if (opts.ignore_workgroups != NULL)
        {
            if ( NULL != sl_find(opts.ignore_workgroups, workgroup_dirent->name) )
            {
                debug("Ignoring Workgroup: %s", workgroup_dirent->name);
                continue;
DBG syslog(LOG_DEBUG, "tid: %i # %s: %i", (int)pthread_self(), __FUNCTION__, __LINE__);
            }
        }

DBG syslog(LOG_DEBUG, "tid: %i # %s: %i", (int)pthread_self(), __FUNCTION__, __LINE__);
        pthread_mutex_lock(&processe_ctrl_lock);
        int pid = fork();
        if ( pid == - 1 )       // error
        {
DBG syslog(LOG_DEBUG, "Failed to create process for workgroup: %s", workgroup_dirent->name);
            pthread_mutex_unlock(&processe_ctrl_lock);
            continue;
        }
        else if ( pid == 0 )    // child
        {
            workgroup_listing(workgroup_dirent->name);
        }
        else                    // parent
        {
            processes[num_processes] = pid;
            num_processes++;
            pid_t *tmp = (pid_t *)realloc(processes, ( num_processes + 1 ) * sizeof(pid_t)); 
            if ( !tmp )
            {
                return -1;  // TODO: shutdown all other processes
            }
            processes = tmp;
            pthread_mutex_unlock(&processe_ctrl_lock);
        }
    }
    ctx->closedir(ctx, dir);

DBG syslog(LOG_DEBUG, "we have to wait for %i processes", num_processes);

    cache = sl_init();

    int still_waiting = num_processes;
    while ( still_waiting )
    {
        int pid = wait(NULL);
        if ( pid == -1 )
        {
            DBG syslog(LOG_DEBUG, "Error while waiting for pid %i", pid);
        }
        else {
            still_waiting--;
            read_tmp_file(pid, cache);
        }
    }
    free(processes);

    write_cache_to_disk();
    return 0;
}

static pthread_t scanner_ctrl_thread;
static void* run_scanner_ctrl(void *arg)
{
    (void)arg;
    SMBCCTX *ctx = fusesmb_cache_new_context(&cfg);
    if ( cache_servers(ctx) == -1 ) {  // empty cache file for the error case/no smb servers.
    char cachefile[1024];
#ifndef ARCHOS
        snprintf(cachefile, 1024, "%s/.smb/fusesmb.cache", getenv("HOME"));
#else
        snprintf(cachefile, 1024, "/var/cache/fusesmb.cache");
#endif
        umask(022);
        FILE *fp = fopen(cachefile, "w");
        if (fp == NULL)
        {
            exit(EXIT_FAILURE);
        }
        fclose(fp);
    }
    smbc_free_context(ctx, 1);
    options_free(&opts);
    unlink(pidfile);
DBG closelog();
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
DBG openlog("fusesmb.cache", LOG_CONS | LOG_PID, LOG_USER);

#ifndef ARCHOS
    snprintf(pidfile, 1024, "%s/.smb/fusesmb.cache.pid", getenv("HOME"));
#else
    snprintf(pidfile, 1024, "/var/run/fusesmb.cache.pid");
#endif

    char configfile[1024];
#ifndef ARCHOS
    snprintf(configfile, 1024, "%s/.smb/fusesmb.conf", getenv("HOME"));
#else
    snprintf(configfile, 1024, "/mnt/system/etc/fusesmb.conf");
#endif
    if (-1 == config_init(&cfg, configfile))
    {
        DBG syslog(LOG_DEBUG, "Could not open config file: %s (%s)", configfile, strerror(errno));
        exit(EXIT_FAILURE);
    }
    options_read(&cfg, &opts);

    struct stat st;
    if (argc == 1)
    {
        pid_t pid, sid;

        if (-1 != stat(pidfile, &st))
        {
            if (is_process_alive(pidfile) == 0)
                unlink(pidfile);
            else
            {
                DBG syslog(LOG_DEBUG, "Error: %s is already running\n", argv[0]);
                exit(EXIT_FAILURE);
            }
        }

        pid = fork();
        if (pid < 0)
            exit(EXIT_FAILURE);
        if (pid > 0)
            exit(EXIT_SUCCESS);

        sid = setsid();
        if (sid < 0) {
            exit(EXIT_FAILURE);
        }
        if (chdir("/") < 0)
            exit(EXIT_FAILURE);

        mode_t oldmask;
        oldmask = umask(077);
        FILE *fp = fopen(pidfile, "w");
        umask(oldmask);
        if (NULL == fp)
            exit(EXIT_FAILURE);
        fprintf(fp, "%i\n", sid);
        fclose(fp);

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    sigset_t blocked;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGUSR1);
    sigaddset(&blocked, SIGUSR2);
    sigaddset(&blocked, SIGPIPE);
    sigaddset(&blocked, SIGHUP);
    sigaddset(&blocked, SIGQUIT);
    sigaddset(&blocked, SIGTERM);
    if ( pthread_sigmask(SIG_SETMASK, &blocked, NULL) ) {
        DBG syslog(LOG_DEBUG, "sigmask failed: %s", strerror(errno));
    }

    FILE *fp = fopen(pidfile, "w");
    if (NULL == fp) {
        exit(EXIT_FAILURE);
    }
    fprintf(fp, "%i\n", getpid());
    fclose(fp);

    cleanup_stale_tmp_files();

    int ret = pthread_create(&scanner_ctrl_thread, NULL, run_scanner_ctrl, NULL);
    if ( ret ) {
        DBG syslog(LOG_DEBUG, "failed to start the scanner_ctrl thread\n");
        exit(EXIT_FAILURE);
    }

    int signum;
    sigwait(&blocked, &signum);
    DBG syslog(LOG_DEBUG, "received signal %i", signum);

    DBG syslog(LOG_DEBUG, "tid: %i # %s: %i killing all", (int)pthread_self(), __FUNCTION__, __LINE__);

    pthread_mutex_lock(&processe_ctrl_lock);
    unsigned int i;
    for ( i = 0; i < num_processes; i++ ) {
        kill(processes[i], SIGTERM);
    }
    pthread_mutex_unlock(&processe_ctrl_lock);
    DBG syslog(LOG_DEBUG, "tid: %i # %s: %i killed all", (int)pthread_self(), __FUNCTION__, __LINE__);

    exit(EXIT_FAILURE);
}

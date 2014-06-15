/* select.c
 * Select Loop
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

/*
#define DEBUG_CALLS
*/

struct thread {
	void (*read_func)(void *);
	void (*write_func)(void *);
	void (*error_func)(void *);
	void *data;
};

static struct thread threads[FD_SETSIZE];

static fd_set w_read;
static fd_set w_write;
static fd_set w_error;

static fd_set x_read;
static fd_set x_write;
static fd_set x_error;

static int w_max;

static int timer_id = 0;

struct timer {
	struct timer *next;
	struct timer *prev;
	ttime interval;
	void (*func)(void *);
	void *data;
	int id;
};

static struct list_head timers = {&timers, &timers};


ttime get_time(void)
{
	struct timeval tv;
	int rs;
	EINTRLOOP(rs, gettimeofday(&tv, NULL));
	if (rs) fatal_exit("gettimeofday failed: %d", errno);
	return (uttime)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

#ifndef OPENVMS
void portable_sleep(unsigned msec)
{
	struct timeval tv;
	int rs;
	tv.tv_sec = msec / 1000;
	tv.tv_usec = msec % 1000 * 1000;
	EINTRLOOP(rs, select(0, NULL, NULL, NULL, &tv));
}
#endif

int can_write(int fd)
{
	fd_set fds;
	struct timeval tv = {0, 0};
	int rs;
	FD_ZERO(&fds);
	if (fd < 0)
		internal("can_write: handle %d", fd);
	if (fd >= (int)FD_SETSIZE) {
		fatal_exit("too big handle %d", fd);
	}
	FD_SET(fd, &fds);
	EINTRLOOP(rs, select(fd + 1, NULL, &fds, NULL, &tv));
	if (rs < 0) fatal_exit("ERROR: select for write (%d) failed: %s", fd, strerror(errno));
	return rs;
}

int can_read_timeout(int fd, int sec)
{
	fd_set fds;
	struct timeval tv;
	int rs;
	tv.tv_sec = sec;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	if (fd < 0)
		internal("can_read: handle %d", fd);
	if (fd >= (int)FD_SETSIZE) {
		fatal_exit("too big handle %d", fd);
	}
	FD_SET(fd, &fds);
	EINTRLOOP(rs, select(fd + 1, &fds, NULL, NULL, &tv));
	if (rs < 0) fatal_exit("ERROR: select for read (%d) failed: %s", fd, strerror(errno));
	return rs;
}

int can_read(int fd)
{
	return can_read_timeout(fd, 0);
}


unsigned long select_info(int type)
{
	int i = 0, j;
	struct cache_entry *ce;
	switch (type) {
		case CI_FILES:
			for (j = 0; j < (int)FD_SETSIZE; j++)
				if (threads[j].read_func || threads[j].write_func || threads[j].error_func) i++;
			return i;
		case CI_TIMERS:
			foreach(ce, timers) i++;
			return i;
		default:
			internal("select_info_info: bad request");
	}
	return 0;
}

struct bottom_half {
	struct bottom_half *next;
	struct bottom_half *prev;
	void (*fn)(void *);
	void *data;
};

static struct list_head bottom_halves = { &bottom_halves, &bottom_halves };

int register_bottom_half(void (*fn)(void *), void *data)
{
	struct bottom_half *bh;
	foreach(bh, bottom_halves) if (bh->fn == fn && bh->data == data) return 0;
	bh = mem_alloc(sizeof(struct bottom_half));
	bh->fn = fn;
	bh->data = data;
	add_to_list(bottom_halves, bh);
	return 0;
}

void unregister_bottom_half(void (*fn)(void *), void *data)
{
	struct bottom_half *bh;
	retry:
	foreach(bh, bottom_halves) if (bh->fn == fn && bh->data == data) {
		del_from_list(bh);
		mem_free(bh);
		goto retry;
	}
}

void check_bottom_halves(void)
{
	struct bottom_half *bh;
	void (*fn)(void *);
	void *data;
	rep:
	if (list_empty(bottom_halves)) return;
	bh = bottom_halves.prev;
	fn = bh->fn;
	data = bh->data;
	del_from_list(bh);
	mem_free(bh);
#ifdef DEBUG_CALLS
	fprintf(stderr, "call: bh %p\n", fn);
#endif
	pr(fn(data)) {
#ifdef OOPS
		free_list(bottom_halves);
		return;
#endif
	};
#ifdef DEBUG_CALLS
	fprintf(stderr, "bh done\n");
#endif
	goto rep;
}

#define CHK_BH if (!list_empty(bottom_halves)) check_bottom_halves()
		
static ttime last_time;

static void check_timers(void)
{
	ttime interval = (uttime)get_time() - (uttime)last_time;
	struct timer * volatile t;	/* volatile because of setjmp */
	foreach(t, timers) t->interval -= (uttime)interval;
	/*ch:*/
	foreach(t, timers) if (t->interval <= 0) {
		struct timer *tt;
#ifdef DEBUG_CALLS
		fprintf(stderr, "call: timer %p\n", t->func);
#endif
		pr(t->func(t->data)) {
#ifdef OOPS
			del_from_list((struct timer *)timers.next);
			return;
#endif
		}
#ifdef DEBUG_CALLS
		fprintf(stderr, "timer done\n");
#endif
		CHK_BH;
		tt = t->prev;
		del_from_list(t);
		mem_free(t);
		t = tt;
	} else break;
	last_time += (uttime)interval;
}

int install_timer(ttime t, void (*func)(void *), void *data)
{
	struct timer *tm, *tt;
	tm = mem_alloc(sizeof(struct timer));
	tm->interval = t;
	tm->func = func;
	tm->data = data;
	new_id:
	tm->id = timer_id;
	timer_id++;
	if (timer_id == MAXINT) timer_id = 0;
	foreach(tt, timers) if (tt->id == tm->id) goto new_id;
	foreach(tt, timers) if (tt->interval >= t) break;
	add_at_pos(tt->prev, tm);
	return tm->id;
}

void kill_timer(int id)
{
	struct timer *tm;
	int k = 0;
	foreach(tm, timers) if (tm->id == id) {
		struct timer *tt = tm;
		del_from_list(tm);
		tm = tm->prev;
		mem_free(tt);
		k++;
	}
	if (!k) internal("trying to kill nonexisting timer");
	if (k >= 2) internal("more timers with same id");
}

void *get_handler(int fd, int tp)
{
	if (fd < 0)
		internal("get_handler: handle %d", fd);
	if (fd >= (int)FD_SETSIZE) {
		fatal_exit("too big handle %d", fd);
		return NULL;
	}
	switch (tp) {
		case H_READ:	return (void *)threads[fd].read_func;
		case H_WRITE:	return (void *)threads[fd].write_func;
		case H_ERROR:	return (void *)threads[fd].error_func;
		case H_DATA:	return threads[fd].data;
	}
	internal("get_handler: bad type %d", tp);
	return NULL;
}

void set_handlers(int fd, void (*read_func)(void *), void (*write_func)(void *), void (*error_func)(void *), void *data)
{
#ifdef __GNU__
	/* GNU Hurd has bugs w.r.t. exceptions */
	error_func = NULL;
#endif
	if (fd < 0)
		internal("set_handlers: handle %d", fd);
	if (fd >= (int)FD_SETSIZE) {
		fatal_exit("too big handle %d", fd);
		return;
	}
	threads[fd].read_func = read_func;
	threads[fd].write_func = write_func;
	threads[fd].error_func = error_func;
	threads[fd].data = data;
	if (read_func) FD_SET(fd, &w_read);
	else {
		FD_CLR(fd, &w_read);
		FD_CLR(fd, &x_read);
	}
	if (write_func) FD_SET(fd, &w_write);
	else {
		FD_CLR(fd, &w_write);
		FD_CLR(fd, &x_write);
	}
	if (error_func) FD_SET(fd, &w_error);
	else {
		FD_CLR(fd, &w_error);
		FD_CLR(fd, &x_error);
	}
	if (read_func || write_func || error_func) {
		if (fd >= w_max) w_max = fd + 1;
	} else if (fd == w_max - 1) {
		int i;
		for (i = fd - 1; i >= 0; i--)
			if (FD_ISSET(i, &w_read) || FD_ISSET(i, &w_write) ||
			    FD_ISSET(i, &w_error)) break;
		w_max = i + 1;
	}
}

void clear_events(int h, int blocking)
{
#if !defined(O_NONBLOCK) && !defined(FIONBIO)
	blocking = 1;
#endif
	while (blocking ? can_read(h) : 1) {
		unsigned char c[64];
		int rd;
		EINTRLOOP(rd, (int)read(h, c, sizeof c));
		if (rd != sizeof c) break;
	}
}

#if defined(NSIG) && NSIG > 32
#define NUM_SIGNALS	NSIG
#else
#define NUM_SIGNALS	32
#endif

#ifndef NO_SIGNAL_HANDLERS

static void clear_events_ptr(void *handle)
{
	clear_events((int)(my_intptr_t)handle, 0);
}


struct signal_handler {
	void (*fn)(void *);
	void *data;
	int critical;
};

static volatile int signal_mask[NUM_SIGNALS];
static volatile struct signal_handler signal_handlers[NUM_SIGNALS];

static int signal_pipe[2];

SIGNAL_HANDLER static void got_signal(int sig)
{
	void (*fn)(void *);
	int sv_errno = errno;
		/*fprintf(stderr, "ERROR: signal number: %d\n", sig);*/

#if !defined(HAVE_SIGACTION)
	do_signal(sig, got_signal);
#endif

	if (sig >= NUM_SIGNALS || sig < 0) {
		/*error("ERROR: bad signal number: %d", sig);*/
		goto ret;
	}
	fn = signal_handlers[sig].fn;
	if (!fn) goto ret;
	if (signal_handlers[sig].critical) {
		fn(signal_handlers[sig].data);
		goto ret;
	}
	signal_mask[sig] = 1;
	ret:
	if (can_write(signal_pipe[1])) {
		int wr;
		EINTRLOOP(wr, (int)write(signal_pipe[1], "", 1));
	}
	errno = sv_errno;
}

#ifdef HAVE_SIGACTION
static struct sigaction sa_zero;
#endif

#endif

void install_signal_handler(int sig, void (*fn)(void *), void *data, int critical)
{
#if defined(NO_SIGNAL_HANDLERS)
#elif defined(HAVE_SIGACTION)
	int rs;
	struct sigaction sa = sa_zero;
	/*debug("install (%d) -> %p,%d", sig, fn, critical);*/
	if (sig >= NUM_SIGNALS || sig < 0) {
		internal("bad signal number: %d", sig);
		return;
	}
	if (!fn) sa.sa_handler = SIG_IGN;
	else sa.sa_handler = (void (*)(int))got_signal;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (!fn)
		EINTRLOOP(rs, sigaction(sig, &sa, NULL));
	signal_handlers[sig].fn = fn;
	signal_handlers[sig].data = data;
	signal_handlers[sig].critical = critical;
	if (fn)
		EINTRLOOP(rs, sigaction(sig, &sa, NULL));
#else
	if (!fn) do_signal(sig, SIG_IGN);
	signal_handlers[sig].fn = fn;
	signal_handlers[sig].data = data;
	signal_handlers[sig].critical = critical;
	if (fn) do_signal(sig, got_signal);
#endif
}

void interruptible_signal(int sig, int in)
{
#if defined(HAVE_SIGACTION) && !defined(NO_SIGNAL_HANDLERS)
	struct sigaction sa = sa_zero;
	int rs;
	if (sig >= NUM_SIGNALS || sig < 0) {
		internal("bad signal number: %d", sig);
		return;
	}
	if (!signal_handlers[sig].fn) return;
	sa.sa_handler = (void (*)(int))got_signal;
	sigfillset(&sa.sa_mask);
	if (!in) sa.sa_flags = SA_RESTART;
	EINTRLOOP(rs, sigaction(sig, &sa, NULL));
#endif
}

static sigset_t sig_old_mask;
static int sig_unblock = 0;

void block_signals(int except1, int except2)
{
	int rs;
	sigset_t mask;
	sigfillset(&mask);
#ifdef HAVE_SIGDELSET
	if (except1) sigdelset(&mask, except1);
	if (except2) sigdelset(&mask, except2);
#ifdef SIGILL
	sigdelset(&mask, SIGILL);
#endif
#ifdef SIGABRT
	sigdelset(&mask, SIGABRT);
#endif
#ifdef SIGFPE
	sigdelset(&mask, SIGFPE);
#endif
#ifdef SIGSEGV
	sigdelset(&mask, SIGSEGV);
#endif
#ifdef SIGBUS
	sigdelset(&mask, SIGBUS);
#endif
#else
	if (except1 || except2) return;
#endif
	EINTRLOOP(rs, do_sigprocmask(SIG_BLOCK, &mask, &sig_old_mask));
	if (!rs) sig_unblock = 1;
}

void unblock_signals(void)
{
	int rs;
	if (sig_unblock) {
		EINTRLOOP(rs, do_sigprocmask(SIG_SETMASK, &sig_old_mask, NULL));
		sig_unblock = 0;
	}
}

static int check_signals(void)
{
	int r = 0;
#ifndef NO_SIGNAL_HANDLERS
	int i;
	for (i = 0; i < NUM_SIGNALS; i++)
		if (signal_mask[i]) {
			signal_mask[i] = 0;
			if (signal_handlers[i].fn) {
#ifdef DEBUG_CALLS
				fprintf(stderr, "call: signal %d -> %p\n", i, signal_handlers[i].fn);
#endif
				pr(signal_handlers[i].fn(signal_handlers[i].data)) {
#ifdef OOPS
					return 1;
#endif
}
#ifdef DEBUG_CALLS
				fprintf(stderr, "signal done\n");
#endif
			}
			CHK_BH;
			r = 1;
		}
#endif
	return r;
}

#ifdef SIGCHLD
static void sigchld(void *p)
{
	pid_t pid;
#ifndef WNOHANG
	EINTRLOOP(pid, wait(NULL));
#else
	do {
		EINTRLOOP(pid, waitpid(-1, NULL, WNOHANG));
	} while (pid > 0);
#endif
}

void set_sigcld(void)
{
	install_signal_handler(SIGCHLD, sigchld, NULL, 1);
}
#else
void set_sigcld(void)
{
}
#endif

int terminate_loop = 0;

void select_loop(void (*init)(void))
{
	struct stat st;
	int rs;
	EINTRLOOP(rs, stat(".", &st));
	if (rs && getenv("HOME"))
		EINTRLOOP(rs, chdir(getenv("HOME")));
#if !defined(NO_SIGNAL_HANDLERS)
#if defined(HAVE_SIGACTION)
	memset(&sa_zero, 0, sizeof sa_zero);
#endif
	memset((void *)signal_mask, 0, sizeof signal_mask);
	memset((void *)signal_handlers, 0, sizeof signal_handlers);
#endif
	FD_ZERO(&w_read);
	FD_ZERO(&w_write);
	FD_ZERO(&w_error);
	w_max = 0;
	last_time = get_time();
	ignore_signals();
#if !defined(NO_SIGNAL_HANDLERS)
	if (c_pipe(signal_pipe)) {
		fatal_exit("ERROR: can't create pipe for signal handling");
	}
	set_nonblock(signal_pipe[0]);
	set_nonblock(signal_pipe[1]);
	set_handlers(signal_pipe[0], clear_events_ptr, NULL, NULL, (void *)(my_intptr_t)signal_pipe[0]);
#endif
	init();
	CHK_BH;
	while (!terminate_loop) {
		volatile int n;	/* volatile because of setjmp */
		int i;
		struct timeval tv;
		struct timeval *tm = NULL;
		check_signals();
		check_timers();
		check_timers();
		if (!F) redraw_all_terminals();
#ifdef OS_BAD_SIGNALS
	/* Cygwin has buggy signals that sometimes don't interrupt select.
	   So don't wait indefinitely in it. */
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		tm = &tv;
#endif
		if (!list_empty(timers)) {
			ttime tt = (uttime)((struct timer *)timers.next)->interval + 1;
			if (tt < 0) tt = 0;
#ifdef OS_BAD_SIGNALS
			if (tt < 1000)
#endif
			{
				tv.tv_sec = tt / 1000 < MAXINT ? (int)(tt / 1000) : MAXINT;
				tv.tv_usec = (tt % 1000) * 1000;
				tm = &tv;
			}
		}
		memcpy(&x_read, &w_read, sizeof(fd_set));
		memcpy(&x_write, &w_write, sizeof(fd_set));
		memcpy(&x_error, &w_error, sizeof(fd_set));
		/*rep_sel:*/
		if (terminate_loop) break;
		if (!w_max && list_empty(timers)) {
			/*internal("select_loop: no more events to wait for");*/
			break;
		}
		if (check_signals()) {
			continue;
		}
			/*{
				int i;
				printf("\nR:");
				for (i = 0; i < 256; i++) if (FD_ISSET(i, &x_read)) printf("%d,", i);
				printf("\nW:");
				for (i = 0; i < 256; i++) if (FD_ISSET(i, &x_write)) printf("%d,", i);
				printf("\nE:");
				for (i = 0; i < 256; i++) if (FD_ISSET(i, &x_error)) printf("%d,", i);
				fflush(stdout);
			}*/
#ifdef DEBUG_CALLS
		fprintf(stderr, "select\n");
#endif
		if ((n = loop_select(w_max, &x_read, &x_write, &x_error, tm)) < 0) {
#ifdef DEBUG_CALLS
			fprintf(stderr, "select intr\n");
#endif
			if (errno != EINTR) {
				fatal_exit("ERROR: select failed: %s", strerror(errno));
			}
			continue;
		}
#ifdef DEBUG_CALLS
		fprintf(stderr, "select done\n");
#endif
		check_signals();
		/*printf("sel: %d\n", n);*/
		check_timers();
		i = -1;
		while (n > 0 && ++i < w_max) {
			int k = 0;
			/*printf("C %d : %d,%d,%d\n",i,FD_ISSET(i, &w_read),FD_ISSET(i, &w_write),FD_ISSET(i, &w_error));
			printf("A %d : %d,%d,%d\n",i,FD_ISSET(i, &x_read),FD_ISSET(i, &x_write),FD_ISSET(i, &x_error));*/
			if (FD_ISSET(i, &x_read)) {
				if (threads[i].read_func) {
#ifdef DEBUG_CALLS
					fprintf(stderr, "call: read %d -> %p\n", i, threads[i].read_func);
#endif
					pr(threads[i].read_func(threads[i].data)) continue;
#ifdef DEBUG_CALLS
					fprintf(stderr, "read done\n");
#endif
					CHK_BH;
				}
				k = 1;
			}
			if (FD_ISSET(i, &x_write)) {
				if (threads[i].write_func) {
#ifdef DEBUG_CALLS
					fprintf(stderr, "call: write %d -> %p\n", i, threads[i].write_func);
#endif
					pr(threads[i].write_func(threads[i].data)) continue;
#ifdef DEBUG_CALLS
					fprintf(stderr, "write done\n");
#endif
					CHK_BH;
				}
				k = 1;
			}
			if (FD_ISSET(i, &x_error)) {
				if (threads[i].error_func) {
#ifdef DEBUG_CALLS
					fprintf(stderr, "call: error %d -> %p\n", i, threads[i].error_func);
#endif
					pr(threads[i].error_func(threads[i].data)) continue;
#ifdef DEBUG_CALLS
					fprintf(stderr, "error done\n");
#endif
					CHK_BH;
				}
				k = 1;
			}
			n -= k;
		}
		nopr();
	}
#ifdef DEBUG_CALLS
	fprintf(stderr, "exit loop\n");
#endif
	nopr();
}

/* sched.c
 * Links internal scheduler
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

static tcount connection_count = 0;

static int active_connections = 0;

tcount netcfg_stamp = 0;

struct list_head queue = {&queue, &queue};

struct h_conn {
	struct h_conn *next;
	struct h_conn *prev;
	unsigned char *host;
	int conn;
};

static struct list_head h_conns = {&h_conns, &h_conns};

struct list_head keepalive_connections = {&keepalive_connections, &keepalive_connections};

/* prototypes */
static void send_connection_info(struct connection *c);
static void check_keepalive_connections(void);

unsigned long connect_info(int type)
{
	int i = 0;
	struct connection *ce;
	struct k_conn *cee;
	switch (type) {
		case CI_FILES:
			foreach(ce, queue) i++;
			return i;
		case CI_CONNECTING:
			foreach(ce, queue) i += ce->state > S_WAIT && ce->state < S_TRANS;
			return i;
		case CI_TRANSFER:
			foreach(ce, queue) i += ce->state == S_TRANS;
			return i;
		case CI_KEEP:
			check_keepalive_connections();
			foreach(cee, keepalive_connections) i++;
			return i;
		default:
			internal("connect_info: bad request");
	}
	return 0;
}

static int connection_disappeared(struct connection *c, tcount count)
{
	struct connection *d;
	foreach(d, queue) if (c == d && count == d->count) return 0;
	return 1;
}

static struct h_conn *is_host_on_list(struct connection *c)
{
	unsigned char *ho;
	struct h_conn *h;
	if (!(ho = get_host_name(c->url))) return NULL;
	foreach(h, h_conns) if (!strcmp(cast_const_char h->host, cast_const_char ho)) {
		mem_free(ho);
		return h;
	}
	mem_free(ho);
	return NULL;
}

static int st_r = 0;

static void stat_timer(struct connection *c)
{
	struct remaining_info *r = &c->prg;
	ttime a = (uttime)get_time() - (uttime)r->last_time;
	if (getpri(c) == PRI_CANCEL && (c->est_length > (longlong)memory_cache_size * MAX_CACHED_OBJECT || c->from > (longlong)memory_cache_size * MAX_CACHED_OBJECT)) register_bottom_half(check_queue, NULL);
	if (c->state > S_WAIT) {
		r->loaded = c->received;
		if ((r->size = c->est_length) < (r->pos = c->from) && r->size != -1)
			r->size = c->from;
		r->dis_b += (uttime)a;
		while (r->dis_b >= SPD_DISP_TIME * CURRENT_SPD_SEC) {
			r->cur_loaded -= r->data_in_secs[0];
			memmove(r->data_in_secs, r->data_in_secs + 1, sizeof(off_t) * (CURRENT_SPD_SEC - 1));
			r->data_in_secs[CURRENT_SPD_SEC - 1] = 0;
			r->dis_b -= (uttime)SPD_DISP_TIME;
		}
		r->data_in_secs[CURRENT_SPD_SEC - 1] += r->loaded - r->last_loaded;
		r->cur_loaded += (uttime)r->loaded - (uttime)r->last_loaded;
		r->last_loaded = (uttime)r->loaded;
		r->elapsed += (uttime)a;
	}
	r->last_time += (uttime)a;
	r->timer = install_timer(SPD_DISP_TIME, (void (*)(void *))stat_timer, c);
	if (!st_r) send_connection_info(c);
}

void setcstate(struct connection *c, int state)
{
	struct status *stat;
	if (c->state < 0 && state >= 0) c->prev_error = c->state;
	if ((c->state = state) == S_TRANS) {
		struct remaining_info *r = &c->prg;
		if (r->timer == -1) {
			tcount count = c->count;
			if (!r->valid) {
				memset(r, 0, sizeof(struct remaining_info));
				r->valid = 1;
			}
			r->last_time = get_time();
			r->last_loaded = r->loaded;
			st_r = 1;
			stat_timer(c);
			st_r = 0;
			if (connection_disappeared(c, count)) return;
		}
	} else {
		struct remaining_info *r = &c->prg;
		if (r->timer != -1) kill_timer(r->timer), r->timer = -1;
	}
	foreach(stat, c->statuss) {
		stat->state = state;
		stat->prev_error = c->prev_error;
	}
	if (state >= 0) send_connection_info(c);
}

static struct k_conn *is_host_on_keepalive_list(struct connection *c)
{
	unsigned char *ho;
	int po;
	void (*ph)(struct connection *);
	struct k_conn *h;
	if ((po = get_port(c->url)) == -1) return NULL;
	if (!(ph = get_protocol_handle(c->url))) return NULL;
	if (!(ho = get_host_and_pass(c->url))) return NULL;
	foreach(h, keepalive_connections)
		if (h->protocol == ph && h->port == po && !strcmp(cast_const_char h->host, cast_const_char ho)) {
			mem_free(ho);
			return h;
		}
	mem_free(ho);
	return NULL;
}

int get_keepalive_socket(struct connection *c, int *protocol_data)
{
	struct k_conn *k;
	int cc;
	if (c->tries > 0 || c->unrestartable) return -1;
	if (!(k = is_host_on_keepalive_list(c))) return -1;
	cc = k->conn;
	if (protocol_data) *protocol_data = k->protocol_data;
	del_from_list(k);
	mem_free(k->host);
	mem_free(k);
	c->sock1 = cc;
	if (max_tries == 1) c->tries = -1;
	return 0;
}

void abort_all_keepalive_connections(void)
{
	struct k_conn *k;
	int rs;
	foreach(k, keepalive_connections) {
		mem_free(k->host);
		EINTRLOOP(rs, close(k->conn));
	}
	free_list(keepalive_connections);
	check_keepalive_connections();
}

static void free_connection_data(struct connection *c)
{
	struct h_conn *h;
	int rs;
	if (c->sock1 != -1) set_handlers(c->sock1, NULL, NULL, NULL, NULL);
	if (c->sock2 != -1) set_handlers(c->sock2, NULL, NULL, NULL, NULL);
	close_socket(&c->sock2);
	if (c->pid) {
		EINTRLOOP(rs, kill(c->pid, SIGINT));
		EINTRLOOP(rs, kill(c->pid, SIGTERM));
		EINTRLOOP(rs, kill(c->pid, SIGKILL));
		c->pid = 0;
	}
	if (!c->running) {
		internal("connection already suspended");
	}
	c->running = 0;
	if (c->dnsquery) kill_dns_request(&c->dnsquery);
	if (c->buffer) {
		mem_free(c->buffer);
		c->buffer = NULL;
	}
	if (c->newconn) {
		mem_free(c->newconn);
		c->newconn = NULL;
	}
	if (c->info) {
		mem_free(c->info);
		c->info = NULL;
	}
	if (c->timer != -1) kill_timer(c->timer), c->timer = -1;
	if (--active_connections < 0) {
		internal("active connections underflow");
		active_connections = 0;
	}
	if (c->state != S_WAIT) {
		if ((h = is_host_on_list(c))) {
			if (!--h->conn) {
				del_from_list(h);
				mem_free(h->host);
				mem_free(h);
			}
		} else internal("suspending connection that is not on the list (state %d)", c->state);
	}
}

static void send_connection_info(struct connection *c)
{
	int st = c->state;
	tcount count = c->count;
	struct status *stat = c->statuss.next;
	while ((void *)stat != &c->statuss) {
		stat->ce = c->cache;
		stat = stat->next;
		if (stat->prev->end) stat->prev->end(stat->prev, stat->prev->data);
		if (st >= 0 && connection_disappeared(c, count)) return;
	}
}

static void del_connection(struct connection *c)
{
	struct cache_entry *ce = c->cache;
	if (ce) ce->refcount++;
	del_from_list(c);
	send_connection_info(c);
	if (ce) ce->refcount--;
	if (c->detached) {
		if (ce && !ce->url[0] && !is_entry_used(ce) && !ce->refcount)
			delete_cache_entry(ce);
	} else {
		if (ce)
			trim_cache_entry(ce);
	}
	mem_free(c->url);
	if (c->prev_url) mem_free(c->prev_url);
	mem_free(c);
}

#ifdef DEBUG
static void check_queue_bugs(void);
#endif

void add_keepalive_socket(struct connection *c, ttime timeout, int protocol_data)
{
	struct k_conn *k;
	int rs;
	free_connection_data(c);
	if (c->sock1 == -1) {
		internal("keepalive connection not connected");
		goto del;
	}
	k = mem_alloc(sizeof(struct k_conn));
	if (c->netcfg_stamp != netcfg_stamp ||
	    (k->port = get_port(c->url)) == -1 ||
	    !(k->protocol = get_protocol_handle(c->url)) ||
	    !(k->host = get_host_and_pass(c->url))) {
		mem_free(k);
		del_connection(c);
		goto clos;
	}
	k->conn = c->sock1;
	k->timeout = timeout;
	k->add_time = get_time();
	k->protocol_data = protocol_data;
	add_to_list(keepalive_connections, k);
	del:
	del_connection(c);
#ifdef DEBUG
	check_queue_bugs();
#endif
	register_bottom_half(check_queue, NULL);
	return;
	clos:
	EINTRLOOP(rs, close(c->sock1));
#ifdef DEBUG
	check_queue_bugs();
#endif
	register_bottom_half(check_queue, NULL);
}

static void del_keepalive_socket(struct k_conn *kc)
{
	int rs;
	del_from_list(kc);
	EINTRLOOP(rs, close(kc->conn));
	mem_free(kc->host);
	mem_free(kc);
}

static int keepalive_timeout = -1;

static void keepalive_timer(void *x)
{
	keepalive_timeout = -1;
	check_keepalive_connections();
}

static void check_keepalive_connections(void)
{
	struct k_conn *kc;
	ttime ct = get_time();
	int p = 0;
	if (keepalive_timeout != -1) kill_timer(keepalive_timeout), keepalive_timeout = -1;
	foreach(kc, keepalive_connections) if (can_read(kc->conn) || (uttime)ct - (uttime)kc->add_time > (uttime)kc->timeout) {
		kc = kc->prev;
		del_keepalive_socket(kc->next);
	} else p++;
	for (; p > MAX_KEEPALIVE_CONNECTIONS; p--)
		if (!list_empty(keepalive_connections))
			del_keepalive_socket(keepalive_connections.prev);
		else internal("keepalive list empty");
	if (!list_empty(keepalive_connections)) keepalive_timeout = install_timer(KEEPALIVE_CHECK_TIME, keepalive_timer, NULL);
}

static void add_to_queue(struct connection *c)
{
	struct connection *cc;
	int pri = getpri(c);
	foreach(cc, queue) if (getpri(cc) > pri) break;
	add_at_pos(cc->prev, c);
}

static void sort_queue(void)
{
	struct connection *c, *n;
	int swp;
	do {
		swp = 0;
		foreach(c, queue) if ((void *)c->next != &queue) {
			if (getpri(c->next) < getpri(c)) {
				n = c->next;
				del_from_list(c);
				add_at_pos(n, c);
				swp = 1;
			}
		}
	} while (swp);
}

static void interrupt_connection(struct connection *c)
{
#ifdef HAVE_SSL
	if (c->ssl == (void *)-1) c->ssl = NULL;
	if (c->ssl) {
		SSL_free(c->ssl);
		c->ssl = NULL;
	}
#endif
	if (c->sock1 != -1) set_handlers(c->sock1, NULL, NULL, NULL, NULL);
	close_socket(&c->sock1);
	free_connection_data(c);
}

static void suspend_connection(struct connection *c)
{
	interrupt_connection(c);
	setcstate(c, S_WAIT);
}

static int try_to_suspend_connection(struct connection *c, unsigned char *ho)
{
	int pri = getpri(c);
	struct connection *d;
	foreachback(d, queue) {
		if (getpri(d) <= pri) return -1;
		if (d->state == S_WAIT) continue;
		if (d->unrestartable == 2 && getpri(d) < PRI_CANCEL) continue;
		if (ho) {
			unsigned char *h;
			if (!(h = get_host_name(d->url))) continue;
			if (strcmp(cast_const_char h, cast_const_char ho)) {
				mem_free(h);
				continue;
			}
			mem_free(h);
		}
		suspend_connection(d);
		return 0;
	}
	return -1;
}

static void run_connection(struct connection *c)
{
	struct h_conn *hc;
	void (*func)(struct connection *);
	if (c->running) {
		internal("connection already running");
		return;
	}

	safe_strncpy(c->socks_proxy, proxies.socks_proxy, sizeof c->socks_proxy);

	if (proxies.only_proxies && casecmp(c->url, cast_uchar "proxy://", 8) && casecmp(c->url, cast_uchar "data:", 5) && (!*c->socks_proxy || url_bypasses_socks(c->url))) {
		setcstate(c, S_NO_PROXY);
		del_connection(c);
		return;
	}
	
	if (!(func = get_protocol_handle(c->url))) {
		s_bad_url:
		if (!casecmp(c->url, cast_uchar "proxy://", 8)) setcstate(c, S_BAD_PROXY);
		else setcstate(c, S_BAD_URL);
		del_connection(c);
		return;
	}
	if (!(hc = is_host_on_list(c))) {
		hc = mem_alloc(sizeof(struct h_conn));
		if (!(hc->host = get_host_name(c->url))) {
			mem_free(hc);
			goto s_bad_url;
		}
		hc->conn = 0;
		add_to_list(h_conns, hc);
	}
	hc->conn++;
	active_connections++;
	c->running = 1;
	func(c);
}

static int is_connection_seekable(struct connection *c)
{
	unsigned char *protocol = get_protocol_name(c->url);
	if (!strcasecmp(cast_const_char protocol, "http") ||
	    !strcasecmp(cast_const_char protocol, "https") ||
	    !strcasecmp(cast_const_char protocol, "proxy")) {
		unsigned char *d;
		mem_free(protocol);
		if (!c->cache || !c->cache->head)
			return 1;
		d = parse_http_header(c->cache->head, cast_uchar "Accept-Ranges", NULL);
		if (d) {
			mem_free(d);
			return 1;
		}
		return 0;
	}
	if (!strcasecmp(cast_const_char protocol, "ftp")) {
		mem_free(protocol);
		return 1;
	}
	mem_free(protocol);
	return 0;
}

int is_connection_restartable(struct connection *c)
{
	return !(c->unrestartable >= 2 || (c->tries + 1 >= (max_tries ? max_tries : 1000)));
}

int is_last_try(struct connection *c)
{
	int is_restartable;
	c->tries++;
	is_restartable = is_connection_restartable(c) && c->tries < 10;
	c->tries--;
	return !is_restartable;
}

void retry_connection(struct connection *c)
{
	interrupt_connection(c);
	if (!is_connection_restartable(c)) {
		del_connection(c);
#ifdef DEBUG
		check_queue_bugs();
#endif
		register_bottom_half(check_queue, NULL);
	} else {
		c->tries++;
		c->prev_error = c->state;
		run_connection(c);
	}
}

void abort_connection(struct connection *c)
{
	if (c->running) interrupt_connection(c);
	del_connection(c);
#ifdef DEBUG
	check_queue_bugs();
#endif
	register_bottom_half(check_queue, NULL);
}

static int try_connection(struct connection *c)
{
	struct h_conn *hc = NULL;
	if ((hc = is_host_on_list(c))) {
		if (hc->conn >= max_connections_to_host) {
			if (try_to_suspend_connection(c, hc->host)) return 0;
			else return -1;
		}
	}
	if (active_connections >= max_connections) {
		if (try_to_suspend_connection(c, NULL)) return 0;
		else return -1;
	}
	run_connection(c);
	return 1;
}

#ifdef DEBUG
static void check_queue_bugs(void)
{
	struct connection *d;
	int p = 0, ps = 0;
	int cc;
	again:
	cc = 0;
	foreach(d, queue) {
		int q = getpri(d);
		cc += d->running;
		if (q < p) {
			if (!ps) {
				internal("queue is not sorted");
				sort_queue();
				ps = 1;
				goto again;
			} else {
				internal("queue is not sorted even after sort_queue!");
				break;
			}
		} else p = q;
		if (d->state < 0) {
			internal("interrupted connection on queue (conn %s, state %d)", d->url, d->state);
			d = d->prev;
			abort_connection(d->next);
		}
	}
	if (cc != active_connections) {
		internal("bad number of active connections (counted %d, stored %d)", cc, active_connections);
		active_connections = cc;
	}
}
#endif

void check_queue(void *dummy)
{
	struct connection *c;
	again:
	c = queue.next;
#ifdef DEBUG
	check_queue_bugs();
#endif
	check_keepalive_connections();
	while (c != (struct connection *)(void *)&queue) {
		struct connection *d;
		int cp = getpri(c);
		for (d = c; d != (struct connection *)(void *)&queue && getpri(d) == cp;) {
			struct connection *dd = d; d = d->next;
			if (!dd->state) if (is_host_on_keepalive_list(dd)) {
				if (try_connection(dd)) goto again;
			}
		}
		for (d = c; d != (struct connection *)(void *)&queue && getpri(d) == cp;) {
			struct connection *dd = d; d = d->next;
			if (!dd->state) {
				if (try_connection(dd)) goto again;
			}
		}
		c = d;
	}
	again2:
	foreachback(c, queue) {
		if (getpri(c) < PRI_CANCEL) break;
		if (c->state == S_WAIT) {
			setcstate(c, S_INTERRUPTED);
			del_connection(c);
			goto again2;
		} else if (c->est_length > (longlong)memory_cache_size * MAX_CACHED_OBJECT || c->from > (longlong)memory_cache_size * MAX_CACHED_OBJECT) {
			setcstate(c, S_INTERRUPTED);
			abort_connection(c);
			goto again2;
		}
	}
#ifdef DEBUG
	check_queue_bugs();
#endif
}

unsigned char *get_proxy_string(unsigned char *url)
{
	if (*proxies.http_proxy && !casecmp(url, cast_uchar "http://", 7)) return proxies.http_proxy;
	if (*proxies.ftp_proxy && !casecmp(url, cast_uchar "ftp://", 6)) return proxies.ftp_proxy;
#ifdef HAVE_SSL
	if (*proxies.https_proxy && !casecmp(url, cast_uchar "https://", 8)) return proxies.https_proxy;
#endif
	return NULL;
}

unsigned char *get_proxy(unsigned char *url)
{
	size_t l = strlen(cast_const_char url);
	unsigned char *proxy = get_proxy_string(url);
	unsigned char *u;
	u = mem_alloc(l + 1 + (proxy ? strlen(cast_const_char proxy) + 9 : 0));
	if (proxy) strcpy(cast_char u, "proxy://"), strcat(cast_char u, cast_const_char proxy), strcat(cast_char u, "/");
	else *u = 0;
	strcat(cast_char u, cast_const_char url);
	return u;
}

int get_allow_flags(unsigned char *url)
{
	return	!casecmp(url, cast_uchar "smb://", 6) ? ALLOW_SMB :
		!casecmp(url, cast_uchar "file://", 7) ? ALLOW_FILE : 0;
}

int disallow_url(unsigned char *url, int allow_flags)
{
	if (!casecmp(url, cast_uchar "smb://", 6) && !(allow_flags & ALLOW_SMB) && !smb_options.allow_hyperlinks_to_smb) {
		return S_SMB_NOT_ALLOWED;
	}
	if (!casecmp(url, cast_uchar "file://", 7) && !(allow_flags & ALLOW_FILE)) {
		return S_FILE_NOT_ALLOWED;
	}
	return 0;
}

/* prev_url is a pointer to previous url or NULL */
/* prev_url will NOT be deallocated */
void load_url(unsigned char *url, unsigned char *prev_url, struct status *stat, int pri, int no_cache, int no_compress, int allow_flags, off_t position)
{
	struct cache_entry *e = NULL;
	struct connection *c;
	unsigned char *u;
	int must_detach = 0;
	int err;
	if (stat) {
		stat->c = NULL;
		stat->ce = NULL;
		stat->state = S_OUT_OF_MEM;
		stat->prev_error = 0;
		stat->pri = pri;
	}
#ifdef DEBUG
	foreach(c, queue) {
		struct status *st;
		foreach (st, c->statuss) {
			if (st == stat) {
				internal("status already assigned to '%s'", c->url);
				stat->state = S_INTERNAL;
				if (stat->end) stat->end(stat, stat->data);
				return;
			}
		}
	}
#endif
	if (is_url_blocked(url)) {
		if (stat) {
			stat->state = S_BLOCKED_URL;
			if (stat->end) stat->end(stat, stat->data);
		}
		return;
	}
	err = disallow_url(url, allow_flags);
	if (err) {
		if (stat) {
			stat->state = err;
			if (stat->end) stat->end(stat, stat->data);
		}
		return;
	}
	if (no_cache <= NC_CACHE && !find_in_cache(url, &e)) {
		if (e->incomplete) {
			e->refcount--;
			goto skip_cache;
		}
		if (!aggressive_cache && no_cache > NC_ALWAYS_CACHE) {
			if (e->expire_time && e->expire_time < time(NULL)) {
				if (no_cache < NC_IF_MOD) no_cache = NC_IF_MOD;
				e->refcount--;
				goto skip_cache;
			}
		}
		if (no_compress) {
			unsigned char *enc;
			enc = parse_http_header(e->head, cast_uchar "Content-Encoding", NULL);
			if (enc) {
				mem_free(enc);
				e->refcount--;
				must_detach = 1;
				goto skip_cache;
			}
		}
		if (stat) {
			stat->ce = e;
			stat->state = S__OK;
			if (stat->end) stat->end(stat, stat->data);
		}
		e->refcount--;
		return;
	}
	skip_cache:
	if (!casecmp(url, cast_uchar "proxy://", 8)) {
		if (stat) {
			stat->state = S_BAD_URL;
			if (stat->end) stat->end(stat, stat->data);
		}
		return;
	}
	u = get_proxy(url);
	foreach(c, queue) if (!c->detached && !strcmp(cast_const_char c->url, cast_const_char u)) {
		if (c->from < position) continue;
		if (no_compress && !c->no_compress) {
			unsigned char *enc;
			if ((c->state >= S_WAIT && c->state < S_TRANS) || !c->cache) {
				must_detach = 1;
				break;
			}
			enc = parse_http_header(c->cache->head, cast_uchar "Content-Encoding", NULL);
			if (enc) {
				mem_free(enc);
				must_detach = 1;
				break;
			}
		}
		mem_free(u);
		if (getpri(c) > pri) {
			del_from_list(c);
			c->pri[pri]++;
			add_to_queue(c);
			register_bottom_half(check_queue, NULL);
		} else c->pri[pri]++;
		if (stat) {
			stat->prg = &c->prg;
			stat->c = c;
			stat->ce = c->cache;
			add_to_list(c->statuss, stat);
			setcstate(c, c->state);
		}
#ifdef DEBUG
		check_queue_bugs();
#endif
		return;
	}
	c = mem_calloc(sizeof(struct connection));
	c->count = connection_count++;
	c->url = u;
	c->prev_url = stracpy(prev_url);
	c->running = 0;
	c->prev_error = 0;
	if (position || must_detach) {
		c->from = position;
	} else if (no_cache >= NC_IF_MOD || !e) {
		c->from = 0;
	} else {
		struct fragment *frag;
		c->from = 0;
		foreach(frag, e->frag) {
			if (frag->offset != c->from)
				break;
			c->from += frag->length;
		}
		
	}
	memset(c->pri, 0, sizeof c->pri);
	c->pri[pri] = 1;
	c->no_cache = no_cache;
	c->sock1 = c->sock2 = -1;
	c->dnsquery = NULL;
	c->tries = 0;
	c->netcfg_stamp = netcfg_stamp;
	init_list(c->statuss);
	c->info = NULL;
	c->buffer = NULL;
	c->newconn = NULL;
	c->cache = NULL;
	c->est_length = -1;
	c->unrestartable = 0;
#ifdef HAVE_ANY_COMPRESSION
	c->no_compress = http_options.no_compression || no_compress;
#else
	c->no_compress = 1;
#endif
	c->prg.timer = -1;
	c->timer = -1;
	if (position || must_detach) {
		if (new_cache_entry(c->url, &c->cache)) {
			mem_free(c->url);
			if (c->prev_url) mem_free(c->prev_url);
			mem_free(c);
			if (stat) {
				stat->state = S_OUT_OF_MEM;
				if (stat->end) stat->end(stat, stat->data);
			}
			return;
		}
		c->cache->refcount--;
		detach_cache_entry(c->cache);
		c->detached = 2;
	}
	if (stat) {
		stat->prg = &c->prg;
		stat->c = c;
		stat->ce = NULL;
		add_to_list(c->statuss, stat);
	}
	add_to_queue(c);
	setcstate(c, S_WAIT);
#ifdef DEBUG
	check_queue_bugs();
#endif
	register_bottom_half(check_queue, NULL);
}

void change_connection(struct status *oldstat, struct status *newstat, int newpri)
{		/* !!! FIXME: one object in more connections */
	struct connection *c;
	int oldpri;
	if (!oldstat) {
		internal("change_connection: oldstat == NULL");
		return;
	}
	oldpri = oldstat->pri;
	if (oldstat->state < 0) {
		if (newstat) {
			struct cache_entry *ce = oldstat->ce;
			if (ce) ce->refcount++;
			newstat->ce = oldstat->ce;
			newstat->state = oldstat->state;
			newstat->prev_error = oldstat->prev_error;
			if (newstat->end) newstat->end(newstat, newstat->data);
			if (ce) ce->refcount--;
		}
		return;
	}
#ifdef DEBUG
	check_queue_bugs();
#endif
	c = oldstat->c;
	if (--c->pri[oldpri] < 0) {
		internal("priority counter underflow");
		c->pri[oldpri] = 0;
	}
	c->pri[newpri]++;
	del_from_list(oldstat);
	oldstat->state = S_INTERRUPTED;
	if (newstat) {
		newstat->prg = &c->prg;
		add_to_list(c->statuss, newstat);
		newstat->state = c->state;
		newstat->prev_error = c->prev_error;
		newstat->pri = newpri;
		newstat->c = c;
		newstat->ce = c->cache;
	}
	if (c->detached && !newstat) {
		setcstate(c, S_INTERRUPTED);
		abort_connection(c);
	}
	sort_queue();
#ifdef DEBUG
	check_queue_bugs();
#endif
	register_bottom_half(check_queue, NULL);
}

void detach_connection(struct status *stat, off_t pos)
{
	struct connection *c;
	int i;
	off_t l;
	if (stat->state < 0) return;
	c = stat->c;
	if (!c->cache) return;
	if (c->detached) goto detach_done;
	if (c->est_length == -1) l = c->from;
	else l = c->est_length;
	if (l < (longlong)memory_cache_size * MAX_CACHED_OBJECT && !(pos > c->from && is_connection_seekable(c))) return;
	l = 0;
	for (i = 0; i < PRI_CANCEL; i++) l += c->pri[i];
	if (!l) internal("detaching free connection");
	if (l != 1 || c->cache->refcount) return;
	shrink_memory(SH_CHECK_QUOTA, 0);
	detach_cache_entry(c->cache);
	c->detached = 1;
	detach_done:
	free_entry_to(c->cache, pos);

	if (c->detached < 2 && pos > c->from && is_connection_seekable(c)) {
		int running = c->running;
		if (running) interrupt_connection(c);
		c->from = pos;
		if (running) run_connection(c);
		c->detached = 2;
	}
}

static void connection_timeout(struct connection *c)
{
	c->timer = -1;
	setcstate(c, S_TIMEOUT);
	if (c->dnsquery) abort_connection(c);
	else retry_connection(c);
}

static void connection_timeout_1(struct connection *c)
{
	c->timer = install_timer((c->unrestartable ? unrestartable_receive_timeout : receive_timeout) * 500, (void (*)(void *))connection_timeout, c);
}

void set_connection_timeout(struct connection *c)
{
	if (c->timer != -1) kill_timer(c->timer);
	c->timer = install_timer((c->unrestartable ? unrestartable_receive_timeout : receive_timeout) * 500, (void (*)(void *))connection_timeout_1, c);
}

void abort_all_connections(void)
{
	while(queue.next != &queue) {
		setcstate(queue.next, S_INTERRUPTED);
		abort_connection(queue.next);
	}
	abort_all_keepalive_connections();
}

void abort_background_connections(void)
{
	int i = 0;
	while (1) {
		int j;
		struct connection *c = (void *)&queue;
		for (j = 0; j <= i; j++) if ((c = c->next) == (void *)&queue) goto brk;
		if (getpri(c) >= PRI_CANCEL) {
			setcstate(c, S_INTERRUPTED);
			abort_connection(c);
		} else i++;
	}
	brk:
	abort_all_keepalive_connections();
}

int is_entry_used(struct cache_entry *e)
{
	struct connection *c;
	foreach(c, queue) if (c->cache == e) return 1;
	return 0;
}

struct blacklist_entry {
	struct blacklist_entry *next;
	struct blacklist_entry *prev;
	int flags;
	unsigned char host[1];
};

static struct list_head blacklist = { &blacklist, &blacklist };

void add_blacklist_entry(unsigned char *host, int flags)
{
	struct blacklist_entry *b;
	foreach(b, blacklist) if (!strcasecmp(cast_const_char host, cast_const_char b->host)) {
		b->flags |= flags;
		return;
	}
	b = mem_alloc(sizeof(struct blacklist_entry) + strlen(cast_const_char host) + 1);
	b->flags = flags;
	strcpy(cast_char b->host, cast_const_char host);
	add_to_list(blacklist, b);
}

void del_blacklist_entry(unsigned char *host, int flags)
{
	struct blacklist_entry *b;
	foreach(b, blacklist) if (!strcasecmp(cast_const_char host, cast_const_char b->host)) {
		b->flags &= ~flags;
		if (!b->flags) {
			del_from_list(b);
			mem_free(b);
		}
		return;
	}
}

int get_blacklist_flags(unsigned char *host)
{
	struct blacklist_entry *b;
	foreach(b, blacklist) if (!strcasecmp(cast_const_char host, cast_const_char b->host)) return b->flags;
	return 0;
}

void free_blacklist(void)
{
	free_list(blacklist);
}

struct s_msg_dsc msg_dsc[] = {
	{S_WAIT,		TEXT_(T_WAITING_IN_QUEUE)},
	{S_DNS,			TEXT_(T_LOOKING_UP_HOST)},
	{S_CONN,		TEXT_(T_MAKING_CONNECTION)},
	{S_CONN_ANOTHER,	TEXT_(T_MAKING_CONNECTION_TO_ANOTHER_ADDRESS)},
	{S_SOCKS_NEG,		TEXT_(T_SOCKS_NEGOTIATION)},
	{S_SSL_NEG,		TEXT_(T_SSL_NEGOTIATION)},
	{S_SENT,		TEXT_(T_REQUEST_SENT)},
	{S_LOGIN,		TEXT_(T_LOGGING_IN)},
	{S_GETH,		TEXT_(T_GETTING_HEADERS)},
	{S_PROC,		TEXT_(T_SERVER_IS_PROCESSING_REQUEST)},
	{S_TRANS,		TEXT_(T_TRANSFERRING)},

	{S__OK,			TEXT_(T_OK)},
	{S_INTERRUPTED,		TEXT_(T_INTERRUPTED)},
	{S_EXCEPT,		TEXT_(T_SOCKET_EXCEPTION)},
	{S_INTERNAL,		TEXT_(T_INTERNAL_ERROR)},
	{S_OUT_OF_MEM,		TEXT_(T_OUT_OF_MEMORY)},
	{S_NO_DNS,		TEXT_(T_HOST_NOT_FOUND)},
	{S_CANT_WRITE,		TEXT_(T_ERROR_WRITING_TO_SOCKET)},
	{S_CANT_READ,		TEXT_(T_ERROR_READING_FROM_SOCKET)},
	{S_MODIFIED,		TEXT_(T_DATA_MODIFIED)},
	{S_BAD_URL,		TEXT_(T_BAD_URL_SYNTAX)},
	{S_BAD_PROXY,		TEXT_(T_BAD_PROXY_SYNTAX)},
	{S_TIMEOUT,		TEXT_(T_RECEIVE_TIMEOUT)},
	{S_RESTART,		TEXT_(T_REQUEST_MUST_BE_RESTARTED)},
	{S_STATE,		TEXT_(T_CANT_GET_SOCKET_STATE)},
	{S_CYCLIC_REDIRECT,	TEXT_(T_CYCLIC_REDIRECT)},
	{S_LARGE_FILE,		TEXT_(T_TOO_LARGE_FILE)},

	{S_HTTP_ERROR,		TEXT_(T_BAD_HTTP_RESPONSE)},
	{S_HTTP_100,		TEXT_(T_HTTP_100)},
	{S_HTTP_204,		TEXT_(T_NO_CONTENT)},
	{S_HTTPS_FWD_ERROR,	TEXT_(T_HTTPS_FWD_ERROR)},

	{S_FILE_TYPE,		TEXT_(T_UNKNOWN_FILE_TYPE)},
	{S_FILE_ERROR,		TEXT_(T_ERROR_OPENING_FILE)},

	{S_FTP_ERROR,		TEXT_(T_BAD_FTP_RESPONSE)},
	{S_FTP_UNAVAIL,		TEXT_(T_FTP_SERVICE_UNAVAILABLE)},
	{S_FTP_LOGIN,		TEXT_(T_BAD_FTP_LOGIN)},
	{S_FTP_PORT,		TEXT_(T_FTP_PORT_COMMAND_FAILED)},
	{S_FTP_NO_FILE,		TEXT_(T_FILE_NOT_FOUND)},
	{S_FTP_FILE_ERROR,	TEXT_(T_FTP_FILE_ERROR)},

	{S_SSL_ERROR,		TEXT_(T_SSL_ERROR)},
	{S_NO_SSL,		TEXT_(T_NO_SSL)},
	{S_BAD_SOCKS_VERSION,	TEXT_(T_BAD_SOCKS_VERSION)},
	{S_SOCKS_REJECTED,	TEXT_(T_SOCKS_REJECTED_OR_FAILED)},
	{S_SOCKS_NO_IDENTD,	TEXT_(T_SOCKS_NO_IDENTD)},
	{S_SOCKS_BAD_USERID,	TEXT_(T_SOCKS_BAD_USERID)},
	{S_SOCKS_UNKNOWN_ERROR,	TEXT_(T_SOCKS_UNKNOWN_ERROR)},

	{S_NO_SMB_CLIENT,	TEXT_(T_NO_SMB_CLIENT)},

	{S_BLOCKED_URL,		TEXT_(T_BLOCKED_URL)},
	{S_NO_PROXY,		TEXT_(T_NO_PROXY)},
	{S_SMB_NOT_ALLOWED,	TEXT_(T_SMB_NOT_ALLOWED)},
	{S_FILE_NOT_ALLOWED,	TEXT_(T_FILE_NOT_ALLOWED)},

	{S_WAIT_REDIR,		TEXT_(T_WAITING_FOR_REDIRECT_CONFIRMATION)},
	{0,			NULL}
};

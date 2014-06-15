/* connect.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

/*
#define LOG_TRANSFER	"/tmp/log"
#define LOG_SSL
*/

#ifdef LOG_TRANSFER
static void log_data(unsigned char *data, int len)
{
	static int hlaseno = 0;
	int fd;
	if (!hlaseno) {
		printf("\n"ANSI_SET_BOLD"WARNING -- LOGGING NETWORK TRANSFERS !!!"ANSI_CLEAR_BOLD ANSI_BELL"\n");
		fflush(stdout);
		sleep(1);
		hlaseno = 1;
	}
	EINTRLOOP(fd, open(LOG_TRANSFER, O_WRONLY | O_APPEND | O_CREAT, 0600));
	if (fd != -1) {
		int rw;
		set_bin(fd);
		EINTRLOOP(rw, write(fd, data, len));
		EINTRLOOP(rw, close(fd));
	}
}

#else
#define log_data(x, y)
#endif

#ifdef LOG_SSL
#include <openssl/err.h>
#endif

static inline void log_ssl_error(int line, int ret1, int ret2)
{
#ifdef LOG_SSL
	SSL_load_error_strings();
	ERR_print_errors_fp(stderr);
	debug("ssl error at %d: %d, %d, %d (%s)", line, ret1, ret2, errno, strerror(errno));
#endif
}

static void connected(struct connection *);
static void dns_found(struct connection *, int);
static void try_connect(struct connection *);
static void handle_socks_reply(struct connection *);

static void exception(struct connection *c)
{
	setcstate(c, S_EXCEPT);
	retry_connection(c);
}

int socket_and_bind(int pf, unsigned char *address)
{
	int s;
	int rs;
	EINTRLOOP(s, socket(pf, SOCK_STREAM, IPPROTO_TCP));
	if (s == -1)
		return -1;
	if (address && *address) {
		switch (pf) {
		case PF_INET: {
			struct sockaddr_in sa;
			unsigned char addr[4];
			if (numeric_ip_address(address, addr) == -1) {
				EINTRLOOP(rs, close(s));
				errno = EINVAL;
				return -1;
			}
			memset(&sa, 0, sizeof sa);
			sa.sin_family = AF_INET;
			memcpy(&sa.sin_addr.s_addr, addr, 4);
			sa.sin_port = htons(0);
			EINTRLOOP(rs, bind(s, (struct sockaddr *)(void *)&sa, sizeof sa));
			if (rs) {
				int sv_errno = errno;
				EINTRLOOP(rs, close(s));
				errno = sv_errno;
				return -1;
			}
			break;
		}
#ifdef SUPPORT_IPV6
		case PF_INET6: {
			struct sockaddr_in6 sa;
			unsigned char addr[16];
			unsigned scope;
			if (numeric_ipv6_address(address, addr, &scope) == -1) {
				EINTRLOOP(rs, close(s));
				errno = EINVAL;
				return -1;
			}
			memset(&sa, 0, sizeof sa);
			sa.sin6_family = AF_INET6;
			memcpy(&sa.sin6_addr, addr, 16);
			sa.sin6_port = htons(0);
#ifdef SUPPORT_IPV6_SCOPE
			sa.sin6_scope_id = scope;
#endif
			EINTRLOOP(rs, bind(s, (struct sockaddr *)(void *)&sa, sizeof sa));
			if (rs) {
				int sv_errno = errno;
				EINTRLOOP(rs, close(s));
				errno = sv_errno;
				return -1;
			}
			break;
		}
#endif
		default: {
			EINTRLOOP(rs, close(s));
			errno = EINVAL;
			return -1;
		}
		}
	}
	return s;
}

void close_socket(int *s)
{
	int rs;
	if (*s == -1) return;
	EINTRLOOP(rs, close(*s));
	set_handlers(*s, NULL, NULL, NULL, NULL);
	*s = -1;
}

struct conn_info {
	void (*func)(struct connection *);
	struct lookup_result addr;
	int addr_index;
	int first_error;
	int port;
	int *sock;
	int real_port;
	int socks_byte_count;
	unsigned char socks_reply[8];
	unsigned char *host;
	unsigned char *dns_append;
};

void make_connection(struct connection *c, int port, int *sock, void (*func)(struct connection *))
{
	int real_port = -1;
	int as;
	unsigned char *dns_append = cast_uchar "";
	unsigned char *host;
	struct conn_info *b;
	if (*c->socks_proxy) {
		unsigned char *p = cast_uchar strchr(cast_const_char c->socks_proxy, '@');
		if (p) p++;
		else p = c->socks_proxy;
		host = stracpy(p);
		real_port = port;
		port = 1080;
		if ((p = cast_uchar strchr(cast_const_char host, ':'))) {
			long lp;
			*p++ = 0;
			if (!*p) goto badu;
			lp = strtol(cast_const_char p, (char **)(void *)&p, 10);
			if (*p || lp <= 0 || lp >= 65536) {
				badu:
				mem_free(host);
				setcstate(c, S_BAD_URL);
				abort_connection(c);
				return;
			}
			port = (int)lp;
		}
		dns_append = proxies.dns_append;
	} else if (!(host = get_host_name(c->url))) {
		setcstate(c, S_INTERNAL);
		abort_connection(c);
		return;
	}
	if (c->newconn)
		internal("already making a connection");
	b = mem_calloc(sizeof(struct conn_info) + strlen(cast_const_char host) + 1 + strlen(cast_const_char dns_append) + 1);
	b->func = func;
	b->sock = sock;
	b->port = port;
	b->real_port = real_port;
	b->host = (unsigned char *)(b + 1);
	strcpy(cast_char b->host, cast_const_char host);
	b->dns_append = cast_uchar strchr(cast_const_char b->host, 0) + 1;
	strcpy(cast_char b->dns_append, cast_const_char dns_append);
	c->newconn = b;
	log_data(cast_uchar "\nCONNECTION: ", 13);
	log_data(host, strlen(cast_const_char host));
	log_data(cast_uchar "\n", 1);
	if (c->no_cache >= NC_RELOAD) as = find_host_no_cache(host, &b->addr, &c->dnsquery, (void(*)(void *, int))dns_found, c);
	else as = find_host(host, &b->addr, &c->dnsquery, (void(*)(void *, int))dns_found, c);
	mem_free(host);
	if (as) setcstate(c, S_DNS);
}

int is_ipv6(int h)
{
#ifdef SUPPORT_IPV6
	union {
		struct sockaddr sa;
		struct sockaddr_in sin;
		struct sockaddr_in6 sin6;
		char pad[128];
	} u;
	socklen_t len = sizeof(u);
	int rs;
	EINTRLOOP(rs, getsockname(h, &u.sa, &len));
	if (rs) return 0;
	return u.sa.sa_family == AF_INET6;
#else
	return 0;
#endif
}

int get_pasv_socket(struct connection *c, int cc, int *sock, unsigned char *port)
{
	int s;
	int rs;
	struct sockaddr_in sa;
	struct sockaddr_in sb;
	socklen_t len = sizeof(sa);
	memset(&sa, 0, sizeof sa);
	memset(&sb, 0, sizeof sb);
	EINTRLOOP(rs, getsockname(cc, (struct sockaddr *)(void *)&sa, &len));
	if (rs) goto e;
	if (sa.sin_family != AF_INET) {
		errno = EINVAL;
		goto e;
	}
	EINTRLOOP(s, socket(PF_INET, SOCK_STREAM, IPPROTO_TCP));
	if (s == -1) goto e;
	*sock = s;
	set_nonblock(s);
	memcpy(&sb, &sa, sizeof(struct sockaddr_in));
	sb.sin_port = htons(0);
	EINTRLOOP(rs, bind(s, (struct sockaddr *)(void *)&sb, sizeof sb));
	if (rs) goto e;
	len = sizeof(sa);
	EINTRLOOP(rs, getsockname(s, (struct sockaddr *)(void *)&sa, &len));
	if (rs) goto e;
	EINTRLOOP(rs, listen(s, 1));
	if (rs) goto e;
	memcpy(port, &sa.sin_addr.s_addr, 4);
	memcpy(port + 4, &sa.sin_port, 2);
	return 0;

	e:
	setcstate(c, get_error_from_errno(errno));
	retry_connection(c);
	return -1;
}

#ifdef SUPPORT_IPV6

int get_pasv_socket_ipv6(struct connection *c, int cc, int *sock, unsigned char *result)
{
	int s;
	int rs;
	struct sockaddr_in6 sa;
	struct sockaddr_in6 sb;
	socklen_t len = sizeof(sa);
	memset(&sa, 0, sizeof sa);
	memset(&sb, 0, sizeof sb);
	EINTRLOOP(rs, getsockname(cc, (struct sockaddr *)(void *)&sa, &len));
	if (rs) goto e;
	if (sa.sin6_family != AF_INET6) {
		errno = EINVAL;
		goto e;
	}
	EINTRLOOP(s, socket(PF_INET6, SOCK_STREAM, IPPROTO_TCP));
	if (s == -1) goto e;
	*sock = s;
	set_nonblock(s);
	memcpy(&sb, &sa, sizeof(struct sockaddr_in6));
	sb.sin6_port = htons(0);
	EINTRLOOP(rs, bind(s, (struct sockaddr *)(void *)&sb, sizeof sb));
	if (rs) goto e;
	len = sizeof(sa);
	EINTRLOOP(rs, getsockname(s, (struct sockaddr *)(void *)&sa, &len));
	if (rs) goto e;
	EINTRLOOP(rs, listen(s, 1));
	if (rs) goto e;
	sprintf(cast_char result, "|2|%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x|%d|",
		sa.sin6_addr.s6_addr[0],
		sa.sin6_addr.s6_addr[1],
		sa.sin6_addr.s6_addr[2],
		sa.sin6_addr.s6_addr[3],
		sa.sin6_addr.s6_addr[4],
		sa.sin6_addr.s6_addr[5],
		sa.sin6_addr.s6_addr[6],
		sa.sin6_addr.s6_addr[7],
		sa.sin6_addr.s6_addr[8],
		sa.sin6_addr.s6_addr[9],
		sa.sin6_addr.s6_addr[10],
		sa.sin6_addr.s6_addr[11],
		sa.sin6_addr.s6_addr[12],
		sa.sin6_addr.s6_addr[13],
		sa.sin6_addr.s6_addr[14],
		sa.sin6_addr.s6_addr[15],
		htons(sa.sin6_port) & 0xffff);
	return 0;

	e:
	setcstate(c, get_error_from_errno(errno));
	retry_connection(c);
	return -1;
}

#endif

#ifdef HAVE_SSL
static void ssl_want_read(struct connection *c)
{
	int ret1, ret2;
	struct conn_info *b = c->newconn;

	set_connection_timeout(c);

#ifndef HAVE_NSS
	if (c->no_tsl) c->ssl->options |= SSL_OP_NO_TLSv1;
#endif
	switch ((ret2 = SSL_get_error(c->ssl, ret1 = SSL_connect(c->ssl)))) {
		case SSL_ERROR_NONE:
			c->newconn = NULL;
			b->func(c);
			mem_free(b);
			break;
		case SSL_ERROR_WANT_READ:
			set_handlers(*b->sock, (void(*)(void *))ssl_want_read, NULL, (void(*)(void *))exception, c);
			break;
		case SSL_ERROR_WANT_WRITE:
			set_handlers(*b->sock, NULL, (void(*)(void *))ssl_want_read, (void(*)(void *))exception, c);
			break;
		default:
			log_ssl_error(__LINE__, ret1, ret2);
			c->no_tsl++;
			setcstate(c, S_SSL_ERROR);
			retry_connection(c);
			break;
	}
}
#endif

static void handle_socks(struct connection *c)
{
	struct conn_info *b = c->newconn;
	unsigned char *command = init_str();
	int len = 0;
	unsigned char *host;
	int wr;
	setcstate(c, S_SOCKS_NEG);
	set_connection_timeout(c);
	add_bytes_to_str(&command, &len, cast_uchar "\004\001", 2);
	add_chr_to_str(&command, &len, b->real_port >> 8);
	add_chr_to_str(&command, &len, b->real_port);
	add_bytes_to_str(&command, &len, cast_uchar "\000\000\000\001", 4);
	if (strchr(cast_const_char c->socks_proxy, '@'))
		add_bytes_to_str(&command, &len, c->socks_proxy, strcspn(cast_const_char c->socks_proxy, "@"));
	add_chr_to_str(&command, &len, 0);
	if (!(host = get_host_name(c->url))) {
		mem_free(command);
		setcstate(c, S_INTERNAL);
		abort_connection(c);
		return;
	}
	add_to_str(&command, &len, host);
	add_to_str(&command, &len, b->dns_append);
	add_chr_to_str(&command, &len, 0);
	mem_free(host);
	if (b->socks_byte_count >= len) {
		mem_free(command);
		setcstate(c, S_MODIFIED);
		retry_connection(c);
		return;
	}
	EINTRLOOP(wr, (int)write(*b->sock, command + b->socks_byte_count, len - b->socks_byte_count));
	mem_free(command);
	if (wr <= 0) {
		setcstate(c, wr ? get_error_from_errno(errno) : S_CANT_WRITE);
		retry_connection(c);
		return;
	}
	b->socks_byte_count += wr;
	if (b->socks_byte_count < len) {
		set_handlers(*b->sock, NULL, (void(*)(void *))handle_socks, (void(*)(void *))exception, c);
		return;
	} else {
		b->socks_byte_count = 0;
		set_handlers(*b->sock, (void(*)(void *))handle_socks_reply, NULL, (void(*)(void *))exception, c);
		return;
	}
}

static void handle_socks_reply(struct connection *c)
{
	struct conn_info *b = c->newconn;
	int rd;
	set_connection_timeout(c);
	EINTRLOOP(rd, (int)read(*b->sock, b->socks_reply + b->socks_byte_count, sizeof b->socks_reply - b->socks_byte_count));
	if (rd <= 0) {
		setcstate(c, rd ? get_error_from_errno(errno) : S_CANT_READ);
		retry_connection(c);
		return;
	}
	b->socks_byte_count += rd;
	if (b->socks_byte_count < (int)sizeof b->socks_reply) return;
	/* debug("%x %x %x %x %x %x %x %x", b->socks_reply[0], b->socks_reply[1], b->socks_reply[2], b->socks_reply[3], b->socks_reply[4], b->socks_reply[5], b->socks_reply[6], b->socks_reply[7]); */
	if (b->socks_reply[0]) {
		setcstate(c, S_BAD_SOCKS_VERSION);
		abort_connection(c);
		return;
	}
	switch (b->socks_reply[1]) {
		case 91:
			setcstate(c, S_SOCKS_REJECTED);
			retry_connection(c);
			return;
		case 92:
			setcstate(c, S_SOCKS_NO_IDENTD);
			abort_connection(c);
			return;
		case 93:
			setcstate(c, S_SOCKS_BAD_USERID);
			abort_connection(c);
			return;
		default:
			setcstate(c, S_SOCKS_UNKNOWN_ERROR);
			retry_connection(c);
			return;
		case 90:
			break;
	}
	b->real_port = -1;
	connected(c);
}

static void dns_found(struct connection *c, int state)
{
	if (state) {
		setcstate(c, S_NO_DNS);
		abort_connection(c);
		return;
	}
	try_connect(c);
}

static void retry_connect(struct connection *c, int err)
{
	struct conn_info *b = c->newconn;
	if (!b->addr_index) b->first_error = err;
	b->addr_index++;
	if (b->addr_index < b->addr.n) {
		close_socket(b->sock);
		try_connect(c);
	} else {
		setcstate(c, b->first_error);
		retry_connection(c);
	}
}

static void try_connect(struct connection *c)
{
	int s;
	int rs;
	struct conn_info *b = c->newconn;
	struct host_address *addr = &b->addr.a[b->addr_index];
	if (addr->af == AF_INET) {
		s = socket_and_bind(PF_INET, bind_ip_address);
#ifdef SUPPORT_IPV6
	} else if (addr->af == AF_INET6) {
		s = socket_and_bind(PF_INET6, bind_ipv6_address);
#endif
	} else {
		setcstate(c, S_INTERNAL);
		abort_connection(c);
		return;
	}
	if (s == -1) {
		retry_connect(c, get_error_from_errno(errno));
		return;
	}
	set_nonblock(s);
	*b->sock = s;
	if (addr->af == AF_INET) {
		struct sockaddr_in sa;
		memset(&sa, 0, sizeof sa);
		sa.sin_family = AF_INET;
		memcpy(&sa.sin_addr.s_addr, addr->addr, 4);
		sa.sin_port = htons(b->port);
		EINTRLOOP(rs, connect(s, (struct sockaddr *)(void *)&sa, sizeof sa));
#ifdef SUPPORT_IPV6
	} else if (addr->af == AF_INET6) {
		struct sockaddr_in6 sa;
		memset(&sa, 0, sizeof sa);
		sa.sin6_family = AF_INET6;
		memcpy(&sa.sin6_addr, addr->addr, 16);
#ifdef SUPPORT_IPV6_SCOPE
		sa.sin6_scope_id = addr->scope_id;
#endif
		sa.sin6_port = htons(b->port);
		EINTRLOOP(rs, connect(s, (struct sockaddr *)(void *)&sa, sizeof sa));
#endif
	} else {
		rs = -1;
		errno = EINVAL;
	}
	if (rs) {
		if (errno != EALREADY && errno != EINPROGRESS) {
#ifdef BEOS
			if (errno == EWOULDBLOCK) errno = ETIMEDOUT;
#endif
			retry_connect(c, get_error_from_errno(errno));
			return;
		}
		set_handlers(s, NULL, (void(*)(void *))connected, (void(*)(void *))exception, c);
		setcstate(c, !b->addr_index ? S_CONN : S_CONN_ANOTHER);
	} else {
		connected(c);
	}
}

void continue_connection(struct connection *c, int *sock, void (*func)(struct connection *))
{
	struct conn_info *b;
	if (c->newconn)
		internal("already making a connection");
	b = mem_calloc(sizeof(struct conn_info));
	b->func = func;
	b->sock = sock;
	b->real_port = -1;
	c->newconn = b;
	log_data(cast_uchar "\nCONTINUE CONNECTION\n", 21);
	connected(c);
}

static void connected(struct connection *c)
{
	struct conn_info *b = c->newconn;
	int err = 0;
	socklen_t len = sizeof(int);
	int rs;
#ifdef SO_ERROR
	errno = 0;
	EINTRLOOP(rs, getsockopt(*b->sock, SOL_SOCKET, SO_ERROR, (void *)&err, &len));
	if (!rs) {
		if (err >= 10000) err -= 10000;	/* Why does EMX return so large values? */
	} else {
		if (!(err = errno)) {
			retry_connect(c, S_STATE);
			return;
		}
	}
	if (err > 0
#ifdef EISCONN
	    && err != EISCONN
#endif
	    ) {
#ifdef DOS
		if (err == EALREADY) err = ETIMEDOUT;
#endif
		retry_connect(c, get_error_from_errno(err));
		return;
	}
#endif
	if (b->addr_index) {
		int i;
		for (i = 0; i < b->addr_index; i++)
			dns_set_priority(b->host, &b->addr.a[i], 0);
		dns_set_priority(b->host, &b->addr.a[i], 1);
	}
	set_connection_timeout(c);
	if (b->real_port != -1) {
		handle_socks(c);
		return;
	}
#ifdef HAVE_SSL
	if (c->ssl) {
		int ret1, ret2;
		c->ssl = getSSL();
		if (!c->ssl) {
			goto ssl_error;
		}
		SSL_set_fd(c->ssl, *b->sock);
#ifndef HAVE_NSS
		if (c->no_tsl) c->ssl->options |= SSL_OP_NO_TLSv1;
#endif
		switch ((ret2 = SSL_get_error(c->ssl, ret1 = SSL_connect(c->ssl)))) {
			case SSL_ERROR_WANT_READ:
				setcstate(c, S_SSL_NEG);
				set_handlers(*b->sock, (void(*)(void *))ssl_want_read, NULL, (void(*)(void *))exception, c);
				return;
			case SSL_ERROR_WANT_WRITE:
				setcstate(c, S_SSL_NEG);
				set_handlers(*b->sock, NULL, (void(*)(void *))ssl_want_read, (void(*)(void *))exception, c);
				return;
			case SSL_ERROR_NONE:
				break;
			default:
				log_ssl_error(__LINE__, ret1, ret2);
			ssl_error:
				c->no_tsl++;
				setcstate(c, S_SSL_ERROR);
				retry_connection(c);
				return;
		}
	}
#endif
	c->newconn = NULL;
	b->func(c);
	mem_free(b);
}

struct write_buffer {
	int sock;
	int len;
	int pos;
	void (*done)(struct connection *);
	unsigned char data[1];
};

static void write_select(struct connection *c)
{
	struct write_buffer *wb;
	int wr;
	if (!(wb = c->buffer)) {
		internal("write socket has no buffer");
		setcstate(c, S_INTERNAL);
		abort_connection(c);
		return;
	}
	set_connection_timeout(c);
	/*printf("ws: %d\n",wb->len-wb->pos);
	for (wr = wb->pos; wr < wb->len; wr++) printf("%c", wb->data[wr]);
	printf("-\n");*/

#ifdef HAVE_SSL
	if(c->ssl) {
		if ((wr = SSL_write(c->ssl, wb->data + wb->pos, wb->len - wb->pos)) <= 0) {
			int err;
			if ((err = SSL_get_error(c->ssl, wr)) != SSL_ERROR_WANT_WRITE) {
				setcstate(c, wr ? (err == SSL_ERROR_SYSCALL ? get_error_from_errno(errno) : S_SSL_ERROR) : S_CANT_WRITE);
				if (c->state == S_SSL_ERROR)
					log_ssl_error(__LINE__, wr, err);
				if (!wr || err == SSL_ERROR_SYSCALL) retry_connection(c);
				else abort_connection(c);
				return;
			}
			else return;
		}
	} else
#endif
	{
		EINTRLOOP(wr, (int)write(wb->sock, wb->data + wb->pos, wb->len - wb->pos));
		if (wr <= 0) {
#if defined(ATHEOS) || defined(DOS)
	/* Workaround for a bug in Syllable */
			if (wr && (errno == EAGAIN || errno == EWOULDBLOCK)) {
				return;
			}
#endif
			setcstate(c, wr ? get_error_from_errno(errno) : S_CANT_WRITE);
			retry_connection(c);
			return;
		}
	}

	if ((wb->pos += wr) == wb->len) {
		void (*f)(struct connection *) = wb->done;
		c->buffer = NULL;
		set_handlers(wb->sock, NULL, NULL, NULL, NULL);
		mem_free(wb);
		f(c);
	}
}

void write_to_socket(struct connection *c, int s, unsigned char *data, int len, void (*write_func)(struct connection *))
{
	struct write_buffer *wb;
	log_data(data, len);
	if ((unsigned)len > MAXINT - sizeof(struct write_buffer)) overalloc();
	wb = mem_alloc(sizeof(struct write_buffer) + len);
	wb->sock = s;
	wb->len = len;
	wb->pos = 0;
	wb->done = write_func;
	memcpy(wb->data, data, len);
	if (c->buffer) mem_free(c->buffer);
	c->buffer = wb;
	set_handlers(s, NULL, (void (*)(void *))write_select, (void (*)(void *))exception, c);
}

#define READ_SIZE	64240

static void read_select(struct connection *c)
{
	struct read_buffer *rb;
	int rd;
	if (!(rb = c->buffer)) {
		internal("read socket has no buffer");
		setcstate(c, S_INTERNAL);
		abort_connection(c);
		return;
	}
	set_handlers(rb->sock, NULL, NULL, NULL, NULL);
	if ((unsigned)rb->len > MAXINT - sizeof(struct read_buffer) - READ_SIZE) overalloc();
	rb = mem_realloc(rb, sizeof(struct read_buffer) + rb->len + READ_SIZE);
	c->buffer = rb;

#ifdef HAVE_SSL
	if(c->ssl) {
		if ((rd = SSL_read(c->ssl, rb->data + rb->len, READ_SIZE)) <= 0) {
			int err;
			if ((err = SSL_get_error(c->ssl, rd)) == SSL_ERROR_WANT_READ) {
				read_from_socket(c, rb->sock, rb, rb->done);
				return;
			}
			if (rb->close && !rd) {
				rb->close = 2;
				rb->done(c, rb);
				return;
			}
			setcstate(c, rd ? (err == SSL_ERROR_SYSCALL ? get_error_from_errno(errno) : S_SSL_ERROR) : S_CANT_READ);
			if (c->state == S_SSL_ERROR)
				log_ssl_error(__LINE__, rd, err);
			/*mem_free(rb);*/
			if (!rd || err == SSL_ERROR_SYSCALL) retry_connection(c);
			else abort_connection(c);
			return;
		}
	} else
#endif
	{
		EINTRLOOP(rd, (int)read(rb->sock, rb->data + rb->len, READ_SIZE));
		if (rd <= 0) {
			if (rb->close && !rd) {
				rb->close = 2;
				rb->done(c, rb);
				return;
			}
			if (!rd) {
/* Many servers supporting compression have a bug
   --- they send the size of uncompressed data.
   Turn off compression support once before the final retry.
*/
				unsigned char *prot, *h;
				if (is_last_try(c) && (prot = get_protocol_name(c->url))) {
					if (!strcasecmp(cast_const_char prot, "http")) {
						if ((h = get_host_name(c->url))) {
							add_blacklist_entry(h, BL_NO_COMPRESSION);
							mem_free(h);
						}
					}
					mem_free(prot);
				}
			}
			setcstate(c, rd ? get_error_from_errno(errno) : S_CANT_READ);
			/*mem_free(rb);*/
			retry_connection(c);
			return;
		}
	}
	log_data(rb->data + rb->len, rd);
	rb->len += rd;
	rb->done(c, rb);
}

struct read_buffer *alloc_read_buffer(struct connection *c)
{
	struct read_buffer *rb;
	rb = mem_alloc(sizeof(struct read_buffer) + READ_SIZE);
	memset(rb, 0, sizeof(struct read_buffer));
	return rb;
}

void read_from_socket(struct connection *c, int s, struct read_buffer *buf, void (*read_func)(struct connection *, struct read_buffer *))
{
	buf->done = read_func;
	buf->sock = s;
	if (c->buffer && buf != c->buffer) mem_free(c->buffer);
	c->buffer = buf;
	set_handlers(s, (void (*)(void *))read_select, NULL, (void (*)(void *))exception, c);
}

void kill_buffer_data(struct read_buffer *rb, int n)
{
	if (n > rb->len || n < 0) {
		internal("called kill_buffer_data with bad value");
		rb->len = 0;
		return;
	}
	memmove(rb->data, rb->data + n, rb->len - n);
	rb->len -= n;
}

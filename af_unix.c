/* af_unix.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL
 */

#include "links.h"

#ifdef DONT_USE_AF_UNIX

int bind_to_af_unix(void)
{
	return -1;
}

void af_unix_close(void)
{
}

#else

#ifdef USE_AF_UNIX
#include <sys/un.h>
#endif

#if defined(__GNU__)
#define SOCKET_TIMEOUT_HACK
#endif

static void af_unix_connection(void *);

#define ADDR_SIZE	4096

union address {
	struct sockaddr s;
#ifdef USE_AF_UNIX
	struct sockaddr_un suni;
#endif
	struct sockaddr_in sin;
	unsigned char buffer[ADDR_SIZE];
};

static union address s_unix;
static union address s_unix_acc;

static socklen_t s_unix_l;
static int s_unix_fd = -1;
static int s_unix_master = 0;


#define S2C1_HANDSHAKE_LENGTH	6
#define C2S2_HANDSHAKE_LENGTH	sizeof(struct links_handshake)
#define S2C3_HANDSHAKE_LENGTH	sizeof(struct links_handshake)

static struct links_handshake {
	unsigned char version[30];
	unsigned char system_name[32];
	unsigned char system_id;
	unsigned char sizeof_long;
} links_handshake;

#define HANDSHAKE_WRITE(hndl, sz)					\
	if ((r = hard_write(hndl, (unsigned char *)&links_handshake, sz)) != (sz))
#define HANDSHAKE_READ(hndl, sz)					\
	if ((r = hard_read(hndl, (unsigned char *)&received_handshake, sz)) != (sz) || memcmp(&received_handshake, &links_handshake, sz))


#ifdef USE_AF_UNIX

static int get_address(void)
{
	unsigned char *path;
	if (!links_home) return -1;
	path = stracpy(links_home);
	add_to_strn(&path, cast_uchar LINKS_SOCK_NAME);
	s_unix_l = (socklen_t)((unsigned char *)&s_unix.suni.sun_path - (unsigned char *)&s_unix.suni + strlen(cast_const_char path) + 1);
	if (strlen(cast_const_char path) > sizeof(union address) || (size_t)s_unix_l > sizeof(union address)) {
		mem_free(path);
		return -1;
	}
	memset(&s_unix, 0, sizeof s_unix);
	s_unix.suni.sun_family = AF_UNIX;
	strcpy(cast_char s_unix.suni.sun_path, cast_const_char path);
	mem_free(path);
	return PF_UNIX;
}

static void unlink_unix(void)
{
	int rs;
	/*debug("unlink: %s", s_unix.suni.sun_path);*/
	EINTRLOOP(rs, unlink(s_unix.suni.sun_path));
	if (rs) {
		/*perror("unlink");*/
	}
}

#else

static int get_address(void)
{
	memset(&s_unix, 0, sizeof s_unix);
	s_unix.sin.sin_family = AF_INET;
	s_unix.sin.sin_port = htons(LINKS_PORT);
	s_unix.sin.sin_addr.s_addr = htonl(0x7f000001);
	s_unix_l = sizeof(struct sockaddr_in);
	return PF_INET;
}

static void unlink_unix(void)
{
}

#endif

int bind_to_af_unix(void)
{
	int u = 0;
	int a1 = 1;
	int cnt = 0;
	int af;
	int r;
	int rs;
	struct links_handshake received_handshake;
	memset(&links_handshake, 0, sizeof links_handshake);
	safe_strncpy(links_handshake.version, cast_uchar("Links " VERSION_STRING), sizeof links_handshake.version);
	safe_strncpy(links_handshake.system_name, system_name, sizeof links_handshake.system_name);
	links_handshake.system_id = SYSTEM_ID;
	links_handshake.sizeof_long = sizeof(long);
	if ((af = get_address()) == -1) return -1;
	again:
	EINTRLOOP(s_unix_fd, socket(af, SOCK_STREAM, 0));
	if (s_unix_fd == -1) return -1;
#if defined(SOL_SOCKET) && defined(SO_REUSEADDR)
	EINTRLOOP(rs, setsockopt(s_unix_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&a1, sizeof a1));
#endif
	EINTRLOOP(rs, bind(s_unix_fd, &s_unix.s, s_unix_l));
	if (rs) {
		/*perror("");
		debug("bind: %d", errno);*/
		if (af == PF_INET && errno == EADDRNOTAVAIL) {
			/* do not try to connect if the user has not configured loopback interface */
			EINTRLOOP(rs, close(s_unix_fd));
			return -1;
		}
		EINTRLOOP(rs, close(s_unix_fd));
		EINTRLOOP(s_unix_fd, socket(af, SOCK_STREAM, 0));
		if (s_unix_fd == -1) return -1;
#if defined(SOL_SOCKET) && defined(SO_REUSEADDR)
		EINTRLOOP(rs, setsockopt(s_unix_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&a1, sizeof a1));
#endif
		EINTRLOOP(rs, connect(s_unix_fd, &s_unix.s, s_unix_l));
		if (rs) {
retry:
			/*perror("");
			debug("connect: %d", errno);*/
			if (++cnt < MAX_BIND_TRIES) {
				portable_sleep(100);
				EINTRLOOP(rs, close(s_unix_fd));
				s_unix_fd = -1;
				goto again;
			}
#ifdef SOCKET_TIMEOUT_HACK
retry_unlink:
#endif
			EINTRLOOP(rs, close(s_unix_fd));
			s_unix_fd = -1;
			if (!u) {
				unlink_unix();
				u = 1;
				goto again;
			}
			return -1;
		}
#ifdef SOCKET_TIMEOUT_HACK
		if (!can_read_timeout(s_unix_fd, AF_UNIX_SOCKET_TIMEOUT))
			goto retry_unlink;
#endif
		HANDSHAKE_READ(s_unix_fd, S2C1_HANDSHAKE_LENGTH) {
			if (r != S2C1_HANDSHAKE_LENGTH) goto retry;
			goto close_and_fail;
		}
		HANDSHAKE_WRITE(s_unix_fd, C2S2_HANDSHAKE_LENGTH)
			goto close_and_fail;
		HANDSHAKE_READ(s_unix_fd, S2C3_HANDSHAKE_LENGTH)
			goto close_and_fail;
		return s_unix_fd;
	}
	EINTRLOOP(rs, listen(s_unix_fd, 100));
	if (rs) {
		error("ERROR: listen failed: %d", errno);
		close_and_fail:
		EINTRLOOP(rs, close(s_unix_fd));
		s_unix_fd = -1;
		return -1;
	}
	s_unix_master = 1;
	set_handlers(s_unix_fd, af_unix_connection, NULL, NULL, NULL);
	return -1;
}

static void af_unix_connection(void *xxx)
{
	socklen_t l = s_unix_l;
	int ns;
	int r;
	int rs;
	struct links_handshake received_handshake;
	memset(&s_unix_acc, 0, sizeof s_unix_acc);
	EINTRLOOP(ns, accept(s_unix_fd, &s_unix_acc.s, &l));
	if (ns == -1) return;
	HANDSHAKE_WRITE(ns, S2C1_HANDSHAKE_LENGTH) {
		EINTRLOOP(rs, close(ns));
		return;
	}
	HANDSHAKE_READ(ns, C2S2_HANDSHAKE_LENGTH) {
		portable_sleep(100);	/* workaround for a race in previous Links version */
		EINTRLOOP(rs, close(ns));
		return;
	}
	HANDSHAKE_WRITE(ns, S2C3_HANDSHAKE_LENGTH) {
		EINTRLOOP(rs, close(ns));
		return;
	}
	init_term(ns, ns, win_func);
	set_highpri();
}

void af_unix_close(void)
{
	int rs;
	if (s_unix_master) {
		set_handlers(s_unix_fd, NULL, NULL, NULL, NULL);
	}
	if (s_unix_fd != -1) {
		EINTRLOOP(rs, close(s_unix_fd));
		s_unix_fd = -1;
	}
	if (s_unix_master) {
		unlink_unix();
		s_unix_master = 0;
	}
}

#endif

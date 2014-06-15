/* menu.c
 * (c) 2002 Mikulas Patocka, Petr 'Brain' Kulhavy
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"


static unsigned char *version_texts[] = {
	TEXT_(T_LINKS_VERSION),
	TEXT_(T_OPERATING_SYSTEM_TYPE),
	TEXT_(T_OPERATING_SYSTEM_VERSION),
	TEXT_(T_COMPILER),
	TEXT_(T_COMPILE_TIME),
	TEXT_(T_WORD_SIZE),
	TEXT_(T_DEBUGGING_LEVEL),
	TEXT_(T_IPV6),
	TEXT_(T_COMPRESSION_METHODS),
	TEXT_(T_ENCRYPTION),
	TEXT_(T_UTF8_TERMINAL),
#if defined(__linux__) || defined(__LINUX__) || defined(__SPAD__) || defined(USE_GPM)
	TEXT_(T_GPM_MOUSE_DRIVER),
#endif
#ifdef OS2
	TEXT_(T_XTERM_FOR_OS2),
#endif
	TEXT_(T_GRAPHICS_MODE),
#ifdef G
	TEXT_(T_IMAGE_LIBRARIES),
#endif
	NULL,
};

static void add_and_pad(unsigned char **s, int *l, struct terminal *term, unsigned char *str, int maxlen)
{
	unsigned char *x = _(str, term);
	int len = cp_len(term->spec->charset, x);
	add_to_str(s, l, x);
	add_to_str(s, l, cast_uchar ":  ");
	while (len++ < maxlen) add_chr_to_str(s, l, ' ');
}

static void menu_version(struct terminal *term)
{
	int i;
	int maxlen = 0;
	unsigned char *s;
	int l;
	unsigned char **text_ptr;
	for (i = 0; version_texts[i]; i++) {
		unsigned char *t = _(version_texts[i], term);
		int tl = cp_len(term->spec->charset, t);
		if (tl > maxlen)
			maxlen = tl;
	}

	s = init_str();
	l = 0;
	text_ptr = version_texts;

	add_and_pad(&s, &l, term, *text_ptr++, maxlen);
	add_to_str(&s, &l, cast_uchar VERSION_STRING);
	add_to_str(&s, &l, cast_uchar "\n");

	add_and_pad(&s, &l, term, *text_ptr++, maxlen);
	add_to_str(&s, &l, cast_uchar SYSTEM_NAME);
	add_to_str(&s, &l, cast_uchar "\n");

	add_and_pad(&s, &l, term, *text_ptr++, maxlen);
	add_to_str(&s, &l, system_name);
	add_to_str(&s, &l, cast_uchar "\n");

	add_and_pad(&s, &l, term, *text_ptr++, maxlen);
	add_to_str(&s, &l, compiler_name);
	add_to_str(&s, &l, cast_uchar "\n");

	add_and_pad(&s, &l, term, *text_ptr++, maxlen);
	add_to_str(&s, &l, cast_uchar(__DATE__ "  " __TIME__));
	add_to_str(&s, &l, cast_uchar "\n");

	add_and_pad(&s, &l, term, *text_ptr++, maxlen);
	add_to_str(&s, &l, _(TEXT_(T_MEMORY), term));
	add_to_str(&s, &l, cast_uchar " ");
	add_num_to_str(&s, &l, sizeof(void *) * 8);
	add_to_str(&s, &l, cast_uchar "-bit, ");
	add_to_str(&s, &l, _(TEXT_(T_FILE_SIZE), term));
	add_to_str(&s, &l, cast_uchar " ");
	add_num_to_str(&s, &l, sizeof(off_t) * 8 /*- ((off_t)-1 < 0)*/);
	add_to_str(&s, &l, cast_uchar "-bit");
	add_to_str(&s, &l, cast_uchar "\n");

	add_and_pad(&s, &l, term, *text_ptr++, maxlen);
	add_num_to_str(&s, &l, DEBUGLEVEL);
	add_to_str(&s, &l, cast_uchar "\n");

	add_and_pad(&s, &l, term, *text_ptr++, maxlen);
#ifdef SUPPORT_IPV6
	if (!support_ipv6) add_to_str(&s, &l, _(TEXT_(T_NOT_ENABLED_IN_SYSTEM), term));
	else if (!ipv6_full_access()) add_to_str(&s, &l, _(TEXT_(T_LOCAL_NETWORK_ONLY), term));
	else add_to_str(&s, &l, _(TEXT_(T_YES), term));
#else
	add_to_str(&s, &l, _(TEXT_(T_NO), term));
#endif
	add_to_str(&s, &l, cast_uchar "\n");

	add_and_pad(&s, &l, term, *text_ptr++, maxlen);
#ifdef HAVE_ANY_COMPRESSION
	add_compress_methods(&s, &l);
#else
	add_to_str(&s, &l, _(TEXT_(T_NO), term));
#endif
	add_to_str(&s, &l, cast_uchar "\n");

	add_and_pad(&s, &l, term, *text_ptr++, maxlen);
#ifdef HAVE_SSL
	add_to_str(&s, &l, (unsigned char *)SSLeay_version(SSLEAY_VERSION));
#else
	add_to_str(&s, &l, _(TEXT_(T_NO), term));
#endif
	add_to_str(&s, &l, cast_uchar "\n");

	add_and_pad(&s, &l, term, *text_ptr++, maxlen);
#ifdef ENABLE_UTF8
	add_to_str(&s, &l, _(TEXT_(T_YES), term));
#else
	add_to_str(&s, &l, _(TEXT_(T_NO), term));
#endif
	add_to_str(&s, &l, cast_uchar "\n");

#if defined(__linux__) || defined(__LINUX__) || defined(__SPAD__) || defined(USE_GPM)
	add_and_pad(&s, &l, term, *text_ptr++, maxlen);
#ifdef USE_GPM
	add_gpm_version(&s, &l);
#else
	add_to_str(&s, &l, _(TEXT_(T_NO), term));
#endif
	add_to_str(&s, &l, cast_uchar "\n");
#endif

#ifdef OS2
	add_and_pad(&s, &l, term, *text_ptr++, maxlen);
#ifdef X2
	add_to_str(&s, &l, _(TEXT_(T_YES), term));
#else
	add_to_str(&s, &l, _(TEXT_(T_NO), term));
#endif
	add_to_str(&s, &l, cast_uchar "\n");
#endif

	add_and_pad(&s, &l, term, *text_ptr++, maxlen);
#ifdef G
	i = l;
	add_graphics_drivers(&s, &l);
	for (; s[i]; i++) if (s[i - 1] == ' ') s[i] = upcase(s[i]);
#else
	add_to_str(&s, &l, _(TEXT_(T_NO), term));
#endif
	add_to_str(&s, &l, cast_uchar "\n");

#ifdef G
	add_and_pad(&s, &l, term, *text_ptr++, maxlen);
	add_png_version(&s, &l);
#ifdef HAVE_JPEG
	add_to_str(&s, &l, cast_uchar ", ");
	add_jpeg_version(&s, &l);
#endif
#ifdef HAVE_TIFF
	add_to_str(&s, &l, cast_uchar ", ");
	add_tiff_version(&s, &l);
#endif
	add_to_str(&s, &l, cast_uchar "\n");
#endif

	s[l - 1] = 0;
	if (*text_ptr)
		internal("menu_version: text mismatched");
	
	msg_box(term, getml(s, NULL), TEXT_(T_VERSION_INFORMATION), AL_LEFT | AL_MONO, s, NULL, 1, TEXT_(T_OK), NULL, B_ENTER | B_ESC);
}

static void menu_about(struct terminal *term, void *d, struct session *ses)
{
	msg_box(term, NULL, TEXT_(T_ABOUT), AL_CENTER, TEXT_(T_LINKS__LYNX_LIKE), term, 2, TEXT_(T_OK), NULL, B_ENTER | B_ESC, TEXT_(T_VERSION), menu_version, 0);
}

static void menu_keys(struct terminal *term, void *d, struct session *ses)
{
	if (!term->spec->braille)
		msg_box(term, NULL, TEXT_(T_KEYS), AL_LEFT | AL_MONO, TEXT_(T_KEYS_DESC), NULL, 1, TEXT_(T_OK), NULL, B_ENTER | B_ESC);
	else
		msg_box(term, NULL, TEXT_(T_KEYS), AL_LEFT | AL_MONO | AL_EXTD_TEXT, TEXT_(T_KEYS_DESC), cast_uchar "\n", TEXT_(T_KEYS_BRAILLE_DESC), NULL, NULL, 1, TEXT_(T_OK), NULL, B_ENTER | B_ESC);
}

void activate_keys(struct session *ses)
{
	menu_keys(ses->term, NULL, ses);
}

static void menu_copying(struct terminal *term, void *d, struct session *ses)
{
	msg_box(term, NULL, TEXT_(T_COPYING), AL_CENTER, TEXT_(T_COPYING_DESC), NULL, 1, TEXT_(T_OK), NULL, B_ENTER | B_ESC);
}

static void menu_manual(struct terminal *term, void *d, struct session *ses)
{
	goto_url(ses, cast_uchar LINKS_MANUAL_URL);
}

static void menu_homepage(struct terminal *term, void *d, struct session *ses)
{
	goto_url(ses, cast_uchar LINKS_HOMEPAGE_URL);
}

#ifdef G
static void menu_calibration(struct terminal *term, void *d, struct session *ses)
{
	goto_url(ses, cast_uchar LINKS_CALIBRATION_URL);
}
#endif

static void menu_for_frame(struct terminal *term, void (*f)(struct session *, struct f_data_c *, int), struct session *ses)
{
	do_for_frame(ses, f, 0);
}

static void menu_goto_url(struct terminal *term, void *d, struct session *ses)
{
	dialog_goto_url(ses, cast_uchar "");
}

static void menu_save_url_as(struct terminal *term, void *d, struct session *ses)
{
	dialog_save_url(ses);
}

static void menu_go_back(struct terminal *term, void *d, struct session *ses)
{
	go_back(ses, 1);
}

static void menu_go_forward(struct terminal *term, void *d, struct session *ses)
{
	go_back(ses, -1);
}

static void menu_reload(struct terminal *term, void *d, struct session *ses)
{
	reload(ses, -1);
}

void really_exit_prog(struct session *ses)
{
	register_bottom_half((void (*)(void *))destroy_terminal, ses->term);
}

static void dont_exit_prog(struct session *ses)
{
	ses->exit_query = 0;
}

void query_exit(struct session *ses)
{
	ses->exit_query = 1;
	msg_box(ses->term, NULL, TEXT_(T_EXIT_LINKS), AL_CENTER, (ses->term->next == ses->term->prev && are_there_downloads()) ? TEXT_(T_DO_YOU_REALLY_WANT_TO_EXIT_LINKS_AND_TERMINATE_ALL_DOWNLOADS) : (!F || ses->term->next == ses->term->prev) ? TEXT_(T_DO_YOU_REALLY_WANT_TO_EXIT_LINKS) : TEXT_(T_DO_YOU_REALLY_WANT_TO_CLOSE_WINDOW), ses, 2, TEXT_(T_YES), (void (*)(void *))really_exit_prog, B_ENTER, TEXT_(T_NO), dont_exit_prog, B_ESC);
}

void exit_prog(struct terminal *term, void *d, struct session *ses)
{
	if (!ses) {
		register_bottom_half((void (*)(void *))destroy_terminal, term);
		return;
	}
	if (!ses->exit_query && (!d || (term->next == term->prev && are_there_downloads()))) {
		query_exit(ses);
		return;
	}
	really_exit_prog(ses);
}

struct refresh {
	struct terminal *term;
	struct window *win;
	struct session *ses;
	int (*fn)(struct terminal *term, struct refresh *r);
	void *data;
	int timer;
};

static void refresh(struct refresh *r)
{
	r->timer = -1;
	if (r->fn(r->term, r) > 0)
		return;
	delete_window(r->win);
}

static void end_refresh(struct refresh *r)
{
	if (r->timer != -1) kill_timer(r->timer);
	mem_free(r);
}

static void refresh_abort(struct dialog_data *dlg)
{
	end_refresh(dlg->dlg->udata2);
}

static int resource_info(struct terminal *term, struct refresh *r2)
{
	unsigned char *a;
	int l;
	struct refresh *r;

	r = mem_alloc(sizeof(struct refresh));
	r->term = term;
	r->win = NULL;
	r->fn = resource_info;
	r->timer = -1;
	l = 0;
	a = init_str();

	add_to_str(&a, &l, _(TEXT_(T_RESOURCES), term));
	add_to_str(&a, &l, cast_uchar ": ");
	add_unsigned_long_num_to_str(&a, &l, select_info(CI_FILES));
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_HANDLES), term));
	add_to_str(&a, &l, cast_uchar ", ");
	add_unsigned_long_num_to_str(&a, &l, select_info(CI_TIMERS));
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_TIMERS), term));
	add_to_str(&a, &l, cast_uchar ".\n");

	add_to_str(&a, &l, _(TEXT_(T_CONNECTIONS), term));
	add_to_str(&a, &l, cast_uchar ": ");
	add_unsigned_long_num_to_str(&a, &l, connect_info(CI_FILES) - connect_info(CI_CONNECTING) - connect_info(CI_TRANSFER));
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_WAITING), term));
	add_to_str(&a, &l, cast_uchar ", ");
	add_unsigned_long_num_to_str(&a, &l, connect_info(CI_CONNECTING));
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_CONNECTING), term));
	add_to_str(&a, &l, cast_uchar ", ");
	add_unsigned_long_num_to_str(&a, &l, connect_info(CI_TRANSFER));
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_tRANSFERRING), term));
	add_to_str(&a, &l, cast_uchar ", ");
	add_unsigned_long_num_to_str(&a, &l, connect_info(CI_KEEP));
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_KEEPALIVE), term));
	add_to_str(&a, &l, cast_uchar ".\n");

	add_to_str(&a, &l, _(TEXT_(T_MEMORY_CACHE), term));
	add_to_str(&a, &l, cast_uchar ": ");
	add_unsigned_long_num_to_str(&a, &l, cache_info(CI_BYTES));
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_BYTES), term));
	add_to_str(&a, &l, cast_uchar ", ");
	add_unsigned_long_num_to_str(&a, &l, cache_info(CI_FILES));
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_FILES), term));
	add_to_str(&a, &l, cast_uchar ", ");
	add_unsigned_long_num_to_str(&a, &l, cache_info(CI_LOCKED));
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_LOCKED), term));
	add_to_str(&a, &l, cast_uchar ", ");
	add_unsigned_long_num_to_str(&a, &l, cache_info(CI_LOADING));
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_LOADING), term));
	add_to_str(&a, &l, cast_uchar ".\n");

#ifdef HAVE_ANY_COMPRESSION
	add_to_str(&a, &l, _(TEXT_(T_DECOMPRESSED_CACHE), term));
	add_to_str(&a, &l, cast_uchar ": ");
	add_unsigned_long_num_to_str(&a, &l, decompress_info(CI_BYTES));
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_BYTES), term));
	add_to_str(&a, &l, cast_uchar ", ");
	add_unsigned_long_num_to_str(&a, &l, decompress_info(CI_FILES));
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_FILES), term));
	add_to_str(&a, &l, cast_uchar ", ");
	add_unsigned_long_num_to_str(&a, &l, decompress_info(CI_LOCKED));
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_LOCKED), term));
	add_to_str(&a, &l, cast_uchar ".\n");
#endif

#ifdef G
	if (F) {
		add_to_str(&a, &l, _(TEXT_(T_IMAGE_CACHE), term));
		add_to_str(&a, &l, cast_uchar ": ");
		add_unsigned_long_num_to_str(&a, &l, imgcache_info(CI_BYTES));
		add_to_str(&a, &l, cast_uchar " ");
		add_to_str(&a, &l, _(TEXT_(T_BYTES), term));
		add_to_str(&a, &l, cast_uchar ", ");
		add_unsigned_long_num_to_str(&a, &l, imgcache_info(CI_FILES));
		add_to_str(&a, &l, cast_uchar " ");
		add_to_str(&a, &l, _(TEXT_(T_IMAGES), term));
		add_to_str(&a, &l, cast_uchar ", ");
		add_unsigned_long_num_to_str(&a, &l, imgcache_info(CI_LOCKED));
		add_to_str(&a, &l, cast_uchar " ");
		add_to_str(&a, &l, _(TEXT_(T_LOCKED), term));
		add_to_str(&a, &l, cast_uchar ".\n");
		
		add_to_str(&a, &l, _(TEXT_(T_FONT_CACHE), term));
		add_to_str(&a, &l, cast_uchar ": ");
		add_unsigned_long_num_to_str(&a, &l, fontcache_info(CI_BYTES));
		add_to_str(&a, &l, cast_uchar " ");
		add_to_str(&a, &l, _(TEXT_(T_BYTES), term));
		add_to_str(&a, &l, cast_uchar ", ");
		add_unsigned_long_num_to_str(&a, &l, fontcache_info(CI_FILES));
		add_to_str(&a, &l, cast_uchar " ");
		add_to_str(&a, &l, _(TEXT_(T_LETTERS), term));
		add_to_str(&a, &l, cast_uchar ".\n");
	}
#endif

	add_to_str(&a, &l, _(TEXT_(T_FORMATTED_DOCUMENT_CACHE), term));
	add_to_str(&a, &l, cast_uchar ": ");
	add_unsigned_long_num_to_str(&a, &l, formatted_info(CI_FILES));
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_DOCUMENTS), term));
	add_to_str(&a, &l, cast_uchar ", ");
	add_unsigned_long_num_to_str(&a, &l, formatted_info(CI_LOCKED));
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_LOCKED), term));
	add_to_str(&a, &l, cast_uchar ".\n");

	add_to_str(&a, &l, _(TEXT_(T_DNS_CACHE), term));
	add_to_str(&a, &l, cast_uchar ": ");
	add_unsigned_long_num_to_str(&a, &l, dns_info(CI_FILES));
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_SERVERS), term));
	add_to_str(&a, &l, cast_uchar ".");

	if (r2 && !strcmp(cast_const_char a, cast_const_char *(unsigned char **)((struct dialog_data *)r2->win->data)->dlg->udata)) {
		mem_free(a);
		mem_free(r);
		r2->timer = install_timer(RESOURCE_INFO_REFRESH, (void (*)(void *))refresh, r2);
		return 1;
	}

	msg_box(term, getml(a, NULL), TEXT_(T_RESOURCES), AL_LEFT, a, r, 1, TEXT_(T_OK), NULL, B_ENTER | B_ESC);
	r->win = term->windows.next;
	((struct dialog_data *)r->win->data)->dlg->abort = refresh_abort;
	r->timer = install_timer(RESOURCE_INFO_REFRESH, (void (*)(void *))refresh, r);
	return 0;
}

static void resource_info_menu(struct terminal *term, void *d, struct session *ses)
{
	resource_info(term, NULL);
}

#ifdef LEAK_DEBUG

static int memory_info(struct terminal *term, struct refresh *r2)
{
	unsigned char *a;
	int l;
	struct refresh *r;

	r = mem_alloc(sizeof(struct refresh));
	r->term = term;
	r->win = NULL;
	r->fn = memory_info;
	r->timer = -1;
	l = 0;
	a = init_str();

	add_unsigned_long_num_to_str(&a, &l, mem_amount);
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_MEMORY_ALLOCATED), term));
	add_to_str(&a, &l, cast_uchar ", ");
	add_unsigned_long_num_to_str(&a, &l, mem_blocks);
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_BLOCKS_ALLOCATED), term));
	add_to_str(&a, &l, cast_uchar ".");

#ifdef MEMORY_REQUESTED
	if (mem_requested && blocks_requested) {
		add_to_str(&a, &l, cast_uchar "\n");
		add_unsigned_long_num_to_str(&a, &l, mem_requested);
		add_to_str(&a, &l, cast_uchar " ");
		add_to_str(&a, &l, _(TEXT_(T_MEMORY_REQUESTED), term));
		add_to_str(&a, &l, cast_uchar ", ");
		add_unsigned_long_num_to_str(&a, &l, blocks_requested);
		add_to_str(&a, &l, cast_uchar " ");
		add_to_str(&a, &l, _(TEXT_(T_BLOCKS_REQUESTED), term));
		add_to_str(&a, &l, cast_uchar ".");
	}
#endif
#ifdef JS
	add_to_str(&a, &l, cast_uchar "\n");
	add_unsigned_long_num_to_str(&a, &l, js_zaflaknuto_pameti);
	add_to_str(&a, &l, cast_uchar " ");
	add_to_str(&a, &l, _(TEXT_(T_JS_MEMORY_ALLOCATED), term));
	add_to_str(&a, &l, cast_uchar ".");
#endif

	if (r2 && !strcmp(cast_const_char a, cast_const_char *(unsigned char **)((struct dialog_data *)r2->win->data)->dlg->udata)) {
		mem_free(a);
		mem_free(r);
		r2->timer = install_timer(RESOURCE_INFO_REFRESH, (void (*)(void *))refresh, r2);
		return 1;
	}

	
	msg_box(term, getml(a, NULL), TEXT_(T_MEMORY_INFO), AL_CENTER, a, r, 1, TEXT_(T_OK), NULL, B_ENTER | B_ESC);
	r->win = term->windows.next;
	((struct dialog_data *)r->win->data)->dlg->abort = refresh_abort;
	r->timer = install_timer(RESOURCE_INFO_REFRESH, (void (*)(void *))refresh, r);
	return 0;
}

static void memory_info_menu(struct terminal *term, void *d, struct session *ses)
{
	memory_info(term, NULL);
}

#endif

static void flush_caches(struct terminal *term, void *d, void *e)
{
	abort_background_connections();
	shrink_memory(SH_FREE_ALL, 0);
}

/* jde v historii na polozku id_ptr */
void go_backwards(struct terminal *term, void *id_ptr, struct session *ses)
{
	unsigned want_id = (unsigned)(my_intptr_t)id_ptr;
	struct location *l;
	int n = 0;
	foreach(l, ses->history) {
		if (l->location_id == want_id) {
			goto have_it;
		}
		n++;
	}
	n = -1;
	foreach(l, ses->forward_history) {
		if (l->location_id == want_id) {
			goto have_it;
		}
		n--;
	}
	return;

	have_it:
	go_back(ses, n);
}

static struct menu_item no_hist_menu[] = {
	{ TEXT_(T_NO_HISTORY), cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

static void add_history_menu_entry(struct menu_item **mi, int *n, struct location *l)
{
	unsigned char *url, *pc;
	if (!*mi) *mi = new_menu(3);
	url = stracpy(l->url);
	if ((pc = cast_uchar strchr(cast_const_char url, POST_CHAR))) *pc = 0;
	add_to_menu(mi, url, cast_uchar "", cast_uchar "", MENU_FUNC go_backwards, (void *)(my_intptr_t)l->location_id, 0, *n);
	(*n)++;
	if (*n == MAXINT) overalloc();
}

static void history_menu(struct terminal *term, void *ddd, struct session *ses)
{
	struct location *l;
	struct menu_item *mi = NULL;
	int n = 0;
	int selected = 0;
	foreachback(l, ses->forward_history) {
		add_history_menu_entry(&mi, &n, l);
	}
	selected = n;
	foreach(l, ses->history) {
		add_history_menu_entry(&mi, &n, l);
	}
	if (!mi) do_menu(term, no_hist_menu, ses);
	else do_menu_selected(term, mi, ses, selected, NULL, NULL);
}

static struct menu_item no_downloads_menu[] = {
	{ TEXT_(T_NO_DOWNLOADS), cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

static void downloads_menu(struct terminal *term, void *ddd, struct session *ses)
{
	struct download *d;
	struct menu_item *mi = NULL;
	int n = 0;
	foreachback(d, downloads) {
		unsigned char *f, *ff;
		if (!mi) mi = new_menu(7);
		f = !d->prog ? d->orig_file : d->url;
		for (ff = f; *ff; ff++)
			if ((dir_sep(ff[0])
#if defined(DOS_FS) || defined(SPAD)
			  || (!d->prog && ff[0] == ':')
#endif
			  ) && ff[1])
				f = ff + 1;
		f = stracpy(f);
		if (d->prog) if ((ff = cast_uchar strchr(cast_const_char f, POST_CHAR))) *ff = 0;
		add_to_menu(&mi, f, download_percentage(d, 0), cast_uchar "", MENU_FUNC display_download, d, 0, n);
		n++;
	}
	if (!n) do_menu(term, no_downloads_menu, ses);
	else do_menu(term, mi, ses);
}

static void menu_doc_info(struct terminal *term, void *ddd, struct session *ses)
{
	state_msg(ses);
}

static void menu_head_info(struct terminal *term, void *ddd, struct session *ses)
{
	head_msg(ses);
}

static void menu_toggle(struct terminal *term, void *ddd, struct session *ses)
{
	toggle(ses, ses->screen, 0);
}

static void display_codepage(struct terminal *term, void *pcp, struct session *ses)
{
	int cp = (int)(my_intptr_t)pcp;
	struct term_spec *t = new_term_spec(term->term);
	if (t) t->charset = cp;
	cls_redraw_all_terminals();
}

static void charset_list(struct terminal *term, void *xxx, struct session *ses)
{
	int i, sel;
	unsigned char *n;
	struct menu_item *mi;
	mi = new_menu(1);
	for (i = 0; (n = get_cp_name(i)); i++) {
#ifndef ENABLE_UTF8
		if (i == utf8_table) continue;
#endif
		add_to_menu(&mi, get_cp_name(i), cast_uchar "", cast_uchar "", MENU_FUNC display_codepage, (void *)(my_intptr_t)i, 0, i);
	}
	sel = ses->term->spec->charset;
	if (sel < 0) sel = 0;
	do_menu_selected(term, mi, ses, sel, NULL, NULL);
}

static void set_val(struct terminal *term, void *ip, int *d)
{
	*d = (int)(my_intptr_t)ip;
}

static void charset_sel_list(struct terminal *term, int *ptr, int utf)
{
	int i, sel;
	unsigned char *n;
	struct menu_item *mi;
	mi = new_menu(1);
	for (i = 0; (n = get_cp_name(i)); i++) {
		if (!utf && i == utf8_table) continue;
		add_to_menu(&mi, get_cp_name(i), cast_uchar "", cast_uchar "", MENU_FUNC set_val, (void *)(my_intptr_t)i, 0, i);
	}
	sel = *ptr;
	if (sel < 0) sel = 0;
	do_menu_selected(term, mi, ptr, sel, NULL, NULL);
}

static void terminal_options_ok(void *p)
{
	cls_redraw_all_terminals();
}

static unsigned char *td_labels[] = { TEXT_(T_NO_FRAMES), TEXT_(T_VT_100_FRAMES), TEXT_(T_LINUX_OR_OS2_FRAMES), TEXT_(T_KOI8R_FRAMES), TEXT_(T_FREEBSD_FRAMES), TEXT_(T_USE_11M), TEXT_(T_RESTRICT_FRAMES_IN_CP850_852), TEXT_(T_BLOCK_CURSOR), TEXT_(T_COLOR), TEXT_(T_BRAILLE_TERMINAL), NULL };

static void terminal_options(struct terminal *term, void *xxx, struct session *ses)
{
	struct dialog *d;
	struct term_spec *ts = new_term_spec(term->term);
	if (!ts) return;
	d = mem_calloc(sizeof(struct dialog) + 12 * sizeof(struct dialog_item));
	d->title = TEXT_(T_TERMINAL_OPTIONS);
	d->fn = checkbox_list_fn;
	d->udata = td_labels;
	d->refresh = (void (*)(void *))terminal_options_ok;
	d->items[0].type = D_CHECKBOX;
	d->items[0].gid = 1;
	d->items[0].gnum = TERM_DUMB;
	d->items[0].dlen = sizeof(int);
	d->items[0].data = (void *)&ts->mode;
	d->items[1].type = D_CHECKBOX;
	d->items[1].gid = 1;
	d->items[1].gnum = TERM_VT100;
	d->items[1].dlen = sizeof(int);
	d->items[1].data = (void *)&ts->mode;
	d->items[2].type = D_CHECKBOX;
	d->items[2].gid = 1;
	d->items[2].gnum = TERM_LINUX;
	d->items[2].dlen = sizeof(int);
	d->items[2].data = (void *)&ts->mode;
	d->items[3].type = D_CHECKBOX;
	d->items[3].gid = 1;
	d->items[3].gnum = TERM_KOI8;
	d->items[3].dlen = sizeof(int);
	d->items[3].data = (void *)&ts->mode;
	d->items[4].type = D_CHECKBOX;
	d->items[4].gid = 1;
	d->items[4].gnum = TERM_FREEBSD;
	d->items[4].dlen = sizeof(int);
	d->items[4].data = (void *)&ts->mode;
	d->items[5].type = D_CHECKBOX;
	d->items[5].gid = 0;
	d->items[5].dlen = sizeof(int);
	d->items[5].data = (void *)&ts->m11_hack;
	d->items[6].type = D_CHECKBOX;
	d->items[6].gid = 0;
	d->items[6].dlen = sizeof(int);
	d->items[6].data = (void *)&ts->restrict_852;
	d->items[7].type = D_CHECKBOX;
	d->items[7].gid = 0;
	d->items[7].dlen = sizeof(int);
	d->items[7].data = (void *)&ts->block_cursor;
	d->items[8].type = D_CHECKBOX;
	d->items[8].gid = 0;
	d->items[8].dlen = sizeof(int);
	d->items[8].data = (void *)&ts->col;
	d->items[9].type = D_CHECKBOX;
	d->items[9].gid = 0;
	d->items[9].dlen = sizeof(int);
	d->items[9].data = (void *)&ts->braille;
	d->items[10].type = D_BUTTON;
	d->items[10].gid = B_ENTER;
	d->items[10].fn = ok_dialog;
	d->items[10].text = TEXT_(T_OK);
	d->items[11].type = D_BUTTON;
	d->items[11].gid = B_ESC;
	d->items[11].fn = cancel_dialog;
	d->items[11].text = TEXT_(T_CANCEL);
	d->items[12].type = D_END;
	do_dialog(term, d, getml(d, NULL));
}

#ifdef JS

static unsigned char *jsopt_labels[] = { TEXT_(T_KILL_ALL_SCRIPTS), TEXT_(T_ENABLE_JAVASCRIPT), TEXT_(T_VERBOSE_JS_ERRORS), TEXT_(T_VERBOSE_JS_WARNINGS), TEXT_(T_ENABLE_ALL_CONVERSIONS), TEXT_(T_ENABLE_GLOBAL_NAME_RESOLUTION), TEXT_(T_MANUAL_JS_CONTROL), TEXT_(T_JS_RECURSION_DEPTH), TEXT_(T_JS_MEMORY_LIMIT_KB), NULL };

static int kill_script_opt;
static unsigned char js_fun_depth_str[7];
static unsigned char js_memory_limit_str[7];


static inline void kill_js_recursively(struct f_data_c *fd)
{
	struct f_data_c *f;

	if (fd->js) js_downcall_game_over(fd->js->ctx);
	foreach(f,fd->subframes) kill_js_recursively(f);
}


static inline void quiet_kill_js_recursively(struct f_data_c *fd)
{
	struct f_data_c *f;

	if (fd->js)js_downcall_game_over(fd->js->ctx);
	foreach(f,fd->subframes) quiet_kill_js_recursively(f);
}


static void refresh_javascript(struct session *ses)
{
	if (ses->screen->f_data)jsint_scan_script_tags(ses->screen);
	if (kill_script_opt)
		kill_js_recursively(ses->screen);
	if (!js_enable) /* vypnuli jsme skribt */
	{
		if (ses->default_status)mem_free(ses->default_status),ses->default_status=NULL;
		quiet_kill_js_recursively(ses->screen);
	}

	js_fun_depth=strtol(cast_const_char js_fun_depth_str,0,10);
	js_memory_limit=strtol(cast_const_char js_memory_limit_str,0,10);

	/* reparse document (muze se zmenit hodne veci) */
	html_interpret_recursive(ses->screen);
	draw_formatted(ses);
}


static void javascript_options(struct terminal *term, void *xxx, struct session *ses)
{
	struct dialog *d;
	kill_script_opt=0;
	snprintf(cast_char js_fun_depth_str,7,"%d",js_fun_depth);
	snprintf(cast_char js_memory_limit_str,7,"%d",js_memory_limit);
	d = mem_calloc(sizeof(struct dialog) + 11 * sizeof(struct dialog_item));
	d->title = TEXT_(T_JAVASCRIPT_OPTIONS);
	d->fn = group_fn;
	d->refresh = (void (*)(void *))refresh_javascript;
	d->refresh_data=ses;
	d->udata = jsopt_labels;
	d->items[0].type = D_CHECKBOX;
	d->items[0].gid = 0;
	d->items[0].dlen = sizeof(int);
	d->items[0].data = (void *)&kill_script_opt;
	d->items[1].type = D_CHECKBOX;
	d->items[1].gid = 0;
	d->items[1].dlen = sizeof(int);
	d->items[1].data = (void *)&js_enable;
	d->items[2].type = D_CHECKBOX;
	d->items[2].gid = 0;
	d->items[2].dlen = sizeof(int);
	d->items[2].data = (void *)&js_verbose_errors;
	d->items[3].type = D_CHECKBOX;
	d->items[3].gid = 0;
	d->items[3].dlen = sizeof(int);
	d->items[3].data = (void *)&js_verbose_warnings;
	d->items[4].type = D_CHECKBOX;
	d->items[4].gid = 0;
	d->items[4].dlen = sizeof(int);
	d->items[4].data = (void *)&js_all_conversions;
	d->items[5].type = D_CHECKBOX;
	d->items[5].gid = 0;
	d->items[5].dlen = sizeof(int);
	d->items[5].data = (void *)&js_global_resolve;
	d->items[6].type = D_CHECKBOX;
	d->items[6].gid = 0;
	d->items[6].dlen = sizeof(int);
	d->items[6].data = (void *)&js_manual_confirmation;
	d->items[7].type = D_FIELD;
	d->items[7].dlen = 7;
	d->items[7].data = js_fun_depth_str;
	d->items[7].fn = check_number;
	d->items[7].gid = 1;
	d->items[7].gnum = 999999;
	d->items[8].type = D_FIELD;
	d->items[8].dlen = 7;
	d->items[8].data = js_memory_limit_str;
	d->items[8].fn = check_number;
	d->items[8].gid = 1024;
	d->items[8].gnum = 30*1024;
	d->items[9].type = D_BUTTON;
	d->items[9].gid = B_ENTER;
	d->items[9].fn = ok_dialog;
	d->items[9].text = TEXT_(T_OK);
	d->items[10].type = D_BUTTON;
	d->items[10].gid = B_ESC;
	d->items[10].fn = cancel_dialog;
	d->items[10].text = TEXT_(T_CANCEL);
	d->items[11].type = D_END;
	do_dialog(term, d, getml(d, NULL));
}

#endif

#ifdef SUPPORT_IPV6

static unsigned char *ipv6_labels[] = { TEXT_(T_IPV6_DEFAULT), TEXT_(T_IPV6_PREFER_IPV4), TEXT_(T_IPV6_PREFER_IPV6), TEXT_(T_IPV6_USE_ONLY_IPV4), TEXT_(T_IPV6_USE_ONLY_IPV6), NULL };

static int dlg_ipv6_options(struct dialog_data *dlg, struct dialog_item_data *di)
{
	struct ipv6_options *i6o = (struct ipv6_options *)di->cdata;
	struct dialog *d;
	d = mem_calloc(sizeof(struct dialog) + 7 * sizeof(struct dialog_item));
	d->title = TEXT_(T_IPV6_OPTIONS);
	d->fn = checkbox_list_fn;
	d->udata = ipv6_labels;
	d->items[0].type = D_CHECKBOX;
	d->items[0].gid = 1;
	d->items[0].gnum = ADDR_PREFERENCE_DEFAULT;
	d->items[0].dlen = sizeof(int);
	d->items[0].data = (void *)&i6o->addr_preference;
	d->items[1].type = D_CHECKBOX;
	d->items[1].gid = 1;
	d->items[1].gnum = ADDR_PREFERENCE_IPV4;
	d->items[1].dlen = sizeof(int);
	d->items[1].data = (void *)&i6o->addr_preference;
	d->items[2].type = D_CHECKBOX;
	d->items[2].gid = 1;
	d->items[2].gnum = ADDR_PREFERENCE_IPV6;
	d->items[2].dlen = sizeof(int);
	d->items[2].data = (void *)&i6o->addr_preference;
	d->items[3].type = D_CHECKBOX;
	d->items[3].gid = 1;
	d->items[3].gnum = ADDR_PREFERENCE_IPV4_ONLY;
	d->items[3].dlen = sizeof(int);
	d->items[3].data = (void *)&i6o->addr_preference;
	d->items[4].type = D_CHECKBOX;
	d->items[4].gid = 1;
	d->items[4].gnum = ADDR_PREFERENCE_IPV6_ONLY;
	d->items[4].dlen = sizeof(int);
	d->items[4].data = (void *)&i6o->addr_preference;
	d->items[5].type = D_BUTTON;
	d->items[5].gid = B_ENTER;
	d->items[5].fn = ok_dialog;
	d->items[5].text = TEXT_(T_OK);
	d->items[6].type = D_BUTTON;
	d->items[6].gid = B_ESC;
	d->items[6].fn = cancel_dialog;
	d->items[6].text = TEXT_(T_CANCEL);
	d->items[7].type = D_END;
	do_dialog(dlg->win->term, d, getml(d, NULL));
	return 0;
}

#endif

static unsigned char *http_labels[] = { TEXT_(T_USE_HTTP_10), TEXT_(T_ALLOW_SERVER_BLACKLIST), TEXT_(T_BROKEN_302_REDIRECT), TEXT_(T_NO_KEEPALIVE_AFTER_POST_REQUEST), TEXT_(T_DO_NOT_SEND_ACCEPT_CHARSET),
#ifdef HAVE_ANY_COMPRESSION
	TEXT_(T_DO_NOT_ADVERTISE_COMPRESSION_SUPPORT),
#endif
	TEXT_(T_RETRY_ON_INTERNAL_ERRORS), NULL };

static unsigned char *http_header_labels[] = { TEXT_(T_DO_NOT_TRACK), TEXT_(T_REFERER_NONE), TEXT_(T_REFERER_SAME_URL), TEXT_(T_REFERER_FAKE), TEXT_(T_REFERER_REAL_SAME_SERVER), TEXT_(T_REFERER_REAL), TEXT_(T_FAKE_REFERER), TEXT_(T_FAKE_USERAGENT), TEXT_(T_EXTRA_HEADER), NULL };

static void httpheadopt_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = 0;
	checkboxes_width(term, dlg->dlg->udata, dlg->n - 5, &max, max_text_width);
	checkboxes_width(term, dlg->dlg->udata, dlg->n - 5, &min, min_text_width);
	max_text_width(term, http_header_labels[dlg->n - 5], &max, AL_LEFT);
	min_text_width(term, http_header_labels[dlg->n - 5], &min, AL_LEFT);
	max_text_width(term, http_header_labels[dlg->n - 4], &max, AL_LEFT);
	min_text_width(term, http_header_labels[dlg->n - 4], &min, AL_LEFT);
	max_text_width(term, http_header_labels[dlg->n - 3], &max, AL_LEFT);
	min_text_width(term, http_header_labels[dlg->n - 3], &min, AL_LEFT);
	max_buttons_width(term, dlg->items + dlg->n - 2, 2, &max);
	min_buttons_width(term, dlg->items + dlg->n - 2, 2, &min);
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB;
	if (w < 5) w = 5;
	rw = 0;
	dlg_format_checkboxes(dlg, NULL, dlg->items, dlg->n - 5, 0, &y, w, &rw, dlg->dlg->udata);
	y += gf_val(1, 1 * G_BFU_FONT_SIZE);
	dlg_format_text_and_field(dlg, NULL, http_header_labels[dlg->n - 5], dlg->items + dlg->n - 5, 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	if (!dlg->win->term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE * 1);
	dlg_format_text_and_field(dlg, NULL, http_header_labels[dlg->n - 4], dlg->items + dlg->n - 4, 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	if (!dlg->win->term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE * 1);
	dlg_format_text_and_field(dlg, NULL, http_header_labels[dlg->n - 3], dlg->items + dlg->n - 3, 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(1, 1 * G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, NULL, dlg->items + dlg->n - 2, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = rw + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB + gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_checkboxes(dlg, term, dlg->items, dlg->n - 5, dlg->x + DIALOG_LB, &y, w, NULL, dlg->dlg->udata);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text_and_field(dlg, term, http_header_labels[dlg->n - 5], dlg->items + dlg->n - 5, dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	if (!dlg->win->term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE * 1);
	dlg_format_text_and_field(dlg, term, http_header_labels[dlg->n - 4], dlg->items + dlg->n - 4, dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	if (!dlg->win->term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE * 1);
	dlg_format_text_and_field(dlg, term, http_header_labels[dlg->n - 3], dlg->items + dlg->n - 3, dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, dlg->items + dlg->n - 2, 2, dlg->x + DIALOG_LB, &y, w, &rw, AL_CENTER);
}

static int dlg_http_header_options(struct dialog_data *dlg, struct dialog_item_data *di)
{
	struct http_header_options *header = (struct http_header_options *)di->cdata;
	struct dialog *d;
	d = mem_calloc(sizeof(struct dialog) + 11 * sizeof(struct dialog_item));
	d->title = TEXT_(T_HTTP_HEADER_OPTIONS);
	d->fn = httpheadopt_fn;
	d->udata = http_header_labels;
	d->items[0].type = D_CHECKBOX;
	d->items[0].gid = 0;
	d->items[0].dlen = sizeof(int);
	d->items[0].data = (void *)&header->do_not_track;
	d->items[1].type = D_CHECKBOX;
	d->items[1].gid = 1;
	d->items[1].gnum = REFERER_NONE;
	d->items[1].dlen = sizeof(int);
	d->items[1].data = (void *)&header->referer;
	d->items[2].type = D_CHECKBOX;
	d->items[2].gid = 1;
	d->items[2].gnum = REFERER_SAME_URL;
	d->items[2].dlen = sizeof(int);
	d->items[2].data = (void *)&header->referer;
	d->items[3].type = D_CHECKBOX;
	d->items[3].gid = 1;
	d->items[3].gnum = REFERER_FAKE;
	d->items[3].dlen = sizeof(int);
	d->items[3].data = (void *)&header->referer;
	d->items[4].type = D_CHECKBOX;
	d->items[4].gid = 1;
	d->items[4].gnum = REFERER_REAL_SAME_SERVER;
	d->items[4].dlen = sizeof(int);
	d->items[4].data = (void *)&header->referer;
	d->items[5].type = D_CHECKBOX;
	d->items[5].gid = 1;
	d->items[5].gnum = REFERER_REAL;
	d->items[5].dlen = sizeof(int);
	d->items[5].data = (void *)&header->referer;

	d->items[6].type = D_FIELD;
	d->items[6].dlen = MAX_STR_LEN;
	d->items[6].data = header->fake_referer;
	d->items[7].type = D_FIELD;
	d->items[7].dlen = MAX_STR_LEN;
	d->items[7].data = header->fake_useragent;
	d->items[8].type = D_FIELD;
	d->items[8].dlen = MAX_STR_LEN;
	d->items[8].data = header->extra_header;
	d->items[9].type = D_BUTTON;
	d->items[9].gid = B_ENTER;
	d->items[9].fn = ok_dialog;
	d->items[9].text = TEXT_(T_OK);
	d->items[10].type = D_BUTTON;
	d->items[10].gid = B_ESC;
	d->items[10].fn = cancel_dialog;
	d->items[10].text = TEXT_(T_CANCEL);
	d->items[11].type = D_END;
	do_dialog(dlg->win->term, d, getml(d, NULL));
	return 0;
}


static int dlg_http_options(struct dialog_data *dlg, struct dialog_item_data *di)
{
	struct http_options *options = (struct http_options *)di->cdata;
	struct dialog *d;
	int a = 0;
	d = mem_calloc(sizeof(struct dialog) + 10 * sizeof(struct dialog_item));
	d->title = TEXT_(T_HTTP_BUG_WORKAROUNDS);
	d->fn = checkbox_list_fn;
	d->udata = http_labels;
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a].data = (void *)&options->http10;
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a].data = (void *)&options->allow_blacklist;
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a].data = (void *)&options->bug_302_redirect;
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a].data = (void *)&options->bug_post_no_keepalive;
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a].data = (void *)&options->no_accept_charset;
	a++;
#ifdef HAVE_ANY_COMPRESSION
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a].data = (void *)&options->no_compression;
	a++;
#endif
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a].data = (void *)&options->retry_internal_errors;
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = 0;
	d->items[a].fn = dlg_http_header_options;
	d->items[a].text = TEXT_(T_HEADER_OPTIONS);
	d->items[a].data = (void *)&options->header;
	d->items[a].dlen = sizeof(struct http_header_options);
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ENTER;
	d->items[a].fn = ok_dialog;
	d->items[a].text = TEXT_(T_OK);
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ESC;
	d->items[a].fn = cancel_dialog;
	d->items[a].text = TEXT_(T_CANCEL);
	a++;
	d->items[a].type = D_END;
	a++;
	do_dialog(dlg->win->term, d, getml(d, NULL));
	return 0;
}

static unsigned char *ftp_texts[] = { TEXT_(T_PASSWORD_FOR_ANONYMOUS_LOGIN), TEXT_(T_USE_PASSIVE_FTP), TEXT_(T_USE_EPRT_EPSV), TEXT_(T_USE_FAST_FTP), TEXT_(T_SET_TYPE_OF_SERVICE), NULL };

static void ftpopt_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = 0;
	if (dlg->win->term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	max_text_width(term, ftp_texts[0], &max, AL_LEFT);
	min_text_width(term, ftp_texts[0], &min, AL_LEFT);
	checkboxes_width(term, ftp_texts + 1, dlg->n - 3, &max, max_text_width);
	checkboxes_width(term, ftp_texts + 1, dlg->n - 3, &min, min_text_width);
	max_buttons_width(term, dlg->items + dlg->n - 2, 2, &max);
	min_buttons_width(term, dlg->items + dlg->n - 2, 2, &min);
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB;
	if (w < 5) w = 5;
	rw = 0;
	dlg_format_text_and_field(dlg, NULL, ftp_texts[0], dlg->items, 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	dlg_format_checkboxes(dlg, NULL, dlg->items + 1, dlg->n - 3, 0, &y, w, &rw, ftp_texts + 1);
	y += gf_val(1, 1 * G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, NULL, dlg->items + dlg->n - 2, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = rw + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	if (dlg->win->term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text_and_field(dlg, term, ftp_texts[0], dlg->items, dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_checkboxes(dlg, term, dlg->items + 1, dlg->n - 3, dlg->x + DIALOG_LB, &y, w, NULL, ftp_texts + 1);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, dlg->items + dlg->n - 2, 2, dlg->x + DIALOG_LB, &y, w, &rw, AL_CENTER);
}


static int dlg_ftp_options(struct dialog_data *dlg, struct dialog_item_data *di)
{
	int a;
	struct ftp_options *ftp_options = (struct ftp_options *)di->cdata;
	struct dialog *d;
	d = mem_calloc(sizeof(struct dialog) + 7 * sizeof(struct dialog_item));
	d->title = TEXT_(T_FTP_OPTIONS);
	d->fn = ftpopt_fn;
	a=0;
	d->items[a].type = D_FIELD;
	d->items[a].dlen = MAX_STR_LEN;
	d->items[a++].data = ftp_options->anon_pass;
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a].data = (void*)&ftp_options->passive_ftp;
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a].data = (void*)&ftp_options->eprt_epsv;
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a].data = (void*)&ftp_options->fast_ftp;
	a++;
#ifdef HAVE_IPTOS
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a].data = (void*)&ftp_options->set_tos;
	a++;
#endif
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ENTER;
	d->items[a].fn = ok_dialog;
	d->items[a].text = TEXT_(T_OK);
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ESC;
	d->items[a].fn = cancel_dialog;
	d->items[a].text = TEXT_(T_CANCEL);
	a++;
	d->items[a].type = D_END;
	do_dialog(dlg->win->term, d, getml(d, NULL));
	return 0;
}

#ifndef DISABLE_SMB

static unsigned char *smb_labels[] = { TEXT_(T_ALLOW_HYPERLINKS_TO_SMB), NULL };

static int dlg_smb_options(struct dialog_data *dlg, struct dialog_item_data *di)
{
	int a;
	struct smb_options *smb_options = (struct smb_options *)di->cdata;
	struct dialog *d;
	d = mem_calloc(sizeof(struct dialog) + 3 * sizeof(struct dialog_item));
	d->title = TEXT_(T_SMB_OPTIONS);
	d->fn = checkbox_list_fn;
	d->udata = smb_labels;
	a=0;
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a].data = (void*)&smb_options->allow_hyperlinks_to_smb;
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ENTER;
	d->items[a].fn = ok_dialog;
	d->items[a].text = TEXT_(T_OK);
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ESC;
	d->items[a].fn = cancel_dialog;
	d->items[a].text = TEXT_(T_CANCEL);
	a++;
	d->items[a].type = D_END;
	do_dialog(dlg->win->term, d, getml(d, NULL));
	return 0;
}

#endif

#ifdef G

#define VO_GAMMA_LEN 9
static unsigned char disp_red_g[VO_GAMMA_LEN];
static unsigned char disp_green_g[VO_GAMMA_LEN];
static unsigned char disp_blue_g[VO_GAMMA_LEN];
static unsigned char user_g[VO_GAMMA_LEN];
static unsigned char aspect_str[VO_GAMMA_LEN];
tcount gamma_stamp; /* stamp counter for gamma changes */

static void refresh_video(struct session *ses)
{
	display_red_gamma=atof(cast_const_char disp_red_g);
	display_green_gamma=atof(cast_const_char disp_green_g);
	display_blue_gamma=atof(cast_const_char disp_blue_g);
	user_gamma=atof(cast_const_char user_g);
	bfu_aspect=atof(cast_const_char aspect_str);
	/* Flush font cache */
	update_aspect();
	gamma_stamp++;
	
	/* Flush dip_get_color cache */
	gamma_cache_rgb = -2;

	/* Recompute dithering tables for the new gamma */
	init_dither(drv->depth);

	shutdown_bfu();
	init_bfu();
	init_grview();

	/* Redraw all terminals */
	cls_redraw_all_terminals();

}

#define video_msg_0	TEXT_(T_VIDEO_OPTIONS_TEXT)

static unsigned char *video_msg_1[] = {
	TEXT_(T_RED_DISPLAY_GAMMA),
	TEXT_(T_GREEN_DISPLAY_GAMMA),
	TEXT_(T_BLUE_DISPLAY_GAMMA),
	TEXT_(T_USER_GAMMA),
	TEXT_(T_ASPECT_RATIO),
};

static unsigned char *video_msg_2[] = {
	TEXT_(T_DISPLAY_OPTIMIZATION_CRT),
	TEXT_(T_DISPLAY_OPTIMIZATION_LCD_RGB),
	TEXT_(T_DISPLAY_OPTIMIZATION_LCD_BGR),
	TEXT_(T_DITHER_LETTERS),
	TEXT_(T_DITHER_IMAGES),
	TEXT_(T_8_BIT_GAMMA_CORRECTION),
	TEXT_(T_16_BIT_GAMMA_CORRECTION),
	TEXT_(T_AUTO_GAMMA_CORRECTION),
	TEXT_(T_OVERWRITE_SCREEN_INSTEAD_OF_SCROLLING_IT),
};

static void videoopt_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = gf_val(-1, -G_BFU_FONT_SIZE);
	max_text_width(term, video_msg_0, &max, AL_LEFT);
	min_text_width(term, video_msg_0, &min, AL_LEFT);
	max_group_width(term, video_msg_1, dlg->items, 5, &max);
	min_group_width(term, video_msg_1, dlg->items, 5, &min);
	checkboxes_width(term, video_msg_2, dlg->n-2-5, &max, max_text_width);
	checkboxes_width(term, video_msg_2, dlg->n-2-5, &min, min_text_width);
	max_buttons_width(term, dlg->items + dlg->n-2, 2, &max);
	min_buttons_width(term, dlg->items + dlg->n-2, 2, &min);
	w = dlg->win->term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > dlg->win->term->x - 2 * DIALOG_LB) w = dlg->win->term->x - 2 * DIALOG_LB;
	if (w < 1) w = 1;
	rw = 0;
	dlg_format_text(dlg, NULL, video_msg_0, 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_group(dlg, NULL, video_msg_1, dlg->items, 5, 0, &y, w, &rw);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_checkboxes(dlg, NULL, dlg->items+5, dlg->n-2-5, dlg->x + DIALOG_LB, &y, w, &rw, video_msg_2);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, NULL, dlg->items+dlg->n-2, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = w + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	dlg_format_text(dlg, term, video_msg_0, dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	y += gf_val(2, G_BFU_FONT_SIZE);
	dlg_format_group(dlg, term, video_msg_1, dlg->items, 5, dlg->x + DIALOG_LB, &y, w, NULL);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_checkboxes(dlg, term, dlg->items+5, dlg->n-2-5, dlg->x + DIALOG_LB, &y, w, NULL, video_msg_2);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, dlg->items+dlg->n-2, 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
}

static void remove_zeroes(unsigned char *string)
{
	int l=(int)strlen(cast_const_char string);

	while(l&&(string[l-1]=='0')){
		l--;
		string[l]=0;
	}
}

static void video_options(struct terminal *term, void *xxx, struct session *ses)
{
	struct dialog *d;
	int a;
	snprintf(cast_char disp_red_g, VO_GAMMA_LEN, "%f", display_red_gamma);
	remove_zeroes(disp_red_g);
	snprintf(cast_char disp_green_g, VO_GAMMA_LEN, "%f", display_green_gamma);
	remove_zeroes(disp_green_g);
	snprintf(cast_char disp_blue_g, VO_GAMMA_LEN, "%f", display_blue_gamma);
	remove_zeroes(disp_blue_g);
	snprintf(cast_char user_g, VO_GAMMA_LEN, "%f", user_gamma);
	remove_zeroes(user_g);
	snprintf(cast_char aspect_str, VO_GAMMA_LEN, "%f", bfu_aspect);
	remove_zeroes(aspect_str);
	d = mem_calloc(sizeof(struct dialog) + 16 * sizeof(struct dialog_item));
	d->title = TEXT_(T_VIDEO_OPTIONS);
	d->fn = videoopt_fn;
	d->refresh = (void (*)(void *))refresh_video;
	d->refresh_data = ses;
	a=0;
	d->items[a].type = D_FIELD;
	d->items[a].dlen = VO_GAMMA_LEN;
	d->items[a].data = disp_red_g;
	d->items[a].fn = check_float;
	d->items[a].gid = 1;
	d->items[a++].gnum = 10000;
	d->items[a].type = D_FIELD;
	d->items[a].dlen = VO_GAMMA_LEN;
	d->items[a].data = disp_green_g;
	d->items[a].fn = check_float;
	d->items[a].gid = 1;
	d->items[a++].gnum = 10000;
	d->items[a].type = D_FIELD;
	d->items[a].dlen = VO_GAMMA_LEN;
	d->items[a].data = disp_blue_g;
	d->items[a].fn = check_float;
	d->items[a].gid = 1;
	d->items[a++].gnum = 10000;
	d->items[a].type = D_FIELD;
	d->items[a].dlen = VO_GAMMA_LEN;
	d->items[a].data = user_g;
	d->items[a].fn = check_float;
	d->items[a].gid = 1;
	d->items[a++].gnum = 10000;

	d->items[a].type = D_FIELD;
	d->items[a].dlen = VO_GAMMA_LEN;
	d->items[a].data = aspect_str;
	d->items[a].fn = check_float;
	d->items[a].gid = 25;
	d->items[a++].gnum = 400;

	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 1;
	d->items[a].gnum = 0;	/* CRT */
	d->items[a].dlen = sizeof(int);
	d->items[a++].data = (void *)&display_optimize;
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 1;
	d->items[a].gnum = 1;	/* LCD RGB */
	d->items[a].dlen = sizeof(int);
	d->items[a++].data = (void *)&display_optimize;
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 1;
	d->items[a].gnum = 2;	/* LCD BGR*/
	d->items[a].dlen = sizeof(int);
	d->items[a++].data = (void *)&display_optimize;

	
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a++].data = (void *)&dither_letters;
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a++].data = (void *)&dither_images;
	d->items[a].type = D_CHECKBOX;

	d->items[a].gid = 2;
	d->items[a].gnum = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a++].data = (void *)&gamma_bits;
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 2;
	d->items[a].gnum = 1;
	d->items[a].dlen = sizeof(int);
	d->items[a++].data = (void *)&gamma_bits;
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 2;
	d->items[a].gnum = 2;
	d->items[a].dlen = sizeof(int);
	d->items[a++].data = (void *)&gamma_bits;

	if (drv->flags & GD_DONT_USE_SCROLL) {
		d->items[a].type = D_CHECKBOX;
		d->items[a].gid = 0;
		d->items[a].dlen = sizeof(int);
		d->items[a++].data = (void *)&overwrite_instead_of_scroll;
	}

	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ENTER;
	d->items[a].fn = ok_dialog;
	d->items[a++].text = TEXT_(T_OK);
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ESC;
	d->items[a].fn = cancel_dialog;
	d->items[a++].text = TEXT_(T_CANCEL);
	d->items[a].type = D_END;
	do_dialog(term, d, getml(d, NULL));
}

#endif

static unsigned char max_c_str[3];
static unsigned char max_cth_str[3];
static unsigned char max_t_str[3];
static unsigned char time_str[5];
static unsigned char unrtime_str[5];

static void refresh_net(void *xxx)
{
	netcfg_stamp++;
	max_connections = atoi(cast_const_char max_c_str);
	max_connections_to_host = atoi(cast_const_char max_cth_str);
	max_tries = atoi(cast_const_char max_t_str);
	receive_timeout = atoi(cast_const_char time_str);
	unrestartable_receive_timeout = atoi(cast_const_char unrtime_str);
	abort_background_connections();
	register_bottom_half(check_queue, NULL);
}

static unsigned char *proxy_msg[] = {
	TEXT_(T_HTTP_PROXY__HOST_PORT),
	TEXT_(T_FTP_PROXY__HOST_PORT),
#ifdef HAVE_SSL
	TEXT_(T_HTTPS_PROXY__HOST_PORT),
#endif
	TEXT_(T_SOCKS_4A_PROXY__USER_HOST_PORT),
	TEXT_(T_APPEND_TEXT_TO_SOCKS_LOOKUPS),
	TEXT_(T_ONLY_PROXIES),
};

#ifdef HAVE_SSL
#define N_N	5
#else
#define N_N	4
#endif

static void proxy_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int i;
	int y = gf_val(-1, -G_BFU_FONT_SIZE);
	if (dlg->win->term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	for (i = 0; i < N_N; i++) {
		max_text_width(term, proxy_msg[i], &max, AL_LEFT);
		min_text_width(term, proxy_msg[i], &min, AL_LEFT);
	}
	max_group_width(term, proxy_msg + N_N, dlg->items + N_N, dlg->n - 2 - N_N, &max);
	min_group_width(term, proxy_msg + N_N, dlg->items + N_N, dlg->n - 2 - N_N, &min);
	max_buttons_width(term, dlg->items + dlg->n - 2, 2, &max);
	min_buttons_width(term, dlg->items + dlg->n - 2, 2, &min);
	w = dlg->win->term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > dlg->win->term->x - 2 * DIALOG_LB) w = dlg->win->term->x - 2 * DIALOG_LB;
	if (w < 1) w = 1;
	rw = 0;
	for (i = 0; i < N_N; i++) {
		dlg_format_text_and_field(dlg, NULL, proxy_msg[i], &dlg->items[i], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
		if (!dlg->win->term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE * 1);
	}
	dlg_format_group(dlg, NULL, proxy_msg + N_N, dlg->items + N_N, dlg->n - 2 - N_N, 0, &y, w, &rw);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, NULL, dlg->items + dlg->n - 2, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = w + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	if (dlg->win->term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	for (i = 0; i < N_N; i++) {
		dlg_format_text_and_field(dlg, term, proxy_msg[i], &dlg->items[i], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
		if (!dlg->win->term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	}
	dlg_format_group(dlg, term, proxy_msg + N_N, &dlg->items[N_N], dlg->n - 2 - N_N, dlg->x + DIALOG_LB, &y, w, NULL);
	y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, &dlg->items[dlg->n - 2], 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
}

static int dlg_proxy_options(struct dialog_data *dlg, struct dialog_item_data *di)
{
	struct proxies *p = (struct proxies *)di->cdata;
	struct dialog *d;
	int a = 0;
	d = mem_calloc(sizeof(struct dialog) + 8 * sizeof(struct dialog_item));
	d->title = TEXT_(T_PROXIES);
	d->fn = proxy_fn;
	d->items[a].type = D_FIELD;
	d->items[a].dlen = MAX_STR_LEN;
	d->items[a].data = p->http_proxy;
	a++;
	d->items[a].type = D_FIELD;
	d->items[a].dlen = MAX_STR_LEN;
	d->items[a].data = p->ftp_proxy;
	a++;
#ifdef HAVE_SSL
	d->items[a].type = D_FIELD;
	d->items[a].dlen = MAX_STR_LEN;
	d->items[a].data = p->https_proxy;
	a++;
#endif
	d->items[a].type = D_FIELD;
	d->items[a].dlen = MAX_STR_LEN;
	d->items[a].data = p->socks_proxy;
	a++;
	d->items[a].type = D_FIELD;
	d->items[a].dlen = MAX_STR_LEN;
	d->items[a].data = p->dns_append;
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *)&p->only_proxies;
	d->items[a].dlen = sizeof(int);
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ENTER;
	d->items[a].fn = ok_dialog;
	d->items[a].text = TEXT_(T_OK);
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ESC;
	d->items[a].fn = cancel_dialog;
	d->items[a].text = TEXT_(T_CANCEL);
	a++;
	d->items[a].type = D_END;
	do_dialog(dlg->win->term, d, getml(d, NULL));
	return 0;
}

#undef N_N

static unsigned char *net_msg[] = {
	TEXT_(T_MAX_CONNECTIONS),
	TEXT_(T_MAX_CONNECTIONS_TO_ONE_HOST),
	TEXT_(T_RETRIES),
	TEXT_(T_RECEIVE_TIMEOUT_SEC),
	TEXT_(T_TIMEOUT_WHEN_UNRESTARTABLE),
	TEXT_(T_BIND_TO_LOCAL_IP_ADDRESS),
	TEXT_(T_ASYNC_DNS_LOOKUP),
	TEXT_(T_SET_TIME_OF_DOWNLOADED_FILES),
	cast_uchar "",
	cast_uchar "",
	cast_uchar "",
	cast_uchar "",
};

#ifdef SUPPORT_IPV6
static unsigned char *net_msg_ipv6[] = {
	TEXT_(T_MAX_CONNECTIONS),
	TEXT_(T_MAX_CONNECTIONS_TO_ONE_HOST),
	TEXT_(T_RETRIES),
	TEXT_(T_RECEIVE_TIMEOUT_SEC),
	TEXT_(T_TIMEOUT_WHEN_UNRESTARTABLE),
	TEXT_(T_BIND_TO_LOCAL_IP_ADDRESS),
	TEXT_(T_BIND_TO_LOCAL_IPV6_ADDRESS),
	TEXT_(T_ASYNC_DNS_LOOKUP),
	TEXT_(T_SET_TIME_OF_DOWNLOADED_FILES),
	cast_uchar "",
	cast_uchar "",
	cast_uchar "",
	cast_uchar "",
	cast_uchar "",
};
#endif

static void net_options(struct terminal *term, void *xxx, void *yyy)
{
	struct dialog *d;
	int a;
	snprint(max_c_str, 3, max_connections);
	snprint(max_cth_str, 3, max_connections_to_host);
	snprint(max_t_str, 3, max_tries);
	snprint(time_str, 5, receive_timeout);
	snprint(unrtime_str, 5, unrestartable_receive_timeout);
	d = mem_calloc(sizeof(struct dialog) + 16 * sizeof(struct dialog_item));
	d->title = TEXT_(T_NETWORK_OPTIONS);
	d->fn = group_fn;
#ifdef SUPPORT_IPV6
	if (support_ipv6) d->udata = net_msg_ipv6;
	else
#endif
		d->udata = net_msg;
	d->refresh = (void (*)(void *))refresh_net;
	a=0;
	d->items[a].type = D_FIELD;
	d->items[a].data = max_c_str;
	d->items[a].dlen = 3;
	d->items[a].fn = check_number;
	d->items[a].gid = 1;
	d->items[a++].gnum = 99;
	d->items[a].type = D_FIELD;
	d->items[a].data = max_cth_str;
	d->items[a].dlen = 3;
	d->items[a].fn = check_number;
	d->items[a].gid = 1;
	d->items[a++].gnum = 99;
	d->items[a].type = D_FIELD;
	d->items[a].data = max_t_str;
	d->items[a].dlen = 3;
	d->items[a].fn = check_number;
	d->items[a].gid = 0;
	d->items[a++].gnum = 16;
	d->items[a].type = D_FIELD;
	d->items[a].data = time_str;
	d->items[a].dlen = 5;
	d->items[a].fn = check_number;
	d->items[a].gid = 1;
	d->items[a++].gnum = 9999;
	d->items[a].type = D_FIELD;
	d->items[a].data = unrtime_str;
	d->items[a].dlen = 5;
	d->items[a].fn = check_number;
	d->items[a].gid = 1;
	d->items[a++].gnum = 9999;
	d->items[a].type = D_FIELD;
	d->items[a].data = bind_ip_address;
	d->items[a].dlen = sizeof(bind_ip_address);
	d->items[a++].fn = check_local_ip_address;
#ifdef SUPPORT_IPV6
	if (support_ipv6) {
		d->items[a].type = D_FIELD;
		d->items[a].data = bind_ipv6_address;
		d->items[a].dlen = sizeof(bind_ipv6_address);
		d->items[a++].fn = check_local_ipv6_address;
	}
#endif
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *)&async_lookup;
	d->items[a++].dlen = sizeof(int);
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *)&download_utime;
	d->items[a++].dlen = sizeof(int);
#ifdef SUPPORT_IPV6
	if (support_ipv6) {
		d->items[a].type = D_BUTTON;
		d->items[a].gid = 0;
		d->items[a].fn = dlg_ipv6_options;
		d->items[a].text = TEXT_(T_IPV6_OPTIONS);
		d->items[a].data = (unsigned char *)&ipv6_options;
		d->items[a++].dlen = sizeof(struct ipv6_options);
	}
#endif
	d->items[a].type = D_BUTTON;
	d->items[a].gid = 0;
	d->items[a].fn = dlg_proxy_options;
	d->items[a].text = TEXT_(T_PROXIES);
	d->items[a].data = (unsigned char *)&proxies;
	d->items[a++].dlen = sizeof(struct proxies);
	d->items[a].type = D_BUTTON;
	d->items[a].gid = 0;
	d->items[a].fn = dlg_http_options;
	d->items[a].text = TEXT_(T_HTTP_OPTIONS);
	d->items[a].data = (unsigned char *)&http_options;
	d->items[a++].dlen = sizeof(struct http_options);
	d->items[a].type = D_BUTTON;
	d->items[a].gid = 0;
	d->items[a].fn = dlg_ftp_options;
	d->items[a].text = TEXT_(T_FTP_OPTIONS);
	d->items[a].data = (unsigned char *)&ftp_options;
	d->items[a++].dlen = sizeof(struct ftp_options);
#ifndef DISABLE_SMB
	d->items[a].type = D_BUTTON;
	d->items[a].gid = 0;
	d->items[a].fn = dlg_smb_options;
	d->items[a].text = TEXT_(T_SMB_OPTIONS);
	d->items[a].data = (unsigned char *)&smb_options;
	d->items[a++].dlen = sizeof(struct smb_options);
#endif
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ENTER;
	d->items[a].fn = ok_dialog;
	d->items[a++].text = TEXT_(T_OK);
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ESC;
	d->items[a].fn = cancel_dialog;
	d->items[a++].text = TEXT_(T_CANCEL);
	d->items[a].type = D_END;
	do_dialog(term, d, getml(d, NULL));
}

static unsigned char *prg_msg[] = {
	TEXT_(T_MAILTO_PROG),
	TEXT_(T_TELNET_PROG),
	TEXT_(T_TN3270_PROG),
	TEXT_(T_MMS_PROG),
	TEXT_(T_MAGNET_PROG),
	TEXT_(T_SHELL_PROG),
	cast_uchar ""
};

static void netprog_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	int max = 0, min = 0;
	int w, rw;
	int y = gf_val(-1, -G_BFU_FONT_SIZE);
	int a;
	a=0;
	max_text_width(term, prg_msg[a], &max, AL_LEFT);
	min_text_width(term, prg_msg[a++], &min, AL_LEFT);
	max_text_width(term, prg_msg[a], &max, AL_LEFT);
	min_text_width(term, prg_msg[a++], &min, AL_LEFT);
	max_text_width(term, prg_msg[a], &max, AL_LEFT);
	min_text_width(term, prg_msg[a++], &min, AL_LEFT);
	max_text_width(term, prg_msg[a], &max, AL_LEFT);
	min_text_width(term, prg_msg[a++], &min, AL_LEFT);
	max_text_width(term, prg_msg[a], &max, AL_LEFT);
	min_text_width(term, prg_msg[a++], &min, AL_LEFT);
#ifdef G
	if (have_extra_exec()) {
		max_text_width(term, prg_msg[a], &max, AL_LEFT);
		min_text_width(term, prg_msg[a++], &min, AL_LEFT);
	}
#endif
	max_buttons_width(term, dlg->items + a, 2, &max);
	min_buttons_width(term, dlg->items + a, 2, &min);
	w = dlg->win->term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > dlg->win->term->x - 2 * DIALOG_LB) w = dlg->win->term->x - 2 * DIALOG_LB;
	if (w < 1) w = 1;
	rw = 0;
	a=0;
	if (term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text_and_field(dlg, NULL, prg_msg[a], &dlg->items[a], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	a++;
	if (!term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text_and_field(dlg, NULL, prg_msg[a], &dlg->items[a], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	a++;
	if (!term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text_and_field(dlg, NULL, prg_msg[a], &dlg->items[a], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	a++;
	if (!term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text_and_field(dlg, NULL, prg_msg[a], &dlg->items[a], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	a++;
	if (!term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text_and_field(dlg, NULL, prg_msg[a], &dlg->items[a], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
	a++;
	if (!term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
#ifdef G
	if (have_extra_exec()) {
		dlg_format_text_and_field(dlg, NULL, prg_msg[a], &dlg->items[a], 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
		a++;
		if (!term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	}
#endif
	if (term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, NULL, dlg->items + a, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = w + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
	if (term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	a=0;
	dlg_format_text_and_field(dlg, term, prg_msg[a], &dlg->items[a], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	a++;
	if (!term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text_and_field(dlg, term, prg_msg[a], &dlg->items[a], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	a++;
	if (!term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text_and_field(dlg, term, prg_msg[a], &dlg->items[a], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	a++;
	if (!term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text_and_field(dlg, term, prg_msg[a], &dlg->items[a], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	a++;
	if (!term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_text_and_field(dlg, term, prg_msg[a], &dlg->items[a], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
	a++;
	if (!term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
#ifdef G
	if (have_extra_exec()) {	
		dlg_format_text_and_field(dlg, term, prg_msg[a], &dlg->items[a], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
		a++;
		if (!term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	}
#endif
	if (term->spec->braille) y += gf_val(1, G_BFU_FONT_SIZE);
	dlg_format_buttons(dlg, term, &dlg->items[a], 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
}

static void net_programs(struct terminal *term, void *xxx, void *yyy)
{
	struct dialog *d;
	int a;
	d = mem_calloc(sizeof(struct dialog) + 8 * sizeof(struct dialog_item));
#ifdef G
	if (have_extra_exec()) d->title = TEXT_(T_MAIL_TELNET_AND_SHELL_PROGRAMS);
	else
#endif
		d->title = TEXT_(T_MAIL_AND_TELNET_PROGRAMS);

	d->fn = netprog_fn;
	a=0;
	d->items[a].type = D_FIELD;
	d->items[a].dlen = MAX_STR_LEN;
	d->items[a++].data = get_prog(&mailto_prog);
	d->items[a].type = D_FIELD;
	d->items[a].dlen = MAX_STR_LEN;
	d->items[a++].data = get_prog(&telnet_prog);
	d->items[a].type = D_FIELD;
	d->items[a].dlen = MAX_STR_LEN;
	d->items[a++].data = get_prog(&tn3270_prog);
	d->items[a].type = D_FIELD;
	d->items[a].dlen = MAX_STR_LEN;
	d->items[a++].data = get_prog(&mms_prog);
	d->items[a].type = D_FIELD;
	d->items[a].dlen = MAX_STR_LEN;
	d->items[a++].data = get_prog(&magnet_prog);
#ifdef G
	if (have_extra_exec()) {
		d->items[a].type = D_FIELD;
		d->items[a].dlen = MAX_STR_LEN;
		d->items[a++].data = drv->shell;
	}
#endif
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ENTER;
	d->items[a].fn = ok_dialog;
	d->items[a++].text = TEXT_(T_OK);
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ESC;
	d->items[a].fn = cancel_dialog;
	d->items[a++].text = TEXT_(T_CANCEL);
	d->items[a].type = D_END;
	do_dialog(term, d, getml(d, NULL));
}

static unsigned char mc_str[8];
#ifdef G
static unsigned char ic_str[8];
static unsigned char fc_str[8];
#endif
static unsigned char doc_str[4];

static void cache_refresh(void *xxx)
{
	memory_cache_size = atoi(cast_const_char mc_str) * 1024;
#ifdef G
	if (F) {
		image_cache_size = atoi(cast_const_char ic_str) * 1024;
		font_cache_size = atoi(cast_const_char fc_str) * 1024;
	}
#endif
	max_format_cache_entries = atoi(cast_const_char doc_str);
	shrink_memory(SH_CHECK_QUOTA, 0);
}

static unsigned char *cache_texts[] = { TEXT_(T_MEMORY_CACHE_SIZE__KB), TEXT_(T_NUMBER_OF_FORMATTED_DOCUMENTS), TEXT_(T_AGGRESSIVE_CACHE) };
#ifdef G
static unsigned char *g_cache_texts[] = { TEXT_(T_MEMORY_CACHE_SIZE__KB), TEXT_(T_IMAGE_CACHE_SIZE__KB), TEXT_(T_FONT_CACHE_SIZE__KB), TEXT_(T_NUMBER_OF_FORMATTED_DOCUMENTS), TEXT_(T_AGGRESSIVE_CACHE) };
#endif

static void cache_opt(struct terminal *term, void *xxx, void *yyy)
{
	struct dialog *d;
	int a;
	snprint(mc_str, 8, memory_cache_size / 1024);
#ifdef G
	if (F) {
		snprint(ic_str, 8, image_cache_size / 1024);
		snprint(fc_str, 8, font_cache_size / 1024);
	}
#endif
	snprint(doc_str, 4, max_format_cache_entries);
#ifdef G
	if (F) {
		d = mem_calloc(sizeof(struct dialog) + 7 * sizeof(struct dialog_item));
	} else
#endif
	{
		d = mem_calloc(sizeof(struct dialog) + 5 * sizeof(struct dialog_item));
	}
	a=0;
	d->title = TEXT_(T_CACHE_OPTIONS);
	d->fn = group_fn;
#ifdef G
	if (F) d->udata = g_cache_texts;
	else
#endif
	d->udata = cache_texts;
	d->refresh = (void (*)(void *))cache_refresh;
	d->items[a].type = D_FIELD;
	d->items[a].dlen = 8;
	d->items[a].data = mc_str;
	d->items[a].fn = check_number;
	d->items[a].gid = 0;
	d->items[a].gnum = MAXINT / 1024;
	a++;
#ifdef G
	if (F)
	{
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 8;
		d->items[a].data = ic_str;
		d->items[a].fn = check_number;
		d->items[a].gid = 0;
		d->items[a].gnum = MAXINT / 1024;
		a++;
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 8;
		d->items[a].data = fc_str;
		d->items[a].fn = check_number;
		d->items[a].gid = 0;
		d->items[a].gnum = MAXINT / 1024;
		a++;
	}
#endif
	d->items[a].type = D_FIELD;
	d->items[a].dlen = 4;
	d->items[a].data = doc_str;
	d->items[a].fn = check_number;
	d->items[a].gid = 0;
	d->items[a].gnum = 999;
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].gid = 0;
	d->items[a].dlen = sizeof(int);
	d->items[a].data = (void *)&aggressive_cache;
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ENTER;
	d->items[a].fn = ok_dialog;
	d->items[a].text = TEXT_(T_OK);
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ESC;
	d->items[a].fn = cancel_dialog;
	d->items[a].text = TEXT_(T_CANCEL);
	a++;
	d->items[a].type = D_END;
	do_dialog(term, d, getml(d, NULL));
}

static void menu_shell(struct terminal *term, void *xxx, void *yyy)
{
	unsigned char *sh;
	if (!(sh = cast_uchar GETSHELL)) sh = cast_uchar DEFAULT_SHELL;
	exec_on_terminal(term, sh, cast_uchar "", 1);
}

static void menu_kill_background_connections(struct terminal *term, void *xxx, void *yyy)
{
	abort_background_connections();
}

static void menu_kill_all_connections(struct terminal *term, void *xxx, void *yyy)
{
	abort_all_connections();
}

static void menu_save_html_options(struct terminal *term, void *xxx, struct session *ses)
{
	memcpy(&dds, &ses->ds, sizeof(struct document_setup));
	write_html_config(term);
}

static unsigned char marg_str[2];
#ifdef G
static unsigned char html_font_str[4];
static unsigned char image_scale_str[6];
#endif

static void html_refresh(struct session *ses)
{
	ses->ds.margin = atoi(cast_const_char marg_str);
#ifdef G
	if (F) {
		ses->ds.font_size = atoi(cast_const_char html_font_str);
		ses->ds.image_scale = atoi(cast_const_char image_scale_str);
	}
#endif
	html_interpret_recursive(ses->screen);
	draw_formatted(ses);
}

#ifdef G
static unsigned char *html_texts_g[] = { TEXT_(T_DISPLAY_TABLES),
	TEXT_(T_DISPLAY_FRAMES), TEXT_(T_DISPLAY_LINKS_TO_IMAGES),
	TEXT_(T_DISPLAY_IMAGE_FILENAMES), TEXT_(T_DISPLAY_IMAGES),
	TEXT_(T_AUTO_REFRESH), TEXT_(T_TARGET_IN_NEW_WINDOW), TEXT_(T_TEXT_MARGIN),
	cast_uchar "", TEXT_(T_IGNORE_CHARSET_INFO_SENT_BY_SERVER), TEXT_(T_USER_FONT_SIZE),
	TEXT_(T_SCALE_ALL_IMAGES_BY), TEXT_(T_PORN_ENABLE)
};
#endif

static unsigned char *html_texts[] = { TEXT_(T_DISPLAY_TABLES), TEXT_(T_DISPLAY_FRAMES), TEXT_(T_DISPLAY_LINKS_TO_IMAGES), TEXT_(T_DISPLAY_IMAGE_FILENAMES), TEXT_(T_LINK_ORDER_BY_COLUMNS), TEXT_(T_NUMBERED_LINKS), TEXT_(T_AUTO_REFRESH), TEXT_(T_TARGET_IN_NEW_WINDOW), TEXT_(T_TEXT_MARGIN), cast_uchar "", TEXT_(T_IGNORE_CHARSET_INFO_SENT_BY_SERVER) };

static int dlg_assume_cp(struct dialog_data *dlg, struct dialog_item_data *di)
{
	charset_sel_list(dlg->win->term, (int *)di->cdata, 1);
	return 0;
}

#ifdef G
static int dlg_kb_cp(struct dialog_data *dlg, struct dialog_item_data *di)
{
	charset_sel_list(dlg->win->term, (int *)di->cdata,
#ifdef DOS
		0
#else
		1
#endif
	);
	return 0;
}
#endif

static void menu_html_options(struct terminal *term, void *xxx, struct session *ses)
{
	struct dialog *d;
	int a;
	
	snprint(marg_str, 2, ses->ds.margin);
	if (!F) {
		d = mem_calloc(sizeof(struct dialog) + 13 * sizeof(struct dialog_item));
#ifdef G
	} else {
		d = mem_calloc(sizeof(struct dialog) + 15 * sizeof(struct dialog_item));
		snprintf(cast_char html_font_str,4,"%d",ses->ds.font_size);
		snprintf(cast_char image_scale_str,6,"%d",ses->ds.image_scale);
#endif
	}
	d->title = TEXT_(T_HTML_OPTIONS);
	d->fn = group_fn;
	d->udata = gf_val(html_texts, html_texts_g);
	d->udata2 = ses;
	d->refresh = (void (*)(void *))html_refresh;
	d->refresh_data = ses;
	a=0;
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *) &ses->ds.tables;
	d->items[a].dlen = sizeof(int);
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *) &ses->ds.frames;
	d->items[a].dlen = sizeof(int);
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *) &ses->ds.images;
	d->items[a].dlen = sizeof(int);
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *) &ses->ds.image_names;
	d->items[a].dlen = sizeof(int);
	a++;
#ifdef G
	if (F) {
		d->items[a].type = D_CHECKBOX;
		d->items[a].data = (unsigned char *) &ses->ds.display_images;
		d->items[a].dlen = sizeof(int);
		a++;
	}
#endif
	if (!F) {
		d->items[a].type = D_CHECKBOX;
		d->items[a].data = (unsigned char *) &ses->ds.table_order;
		d->items[a].dlen = sizeof(int);
		a++;
		d->items[a].type = D_CHECKBOX;
		d->items[a].data = (unsigned char *) &ses->ds.num_links;
		d->items[a].dlen = sizeof(int);
		a++;
	}
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *) &ses->ds.auto_refresh;
	d->items[a].dlen = sizeof(int);
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *) &ses->ds.target_in_new_window;
	d->items[a].dlen = sizeof(int);
	a++;
	d->items[a].type = D_FIELD;
	d->items[a].dlen = 2;
	d->items[a].data = marg_str;
	d->items[a].fn = check_number;
	d->items[a].gid = 0;
	d->items[a].gnum = 9;
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = 0;
	d->items[a].fn = dlg_assume_cp;
	d->items[a].text = TEXT_(T_DEFAULT_CODEPAGE);
	d->items[a].data = (unsigned char *) &ses->ds.assume_cp;
	d->items[a].dlen = sizeof(int);
	a++;
	d->items[a].type = D_CHECKBOX;
	d->items[a].data = (unsigned char *) &ses->ds.hard_assume;
	d->items[a].dlen = sizeof(int);
	a++;
#ifdef G
	if (F) {
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 4;
		d->items[a].data = html_font_str;
		d->items[a].fn = check_number;
		d->items[a].gid = 1;
		d->items[a].gnum = MAX_FONT_SIZE;
		a++;
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 4;
		d->items[a].data = image_scale_str;
		d->items[a].fn = check_number;
		d->items[a].gid = 1;
		d->items[a].gnum = 500;
		a++;
		d->items[a].type = D_CHECKBOX;
		d->items[a].data = (unsigned char *) &ses->ds.porn_enable;
		d->items[a].dlen = sizeof(int);
		a++;
	}
#endif
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ENTER;
	d->items[a].fn = ok_dialog;
	d->items[a].text = TEXT_(T_OK);
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ESC;
	d->items[a].fn = cancel_dialog;
	d->items[a].text = TEXT_(T_CANCEL);
	a++;
	d->items[a].type = D_END;
	do_dialog(term, d, getml(d, NULL));
}

static unsigned char *color_texts[] = { cast_uchar "", cast_uchar "", cast_uchar "", TEXT_(T_IGNORE_DOCUMENT_COLOR) };

#ifdef G
static unsigned char *color_texts_g[] = { TEXT_(T_TEXT_COLOR), TEXT_(T_LINK_COLOR), TEXT_(T_BACKGROUND_COLOR), TEXT_(T_IGNORE_DOCUMENT_COLOR) };

static unsigned char g_text_color_str[7];
static unsigned char g_link_color_str[7];
static unsigned char g_background_color_str[7];
#endif

static void html_color_refresh(struct session *ses)
{
#ifdef G
	if (F) {
		ses->ds.g_text_color = (int)strtol(cast_const_char g_text_color_str, NULL, 16);
		ses->ds.g_link_color = (int)strtol(cast_const_char g_link_color_str, NULL, 16);
		ses->ds.g_background_color = (int)strtol(cast_const_char g_background_color_str, NULL, 16);
	}
#endif
	html_interpret_recursive(ses->screen);
	draw_formatted(ses);
}

static void select_color(struct terminal *term, int n, int *ptr)
{
	int i;
	struct menu_item *mi;
	mi = new_menu(1);
	for (i = 0; i < n; i++) {
		add_to_menu(&mi, TEXT_(T_COLOR_0 + i), cast_uchar "", cast_uchar "", MENU_FUNC set_val, (void *)(unsigned long)i, 0, i);
	}
	do_menu_selected(term, mi, ptr, *ptr, NULL, NULL);
}

static int select_color_8(struct dialog_data *dlg, struct dialog_item_data *di)
{
	select_color(dlg->win->term, 8, (int *)di->cdata);
	return 0;
}

static int select_color_16(struct dialog_data *dlg, struct dialog_item_data *di)
{
	select_color(dlg->win->term, 16, (int *)di->cdata);
	return 0;
}

static void menu_color(struct terminal *term, void *xxx, struct session *ses)
{
	struct dialog *d;

#ifdef G
	if (F) {
		snprintf(cast_char g_text_color_str, 7, "%06x", (unsigned)ses->ds.g_text_color);
		snprintf(cast_char g_link_color_str, 7, "%06x", (unsigned)ses->ds.g_link_color);
		snprintf(cast_char g_background_color_str,7,"%06x", (unsigned)ses->ds.g_background_color);
	}
#endif

	d = mem_calloc(sizeof(struct dialog) + 6 * sizeof(struct dialog_item));
	d->title = TEXT_(T_COLOR);
	d->fn = group_fn;
	d->udata = gf_val(color_texts, color_texts_g);
	d->udata2 = ses;
	d->refresh = (void (*)(void *))html_color_refresh;
	d->refresh_data = ses;

	if (!F) {
		d->items[0].type = D_BUTTON;
		d->items[0].gid = 0;
		d->items[0].text = TEXT_(T_TEXT_COLOR);
		d->items[0].fn = select_color_16;
		d->items[0].data = (unsigned char *)&ses->ds.t_text_color;
		d->items[0].dlen = sizeof(int);
	
		d->items[1].type = D_BUTTON;
		d->items[1].gid = 0;
		d->items[1].text = TEXT_(T_LINK_COLOR);
		d->items[1].fn = select_color_16;
		d->items[1].data = (unsigned char *)&ses->ds.t_link_color;
		d->items[1].dlen = sizeof(int);
	
		d->items[2].type = D_BUTTON;
		d->items[2].gid = 0;
		d->items[2].text = TEXT_(T_BACKGROUND_COLOR);
		d->items[2].fn = select_color_8;
		d->items[2].data = (unsigned char *)&ses->ds.t_background_color;
		d->items[2].dlen = sizeof(int);
	}
#ifdef G
	else {
		d->items[0].type = D_FIELD;
		d->items[0].dlen = 7;
		d->items[0].data = g_text_color_str;
		d->items[0].fn = check_hex_number;
		d->items[0].gid = 0;
		d->items[0].gnum = 0xffffff;

		d->items[1].type = D_FIELD;
		d->items[1].dlen = 7;
		d->items[1].data = g_link_color_str;
		d->items[1].fn = check_hex_number;
		d->items[1].gid = 0;
		d->items[1].gnum = 0xffffff;

		d->items[2].type = D_FIELD;
		d->items[2].dlen = 7;
		d->items[2].data = g_background_color_str;
		d->items[2].fn = check_hex_number;
		d->items[2].gid = 0;
		d->items[2].gnum = 0xffffff;
	}
#endif

	d->items[3].type = D_CHECKBOX;
	d->items[3].data = (unsigned char *) gf_val(&ses->ds.t_ignore_document_color, &ses->ds.g_ignore_document_color);
	d->items[3].dlen = sizeof(int);

	d->items[4].type = D_BUTTON;
	d->items[4].gid = B_ENTER;
	d->items[4].fn = ok_dialog;
	d->items[4].text = TEXT_(T_OK);

	d->items[5].type = D_BUTTON;
	d->items[5].gid = B_ESC;
	d->items[5].fn = cancel_dialog;
	d->items[5].text = TEXT_(T_CANCEL);

	d->items[6].type = D_END;

	do_dialog(term, d, getml(d, NULL));
}

static unsigned char new_bookmarks_file[MAX_STR_LEN];
static int new_bookmarks_codepage;

#ifdef G
static unsigned char menu_font_str[4];
static unsigned char bg_color_str[7];
static unsigned char fg_color_str[7];
static unsigned char scroll_area_color_str[7];
static unsigned char scroll_bar_color_str[7];
static unsigned char scroll_frame_color_str[7];
#endif

static void refresh_misc(struct session *ses)
{
#ifdef G
	if (F) {
		struct session *ses;

		menu_font_size=(int)strtol(cast_const_char menu_font_str,NULL,10);
		G_BFU_FG_COLOR=(int)strtol(cast_const_char fg_color_str,NULL,16);
		G_BFU_BG_COLOR=(int)strtol(cast_const_char bg_color_str,NULL,16);
		G_SCROLL_BAR_AREA_COLOR=(int)strtol(cast_const_char scroll_area_color_str,NULL,16);
		G_SCROLL_BAR_BAR_COLOR=(int)strtol(cast_const_char scroll_bar_color_str,NULL,16);
		G_SCROLL_BAR_FRAME_COLOR=(int)strtol(cast_const_char scroll_frame_color_str,NULL,16);
		shutdown_bfu();
		init_bfu();
		init_grview();
		foreach(ses, sessions) {
			ses->term->dev->resize_handler(ses->term->dev);
		}
	}
#endif
	if (strcmp(cast_const_char new_bookmarks_file, cast_const_char bookmarks_file) || new_bookmarks_codepage != bookmarks_codepage)
	{
		reinit_bookmarks(ses, new_bookmarks_file, new_bookmarks_codepage);
	}
}

#ifdef G
static unsigned char *miscopt_labels_g[] = { TEXT_(T_MENU_FONT_SIZE), TEXT_(T_ENTER_COLORS_AS_RGB_TRIPLETS), TEXT_(T_MENU_FOREGROUND_COLOR), TEXT_(T_MENU_BACKGROUND_COLOR), TEXT_(T_SCROLL_BAR_AREA_COLOR), TEXT_(T_SCROLL_BAR_BAR_COLOR), TEXT_(T_SCROLL_BAR_FRAME_COLOR), TEXT_(T_BOOKMARKS_FILE), NULL };
#endif
static unsigned char *miscopt_labels[] = { TEXT_(T_BOOKMARKS_FILE), NULL };
static unsigned char *miscopt_checkbox_labels[] = { TEXT_(T_SAVE_URL_HISTORY_ON_EXIT), NULL };

static void miscopt_fn(struct dialog_data *dlg)
{
	struct terminal *term = dlg->win->term;
	unsigned char **labels=dlg->dlg->udata;
	int max = 0, min = 0;
	int w, rw;
	int y = 0;
	int a=0;
	int bmk=!anonymous;

#ifdef G
	if (F&&((drv->flags)&GD_NEED_CODEPAGE))a=1;
#endif

	max_text_width(term, labels[F?7:0], &max, AL_LEFT);
	min_text_width(term, labels[F?7:0], &min, AL_LEFT);
#ifdef G
	if (F)
	{
		max_text_width(term, labels[1], &max, AL_LEFT);
		min_text_width(term, labels[1], &min, AL_LEFT);
		max_group_width(term, labels, dlg->items, 1, &max);
		min_group_width(term, labels, dlg->items, 1, &min);
		max_group_width(term, labels + 2, dlg->items+1, 5, &max);
		min_group_width(term, labels + 2, dlg->items+1, 5, &min);
	}
#endif
	if (bmk)
	{
		max_buttons_width(term, dlg->items + dlg->n - 3 - a - bmk, 1, &max);
		min_buttons_width(term, dlg->items + dlg->n - 3 - a - bmk, 1, &min);
	}
	if (a)
	{
		max_buttons_width(term, dlg->items + dlg->n - 3 - bmk, 1, &max);
		min_buttons_width(term, dlg->items + dlg->n - 3 - bmk, 1, &min);
	}
	if (bmk) {
		checkboxes_width(term, miscopt_checkbox_labels, 1, &max, max_text_width);
		checkboxes_width(term, miscopt_checkbox_labels, 1, &min, min_text_width);
	}
	max_buttons_width(term, dlg->items + dlg->n - 2, 2, &max);
	min_buttons_width(term, dlg->items + dlg->n - 2, 2, &min);
	w = term->x * 9 / 10 - 2 * DIALOG_LB;
	if (w > max) w = max;
	if (w < min) w = min;
	if (w > term->x - 2 * DIALOG_LB) w = term->x - 2 * DIALOG_LB;
	if (w < 5) w = 5;
	rw = 0;

#ifdef G
	if (F)
	{
		dlg_format_group(dlg, NULL, labels, dlg->items,1,dlg->x + DIALOG_LB, &y, w, &rw);
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_text(dlg, NULL, labels[1], dlg->x + DIALOG_LB, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_group(dlg, NULL, labels+2, dlg->items+1,5,dlg->x + DIALOG_LB, &y, w, &rw);
		y += gf_val(1, G_BFU_FONT_SIZE);
	}
#endif
	if (bmk)
	{
		dlg_format_text_and_field(dlg, NULL, labels[F?7:0], dlg->items + dlg->n - 4 - a - bmk, 0, &y, w, &rw, COLOR_DIALOG_TEXT, AL_LEFT);
		y += gf_val(1, G_BFU_FONT_SIZE);
	}
	if (bmk) {
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_buttons(dlg, NULL, dlg->items + dlg->n - 3 - a - bmk, 1, 0, &y, w, &rw, AL_LEFT);
	}
	if (a) dlg_format_buttons(dlg, NULL, dlg->items + dlg->n - 3 - bmk, 1, 0, &y, w, &rw, AL_LEFT);
	if (bmk) dlg_format_checkboxes(dlg, NULL, dlg->items + dlg->n - 3, 1, 0, &y, w, &rw, miscopt_checkbox_labels);
	dlg_format_buttons(dlg, NULL, dlg->items + dlg->n - 2, 2, 0, &y, w, &rw, AL_CENTER);
	w = rw;
	dlg->xw = w + 2 * DIALOG_LB;
	dlg->yw = y + 2 * DIALOG_TB;
	center_dlg(dlg);
	draw_dlg(dlg);
	y = dlg->y + DIALOG_TB;
#ifdef G
	if (F)
	{
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_group(dlg, term, labels, dlg->items,1,dlg->x + DIALOG_LB, &y, w, NULL);
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_text(dlg, term, labels[1], dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_group(dlg, term, labels+2, dlg->items+1,5,dlg->x + DIALOG_LB, &y, w, NULL);
		y += gf_val(1, G_BFU_FONT_SIZE);
	} else
#endif
	{
		y += gf_val(1, G_BFU_FONT_SIZE);
	}
	if (bmk)
	{
		dlg_format_text_and_field(dlg, term, labels[F?7:0], dlg->items + dlg->n - 4 - a - bmk, dlg->x + DIALOG_LB, &y, w, NULL, COLOR_DIALOG_TEXT, AL_LEFT);
		y += gf_val(1, G_BFU_FONT_SIZE);
		dlg_format_buttons(dlg, term, dlg->items + dlg->n - 3 - a - bmk, 1, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
	}
	if (a) dlg_format_buttons(dlg, term, dlg->items + dlg->n - 3 - bmk, 1, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
	if (bmk) {
		dlg_format_checkboxes(dlg, term, dlg->items + dlg->n - 3, 1, dlg->x + DIALOG_LB, &y, w, NULL, miscopt_checkbox_labels);
		y += gf_val(1, G_BFU_FONT_SIZE);
	}
	dlg_format_buttons(dlg, term, dlg->items+dlg->n-2, 2, dlg->x + DIALOG_LB, &y, w, NULL, AL_CENTER);
}


static void miscelaneous_options(struct terminal *term, void *xxx, struct session *ses)
{
	struct dialog *d;
	int a=0;

	if (anonymous&&!F) return;	/* if you add something into text mode (or both text and graphics), remove this (and enable also miscelaneous_options in setip_menu_anon) */

	safe_strncpy(new_bookmarks_file,bookmarks_file,MAX_STR_LEN);
	new_bookmarks_codepage=bookmarks_codepage;
	if (!F) {
		d = mem_calloc(sizeof(struct dialog) + 5 * sizeof(struct dialog_item));
	}
#ifdef G
	else {
		d = mem_calloc(sizeof(struct dialog) + 12 * sizeof(struct dialog_item));
		snprintf(cast_char menu_font_str,4,"%d",menu_font_size);
		snprintf(cast_char fg_color_str,7,"%06x",(unsigned) G_BFU_FG_COLOR);
		snprintf(cast_char bg_color_str,7,"%06x",(unsigned) G_BFU_BG_COLOR);
		snprintf(cast_char scroll_bar_color_str,7,"%06x",(unsigned) G_SCROLL_BAR_BAR_COLOR);
		snprintf(cast_char scroll_area_color_str,7,"%06x",(unsigned) G_SCROLL_BAR_AREA_COLOR);
		snprintf(cast_char scroll_frame_color_str,7,"%06x",(unsigned) G_SCROLL_BAR_FRAME_COLOR);
	}
#endif
	d->title = TEXT_(T_MISCELANEOUS_OPTIONS);
	d->refresh = (void (*)(void *))refresh_misc;
	d->refresh_data = ses;
	d->fn=miscopt_fn;
	if (!F)
		d->udata = miscopt_labels;
#ifdef G
	else 
		d->udata = miscopt_labels_g;
#endif
	d->udata2 = ses;
#ifdef G
	if (F) {
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 4;
		d->items[a].data = menu_font_str;
		d->items[a].fn = check_number;
		d->items[a].gid = 1;
		d->items[a].gnum = MAX_FONT_SIZE;
		a++;
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 7;
		d->items[a].data = fg_color_str;
		d->items[a].fn = check_hex_number;
		d->items[a].gid = 0;
		d->items[a].gnum = 0xffffff;
		a++;
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 7;
		d->items[a].data = bg_color_str;
		d->items[a].fn = check_hex_number;
		d->items[a].gid = 0;
		d->items[a].gnum = 0xffffff;
		a++;
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 7;
		d->items[a].data = scroll_area_color_str;
		d->items[a].fn = check_hex_number;
		d->items[a].gid = 0;
		d->items[a].gnum = 0xffffff;
		a++;
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 7;
		d->items[a].data = scroll_bar_color_str;
		d->items[a].fn = check_hex_number;
		d->items[a].gid = 0;
		d->items[a].gnum = 0xffffff;
		a++;
		d->items[a].type = D_FIELD;
		d->items[a].dlen = 7;
		d->items[a].data = scroll_frame_color_str;
		d->items[a].fn = check_hex_number;
		d->items[a].gid = 0;
		d->items[a].gnum = 0xffffff;
		a++;
	}
#endif
	if (!anonymous) {
		d->items[a].type = D_FIELD;
		d->items[a].dlen = MAX_STR_LEN;
		d->items[a].data = new_bookmarks_file;
		a++;
		d->items[a].type = D_BUTTON;
		d->items[a].gid = 0;
		d->items[a].fn = dlg_assume_cp;
		d->items[a].text = TEXT_(T_BOOKMARKS_ENCODING);
		d->items[a].data = (unsigned char *) &new_bookmarks_codepage;
		d->items[a].dlen = sizeof(int);
		a++;
	}
#ifdef G
	if (F && (drv->flags & GD_NEED_CODEPAGE)) {
		d->items[a].type = D_BUTTON;
		d->items[a].gid = 0;
		d->items[a].fn = dlg_kb_cp;
		d->items[a].text = TEXT_(T_KEYBOARD_CODEPAGE);
		d->items[a].data = (unsigned char *) &(drv->codepage);
		d->items[a].dlen = sizeof(int);
		a++;
	}
#endif
	if (!anonymous) {
		d->items[a].type = D_CHECKBOX;
		d->items[a].gid = 0;
		d->items[a].dlen = sizeof(int);
		d->items[a].data = (void *)&save_history;
		a++;
	}
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ENTER;
	d->items[a].fn = ok_dialog;
	d->items[a].text = TEXT_(T_OK);
	a++;
	d->items[a].type = D_BUTTON;
	d->items[a].gid = B_ESC;
	d->items[a].fn = cancel_dialog;
	d->items[a].text = TEXT_(T_CANCEL);
	a++;
	d->items[a].type = D_END;
	do_dialog(term, d, getml(d, NULL));
}

static void menu_set_language(struct terminal *term, void *pcp, struct session *ses)
{
	set_language((int)(my_intptr_t)pcp);
	cls_redraw_all_terminals();
}

static void menu_language_list(struct terminal *term, void *xxx, struct session *ses)
{
	int i, sel;
	unsigned char *n;
	struct menu_item *mi;
	mi = new_menu(1);
	for (i = 0; i < n_languages(); i++) {
		n = language_name(i);
		add_to_menu(&mi, n, cast_uchar "", cast_uchar "", MENU_FUNC menu_set_language, (void *)(my_intptr_t)i, 0, i);
	}
	sel = current_language;
	do_menu_selected(term, mi, ses, sel, NULL, NULL);
}

static unsigned char *resize_texts[] = { TEXT_(T_COLUMNS), TEXT_(T_ROWS) };

static unsigned char x_str[4];
static unsigned char y_str[4];

static void do_resize_terminal(struct terminal *term)
{
	unsigned char str[8];
	strcpy(cast_char str, cast_const_char x_str);
	strcat(cast_char str, ",");
	strcat(cast_char str, cast_const_char y_str);
	do_terminal_function(term, TERM_FN_RESIZE, str);
}

static void dlg_resize_terminal(struct terminal *term, void *xxx, struct session *ses)
{
	struct dialog *d;
	unsigned x = (unsigned)term->x > 999 ? 999 : term->x;
	unsigned y = (unsigned)term->y > 999 ? 999 : term->y;
	sprintf(cast_char x_str, "%u", x);
	sprintf(cast_char y_str, "%u", y);
	d = mem_calloc(sizeof(struct dialog) + 4 * sizeof(struct dialog_item));
	d->title = TEXT_(T_RESIZE_TERMINAL);
	d->fn = group_fn;
	d->udata = resize_texts;
	d->refresh = (void (*)(void *))do_resize_terminal;
	d->refresh_data = term;
	d->items[0].type = D_FIELD;
	d->items[0].dlen = 4;
	d->items[0].data = x_str;
	d->items[0].fn = check_number;
	d->items[0].gid = 1;
	d->items[0].gnum = 999;
	d->items[1].type = D_FIELD;
	d->items[1].dlen = 4;
	d->items[1].data = y_str;
	d->items[1].fn = check_number;
	d->items[1].gid = 1;
	d->items[1].gnum = 999;
	d->items[2].type = D_BUTTON;
	d->items[2].gid = B_ENTER;
	d->items[2].fn = ok_dialog;
	d->items[2].text = TEXT_(T_OK);
	d->items[3].type = D_BUTTON;
	d->items[3].gid = B_ESC;
	d->items[3].fn = cancel_dialog;
	d->items[3].text = TEXT_(T_CANCEL);
	d->items[4].type = D_END;
	do_dialog(term, d, getml(d, NULL));

}

static struct menu_item file_menu11[] = {
	{ TEXT_(T_GOTO_URL), cast_uchar "g", TEXT_(T_HK_GOTO_URL), MENU_FUNC menu_goto_url, (void *)0, 0, 0 },
	{ TEXT_(T_GO_BACK), cast_uchar "z", TEXT_(T_HK_GO_BACK), MENU_FUNC menu_go_back, (void *)0, 0, 0 },
	{ TEXT_(T_GO_FORWARD), cast_uchar "x", TEXT_(T_HK_GO_FORWARD), MENU_FUNC menu_go_forward, (void *)0, 0, 0 },
	{ TEXT_(T_HISTORY), cast_uchar ">", TEXT_(T_HK_HISTORY), MENU_FUNC history_menu, (void *)0, 1, 0 },
	{ TEXT_(T_RELOAD), cast_uchar "Ctrl-R", TEXT_(T_HK_RELOAD), MENU_FUNC menu_reload, (void *)0, 0, 0 },
};

#ifdef G
static struct menu_item file_menu111[] = {
	{ TEXT_(T_GOTO_URL), cast_uchar "g", TEXT_(T_HK_GOTO_URL), MENU_FUNC menu_goto_url, (void *)0, 0, 0 },
	{ TEXT_(T_GO_BACK), cast_uchar "z", TEXT_(T_HK_GO_BACK), MENU_FUNC menu_go_back, (void *)0, 0, 0 },
	{ TEXT_(T_GO_FORWARD), cast_uchar "x", TEXT_(T_HK_GO_FORWARD), MENU_FUNC menu_go_forward, (void *)0, 0, 0 },
	{ TEXT_(T_HISTORY), cast_uchar ">", TEXT_(T_HK_HISTORY), MENU_FUNC history_menu, (void *)0, 1, 0 },
	{ TEXT_(T_RELOAD), cast_uchar "Ctrl-R", TEXT_(T_HK_RELOAD), MENU_FUNC menu_reload, (void *)0, 0, 0 },
};
#endif

static struct menu_item file_menu12[] = {
	{ TEXT_(T_BOOKMARKS), cast_uchar "s", TEXT_(T_HK_BOOKMARKS), MENU_FUNC menu_bookmark_manager, (void *)0, 0, 0 },
};

static struct menu_item file_menu21[] = {
	{ cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT_(T_SAVE_AS), cast_uchar "", TEXT_(T_HK_SAVE_AS), MENU_FUNC save_as, (void *)0, 0, 0 },
	{ TEXT_(T_SAVE_URL_AS), cast_uchar "", TEXT_(T_HK_SAVE_URL_AS), MENU_FUNC menu_save_url_as, (void *)0, 0, 0 },
	{ TEXT_(T_SAVE_FORMATTED_DOCUMENT), cast_uchar "", TEXT_(T_HK_SAVE_FORMATTED_DOCUMENT), MENU_FUNC menu_save_formatted, (void *)0, 0, 0 },
};

#ifdef G
static struct menu_item file_menu211[] = {
	{ cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT_(T_SAVE_AS), cast_uchar "", TEXT_(T_HK_SAVE_AS), MENU_FUNC save_as, (void *)0, 0, 0 },
	{ TEXT_(T_SAVE_URL_AS), cast_uchar "", TEXT_(T_HK_SAVE_URL_AS), MENU_FUNC menu_save_url_as, (void *)0, 0, 0 },
};
#endif

#ifdef G
static struct menu_item file_menu211_clipb[] = {
	{ cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT_(T_SAVE_AS), cast_uchar "", TEXT_(T_HK_SAVE_AS), MENU_FUNC save_as, (void *)0, 0, 0 },
	{ TEXT_(T_SAVE_URL_AS), cast_uchar "", TEXT_(T_HK_SAVE_URL_AS), MENU_FUNC menu_save_url_as, (void *)0, 0, 0 },
	{ TEXT_(T_COPY_URL_LOCATION), cast_uchar "", TEXT_(T_HK_COPY_URL_LOCATION), MENU_FUNC copy_url_location, (void *)0, 0, 0 },
};
#endif

static struct menu_item file_menu22[] = {
	{ cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0} ,
	{ TEXT_(T_KILL_BACKGROUND_CONNECTIONS), cast_uchar "", TEXT_(T_HK_KILL_BACKGROUND_CONNECTIONS), MENU_FUNC menu_kill_background_connections, (void *)0, 0, 0 },
	{ TEXT_(T_KILL_ALL_CONNECTIONS), cast_uchar "", TEXT_(T_HK_KILL_ALL_CONNECTIONS), MENU_FUNC menu_kill_all_connections, (void *)0, 0, 0 },
	{ TEXT_(T_FLUSH_ALL_CACHES), cast_uchar "", TEXT_(T_HK_FLUSH_ALL_CACHES), MENU_FUNC flush_caches, (void *)0, 0, 0 },
	{ TEXT_(T_RESOURCE_INFO), cast_uchar "", TEXT_(T_HK_RESOURCE_INFO), MENU_FUNC resource_info_menu, (void *)0, 0, 0 },
#ifdef LEAK_DEBUG
	{ TEXT_(T_MEMORY_INFO), cast_uchar "", TEXT_(T_HK_MEMORY_INFO), MENU_FUNC memory_info_menu, (void *)0, 0, 0 },
#endif
	{ cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
};

static struct menu_item file_menu3[] = {
	{ cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT_(T_EXIT), cast_uchar "q", TEXT_(T_HK_EXIT), MENU_FUNC exit_prog, (void *)0, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

static void do_file_menu(struct terminal *term, void *xxx, struct session *ses)
{
	int x;
	int o;
	struct menu_item *file_menu, *e, *f;
	file_menu = mem_alloc(sizeof(file_menu11) + sizeof(file_menu12) + sizeof(file_menu21) + sizeof(file_menu22) + sizeof(file_menu3) + 3 * sizeof(struct menu_item));
	e = file_menu;
	if (!F) {
		memcpy(e, file_menu11, sizeof(file_menu11));
		e += sizeof(file_menu11) / sizeof(struct menu_item);
#ifdef G
	} else {
		memcpy(e, file_menu111, sizeof(file_menu111));
		e += sizeof(file_menu111) / sizeof(struct menu_item);
#endif
	}
	if (!anonymous) {
		memcpy(e, file_menu12, sizeof(file_menu12));
		e += sizeof(file_menu12) / sizeof(struct menu_item);
	}
	if ((o = can_open_in_new(term))) {
		e->text = TEXT_(T_NEW_WINDOW);
		e->rtext = o - 1 ? cast_uchar ">" : cast_uchar "";
		e->hotkey = TEXT_(T_HK_NEW_WINDOW);
		e->func = MENU_FUNC open_in_new_window;
		e->data = (void *)send_open_new_xterm;
		e->in_m = o - 1;
		e->free_i = 0;
		e++;
	}
	if (!anonymous) {
		if (!F) {
			memcpy(e, file_menu21, sizeof(file_menu21));
			e += sizeof(file_menu21) / sizeof(struct menu_item);
#ifdef G
		} else {
			if (clipboard_support(term))
			{
				memcpy(e, file_menu211_clipb, sizeof(file_menu211_clipb));
				e += sizeof(file_menu211_clipb) / sizeof(struct menu_item);
			}
			else
			{
				memcpy(e, file_menu211, sizeof(file_menu211));
				e += sizeof(file_menu211) / sizeof(struct menu_item);
			}
#endif
		}
	}
	memcpy(e, file_menu22, sizeof(file_menu22));
	e += sizeof(file_menu22) / sizeof(struct menu_item);
	/*cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0,
	TEXT_(T_OS_SHELL), cast_uchar "", TEXT_(T_HK_OS_SHELL), MENU_FUNC menu_shell, NULL, 0, 0,*/
	x = 1;
	if (!anonymous && can_open_os_shell(term->environment)) {
		e->text = TEXT_(T_OS_SHELL);
		e->rtext = cast_uchar "";
		e->hotkey = TEXT_(T_HK_OS_SHELL);
		e->func = MENU_FUNC menu_shell;
		e->data = NULL;
		e->in_m = 0;
		e->free_i = 0;
		e++;
		x = 0;
	}
	if (can_resize_window(term)) {
		e->text = TEXT_(T_RESIZE_TERMINAL);
		e->rtext = cast_uchar "";
		e->hotkey = TEXT_(T_HK_RESIZE_TERMINAL);
		e->func = MENU_FUNC dlg_resize_terminal;
		e->data = NULL;
		e->in_m = 0;
		e->free_i = 0;
		e++;
		x = 0;
	}
	memcpy(e, file_menu3 + x, sizeof(file_menu3) - x * sizeof(struct menu_item));
	e += sizeof(file_menu3) / sizeof(struct menu_item);
	for (f = file_menu; f < e; f++) f->free_i = 1;
	do_menu(term, file_menu, ses);
}

static struct menu_item view_menu[] = {
	{ TEXT_(T_SEARCH), cast_uchar "/", TEXT_(T_HK_SEARCH), MENU_FUNC menu_for_frame, (void *)search_dlg, 0, 0 },
	{ TEXT_(T_SEARCH_BACK), cast_uchar "?", TEXT_(T_HK_SEARCH_BACK), MENU_FUNC menu_for_frame, (void *)search_back_dlg, 0, 0 },
	{ TEXT_(T_FIND_NEXT), cast_uchar "n", TEXT_(T_HK_FIND_NEXT), MENU_FUNC menu_for_frame, (void *)find_next, 0, 0 },
	{ TEXT_(T_FIND_PREVIOUS), cast_uchar "N", TEXT_(T_HK_FIND_PREVIOUS), MENU_FUNC menu_for_frame, (void *)find_next_back, 0, 0 },
	{ cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT_(T_TOGGLE_HTML_PLAIN), cast_uchar "\\", TEXT_(T_HK_TOGGLE_HTML_PLAIN), MENU_FUNC menu_toggle, NULL, 0, 0 },
	{ TEXT_(T_DOCUMENT_INFO), cast_uchar "=", TEXT_(T_HK_DOCUMENT_INFO), MENU_FUNC menu_doc_info, NULL, 0, 0 },
	{ TEXT_(T_HEADER_INFO), cast_uchar "|", TEXT_(T_HK_HEADER_INFO), MENU_FUNC menu_head_info, NULL, 0, 0 },
	{ TEXT_(T_FRAME_AT_FULL_SCREEN), cast_uchar "f", TEXT_(T_HK_FRAME_AT_FULL_SCREEN), MENU_FUNC menu_for_frame, (void *)set_frame, 0, 0 },
	{ cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT_(T_HTML_OPTIONS), cast_uchar "", TEXT_(T_HK_HTML_OPTIONS), MENU_FUNC menu_html_options, (void *)0, 0, 0 },
	{ TEXT_(T_SAVE_HTML_OPTIONS), cast_uchar "", TEXT_(T_HK_SAVE_HTML_OPTIONS), MENU_FUNC menu_save_html_options, (void *)0, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

static struct menu_item view_menu_anon[] = {
	{ TEXT_(T_SEARCH), cast_uchar "/", TEXT_(T_HK_SEARCH), MENU_FUNC menu_for_frame, (void *)search_dlg, 0, 0 },
	{ TEXT_(T_SEARCH_BACK), cast_uchar "?", TEXT_(T_HK_SEARCH_BACK), MENU_FUNC menu_for_frame, (void *)search_back_dlg, 0, 0 },
	{ TEXT_(T_FIND_NEXT), cast_uchar "n", TEXT_(T_HK_FIND_NEXT), MENU_FUNC menu_for_frame, (void *)find_next, 0, 0 },
	{ TEXT_(T_FIND_PREVIOUS), cast_uchar "N", TEXT_(T_HK_FIND_PREVIOUS), MENU_FUNC menu_for_frame, (void *)find_next_back, 0, 0 },
	{ cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT_(T_TOGGLE_HTML_PLAIN), cast_uchar "\\", TEXT_(T_HK_TOGGLE_HTML_PLAIN), MENU_FUNC menu_toggle, NULL, 0, 0 },
	{ TEXT_(T_DOCUMENT_INFO), cast_uchar "=", TEXT_(T_HK_DOCUMENT_INFO), MENU_FUNC menu_doc_info, NULL, 0, 0 },
	{ TEXT_(T_FRAME_AT_FULL_SCREEN), cast_uchar "f", TEXT_(T_HK_FRAME_AT_FULL_SCREEN), MENU_FUNC menu_for_frame, (void *)set_frame, 0, 0 },
	{ cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT_(T_HTML_OPTIONS), cast_uchar "", TEXT_(T_HK_HTML_OPTIONS), MENU_FUNC menu_html_options, (void *)0, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

static struct menu_item view_menu_color[] = {
	{ TEXT_(T_SEARCH), cast_uchar "/", TEXT_(T_HK_SEARCH), MENU_FUNC menu_for_frame, (void *)search_dlg, 0, 0 },
	{ TEXT_(T_SEARCH_BACK), cast_uchar "?", TEXT_(T_HK_SEARCH_BACK), MENU_FUNC menu_for_frame, (void *)search_back_dlg, 0, 0 },
	{ TEXT_(T_FIND_NEXT), cast_uchar "n", TEXT_(T_HK_FIND_NEXT), MENU_FUNC menu_for_frame, (void *)find_next, 0, 0 },
	{ TEXT_(T_FIND_PREVIOUS), cast_uchar "N", TEXT_(T_HK_FIND_PREVIOUS), MENU_FUNC menu_for_frame, (void *)find_next_back, 0, 0 },
	{ cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT_(T_TOGGLE_HTML_PLAIN), cast_uchar "\\", TEXT_(T_HK_TOGGLE_HTML_PLAIN), MENU_FUNC menu_toggle, NULL, 0, 0 },
	{ TEXT_(T_DOCUMENT_INFO), cast_uchar "=", TEXT_(T_HK_DOCUMENT_INFO), MENU_FUNC menu_doc_info, NULL, 0, 0 },
	{ TEXT_(T_HEADER_INFO), cast_uchar "|", TEXT_(T_HK_HEADER_INFO), MENU_FUNC menu_head_info, NULL, 0, 0 },
	{ TEXT_(T_FRAME_AT_FULL_SCREEN), cast_uchar "f", TEXT_(T_HK_FRAME_AT_FULL_SCREEN), MENU_FUNC menu_for_frame, (void *)set_frame, 0, 0 },
	{ cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT_(T_HTML_OPTIONS), cast_uchar "", TEXT_(T_HK_HTML_OPTIONS), MENU_FUNC menu_html_options, (void *)0, 0, 0 },
	{ TEXT_(T_COLOR), cast_uchar "", TEXT_(T_HK_COLOR), MENU_FUNC menu_color, (void *)0, 0, 0 },
	{ TEXT_(T_SAVE_HTML_OPTIONS), cast_uchar "", TEXT_(T_HK_SAVE_HTML_OPTIONS), MENU_FUNC menu_save_html_options, (void *)0, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

static struct menu_item view_menu_anon_color[] = {
	{ TEXT_(T_SEARCH), cast_uchar "/", TEXT_(T_HK_SEARCH), MENU_FUNC menu_for_frame, (void *)search_dlg, 0, 0 },
	{ TEXT_(T_SEARCH_BACK), cast_uchar "?", TEXT_(T_HK_SEARCH_BACK), MENU_FUNC menu_for_frame, (void *)search_back_dlg, 0, 0 },
	{ TEXT_(T_FIND_NEXT), cast_uchar "n", TEXT_(T_HK_FIND_NEXT), MENU_FUNC menu_for_frame, (void *)find_next, 0, 0 },
	{ TEXT_(T_FIND_PREVIOUS), cast_uchar "N", TEXT_(T_HK_FIND_PREVIOUS), MENU_FUNC menu_for_frame, (void *)find_next_back, 0, 0 },
	{ cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT_(T_TOGGLE_HTML_PLAIN), cast_uchar "\\", TEXT_(T_HK_TOGGLE_HTML_PLAIN), MENU_FUNC menu_toggle, NULL, 0, 0 },
	{ TEXT_(T_DOCUMENT_INFO), cast_uchar "=", TEXT_(T_HK_DOCUMENT_INFO), MENU_FUNC menu_doc_info, NULL, 0, 0 },
	{ TEXT_(T_FRAME_AT_FULL_SCREEN), cast_uchar "f", TEXT_(T_HK_FRAME_AT_FULL_SCREEN), MENU_FUNC menu_for_frame, (void *)set_frame, 0, 0 },
	{ cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT_(T_HTML_OPTIONS), cast_uchar "", TEXT_(T_HK_HTML_OPTIONS), MENU_FUNC menu_html_options, (void *)0, 0, 0 },
	{ TEXT_(T_COLOR), cast_uchar "", TEXT_(T_HK_COLOR), MENU_FUNC menu_color, (void *)0, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

static struct menu_item help_menu[] = {
	{ TEXT_(T_ABOUT), cast_uchar "", TEXT_(T_HK_ABOUT), MENU_FUNC menu_about, (void *)0, 0, 0 },
	{ TEXT_(T_KEYS), cast_uchar "F1", TEXT_(T_HK_KEYS), MENU_FUNC menu_keys, (void *)0, 0, 0 },
	{ TEXT_(T_MANUAL), cast_uchar "", TEXT_(T_HK_MANUAL), MENU_FUNC menu_manual, (void *)0, 0, 0 },
	{ TEXT_(T_HOMEPAGE), cast_uchar "", TEXT_(T_HK_HOMEPAGE), MENU_FUNC menu_homepage, (void *)0, 0, 0 },
	{ TEXT_(T_COPYING), cast_uchar "", TEXT_(T_HK_COPYING), MENU_FUNC menu_copying, (void *)0, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

#ifdef G
static struct menu_item help_menu_g[] = {
	{ TEXT_(T_ABOUT), cast_uchar "", TEXT_(T_HK_ABOUT), MENU_FUNC menu_about, (void *)0, 0, 0 },
	{ TEXT_(T_KEYS), cast_uchar "F1", TEXT_(T_HK_KEYS), MENU_FUNC menu_keys, (void *)0, 0, 0 },
	{ TEXT_(T_MANUAL), cast_uchar "", TEXT_(T_HK_MANUAL), MENU_FUNC menu_manual, (void *)0, 0, 0 },
	{ TEXT_(T_HOMEPAGE), cast_uchar "", TEXT_(T_HK_HOMEPAGE), MENU_FUNC menu_homepage, (void *)0, 0, 0 },
	{ TEXT_(T_CALIBRATION), cast_uchar "", TEXT_(T_HK_CALIBRATION), MENU_FUNC menu_calibration, (void *)0, 0, 0 },
	{ TEXT_(T_COPYING), cast_uchar "", TEXT_(T_HK_COPYING), MENU_FUNC menu_copying, (void *)0, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};
#endif

static struct menu_item setup_menu[] = {
	{ TEXT_(T_LANGUAGE), cast_uchar ">", TEXT_(T_HK_LANGUAGE), MENU_FUNC menu_language_list, NULL, 1, 0 },
	{ TEXT_(T_CHARACTER_SET), cast_uchar ">", TEXT_(T_HK_CHARACTER_SET), MENU_FUNC charset_list, (void *)1, 1, 0 },
	{ TEXT_(T_TERMINAL_OPTIONS), cast_uchar "", TEXT_(T_HK_TERMINAL_OPTIONS), MENU_FUNC terminal_options, NULL, 0, 0 },
	{ TEXT_(T_NETWORK_OPTIONS), cast_uchar "", TEXT_(T_HK_NETWORK_OPTIONS), MENU_FUNC net_options, NULL, 0, 0 },
#ifdef JS
	{ TEXT_(T_JAVASCRIPT_OPTIONS), cast_uchar "", TEXT_(T_HK_JAVASCRIPT_OPTIONS), MENU_FUNC javascript_options, NULL, 0, 0 },
#endif
	{ TEXT_(T_MISCELANEOUS_OPTIONS), cast_uchar "", TEXT_(T_HK_MISCELANEOUS_OPTIONS), MENU_FUNC miscelaneous_options, NULL, 0, 0 },
	{ TEXT_(T_CACHE), cast_uchar "", TEXT_(T_HK_CACHE), MENU_FUNC cache_opt, NULL, 0, 0 },
	{ TEXT_(T_MAIL_AND_TELNEL), cast_uchar "", TEXT_(T_HK_MAIL_AND_TELNEL), MENU_FUNC net_programs, NULL, 0, 0 },
	{ TEXT_(T_ASSOCIATIONS), cast_uchar "", TEXT_(T_HK_ASSOCIATIONS), MENU_FUNC menu_assoc_manager, NULL, 0, 0 },
	{ TEXT_(T_FILE_EXTENSIONS), cast_uchar "", TEXT_(T_HK_FILE_EXTENSIONS), MENU_FUNC menu_ext_manager, NULL, 0, 0 },
	{ TEXT_(T_BLOCK_LIST), cast_uchar "", TEXT_(T_HK_BLOCK_LIST), MENU_FUNC block_manager, NULL, 0, 0 },
	{ cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT_(T_SAVE_OPTIONS), cast_uchar "", TEXT_(T_HK_SAVE_OPTIONS), MENU_FUNC write_config, NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

static struct menu_item setup_menu_anon[] = {
	{ TEXT_(T_LANGUAGE), cast_uchar ">", TEXT_(T_HK_LANGUAGE), MENU_FUNC menu_language_list, NULL, 1, 0 },
	{ TEXT_(T_CHARACTER_SET), cast_uchar ">", TEXT_(T_HK_CHARACTER_SET), MENU_FUNC charset_list, (void *)1, 1, 0 },
	{ TEXT_(T_TERMINAL_OPTIONS), cast_uchar "", TEXT_(T_HK_TERMINAL_OPTIONS), MENU_FUNC terminal_options, NULL, 0, 0 },
#ifdef JS
	{ TEXT_(T_JAVASCRIPT_OPTIONS), cast_uchar "", TEXT_(T_HK_JAVASCRIPT_OPTIONS), MENU_FUNC javascript_options, NULL, 0, 0 },
#endif
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

#ifdef G

static struct menu_item setup_menu_g[] = {
	{ TEXT_(T_LANGUAGE), cast_uchar ">", TEXT_(T_HK_LANGUAGE), MENU_FUNC menu_language_list, NULL, 1, 0 },
	{ TEXT_(T_VIDEO_OPTIONS), cast_uchar "", TEXT_(T_HK_VIDEO_OPTIONS), MENU_FUNC video_options, NULL, 0, 0 },
	{ TEXT_(T_NETWORK_OPTIONS), cast_uchar "", TEXT_(T_HK_NETWORK_OPTIONS), MENU_FUNC net_options, NULL, 0, 0 },
#ifdef JS
	{ TEXT_(T_JAVASCRIPT_OPTIONS), cast_uchar "", TEXT_(T_HK_JAVASCRIPT_OPTIONS), MENU_FUNC javascript_options, NULL, 0, 0 },
#endif
	{ TEXT_(T_MISCELANEOUS_OPTIONS), cast_uchar "", TEXT_(T_HK_MISCELANEOUS_OPTIONS), MENU_FUNC miscelaneous_options, NULL, 0, 0 },
	{ TEXT_(T_CACHE), cast_uchar "", TEXT_(T_HK_CACHE), MENU_FUNC cache_opt, NULL, 0, 0 },
	{ TEXT_(T_MAIL_TELNET_AND_SHELL), cast_uchar "", TEXT_(T_HK_MAIL_AND_TELNEL), MENU_FUNC net_programs, NULL, 0, 0 },
	{ TEXT_(T_ASSOCIATIONS), cast_uchar "", TEXT_(T_HK_ASSOCIATIONS), MENU_FUNC menu_assoc_manager, NULL, 0, 0 },
	{ TEXT_(T_FILE_EXTENSIONS), cast_uchar "", TEXT_(T_HK_FILE_EXTENSIONS), MENU_FUNC menu_ext_manager, NULL, 0, 0 },
	{ TEXT_(T_BLOCK_LIST), cast_uchar "", TEXT_(T_HK_BLOCK_LIST), MENU_FUNC block_manager, NULL, 0, 0 },
	{ cast_uchar "", cast_uchar "", M_BAR, NULL, NULL, 0, 0 },
	{ TEXT_(T_SAVE_OPTIONS), cast_uchar "", TEXT_(T_HK_SAVE_OPTIONS), MENU_FUNC write_config, NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

static struct menu_item setup_menu_anon_g[] = {
	{ TEXT_(T_LANGUAGE), cast_uchar ">", TEXT_(T_HK_LANGUAGE), MENU_FUNC menu_language_list, NULL, 1, 0 },
	{ TEXT_(T_VIDEO_OPTIONS), cast_uchar "", TEXT_(T_HK_VIDEO_OPTIONS), MENU_FUNC video_options, NULL, 0, 0 },
#ifdef JS
	{ TEXT_(T_JAVASCRIPT_OPTIONS), cast_uchar "", TEXT_(T_HK_JAVASCRIPT_OPTIONS), MENU_FUNC javascript_options, NULL, 0, 0 },
#endif
	{ TEXT_(T_MISCELANEOUS_OPTIONS), cast_uchar "", TEXT_(T_HK_MISCELANEOUS_OPTIONS), MENU_FUNC miscelaneous_options, NULL, 0, 0 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

#endif

static void do_view_menu(struct terminal *term, void *xxx, struct session *ses)
{
	if (F || term->spec->col) {
		if (!anonymous) do_menu(term, view_menu_color, ses);
		else do_menu(term, view_menu_anon_color, ses);
	} else {
		if (!anonymous) do_menu(term, view_menu, ses);
		else do_menu(term, view_menu_anon, ses);
	}
}

static void do_setup_menu(struct terminal *term, void *xxx, struct session *ses)
{
#ifdef G
	if (F) {
		if (!anonymous) do_menu(term, setup_menu_g, ses);
		else do_menu(term, setup_menu_anon_g, ses);
	} else
#endif
	{
		if (!anonymous) do_menu(term, setup_menu, ses);
		else do_menu(term, setup_menu_anon, ses);
	}
}

static struct menu_item main_menu[] = {
	{ TEXT_(T_FILE), cast_uchar "", TEXT_(T_HK_FILE), MENU_FUNC do_file_menu, NULL, 1, 1 },
	{ TEXT_(T_VIEW), cast_uchar "", TEXT_(T_HK_VIEW), MENU_FUNC do_view_menu, NULL, 1, 1 },
	{ TEXT_(T_LINK), cast_uchar "", TEXT_(T_HK_LINK), MENU_FUNC link_menu, NULL, 1, 1 },
	{ TEXT_(T_DOWNLOADS), cast_uchar "", TEXT_(T_HK_DOWNLOADS), MENU_FUNC downloads_menu, NULL, 1, 1 },
	{ TEXT_(T_SETUP), cast_uchar "", TEXT_(T_HK_SETUP), MENU_FUNC do_setup_menu, NULL, 1, 1 },
	{ TEXT_(T_HELP), cast_uchar "", TEXT_(T_HK_HELP), MENU_FUNC do_menu, help_menu, 1, 1 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};

#ifdef G
static struct menu_item main_menu_g[] = {
	{ TEXT_(T_FILE), cast_uchar "", TEXT_(T_HK_FILE), MENU_FUNC do_file_menu, NULL, 1, 1 },
	{ TEXT_(T_VIEW), cast_uchar "", TEXT_(T_HK_VIEW), MENU_FUNC do_view_menu, NULL, 1, 1 },
	{ TEXT_(T_LINK), cast_uchar "", TEXT_(T_HK_LINK), MENU_FUNC link_menu, NULL, 1, 1 },
	{ TEXT_(T_DOWNLOADS), cast_uchar "", TEXT_(T_HK_DOWNLOADS), MENU_FUNC downloads_menu, NULL, 1, 1 },
	{ TEXT_(T_SETUP), cast_uchar "", TEXT_(T_HK_SETUP), MENU_FUNC do_setup_menu, NULL, 1, 1 },
	{ TEXT_(T_HELP), cast_uchar "", TEXT_(T_HK_HELP), MENU_FUNC do_menu, help_menu_g, 1, 1 },
	{ NULL, NULL, 0, NULL, NULL, 0, 0 }
};
#endif

/* lame technology rulez ! */

void activate_bfu_technology(struct session *ses, int item)
{
	struct terminal *term = ses->term;
	do_mainmenu(term, gf_val(main_menu, main_menu_g), ses, item);
}

struct history goto_url_history = { 0, { &goto_url_history.items, &goto_url_history.items } };

void dialog_goto_url(struct session *ses, unsigned char *url)
{
	input_field(ses->term, NULL, TEXT_(T_GOTO_URL), TEXT_(T_ENTER_URL), ses, &goto_url_history, MAX_INPUT_URL_LEN, url, 0, 0, NULL, TEXT_(T_OK), (void (*)(void *, unsigned char *)) goto_url, TEXT_(T_CANCEL), NULL, NULL);
}

void dialog_save_url(struct session *ses)
{
	input_field(ses->term, NULL, TEXT_(T_SAVE_URL), TEXT_(T_ENTER_URL), ses, &goto_url_history, MAX_INPUT_URL_LEN, cast_uchar "", 0, 0, NULL, TEXT_(T_OK), (void (*)(void *, unsigned char *)) save_url, TEXT_(T_CANCEL), NULL, NULL);
}


struct does_file_exist_s {
	void (*fn)(void *, unsigned char *, int);
	void (*cancel)(void *);
	int flags;
	struct session *ses;
	unsigned char *file;
	unsigned char *url;
	unsigned char *head;
};

static void does_file_exist_ok(struct does_file_exist_s *h, int mode)
{
	if (h->fn) {
		unsigned char *d = h->file;
		unsigned char *dd;
		for (dd = h->file; *dd; dd++) if (dir_sep(*dd)) d = dd + 1;
		if (d - h->file < MAX_STR_LEN) {
			memcpy(download_dir, h->file, d - h->file);
			download_dir[d - h->file] = 0;
		}
		h->fn(h->ses, h->file, mode);
	}
}


static void does_file_exist_continue(void *data)
{
	does_file_exist_ok(data, DOWNLOAD_CONTINUE);
}

static void does_file_exist_overwrite(void *data)
{
	does_file_exist_ok(data, DOWNLOAD_OVERWRITE);
}

static void does_file_exist_cancel(void *data)
{
	struct does_file_exist_s *h=(struct does_file_exist_s *)data;
	if (h->cancel) h->cancel(h->ses);
}

static void does_file_exist_rename(void *data)
{
	struct does_file_exist_s *h=(struct does_file_exist_s *)data;
	query_file(h->ses, h->url, h->head, (void (*)(struct session *, unsigned char *, int))h->fn, (void (*)(struct session *))h->cancel, h->flags);
}

static void does_file_exist(struct does_file_exist_s *d, unsigned char *file)
{
	unsigned char *f;
	unsigned char *wd;
	struct session *ses = d->ses;
	struct stat st;
	int r;
	struct does_file_exist_s *h;
	unsigned char *msg;
	int file_type = 0;

	h = mem_alloc(sizeof(struct does_file_exist_s));
	h->fn = d->fn;
	h->cancel = d->cancel;
	h->flags = d->flags;
	h->ses = ses;
	h->file = stracpy(file);
	h->url = stracpy(d->url);
	h->head = stracpy(d->head);

	if (!*file) {
		does_file_exist_rename(h);
		goto free_h_ret;
	}

	if (test_abort_downloads_to_file(file, ses->term->cwd, 0)) {
		msg = TEXT_(T_ALREADY_EXISTS_AS_DOWNLOAD);
		goto display_msgbox;
	}
	
	wd = get_cwd();
	set_cwd(ses->term->cwd);
	f = translate_download_file(file);
	EINTRLOOP(r, stat(cast_const_char f, &st));
	mem_free(f);
	if (wd) set_cwd(wd), mem_free(wd);
	if (r) {
		does_file_exist_ok(h, DOWNLOAD_DEFAULT);
free_h_ret:
		if (h->head) mem_free(h->head);
		mem_free(h->file);
		mem_free(h->url);
		mem_free(h);
		return;
	}

	if (!S_ISREG(st.st_mode)) {
		if (S_ISDIR(st.st_mode))
			file_type = 2;
		else
			file_type = 1;
	}

	msg = TEXT_(T_ALREADY_EXISTS);
	display_msgbox:
	if (file_type == 2) {
		msg_box(
			ses->term,
			getml(h, h->file, h->url, h->head, NULL),
			TEXT_(T_FILE_ALREADY_EXISTS),
			AL_CENTER|AL_EXTD_TEXT, 
			TEXT_(T_DIRECTORY), cast_uchar " ", h->file, cast_uchar " ", TEXT_(T_ALREADY_EXISTS), NULL,
			h,
			2,
			TEXT_(T_RENAME), does_file_exist_rename, B_ENTER,
			TEXT_(T_CANCEL), does_file_exist_cancel, B_ESC
		);
	} else if (file_type || h->flags != DOWNLOAD_CONTINUE) {
		msg_box(
			ses->term,
			getml(h, h->file, h->url, h->head, NULL),
			TEXT_(T_FILE_ALREADY_EXISTS),
			AL_CENTER|AL_EXTD_TEXT, 
			TEXT_(T_FILE), cast_uchar " ", h->file, cast_uchar " ", msg, cast_uchar " ", TEXT_(T_DO_YOU_WISH_TO_OVERWRITE), NULL,
			h,
			3,
			TEXT_(T_OVERWRITE), does_file_exist_overwrite, B_ENTER,
			TEXT_(T_RENAME), does_file_exist_rename, 0,
			TEXT_(T_CANCEL), does_file_exist_cancel, B_ESC
		);
	} else {
		msg_box(
			ses->term,
			getml(h, h->file, h->url, h->head, NULL),
			TEXT_(T_FILE_ALREADY_EXISTS),
			AL_CENTER|AL_EXTD_TEXT, 
			TEXT_(T_FILE), cast_uchar " ", h->file, cast_uchar " ", msg, cast_uchar " ", TEXT_(T_DO_YOU_WISH_TO_CONTINUE), NULL,
			h,
			4,
			TEXT_(T_CONTINUE), does_file_exist_continue, B_ENTER,
			TEXT_(T_OVERWRITE), does_file_exist_overwrite, 0,
			TEXT_(T_RENAME), does_file_exist_rename, 0,
			TEXT_(T_CANCEL), does_file_exist_cancel, B_ESC
		);
	}
}


static struct history file_history = { 0, { &file_history.items, &file_history.items } };


static void query_file_cancel(struct does_file_exist_s *d)
{
	if (d->cancel) d->cancel(d->ses);
}


void query_file(struct session *ses, unsigned char *url, unsigned char *head, void (*fn)(struct session *, unsigned char *, int), void (*cancel)(struct session *), int flags)
{
	unsigned char *file, *def;
	int dfl = 0;
	struct does_file_exist_s *h;

	h = mem_alloc(sizeof(struct does_file_exist_s));

	file = get_filename_from_url(url, head, 0);
	def = init_str();
	add_to_str(&def, &dfl, download_dir);
	if (*def && !dir_sep(def[strlen(cast_const_char def) - 1])) add_chr_to_str(&def, &dfl, '/');
	add_to_str(&def, &dfl, file);
	mem_free(file);

	h->fn = (void (*)(void *, unsigned char *, int))fn;
	h->cancel = (void (*)(void *))cancel;
	h->flags = flags;
	h->ses = ses;
	h->file = NULL;
	h->url = stracpy(url);
	h->head = stracpy(head);

	input_field(ses->term, getml(h, h->url, h->head, NULL), TEXT_(T_DOWNLOAD), TEXT_(T_SAVE_TO_FILE), h, &file_history, MAX_INPUT_URL_LEN, def, 0, 0, NULL, TEXT_(T_OK), (void (*)(void *, unsigned char *))does_file_exist, TEXT_(T_CANCEL), (void (*)(void *))query_file_cancel, NULL);
	mem_free(def);
}

static struct history search_history = { 0, { &search_history.items, &search_history.items } };

void search_back_dlg(struct session *ses, struct f_data_c *f, int a)
{
	if (list_empty(ses->history) || !f->f_data || !f->vs) {
		msg_box(ses->term, NULL, TEXT_(T_SEARCH), AL_LEFT, TEXT_(T_YOU_ARE_NOWHERE), NULL, 1, TEXT_(T_OK), NULL, B_ENTER | B_ESC);
		return;
	}
	input_field(ses->term, NULL, TEXT_(T_SEARCH_BACK), TEXT_(T_SEARCH_FOR_TEXT), ses, &search_history, MAX_INPUT_URL_LEN, cast_uchar "", 0, 0, NULL, TEXT_(T_OK), (void (*)(void *, unsigned char *)) search_for_back, TEXT_(T_CANCEL), NULL, NULL);
}

void search_dlg(struct session *ses, struct f_data_c *f, int a)
{
	if (list_empty(ses->history) || !f->f_data || !f->vs) {
		msg_box(ses->term, NULL, TEXT_(T_SEARCH_FOR_TEXT), AL_LEFT, TEXT_(T_YOU_ARE_NOWHERE), NULL, 1, TEXT_(T_OK), NULL, B_ENTER | B_ESC);
		return;
	}
	input_field(ses->term, NULL, TEXT_(T_SEARCH), TEXT_(T_SEARCH_FOR_TEXT), ses, &search_history, MAX_INPUT_URL_LEN, cast_uchar "", 0, 0, NULL, TEXT_(T_OK), (void (*)(void *, unsigned char *)) search_for, TEXT_(T_CANCEL), NULL, NULL);
}

void free_history_lists(void)
{
	free_list(goto_url_history.items);
	free_list(file_history.items);
	free_list(search_history.items);
#ifdef JS
	free_list(js_get_string_history.items);   /* is in jsint.c */
#endif
}


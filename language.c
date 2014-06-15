/* language.c
 * (c) 2002 Mikulas Patocka
 * This file is a part of the Links program, released under GPL.
 */

#include "links.h"

struct translation {
	int code;
	char *name;
};

struct translation_desc {
	struct translation *t;
};

unsigned char dummyarray[T__N_TEXTS];

#include "language.inc"

static unsigned char **translation_array[N_LANGUAGES][N_CODEPAGES];

int current_language;
static int current_lang_charset;

void init_trans(void)
{
	int i, j;
	for (i = 0; i < N_LANGUAGES; i++)
		for (j = 0; j < N_CODEPAGES; j++)
			translation_array[i][j] = NULL;
	current_language = 0;
	current_lang_charset = 0;
}

void shutdown_trans(void)
{
	int i, j, k;
	for (i = 0; i < N_LANGUAGES; i++)
		for (j = 0; j < N_CODEPAGES; j++) if (translation_array[i][j]) {
			for (k = 0; k < T__N_TEXTS; k++) {
				unsigned char *txt = translation_array[i][j][k];
				if (txt &&
				    txt != cast_uchar translations[i].t[k].name &&
				    txt != cast_uchar translations[0].t[k].name)
					mem_free(txt);
			}
			mem_free(translation_array[i][j]);
		}
}

static inline int is_direct_text(unsigned char *text)
{
/* Do not compare to dummyarray directly - thwart some misoptimizations */
	unsigned char * volatile dm = dummyarray;
	return !(text >= dm && text < dm + T__N_TEXTS);
}

unsigned char *get_text_translation(unsigned char *text, struct terminal *term)
{
	unsigned char **current_tra;
	struct conv_table *conv_table;
	unsigned char *trn;
	int charset;
	if (!term) charset = 0;
	else if (term->spec) charset = term->spec->charset;
	else charset = utf8_table;
	if (is_direct_text(text)) return text;
	if ((current_tra = translation_array[current_language][charset])) {
		unsigned char *tt;
		if ((trn = current_tra[text - dummyarray])) return trn;
		tr:
		if (!(tt = cast_uchar translations[current_language].t[text - dummyarray].name)) {
			trn = cast_uchar translation_english[text - dummyarray].name;
		} else {
			struct document_options l_opt;
			memset(&l_opt, 0, sizeof(l_opt));
			l_opt.plain = 0;
			l_opt.cp = charset;
			conv_table = get_translation_table(current_lang_charset, charset);
			trn = convert_string(conv_table, tt, (int)strlen(cast_const_char tt), &l_opt);
			if (!strcmp(cast_const_char trn, cast_const_char tt)) {
				mem_free(trn);
				trn = tt;
			}
		}
		current_tra[text - dummyarray] = trn;
	} else {
		if (current_lang_charset && charset != current_lang_charset) {
			current_tra = translation_array[current_language][charset] = mem_alloc(sizeof (unsigned char **) * T__N_TEXTS);
			memset(current_tra, 0, sizeof (unsigned char **) * T__N_TEXTS);
			goto tr;
		}
		if (!(trn = cast_uchar translations[current_language].t[text - dummyarray].name)) {
			trn = cast_uchar(translations[current_language].t[text - dummyarray].name = translation_english[text - dummyarray].name);	/* modifying translation structure */
		}
	}
	return trn;
}

unsigned char *get_english_translation(unsigned char *text)
{
	if (is_direct_text(text)) return text;
	return cast_uchar translation_english[text - dummyarray].name;
}

int n_languages(void)
{
	return N_LANGUAGES;
}

unsigned char *language_name(int l)
{
	return cast_uchar translations[l].t[T__LANGUAGE].name;
}

void set_language(int l)
{
	int i;
	unsigned char *cp;
	for (i = 0; i < T__N_TEXTS; i++) if (translations[l].t[i].code != i) {
		internal("Bad table for language %s. Run script synclang.", translations[l].t[T__LANGUAGE].name);
		return;
	}
	current_language = l;
	cp = cast_uchar translations[l].t[T__CHAR_SET].name;
	i = get_cp_index(cp);
	if (i == -1) {
		internal("Unknown charset for language %s.", translations[l].t[T__LANGUAGE].name);
		i = 0;
	}
	current_lang_charset = i;
}

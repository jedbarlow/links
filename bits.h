/* bits.h
 * (c) 2002 Karel 'Clock' Kulhavy
 * This file is a part of the Links program, released under GPL.
 */

/* t2c
 * Type that has exactly 2 chars.
 * If there is none, t2c is not defined
 * The type may be signed or unsigned, it doesn't matter
 */
#if SIZEOF_UNSIGNED_SHORT == 2
#define t2c unsigned short
#elif SIZEOF_UNSIGNED == 2
#define t2c unsigned
#elif SIZEOF_UNSIGNED_LONG == 2
#define t2c unsigned long
#elif defined(SIZEOF_UNSIGNED_LONG_LONG) && SIZEOF_UNSIGNED_LONG_LONG == 2
#define t2c unsigned long long
#endif /* #if sizeof(short) */

/* t4c
 * Type that has exactly 4 chars.
 * If there is none, t4c is not defined
 * The type may be signed or unsigned, it doesn't matter
 */
#if SIZEOF_UNSIGNED_SHORT == 4
#define t4c unsigned short
#elif SIZEOF_UNSIGNED == 4
#define t4c unsigned
#elif SIZEOF_UNSIGNED_LONG == 4
#define t4c unsigned long
#elif defined(SIZEOF_UNSIGNED_LONG_LONG) && SIZEOF_UNSIGNED_LONG_LONG == 4
#define t4c unsigned long long
#endif /* #if sizeof(short) */

/* t8c
 * Type that has exactly 8 chars.
 * If there is none, t8c is not defined
 * The type may be signed or unsigned, it doesn't matter
 */
#if SIZEOF_UNSIGNED_SHORT == 8
#define t8c unsigned short
#elif SIZEOF_UNSIGNED == 8
#define t8c unsigned
#elif SIZEOF_UNSIGNED_LONG == 8
#define t8c unsigned long
#elif defined(SIZEOF_UNSIGNED_LONG_LONG) && SIZEOF_UNSIGNED_LONG_LONG == 8
#define t8c unsigned long long
#endif /* #if sizeof(short) */


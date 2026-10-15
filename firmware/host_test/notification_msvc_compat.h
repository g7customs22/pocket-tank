/* Host-only scalar dependency compatibility; not an inference benchmark. */
#if defined(_MSC_VER) && !defined(__clang__)
#define __attribute__(x)
#endif

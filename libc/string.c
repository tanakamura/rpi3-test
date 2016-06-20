#include <string.h>
#include <stdint.h>
#include <arm_neon.h>
#include <stddef.h>

void *
memset(void *p, int c, size_t len)
{
    char *s = p;
    size_t i;
    for (i=0; i<len; i++) {
        s[i] = c;
    }
    return p;
}
void *
memcpy(void *p, const void *src, size_t len)
{
    char *d = p;
    const char *s = src;
    size_t i;

    uintptr_t d_addr = (uintptr_t)d;
    uintptr_t s_addr = (uintptr_t)s;

    if (((d_addr & 15) == 0) &&
        ((s_addr & 15) == 0) &&
        ((len & 15) == 0))
    {
        size_t nvec = len >> 4;
        uint32x4_t *vs = (uint32x4_t*)s;
        uint32x4_t *vd = (uint32x4_t*)d;

        for (i=0; i<nvec; i++) {
            vd[i] = vs[i];
        }
    } else {
        for (i=0; i<len; i++) {
            d[i] = s[i];
        }
    }

    return p;
}

void *
memmove(void *dst, const void *src, size_t len)
{
    ptrdiff_t dptr = dst - src;
    char *d = dst;
    const char *s = src;
    intptr_t i;

    if (dptr > 0) {
        for (i=len-1; i>=0; i--) {
            d[i] = s[i];
        }
    } else {
        uintptr_t d_addr = (uintptr_t)d;
        uintptr_t s_addr = (uintptr_t)s;

        if (((d_addr & 15) == 0) &&
            ((s_addr & 15) == 0) &&
            ((len & 15) == 0) &&
            ((dptr <= -16)))
        {
            size_t nvec = len >> 4;
            uint32x4_t *vs = (uint32x4_t*)s;
            uint32x4_t *vd = (uint32x4_t*)d;

            for (i=0; i<nvec; i++) {
                vd[i] = vs[i];
            }
        } else {
            for (i=0; i<len; i++) {
                d[i] = s[i];
            }
        }
    }

    return dst;
}

char *
strcpy(char *dest, const char *src)
{
    char *ret = dest;
    while (*src)
        *dest++ = *src++;
    return ret;
}

char *
strncpy(char *dest, const char *src, size_t n)
{
    char *ret = dest;
    size_t i;

    for (i=0; i<n; i++) {
        if (src[i] == '\0') break;
        dest[i] = src[i];
    }
    return ret;
}

int
strcmp(const char *s1, const char *s2)
{
    while (1) {
        if (*s1 != *s2)
            return *s1 - *s2;

        if (*s1 == '\0')
            return 0;

        s1++;
        s2++;
    }
}
int
memcmp(const void *s1, const void *s2, size_t n)
{
    const char *p1 = s1;
    const char *p2 = s2;

    size_t i;
    for (i=0; i<n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

char *
strcat(char *dest, const char *src)
{
    size_t dl = strlen(dest);
    size_t sl = strlen(src);

    memcpy(dest + dl, src, sl);

    return dest;
}

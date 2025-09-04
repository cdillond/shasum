#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <byteswap.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define BLOCK_BYTES 64
#define BUF_WORDS 64
#define BUF_MAX (64 - 8)
#define BUF_BYTES 256

union buffer
{
    u8 bytes[256];
    u32 words[64];
};

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"

#define BYTE_TO_BINARY(byte)         \
    ((byte) & 0x80 ? '1' : '0'),     \
        ((byte) & 0x40 ? '1' : '0'), \
        ((byte) & 0x20 ? '1' : '0'), \
        ((byte) & 0x10 ? '1' : '0'), \
        ((byte) & 0x08 ? '1' : '0'), \
        ((byte) & 0x04 ? '1' : '0'), \
        ((byte) & 0x02 ? '1' : '0'), \
        ((byte) & 0x01 ? '1' : '0')

static u32
ch(u32 x, u32 y, u32 z)
{
    return (x & y) ^ ((~x) & z);
}

static u32 maj(u32 x, u32 y, u32 z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

/* n must be < 32 */
static u32 rotl(u32 x, u32 n)
{
    return (x << n) | (x >> ((-n) & 31));
}

static u32 rotr(u32 x, u32 n)
{
    return (x >> n) | (x << (32 - n));
}

static u32 bsig0(u32 x)
{
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

static u32 bsig1(u32 x)
{
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

static u32 ssig0(u32 x)
{
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

static u32 ssig1(u32 x)
{
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

static u32 K[64] = {
    0x428a2f98,
    0x71374491,
    0xb5c0fbcf,
    0xe9b5dba5,
    0x3956c25b,
    0x59f111f1,
    0x923f82a4,
    0xab1c5ed5,
    0xd807aa98,
    0x12835b01,
    0x243185be,
    0x550c7dc3,
    0x72be5d74,
    0x80deb1fe,
    0x9bdc06a7,
    0xc19bf174,
    0xe49b69c1,
    0xefbe4786,
    0x0fc19dc6,
    0x240ca1cc,
    0x2de92c6f,
    0x4a7484aa,
    0x5cb0a9dc,
    0x76f988da,
    0x983e5152,
    0xa831c66d,
    0xb00327c8,
    0xbf597fc7,
    0xc6e00bf3,
    0xd5a79147,
    0x06ca6351,
    0x14292967,
    0x27b70a85,
    0x2e1b2138,
    0x4d2c6dfc,
    0x53380d13,
    0x650a7354,
    0x766a0abb,
    0x81c2c92e,
    0x92722c85,
    0xa2bfe8a1,
    0xa81a664b,
    0xc24b8b70,
    0xc76c51a3,
    0xd192e819,
    0xd6990624,
    0xf40e3585,
    0x106aa070,
    0x19a4c116,
    0x1e376c08,
    0x2748774c,
    0x34b0bcb5,
    0x391c0cb3,
    0x4ed8aa4a,
    0x5b9cca4f,
    0x682e6ff3,
    0x748f82ee,
    0x78a5636f,
    0x84c87814,
    0x8cc70208,
    0x90befffa,
    0xa4506ceb,
    0xbef9a3f7,
    0xc67178f2,
};

void print_word(u32 pre, u32 *W)
{
    u32 be = bswap_32(*W);
    u8 *w = (u8 *)&be;
    printf("%02d  ", pre);

    printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY((*w)));
    printf(" ");

    printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY((w[1])));
    printf(" ");

    printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY((w[2])));
    printf(" ");

    printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY((w[3])));

    printf("\n");
}

void print_bits(u8 w[BUF_BYTES])
{
    u32 i;
    for (i = 0; i < BUF_BYTES; i += 4)
    {
        printf("%02d  ", i / 4);

        printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY((w[i])));
        printf(" ");

        printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY((w[i + 1])));
        printf(" ");

        printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY((w[i + 2])));
        printf(" ");

        printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY((w[i + 3])));

        printf("\n");
    }
}

int digest(u32 arr[8], const u8 *restrict src, u32 len)
{

    u32 i, n, t, t1, t2;
    u32 a, b, c, d, e, f, g, h;
    u32 H[8] = {
        0x6a09e667,
        0xbb67ae85,
        0x3c6ef372,
        0xa54ff53a,
        0x510e527f,
        0x9b05688c,
        0x1f83d9ab,
        0x5be0cd19};

    union buffer buf;
    u8 *ptr;
    u32 done = 0;
    n = 0;

    while (!done)
    {
        ptr = buf.bytes;
        /* copy int buf, assuming it all fits; all message block bits are big endian */
        for (i = 0; i < BLOCK_BYTES && n < len; i++)
            *ptr++ = src[n++];

        if (n == len)
        {
            if ((n & 63) < (64 - 5))
            {
                done = 1;
                *ptr++ = 0x80;
                n++;
                for (; (n & 63) < (BLOCK_BYTES - sizeof(u32)); n++)
                    *ptr++ = 0;

                /* convert back to native endian */
                for (i = 0; i < 15; i++)
                    buf.words[i] = bswap_32(buf.words[i]);

                buf.words[15] = len * 8;
            }
            else
            {
                /* buffer has nearly been filled, but no room for len */
                *ptr++ = 0x80;
                n++;

                for (; (n & 63) != 0; n++)
                    *ptr++ = 0;

                /* convert back to native endian */
                for (i = 0; i < 16; i++)
                    buf.words[i] = bswap_32(buf.words[i]);
            }
        }
        else
        {
            /* buffer has been filled */
            for (i = 0; i < 16; i++)
                buf.words[i] = bswap_32(buf.words[i]);
        }

        for (i = 16; i < BUF_WORDS; i++)
        {
            buf.words[i] =
                ssig1(buf.words[i - 2]) +
                buf.words[i - 7] +
                ssig0(buf.words[i - 15]) +
                buf.words[i - 16];
        }

        a = H[0];
        b = H[1];
        c = H[2];
        d = H[3];
        e = H[4];
        f = H[5];
        g = H[6];
        h = H[7];

        for (t = 0; t < 64; t++)
        {
            t1 = h + bsig1(e) + ch(e, f, g) + K[t] + buf.words[t];
            t2 = bsig0(a) + maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        H[0] += a;
        H[1] += b;
        H[2] += c;
        H[3] += d;
        H[4] += e;
        H[5] += f;
        H[6] += g;
        H[7] += h;
    }

    arr[0] = H[0];
    arr[1] = H[1];
    arr[2] = H[2];
    arr[3] = H[3];
    arr[4] = H[4];
    arr[5] = H[5];
    arr[6] = H[6];
    arr[7] = H[7];

    return 0;
}

int main(int argc, char *argv[])
{
    u32 arr[8];
    u8 *msg = (argc > 1) ? argv[1] : argv[0];

    digest(arr, msg, strlen(msg));

    for (int i = 0; i < 8; i++)
    {
        printf("%08x", arr[i]);
    }
    printf("\n");

    return 0;
}
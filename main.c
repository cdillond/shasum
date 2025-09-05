#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#if (defined(__GNUC__) && !defined(__clang__))
// annoyingly needed to keep GCC from complaining about
// memset-ting only part of the buffer when certain compile
// flags are passed
#pragma GCC diagnostic ignored "-Wmemset-elt-size"
#endif

typedef uint32_t u32;
typedef uint8_t u8;

#define HASH_LEN 8
#define BLOCK_WORDS 16
#define BUF_WORDS 64

// functions described in RFC6234

static u32 ch(u32 x, u32 y, u32 z)
{
    return (x & y) ^ ((~x) & z);
}

static u32 maj(u32 x, u32 y, u32 z)
{
    return (x & y) ^ (x & z) ^ (y & z);
}

static u32 rotr(u32 x, u32 n)
{
    return (x >> n) | (x << ((-n) & 31));
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

void hash(u32 digest[HASH_LEN])
{
    int cur;
    u32 i, shift;
    u32 a, b, c, d, e, f, g, h, t1, t2;
    u32 done = 0;
    u32 len = 0;
    u32 buf[BUF_WORDS];

    while (!done)
    {
        memset(buf, 0, sizeof(*buf) * BLOCK_WORDS);

        shift = 24;
        for (i = 0; i < BLOCK_WORDS; len++)
        {
            cur = getchar();
            if (cur == EOF)
            {
                buf[i] |= 0x80 << shift;
                done = 2;
                if (i < (BLOCK_WORDS - 1))
                    // the full padding can fit in this block
                    buf[15] = len * 8;
                else
                    // only the end bit and maybe some zeros fit
                    done--;

                break;
            }

            buf[i] |= (u32)(cur) << shift;

            if (shift == 0)
            {
                i++;
                shift = 24;
            }
            else
                shift -= 8;
        }

    finish:
        for (i = 16; i < BUF_WORDS; i++)
        {
            buf[i] =
                ssig1(buf[i - 2]) +
                buf[i - 7] +
                ssig0(buf[i - 15]) +
                buf[i - 16];
        }

        // initialize working variables
        a = digest[0];
        b = digest[1];
        c = digest[2];
        d = digest[3];
        e = digest[4];
        f = digest[5];
        g = digest[6];
        h = digest[7];

        for (i = 0; i < BUF_WORDS; i++)
        {
            t1 = h + bsig1(e) + ch(e, f, g) + K[i] + buf[i];
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

        digest[0] += a;
        digest[1] += b;
        digest[2] += c;
        digest[3] += d;
        digest[4] += e;
        digest[5] += f;
        digest[6] += g;
        digest[7] += h;

        if (done == 1)
        {
            // set up the final block
            done = 2;
            memset(buf, 0, sizeof(*buf) * BLOCK_WORDS);
            // the end bit has already been written to the previous block
            buf[15] = len * 8;
            goto finish;
        }
    }
    return;
}

int main(int argc, char *argv[])
{
    u32 digest[HASH_LEN] = {0x6a09e667,
                            0xbb67ae85,
                            0x3c6ef372,
                            0xa54ff53a,
                            0x510e527f,
                            0x9b05688c,
                            0x1f83d9ab,
                            0x5be0cd19};
    char res[(HASH_LEN * 8) + 1];
    int n = 0;
    int i = 0;
    int ok = 0;

    hash(digest);

    for (i = 0; i < HASH_LEN; i++)
        n += snprintf(&res[n], 9, "%08x", digest[i]);

    printf("%s", res);

    if (argc > 1)
    {
        ok = strcmp(argv[1], res);
        if (isatty(1))
            printf(" %s", ok == 0 ? "\033[32mOK\033[0m" : "\033[31mBAD\033[0m");
        else
            printf(" %s", ok == 0 ? "OK" : "BAD");
    }

    printf("\n");

    return ok;
}
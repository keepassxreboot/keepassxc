/* $OpenBSD: bcrypt_pbkdf.c,v 1.13 2015/01/12 03:20:04 tedu Exp $ */
/*
 * Copyright (c) 2013 Ted Unangst <tedu@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <QtCore>

extern "C" {
#include "blf.h"
}

#define	MINIMUM(a,b) (((a) < (b)) ? (a) : (b))

/*
 * pkcs #5 pbkdf2 implementation using the "bcrypt" hash
 *
 * The bcrypt hash function is derived from the bcrypt password hashing
 * function with the following modifications:
 * 1. The input password and salt are preprocessed with SHA512.
 * 2. The output length is expanded to 256 bits.
 * 3. Subsequently the magic string to be encrypted is lengthened and modifed
 *    to "OxychromaticBlowfishSwatDynamite"
 * 4. The hash function is defined to perform 64 rounds of initial state
 *    expansion. (More rounds are performed by iterating the hash.)
 *
 * Note that this implementation pulls the SHA512 operations into the caller
 * as a performance optimization.
 *
 * One modification from official pbkdf2. Instead of outputting key material
 * linearly, we mix it. pbkdf2 has a known weakness where if one uses it to
 * generate (e.g.) 512 bits of key material for use as two 256 bit keys, an
 * attacker can merely run once through the outer loop, but the user
 * always runs it twice. Shuffling output bytes requires computing the
 * entirety of the key material to assemble any subkey. This is something a
 * wise caller could do; we just do it for you.
 */

#define BCRYPT_WORDS 8
#define BCRYPT_HASHSIZE (BCRYPT_WORDS * 4)
#define SHA512_DIGEST_LENGTH 64

// FIXME: explicit_bzero exists to ensure bzero is not optimized out
#define explicit_bzero bzero

static void
bcrypt_hash(const quint8* sha2pass, const quint8* sha2salt, quint8* out)
{
    blf_ctx state;
    quint8 ciphertext[BCRYPT_HASHSIZE] = // "OxychromaticBlowfishSwatDynamite"
        { 0x4f, 0x78, 0x79, 0x63, 0x68, 0x72, 0x6f, 0x6d,
          0x61, 0x74, 0x69, 0x63, 0x42, 0x6c, 0x6f, 0x77,
          0x66, 0x69, 0x73, 0x68, 0x53, 0x77, 0x61, 0x74,
          0x44, 0x79, 0x6e, 0x61, 0x6d, 0x69, 0x74, 0x65 };
    quint32 cdata[BCRYPT_WORDS];
    int i;
    quint16 j;
    size_t shalen = SHA512_DIGEST_LENGTH;

    /* key expansion */
    Blowfish_initstate(&state);
    Blowfish_expandstate(&state, sha2salt, shalen, sha2pass, shalen);
    for (i = 0; i < 64; i++) {
        Blowfish_expand0state(&state, sha2salt, shalen);
        Blowfish_expand0state(&state, sha2pass, shalen);
    }

    /* encryption */
    j = 0;
    for (i = 0; i < BCRYPT_WORDS; i++)
        cdata[i] = Blowfish_stream2word(ciphertext, sizeof(ciphertext),
            &j);
    for (i = 0; i < 64; i++)
        blf_enc(&state, cdata, BCRYPT_WORDS / 2);

    /* copy out */
    for (i = 0; i < BCRYPT_WORDS; i++) {
        out[4 * i + 3] = (cdata[i] >> 24) & 0xff;
        out[4 * i + 2] = (cdata[i] >> 16) & 0xff;
        out[4 * i + 1] = (cdata[i] >> 8) & 0xff;
        out[4 * i + 0] = cdata[i] & 0xff;
    }

    /* zap */
    explicit_bzero(ciphertext, sizeof(ciphertext));
    explicit_bzero(cdata, sizeof(cdata));
    explicit_bzero(&state, sizeof(state));
}

int bcrypt_pbkdf(const QByteArray& pass, const QByteArray& salt, QByteArray& key, quint32 rounds)
{
    QCryptographicHash ctx(QCryptographicHash::Sha512);
    QByteArray sha2pass;
    QByteArray sha2salt;
    quint8 out[BCRYPT_HASHSIZE];
    quint8 tmpout[BCRYPT_HASHSIZE];
    quint8 countsalt[4];

    /* nothing crazy */
    if (rounds < 1) {
        return -1;
    }

    if (pass.isEmpty() || salt.isEmpty() || key.isEmpty() ||
        static_cast<quint32>(key.length()) > sizeof(out) * sizeof(out)) {
        return -1;
    }

    quint32 stride = (key.length() + sizeof(out) - 1) / sizeof(out);
    quint32 amt = (key.length() + stride - 1) / stride;

    /* collapse password */
    ctx.reset();
    ctx.addData(pass);
    sha2pass = ctx.result();

    /* generate key, sizeof(out) at a time */
    for (quint32 count = 1, keylen = key.length(); keylen > 0; count++) {
        countsalt[0] = (count >> 24) & 0xff;
        countsalt[1] = (count >> 16) & 0xff;
        countsalt[2] = (count >> 8) & 0xff;
        countsalt[3] = count & 0xff;

        /* first round, salt is salt */
        ctx.reset();
        ctx.addData(salt);
        ctx.addData(reinterpret_cast<char *>(countsalt), sizeof(countsalt));
        sha2salt = ctx.result();

        bcrypt_hash(reinterpret_cast<quint8 *>(sha2pass.data()), reinterpret_cast<quint8 *>(sha2salt.data()), tmpout);
        memcpy(out, tmpout, sizeof(out));

        for (quint32 i = 1; i < rounds; i++) {
            /* subsequent rounds, salt is previous output */
            ctx.reset();
            ctx.addData(reinterpret_cast<char *>(tmpout), sizeof(tmpout));
            sha2salt = ctx.result();
            bcrypt_hash(reinterpret_cast<quint8 *>(sha2pass.data()), reinterpret_cast<quint8 *>(sha2salt.data()), tmpout);
            for (quint32 j = 0; j < sizeof(out); j++)
                out[j] ^= tmpout[j];
        }

        /*
         * pbkdf2 deviation: output the key material non-linearly.
         */
        amt = MINIMUM(amt, keylen);
        quint32 i;
        for (i = 0; i < amt; i++) {
            int dest = i * stride + (count - 1);
            if (dest >= key.length())
                break;
            key.data()[dest] = out[i];
        }
        keylen -= i;
    }

    /* zap */
    explicit_bzero(out, sizeof(out));

    return 0;
}

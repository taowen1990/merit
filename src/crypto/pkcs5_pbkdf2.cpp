/* OpenBSD: pkcs5_pbkdf2.c, v 1.9 2015/02/05 12:59:57 millert */
/**
 * Copyright (c) 2008 Damien Bergamini <damien.bergamini@free.fr>
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
#include <algorithm>
#include <vector>
#include "pkcs5_pbkdf2.h"
#include "hmac_sha512.h"

int pkcs5_pbkdf2(const std::string& passphrase,
        const std::string& salt,
        uint8_t* key,
        size_t key_length,
        size_t iterations)
{
    const uint8_t *passphrasePtr = reinterpret_cast<const uint8_t*>(passphrase.c_str());
    const uint8_t *saltPtr = reinterpret_cast<const uint8_t*>(salt.c_str());
    const size_t passphrase_length = passphrase.length();
    const size_t salt_length = salt.length();

    size_t asalt_size;
    size_t count, index, iteration, length;
    uint8_t buffer[CHMAC_SHA512::OUTPUT_SIZE];
    uint8_t digest1[CHMAC_SHA512::OUTPUT_SIZE];
    uint8_t digest2[CHMAC_SHA512::OUTPUT_SIZE];

    /* An iteration count of 0 is equivalent to a count of 1. */
    /* A key_length of 0 is a no-op. */
    /* A salt_length of 0 is perfectly valid. */

    if (salt_length > SIZE_MAX - 4)
        return -1;
    asalt_size = salt_length + 4;
    std::vector<uint8_t> asalt(asalt_size);

    std::copy_n(saltPtr, salt_length, asalt.begin());
    for (count = 1; key_length > 0; count++)
    {
        asalt[salt_length + 0] = (count >> 24) & 0xff;
        asalt[salt_length + 1] = (count >> 16) & 0xff;
        asalt[salt_length + 2] = (count >> 8) & 0xff;
        asalt[salt_length + 3] = (count >> 0) & 0xff;
        CHMAC_SHA512(passphrasePtr, passphrase_length).Write(asalt.data(), asalt_size).Finalize(digest1);
        std::copy_n(digest1, sizeof(buffer), buffer);

        for (iteration = 1; iteration < iterations; iteration++)
        {
            CHMAC_SHA512(passphrasePtr, passphrase_length).Write(digest1, sizeof(digest1)).Finalize(digest2);
            std::copy_n(digest2, sizeof(digest1), digest1);
            for (index = 0; index < sizeof(buffer); index++)
                buffer[index] ^= digest1[index];
        }

        length = std::min(key_length, sizeof(buffer));
        std::copy_n(buffer, length, key);
        key += length;
        key_length -= length;
    };

    std::fill_n(digest1, sizeof(digest1), 0);
    std::fill_n(digest2, sizeof(digest2), 0);
    std::fill_n(buffer, sizeof(buffer), 0);
    std::fill_n(asalt.begin(), asalt_size, 0);

    return 0;
}

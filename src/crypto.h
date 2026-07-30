#pragma once
#include <Arduino.h>
#include <mbedtls/gcm.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/md.h>
#include <esp_system.h>

// ============================================================
// crypto.h — AES-256-GCM encryption/decryption + key derivation
//
// Honest security notes (read this):
// - Key is derived from the user's PIN via PBKDF2-HMAC-SHA256
//   with a random per-device salt stored in NVS. A short numeric
//   PIN has low entropy -- this protects against casual file
//   access, NOT against a determined attacker with the flash
//   dumped and unlimited offline guesses. There is no secure
//   element on this board to rate-limit or hardware-bind guesses.
// - AES-256-GCM gives you confidentiality + integrity (tamper
//   detection) for data at rest on the SD card.
// - This is NOT equivalent to a hardware-backed keystore
//   (Secure Enclave / StrongBox / TPM). Anyone with physical
//   access to the flash + enough offline compute against a weak
//   PIN can eventually brute force it. Use a long passphrase,
//   not a 4-digit PIN, if this matters to you.
// ============================================================

#define AES_KEY_LEN     32   // AES-256
#define GCM_IV_LEN      12   // 96-bit nonce, standard for GCM
#define GCM_TAG_LEN     16   // 128-bit auth tag
#define PBKDF2_ITERS    100000
#define SALT_LEN        16

class SecureCrypto {
public:
    // Derive a 256-bit key from a passphrase + salt using PBKDF2-HMAC-SHA256
    static bool deriveKey(const String &passphrase, const uint8_t *salt, size_t saltLen,
                           uint8_t *outKey /* AES_KEY_LEN bytes */) {
        const mbedtls_md_info_t *mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
        if (!mdInfo) return false;

        mbedtls_md_context_t ctx;
        mbedtls_md_init(&ctx);
        if (mbedtls_md_setup(&ctx, mdInfo, 1) != 0) {
            mbedtls_md_free(&ctx);
            return false;
        }

        int rc = mbedtls_pkcs5_pbkdf2_hmac(&ctx,
                                            (const unsigned char *)passphrase.c_str(),
                                            passphrase.length(),
                                            salt, saltLen,
                                            PBKDF2_ITERS,
                                            AES_KEY_LEN, outKey);
        mbedtls_md_free(&ctx);
        return rc == 0;
    }

    // Fill buf with cryptographically-strong random bytes from the ESP32 HWRNG
    static void randomBytes(uint8_t *buf, size_t len) {
        for (size_t i = 0; i < len; i++) {
            buf[i] = (uint8_t)(esp_random() & 0xFF);
        }
    }

    // Encrypt plaintext -> output format: [12-byte IV][ciphertext][16-byte tag]
    static bool encrypt(const uint8_t *key, const uint8_t *plaintext, size_t ptLen,
                         uint8_t *outBuf, size_t &outLen) {
        uint8_t iv[GCM_IV_LEN];
        randomBytes(iv, GCM_IV_LEN);

        mbedtls_gcm_context gcm;
        mbedtls_gcm_init(&gcm);
        if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, AES_KEY_LEN * 8) != 0) {
            mbedtls_gcm_free(&gcm);
            return false;
        }

        uint8_t tag[GCM_TAG_LEN];
        uint8_t *ct = outBuf + GCM_IV_LEN;

        int rc = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, ptLen,
                                            iv, GCM_IV_LEN,
                                            nullptr, 0,        // no additional authenticated data
                                            plaintext, ct,
                                            GCM_TAG_LEN, tag);
        mbedtls_gcm_free(&gcm);
        if (rc != 0) return false;

        memcpy(outBuf, iv, GCM_IV_LEN);
        memcpy(outBuf + GCM_IV_LEN + ptLen, tag, GCM_TAG_LEN);
        outLen = GCM_IV_LEN + ptLen + GCM_TAG_LEN;
        return true;
    }

    // Decrypt buffer produced by encrypt(). Returns false on tamper/auth failure
    // or wrong key -- always check the return value before trusting outPlain.
    static bool decrypt(const uint8_t *key, const uint8_t *inBuf, size_t inLen,
                         uint8_t *outPlain, size_t &outLen) {
        if (inLen < GCM_IV_LEN + GCM_TAG_LEN) return false;

        const uint8_t *iv  = inBuf;
        const uint8_t *ct  = inBuf + GCM_IV_LEN;
        size_t ctLen        = inLen - GCM_IV_LEN - GCM_TAG_LEN;
        const uint8_t *tag = inBuf + GCM_IV_LEN + ctLen;

        mbedtls_gcm_context gcm;
        mbedtls_gcm_init(&gcm);
        if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, AES_KEY_LEN * 8) != 0) {
            mbedtls_gcm_free(&gcm);
            return false;
        }

        int rc = mbedtls_gcm_auth_decrypt(&gcm, ctLen,
                                           iv, GCM_IV_LEN,
                                           nullptr, 0,
                                           tag, GCM_TAG_LEN,
                                           ct, outPlain);
        mbedtls_gcm_free(&gcm);
        if (rc != 0) return false; // auth failure -- tampered or wrong key

        outLen = ctLen;
        return true;
    }

    // Wipe sensitive buffers rather than relying on the compiler not to
    // optimize away a plain memset.
    static void wipe(uint8_t *buf, size_t len) {
        volatile uint8_t *p = buf;
        while (len--) *p++ = 0;
    }
};

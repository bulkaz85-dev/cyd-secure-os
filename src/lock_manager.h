#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "crypto.h"

// ============================================================
// lock_manager.h — passphrase setup/verification + session key
//
// Note on "brute-force protection": we add a software delay that
// grows with failed attempts. This is NOT hardware-enforced (an
// attacker who pulls the flash and inspects it offline isn't
// slowed by this at all -- it only protects the on-device unlock
// UI). Real protection against offline attack comes only from
// passphrase strength.
// ============================================================

class LockManager {
public:
    bool begin() {
        prefs.begin("cydsec", false);
        return true;
    }

    bool isProvisioned() {
        return prefs.isKey("salt") && prefs.isKey("verifier");
    }

    // First-run setup: derive key from passphrase, store salt + a
    // verifier (HMAC-style check value) so we can validate future
    // unlock attempts without ever storing the passphrase or key itself.
    bool provision(const String &passphrase) {
        uint8_t salt[SALT_LEN];
        SecureCrypto::randomBytes(salt, SALT_LEN);

        uint8_t key[AES_KEY_LEN];
        if (!SecureCrypto::deriveKey(passphrase, salt, SALT_LEN, key)) return false;

        // Verifier = encrypt a known constant with the derived key.
        // On unlock we re-derive and check decryption succeeds (GCM auth tag).
        const char *marker = "CYD-SECURE-OS-V1";
        uint8_t outBuf[64];
        size_t outLen;
        if (!SecureCrypto::encrypt(key, (const uint8_t *)marker, strlen(marker), outBuf, outLen)) {
            SecureCrypto::wipe(key, AES_KEY_LEN);
            return false;
        }

        prefs.putBytes("salt", salt, SALT_LEN);
        prefs.putBytes("verifier", outBuf, outLen);
        prefs.putUInt("verLen", outLen);
        prefs.putUInt("failCount", 0);

        memcpy(sessionKey, key, AES_KEY_LEN);
        sessionUnlocked = true;
        SecureCrypto::wipe(key, AES_KEY_LEN);
        return true;
    }

    // Returns true and unlocks the session key on success.
    bool tryUnlock(const String &passphrase) {
        uint32_t fails = prefs.getUInt("failCount", 0);
        if (fails >= 5) {
            uint32_t delayMs = min<uint32_t>(30000, 1000 * (1 << min<uint32_t>(fails - 4, 5)));
            delay(delayMs); // grows: 2s,4s,8s,16s,30s cap -- deters casual UI guessing only
        }

        uint8_t salt[SALT_LEN];
        if (prefs.getBytes("salt", salt, SALT_LEN) != SALT_LEN) return false;

        uint8_t key[AES_KEY_LEN];
        if (!SecureCrypto::deriveKey(passphrase, salt, SALT_LEN, key)) return false;

        uint8_t verifier[64];
        size_t verLen = prefs.getUInt("verLen", 0);
        prefs.getBytes("verifier", verifier, verLen);

        uint8_t plain[64];
        size_t plainLen;
        bool ok = SecureCrypto::decrypt(key, verifier, verLen, plain, plainLen);

        if (ok) {
            memcpy(sessionKey, key, AES_KEY_LEN);
            sessionUnlocked = true;
            prefs.putUInt("failCount", 0);
        } else {
            prefs.putUInt("failCount", fails + 1);
        }
        SecureCrypto::wipe(key, AES_KEY_LEN);
        return ok;
    }

    void lock() {
        SecureCrypto::wipe(sessionKey, AES_KEY_LEN);
        sessionUnlocked = false;
    }

    bool unlocked() const { return sessionUnlocked; }
    const uint8_t *getSessionKey() const { return sessionKey; }

    // Full wipe: erases the vault key material AND signals caller to
    // also wipe SD contents. Use after N catastrophic failed attempts
    // if you want a "self-destruct" style panic wipe (opt-in, off by
    // default -- irreversible data loss on a false trigger is a real risk).
    void factoryReset() {
        prefs.clear();
        lock();
    }

private:
    Preferences prefs;
    uint8_t sessionKey[AES_KEY_LEN];
    bool sessionUnlocked = false;
};

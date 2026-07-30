#pragma once
#include <Arduino.h>
#include <SD.h>
#include "crypto.h"

// ============================================================
// vault.h — encrypted-at-rest file storage on the SD card
//
// Every file written through this class is stored on disk as
// raw AES-256-GCM ciphertext (IV || ciphertext || tag). Plaintext
// filenames are still visible on the SD card's FAT filesystem --
// this encrypts CONTENTS, not metadata/filenames. If filename
// confidentiality matters to you, store an internal index (also
// encrypted) mapping opaque filenames to display names -- left as
// a phase-2 extension, noted in README.
// ============================================================

#define VAULT_DIR "/vault"

class Vault {
public:
    bool begin() {
        if (!SD.exists(VAULT_DIR)) {
            return SD.mkdir(VAULT_DIR);
        }
        return true;
    }

    // Write plaintext to an encrypted file. name is used as-is under /vault.
    bool writeFile(const uint8_t *key, const String &name, const uint8_t *data, size_t len) {
        String path = String(VAULT_DIR) + "/" + name + ".enc";
        File f = SD.open(path, FILE_WRITE);
        if (!f) return false;

        size_t bufCap = len + GCM_IV_LEN + GCM_TAG_LEN;
        uint8_t *outBuf = (uint8_t *)malloc(bufCap);
        if (!outBuf) { f.close(); return false; }

        size_t outLen;
        bool ok = SecureCrypto::encrypt(key, data, len, outBuf, outLen);
        if (ok) {
            f.write(outBuf, outLen);
        }
        free(outBuf);
        f.close();
        return ok;
    }

    // Read + decrypt a file. Caller owns outData (malloc'd) and must free() it.
    bool readFile(const uint8_t *key, const String &name, uint8_t **outData, size_t &outLen) {
        String path = String(VAULT_DIR) + "/" + name + ".enc";
        File f = SD.open(path, FILE_READ);
        if (!f) return false;

        size_t fileLen = f.size();
        uint8_t *cipherBuf = (uint8_t *)malloc(fileLen);
        if (!cipherBuf) { f.close(); return false; }
        f.read(cipherBuf, fileLen);
        f.close();

        uint8_t *plainBuf = (uint8_t *)malloc(fileLen); // plaintext always <= ciphertext len
        if (!plainBuf) { free(cipherBuf); return false; }

        bool ok = SecureCrypto::decrypt(key, cipherBuf, fileLen, plainBuf, outLen);
        free(cipherBuf);
        if (!ok) {
            free(plainBuf);
            return false; // wrong key or tampered file -- caller should treat as untrusted
        }
        *outData = plainBuf;
        return true;
    }

    bool deleteFile(const String &name) {
        String path = String(VAULT_DIR) + "/" + name + ".enc";
        return SD.remove(path);
    }

    // List vault entries (encrypted filenames minus the .enc suffix)
    std::vector<String> list() {
        std::vector<String> out;
        File dir = SD.open(VAULT_DIR);
        if (!dir) return out;
        File entry = dir.openNextFile();
        while (entry) {
            String n = String(entry.name());
            if (n.endsWith(".enc")) {
                out.push_back(n.substring(0, n.length() - 4));
            }
            entry.close();
            entry = dir.openNextFile();
        }
        dir.close();
        return out;
    }

    // Convenience: save/load a text note
    bool saveNote(const uint8_t *key, const String &title, const String &body) {
        return writeFile(key, "note_" + title, (const uint8_t *)body.c_str(), body.length());
    }

    bool loadNote(const uint8_t *key, const String &title, String &outBody) {
        uint8_t *data; size_t len;
        if (!readFile(key, "note_" + title, &data, len)) return false;
        outBody = String((const char *)data, len);
        free(data);
        return true;
    }
};

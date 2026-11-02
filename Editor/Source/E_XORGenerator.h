#pragma once

#include <pch.h>
#include <U_Variables.h>

// ============================================================================
// E_XORGenerator: Aurora Editor XOR Encryption Key Generator
// ============================================================================
// Converts any user passphrase into a high-entropy 256-bit (32-byte) keystream
// and auto-generates the corresponding C++ header for standalone game builds!
// ============================================================================
class E_XORGenerator {
public:
    // Derives a 32-byte (256-bit) cryptographic keystream from an arbitrary string passphrase
    static Array<BYTE> Generate_Key(const String &passphrase);

    // Formats the key as a hexadecimal string (e.g. "0x3A, 0xF1, 0x09, ...")
    static String Format_HexBytes(const Array<BYTE> &key);

    // Generates a ready-to-compile C++ header file string containing the key
    static String Generate_Header_Source(const String &passphrase, const Array<BYTE> &key);

    // Saves the generated C++ header file directly to a project's Config directory
    static BOOL Save_Header_To_File(const FilePath &outPath, const String &passphrase, const Array<BYTE> &key);
};

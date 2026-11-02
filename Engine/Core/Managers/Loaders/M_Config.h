#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <M_Log.h>
#include <M_File.h>

// ============================================================================
// Auto-Converting Config Value (Implicit cast to int, bool, float, String!)
// ============================================================================
struct ConfigValue {
    String Raw;

    operator String() const { return Raw; }
    operator StringView() const { return Raw; }
    operator const char*() const { return Raw.c_str(); }
    operator bool() const { return (Raw == "true" || Raw == "True" || Raw == "TRUE" || Raw == "1"); }
    operator int() const { try { return std::stoi(Raw); } catch (...) { return 0; } }
    operator unsigned int() const { try { return static_cast<unsigned int>(std::stoul(Raw)); } catch (...) { return 0; } }
    operator float() const { try { return std::stof(Raw); } catch (...) { return 0.0f; } }
    operator double() const { try { return std::stod(Raw); } catch (...) { return 0.0; } }
};

// ============================================================================
// Lightweight In-Memory Config (.ini / .cfg Key-Value Store)
// ============================================================================
// Knows NOTHING about your game. It only reads, stores and writes "Key = Value" lines.
// Your game decides which keys exist and what their defaults are.
//
// Example:
//   M_Config Settings;
//   Settings.Load("Settings.ini", "Width = 1280\nHeight = 720\n"); // 2nd arg = file written if missing
//   INT  width = Settings.Get("Width");
//   INT  newW  = Settings.Set("Width", 1920);
//   Settings.Save("Settings.ini");                                  // Keeps your # comments!
// ============================================================================
struct M_Config {
    Map<String, String> Values;

private:
    static String Trim(StringView text);
    static BOOL   ParseLine(StringView rawLine, String &key, String &value);

public:
    // Loads "Key = Value" lines from disk into RAM.
    // If the file is missing and 'defaultText' is given, the file is created from it first.
    BOOL Load(const FilePath &path, StringView defaultText = "");

    // Saves RAM values to disk. Existing lines (and their comments) are kept in place,
    // only the values are updated. New keys are added at the bottom.
    BOOL Save(const FilePath &path) const;

    // Read-only: returns an auto-converting value (never changes the map)
    ConfigValue Get(StringView key) const;

    // Writes to RAM and returns the newly set value
    template<typename T> requires (!std::is_convertible_v<T, StringView>)
    T Set(StringView key, const T &val) {
        if constexpr (std::is_same_v<T, bool>) {
            Values[String(key)] = val ? "true" : "false";
        } else {
            Values[String(key)] = StringFormat("{}", val);
        }
        return val;
    }

    // Text overload so Settings.Set("API", "Vulkan") works with plain quotes
    StringView Set(StringView key, StringView val);
};

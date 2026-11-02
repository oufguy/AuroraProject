#include <M_Config.h>

String M_Config::Trim(StringView text) {
    size_t start = text.find_first_not_of(" \t\r\n");
    if (start == StringView::npos) return "";
    size_t end = text.find_last_not_of(" \t\r\n");
    return String(text.substr(start, end - start + 1));
}

BOOL M_Config::ParseLine(StringView rawLine, String &key, String &value) {
    String line = Trim(rawLine);
    if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') return false;

    size_t commentPos = line.find_first_of(";#");
    if (commentPos != String::npos) line = Trim(StringView(line).substr(0, commentPos));

    size_t eqPos = line.find('=');
    if (eqPos == String::npos) return false;

    key   = Trim(StringView(line).substr(0, eqPos));
    value = Trim(StringView(line).substr(eqPos + 1));
    return !key.empty();
}

BOOL M_Config::Load(const FilePath &path, StringView defaultText) {
    if (!M_File::Find(path)) {
        if (defaultText.empty()) {
            LOG(Warn, LogLevel::Core, "M_Config::Load '{}' not found.", path.string());
            return false;
        }
        LOG(Warn, LogLevel::Core, "M_Config::Load '{}' not found - creating it from defaults.", path.string());
        M_File::Write(path, defaultText, true);
    }

    String rawText = M_File::Open(path);
    if (rawText.empty()) return false;

    Values.clear();
    size_t pos = 0;
    while (pos < rawText.size()) {
        size_t nextLine = rawText.find('\n', pos);
        if (nextLine == String::npos) nextLine = rawText.size();

        String key, value;
        if (ParseLine(StringView(rawText).substr(pos, nextLine - pos), key, value)) {
            Values[key] = value;
        }
        pos = nextLine + 1;
    }
    LOG(Info, LogLevel::Core, "M_Config::Load loaded {} settings from {}", Values.size(), path.string());
    return true;
}

BOOL M_Config::Save(const FilePath &path) const {
    String oldText = M_File::Find(path) ? M_File::Open(path) : "";
    String output;
    ::Set<String> written;

    size_t pos = 0;
    while (pos < oldText.size()) {
        size_t nextLine = oldText.find('\n', pos);
        if (nextLine == String::npos) nextLine = oldText.size();
        StringView line = StringView(oldText).substr(pos, nextLine - pos);
        pos = nextLine + 1;

        String key, oldValue;
        auto it = ParseLine(line, key, oldValue) ? Values.find(key) : Values.end();
        if (it == Values.end()) {
            output += String(line) + "\n";
            continue;
        }

        size_t commentPos = line.find_first_of(";#");
        if (commentPos != StringView::npos) {
            output += StringFormat("{} = {}\t{}\n", key, it->second, line.substr(commentPos));
        } else {
            output += StringFormat("{} = {}\n", key, it->second);
        }
        written.insert(key);
    }

    for (const auto &[k, v] : Values) {
        if (!written.contains(k)) output += StringFormat("{} = {}\n", k, v);
    }
    return M_File::Write(path, output, true);
}

ConfigValue M_Config::Get(StringView key) const {
    auto it = Values.find(String(key));
    if (it != Values.end()) return { it->second };
    return { "" };
}

StringView M_Config::Set(StringView key, StringView val) {
    Values[String(key)] = String(val);
    return val;
}

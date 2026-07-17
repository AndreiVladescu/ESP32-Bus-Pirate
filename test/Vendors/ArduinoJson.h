#pragma once

#include <cctype>
#include <map>
#include <string>

class DeserializationError {
public:
    explicit DeserializationError(bool failed = false) : failed(failed) {}
    explicit operator bool() const { return failed; }

private:
    bool failed;
};

class JsonVariant {
public:
    JsonVariant& operator=(const std::string& nextValue) {
        value = nextValue;
        return *this;
    }

    JsonVariant& operator=(const char* nextValue) {
        value = nextValue ? nextValue : "";
        return *this;
    }

    template <typename T>
    T as() const;

    const std::string& asString() const {
        return value;
    }

private:
    std::string value;
};

template <>
inline std::string JsonVariant::as<std::string>() const {
    return value;
}

class JsonDocument {
public:
    bool containsKey(const char* key) const {
        return values.find(key ? key : "") != values.end();
    }

    JsonVariant& operator[](const char* key) {
        return values[key ? key : ""];
    }

    const JsonVariant& operator[](const char* key) const {
        static const JsonVariant empty;
        auto it = values.find(key ? key : "");
        return it == values.end() ? empty : it->second;
    }

    void clear() {
        values.clear();
        raw.clear();
    }

    std::map<std::string, JsonVariant> values;
    std::string raw;
};

namespace arduino_json_test {

inline std::string trim(const std::string& input) {
    const size_t start = input.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const size_t end = input.find_last_not_of(" \t\r\n");
    return input.substr(start, end - start + 1);
}

inline bool looksBalancedJson(const std::string& input) {
    bool inString = false;
    bool escaped = false;
    int braces = 0;
    int brackets = 0;

    for (char c : input) {
        if (escaped) {
            escaped = false;
            continue;
        }

        if (inString) {
            if (c == '\\') escaped = true;
            else if (c == '"') inString = false;
            continue;
        }

        if (c == '"') inString = true;
        else if (c == '{') ++braces;
        else if (c == '}') --braces;
        else if (c == '[') ++brackets;
        else if (c == ']') --brackets;

        if (braces < 0 || brackets < 0) return false;
    }

    return !inString && braces == 0 && brackets == 0;
}

inline bool readJsonString(const std::string& input, size_t& pos, std::string& out) {
    if (pos >= input.size() || input[pos] != '"') return false;
    ++pos;
    out.clear();

    bool escaped = false;
    for (; pos < input.size(); ++pos) {
        const char c = input[pos];
        if (escaped) {
            switch (c) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                default: out.push_back(c); break;
            }
            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '"') {
            ++pos;
            return true;
        }

        out.push_back(c);
    }

    return false;
}

inline void parseTopLevelStringValues(const std::string& input, JsonDocument& doc) {
    size_t pos = 0;
    while (pos < input.size()) {
        if (input[pos] != '"') {
            ++pos;
            continue;
        }

        std::string key;
        if (!readJsonString(input, pos, key)) return;

        while (pos < input.size() && std::isspace(static_cast<unsigned char>(input[pos]))) ++pos;
        if (pos >= input.size() || input[pos] != ':') continue;
        ++pos;

        while (pos < input.size() && std::isspace(static_cast<unsigned char>(input[pos]))) ++pos;
        if (pos >= input.size() || input[pos] != '"') continue;

        std::string value;
        if (!readJsonString(input, pos, value)) return;
        doc.values[key] = value;
    }
}

inline std::string escape(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 8);

    for (char c : input) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }

    return out;
}

inline std::string prettyFormat(const std::string& input) {
    std::string out;
    int indent = 0;
    bool inString = false;
    bool escaped = false;

    auto newline = [&]() {
        out.push_back('\n');
        for (int i = 0; i < indent; ++i) out += "  ";
    };

    for (char c : input) {
        if (escaped) {
            out.push_back(c);
            escaped = false;
            continue;
        }

        if (inString) {
            out.push_back(c);
            if (c == '\\') escaped = true;
            else if (c == '"') inString = false;
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(c))) continue;

        switch (c) {
            case '"':
                inString = true;
                out.push_back(c);
                break;
            case '{':
            case '[':
                out.push_back(c);
                ++indent;
                newline();
                break;
            case '}':
            case ']':
                --indent;
                newline();
                out.push_back(c);
                break;
            case ',':
                out.push_back(c);
                newline();
                break;
            case ':':
                out += ": ";
                break;
            default:
                out.push_back(c);
                break;
        }
    }

    return out;
}

}  // namespace arduino_json_test

inline DeserializationError deserializeJson(JsonDocument& doc, const std::string& json) {
    doc.clear();

    const std::string trimmed = arduino_json_test::trim(json);
    if (trimmed.empty()) return DeserializationError(true);
    if ((trimmed.front() != '{' && trimmed.front() != '[') || !arduino_json_test::looksBalancedJson(trimmed)) {
        return DeserializationError(true);
    }

    doc.raw = trimmed;
    arduino_json_test::parseTopLevelStringValues(trimmed, doc);
    return DeserializationError(false);
}

inline void serializeJson(const JsonDocument& doc, std::string& output) {
    output = "{";
    bool first = true;
    for (const auto& entry : doc.values) {
        if (!first) output += ",";
        first = false;
        output += "\"";
        output += arduino_json_test::escape(entry.first);
        output += "\":\"";
        output += arduino_json_test::escape(entry.second.asString());
        output += "\"";
    }
    output += "}";
}

inline void serializeJsonPretty(const JsonDocument& doc, std::string& output) {
    if (!doc.raw.empty()) {
        output = arduino_json_test::prettyFormat(doc.raw);
        return;
    }

    serializeJson(doc, output);
}

#include "Config.h"

#include <cctype>
#include <charconv>
#include <fstream>
#include <sstream>

namespace bufman {
namespace {

// Minimal cursor over the config text: whitespace skipping plus one-char
// match/consume helpers, enough for a flat JSON object.
class Scanner {
public:
    explicit Scanner(const std::string& text) : text_(text) {}

    void skip_ws() {
        while (pos_ < text_.size() &&
               std::isspace(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }
    bool at_end() {
        skip_ws();
        return pos_ >= text_.size();
    }
    char peek() {
        skip_ws();
        return pos_ < text_.size() ? text_[pos_] : '\0';
    }
    // Consumes `expected` if it is the next non-whitespace character.
    bool take(char expected) {
        if (peek() != expected) {
            return false;
        }
        ++pos_;
        return true;
    }
    // Raw access for value parsers that manage whitespace themselves.
    char raw_peek() const {
        return pos_ < text_.size() ? text_[pos_] : '\0';
    }
    char raw_take() { return text_[pos_++]; }
    bool raw_at_end() const { return pos_ >= text_.size(); }
    std::size_t pos() const { return pos_; }

private:
    const std::string& text_;
    std::size_t pos_ = 0;
};

// Parses a JSON string (double-quoted) into `value`. Simple escapes are
// supported; \u is rejected with a clear message rather than half-decoded.
bool parse_string(Scanner& s, std::string& value, std::string& error) {
    if (!s.take('"')) {
        error = "expected a quoted string";
        return false;
    }
    value.clear();
    for (;;) {
        if (s.raw_at_end()) {
            error = "unterminated string";
            return false;
        }
        const char c = s.raw_take();
        if (c == '"') {
            return true;
        }
        if (c == '\\') {
            if (s.raw_at_end()) {
                error = "unterminated string";
                return false;
            }
            const char esc = s.raw_take();
            switch (esc) {
                case '"': value.push_back('"'); break;
                case '\\': value.push_back('\\'); break;
                case '/': value.push_back('/'); break;
                case 'b': value.push_back('\b'); break;
                case 'f': value.push_back('\f'); break;
                case 'n': value.push_back('\n'); break;
                case 'r': value.push_back('\r'); break;
                case 't': value.push_back('\t'); break;
                case 'u':
                    error = "\\u escapes are not supported in config values";
                    return false;
                default:
                    error = "invalid escape '\\" + std::string(1, esc) + "'";
                    return false;
            }
            continue;
        }
        if (static_cast<unsigned char>(c) < 0x20) {
            error = "raw control character inside a string";
            return false;
        }
        value.push_back(c);
    }
}

// Parses a run of digits into a nonnegative integer.
bool parse_size(Scanner& s, std::size_t& value, std::string& error) {
    s.skip_ws();
    std::string digits;
    while (std::isdigit(static_cast<unsigned char>(s.raw_peek()))) {
        digits.push_back(s.raw_take());
    }
    if (digits.empty()) {
        error = "expected a nonnegative integer";
        return false;
    }
    const char* first = digits.data();
    const char* last = first + digits.size();
    const auto result = std::from_chars(first, last, value);
    if (result.ec != std::errc{}) {
        error = "integer too large";
        return false;
    }
    return true;
}

bool parse_config_text(const std::string& text, Config& out,
                       std::ostream& diagnostics, std::string& error) {
    Scanner s(text);
    if (!s.take('{')) {
        error = "expected '{' at the start of the config";
        return false;
    }
    if (s.peek() == '}') {
        error = "config object has no entries";
        return false;
    }

    bool have_policy = false;
    bool have_pool_size = false;
    for (;;) {
        std::string key;
        if (!parse_string(s, key, error)) {
            return false;
        }
        if (!s.take(':')) {
            error = "expected ':' after key '" + key + "'";
            return false;
        }

        if (key == "policy") {
            if (have_policy) {
                error = "duplicate key 'policy'";
                return false;
            }
            if (!parse_string(s, out.policy, error)) {
                return false;
            }
            have_policy = true;
        } else if (key == "pool_size") {
            if (have_pool_size) {
                error = "duplicate key 'pool_size'";
                return false;
            }
            if (!parse_size(s, out.pool_size, error)) {
                return false;
            }
            have_pool_size = true;
        } else {
            // Unknown key: consume its value (string or number) and move on.
            if (s.peek() == '"') {
                std::string ignored;
                if (!parse_string(s, ignored, error)) {
                    return false;
                }
            } else {
                std::size_t ignored = 0;
                if (!parse_size(s, ignored, error)) {
                    return false;
                }
            }
            diagnostics << "config: ignoring unknown key '" << key << "'\n";
        }

        if (s.take(',')) {
            if (s.peek() == '}') {
                error = "trailing comma before '}'";
                return false;
            }
            continue;
        }
        if (s.take('}')) {
            break;
        }
        error = "expected ',' or '}' after entry '" + key + "'";
        return false;
    }

    if (!s.at_end()) {
        error = "unexpected content after the closing '}'";
        return false;
    }
    if (!have_policy) {
        error = "missing 'policy' entry";
        return false;
    }
    if (!have_pool_size) {
        error = "missing 'pool_size' entry";
        return false;
    }
    if (out.pool_size == 0) {
        error = "'pool_size' must be >= 1";
        return false;
    }
    return true;
}

}

bool load_config(const std::string& path, Config& out,
                 std::ostream& diagnostics, std::string& error) {
    error.clear();
    out = Config{};

    std::ifstream input(path);
    if (!input) {
        error = "cannot open config file: " + path;
        return false;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return parse_config_text(buffer.str(), out, diagnostics, error);
}

}

// foliage_un06 — structured JSON parser implementation.
#include "foliage_json.h"

namespace foliage { namespace json {

// ---------------------------------------------------------------------------
// Value helpers
// ---------------------------------------------------------------------------
const std::string& Value::StrOr(const std::string& def) const {
    static std::string empty;
    return IsString() ? strVal : (def.empty() ? empty : def);
}

double Value::ClampedNum(const char* key, double def, double min, double max) const {
    double v = NumKey(key, def);
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

int Value::ClampedInt(const char* key, int def, int min, int max) const {
    int v = IntKey(key, def);
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------
static void SerializeVal(const Value& v, std::string& out) {
    switch (v.type) {
    case Type::Null:   out += "null"; break;
    case Type::Bool:   out += v.boolVal ? "true" : "false"; break;
    case Type::Number: {
        char buf[64];
        if (std::abs(v.numVal - std::round(v.numVal)) < 1e-9 && std::abs(v.numVal) < 1e15)
            std::snprintf(buf,sizeof(buf),"%.0f",v.numVal);
        else
            std::snprintf(buf,sizeof(buf),"%g",v.numVal);
        out += buf; break;
    }
    case Type::String: out += "\"" + v.strVal + "\""; break; // simplified (no escapes)
    case Type::Array:
        out += "[";
        for (size_t i=0; i<v.arrVal.size(); ++i) {
            if (i) out += ",";
            SerializeVal(v.arrVal[i], out);
        }
        out += "]"; break;
    case Type::Object:
        out += "{";
        bool first=true;
        for (auto& [k,val] : v.objVal) {
            if (!first) out += ",";
            first=false;
            out += "\"" + k + "\":";
            SerializeVal(val, out);
        }
        out += "}"; break;
    }
}

std::string Value::Serialize() const { std::string s; SerializeTo(s); return s; }
void Value::SerializeTo(std::string& out) const { SerializeVal(*this, out); }

// ---------------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------------
void Parser::SkipWS() const {
    while (src[pos]) {
        char c = src[pos];
        if (c==' '||c=='\t'||c=='\n'||c=='\r') ++pos;
        else break;
    }
}
char Parser::Peek() const {
    const_cast<Parser*>(this)->SkipWS();
    return src[pos];
}
char Parser::Next()  { SkipWS(); return src[pos] ? src[pos++] : '\0'; }

void Parser::SetError(const char* msg) {
    error.message = msg;
    error.offset = pos;
}

Value Parser::Parse(const char* json) {
    src = json;
    pos = 0;
    error = {};
    if (!json || !json[0]) { SetError("empty input"); return Value::Null(); }
    Value v = ParseValue();
    SkipWS();
    if (error.ok() && src[pos]) SetError("trailing data after root value");
    return v;
}

Value Parser::ParseValue() {
    SkipWS();
    if (!src[pos]) { SetError("unexpected end of input"); return Value::Null(); }
    char c = src[pos];
    if (c == '{') return ParseObject();
    if (c == '[') return ParseArray();
    if (c == '"') return ParseString();
    if (c == 't' || c == 'f') return ParseLiteral(c=='t'?"true":"false", Type::Bool, c=='t');
    if (c == 'n') return ParseLiteral("null", Type::Null);
    if ((c>='0'&&c<='9') || c=='-'||c=='+') return ParseNumber();
    SetError("unexpected character");
    return Value::Null();
}

Value Parser::ParseObject() {
    std::map<std::string,Value> obj;
    if (Next() != '{') { SetError("expected {"); return Value::Object(std::move(obj)); }
    SkipWS();
    if (Peek() == '}') { ++pos; return Value::Object(std::move(obj)); }
    while (true) {
        SkipWS();
        if (src[pos] != '"') { SetError("expected string key"); break; }
        Value key = ParseString();
        if (!key.IsString()) break;
        SkipWS();
        if (src[pos] != ':') { SetError("expected :"); break; }
        ++pos;
        Value val = ParseValue();
        if (!error.ok()) break;
        obj[key.strVal] = std::move(val);
        SkipWS();
        if (src[pos] == ',') { ++pos; continue; }
        if (src[pos] == '}') { ++pos; break; }
        SetError("expected , or }");
        break;
    }
    return Value::Object(std::move(obj));
}

Value Parser::ParseArray() {
    std::vector<Value> arr;
    if (Next() != '[') { SetError("expected ["); return Value::Array(std::move(arr)); }
    SkipWS();
    if (Peek() == ']') { ++pos; return Value::Array(std::move(arr)); }
    while (true) {
        Value v = ParseValue();
        if (!error.ok()) break;
        arr.push_back(std::move(v));
        SkipWS();
        if (src[pos] == ',') { ++pos; continue; }
        if (src[pos] == ']') { ++pos; break; }
        SetError("expected , or ]");
        break;
    }
    return Value::Array(std::move(arr));
}

Value Parser::ParseString() {
    if (src[pos] != '"') { SetError("expected \""); return Value(""); }
    ++pos;
    std::string s; s.reserve(64);
    while (src[pos] && src[pos] != '"') {
        if (src[pos] == '\\') {
            ++pos;
            if (!src[pos]) break;
            switch (src[pos]) {
                case '"': s+='"'; break; case '\\': s+='\\'; break;
                case '/': s+='/'; break; case 'n': s+='\n'; break;
                case 'r': s+='\r'; break; case 't': s+='\t'; break;
                default: s+=src[pos]; break;
            }
        } else {
            s += src[pos];
        }
        ++pos;
    }
    if (src[pos] == '"') ++pos;
    return Value(std::move(s));
}

Value Parser::ParseNumber() {
    char* end = nullptr;
    double v = std::strtod(src + pos, &end);
    if (end == src + pos) { SetError("invalid number"); return Value(0.0); }
    pos = static_cast<int>(end - src);
    return Value(v);
}

Value Parser::ParseLiteral(const char* word, Type t, bool b) {
    size_t len = std::strlen(word);
    if (std::strncmp(src+pos, word, len) == 0) { pos += (int)len; return t==Type::Null ? Value::Null() : Value(b); }
    SetError("unexpected token");
    return Value::Null();
}

// ---------------------------------------------------------------------------
// ResponseEnvelope
// ---------------------------------------------------------------------------
std::string ResponseEnvelope::Serialize() const {
    std::string out;
    char buf[512];
    out += "{";
    std::snprintf(buf,sizeof(buf),"\"ok\":%s,\"plugin\":\"foliage_un06\",\"command\":\"%s\",\"version\":\"%s\"",
                  ok?"true":"false", command.c_str(), version.c_str());
    out += buf;
    if (!storeKey.empty()) { out += ",\"storeKey\":\""; out += storeKey; out += "\""; }
    if (dryRun) out += ",\"dryRun\":true";
    if (traceId) { std::snprintf(buf,sizeof(buf),",\"traceId\":%llu",(unsigned long long)traceId); out+=buf; }

    if (ok) {
        out += ",\"result\":";
        result.SerializeTo(out);
    } else {
        out += ",\"error\":{";
        out += "\"code\":\"" + errorCode + "\"";
        if (!errorField.empty()) { out += ",\"field\":\""; out += errorField; out += "\""; }
        out += ",\"message\":\""; out += errorMessage; out += "\"";
        out += "}";
    }
    if (!diagnostics.empty()) {
        out += ",\"diagnostics\":[";
        for (size_t i=0;i<diagnostics.size();++i){ if(i)out+=","; out+="\""; out+=diagnostics[i]; out+="\""; }
        out += "]";
    }
    out += "}";
    return out;
}

} } // namespace foliage::json

// foliage_un06 — structured JSON value model + lightweight parser.
// Replaces fragile string-scan parsing with structural validation.
#pragma once

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <map>

namespace foliage { namespace json {

// ---------------------------------------------------------------------------
// Value — tagged union for JSON values
// ---------------------------------------------------------------------------
enum class Type : uint8_t { Null, Bool, Number, String, Array, Object };

struct ParseError {
    std::string message;
    int         offset = 0;
    bool        ok() const { return message.empty(); }
};

struct Value {
    Type type = Type::Null;

    // Scalar storage
    bool   boolVal   = false;
    double numVal    = 0.0;
    std::string strVal;

    // Compound storage
    std::vector<Value>        arrVal;
    std::map<std::string,Value> objVal;

    // Constructors
    Value() = default;
    explicit Value(bool v)       : type(Type::Bool),   boolVal(v) {}
    explicit Value(double v)     : type(Type::Number), numVal(v) {}
    explicit Value(int v)        : type(Type::Number), numVal(static_cast<double>(v)) {}
    explicit Value(const char* v): type(Type::String), strVal(v?v:"") {}
    explicit Value(std::string v): type(Type::String), strVal(std::move(v)) {}

    static Value Null() { return Value(); }
    static Value Array(std::vector<Value> v) { Value r; r.type=Type::Array; r.arrVal=std::move(v); return r; }
    static Value Object(std::map<std::string,Value> v) { Value r; r.type=Type::Object; r.objVal=std::move(v); return r; }
    static Value Error(const std::string& msg) { Value r; r.type=Type::String; r.strVal=msg; return r; }

    // Type checks
    bool IsNull()   const { return type==Type::Null; }
    bool IsBool()   const { return type==Type::Bool; }
    bool IsNumber() const { return type==Type::Number; }
    bool IsString() const { return type==Type::String; }
    bool IsArray()  const { return type==Type::Array; }
    bool IsObject() const { return type==Type::Object; }

    // Safe accessors
    const Value* Find(const char* key) const {
        if (!IsObject()) return nullptr;
        auto it = objVal.find(key);
        return (it != objVal.end()) ? &it->second : nullptr;
    }

    bool Has(const char* key) const { return Find(key) != nullptr; }

    double NumOr(double def=0.0) const { return IsNumber() ? numVal : def; }
    int    IntOr(int def=0) const { return IsNumber() ? static_cast<int>(numVal) : def; }
    bool   BoolOr(bool def=false) const { return IsBool() ? boolVal : def; }
    const std::string& StrOr(const std::string& def) const;
    uint32_t U32Or(uint32_t def=0) const { return IsNumber() ? static_cast<uint32_t>(numVal) : def; }

    // Object helpers
    double NumKey(const char* key, double def=0.0) const {
        auto* v = Find(key); return v ? v->NumOr(def) : def;
    }
    int IntKey(const char* key, int def=0) const {
        auto* v = Find(key); return v ? v->IntOr(def) : def;
    }
    bool BoolKey(const char* key, bool def=false) const {
        auto* v = Find(key); return v ? v->BoolOr(def) : def;
    }
    std::string StrKey(const char* key, const std::string& def="") const {
        auto* v = Find(key); return v ? v->StrOr(def) : def;
    }
    uint32_t U32Key(const char* key, uint32_t def=0) const {
        auto* v = Find(key); return v ? v->U32Or(def) : def;
    }

    // Array helpers
    size_t ArrSize() const { return IsArray() ? arrVal.size() : 0; }
    const Value* ArrAt(size_t i) const { return (IsArray()&&i<arrVal.size()) ? &arrVal[i] : nullptr; }

    // Serialize to JSON string
    std::string Serialize() const;
    void SerializeTo(std::string& out) const;

    // Bound + clamp helpers (for limits enforcement)
    double ClampedNum(const char* key, double def, double min, double max) const;
    int    ClampedInt(const char* key, int def, int min, int max) const;
};

// ---------------------------------------------------------------------------
// Parser — structural JSON parser with error reporting
// ---------------------------------------------------------------------------
struct Parser {
    const char* src = nullptr;
    ParseError error;

    Value Parse(const char* json);
    bool   IsDone() const { return pos >= (int)std::strlen(src); }

private:
    mutable int pos = 0; // mutable so Peek() can skip whitespace while const
    void SkipWS() const;
    char Peek() const;
    char Next();
    Value ParseValue();
    Value ParseObject();
    Value ParseArray();
    Value ParseString();
    Value ParseNumber();
    Value ParseLiteral(const char* word, Type t, bool b=false);
    void SetError(const char* msg);
};

// ---------------------------------------------------------------------------
// Envelope builder — consistent response format
// ---------------------------------------------------------------------------
struct ResponseEnvelope {
    bool        ok       = true;
    std::string command  = "foliage_un06.inspect";
    std::string version  = "0.5.0";
    std::string storeKey;
    bool        dryRun   = false;
    uint64_t    traceId  = 0;
    Value       result   = Value::Object({});
    std::vector<std::string> diagnostics;
    std::vector<std::string> warnings;
    std::string  errorCode;
    std::string  errorField;
    std::string  errorMessage;

    // Set error
    void SetError(const char* code, const char* msg, const char* field="") {
        ok=false; errorCode=code; errorMessage=msg; errorField=field?field:"";
    }

    // Serialize
    std::string Serialize() const;
};

// ---------------------------------------------------------------------------
// Validation helpers
// ---------------------------------------------------------------------------
struct ValidationResult {
    bool ok = true;
    std::string field;
    std::string message;

    static ValidationResult Pass() { return {true,"",""}; }
    static ValidationResult Fail(const char* f, const char* m) { return {false,f,m}; }

    void Apply(ResponseEnvelope& env) const {
        if (!ok) env.SetError("InvalidArgument", message.c_str(), field.c_str());
    }
};

inline ValidationResult RequirePresent(const Value& obj, const char* key) {
    if (!obj.Find(key)) return ValidationResult::Fail(key, "required field missing");
    return ValidationResult::Pass();
}
inline ValidationResult RequireRange(const Value& obj, const char* key, double min, double max) {
    auto* v = obj.Find(key);
    if (!v) return ValidationResult::Pass(); // optional
    if (!v->IsNumber()) return ValidationResult::Fail(key, "must be a number");
    double n = v->numVal;
    if (n < min || n > max) {
        char buf[128]; std::snprintf(buf,sizeof(buf),"must be in [%.2f, %.2f], got %.2f",min,max,n);
        return ValidationResult::Fail(key, buf);
    }
    return ValidationResult::Pass();
}

} } // namespace foliage::json

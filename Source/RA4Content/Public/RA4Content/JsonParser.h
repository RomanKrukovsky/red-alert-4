// Copyright (c) Red Alert 4 project. Minimal JSON parser (engine-free, no deps).
#pragma once

#include <map>
#include <string>
#include <variant>
#include <vector>

#ifndef RA4CONTENT_API
#define RA4CONTENT_API
#endif

namespace RA4
{
namespace Json
{

class Value;
using Object = std::map<std::string, Value>;
using Array = std::vector<Value>;

class Value
{
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    Value() : Type_(Type::Null) {}
    Value(bool b) : Type_(Type::Bool), Bool_(b) {}
    Value(double n) : Type_(Type::Number), Number_(n) {}
    Value(const std::string& s) : Type_(Type::String), String_(s) {}
    Value(std::string&& s) : Type_(Type::String), String_(std::move(s)) {}
    Value(Array&& a) : Type_(Type::Array), Array_(std::move(a)) {}
    Value(Object&& o) : Type_(Type::Object), Object_(std::move(o)) {}

    bool IsNull() const { return Type_ == Type::Null; }
    bool IsBool() const { return Type_ == Type::Bool; }
    bool IsNumber() const { return Type_ == Type::Number; }
    bool IsString() const { return Type_ == Type::String; }
    bool IsArray() const { return Type_ == Type::Array; }
    bool IsObject() const { return Type_ == Type::Object; }

    bool AsBool() const { return Bool_; }
    double AsNumber() const { return Number_; }
    int32_t AsInt() const { return static_cast<int32_t>(Number_); }
    const std::string& AsString() const { return String_; }
    const Array& AsArray() const { return Array_; }
    const Object& AsObject() const { return Object_; }

    const Value* Find(const std::string& Key) const
    {
        if (Type_ != Type::Object) return nullptr;
        auto It = Object_.find(Key);
        return It != Object_.end() ? &It->second : nullptr;
    }

private:
    Type Type_;
    bool Bool_ = false;
    double Number_ = 0;
    std::string String_;
    Array Array_;
    Object Object_;
};

// Parses a JSON string. Returns true on success.
// Exported: consumed across module boundaries (RA4Recon loads its settings JSON),
// and a non-exported symbol links fine in the static CMake harness but fails at
// dylib link time in the modular editor build.
RA4CONTENT_API bool Parse(const std::string& Text, Value& OutResult, std::string& OutError);

} // namespace Json
} // namespace RA4
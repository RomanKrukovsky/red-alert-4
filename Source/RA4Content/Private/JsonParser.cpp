// Copyright (c) Red Alert 4 project. Minimal JSON parser implementation.
#include "RA4Content/JsonParser.h"

#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace RA4
{
namespace Json
{

namespace
{

class Parser
{
public:
    Parser(const std::string& Text) : Text_(Text), Pos_(0) {}

    bool Parse(Value& Out, std::string& OutError)
    {
        SkipWhitespace();
        if (Pos_ >= Text_.size())
        {
            OutError = "empty input";
            return false;
        }
        return ParseValue(Out, OutError);
    }

private:
    const std::string& Text_;
    size_t Pos_;

    char Peek() const { return Pos_ < Text_.size() ? Text_[Pos_] : '\0'; }
    char Advance() { return Text_[Pos_++]; }
    bool AtEnd() const { return Pos_ >= Text_.size(); }

    void SkipWhitespace()
    {
        while (!AtEnd() && std::isspace(static_cast<unsigned char>(Peek())))
        {
            ++Pos_;
        }
    }

    bool ParseValue(Value& Out, std::string& OutError)
    {
        SkipWhitespace();
        if (AtEnd())
        {
            OutError = "unexpected end of input";
            return false;
        }
        const char C = Peek();
        if (C == '{') return ParseObject(Out, OutError);
        if (C == '[') return ParseArray(Out, OutError);
        if (C == '"') return ParseString(Out, OutError);
        if (C == 't' || C == 'f') return ParseBool(Out, OutError);
        if (C == 'n') return ParseNull(Out, OutError);
        return ParseNumber(Out, OutError);
    }

    bool ParseObject(Value& Out, std::string& OutError)
    {
        ++Pos_; // skip {
        SkipWhitespace();
        Object Obj;
        if (Peek() == '}')
        {
            ++Pos_;
            Out = Value(std::move(Obj));
            return true;
        }
        while (true)
        {
            SkipWhitespace();
            if (Peek() != '"')
            {
                OutError = "expected string key in object";
                return false;
            }
            Value KeyVal;
            if (!ParseString(KeyVal, OutError)) return false;
            const std::string Key = KeyVal.AsString();
            SkipWhitespace();
            if (Peek() != ':')
            {
                OutError = "expected ':' after key";
                return false;
            }
            ++Pos_; // skip :
            Value Val;
            if (!ParseValue(Val, OutError)) return false;
            Obj[Key] = std::move(Val);
            SkipWhitespace();
            if (Peek() == ',')
            {
                ++Pos_;
                continue;
            }
            if (Peek() == '}')
            {
                ++Pos_;
                break;
            }
            OutError = "expected ',' or '}' in object";
            return false;
        }
        Out = Value(std::move(Obj));
        return true;
    }

    bool ParseArray(Value& Out, std::string& OutError)
    {
        ++Pos_; // skip [
        SkipWhitespace();
        Array Arr;
        if (Peek() == ']')
        {
            ++Pos_;
            Out = Value(std::move(Arr));
            return true;
        }
        while (true)
        {
            Value Val;
            if (!ParseValue(Val, OutError)) return false;
            Arr.push_back(std::move(Val));
            SkipWhitespace();
            if (Peek() == ',')
            {
                ++Pos_;
                continue;
            }
            if (Peek() == ']')
            {
                ++Pos_;
                break;
            }
            OutError = "expected ',' or ']' in array";
            return false;
        }
        Out = Value(std::move(Arr));
        return true;
    }

    bool ParseString(Value& Out, std::string& OutError)
    {
        ++Pos_; // skip opening "
        std::string Result;
        while (!AtEnd())
        {
            const char C = Advance();
            if (C == '"')
            {
                Out = Value(std::move(Result));
                return true;
            }
            if (C == '\\')
            {
                if (AtEnd()) break;
                const char Esc = Advance();
                switch (Esc)
                {
                    case '"': Result.push_back('"'); break;
                    case '\\': Result.push_back('\\'); break;
                    case '/': Result.push_back('/'); break;
                    case 'n': Result.push_back('\n'); break;
                    case 't': Result.push_back('\t'); break;
                    case 'r': Result.push_back('\r'); break;
                    case 'b': Result.push_back('\b'); break;
                    case 'f': Result.push_back('\f'); break;
                    case 'u':
                    {
                        // Parse 4 hex digits
                        if (Pos_ + 4 > Text_.size())
                        {
                            OutError = "incomplete unicode escape";
                            return false;
                        }
                        unsigned int Code = 0;
                        for (int I = 0; I < 4; ++I)
                        {
                            const char H = Text_[Pos_++];
                            Code <<= 4;
                            if (H >= '0' && H <= '9') Code |= static_cast<unsigned int>(H - '0');
                            else if (H >= 'a' && H <= 'f') Code |= static_cast<unsigned int>(H - 'a' + 10);
                            else if (H >= 'A' && H <= 'F') Code |= static_cast<unsigned int>(H - 'A' + 10);
                            else { OutError = "bad hex digit"; return false; }
                        }
                        // UTF-8 encode
                        if (Code < 0x80)
                        {
                            Result.push_back(static_cast<char>(Code));
                        }
                        else if (Code < 0x800)
                        {
                            Result.push_back(static_cast<char>(0xC0 | (Code >> 6)));
                            Result.push_back(static_cast<char>(0x80 | (Code & 0x3F)));
                        }
                        else
                        {
                            Result.push_back(static_cast<char>(0xE0 | (Code >> 12)));
                            Result.push_back(static_cast<char>(0x80 | ((Code >> 6) & 0x3F)));
                            Result.push_back(static_cast<char>(0x80 | (Code & 0x3F)));
                        }
                        break;
                    }
                    default:
                        Result.push_back(Esc);
                        break;
                }
            }
            else
            {
                Result.push_back(C);
            }
        }
        OutError = "unterminated string";
        return false;
    }

    bool ParseNumber(Value& Out, std::string& OutError)
    {
        const size_t Start = Pos_;
        if (Peek() == '-') ++Pos_;
        while (!AtEnd() && (std::isdigit(static_cast<unsigned char>(Peek())) || Peek() == '.' || Peek() == 'e' || Peek() == 'E' || Peek() == '+' || Peek() == '-'))
        {
            ++Pos_;
        }
        const std::string NumStr = Text_.substr(Start, Pos_ - Start);
        if (NumStr.empty())
        {
            OutError = "expected number";
            return false;
        }
        double Num = std::strtod(NumStr.c_str(), nullptr);
        Out = Value(Num);
        return true;
    }

    bool ParseBool(Value& Out, std::string& OutError)
    {
        if (Text_.compare(Pos_, 4, "true") == 0)
        {
            Pos_ += 4;
            Out = Value(true);
            return true;
        }
        if (Text_.compare(Pos_, 5, "false") == 0)
        {
            Pos_ += 5;
            Out = Value(false);
            return true;
        }
        OutError = "expected true or false";
        return false;
    }

    bool ParseNull(Value& Out, std::string& OutError)
    {
        if (Text_.compare(Pos_, 4, "null") == 0)
        {
            Pos_ += 4;
            Out = Value();
            return true;
        }
        OutError = "expected null";
        return false;
    }
};

} // namespace

bool Parse(const std::string& Text, Value& OutResult, std::string& OutError)
{
    Parser P(Text);
    return P.Parse(OutResult, OutError);
}

} // namespace Json
} // namespace RA4
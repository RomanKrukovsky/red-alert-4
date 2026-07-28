// Copyright (c) Red Alert 4 project. Minimal test harness for the headless core.
//
// The same test bodies are registered with Unreal's Automation framework in the
// editor build (see RA4Tests/Private/AutomationBridge.cpp). Keeping the assertions
// engine-free means the determinism suite can run in CI in under a second without
// launching the editor.
#pragma once

#include <cstdio>
#include <string>
#include <vector>

namespace RA4Test
{

struct TestCase
{
    const char* Suite;
    const char* Name;
    void (*Func)();
};

// Registry is a function-local static so registration order does not depend on
// static initialization order across translation units.
inline std::vector<TestCase>& Registry()
{
    static std::vector<TestCase> Instance;
    return Instance;
}

struct Registrar
{
    Registrar(const char* Suite, const char* Name, void (*Func)()) { Registry().push_back({Suite, Name, Func}); }
};

struct Failure
{
    std::string Message;
    const char* File;
    int Line;
};

inline std::vector<Failure>& CurrentFailures()
{
    static std::vector<Failure> Instance;
    return Instance;
}

inline void ReportFailure(const std::string& Message, const char* File, int Line)
{
    CurrentFailures().push_back({Message, File, Line});
}

} // namespace RA4Test

#define RA4_TEST(Suite, Name)                                                                      \
    static void Suite##_##Name##_Body();                                                           \
    static RA4Test::Registrar Suite##_##Name##_Registrar(#Suite, #Name, &Suite##_##Name##_Body);    \
    static void Suite##_##Name##_Body()

#define RA4_EXPECT(Condition)                                                                      \
    do                                                                                             \
    {                                                                                              \
        if (!(Condition))                                                                          \
        {                                                                                          \
            RA4Test::ReportFailure("expected: " #Condition, __FILE__, __LINE__);                   \
        }                                                                                          \
    } while (0)

// Stops the test body on failure. Use where continuing would crash (null pointers,
// invalid handles) rather than produce more useful diagnostics.
#define RA4_REQUIRE(Condition)                                                                      \
    do                                                                                             \
    {                                                                                              \
        if (!(Condition))                                                                          \
        {                                                                                          \
            RA4Test::ReportFailure("required: " #Condition, __FILE__, __LINE__);                   \
            return;                                                                                \
        }                                                                                          \
    } while (0)

#define RA4_EXPECT_EQ(A, B)                                                                        \
    do                                                                                             \
    {                                                                                              \
        const auto RA4_A = (A);                                                                    \
        const auto RA4_B = (B);                                                                    \
        if (!(RA4_A == RA4_B))                                                                     \
        {                                                                                          \
            RA4Test::ReportFailure(std::string("expected ") + #A + " == " + #B + " (got " +        \
                                       std::to_string(RA4_A) + " vs " + std::to_string(RA4_B) + ")", \
                                   __FILE__, __LINE__);                                            \
        }                                                                                          \
    } while (0)

#define RA4_EXPECT_NEAR(A, B, Tolerance)                                                           \
    do                                                                                             \
    {                                                                                              \
        const auto RA4_A = (A);                                                                    \
        const auto RA4_B = (B);                                                                    \
        const auto RA4_D = RA4_A > RA4_B ? RA4_A - RA4_B : RA4_B - RA4_A;                          \
        if (!(RA4_D <= (Tolerance)))                                                               \
        {                                                                                          \
            RA4Test::ReportFailure(std::string("expected ") + #A + " ~= " + #B + " (got " +        \
                                       std::to_string(RA4_A) + " vs " + std::to_string(RA4_B) +    \
                                       ", tolerance " + std::to_string(Tolerance) + ")",           \
                                   __FILE__, __LINE__);                                            \
        }                                                                                          \
    } while (0)

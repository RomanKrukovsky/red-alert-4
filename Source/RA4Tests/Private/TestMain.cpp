// Copyright (c) Red Alert 4 project. Headless test runner entry point.
#include "TestFramework.h"

#include <chrono>
#include <cstring>

#if defined(_WIN32)
#include <direct.h>
#define RA4_CHDIR _chdir
#else
#include <unistd.h>
#define RA4_CHDIR chdir
#endif

namespace
{

// Content-loading tests open paths relative to the repository root. Whether that
// happens to be the working directory depends on where the binary was launched
// from, so running out of the build directory used to fail eighteen tests on file
// paths while reporting them as content errors. The root is baked in at configure
// time and entered here, once, so no test has to care.
void EnterRepositoryRoot()
{
#if defined(RA4_REPO_ROOT)
    if (RA4_CHDIR(RA4_REPO_ROOT) != 0)
    {
        std::printf("[ WARN ] could not enter repository root %s; "
                    "content-loading tests will fail on paths\n",
                    RA4_REPO_ROOT);
    }
#else
    std::printf("[ WARN ] RA4_REPO_ROOT not defined; content-loading tests depend on "
                "the working directory\n");
#endif
}

} // namespace

int main(int argc, char** argv)
{
    // Unbuffered: if a test crashes the harness, the name of the test that did it
    // must already be on the terminal.
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    EnterRepositoryRoot();

    const char* Filter = nullptr;
    bool bListOnly = false;
    for (int I = 1; I < argc; ++I)
    {
        if (std::strcmp(argv[I], "--list") == 0)
        {
            bListOnly = true;
        }
        else if (std::strncmp(argv[I], "--filter=", 9) == 0)
        {
            Filter = argv[I] + 9;
        }
    }

    auto& Tests = RA4Test::Registry();
    if (bListOnly)
    {
        for (const RA4Test::TestCase& T : Tests)
        {
            std::printf("%s.%s\n", T.Suite, T.Name);
        }
        return 0;
    }

    int Passed = 0;
    int Failed = 0;
    const auto Start = std::chrono::steady_clock::now();

    for (const RA4Test::TestCase& T : Tests)
    {
        const std::string FullName = std::string(T.Suite) + "." + T.Name;
        if (Filter != nullptr && FullName.find(Filter) == std::string::npos)
        {
            continue;
        }

        RA4Test::CurrentFailures().clear();
        const auto TestStart = std::chrono::steady_clock::now();
        T.Func();
        const auto TestEnd = std::chrono::steady_clock::now();
        const long long Ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(TestEnd - TestStart).count();

        if (RA4Test::CurrentFailures().empty())
        {
            std::printf("[ PASS ] %-52s %5lld ms\n", FullName.c_str(), Ms);
            ++Passed;
        }
        else
        {
            std::printf("[ FAIL ] %-52s %5lld ms\n", FullName.c_str(), Ms);
            for (const RA4Test::Failure& F : RA4Test::CurrentFailures())
            {
                std::printf("         %s:%d: %s\n", F.File, F.Line, F.Message.c_str());
            }
            ++Failed;
        }
    }

    const auto End = std::chrono::steady_clock::now();
    const long long TotalMs = std::chrono::duration_cast<std::chrono::milliseconds>(End - Start).count();

    std::printf("\n%d passed, %d failed, %lld ms total\n", Passed, Failed, TotalMs);
    return Failed == 0 ? 0 : 1;
}

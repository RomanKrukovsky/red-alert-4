// Copyright (c) Red Alert 4 project.
// Isolated external research analyzer tool for offline structural metrics extraction.
// MUST NOT be linked into game production runtime.

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    std::cout << "=== Red Alert 4 Research Analyzer Tool (Offline Isolation) ===" << std::endl;
    if (argc < 2)
    {
        std::cout << "Usage: RA3ResearchAnalyzer <path_to_research_summary_or_schema>" << std::endl;
        return 0;
    }

    std::string FilePath = argv[1];
    std::ifstream File(FilePath);
    if (!File.is_open())
    {
        std::cerr << "[ERROR] Unable to open input research file: " << FilePath << std::endl;
        return 1;
    }

    std::string Line;
    size_t LineCount = 0;
    size_t TagCount = 0;

    while (std::getline(File, Line))
    {
        LineCount++;
        if (Line.find('<') != std::string::npos && Line.find('>') != std::string::npos)
        {
            TagCount++;
        }
    }

    std::cout << "[INFO] Analysis completed for file: " << FilePath << std::endl;
    std::cout << "  - Total Lines Processed: " << LineCount << std::endl;
    std::cout << "  - Total Structured Tags Found: " << TagCount << std::endl;

    return 0;
}

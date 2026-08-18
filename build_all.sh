#!/bin/bash
# Red Alert 4 Unity - Build Automation Script
# Usage: ./build_all.sh [Debug|Release]

set -e

echo "🚀 Starting Red Alert 4 Unity Build..."

# Build all C# projects
for project in *.csproj; do
    echo "📦 Building $project..."
    dotnet build "$project" -c Release
    if [ $? -ne 0 ]; then
        echo "❌ Build failed!"
        exit 1
    fi
done

echo "✅ All projects built successfully!"

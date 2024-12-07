#!/bin/bash

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "clang-format not found. Please install it and try again."
    exit 1
fi

# Format files in ./core directory
echo "Formatting files in ./core..."
find ./core \( -name '*.h' -o -name '*.c' \) -exec clang-format -i "{}" --verbose \;

exec clang-format -i main.c --verbose

echo "Formatting completed."

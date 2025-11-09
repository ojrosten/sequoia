#!/bin/bash

# Check if a test directory was provided as an argument
if [[ -z "$1" ]]; then
  echo "Usage: $0 <Test Executable Directory>"
  exit 1
fi

# Get the directory of the script
script_dir=$(dirname "$0")

# Get the test directory from the first argument
test_dir="$1"

# Cleanup lcov
lcov --zerocounters --directory "$test_Dir"

# Relative location of the html output directory
output_dir="${script_dir}/../coverage_reports/$(basename "$PWD")"

# Create output directory if it doesn't exist
mkdir -p "$output_dir"

# Run ctest in coverage mode
ctest -T Coverage

# Generate lcov coverage report
lcov --directory "$test_dir".  --capture --output-file coverage.info --keep-going --filter range --rc geninfo_unexecuted_blocks=1 --gcov-tool /usr/bin/gcov-15
lcov --remove coverage.info '/usr/*' --output-file coverage.info --ignore-errors inconsistent

# Generate HTML report
genhtml --demangle-cpp -o "${output_dir}" coverage.info --ignore-errors inconsistent --ignore-errors range

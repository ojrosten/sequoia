#!/bin/bash

# Check if a test directory was provided as an argument
if [[ -z "$1" ]]; then
  echo "Usage: $0 <Test Executable Directory>"
  exit 1
fi

# Get the directory of the script
script_dir=$(dirname "$0")
echo "Script Dir: ${script_dir}"

# Get the test executable directory from the first argument
test_exe_dir_relative="$1"
test_exe_dir=$(cd "$test_exe_dir_relative" && pwd -P)
echo "Test Dir: ${test_exe_dir}"

# Cleanup lcov
lcov --zerocounters --directory "${test_exe_dir}"

# Get the path components after 'build/'
path_suffix="${test_exe_dir#*build/}"

# Relative location of the html output directory
output_dir="${script_dir}/../coverage_reports/${path_suffix}"
echo "Output Dir: ${output_dir}"

# Create output directory if it doesn't exist
mkdir -p "${output_dir}"

# Run ctest in coverage mode
pushd "${test_exe_dir}"
ctest -T Coverage
popd

# Generate lcov coverage report
lcov --directory "${test_exe_dir}"  --capture --output-file "${test_exe_dir}/coverage.info" --keep-going --filter range --rc geninfo_unexecuted_blocks=1 --ignore-errors empty --gcov-tool /usr/bin/gcov-15
lcov --remove  "${test_exe_dir}/coverage.info" '/usr/*' --output-file "${test_exe_dir}/coverage.info" --ignore-errors inconsistent

# Generate HTML report
genhtml --demangle-cpp -o "${output_dir}" "${test_exe_dir}/coverage.info" --ignore-errors inconsistent --ignore-errors range --ignore-errors empty

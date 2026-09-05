#!/bin/bash

# Check if a test directory was provided as an argument
if [[ -z "$1" ]]; then
  echo "Usage: $0 <Test Executable Directory>"
  exit 1
fi

# Get the test executable directory from the first argument
test_exe_dir_relative="$1"
test_exe_dir=$(cd "$test_exe_dir_relative" && pwd -P)
echo "Test Dir: ${test_exe_dir}"

# Get the path components before 'build/'
path_prefix="${test_exe_dir%%build/*}"

# Get the path components after 'build/'
path_suffix="${test_exe_dir#*build/}"

setup_file="${test_exe_dir}/Setup.txt"

if [[ -f "${setup_file}" ]]; then
    discriminator=$(head -n 1 "${setup_file}")
    path_suffix="${path_suffix}/${discriminator}"
fi

# Relative location of the html output directory
output_dir="${path_prefix}/coverage_reports/${path_suffix}"
echo "Output Dir: ${output_dir}"

# Create output directory if it doesn't exist
mkdir -p "${output_dir}"

# Cleanup lcov
lcov --zerocounters --directory "${test_exe_dir}"

# Run the tests to generate fresh .gcda files
pushd "${test_exe_dir}"
ctest -T Test
popd

# gcov must match the compiler which produced the .gcda files, so take it from the build itself
if [[ -z "${gcov_tool}" ]]; then
  cxx=$(sed -n 's/^CMAKE_CXX_COMPILER:[^=]*=//p' "${test_exe_dir}/CMakeCache.txt")
  case "${cxx##*/}" in
    g++-*)    gcov_tool="${cxx%/*}/gcov-${cxx##*g++-}" ;;
    clang++)  gcov_tool="${cxx%/*}/llvm-cov gcov"      ;;
    *)        gcov_tool="gcov"                         ;;
  esac
fi
echo "gcov: ${gcov_tool}"

# Generate lcov coverage report
lcov --directory "${test_exe_dir}"  --capture --output-file "${test_exe_dir}/coverage.info" --keep-going --filter range --rc geninfo_unexecuted_blocks=1 --ignore-errors empty --gcov-tool ${gcov_tool}
foreign=('/usr/*')
if [[ "$(uname -s)" == Darwin ]]; then
  foreign+=('/opt/homebrew/*' '/Library/Developer/*' '/Applications/Xcode.app/*')
fi

# The doubling is deliberate: it suppresses display too, leaving genhtml the sole reporter
lcov --remove  "${test_exe_dir}/coverage.info" "${foreign[@]}" --output-file "${test_exe_dir}/coverage.info" --ignore-errors inconsistent,inconsistent --ignore-errors empty

# lcov forces --no-strip-underscores on Darwin, which only GNU c++filt accepts
gnu_cxxfilt="/opt/homebrew/opt/binutils/bin/c++filt"
demangle=(--demangle-cpp)
[[ -x "${gnu_cxxfilt}" ]] && demangle+=("${gnu_cxxfilt}")

# Generate HTML report
genhtml "${demangle[@]}" --suppress-aliases -o "${output_dir}" "${test_exe_dir}/coverage.info" --ignore-errors inconsistent --ignore-errors range --ignore-errors empty

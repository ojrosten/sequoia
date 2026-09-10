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
    # lcov invokes the tool with the .gcda as its first argument, so the two-word
    # 'llvm-cov gcov' has to be wrapped rather than passed
    clang++)  gcov_tool="${test_exe_dir}/llvm-gcov.sh"
              printf '#!/bin/sh\nexec "%s/llvm-cov" gcov "$@"\n' "${cxx%/*}" > "${gcov_tool}"
              chmod +x "${gcov_tool}"                  ;;
    *)        gcov_tool="gcov"                         ;;
  esac
fi
echo "gcov: ${gcov_tool}"

# Generate lcov coverage report
lcov --directory "${test_exe_dir}"  --capture --output-file "${test_exe_dir}/coverage.info" --keep-going --filter range --rc geninfo_unexecuted_blocks=1 --ignore-errors empty --ignore-errors inconsistent,inconsistent --ignore-errors format,format --gcov-tool "${gcov_tool}"
foreign=('/usr/*')
if [[ "$(uname -s)" == Darwin ]]; then
  foreign+=('/opt/homebrew/*' '/Library/Developer/*' '/Applications/Xcode.app/*')
fi

# The doubling is deliberate: it suppresses display too, leaving genhtml the sole reporter
lcov --remove  "${test_exe_dir}/coverage.info" "${foreign[@]}" --output-file "${test_exe_dir}/coverage.info" --keep-going --ignore-errors inconsistent,inconsistent --ignore-errors empty

# lcov forces --no-strip-underscores on Darwin, which only GNU c++filt accepts
gnu_cxxfilt="/opt/homebrew/opt/binutils/bin/c++filt"
demangle=(--demangle-cpp)
[[ -x "${gnu_cxxfilt}" ]] && demangle+=("${gnu_cxxfilt}")

# genhtml's set of ignorable error categories varies by lcov version: 'range' is
# accepted by 2.5 and rejected outright by the lcov in Ubuntu's archives, which
# stopped a coverage run dead. Each category below was added for a reason, so the
# list is filtered to what this genhtml accepts rather than trimmed to whatever
# every version has in common. Unknown categories are refused during argument
# parsing, before genhtml looks at its input, which is what makes the probe cheap.
probe_dir=$(mktemp -d)
ignore=()
for category in inconsistent range empty category; do
  if ! genhtml --ignore-errors "${category}" -o "${probe_dir}" /dev/null 2>&1 \
       | grep -q "unknown argument for --ignore-errors"; then
    ignore+=(--ignore-errors "${category}")
  else
    echo "genhtml does not support --ignore-errors ${category}; continuing without it"
  fi
done
rm -rf "${probe_dir}"

# Generate HTML report
genhtml "${demangle[@]}" --suppress-aliases -o "${output_dir}" "${test_exe_dir}/coverage.info" "${ignore[@]}"

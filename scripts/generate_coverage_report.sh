#!/bin/bash

# Any command that fails ends the script with a non-zero status, so the step that runs the
# script fails with it: a failing suite or a failed capture must not leave a report that looks
# sound. There is no pipefail: the genhtml probe below pipes genhtml, which always fails there,
# into grep, and under pipefail every category would read as unsupported.
set -e

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

# Runs a command and, if it fails, names it before ending the script.
run_checked() {
  "$@" && return
  local status=$?
  echo "error: exit status ${status} from: $*" >&2
  exit 1
}

run_checked lcov --zerocounters --directory "${test_exe_dir}"

# Run the tests to generate fresh .gcda files
pushd "${test_exe_dir}"
run_checked ctest -T Test
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

# lcov checks coverage data for consistency, and repairs what it finds by overriding gcov's
# counts, in both directions. It checks on every read of a tracefile, and at capture wherever
# it has to derive the end lines of functions, which llvm-cov never supplies:
#  - A function gcov says was never called, but with a line that ran, is marked called.
#    The check exempts the first line of a lambda, which runs when the closure is built,
#    but it recognises a lambda only by its demangled name, and the tracefile holds mangled
#    names. So every uncalled lambda whose first line ran is reported as called.
#  - A function gcov says was called, but with no line that ran, is shown as uncalled in
#    genhtml's function tables.
# Turning the check off keeps gcov's counts. The lambda defect is drafted as an upstream
# lcov report, not yet filed.
consistency_options=(--rc check_data_consistency=0)

capture="${test_exe_dir}/coverage_capture.info"
info="${test_exe_dir}/coverage.info"
run_checked lcov --directory "${test_exe_dir}" --capture --output-file "${capture}" --gcov-tool "${gcov_tool}" \
                 --keep-going --filter range --rc geninfo_unexecuted_blocks=1 "${consistency_options[@]}" \
                 --ignore-errors empty --ignore-errors inconsistent,inconsistent --ignore-errors format,format

# A read derives end lines again for functions that have none. The capture has already done
# so, and a second derivation raises `inconsistent` wherever it fails.
read_options=("${consistency_options[@]}" --rc derive_function_end_line=0)

foreign=('/usr/*')
# Writing the tracefile again raises `format` for each function llvm-cov places at line 0.
# The capture has already raised that error and been told to ignore it.
remove_options=(--keep-going --ignore-errors empty --ignore-errors format)
if [[ "$(uname -s)" == Darwin ]]; then
  foreign+=('/opt/homebrew/*' '/Library/Developer/*' '/Applications/Xcode.app/*')
  # The patterns cover every toolchain's system headers, and no one build uses them all.
  # lcov treats a pattern that removes nothing as an error.
  remove_options+=(--ignore-errors unused)
fi

run_checked lcov --remove "${capture}" "${foreign[@]}" --output-file "${info}" \
                 "${remove_options[@]}" "${read_options[@]}"

# Removal must drop files and change nothing else, and lcov's figures must be counts of the
# tracefile's records. check_tracefile.py names the first difference. Nothing checks genhtml's
# function tables, where the second repair above would show.
summary="${test_exe_dir}/coverage_summary.txt"
if ! lcov --summary "${info}" "${read_options[@]}" > "${summary}" 2>&1; then
  cat "${summary}"
  echo "error: lcov --summary failed on ${info}" >&2
  exit 1
fi
cat "${summary}"
script_dir=$(cd "$(dirname "$0")" && pwd -P)
run_checked python3 "${script_dir}/check_tracefile.py" --capture "${capture}" --filtered "${info}" \
                                                        --summary "${summary}" --removed "${foreign[@]}"

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
for category in range empty category; do
  if ! genhtml --ignore-errors "${category}" -o "${probe_dir}" /dev/null 2>&1 \
       | grep -q "unknown argument for --ignore-errors"; then
    ignore+=(--ignore-errors "${category}")
  else
    echo "genhtml does not support --ignore-errors ${category}; continuing without it"
  fi
done
rm -rf "${probe_dir}"

# Generate HTML report
run_checked genhtml "${demangle[@]}" --suppress-aliases -o "${output_dir}" "${info}" "${ignore[@]}" "${read_options[@]}"

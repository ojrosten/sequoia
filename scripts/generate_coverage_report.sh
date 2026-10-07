#!/bin/bash
# Usage: generate_coverage_report.sh <build directory>
#
# Runs the suite of a coverage build, and writes an HTML report of the
# coverage the run measured. <build directory> is the build's binary
# directory, configured with Ninja by one of the coverage presets, at
# <root>/build/<project>/<preset>. The report goes to
# <root>/coverage_reports/<project>/<preset>. If the build directory holds a
# Setup.txt, the report goes to the subdirectory named by its first line. The
# script takes <root>/build to be the last directory named build in the build
# directory's physical path.
#
# The script leaves three files in the build directory:
#   - coverage_capture.info, the tracefile lcov captured;
#   - coverage.info, that tracefile without the files of the system and the
#     toolchains, from which the report is made;
#   - coverage_summary.txt, lcov's summary of coverage.info.
#
# The script runs lcov, genhtml, ctest and python3 from PATH, the ninja the
# build's cache names, and check_tracefile.py from its own directory. It runs
# the gcov tool which matches the build's compiler. On macOS it needs GNU
# c++filt where Homebrew's binutils puts it.
#
# Before it runs anything, the script refuses with status 2 a missing, empty
# or second argument, and a build directory with no directory named build in
# its path. It fails before it runs anything if the build directory or its
# CMakeCache.txt is missing, or, on macOS, GNU c++filt. A step that fails
# stops the script with a non-zero status.

# A command outside run_checked which fails, such as reading the cache, ends
# the script. There is no pipefail: the genhtml probe below pipes genhtml,
# which always fails there, into grep. Under pipefail the pipeline would fail
# whatever grep found, and every category would read as supported.
set -e

if [[ $# -ne 1 || -z "$1" ]]; then
  echo "Usage: $0 <build directory>" >&2
  exit 2
fi

test_exe_dir_relative="$1"
test_exe_dir=$(cd "$test_exe_dir_relative" && pwd -P)
echo "Test Dir: ${test_exe_dir}"

if [[ "${test_exe_dir}" != */build/* ]]; then
  echo "error: ${test_exe_dir} is not within a directory named build" >&2
  exit 2
fi
path_prefix="${test_exe_dir%/build/*}"
path_suffix="${test_exe_dir##*/build/}"

setup_file="${test_exe_dir}/Setup.txt"

if [[ -f "${setup_file}" ]]; then
  discriminator=$(head -n 1 "${setup_file}")
  path_suffix="${path_suffix}/${discriminator}"
fi

output_dir="${path_prefix}/coverage_reports/${path_suffix}"
echo "Output Dir: ${output_dir}"

# lcov forces --no-strip-underscores on Darwin, which only GNU c++filt
# accepts. Apple's c++filt refuses it, and genhtml then reports that the
# tracefile holds no valid records. The script checks for GNU c++filt before
# the suite runs, rather than fail once it has.
platform=$(uname -s)
gnu_cxxfilt="/opt/homebrew/opt/binutils/bin/c++filt"
demangle=(--demangle-cpp)
if [[ "${platform}" == Darwin ]]; then
  if [[ ! -x "${gnu_cxxfilt}" ]]; then
    echo "error: on macOS, genhtml needs GNU c++filt at ${gnu_cxxfilt}" >&2
    exit 1
  fi
  demangle+=("${gnu_cxxfilt}")
fi

# Runs a command and, if it fails, names it before ending the script.
run_checked() {
  "$@" && return
  local status=$?
  echo "error: exit status ${status} from: $*" >&2
  exit 1
}

# The capture below reads every notes file (.gcno) in the build, stale ones
# included. A stale notes file of a source since dropped from the build gives
# functions nothing can call. One of a deleted source fails the capture.
# Ninja's cleandead deletes the objects of sources the build no longer has.
# Then the script deletes each notes file without its object.
make_program=$(sed -n 's/^CMAKE_MAKE_PROGRAM:[^=]*=//p' "${test_exe_dir}/CMakeCache.txt")
run_checked "${make_program}" -C "${test_exe_dir}" -t cleandead
while IFS= read -r notes; do
  [[ -f "${notes%.gcno}.o" ]] || run_checked rm "${notes}"
done < <(find "${test_exe_dir}" -name '*.gcno')

run_checked lcov --zerocounters --directory "${test_exe_dir}"

pushd "${test_exe_dir}"
run_checked ctest -T Test
popd

# gcov must match the compiler which wrote the data files (.gcda), so the
# script chooses the tool by the build's compiler.
cxx=$(sed -n 's/^CMAKE_CXX_COMPILER:[^=]*=//p' "${test_exe_dir}/CMakeCache.txt")
case "${cxx##*/}" in
  g++-*)    gcov_tool="${cxx%/*}/gcov-${cxx##*g++-}" ;;
  # lcov runs the tool with a data file as its first argument, so the script
  # writes a wrapper to run the two words `llvm-cov gcov`.
  clang++)  gcov_tool="${test_exe_dir}/llvm-gcov.sh"
            printf '#!/bin/sh\nexec "%s/llvm-cov" gcov "$@"\n' "${cxx%/*}" > "${gcov_tool}"
            chmod +x "${gcov_tool}"                  ;;
  *)        gcov_tool="gcov"                         ;;
esac
echo "gcov: ${gcov_tool}"

# lcov checks coverage data for consistency, and repairs what it finds by
# overriding gcov's counts, in both directions. It checks on every read of a
# tracefile, and at capture wherever it has to derive the end lines of
# functions, which llvm-cov never supplies:
#   - A function gcov says was never called, but with a line that ran, is
#     marked called. The check exempts the first line of a lambda, which runs
#     when the closure is built. But it recognises a lambda only by its
#     demangled name, and the tracefile holds mangled names. So every uncalled
#     lambda whose first line ran is reported as called: lcov issue 557,
#     https://github.com/linux-test-project/lcov/issues/557.
#   - A function gcov says was called, but with no line that ran, is shown as
#     uncalled in genhtml's function tables.
# Turning the check off keeps gcov's counts.
consistency_options=(--rc check_data_consistency=0)

capture="${test_exe_dir}/coverage_capture.info"
info="${test_exe_dir}/coverage.info"
# --all captures each object that never ran, with every count zero. Without
# it, such an object is absent from the tracefile. The linker leaves out any
# object of a static library which nothing references, and that object's
# functions would then be neither called nor uncalled.
run_checked lcov --directory "${test_exe_dir}" --capture --all --output-file "${capture}" --gcov-tool "${gcov_tool}" \
                 --keep-going --filter range --rc geninfo_unexecuted_blocks=1 "${consistency_options[@]}" \
                 --ignore-errors empty --ignore-errors inconsistent,inconsistent --ignore-errors format,format

# A read derives end lines again for functions that have none. The capture
# has already done so, and a second derivation raises `inconsistent` wherever
# it fails.
read_options=("${consistency_options[@]}" --rc derive_function_end_line=0)

foreign=('/usr/*')
# Writing the tracefile again raises `format` for each function llvm-cov
# places at line 0. The capture has already raised that error and been told
# to ignore it.
remove_options=(--keep-going --ignore-errors empty --ignore-errors format)
if [[ "${platform}" == Darwin ]]; then
  foreign+=('/opt/homebrew/*' '/Library/Developer/*' '/Applications/Xcode.app/*')
  # The patterns cover every toolchain's system headers, and no one build
  # uses them all. lcov treats a pattern that removes nothing as an error.
  remove_options+=(--ignore-errors unused)
fi

run_checked lcov --remove "${capture}" "${foreign[@]}" --output-file "${info}" \
                 "${remove_options[@]}" "${read_options[@]}"

# Removal must drop files and change nothing else, and lcov's figures must be
# counts of the tracefile's records. check_tracefile.py names the first
# difference. Nothing checks genhtml's function tables, where the second
# repair above would show.
summary="${test_exe_dir}/coverage_summary.txt"
run_checked lcov --summary "${info}" "${read_options[@]}" > "${summary}"
cat "${summary}"
script_dir=$(cd "$(dirname "$0")" && pwd -P)
run_checked python3 "${script_dir}/check_tracefile.py" --capture "${capture}" --filtered "${info}" \
                                                        --summary "${summary}" --removed "${foreign[@]}"

# genhtml refuses to run when given an error category it does not know, and
# the categories it knows vary by lcov release: lcov 2.0, which Ubuntu 24.04
# ships, does not know `range`. So the script passes each category only if
# this genhtml knows it, and names each category it drops. genhtml refuses an
# unknown category while it parses its arguments, before it reads its input,
# so the probe is cheap.
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

run_checked genhtml "${demangle[@]}" --suppress-aliases -o "${output_dir}" "${info}" "${ignore[@]}" "${read_options[@]}"

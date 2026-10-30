# Sourced, not run. Defines macos_crash_report_directories, which prints each
# directory in which macOS may write a crash report, one per line.
#
# macOS usually writes a crash report to the user's
# ~/Library/Logs/DiagnosticReports. The kernel sometimes kills the user's
# ReportCrash as the crash arrives, if ReportCrash is idle and over its memory
# limit. The instance that takes over then writes the report to the system's
# /Library/Logs/DiagnosticReports, which only the group _analyticsusers can
# read.
#
# A REPORT_CRASHES_MACOS_DIRECTORIES that is set and not empty replaces the
# list with its colon-separated directories.

macos_crash_report_directories() {
  if [ -n "${REPORT_CRASHES_MACOS_DIRECTORIES:-}" ]; then
    tr ':' '\n' <<< "$REPORT_CRASHES_MACOS_DIRECTORIES"
  else
    printf '%s\n' "$HOME/Library/Logs/DiagnosticReports" /Library/Logs/DiagnosticReports
  fi
}

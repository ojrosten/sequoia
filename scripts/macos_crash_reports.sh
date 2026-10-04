# Sourced, not run. Defines macos_crash_report_directories, which prints, one per line, each
# directory in which macOS may write a crash report.
#
# A report usually goes to the user's ~/Library/Logs/DiagnosticReports. It goes to the system's
# /Library/Logs/DiagnosticReports when the kernel kills the user's ReportCrash, idle and over its
# memory limit, as the crash arrives: the instance that takes over files the report there. Only
# the group _analyticsusers can read the system directory.
#
# REPORT_CRASHES_MACOS_DIRECTORIES, colon-separated, replaces the list.

macos_crash_report_directories() {
  if [ -n "${REPORT_CRASHES_MACOS_DIRECTORIES:-}" ]; then
    tr ':' '\n' <<< "$REPORT_CRASHES_MACOS_DIRECTORIES"
  else
    printf '%s\n' "$HOME/Library/Logs/DiagnosticReports" /Library/Logs/DiagnosticReports
  fi
}

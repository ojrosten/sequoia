# Sourced, not run. Defines windows_debugger_path, which prints the path of cdb.exe, or nothing
# where it is not installed.
#
# cdb comes with the Windows SDK's Debugging Tools, which the hosted Windows images install. cdb
# is not on PATH, so it is looked for where the SDK puts it.

windows_debugger_path() {
  local cdb
  for cdb in "/c/Program Files (x86)/Windows Kits/10/Debuggers/x64/cdb.exe" \
             "/c/Program Files/Windows Kits/10/Debuggers/x64/cdb.exe"; do
    if [ -x "$cdb" ]; then
      echo "$cdb"
      return
    fi
  done
}

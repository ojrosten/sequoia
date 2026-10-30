# Sourced, not run. Defines windows_debugger_path, which prints the path of
# cdb.exe, or nothing where cdb is not installed.
#
# cdb comes with the Windows SDK's Debugging Tools, which GitHub's hosted
# Windows images install, though not on PATH. windows_debugger_path looks for
# cdb where the SDK puts it.

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

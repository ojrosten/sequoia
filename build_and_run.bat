@echo off
rem %1 = vcvars64.bat path; builds Debug and Release, runs each in every mode
call %1 >nul || exit /b 1
cl /nologo /std:c++20 /EHsc /Od /MDd /Zi probe.cpp /Fe:probe_debug.exe >nul || exit /b 1
cl /nologo /std:c++20 /EHsc /O2 /MD probe.cpp /Fe:probe_release.exe >nul || exit /b 1
for %%b in (release debug) do (
  for %%m in ("default" "period" "period load") do (
    echo ===== %%b %%~m
    probe_%%b.exe %%~m || exit /b 1
  )
)

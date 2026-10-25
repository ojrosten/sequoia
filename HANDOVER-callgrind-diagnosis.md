# Handover: callgrind-diagnosis
Runs 40186679215 and 40186689490 (6db2008c7) finished green; both printed "Only a run on staging stores its record".
A: setup.callgrind.out is one file (pid 8169), events "Ir sysCount sysTime"; summary "311650326 1719515 701965" vs totals "311650326 833201 552889": Ir equal, sysCount/sysTime differ, and runner_setup_record.sh compares whole lines, so awk's END exits and prints nothing -> '' ''.
B: prune out files 1,207,634,818 vs 1,207,609,049 (-25,769): 92 functions differ in self cost, all malloc/free, memcmp, fs::path compare/_List; call counts of path compare and of the std::map<path,path> in resolved_paths (DependencyAnalyzer.cpp:128) differ (+126, +69): tree shape depends on insertion order = build-record order (ninja deps order from a parallel build). Inferred, not shown directly.

## Runner-setup Ir spread (311,650,326 vs 310,586,812, -1,063,514, 0.34%)
1598 functions differ, none by more than 30k (diffuse): string construct/copy, fmt sink writes, memcpy, malloc, fs::path _M_split_cmpts/_List copy. Call counts differ in 695 functions (e.g. 30M iota_view ++ vs * swapped, 508k join_view, 324k string push_back).
Input that varies: wall-clock-dependent tests. Run 2's suite text shows TestRunnerPerformanceTest failing its pool-of-eight-threads check ("attempt 3 of 3", speed-up 10.5 outside predicted [6,9]), total run time 324s vs 403s, so retries and failure reports (formatted text, paths) change the work. Not proven that this is the whole 1.06M; per-function deltas do not single out one function.
Also: both runs print "Standard Top Level Checks: 159; Failures: 1" and 2541;1, so TestFrameworkDiagnostics has a natively failing check (nonzero exit) - #380's "pass is exit 0" would drop its points.

# Handover: runner-setup-exclude-performance
Done: the callgrind step runs TestFrameworkDiagnostics --serial --exclude-performance, named in boundary and on the dashboard. Measured on callgrind-diagnosis runs 40190814463 and 40190825012: Ir 51,482,755 vs 51,530,080 (0.092%, was 0.34% at 311M), exit status 82 both (output_free_test, OutputFreeTest.cpp:525, long double template argument: probably valgrind's 64-bit x87, unverified), 98 tests.
Delete this file before merging.

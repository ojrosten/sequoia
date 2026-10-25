Branch runner-overhead-callgrind (worktree in scratchpad/seq), stacked on prune-toggle-collect.
Steps: 1 workflow (strace -> callgrind, field runner_setup) [done once committed]; 2 dashboard js + index.html; 3 selftest + mutants; 4 tsc; 5 PR, delete this file.
Unverified: no valgrind on this Mac; patterns, call counts under toggling.

# Handover: prune cachegrind series

Done: workflow steps in .github/workflows/symbol-table.yml (valgrind install, one full suite run to leave prune a stamp, cachegrind of `TestAll prune --serial`, jq merges `prune_analysis {invocation, instructions}` into the existing record before upload/store); dashboard third panel (docs/dashboard/index.html, symbol_dashboard.js: pruneInstructions, problemsWith check); selftest controls and mutants added to scripts/tests/symbol_dashboard_selftest.sh.

Next: run the selftest (needs node: a node binary is at /private/tmp/claude-501/-Users-oliver-sequoia-LLM/7aaed754-b28e-44fb-b20b-15a9ba7f2ca6/scratchpad/dashboard/node/node-v22.20.0-darwin-arm64/bin; symlinked in the 8cf4c241 scratchpad bin; PATH export does not persist between Bash calls), also `--mutations`; rebase onto origin/staging; delete this file; open PR against staging.

Verify: `scripts/tests/symbol_dashboard_selftest.sh` and `--mutations` (no SURVIVED).

Open decisions: field in the existing record rather than a sibling record; the stamping full suite run costs suite time in this job (30 min cap); `--serial` for determinism.

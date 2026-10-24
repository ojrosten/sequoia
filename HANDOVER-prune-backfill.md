# Handover: prune_analysis backfill (branch prune-analysis-backfill, off origin/staging)

Delete this file before opening the PR.

## Done
- Step 1: store_symbol_table_record.sh replaces a held record only to ADD fields (every held field, bar `run`, kept unchanged); selftest 82 controls, 26 mutants killed.

## Next
- Step 2: symbol-table.yml: measured step tees its output and writes prune_analysis.json only if the output shows the analysis ran (contains "Analyzing dependencies", and neither "unable to prune" nor "prune is ignored"); docs in the workflow header.
- Step 3: run both selftests locally (store: scripts/tests/store_symbol_table_record_selftest.sh; dashboard: node docs/dashboard/*selftest*), check workflow YAML parses.
- Step 4: delete this file, open PR against staging, message main with the branch name.

## Facts
- metrics holds 58 records; 56 lack prune_analysis; the 42 backfill records are those with run.id 39794856100. All 58 share one toolchain (g++-15 15.2.0-16ubuntu1), so a rebuild's record should equal the held one bar run.
- `prune` and `--serial` exist in TestRunner.cpp since long before 2026-09-06 (git log -S: --serial 2023-04-03). Without a stamp, prune falls back to a full run and prints a warning (old: "unable to prune"; since 7f0cef035: that or "prune is ignored").
- The dispatch must rebuild TestAll (the prune steps run it), though the symbol table is already held.

## Open decisions
- Sibling record rejected (needs dashboard join code; each sibling would carry its own toolchain). Add-only replacement chosen.

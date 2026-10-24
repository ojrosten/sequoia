# Handover: runner-overhead-dashboard (delete before the PR)

Branch `runner-overhead-dashboard` off origin/staging (bac045a31), worktree /Users/oliver/sequoia-worktrees/runner-overhead.
Task: a third report on the dashboard, runner overhead as syscall counts (strace -c -f around the existing full-suite step).
Steps: 1 workflow (symbol-table.yml) - 2 dashboard js/html + selftest + mutants + tsc - 3 PR against staging, SendMessage branch name to main.
Status: step 1 in progress.
Local checks: scripts/tests/symbol_dashboard_selftest.sh [--mutations]; tsc via npx typescript@5.9.3 -p docs/dashboard --noEmit.

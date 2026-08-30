#!/usr/bin/env python3
"""Report the machine-invariant half of a clang -ftime-trace build.

Compile time is a per-machine property, so wall-clock figures from one machine
do not transfer to another, or reliably even to the next session on the same
one. The counts clang records alongside them do transfer: given the same
sources, flags and compiler, the number of times a template is instantiated or
a constraint checked is fixed. This script leads with those and prints the
durations beside them, marked, so they are read as this machine's answer rather
than the answer.

Two modes, and the second is where an investigation ends up:

  compile_time_report.py <build-dir>
      Sums the `Total *` events over every trace in the build, ranked by count.
      One row per phase - InstantiateClass, CheckConstraintSatisfaction, and so
      on - plus a per-translation-unit breakdown.

  compile_time_report.py <build-dir> --detail <event> [--source <fragment>]
      Recompiles one translation unit at full granularity and attributes that
      event to the source locations or entities responsible. The recompile is
      needed because the standing build discards sub-500us events, which is
      every individual constraint check; it keeps the traces ~400x smaller and
      leaves the totals byte-identical, so the detail is worth paying for only
      when something in the summary needs explaining.

Compare two builds with --baseline: counts that moved are attributable to the
change, counts that did not are not, whatever the clock says.
"""
import argparse, collections, json, os, re, subprocess, sys

LOC = re.compile(r"<(?P<file>[^:<>]+):(?P<line>\d+):\d+")


def traces(build_dir):
    """Every -ftime-trace JSON under build_dir, keyed by translation unit.

    A trace counts only if the object file it was emitted beside is still
    there. CMake's own compiler checks are compiled with the project's flags
    and leave traces of their own in the build tree - `-.json` from a check
    that compiles to stdout, and `a-CMakeCXXCompilerId.json` - which are
    indistinguishable from real ones by name or content, and which inflated
    the unit count by two before this check existed. The object is the
    evidence that something in the project was actually built.
    """
    out = {}
    for root, _, files in os.walk(build_dir):
        for f in files:
            if not f.endswith(".json") or f == "compile_commands.json":
                continue
            path = os.path.join(root, f)
            if not os.path.exists(path[:-len(".json")] + ".o"):
                continue
            try:
                with open(path, encoding="utf-8") as fh:
                    d = json.load(fh)
            except (json.JSONDecodeError, UnicodeDecodeError):
                continue
            if not isinstance(d, dict) or "traceEvents" not in d:
                continue
            out[f[:-len(".json")]] = d["traceEvents"]
    return out


def totals(events):
    """The `Total <phase>` summary events, as {phase: (count, microseconds)}.

    clang emits these whatever the granularity, which is what makes the cheap
    build sufficient for everything except attribution.
    """
    out = {}
    for e in events:
        name = e.get("name", "")
        if name.startswith("Total "):
            out[name[len("Total "):]] = (e.get("args", {}).get("count", 0),
                                         e.get("dur", 0))
    return out


def self_times(events):
    """Exclusive time per event, by phase and by entity.

    A `Total X` row sums every X event, so a phase that mostly contains other
    phases reads as enormous while explaining nothing. Self time subtracts what
    a span spends inside its children, which is what separates "this is where
    the work is" from "this is what the work happened under".

    It also exposes the case that no count can: a span with a large self time
    and no children at all is the compiler working inside machinery it does not
    instrument - constraint normalisation being the one that bites here. That
    shows up as time and never as a count.
    """
    spans = [e for e in events
             if e.get("ph") == "X" and not e.get("name", "").startswith("Total ")
             and e.get("name") not in ("ExecuteCompiler", "Frontend", "Backend",
                                       "PerformPendingInstantiations", "Source")]
    spans.sort(key=lambda e: (e["ts"], -e.get("dur", 0)))
    by_phase, by_entity = collections.Counter(), collections.Counter()
    stack = []
    def close(frame):
        _, _, slf, e = frame
        by_phase[e["name"]] += slf
        by_entity[(e["name"], e.get("args", {}).get("detail", ""))] += slf
    for e in spans:
        ts, dur = e["ts"], e.get("dur", 0)
        while stack and stack[-1][0] + stack[-1][1] <= ts:
            close(stack.pop())
        if stack:
            stack[-1][2] -= dur
        stack.append([ts, dur, dur, e])
    while stack:
        close(stack.pop())
    return by_phase, by_entity


def wall(events):
    """This translation unit's own compile time."""
    for e in events:
        if e.get("name") == "Total ExecuteCompiler":
            return e.get("dur", 0)
    return 0


def summarize(build_dir, top):
    tus = traces(build_dir)
    if not tus:
        sys.exit(f"no -ftime-trace output under {build_dir}\n"
                 "configure with a *-time-trace preset and build")
    agg, per_tu = collections.Counter(), collections.Counter()
    dur = collections.Counter()
    for tu, events in tus.items():
        for phase, (count, d) in totals(events).items():
            agg[phase] += count
            dur[phase] += d
        per_tu[tu] = sum(c for c, _ in totals(events).values())

    print(f"{len(tus)} translation units\n")
    print("phase totals - counts are machine-invariant, times are this machine")
    print(f"  {'count':>12}  {'this machine':>13}  phase")
    for phase, count in agg.most_common(top):
        print(f"  {count:12d}  {dur[phase] / 1e6:11.2f}s  {phase}")
    print("\nslowest translation units - this machine, but the ranking is stable")
    for tu, d in sorted(((t, wall(e)) for t, e in tus.items()),
                        key=lambda x: -x[1])[:top]:
        print(f"  {d / 1e6:11.2f}s  {tu}")
    print("\nheaviest translation units, by counted events")
    for tu, count in per_tu.most_common(top):
        print(f"  {count:12d}  {tu}")
    print("\nNote the two rankings can disagree, and when they do the time one is\n"
          "the finding: work the compiler does not instrument has a duration and\n"
          "no count. Use --self on such a unit.")
    return agg


def compare(agg, baseline_path):
    with open(baseline_path, encoding="utf-8") as f:
        base = json.load(f)
    print(f"\nagainst baseline {baseline_path}")
    print(f"  {'baseline':>12}  {'now':>12}  {'delta':>12}  phase")
    for phase in sorted(set(base) | set(agg), key=lambda p: -abs(agg.get(p, 0) - base.get(p, 0))):
        b, n = base.get(phase, 0), agg.get(phase, 0)
        if b == n:
            continue
        ratio = f"  x{n / b:.2f}" if b else "  (new)"
        print(f"  {b:12d}  {n:12d}  {n - b:+12d}{ratio}  {phase}")


def compile_command(build_dir, fragment):
    """The ninja command line for the one object matching `fragment`."""
    objs = subprocess.run(["ninja", "-C", build_dir, "-t", "targets", "all"],
                          capture_output=True, text=True).stdout
    hits = [l.split(":")[0] for l in objs.splitlines()
            if l.endswith("o: CXX_COMPILER__" + l.split("CXX_COMPILER__")[-1])
            and fragment in l and ".o:" in l]
    if not hits:
        sys.exit(f"no object matching {fragment!r} in {build_dir}")
    if len(hits) > 1:
        sys.exit("ambiguous --source; matches:\n  " + "\n  ".join(hits))
    cmd = subprocess.run(["ninja", "-C", build_dir, "-t", "commands", hits[0]],
                         capture_output=True, text=True).stdout.strip().splitlines()[-1]
    return hits[0], cmd


def detail(build_dir, event, fragment, top, out_dir):
    obj, cmd = compile_command(build_dir, fragment)
    print(f"recompiling {obj} at full granularity", file=sys.stderr)
    probe = os.path.join(out_dir, "time_trace_probe.o")
    cmd = cmd.replace("-ftime-trace", "-ftime-trace -ftime-trace-granularity=0")
    cmd = re.sub(r"-o \S+\.o", f"-o {probe}", cmd)
    r = subprocess.run(cmd, shell=True, cwd=build_dir)
    if r.returncode:
        sys.exit(r.returncode)

    with open(probe[:-2] + ".json", encoding="utf-8") as f:
        events = json.load(f)["traceEvents"]
    hits = [e for e in events if e.get("name") == event]
    if not hits:
        names = sorted({e.get("name", "") for e in events
                        if not e.get("name", "").startswith("Total ")})
        sys.exit(f"no {event!r} events; this TU recorded:\n  " + "\n  ".join(names))

    count, dur = collections.Counter(), collections.Counter()
    for e in hits:
        d = e.get("args", {}).get("detail", "<none>")
        m = LOC.match(d)
        key = f"{m.group('file')}:{m.group('line')}" if m else d
        count[key] += 1
        dur[key] += e.get("dur", 0)

    print(f"\n{len(hits)} {event} events over {len(count)} distinct sites\n")
    print(f"  {'count':>9}  {'this machine':>13}  site")
    for key, n in count.most_common(top):
        short = key.replace(os.path.expanduser("~"), "~")
        print(f"  {n:9d}  {dur[key] / 1e3:11.1f}ms  {short}")


def show_self(build_dir, fragment, top):
    tus = traces(build_dir)
    hits = [t for t in tus if fragment in t] if fragment else list(tus)
    if not hits:
        sys.exit(f"no translation unit matching {fragment!r} in {build_dir}")
    if len(hits) > 1:
        hits = [max(hits, key=lambda t: wall(tus[t]))]
        print(f"several matched; taking the slowest, {hits[0]}\n")
    events = tus[hits[0]]
    by_phase, by_entity = self_times(events)
    print(f"{hits[0]}: {wall(events) / 1e6:.2f}s total\n")
    print("self time by phase")
    for name, d in by_phase.most_common(top):
        print(f"  {d / 1e6:9.2f}s  {name}")
    print("\nself time by entity")
    for (name, det), d in by_entity.most_common(top):
        print(f"  {d / 1e6:9.2f}s  {name}: {det[:100]}")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("build_dir")
    ap.add_argument("--top", type=int, default=20, help="rows per table (default 20)")
    ap.add_argument("--write-baseline", metavar="FILE",
                    help="write this run's phase counts, to compare a later one against")
    ap.add_argument("--baseline", metavar="FILE", help="compare against counts written earlier")
    ap.add_argument("--detail", metavar="EVENT",
                    help="attribute one event kind to source sites; needs a recompile")
    ap.add_argument("--source", default="", metavar="FRAGMENT",
                    help="which translation unit --detail should recompile")
    ap.add_argument("--self", metavar="FRAGMENT", nargs="?", const="",
                    help="exclusive time per phase and entity for one translation unit")
    ap.add_argument("--out-dir", default=None,
                    help="where --detail puts its probe (default: inside the build directory)")
    a = ap.parse_args()

    if a.self is not None:
        show_self(a.build_dir, a.self, a.top)
        return
    if a.detail:
        detail(a.build_dir, a.detail, a.source, a.top, a.out_dir or a.build_dir)
        return
    agg = summarize(a.build_dir, a.top)
    if a.baseline:
        compare(agg, a.baseline)
    if a.write_baseline:
        with open(a.write_baseline, "w", encoding="utf-8") as f:
            json.dump(dict(agg), f, indent=1, sort_keys=True)
        print(f"\nbaseline written to {a.write_baseline}")


if __name__ == "__main__":
    main()

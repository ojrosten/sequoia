#!/usr/bin/env python3
"""Report where a clang -ftime-trace build spends its compile time.

A compile time measured on one machine does not carry over to another, nor
reliably to another session on the same machine. The counts clang records
beside the times do carry over: given the same sources, flags and compiler,
the number of times a template is instantiated or a constraint is checked is
fixed. So each report leads with the counts, and marks the times as this
machine's.

  compile_time_report.py <build-dir>
      Sums each phase's `Total` event over every translation unit, and ranks
      the phases by count. Ranks the units by time and by count.

      --write-baseline writes each phase's count to a file. --baseline lists
      the phases whose counts differ from those in such a file. A count which
      moved shows that the compiler did more or less of something. A count
      which did not move does not show that the time stayed the same.

  compile_time_report.py <build-dir> --self [<fragment>]
      Ranks phases and entities by self time, a span's time less its
      children's. The unit is the slowest of those whose names contain the
      fragment. A span with a large self time and no children is the compiler
      working where it records no events, which no count shows.

  compile_time_report.py <build-dir> --detail <event> [--source <fragment>]
      Recompiles the one object matching the fragment, recording every
      event, and ranks the source lines at which the event occurs. The
      build's own traces omit events shorter than 500us, which is every
      single constraint check, so attributing one needs the recompile.
"""
import argparse, collections, json, os, re, subprocess, sys

# A source location as clang writes it in a detail: <file:line:column...>.
LOC = re.compile(r"<(?P<file>[^:<>]+):(?P<line>\d+):\d+")


def traces(build_dir):
    """The events of every trace anywhere within build_dir, keyed by the
    trace's file name less `.json`.

    A trace counts only if its object lies beside it. A compiler check run by
    CMake with -ftime-trace in CMAKE_CXX_FLAGS leaves a trace with no object,
    and the trace is otherwise indistinguishable from a unit's. A file which
    is not a readable trace is skipped.
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
    """Each phase's `Total` event, as {phase: (count, microseconds)}.

    clang writes these at any granularity, so the build's own traces suffice
    for every report but --detail's.
    """
    out = {}
    for e in events:
        name = e.get("name", "")
        if name.startswith("Total "):
            out[name[len("Total "):]] = (e.get("args", {}).get("count", 0),
                                         e.get("dur", 0))
    return out


def self_times(events):
    """Self time in microseconds, by phase and by (phase, detail).

    A span's self time is its duration less its children's. A phase's `Total`
    event sums every span of the phase, so a phase whose spans mostly enclose
    others reads as large while explaining nothing. Self time puts the time
    in the span which spent it.

    A span with a large self time and no children is the compiler working
    where it records no events, constraint normalisation among them. That
    work has a duration and no count.

    The spans of the compiler's stages enclose the rest, and their self time
    names no entity, so they are left out. So are any `Source` spans; clang
    23 writes `Source` as asynchronous events, which are not spans.
    """
    spans = [e for e in events
             if e.get("ph") == "X" and not e.get("name", "").startswith("Total ")
             and e.get("name") not in ("ExecuteCompiler", "Frontend", "Backend",
                                       "PerformPendingInstantiations", "Source")]
    spans.sort(key=lambda e: (e["ts"], -e.get("dur", 0)))
    by_phase, by_entity = collections.Counter(), collections.Counter()
    stack = []  # each frame is [start, duration, self time, event]
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
    """The unit's compile time: its `Total ExecuteCompiler` event's duration,
    or 0 if it has none."""
    for e in events:
        if e.get("name") == "Total ExecuteCompiler":
            return e.get("dur", 0)
    return 0


def summarize(build_dir, top):
    """Prints the summary of every unit, and returns each phase's count."""
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
    print("phase totals - the counts are machine-invariant, and the times are this machine's")
    print(f"  {'count':>12}  {'this machine':>13}  phase")
    for phase, count in agg.most_common(top):
        print(f"  {count:12d}  {dur[phase] / 1e6:11.2f}s  {phase}")
    print("\nslowest translation units - by this machine's times")
    for tu, d in sorted(((t, wall(e)) for t, e in tus.items()),
                        key=lambda x: -x[1])[:top]:
        print(f"  {d / 1e6:11.2f}s  {tu}")
    print("\nheaviest translation units, by counted events")
    for tu, count in per_tu.most_common(top):
        print(f"  {count:12d}  {tu}")
    print("\nWhere the two rankings disagree, the ranking by time is the finding:\n"
          "work for which clang records no events has a duration and no count.\n"
          "Run --self on such a unit.")
    return agg


def compare(agg, baseline_path):
    """Prints each phase whose count differs from the baseline's, the largest
    difference first."""
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
    """The one object whose line in ninja's list of targets contains
    `fragment`, and the command which compiles it."""
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
    """Recompiles the object matching `fragment`, recording every event, and
    prints the sites of `event`, ranked by count."""
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
    """Prints the self times of the slowest unit whose name contains
    `fragment`."""
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
                    help="write each phase's count to FILE")
    ap.add_argument("--baseline", metavar="FILE", help="list the phases whose counts differ from FILE's")
    ap.add_argument("--detail", metavar="EVENT",
                    help="rank the source lines at which EVENT occurs, recompiling one unit")
    ap.add_argument("--source", default="", metavar="FRAGMENT",
                    help="the fragment by which --detail finds the object to recompile")
    ap.add_argument("--self", metavar="FRAGMENT", nargs="?", const="",
                    help="rank phases and entities by self time, in the slowest unit "
                         "whose name contains FRAGMENT")
    ap.add_argument("--out-dir", default=None, metavar="DIR",
                    help="the directory to which --detail writes the recompiled object "
                         "and its trace (default: the build directory)")
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

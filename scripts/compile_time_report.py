#!/usr/bin/env python3
"""Report where a clang -ftime-trace build spends its compile time.

A compile time measured on one machine does not carry over to another, nor
reliably to another session on the same machine. The counts clang records
beside the times do carry over: given the same sources, flags and compiler,
the number of times a template is instantiated or a constraint is checked is
fixed. So the summary and --detail lead with the counts, and mark the times
as this machine's.

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
      Recompiles the one object whose path contains the fragment, recording
      every event, and ranks the source lines at which the event occurs. The
      build's own traces omit events shorter than 500us. Nearly every
      constraint check is that short, so attributing the checks needs the
      recompile.
"""
import argparse, collections, contextlib, json, os, re, shlex, subprocess, sys, tempfile

# A source location as clang writes it in a detail: <file:line:column...>.
LOC = re.compile(r"<(?P<file>[^:<>]+):(?P<line>\d+):\d+")


def traces(build_dir):
    """The events of every trace anywhere within build_dir, keyed by the
    trace's file name less `.json`.

    A trace counts only if its object lies beside it. A compiler check run by
    CMake with -ftime-trace in CMAKE_CXX_FLAGS leaves a trace with no object,
    and the trace is otherwise indistinguishable from a unit's.

    Exits if there are no traces. Also exits if a JSON file beside an object
    is not a readable trace, or if two traces have the same file name: either
    would drop a unit from every report.
    """
    out, paths = {}, {}
    for root, _, files in os.walk(build_dir):
        for f in files:
            path = os.path.join(root, f)
            if not f.endswith(".json") or not os.path.exists(path[:-len(".json")] + ".o"):
                continue
            unit = f[:-len(".json")]
            if unit in paths:
                sys.exit(f"two traces are named {f}:\n  {paths[unit]}\n  {path}")
            try:
                with open(path, encoding="utf-8") as fh:
                    d = json.load(fh)
            except (OSError, json.JSONDecodeError, UnicodeDecodeError) as e:
                sys.exit(f"{path} lies beside an object but is not a readable trace: {e}")
            if not isinstance(d, dict) or not isinstance(d.get("traceEvents"), list):
                sys.exit(f"{path} lies beside an object but is not a readable trace: it has no list of traceEvents")
            paths[unit] = path
            out[unit] = d["traceEvents"]
    if not out:
        sys.exit(f"no -ftime-trace output under {build_dir}\n"
                 "configure with a *-time-trace preset and build")
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
    names no entity, so they are left out.
    """
    spans = [e for e in events
             if e.get("ph") == "X" and not e.get("name", "").startswith("Total ")
             and e.get("name") not in ("ExecuteCompiler", "Frontend", "Backend",
                                       "PerformPendingInstantiations")]
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
    agg, per_tu = collections.Counter(), collections.Counter()
    dur = collections.Counter()
    for tu, events in tus.items():
        phases = totals(events)
        for phase, (count, d) in phases.items():
            agg[phase] += count
            dur[phase] += d
        per_tu[tu] = sum(c for c, _ in phases.values())

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


def ninja(build_dir, *arguments):
    """The output of `ninja -C build_dir <arguments>`. Exits if ninja fails."""
    r = subprocess.run(["ninja", "-C", build_dir, *arguments], capture_output=True, text=True)
    if r.returncode:
        sys.exit(f"ninja -C {build_dir} {' '.join(arguments)} failed:\n{r.stderr}{r.stdout}".rstrip())
    return r.stdout


def compile_command(build_dir, fragment):
    """The one object whose path contains `fragment`, among those ninja
    compiles from C++, and the command which compiles it. Exits unless exactly
    one object matches."""
    targets = [line.partition(": ") for line in ninja(build_dir, "-t", "targets", "all").splitlines()]
    hits = [target for target, _, rule in targets
            if rule.startswith("CXX_COMPILER__") and target.endswith(".o") and fragment in target]
    if not hits:
        sys.exit(f"no object matching {fragment!r} in {build_dir}")
    if len(hits) > 1:
        sys.exit("ambiguous --source; matches:\n  " + "\n  ".join(hits))
    return hits[0], ninja(build_dir, "-t", "commands", hits[0]).strip().splitlines()[-1]


def detail(build_dir, event, fragment, top, out_dir):
    """Recompiles the object matching `fragment`, recording every event, and
    prints the sites of `event`, ranked by count.

    The recompile writes its object, depfile and trace to `out_dir`, and
    nothing to the build directory. Exits, before compiling, if the command
      - has no -ftime-trace;
      - names a response file which cannot be read;
      - writes a module's BMI to a path given by -fmodule-output=, which the
        recompile would overwrite;
      - does not name its object exactly once, as `-o <path>.o`.
    Exits with the compiler's status if the recompile fails, and exits if it
    writes no trace.
    """
    obj, cmd = compile_command(build_dir, fragment)
    words = shlex.split(cmd)
    if "-ftime-trace" not in words:
        sys.exit(f"the command compiling {obj} has no -ftime-trace\n"
                 "configure with a *-time-trace preset and build")
    # CMake passes -fmodule-output= to clang in a response file, which the
    # command names as @<path>, relative to build_dir.
    for response_file in [w[1:] for w in words if w.startswith("@")]:
        try:
            with open(os.path.join(build_dir, response_file), encoding="utf-8") as f:
                words += shlex.split(f.read())
        except (OSError, UnicodeDecodeError) as e:
            sys.exit(f"the command compiling {obj} names a response file which cannot be read: {e}")
    if any(w.startswith("-fmodule-output=") for w in words):
        sys.exit(f"the command compiling {obj} writes a module's BMI, which the recompile would overwrite")

    # The command runs in build_dir, so the compiler would resolve a relative
    # out_dir against build_dir, not against this process's working directory.
    probe = os.path.join(os.path.abspath(out_dir), "time_trace_probe.o")
    trace = probe[:-len(".o")] + ".json"
    cmd = re.sub(r"(?<!\S)-ftime-trace(?!\S)", "-ftime-trace -ftime-trace-granularity=0", cmd)
    cmd, objects = re.subn(r"(?<!\S)-o \S+\.o(?!\S)", lambda _: "-o " + shlex.quote(probe), cmd)
    if objects != 1:
        sys.exit(f"the command compiling {obj} does not name its object once as -o <path>.o")
    cmd = re.sub(r"(?<!\S)-MF \S+", lambda _: "-MF " + shlex.quote(probe + ".d"), cmd)
    # A trace left by an earlier recompile must not stand in for this one's.
    for output in (probe, probe + ".d", trace):
        with contextlib.suppress(FileNotFoundError):
            os.remove(output)
    print(f"recompiling {obj} at full granularity", file=sys.stderr)
    r = subprocess.run(cmd, shell=True, cwd=build_dir)
    if r.returncode:
        sys.exit(r.returncode)
    if not os.path.exists(trace):
        sys.exit(f"the recompile of {obj} wrote no trace at {trace}")

    with open(trace, encoding="utf-8") as f:
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
    hits = [t for t in tus if fragment in t]
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
    mode = ap.add_mutually_exclusive_group()
    mode.add_argument("--detail", metavar="EVENT",
                      help="rank the source lines at which EVENT occurs, recompiling one unit")
    ap.add_argument("--source", metavar="FRAGMENT",
                    help="a fragment of the path of the object --detail recompiles")
    mode.add_argument("--self", metavar="FRAGMENT", nargs="?", const="",
                      help="rank phases and entities by self time, in the slowest unit "
                           "whose name contains FRAGMENT")
    ap.add_argument("--out-dir", metavar="DIR",
                    help="the directory, outside the build directory, to which --detail writes "
                         "the recompiled object, its depfile and its trace "
                         "(default: a temporary directory, removed afterwards)")
    a = ap.parse_args()
    if a.detail is None and (a.source is not None or a.out_dir is not None):
        ap.error("--source and --out-dir apply only to --detail")
    if (a.detail is not None or a.self is not None) and \
       (a.baseline is not None or a.write_baseline is not None):
        ap.error("--baseline and --write-baseline apply only to the summary")

    if a.self is not None:
        show_self(a.build_dir, a.self, a.top)
        return
    if a.detail is not None:
        if a.out_dir is None:
            with tempfile.TemporaryDirectory() as out_dir:
                detail(a.build_dir, a.detail, a.source or "", a.top, out_dir)
        else:
            # A trace beside its object within the build directory would be
            # read as a unit by every later summary.
            build = os.path.realpath(a.build_dir)
            if os.path.commonpath([os.path.realpath(a.out_dir), build]) == build:
                ap.error("--out-dir must be outside the build directory")
            os.makedirs(a.out_dir, exist_ok=True)
            detail(a.build_dir, a.detail, a.source or "", a.top, a.out_dir)
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

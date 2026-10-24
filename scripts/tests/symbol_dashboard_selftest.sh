#!/bin/bash
# Controls for docs/dashboard/symbol_dashboard.js, and a mutation check of the
# controls.
#
#   symbol_dashboard_selftest.sh [--mutations]
#
# Without --mutations, the selftest runs the controls. With it, the selftest
# runs the controls against the script and against each mutant of the script.
# The script must fail no control, and each mutant at least one.
#
# The controls run the script under node, which must be on PATH. They read
# the fixture in symbol_dashboard/, which mirrors what GitHub serves:
#   - tree.json, the git trees API's listing of the metrics branch;
#   - commits/<start>.json, the commits API's first page of the history from
#     the commit <start>, or from staging's head;
#   - symbol-table/<column>/<commit>.json, the records.
# Every record is a record CI stored, of 680,086 symbols, with its commit and
# counts changed. Two hold the counts of CI's symbol_table_report runs
# 39655485487 and 39655484504 unchanged: the second is that of a build with
# SEQUOIA_FORCE_INLINE removed from erased_target's constructor, and gained
# 12,982 symbols, in 56 objects. The history is ten commits, a merge of a
# topic branch among them, and staging's first parent passes through six of
# the eight records, in an order the listing does not follow. The claims:
#
#   - the records are those listed under symbol-table/<column>/, and nothing
#     else listed there; a truncated listing is an error;
#   - the points are ordered by staging's first-parent history, not by the
#     listing, and a record of a commit off that history is a problem;
#   - the history is fetched from staging's head, then from each commit at
#     which the walk leaves what is known, until every record's commit is
#     found, the walk reaches a root, or the limit of listings is reached.
#     Commits already known are not fetched, and a listing lacking the
#     commit it starts at is an error;
#   - a run of points ends where the toolchain changes, and the first point
#     of the next run names each difference. A toolchain whose keys are
#     reordered is the same toolchain;
#   - the movers between two records are every object whose count differs,
#     the largest change first; for the regression they are the 56 objects
#     an independent reading of the two records finds, in the same order;
#   - a record which cannot be fetched, names another commit, or lacks a
#     count is a problem, and the others still load;
#   - a missing metrics branch, and one holding no records, are each
#     reported as such.

set -u
here=$(cd "$(dirname "$0")" && pwd -P)
script="$here/../../docs/dashboard/symbol_dashboard.js"
fixture="$here/symbol_dashboard"
tmp=$(mktemp -d); trap 'rm -rf "$tmp"' EXIT
verbose=yes

command -v node > /dev/null || { echo "node is not on PATH"; exit 1; }

# The oracle: the regression's movers, read from the two records by Python,
# so that the controls do not check the script against itself.
python3 - "$fixture" > "$tmp/oracle.json" <<'EOF' || { echo "the oracle could not read the fixture"; exit 1; }
import json, os, sys
directory = os.path.join(sys.argv[1], 'symbol-table', 'linux-gcc-debug')
def counts(commit):
    with open(os.path.join(directory, commit + '.json'), encoding='utf-8') as f:
        return json.load(f)['defined_symbols_by_object']
before = counts('8072fccffa4c9dbd5ffe7ee50b0a2ed8aa036dba')
after  = counts('4e12898486c1307225e017a1b562ec93b54f28ef')
movers = [{'object': o, 'before': before.get(o), 'after': after.get(o), 'delta': after.get(o, 0) - before.get(o, 0)}
          for o in sorted(set(before) | set(after)) if after.get(o, 0) != before.get(o, 0)]
movers.sort(key=lambda m: -abs(m['delta']))
json.dump(movers, sys.stdout)
EOF

cat > "$tmp/controls.js" <<'EOF'
'use strict';
const fs     = require('fs');
const path   = require('path');
const assert = require('assert');
const [script, fixture, oraclePath, verbose] = process.argv.slice(2);
const dashboard = require(script);
const oracle = JSON.parse(fs.readFileSync(oraclePath, 'utf-8'));

const c = {
  R0: 'c154eb088cf2795f40599b78637521955a9bb887', F1: '1d68bd6f04e16bc552900ae56a03496818a64578',
  F2: '6219e699cb3631d5af8d256566f7d25ea90f12de', B1: 'a66349a6fa81867de91660d5ff44a3c88c799f2e',
  B2: '700628c1e857d9d4a5779960b522096bd326020f', F3: '00a9d02bda0162b540fbcf1d00b5ece04dd8d1ab', F4: '8072fccffa4c9dbd5ffe7ee50b0a2ed8aa036dba',
  F5: '4e12898486c1307225e017a1b562ec93b54f28ef', F6: '42bb757493728752cd9717784df9767ec67a2faf',
  F7: 'df6451817a6dd749a9187c7a277dc8b45290cb5b'
};
const read = file => JSON.parse(fs.readFileSync(path.join(fixture, file), 'utf-8'));
const recordOf = name => read(`symbol-table/linux-gcc-debug/${c[name]}.json`);
const listingFrom = start => read(`commits/${start || 'staging'}.json`);
const allCommits = new Map([null, c.F3, c.F1].flatMap(listingFrom).map(commit => [commit.sha, dashboard.summarise(commit)]));

// A source reading the fixture, which records each call made through it.
function fixtureSource(changes = {}) {
  const calls = [];
  return {
    calls,
    tree:    async ()           => { calls.push(['tree']); return 'tree' in changes ? changes.tree : read('tree.json'); },
    commits: async start => {
      calls.push(['commits', start]);
      return changes.commits ? changes.commits(start) : listingFrom(start);
    },
    record:  async recordPath   => {
      calls.push(['record', recordPath]);
      if (changes.record) return changes.record(recordPath);
      return read(recordPath);
    }
  };
}

let total = 0, fails = 0;
async function control(name, body) {
  ++total;
  try { await body(); }
  catch (error) { ++fails; if (verbose === 'yes') console.log(`FAIL: ${name}: ${error.message}`); }
}
const names = commits => commits.map(commit => Object.keys(c).find(key => c[key] === commit) || commit);

(async () => {
  await control('the listing yields the records under symbol-table/<column>/ alone', () => {
    const listing = dashboard.recordsInTree(read('tree.json'));
    assert.deepStrictEqual([...listing.keys()], ['linux-gcc-debug']);
    assert.deepStrictEqual(names(listing.get('linux-gcc-debug').map(entry => entry.commit)).sort(),
                           ['B1', 'F1', 'F2', 'F4', 'F5', 'F6', 'F7', 'R0']);
  });

  await control('a truncated listing is an error', () => {
    assert.throws(() => dashboard.recordsInTree({...read('tree.json'), truncated: true}), /truncated/);
  });

  await control('the walk follows first parents through a merge', () => {
    const walk = dashboard.walkFirstParents(allCommits, c.F7, new Set([c.F1, c.F4, c.B1]));
    assert.deepStrictEqual(names(walk.found), ['F4', 'F1']);
    assert.strictEqual(walk.unknown, null);
  });

  await control('the walk stops once every wanted commit is found', () => {
    const known = new Map(allCommits);
    known.delete(c.F2);
    const walk = dashboard.walkFirstParents(known, c.F7, new Set([c.F4]));
    assert.deepStrictEqual(names(walk.found), ['F4']);
    assert.strictEqual(walk.unknown, null);
  });

  await control('the walk names the commit at which it leaves what is known', () => {
    const known = new Map(allCommits);
    known.delete(c.F2);
    const walk = dashboard.walkFirstParents(known, c.F7, new Set([c.F1]));
    assert.deepStrictEqual(walk.found, []);
    assert.strictEqual(walk.unknown, c.F2);
  });

  await control('the points follow staging\'s first-parent history, not the listing', async () => {
    const loaded = await dashboard.loadHistory(fixtureSource(), new Map());
    assert.strictEqual(loaded.state, 'loaded');
    assert.strictEqual(loaded.head, c.F7);
    assert.deepStrictEqual(names(loaded.columns[0].points.map(point => point.commit)),
                           ['F1', 'F2', 'F4', 'F5', 'F6', 'F7']);
    assert.strictEqual(loaded.columns[0].points[3].subject,
                       'Merge pull request #5 from ojrosten/inline: SEQUOIA_FORCE_INLINE removed from erased_target');
    assert.strictEqual(loaded.columns[0].points[3].date, '2026-10-17T12:00:00Z');
  });

  await control('a record off the first-parent history, or naming another commit, is a problem', async () => {
    const loaded = await dashboard.loadHistory(fixtureSource(), new Map());
    const problems = new Map(loaded.columns[0].problems.map(p => [names([p.path.slice(-45, -5)])[0], p.problems]));
    assert.deepStrictEqual([...problems.keys()].sort(), ['B1', 'R0']);
    assert.deepStrictEqual(problems.get('B1'), ['it is not on staging\'s first-parent history']);
    assert.deepStrictEqual(problems.get('R0'), [`it names commit ${c.F1}`]);
  });

  await control('history is fetched from each commit at which the walk leaves what is known', async () => {
    const source = fixtureSource();
    const loaded = await dashboard.loadHistory(source, new Map());
    assert.deepStrictEqual(source.calls.filter(call => call[0] === 'commits'),
                           [['commits', null], ['commits', c.F3], ['commits', c.F1]]);
    assert.strictEqual(loaded.pages, 3);
    assert.strictEqual(source.calls.filter(call => call[0] === 'tree').length, 1);
  });

  await control('commits already known are not fetched', async () => {
    const source = fixtureSource();
    await dashboard.loadHistory(source, new Map(allCommits));
    assert.deepStrictEqual(source.calls.filter(call => call[0] === 'commits'), [['commits', null]]);
  });

  await control('the commits fetched become known', async () => {
    const known = new Map();
    await dashboard.loadHistory(fixtureSource(), known);
    assert.deepStrictEqual(names([...known.keys()]).sort(), ['B1', 'B2', 'F1', 'F2', 'F3', 'F4', 'F5', 'F6', 'F7', 'R0']);
  });

  await control('a listing lacking the commit it starts at is an error', async () => {
    const source = fixtureSource({commits: start => start === null ? listingFrom(null) : listingFrom(c.F1)});
    await assert.rejects(dashboard.loadHistory(source, new Map()), new RegExp(`listing of the history from ${c.F3} lacks that commit`));
  });

  await control('the walk fetches the history from where the known commits end', async () => {
    const known = new Map(allCommits);
    known.delete(c.F1);
    const source = fixtureSource();
    const loaded = await dashboard.loadHistory(source, known);
    assert.deepStrictEqual(source.calls.filter(call => call[0] === 'commits'), [['commits', null], ['commits', c.F1]]);
    assert.deepStrictEqual(names(loaded.columns[0].points.map(point => point.commit)), ['F1', 'F2', 'F4', 'F5', 'F6', 'F7']);
  });

  await control('the walk fetches no more than the limit of pages', async () => {
    // An endless history in which no record's commit lies.
    const sha = n => n.toString(16).padStart(40, '0');
    const listing = start => {
      const from = start === null ? 1 : parseInt(start, 16);
      return [from, from + 1].map(k => ({sha: sha(k), parents: [{sha: sha(k + 1)}],
                                         commit: {committer: {date: '2026-01-01T00:00:00Z'}, message: 'm'}}));
    };
    let listings = 0;
    const source = fixtureSource({commits: start => {
      if (++listings > dashboard.maxCommitPages + 5) throw new Error('the walk went on past the limit of listings');
      return listing(start);
    }});
    const loaded = await dashboard.loadHistory(source, new Map());
    assert.strictEqual(source.calls.filter(call => call[0] === 'commits').length, dashboard.maxCommitPages);
    assert.strictEqual(loaded.pages, dashboard.maxCommitPages);
    assert.strictEqual(loaded.columns[0].points.length, 0);
    assert.ok(loaded.columns[0].problems[0].problems.includes(
      `the walk along staging's first parents stopped after ${dashboard.maxCommitPages} pages of history, before reaching it`));
  });

  await control('a run ends where the toolchain changes, and its successor names the change', async () => {
    const points = (await dashboard.loadHistory(fixtureSource(), new Map())).columns[0].points;
    assert.deepStrictEqual(points.map(point => point.run), [0, 0, 1, 1, 1, 1]);
    assert.deepStrictEqual(points[2].toolchainChange,
                           [{path: 'compiler.package.version', before: '15.2.0-14ubuntu1', after: '15.2.0-16ubuntu1'}]);
    assert.deepStrictEqual(points.filter((point, i) => i !== 2).map(point => point.toolchainChange),
                           [null, null, null, null, null]);
  });

  await control('a toolchain with its keys reordered is the same toolchain', () => {
    const before = recordOf('F4');
    const after  = recordOf('F6');
    after.toolchain = Object.fromEntries(Object.entries(after.toolchain).reverse());
    const summary = {date: 'd', subject: 's'};
    const points = dashboard.pointsOf([{commit: c.F4, summary, record: before}, {commit: c.F6, summary, record: after}]);
    assert.deepStrictEqual(points.map(point => point.run), [0, 0]);
  });

  await control('a leaf absent from one toolchain is a difference', () => {
    assert.deepStrictEqual(dashboard.differences({a: {b: 1}}, {a: {}}), [{path: 'a.b', before: 1, after: undefined}]);
  });

  await control('the regression\'s movers are those the records show, in order', () => {
    const movers = dashboard.movers(recordOf('F4'), recordOf('F5'));
    assert.strictEqual(movers.length, 56);
    assert.deepStrictEqual(movers, oracle);
    assert.deepStrictEqual(movers[0], {object: 'Tests/Physics/AbsolutePhysicalValueTest.cpp',
                                       before: movers[0].before, after: movers[0].before + 795, delta: 795});
  });

  await control('records with equal counts have no movers', () => {
    assert.deepStrictEqual(dashboard.movers(recordOf('F4'), recordOf('F6')), []);
  });

  await control('an object in one record alone moves by its whole count', () => {
    const movers = dashboard.movers({defined_symbols_by_object: {a: 1}}, {defined_symbols_by_object: {b: 2}});
    assert.deepStrictEqual(movers, [{object: 'b', before: null, after: 2, delta: 2},
                                    {object: 'a', before: 1, after: null, delta: -1}]);
  });

  await control('movers of equal change are ordered by path', () => {
    const movers = dashboard.movers({defined_symbols_by_object: {b: 1, a: 3, c: 1}},
                                    {defined_symbols_by_object: {b: 2, a: 2, c: 0}});
    assert.deepStrictEqual(movers.map(mover => mover.object), ['a', 'b', 'c']);
  });

  await control('a record which is not an object is a problem, and the others load', async () => {
    const source = fixtureSource({record: recordPath => recordPath.includes(c.F6) ? null : read(recordPath)});
    const column = (await dashboard.loadHistory(source, new Map())).columns[0];
    assert.deepStrictEqual(column.problems.find(p => p.path.includes(c.F6)).problems, ['it is not a JSON object']);
    assert.strictEqual(column.points.length, 5);
  });

  await control('objects named as the prototype\'s properties move as any other', () => {
    const movers = dashboard.movers({defined_symbols_by_object: {}}, {defined_symbols_by_object: {constructor: 3}});
    assert.deepStrictEqual(movers, [{object: 'constructor', before: null, after: 3, delta: 3}]);
  });

  await control('a stored record has no problem', () => {
    assert.deepStrictEqual(dashboard.problemsWith(recordOf('F5'), c.F5), []);
  });

  for (const [name, change, problem] of [
    ['no toolchain',              r => { delete r.toolchain; },                       'it has no toolchain'],
    ['a tree not clean',          r => { r.clean = false; },                          'it was not measured on a clean tree'],
    ['no count of symbols',       r => { delete r.executable.symbols; },              'it has no count of the executable\'s symbols'],
    ['a fractional count',        r => { r.executable.symbols = 1.5; },               'it has no count of the executable\'s symbols'],
    ['a negative count of bytes', r => { r.executable.symbol_name_bytes = -1; },      'it has no count of the executable\'s name bytes'],
    ['no executable',             r => { delete r.executable; },                      'it has no count of the executable\'s symbols'],
    ['no objects',                r => { delete r.defined_symbols_by_object; },       'it has no count of defined symbols for each object'],
    ['an object without a count', r => { r.defined_symbols_by_object.x = '1'; },      'it has no count of defined symbols for each object']
  ]) {
    await control(`a record with ${name} is a problem`, () => {
      const record = recordOf('F5');
      change(record);
      assert.ok(dashboard.problemsWith(record, c.F5).includes(problem));
    });
  }

  await control('prune\'s instructions are null in a record without them, and the count in one with them', () => {
    const record = recordOf('F5');
    assert.strictEqual(dashboard.pruneInstructions(record), null);
    record.prune_analysis = {invocation: 'x', instructions: 0};
    assert.strictEqual(dashboard.pruneInstructions(record), 0);
    record.prune_analysis.instructions = 123;
    assert.strictEqual(dashboard.pruneInstructions(record), 123);
    assert.deepStrictEqual(dashboard.problemsWith(record, c.F5), []);
  });

  for (const [name, analysis] of [
    ['no count',           {invocation: 'x'}],
    ['a fractional count', {invocation: 'x', instructions: 1.5}],
    ['a text count',       {invocation: 'x', instructions: '5'}],
    ['no invocation',      {instructions: 5}],
    ['null',               null]
  ]) {
    await control(`a record with a prune_analysis of ${name} is a problem`, () => {
      const record = recordOf('F5');
      record.prune_analysis = analysis;
      assert.deepStrictEqual(dashboard.problemsWith(record, c.F5),
                             ['it has a prune_analysis with no invocation, or no count of instructions']);
    });
  }

  await control('points lacking the count keep their place, and the count is read where present', async () => {
    const source = fixtureSource({record: recordPath => {
      const record = read(recordPath);
      if (recordPath.includes(c.F5)) record.prune_analysis = {invocation: 'x', instructions: 777};
      return record;
    }});
    const column = (await dashboard.loadHistory(source, new Map())).columns[0];
    const counts = column.points.map(point => dashboard.pruneInstructions(point.record));
    assert.deepStrictEqual(counts, column.points.map(point => point.commit === c.F5 ? 777 : null));
    assert.ok(counts.includes(777));
  });

  const overheadOf = tests => ({invocation: 'x', exit_status: 0, tests,
                                 syscalls: {directory_walks: 10, metadata_reads: 400, opens: 60, clones: 2, execs: 8}});

  const perTest = {directory_walks: 2.5, metadata_reads: 100, opens: 15, clones: 0.5, execs: 2};

  await control('each category is null in a record without the runner\'s overhead or without a number of tests, and per test otherwise', () => {
    const record = recordOf('F5');
    for (const category of dashboard.runnerSyscallCategories) {
      assert.strictEqual(dashboard.runnerSyscallPerTest(record, category), null);
    }
    record.runner_overhead = overheadOf(null);
    for (const category of dashboard.runnerSyscallCategories) {
      assert.strictEqual(dashboard.runnerSyscallPerTest(record, category), null);
    }
    assert.deepStrictEqual(dashboard.problemsWith(record, c.F5), []);
    record.runner_overhead = overheadOf(4);
    for (const [category, count] of Object.entries(perTest)) {
      assert.strictEqual(dashboard.runnerSyscallPerTest(record, category), count, category);
    }
    assert.deepStrictEqual(dashboard.problemsWith(record, c.F5), []);
  });

  await control('a count of zero is a count, plotted as zero', () => {
    const record = recordOf('F5');
    record.runner_overhead = overheadOf(5);
    for (const category of dashboard.runnerSyscallCategories) {
      record.runner_overhead.syscalls[category] = 0;
      assert.strictEqual(dashboard.runnerSyscallPerTest(record, category), 0);
    }
    assert.deepStrictEqual(dashboard.problemsWith(record, c.F5), []);
  });

  await control('the categories are the five the workflow records', () => {
    assert.deepStrictEqual(dashboard.runnerSyscallCategories,
                           ['directory_walks', 'metadata_reads', 'opens', 'clones', 'execs']);
  });

  for (const [name, overhead, problem] of [
    ['null',            null,                                     'it has a runner_overhead with no invocation'],
    ['no invocation',   {...overheadOf(4), invocation: 5},        'it has a runner_overhead with no invocation'],
    ['zero tests',      overheadOf(0),                            'it has a runner_overhead whose number of tests is neither null nor a positive integer'],
    ['fractional tests', overheadOf(1.5),                         'it has a runner_overhead whose number of tests is neither null nor a positive integer'],
    ['text tests',      overheadOf('4'),                          'it has a runner_overhead whose number of tests is neither null nor a positive integer'],
    ['absent tests',    (({tests, ...rest}) => rest)(overheadOf(4)), 'it has a runner_overhead whose number of tests is neither null nor a positive integer'],
    ['no syscalls',     {...overheadOf(4), syscalls: null},       'it has a runner_overhead with no count of directory_walks']
  ]) {
    await control(`a record with a runner_overhead of ${name} is a problem`, () => {
      const record = recordOf('F5');
      record.runner_overhead = overhead;
      assert.ok(dashboard.problemsWith(record, c.F5).includes(problem), dashboard.problemsWith(record, c.F5).join('; '));
    });
  }

  for (const category of ['directory_walks', 'metadata_reads', 'opens', 'clones', 'execs']) {
    for (const [name, value] of [['missing', undefined], ['fractional', 1.5], ['negative', -1], ['text', '3']]) {
      await control(`a runner_overhead with ${category} ${name} is a problem naming it, and no other`, () => {
        const record = recordOf('F5');
        record.runner_overhead = overheadOf(4);
        if (value === undefined) delete record.runner_overhead.syscalls[category];
        else record.runner_overhead.syscalls[category] = value;
        assert.deepStrictEqual(dashboard.problemsWith(record, c.F5), [`it has a runner_overhead with no count of ${category}`]);
      });
    }
  }

  await control('points lacking the runner\'s overhead keep their place, and it is read where present', async () => {
    const source = fixtureSource({record: recordPath => {
      const record = read(recordPath);
      if (recordPath.includes(c.F5)) record.runner_overhead = overheadOf(2);
      return record;
    }});
    const column = (await dashboard.loadHistory(source, new Map())).columns[0];
    const counts = column.points.map(point => dashboard.runnerSyscallPerTest(point.record, 'metadata_reads'));
    assert.deepStrictEqual(counts, column.points.map(point => point.commit === c.F5 ? 200 : null));
    assert.ok(counts.includes(200));
  });

  const treeWith = (...commits) => ({...read('tree.json'), tree: read('tree.json').tree.concat(commits.map(commit =>
    ({path: `uncalled-functions/linux-gcc/${commit}.json`, type: 'blob'})))});
  // The fixture's own records have problems of their own, which these controls leave aside
  const uncalledProblems = column => column.problems.filter(entry => entry.path.startsWith('uncalled-functions/'));
  const uncalledRecord = (name, count, tools = ['g++-15 15.2.0']) =>
    ({commit: c[name], clean: true, uncalled_functions: count, toolchain: {demanglers: ['c++filt 2.46'], tools}});

  await control('a record of uncalled functions without a problem has none, and with one has it named', () => {
    assert.deepStrictEqual(dashboard.problemsWithUncalled(uncalledRecord('F5', 0), c.F5), []);
    assert.deepStrictEqual(dashboard.problemsWithUncalled(uncalledRecord('F5', 5), c.F4), [`it names commit ${c.F5}`]);
  });

  for (const [name, change, problem] of [
    ['a tree not clean',         r => { r.clean = false; },                     'it was not measured on a clean tree'],
    ['no count',                 r => { delete r.uncalled_functions; },         'it has no count of uncalled functions'],
    ['a fractional count',       r => { r.uncalled_functions = 1.5; },          'it has no count of uncalled functions'],
    ['a text count',             r => { r.uncalled_functions = '5'; },          'it has no count of uncalled functions'],
    ['a negative count',         r => { r.uncalled_functions = -1; },           'it has no count of uncalled functions'],
    ['no toolchain',             r => { delete r.toolchain; },                  'it has no toolchain of demanglers and tools'],
    ['tools of numbers',         r => { r.toolchain.tools = [1]; },             'it has no toolchain of demanglers and tools'],
    ['no demanglers',            r => { delete r.toolchain.demanglers; },       'it has no toolchain of demanglers and tools']
  ]) {
    await control(`a record of uncalled functions with ${name} is a problem`, () => {
      const record = uncalledRecord('F5', 5);
      change(record);
      assert.ok(dashboard.problemsWithUncalled(record, c.F5).includes(problem));
    });
  }

  await control('the nightly counts join the points of their commits, with their own toolchain runs', async () => {
    const records = new Map([[c.F2, uncalledRecord('F2', 7)], [c.F4, uncalledRecord('F4', 6)],
                             [c.F6, uncalledRecord('F6', 6, ['g++-16 16.1.0'])]]);
    const source = fixtureSource({
      tree: treeWith(...records.keys()),
      record: recordPath => {
        const found = [...records].find(([commit]) => recordPath === `uncalled-functions/linux-gcc/${commit}.json`);
        return found ? found[1] : read(recordPath);
      }
    });
    const column = (await dashboard.loadHistory(source, new Map())).columns[0];
    assert.deepStrictEqual(uncalledProblems(column), []);
    assert.deepStrictEqual(column.points.map(point => dashboard.uncalledCount(point.record)),
                           column.points.map(point => records.has(point.commit) ? records.get(point.commit).uncalled_functions : null));
    const [first, second, third] = column.points.filter(point => records.has(point.commit)).map(point => point.record.uncalled);
    assert.deepStrictEqual([first.toolchainChange, second.toolchainChange, first.run, second.run, third.run],
                           [null, null, 0, 0, 1]);
    assert.deepStrictEqual(third.toolchainChange.map(change => change.path), ['tools']);
  });

  await control('a record of uncalled functions with a problem is listed, and the others load', async () => {
    const source = fixtureSource({
      tree: treeWith(c.F2),
      record: recordPath => recordPath.startsWith('uncalled-functions/') ? {...uncalledRecord('F2', 1), clean: false} : read(recordPath)
    });
    const column = (await dashboard.loadHistory(source, new Map())).columns[0];
    assert.deepStrictEqual(uncalledProblems(column).map(entry => entry.path), [`uncalled-functions/linux-gcc/${c.F2}.json`]);
    assert.strictEqual(column.points.filter(point => dashboard.uncalledCount(point.record) !== null).length, 0);
  });

  await control('a record is null as a count where no measurement is, never zero', () => {
    assert.strictEqual(dashboard.uncalledCount(recordOf('F5')), null);
  });

  await control('a record of uncalled functions at no plotted commit is listed', async () => {
    const absent = '0'.repeat(40);
    const source = fixtureSource({
      tree: treeWith(absent),
      record: recordPath => recordPath.startsWith('uncalled-functions/') ? {...uncalledRecord('F2', 1), commit: absent} : read(recordPath)
    });
    const column = (await dashboard.loadHistory(source, new Map())).columns[0];
    assert.strictEqual(uncalledProblems(column).length, 1);
    assert.ok(uncalledProblems(column)[0].problems.length >= 1);
  });

  await control('a record which cannot be fetched is a problem, and the others load', async () => {
    const source = fixtureSource({record: recordPath => {
      if (recordPath.includes(c.F6)) throw new Error('404 for the record');
      return read(recordPath);
    }});
    const column = (await dashboard.loadHistory(source, new Map())).columns[0];
    assert.deepStrictEqual(names(column.points.map(point => point.commit)), ['F1', 'F2', 'F4', 'F5', 'F7']);
    assert.deepStrictEqual(column.problems.find(p => p.path.includes(c.F6)).problems,
                           ['it could not be fetched: 404 for the record']);
  });

  const day = 24 * 60 * 60 * 1000, t0 = Date.parse('2026-10-20T00:00:00Z');
  await control('points of one day are close, and days without a point leave a gap', () => {
    const {xs} = dashboard.timeAxis([t0, t0 + 3600e3, t0 + 6 * day], 0, 700, 80);
    assert.ok(xs[1] - xs[0] < 10);
    assert.ok(Math.abs((xs[2] - xs[1]) - 6 * 100) < 10);
    assert.ok(xs[0] >= 0 && xs[2] <= 700);
  });

  await control('a date is ticked once, at a midnight, with ticks at least the gap apart', () => {
    for (const span of [0, 1, 5, 40, 400]) {
      const times = [t0, t0 + 17 * 3600e3, t0 + span * day];
      const {ticks} = dashboard.timeAxis(times, 0, 600, 80);
      const labels = ticks.map(tick => new Date(tick.time).toISOString().slice(0, 10));
      assert.strictEqual(new Set(labels).size, labels.length);
      assert.ok(ticks.every(tick => tick.time % day === 0));
      assert.ok(ticks.every((tick, i) => i === 0 || tick.x - ticks[i - 1].x >= 80 - 1e-9));
    }
  });

  await control('points all on one day are spread over the axis in time order', () => {
    const {xs} = dashboard.timeAxis([t0 + 12 * 3600e3, t0 + 6 * 3600e3, t0 + 18 * 3600e3], 0, 600, 80);
    assert.deepStrictEqual(xs.map(Math.round), [300, 150, 450]);
  });

  await control('the smallest gap is that of neighbours in x, whatever the order of the points', () => {
    assert.strictEqual(dashboard.smallestGap([30, 0, 10, 100]), 10);
    assert.strictEqual(dashboard.smallestGap([5, 5]), 0);
    assert.strictEqual(dashboard.smallestGap([5]), Infinity);
  });

  await control('a commit with a date which is not a date is refused', () => {
    const commit = date => ({sha: c.F1, parents: [], commit: {committer: {date}, message: 'm'}});
    assert.strictEqual(dashboard.summarise(commit('2026-10-17T12:00:00Z')).date, '2026-10-17T12:00:00Z');
    assert.throws(() => dashboard.summarise(commit('yesterday')), /not a date/);
  });

  await control('a single point is placed on the axis', () => {
    const {xs, ticks} = dashboard.timeAxis([t0 + 5 * 3600e3], 0, 600, 80);
    assert.deepStrictEqual([xs.length, ticks.length], [1, 1]);
    assert.ok(Number.isFinite(xs[0]));
  });

  await control('hover picks the point nearest in x, the earliest of equals', () => {
    const xs = [10, 12, 100];
    assert.deepStrictEqual([0, 10, 11.5, 50, 60, 1000].map(offset => dashboard.nearestPoint(xs, offset)), [0, 0, 1, 1, 2, 2]);
    assert.strictEqual(dashboard.nearestPoint([5, 5], 5), 0);
  });

  await control('a missing metrics branch is reported as absent', async () => {
    const source = fixtureSource({tree: null});
    assert.deepStrictEqual(await dashboard.loadHistory(source, new Map()), {state: 'absent'});
  });

  await control('a metrics branch without records is reported as empty', async () => {
    const tree = read('tree.json');
    tree.tree = tree.tree.filter(entry => !entry.path.endsWith('.json'));
    assert.ok(tree.tree.length > 0);
    assert.deepStrictEqual(await dashboard.loadHistory(fixtureSource({tree}), new Map()), {state: 'empty'});
  });

  console.log(`${total} ${fails}`);
})();
EOF

# run_controls <script>: the controls' count in $total, and their failures in
# $fails. A run which does not finish within a minute fails.
run_controls() {
  local summary
  summary=$(perl -e 'alarm 60; exec @ARGV' node "$tmp/controls.js" "$1" "$fixture" "$tmp/oracle.json" "$verbose" 2>&1)
  if [[ $verbose == yes ]]; then sed '$d' <<< "$summary"; fi
  read -r total fails <<< "$(tail -n 1 <<< "$summary")"
  if ! [[ $total =~ ^[0-9]+$ && $fails =~ ^[0-9]+$ ]]; then
    echo "the controls did not report a count${summary:+: $summary}"
    total=1 fails=1
  fi
}

# mutants <directory>: writes each mutant of the script to <directory>/<n>/,
# and prints a line for each: n, how often its text occurs, its description
mutants() {
  python3 - "$script" "$1" <<'EOF'
import os, sys
script, directory = sys.argv[1], sys.argv[2]
MUTATIONS = [
    ('a truncated listing accepted',      'if (tree.truncated) {', 'if (false) {'),
    ('any path listed as a record',       'const match = recordPathPattern.exec(entry.path);',
                                          'const match = /^(.*)\\/([^/]*)\\.json$/.exec(entry.path);'),
    ('the last parent followed',          'commit = summary.parents[0];', 'commit = summary.parents[summary.parents.length - 1];'),
    ('the walk stops at the first find',  'while (found.length < wanted.size) {', 'while (found.length < 1) {'),
    ('an unknown commit ends the walk',   'if (!summary) return {found, unknown: commit};', 'if (!summary) return {found, unknown: null};'),
    ('the points in listing order',       'const historyOrder = walk.found.slice().reverse();', 'const historyOrder = [...wanted];'),
    ('the points newest first',           'const historyOrder = walk.found.slice().reverse();', 'const historyOrder = walk.found.slice();'),
    ('records off the history plotted',   'if (!walk.found.includes(entry.commit)) entryProblems.push(notFound);', ''),
    ('no limit of listings',              'while (walk.unknown && pages < maxCommitPages) {', 'while (walk.unknown) {'),
    ('a listing at the limit too many',   'while (walk.unknown && pages < maxCommitPages) {', 'while (walk.unknown && pages <= maxCommitPages) {'),
    ('later listings from staging',       'learn(await source.commits(walk.unknown));', 'learn(await source.commits(null));'),
    ('a listing lacking its start kept',  'if (!known.has(walk.unknown)) throw', 'if (false) throw'),
    ('the known commits ignored',         'let walk  = walkFirstParents(known, head, wanted);',
                                          'let walk  = {unknown: head};'),
    ('fetched commits not kept',          'for (const commit of listing) known.set(commit.sha, summarise(commit));', ''),
    ('toolchains compared as written',    'canonical(previous.record.toolchain) !== canonical(record.toolchain)',
                                          'JSON.stringify(previous.record.toolchain) !== JSON.stringify(record.toolchain)'),
    ('keys left unsorted',                'Object.keys(value).sort().map(', 'Object.keys(value).map('),
    ('no break at a toolchain change',    'if (toolchainChange) ++run;', ''),
    ('absent leaves ignored',             "return canonical(before) === canonical(after) ? [] : [{path: prefix, before, after}];",
                                          "return before === undefined || after === undefined || canonical(before) === canonical(after) ? [] : [{path: prefix, before, after}];"),
    ('movers by signed change',           'Math.abs(b.delta) - Math.abs(a.delta)', 'b.delta - a.delta'),
    ('ties unordered',                    '|| (a.object < b.object ? -1 : a.object > b.object ? 1 : 0)', ''),
    ('objects of the later record alone', 'new Set([...Object.keys(countsBefore), ...Object.keys(countsAfter)])',
                                          'new Set(Object.keys(countsAfter))'),
    ('unmoved objects kept',              '.filter(mover => mover.delta !== 0)', ''),
    ('the commit not checked',            "if (record.commit !== commit) problems.push", "if (false) problems.push"),
    ('a record not an object read',       "if (!isObject(record)) return ['it is not a JSON object'];", "if (false) return [];"),
    ('a dirty tree accepted',             "if (record.clean !== true) problems.push", "if (false) problems.push"),
    ('prototype keys counted',            'Object.hasOwn(countsBefore, object)', 'object in countsBefore'),
    ('any toolchain accepted',            "if (record.toolchain === null || typeof record.toolchain !== 'object')", 'if (false)'),
    ('fractions accepted',                'Number.isInteger(value) && value >= 0', "typeof value === 'number' && value >= 0"),
    ('negative counts accepted',          'Number.isInteger(value) && value >= 0', 'Number.isInteger(value)'),
    ('bytes not checked',                 "if (!isCount(executable.symbol_name_bytes)) problems.push", "if (false) problems.push"),
    ('objects not checked',               '!Object.values(objects).every(isCount)', 'false'),
    ('a failed fetch fails the load',     'catch (error) { outcomes[index] = {error}; }', 'catch (error) { throw error; }'),
    ('an absent branch read as empty',    "if (tree === null) return {state: 'absent'};", "if (tree === null) return {state: 'empty'};"),
    ('a missing count plotted as zero',  "hasOwn(record, 'prune_analysis') ? record.prune_analysis.instructions : null", "hasOwn(record, 'prune_analysis') ? record.prune_analysis.instructions : 0"),
    ('the analysis not checked',          "if (Object.hasOwn(record, 'prune_analysis')) {", 'if (false) {'),
    ('the count not checked',             '|| !isCount(analysis.instructions)', ''),
    ('the invocation not checked',        "|| typeof analysis.invocation !== 'string'", ''),
    ('a missing overhead plotted as zero',  "if (!Object.hasOwn(record, 'runner_overhead') || record.runner_overhead.tests === null) return null;", "if (!Object.hasOwn(record, 'runner_overhead')) return null;"),
    ('a missing number of tests divided by', "|| record.runner_overhead.tests === null) return null;", ") return null;"),
    ('the overhead not checked',            "if (Object.hasOwn(record, 'runner_overhead')) {", 'if (false) {'),
    ('the overhead\'s invocation not checked', "|| typeof overhead.invocation !== 'string') {", ') {'),
    ('the number of tests not checked',     "if (overhead.tests !== null && !(isCount(overhead.tests) && overhead.tests > 0)) {", 'if (false) {'),
    ('zero tests accepted',                 "isCount(overhead.tests) && overhead.tests > 0", 'isCount(overhead.tests)'),
    ('the directory walks unchecked',       "const runnerSyscallCategories = ['directory_walks', ", "const runnerSyscallCategories = ["),
    ('the metadata reads unchecked',        "'metadata_reads', 'opens'", "'opens'"),
    ('the opens unchecked',                 "'opens', 'clones'", "'clones'"),
    ('the clones unchecked',                "'clones', 'execs']", "'execs']"),
    ('the execs unchecked',                 "'clones', 'execs']", "'clones']"),
    ('the counts not checked at all',       "|| !isCount(syscalls[category])", ''),
    ('totals plotted, not per test',        "return syscalls[category] / tests;", "return syscalls[category];"),
    ('the wrong category read',             "return syscalls[category] / tests;", "return syscalls.opens / tests;"),
    ('a missing uncalled count plotted as zero',    "Object.hasOwn(record, 'uncalled') ? record.uncalled.count : null", "Object.hasOwn(record, 'uncalled') ? record.uncalled.count : 0"),
    ('the uncalled count not checked',      '|| record.uncalled_functions < 0', '|| false'),
    ('the uncalled toolchain not checked',  '|| !isNames(toolchain.tools)', ''),
    ('the toolchain change dropped',       'toolchainChange: found.toolchainChange}', 'toolchainChange: null}'),
    ('the run of the symbol table used',   'run: found.run,', 'run: point.run,'),
    ('x by index, not time',             'return {xs: times.map(x), ticks};', 'return {xs: times.map((time, i) => left + (i + 0.5) * (right - left) / times.length), ticks};'),
    ('the last nearest point taken',     'if (Math.abs(x - offset) < Math.abs(xs[nearest] - offset))', 'if (Math.abs(x - offset) <= Math.abs(xs[nearest] - offset))'),
    ('the farthest point taken',         'if (Math.abs(x - offset) < Math.abs(xs[nearest] - offset))', 'if (Math.abs(x - offset) > Math.abs(xs[nearest] - offset))'),
    ('ticks at every day',               'candidate * (right - left) / days >= minGap) ??', 'true) ??'),
    ('ticks at a fixed step in points',  'for (let day = first; day <= last; day += step)', 'for (let day = first; day <= last; day += 1)'),
    ('the smallest gap taken unsorted',  'const sorted = xs.slice().sort((a, b) => a - b);', 'const sorted = xs.slice();'),
    ('an unreadable date accepted',      'if (!Number.isFinite(Date.parse(commit.commit.committer.date))) {', 'if (false) {'),
    ('an empty branch loaded',            "if (listing.size === 0) return {state: 'empty'};", ''),
]
with open(script, encoding='utf-8') as f:
    source = f.read()
for n, (description, old, new) in enumerate(MUTATIONS):
    os.makedirs(os.path.join(directory, str(n)))
    with open(os.path.join(directory, str(n), 'symbol_dashboard.js'), 'w', encoding='utf-8') as f:
        f.write(source.replace(old, new))
    print(n, source.count(old), description)
EOF
}

if [[ ${1:-} == --mutations ]]; then
  verbose=no survivors=0
  run_controls "$script"
  echo "unmutated: $fails of $total controls fail$( ((fails)) && echo '  <-- must be none')"
  if ((fails)); then survivors=1; fi
  mutants "$tmp/mutants" > "$tmp/mutants.txt" || exit 1
  while read -r n occurrences description; do
    if [[ $occurrences -ne 1 ]]; then
      echo "$description: the text to mutate occurs $occurrences times"; survivors=1; continue
    fi
    run_controls "$tmp/mutants/$n/symbol_dashboard.js"
    echo "$description: $fails of $total controls fail$( ((fails)) || echo '  <-- SURVIVED')"
    if ((fails == 0)); then survivors=1; fi
  done < "$tmp/mutants.txt"
  exit $survivors
fi

run_controls "$script"
if [[ $fails -eq 0 ]]; then echo "symbol_dashboard selftest: all $total controls pass"; else
  echo "symbol_dashboard selftest: $fails of $total controls failed"; fi
exit $((fails > 0))

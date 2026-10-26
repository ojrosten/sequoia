// The data behind the symbol dashboard: which records the metrics branch
// holds, their order along staging's first-parent history, where the
// toolchain changes, and which objects moved between two records. Nothing
// here touches the page, so the selftest runs it under Node.
//
// A record is the output of scripts/symbol_table_report.py, stored as
// symbol-table/<column>/<commit>.json on the metrics branch.
/** @param {Record<string, any>} exports */
(function (exports) {
  'use strict';

  /**
   * A record as the metrics branch holds it: JSON read from outside, whose
   * fields problemsWith and problemsWithUncalled check before any is used.
   * @typedef {any} JsonRecord
   */
  /**
   * @typedef {{path: string}} TreeEntry
   * @typedef {{tree: TreeEntry[], truncated?: boolean}} Tree
   * @typedef {{commit: string, path: string}} Entry
   * @typedef {{sha: string, parents: {sha: string}[], commit: {committer: {date: string}, message: string}}} ApiCommit
   * @typedef {{parents: string[], date: string, subject: string}} Summary
   * @typedef {{commit: string, summary: Summary, record: JsonRecord}} OrderedRecord
   * @typedef {{path: string, before: any, after: any}} Difference
   * @typedef {{commit: string, date: string, subject: string, record: JsonRecord, run: number,
   *            toolchainChange: Difference[] | null, setupRun: number, boundaryChange: boolean}} Point
   * @typedef {{path: string, problems: string[]}} Unplottable
   * @typedef {{tree(): Promise<Tree | null>, commits(start: string | null): Promise<ApiCommit[]>,
   *            record(path: string): Promise<JsonRecord>}} Source
   */

  const recordPathPattern = /^symbol-table\/([^/]+)\/([0-9a-f]{40})\.json$/;

  const uncalledPathPattern = /^uncalled-functions\/[^/]+\/([0-9a-f]{40})\.json$/;

  // The commits API's largest page, as the number of commits it lists.
  const commitsPerPage = 100;

  // The most listings of history a view fetches. It keeps one view within
  // the anonymous limit of 60 API calls an hour. A record off staging's
  // first-parent history makes the walk head for the root, so a first view
  // then spends all of them.
  const maxCommitPages = 40;

  // The most records fetched at once.
  const recordFetchWidth = 8;

  /**
   * Lists the records in `tree`, a response of GitHub's git trees API, by
   * column, in the tree's order.
   *
   * Throws if the listing is truncated.
   *
   * @param {Tree} tree
   * @returns {Map<string, Entry[]>}
   */
  function recordsInTree(tree) {
    if (tree.truncated) {
      throw new Error('GitHub truncated the listing of the metrics branch, so some records would be missing');
    }
    const columns = new Map();
    for (const entry of tree.tree) {
      const match = recordPathPattern.exec(entry.path);
      if (!match) continue;
      const [, column, commit] = match;
      if (!columns.has(column)) columns.set(column, []);
      columns.get(column).push({commit, path: entry.path});
    }
    return columns;
  }

  /**
   * Lists the records of uncalled functions in `tree`, a response of GitHub's
   * git trees API which recordsInTree has accepted, as {commit, path}.
   * They are the nightly measurements of baseline-tightener.yml, one file
   * per commit measured, beside the symbol table's columns.
   *
   * @param {Tree} tree
   * @returns {Entry[]}
   */
  function uncalledRecordsInTree(tree) {
    return tree.tree.flatMap(entry => {
      const match = uncalledPathPattern.exec(entry.path);
      return match ? [{commit: match[1], path: entry.path}] : [];
    });
  }

  /**
   * Returns the parents, date and subject of `commit`, a commit as the
   * commits API returns it.
   *
   * @param {ApiCommit} commit
   * @returns {Summary}
   * @throws if the commit's date cannot be read
   */
  function summarise(commit) {
    if (!Number.isFinite(Date.parse(commit.commit.committer.date))) {
      throw new Error(`commit ${commit.sha} has a date which is not a date: ${commit.commit.committer.date}`);
    }
    return {
      parents: commit.parents.map(parent => parent.sha),
      date:    commit.commit.committer.date,
      subject: commit.commit.message.split('\n', 1)[0]
    };
  }

  /**
   * Follows first parents from `head` through `known`, a map from each
   * commit to its summary, until every commit in `wanted` is found.
   *
   * Returns the commits of `wanted` found, newest first, and `unknown`: the
   * commit at which the walk left `known`, or null if the walk found every
   * wanted commit or reached a commit with no parent.
   *
   * @param {Map<string, Summary>} known
   * @param {string} head
   * @param {Set<string>} wanted
   * @returns {{found: string[], unknown: string | null}}
   */
  function walkFirstParents(known, head, wanted) {
    const found = [];
    let commit = head;
    while (found.length < wanted.size) {
      const summary = known.get(commit);
      if (!summary) return {found, unknown: commit};
      if (wanted.has(commit)) found.push(commit);
      if (summary.parents.length === 0) break;
      commit = summary.parents[0];
    }
    return {found, unknown: null};
  }

  /**
   * Returns `value` as JSON, with each object's keys sorted.
   * @param {any} value
   * @returns {string}
   */
  function canonical(value) {
    if (Array.isArray(value)) return '[' + value.map(canonical).join(',') + ']';
    if (value !== null && typeof value === 'object') {
      return '{' + Object.keys(value).sort().map(key => JSON.stringify(key) + ':' + canonical(value[key])).join(',') + '}';
    }
    return JSON.stringify(value);
  }

  /**
   * Lists each leaf of `before` and `after` whose value differs, as a dotted
   * path with both values. A leaf absent from one side has the value
   * undefined there.
   *
   * @param {any} before
   * @param {any} after
   * @param {string} [prefix]
   * @returns {Difference[]}
   */
  function differences(before, after, prefix = '') {
    /** @param {any} value */
    const isObject = value => value !== null && typeof value === 'object' && !Array.isArray(value);
    if (!isObject(before) || !isObject(after)) {
      return canonical(before) === canonical(after) ? [] : [{path: prefix, before, after}];
    }
    const keys = [...new Set([...Object.keys(before), ...Object.keys(after)])].sort();
    return keys.flatMap(key => differences(before[key], after[key], prefix ? prefix + '.' + key : key));
  }

  /** @param {any} value */
  const isObject = value => value !== null && typeof value === 'object' && !Array.isArray(value);

  /**
   * Lists what is wrong with `record`, of any column, stored under the name
   * `commit`, in the fields which every record holds: that it is a JSON
   * object, names `commit` as its commit and was measured on a clean tree.
   *
   * @param {JsonRecord} record
   * @param {string} commit
   * @returns {string[]}
   */
  function commonProblems(record, commit) {
    if (!isObject(record)) return ['it is not a JSON object'];
    const problems = [];
    if (record.commit !== commit) problems.push(`it names commit ${record.commit}`);
    if (record.clean !== true) problems.push('it was not measured on a clean tree');
    return problems;
  }

  /**
   * Lists what is wrong with `record`, stored under the name `commit`.
   *
   * A record with no problem is an object holding `commit` as its commit, a
   * clean tree, a toolchain, the executable's symbols and name bytes, and a
   * count of defined symbols for each object. It may also hold
   * `prune_dependency_analysis`, with the name of the function whose
   * instructions were counted, a positive count of them and the exit status
   * of the run which stamped, and `prune_analysis` (the older field, kept
   * and not plotted), with an invocation and a count of instructions, and
   * `runner_setup`, with a boundary, the exit status of the run, a positive
   * number of tests and a positive count for each of `runnerSetupEvents`.
   * The older `runner_overhead` is kept and not checked, nor plotted.
   *
   * @param {JsonRecord} record
   * @param {string} commit
   * @returns {string[]}
   */
  function problemsWith(record, commit) {
    const problems = commonProblems(record, commit);
    if (!isObject(record)) return problems;
    /** @param {any} value */
    const isCount = value => Number.isInteger(value) && value >= 0;
    if (record.toolchain === null || typeof record.toolchain !== 'object') problems.push('it has no toolchain');
    const executable = record.executable || {};
    if (!isCount(executable.symbols)) problems.push('it has no count of the executable\'s symbols');
    if (!isCount(executable.symbol_name_bytes)) problems.push('it has no count of the executable\'s name bytes');
    if (Object.hasOwn(record, 'prune_dependency_analysis')) {
      const analysis = record.prune_dependency_analysis;
      if (analysis === null || typeof analysis !== 'object' || !isCount(analysis.instructions) || analysis.instructions === 0
          || typeof analysis.function !== 'string' || !isCount(analysis.stamping_exit_status)) {
        problems.push('it has a prune_dependency_analysis with no function, no positive count of instructions, or no exit status');
      }
    }
    if (Object.hasOwn(record, 'prune_analysis')) {
      const analysis = record.prune_analysis;
      if (analysis === null || typeof analysis !== 'object' || !isCount(analysis.instructions)
          || typeof analysis.invocation !== 'string') {
        problems.push('it has a prune_analysis with no invocation, or no count of instructions');
      }
    }
    if (Object.hasOwn(record, 'runner_setup')) {
      const setup = record.runner_setup;
      if (setup === null || typeof setup !== 'object' || typeof setup.boundary !== 'string') {
        problems.push('it has a runner_setup with no boundary');
      } else {
        if (!isCount(setup.exit_status)) problems.push('it has a runner_setup with no exit status');
        if (!(isCount(setup.tests) && setup.tests > 0)) problems.push('it has a runner_setup whose number of tests is not a positive integer');
        for (const event of runnerSetupEvents) {
          if (!(isCount(setup[event]) && setup[event] > 0)) problems.push(`it has a runner_setup with no positive count of ${event}`);
        }
      }
    }
    const objects = record.defined_symbols_by_object;
    if (objects === null || typeof objects !== 'object' || !Object.values(objects).every(isCount)) {
      problems.push('it has no count of defined symbols for each object');
    }
    return problems;
  }

  /**
   * The events a record's `runner_setup` counts: the instructions and the
   * system calls (callgrind's Ir and sysCount) of the runner's setup of each
   * test, that is, of `test_to_run::execute` outside
   * `execution_timer::time_execution`, where the test runs.
   */
  const runnerSetupEvents = ['instructions', 'syscalls'];

  /**
   * Returns the `event` counted in the runner's setup per test in `record`'s
   * run, or null if the record has no `runner_setup` (an old record, or one
   * whose measurement failed). Such a record is left out of the series, not
   * plotted as zero. A record is plotted whatever its run's exit status.
   *
   * @param {JsonRecord} record
   * @param {string} event one of `runnerSetupEvents`
   * @returns {number | null}
   */
  function runnerSetupPerTest(record, event) {
    if (!Object.hasOwn(record, 'runner_setup')) return null;
    const setup = record.runner_setup;
    return setup[event] / setup.tests;
  }

  /**
   * Returns the instructions counted inside prune's analysis in `record`'s
   * run, or null if the record has none (an old record, or one whose
   * measurement failed). Such a record is left out of the series, not
   * plotted as zero. A record whose stamping run failed is plotted too: the
   * analysis reads the stamps of one run on a fresh runner, whatever its outcome.
   *
   * @param {JsonRecord} record
   * @returns {number | null}
   */
  function pruneInstructions(record) {
    if (!Object.hasOwn(record, 'prune_dependency_analysis')) return null;
    return record.prune_dependency_analysis.instructions;
  }

  /**
   * Lists what is wrong with `record`, a record of uncalled functions stored
   * under the name `commit`.
   *
   * A record with no problem is an object holding `commit` as its commit, a
   * clean tree, the count of uncalled functions, and a toolchain of the
   * demanglers and the tools which measured it, each named by a string.
   *
   * @param {JsonRecord} record
   * @param {string} commit
   * @returns {string[]}
   */
  function problemsWithUncalled(record, commit) {
    const problems = commonProblems(record, commit);
    if (!isObject(record)) return problems;
    const toolchain = record.toolchain;
    /** @param {any} value */
    const isNames = value => Array.isArray(value) && value.every(name => typeof name === 'string');
    if (!Number.isInteger(record.uncalled_functions) || record.uncalled_functions < 0) {
      problems.push('it has no count of uncalled functions');
    }
    if (toolchain === null || typeof toolchain !== 'object' || !isNames(toolchain.demanglers) || !isNames(toolchain.tools)) {
      problems.push('it has no toolchain of demanglers and tools');
    }
    return problems;
  }

  /**
   * Returns the count of uncalled functions measured at the point whose
   * record is `record`, or null if no measurement is at that commit. Such a
   * point is left out of the series, not plotted as zero.
   *
   * @param {JsonRecord} record
   * @returns {number | null}
   */
  function uncalledCount(record) {
    return Object.hasOwn(record, 'uncalled') ? record.uncalled.count : null;
  }

  /**
   * Returns `points`, those of a column, with each point whose commit has a
   * record in `uncalledPoints`, built by pointsOf from the records of
   * uncalled functions, holding in its record `uncalled`: the count, the
   * toolchain run of the measurement and the toolchain's change from the
   * measurement before. The two series' toolchains are independent.
   *
   * @param {Point[]} points
   * @param {Point[]} uncalledPoints
   * @returns {Point[]}
   */
  function withUncalled(points, uncalledPoints) {
    const measured = new Map(uncalledPoints.map(point => [point.commit, point]));
    return points.map(point => {
      const found = measured.get(point.commit);
      if (!found) return point;
      const uncalled = {count: found.record.uncalled_functions, run: found.run, toolchainChange: found.toolchainChange};
      return {...point, record: {...point.record, uncalled}};
    });
  }

  /**
   * Builds the points of one column, oldest first, from its records in
   * history order.
   *
   * Each point holds its commit's summary and record, and the toolchain
   * run it belongs to. A run is a maximal sequence of consecutive points
   * whose toolchains are equal. The first point of each run after the
   * first holds the toolchain's differences from the point before it.
   *
   * A point also holds `setupRun`, the number of changes of the `boundary`
   * of its `runner_setup` up to it, among the points which have one, and
   * `boundaryChange`, whether this point's is the first of a different
   * boundary: the runner's setup of a different boundary (another
   * executable, say) is a different series, which the plot breaks the line
   * at.
   *
   * @param {OrderedRecord[]} orderedRecords
   * @returns {Point[]}
   */
  function pointsOf(orderedRecords) {
    let run = 0, setupRun = 0;
    /** @type {string | null} */
    let boundary = null;
    return orderedRecords.map(({commit, summary, record}, index) => {
      const previous   = orderedRecords[index - 1];
      const toolchainChange = previous && canonical(previous.record.toolchain) !== canonical(record.toolchain)
        ? differences(previous.record.toolchain, record.toolchain)
        : null;
      if (toolchainChange) ++run;
      const setupBoundary = Object.hasOwn(record, 'runner_setup') ? record.runner_setup.boundary : null;
      const boundaryChange = typeof setupBoundary === 'string' && boundary !== null && setupBoundary !== boundary;
      if (boundaryChange) ++setupRun;
      if (typeof setupBoundary === 'string') boundary = setupBoundary;
      return {commit, date: summary.date, subject: summary.subject, record, run, toolchainChange, setupRun, boundaryChange};
    });
  }

  /**
   * Lists every object whose count of defined symbols differs between the
   * records `before` and `after`, the largest change first, and objects of
   * equal change by path.
   *
   * An object absent from one record has a count of null there, and is
   * taken to define no symbols in it.
   *
   * @param {JsonRecord} before
   * @param {JsonRecord} after
   * @returns {{object: string, before: number | undefined, after: number | undefined, delta: number}[]}
   */
  function movers(before, after) {
    const countsBefore = before.defined_symbols_by_object;
    const countsAfter  = after.defined_symbols_by_object;
    const objects = new Set([...Object.keys(countsBefore), ...Object.keys(countsAfter)]);
    return [...objects]
      .map(object => {
        const was = Object.hasOwn(countsBefore, object) ? countsBefore[object] : null;
        const is  = Object.hasOwn(countsAfter,  object) ? countsAfter[object]  : null;
        return {object, before: was, after: is, delta: (is || 0) - (was || 0)};
      })
      .filter(mover => mover.delta !== 0)
      .sort((a, b) => Math.abs(b.delta) - Math.abs(a.delta) || (a.object < b.object ? -1 : a.object > b.object ? 1 : 0));
  }

  /**
   * Calls `task` on each element of `items`, at most `width` at a time.
   *
   * Returns each call's outcome, in the order of `items`: {value} if it
   * resolved, {error} if it rejected.
   *
   * @template T, R
   * @param {T[]} items
   * @param {number} width
   * @param {(item: T) => Promise<R>} task
   * @returns {Promise<{value?: R, error?: Error}[]>}
   */
  async function settleEach(items, width, task) {
    const outcomes = new Array(items.length);
    let next = 0;
    async function worker() {
      while (next < items.length) {
        const index = next++;
        try { outcomes[index] = {value: await task(items[index])}; }
        catch (error) { outcomes[index] = {error}; }
      }
    }
    await Promise.all(Array.from({length: Math.min(width, items.length)}, worker));
    return outcomes;
  }

  /**
   * Fetches staging's history through `source` until the first-parent walk
   * from its head finds every commit of `wanted`, reaches a root, or has
   * fetched `maxCommitPages` listings.
   *
   * Each listing after the first starts at the commit where the walk left
   * `known`. `known` maps commits to their summaries, and gains those of
   * every commit fetched.
   *
   * Throws if a listing lacks the commit it starts at.
   *
   * @param {Source} source
   * @param {Map<string, Summary>} known
   * @param {Set<string>} wanted
   * @returns {Promise<{head: string, pages: number, walk: {found: string[], unknown: string | null}}>}
   */
  async function walkStaging(source, known, wanted) {
    /** @param {ApiCommit[]} listing */
    const learn = listing => { for (const commit of listing) known.set(commit.sha, summarise(commit)); };
    const first = await source.commits(null);
    if (first.length === 0) throw new Error('GitHub listed no commits on staging');
    learn(first);
    const head = first[0].sha;
    let walk  = walkFirstParents(known, head, wanted);
    let pages = 1;
    while (walk.unknown && pages < maxCommitPages) {
      learn(await source.commits(walk.unknown));
      ++pages;
      if (!known.has(walk.unknown)) throw new Error(`GitHub's listing of the history from ${walk.unknown} lacks that commit`);
      walk = walkFirstParents(known, head, wanted);
    }
    return {head, pages, walk};
  }

  /**
   * Loads every column's records through `source`, and orders them by
   * staging's first-parent history.
   *
   * `source` has three asynchronous functions:
   *  - tree(), the listing of the metrics branch as the git trees API gives
   *    it, or null if the branch does not exist;
   *  - commits(start), the first page of the commits API's listing of the
   *    history from the commit `start`, or from staging's head if `start` is
   *    null;
   *  - record(path), the record at `path` on the metrics branch.
   * `known` is as for walkStaging.
   *
   * Returns {state: 'absent'} if the metrics branch does not exist, and
   * {state: 'empty'} if it holds no records. Otherwise returns {state:
   * 'loaded', head, pages, columns}, where each column has its name, its
   * points and its problems. A record which cannot be fetched, has a
   * problem or is not found on the walk is among the problems, and not
   * among the points.
   *
   * @param {Source} source
   * @param {Map<string, Summary>} known
   * @returns {Promise<{state: "absent"} | {state: "empty"} | {state: "loaded", head: string, pages: number,
   *           columns: {name: string, points: Point[], problems: Unplottable[]}[]}>}
   */
  async function loadHistory(source, known) {
    const tree = await source.tree();
    if (tree === null) return {state: 'absent'};
    const listing = recordsInTree(tree);
    if (listing.size === 0) return {state: 'empty'};
    const uncalledEntries = uncalledRecordsInTree(tree);
    const entries = [...listing.values()].flat().concat(uncalledEntries);
    const wanted  = new Set(entries.map(entry => entry.commit));
    const [{head, pages, walk}, outcomes] = await Promise.all([
      walkStaging(source, known, wanted),
      settleEach(entries, recordFetchWidth, entry => source.record(entry.path))
    ]);
    const fetched = new Map(entries.map((entry, index) => [entry.path, outcomes[index]]));
    /** @param {Entry} entry */
    const outcomeOf = entry => fetched.get(entry.path) ?? {error: new Error('it was not fetched')};
    /** @param {string} commit */
    const summaryOf = commit => {
      const summary = known.get(commit);
      if (!summary) throw new Error(`the walk found ${commit} but holds no summary of it`);
      return summary;
    };
    const historyOrder = walk.found.slice().reverse();
    const notFound = walk.unknown
      ? `the walk along staging's first parents stopped after ${pages} pages of history, before reaching it`
      : 'it is not on staging\'s first-parent history';
    // The records of `entries` which have no problem by commit, and those which have, by path
    /**
     * @param {Entry[]} entries
     * @param {(record: JsonRecord, commit: string) => string[]} problemsOf
     */
    const sorted = (entries, problemsOf) => {
      const problems = [];
      const byCommit = new Map();
      for (const entry of entries) {
        const outcome = outcomeOf(entry);
        const entryProblems = outcome.error
          ? [`it could not be fetched: ${outcome.error.message}`]
          : problemsOf(outcome.value, entry.commit);
        if (!walk.found.includes(entry.commit)) entryProblems.push(notFound);
        if (entryProblems.length) problems.push({path: entry.path, problems: entryProblems});
        else byCommit.set(entry.commit, outcome.value);
      }
      return {problems, byCommit};
    };
    const {problems: uncalledProblems, byCommit: uncalledByCommit} = sorted(uncalledEntries, problemsWithUncalled);
    const uncalledPoints = pointsOf(historyOrder
      .filter(commit => uncalledByCommit.has(commit))
      .map(commit => ({commit, summary: summaryOf(commit), record: uncalledByCommit.get(commit)})));
    const columns = [...listing].map(([name, columnEntries]) => {
      const {problems, byCommit} = sorted(columnEntries, problemsWith);
      const ordered = historyOrder
        .filter(commit => byCommit.has(commit))
        .map(commit => ({commit, summary: summaryOf(commit), record: byCommit.get(commit)}));
      const points = withUncalled(pointsOf(ordered), uncalledPoints);
      const unplotted = uncalledPoints.filter(point => !points.some(plotted => plotted.commit === point.commit))
        .map(point => ({path: uncalledEntries.find(entry => entry.commit === point.commit)?.path ?? '',
                        problems: ['it is not at a commit with a plotted symbol table']}));
      return {name, points, problems: problems.concat(uncalledProblems, unplotted)};
    });
    return {state: 'loaded', head, pages, columns};
  }

  const millisecondsPerDay = 24 * 60 * 60 * 1000;
  const dayStepsToTry      = [1, 2, 3, 7, 14, 28, 56, 91, 182, 365];

  /**
   * Returns the horizontal positions of points at the given times, linear in
   * time over whole UTC days: from the midnight which starts the earliest
   * day to the one which ends the latest. Also returns the ticks, at
   * midnights a whole number of days apart from the first, up to and
   * including the one which ends the latest day, the fewest days for which
   * neighbouring ticks are at least `minGap` apart, each tick a different day,
   * and the position of any time, which lies outside `left` to `right` for a
   * time outside the days of `times`.
   *
   * @param {number[]} times  Milliseconds since the epoch, one for each point
   * @param {number} left     The position of the start of the first day
   * @param {number} right    The position of the end of the last day
   * @param {number} minGap
   * @returns {{xs: number[], ticks: {x: number, time: number}[], x: (time: number) => number}}
   */
  function timeAxis(times, left, right, minGap) {
    const first = Math.floor(Math.min(...times) / millisecondsPerDay);
    const last  = Math.floor(Math.max(...times) / millisecondsPerDay);
    const days  = last - first + 1;
    const x     = (/** @type {number} */ time) => left + (time / millisecondsPerDay - first) / days * (right - left);
    const step  = dayStepsToTry.find(candidate => candidate * (right - left) / days >= minGap) ?? dayStepsToTry[dayStepsToTry.length - 1];
    const ticks = [];
    for (let day = first; day <= last + 1; day += step) {
      ticks.push({x: x(day * millisecondsPerDay), time: day * millisecondsPerDay});
    }
    return {xs: times.map(x), ticks, x};
  }

  /**
   * Returns the index of the position nearest `offset`, the earliest of equals.
   *
   * @param {number[]} xs  Non-empty
   * @param {number} offset
   * @returns {number}
   */
  function nearestPoint(xs, offset) {
    let nearest = 0;
    xs.forEach((x, index) => { if (Math.abs(x - offset) < Math.abs(xs[nearest] - offset)) nearest = index; });
    return nearest;
  }

  /**
   * Returns the least distance between two of the positions, or Infinity
   * where there are fewer than two.
   *
   * @param {number[]} xs
   * @returns {number}
   */
  function smallestGap(xs) {
    const sorted = xs.slice().sort((a, b) => a - b);
    return sorted.reduce((gap, x, index) => index === 0 ? gap : Math.min(gap, x - sorted[index - 1]), Infinity);
  }

  Object.assign(exports, {
    commitsPerPage, maxCommitPages, timeAxis, nearestPoint, smallestGap,
    recordsInTree, summarise, walkFirstParents, canonical, differences, problemsWith, problemsWithUncalled, pruneInstructions, runnerSetupEvents, runnerSetupPerTest, uncalledCount, uncalledRecordsInTree, withUncalled, pointsOf, movers, loadHistory
  });
})(typeof module === 'object' && module.exports ? module.exports : (window.symbolDashboard = {}));

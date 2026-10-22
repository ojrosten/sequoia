// The data behind the symbol dashboard: which records the metrics branch
// holds, their order along staging's first-parent history, where the
// toolchain changes, and which objects moved between two records. Nothing
// here touches the page, so the selftest runs it under Node.
//
// A record is the output of scripts/symbol_table_report.py, stored as
// symbol-table/<column>/<commit>.json on the metrics branch.
(function (exports) {
  'use strict';

  const recordPathPattern = /^symbol-table\/([^/]+)\/([0-9a-f]{40})\.json$/;

  // The commits API's largest page.
  const commitsPerPage = 100;

  // The most pages of history a view fetches. It keeps one view within the
  // anonymous limit of 60 API calls an hour.
  const maxCommitPages = 40;

  // The most records fetched at once.
  const recordFetchWidth = 8;

  /**
   * Lists the records in `tree`, a response of GitHub's git trees API, by
   * column, in the tree's order.
   *
   * Throws if the listing is truncated.
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
   * Returns the parents, date and subject of `commit`, a commit as the
   * commits API returns it.
   */
  function summarise(commit) {
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

  /** Returns `value` as JSON, with each object's keys sorted. */
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
   */
  function differences(before, after, prefix = '') {
    const isObject = value => value !== null && typeof value === 'object' && !Array.isArray(value);
    if (!isObject(before) || !isObject(after)) {
      return canonical(before) === canonical(after) ? [] : [{path: prefix, before, after}];
    }
    const keys = [...new Set([...Object.keys(before), ...Object.keys(after)])].sort();
    return keys.flatMap(key => differences(before[key], after[key], prefix ? prefix + '.' + key : key));
  }

  /**
   * Lists what is wrong with `record`, stored under the name `commit`.
   *
   * A record with no problem holds `commit` as its commit, a toolchain, the
   * executable's symbols and name bytes, and a count of defined symbols for
   * each object.
   */
  function problemsWith(record, commit) {
    const problems = [];
    const isCount = value => Number.isInteger(value) && value >= 0;
    if (record.commit !== commit) problems.push(`it names commit ${record.commit}`);
    if (record.toolchain === null || typeof record.toolchain !== 'object') problems.push('it has no toolchain');
    const executable = record.executable || {};
    if (!isCount(executable.symbols)) problems.push('it has no count of the executable\'s symbols');
    if (!isCount(executable.symbol_name_bytes)) problems.push('it has no count of the executable\'s name bytes');
    const objects = record.defined_symbols_by_object;
    if (objects === null || typeof objects !== 'object' || !Object.values(objects).every(isCount)) {
      problems.push('it has no count of defined symbols for each object');
    }
    return problems;
  }

  /**
   * Builds the points of one column, oldest first, from its records in
   * history order.
   *
   * Each point holds its commit's summary and record, and the toolchain
   * run it belongs to. A run is a maximal sequence of consecutive points
   * whose toolchains are equal. The first point of each run after the
   * first holds the toolchain's differences from the point before it.
   */
  function pointsOf(orderedRecords) {
    let run = 0;
    return orderedRecords.map(({commit, summary, record}, index) => {
      const previous   = orderedRecords[index - 1];
      const toolchainChange = previous && canonical(previous.record.toolchain) !== canonical(record.toolchain)
        ? differences(previous.record.toolchain, record.toolchain)
        : null;
      if (toolchainChange) ++run;
      return {commit, date: summary.date, subject: summary.subject, record, run, toolchainChange};
    });
  }

  /**
   * Lists every object whose count of defined symbols differs between the
   * records `before` and `after`, the largest change first, and objects of
   * equal change by path.
   *
   * An object absent from one record has a count of null there, and is
   * taken to define no symbols in it.
   */
  function movers(before, after) {
    const countsBefore = before.defined_symbols_by_object;
    const countsAfter  = after.defined_symbols_by_object;
    const objects = new Set([...Object.keys(countsBefore), ...Object.keys(countsAfter)]);
    return [...objects]
      .map(object => {
        const was = object in countsBefore ? countsBefore[object] : null;
        const is  = object in countsAfter  ? countsAfter[object]  : null;
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
   * fetched `maxCommitPages` pages.
   *
   * `known` maps commits to their summaries, and gains those of every
   * commit fetched.
   */
  async function walkStaging(source, known, wanted) {
    const firstPage = await source.commits(1, null);
    if (firstPage.length === 0) throw new Error('GitHub listed no commits on staging');
    const head = firstPage[0].sha;
    for (const commit of firstPage) known.set(commit.sha, summarise(commit));
    let walk  = walkFirstParents(known, head, wanted);
    let pages = 1;
    let exhausted = false;
    while (walk.unknown && !exhausted && pages < maxCommitPages) {
      const page = await source.commits(++pages, head);
      for (const commit of page) known.set(commit.sha, summarise(commit));
      exhausted = page.length === 0;
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
   *  - commits(page, head), a page of the commits API's listing of `head`,
   *    or of staging if `head` is null;
   *  - record(path), the record at `path` on the metrics branch.
   * `known` is as for walkStaging.
   *
   * Returns {state: 'absent'} if the metrics branch does not exist, and
   * {state: 'empty'} if it holds no records. Otherwise returns {state:
   * 'loaded', head, pages, columns}, where each column has its name, its
   * points and its problems. A record which cannot be fetched, has a
   * problem or is not found on the walk is among the problems, and not
   * among the points.
   */
  async function loadHistory(source, known) {
    const tree = await source.tree();
    if (tree === null) return {state: 'absent'};
    const listing = recordsInTree(tree);
    if (listing.size === 0) return {state: 'empty'};
    const entries = [...listing.values()].flat();
    const wanted  = new Set(entries.map(entry => entry.commit));
    const [{head, pages, walk}, outcomes] = await Promise.all([
      walkStaging(source, known, wanted),
      settleEach(entries, recordFetchWidth, entry => source.record(entry.path))
    ]);
    const fetched = new Map(entries.map((entry, index) => [entry.path, outcomes[index]]));
    const historyOrder = walk.found.slice().reverse();
    const notFound = walk.unknown
      ? `it is not among the ${pages} pages of staging's history fetched`
      : 'it is not on staging\'s first-parent history';
    const columns = [...listing].map(([name, columnEntries]) => {
      const problems = [];
      const byCommit = new Map();
      for (const entry of columnEntries) {
        const outcome = fetched.get(entry.path);
        const entryProblems = outcome.error
          ? [`it could not be fetched: ${outcome.error.message}`]
          : problemsWith(outcome.value, entry.commit);
        if (!walk.found.includes(entry.commit)) entryProblems.push(notFound);
        if (entryProblems.length) problems.push({path: entry.path, problems: entryProblems});
        else byCommit.set(entry.commit, outcome.value);
      }
      const ordered = historyOrder
        .filter(commit => byCommit.has(commit))
        .map(commit => ({commit, summary: known.get(commit), record: byCommit.get(commit)}));
      return {name, points: pointsOf(ordered), problems};
    });
    return {state: 'loaded', head, pages, columns};
  }

  Object.assign(exports, {
    commitsPerPage, maxCommitPages,
    recordsInTree, summarise, walkFirstParents, canonical, differences, problemsWith, pointsOf, movers, loadHistory
  });
})(typeof module === 'object' && module.exports ? module.exports : (window.symbolDashboard = {}));

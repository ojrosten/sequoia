// The symbol dashboard's page: it fetches the records, draws them as charts,
// tiles and tables in index.html's elements, and answers the pointer and the
// keys. symbol_dashboard.js holds the data logic.
'use strict';
(() => {
  /**
   * @typedef {{parents: string[], date: string, subject: string}} Summary
   * @typedef {{path: string, before: any, after: any}} Difference
   * @typedef {{commit: string, date: string, subject: string, record: any,
   *            run: number, toolchainChange: Difference[] | null,
   *            setupRun: number, boundaryChange: boolean,
   *            leftoverBoundaryChange: boolean}} Point
   * @typedef {{path: string, problems: string[]}} Unplottable
   * @typedef {{name: string, points: Point[], problems: Unplottable[]}} Column
   * @typedef {{object: string, before: number | null, after: number | null,
   *            delta: number}} Mover
   * @typedef {{tree(): Promise<any>, commits(start: string | null): Promise<any>,
   *            record(path: string): Promise<any>}} Source
   * @typedef {{name: string, value: (record: any) => number | null,
   *            unit: string, exitStatus?: (record: any) => number | null,
   *            includesZero?: boolean,
   *            blindLabel?: (record: any) => string | null,
   *            markersOnly?: boolean, joinsGaps?: boolean,
   *            runOf?: (point: Point) => any, changeOf?: (point: Point) => any,
   *            breakLabel?: (point: Point) => string,
   *            event?: string}} Measure
   * @typedef {{svg: SVGElement, markHover(index: number | null): void,
   *            markSelected(index: number): void,
   *            anchor(index: number): {x: number, y: number}}} Chart
   */

  const repository = 'ojrosten/sequoia';
  const api        = `https://api.github.com/repos/${repository}`;
  const raw        = `https://raw.githubusercontent.com/${repository}/metrics/`;
  const cacheKey   = 'sequoia-symbol-dashboard-commits-1';
  const defaultColumn = 'linux-gcc-debug';
  const topMovers  = 15;
  // Wider than either label of a break at the labels' size, so that labels of neighbouring breaks never overlap
  const breakLabelWidth = 64;
  const tickMarkLength  = 6;
  // Half the width of the widest short date, such as "30 Sept", at the axis's size
  const dateLabelHalfWidth = 24;
  const parameters = new URLSearchParams(location.search);
  // A relative path only, so that no link can make the public page show
  // another site's figures.
  const fixtures   = parameters.has('fixtures') && /^(?![a-z][a-z0-9+.-]*:|\/\/)/i.test(/** @type {string} */ (parameters.get('fixtures')))
                   ? parameters.get('fixtures') : null;
  /** @type {{calls: number, records: number, remaining: string | null,
   *           reset: string | null}} */
  const meter      = {calls: 0, records: 0, remaining: null, reset: null};
  const svgNamespace = 'http://www.w3.org/2000/svg';

  const integer = new Intl.NumberFormat('en-GB');
  const oneDecimal = new Intl.NumberFormat('en-GB', {maximumFractionDigits: 1});
  /** @param {number} value */
  const signed  = value => (value > 0 ? '+' : value < 0 ? '−' : '') + integer.format(Math.abs(value));
  /** @param {number} delta @param {number} base */
  const percent = (delta, base) => {
    if (base === 0) return 'from nothing';
    const fraction  = 100 * delta / base;
    const magnitude = Math.abs(fraction);
    return (fraction > 0 ? '+' : fraction < 0 ? '−' : '')
         + (magnitude >= 0.1 || magnitude === 0 ? magnitude.toFixed(2) : magnitude.toPrecision(2)) + '%';
  };
  /** @param {string} iso */
  const shortDate = iso => new Date(iso).toLocaleDateString('en-GB', {day: 'numeric', month: 'short', timeZone: 'UTC'});
  /** @param {string} iso */
  const longDate  = iso => new Date(iso).toLocaleDateString('en-GB', {day: 'numeric', month: 'short', year: 'numeric', timeZone: 'UTC'});
  /** @param {any} seconds  A header's value, a string, which `*` converts */
  const clock     = seconds => new Date(seconds * 1000).toLocaleTimeString('en-GB', {hour: '2-digit', minute: '2-digit'});
  /** @param {string} commit */
  const short     = commit => commit.slice(0, 7);
  /** @param {string} commit */
  const commitUrl = commit => `https://github.com/${repository}/commit/${commit}`;

  /**
   * Builds an element, sets its attributes, and appends its children,
   * strings as text.
   *
   * @template {string} Name
   * @param {Name} name  Prefixed `svg:` for an element of the SVG namespace
   * @param {Record<string, string | number>} [attributes]
   * @param {...(string | Node)} children
   * @returns {Name extends `svg:${string}` ? SVGElement : HTMLElement}
   */
  function element(name, attributes = {}, ...children) {
    const node = name.startsWith('svg:') ? document.createElementNS(svgNamespace, name.slice(4)) : document.createElement(name);
    for (const [key, value] of Object.entries(attributes)) node.setAttribute(key, value);
    for (const child of children) node.append(child);
    return /** @type {any} */ (node);
  }
  /** @param {string} href @param {string} text */
  const link = (href, text) => element('a', {href}, text);

  /**
   * Fetches JSON from `url`. Returns null on a 404 if `nullOn404`, and
   * throws on any other failure, naming the url.
   *
   * @param {string} url
   * @param {boolean} nullOn404
   * @param {(response: Response) => void} [onResponse]
   * @returns {Promise<any>}
   */
  async function fetchJson(url, nullOn404, onResponse = () => {}) {
    let response;
    try { response = await fetch(url, {headers: {Accept: 'application/vnd.github+json'}}); }
    catch (error) { throw new Error(`${url} could not be reached: ${/** @type {Error} */ (error).message}`); }
    onResponse(response);
    if (response.status === 404 && nullOn404) return null;
    if (!response.ok) {
      let message = '';
      try { message = (await response.json()).message || ''; } catch (error) { /* a body which is not JSON */ }
      if ((response.status === 403 || response.status === 429) && response.headers.get('x-ratelimit-remaining') === '0') {
        throw new Error(`GitHub's limit of 60 API calls an hour, for a reader not signed in, is spent. It resets at ${clock(response.headers.get('x-ratelimit-reset'))}.`);
      }
      throw new Error(`${url} answered ${response.status}${message ? ': ' + message : ''}`);
    }
    try { return await response.json(); }
    catch (error) { throw new Error(`${url} did not answer with JSON: ${/** @type {Error} */ (error).message}`); }
  }

  /** @returns {Source} */
  function githubSource() {
    /** @param {Response} response */
    const count = response => {
      ++meter.calls;
      meter.remaining = response.headers.get('x-ratelimit-remaining');
      meter.reset     = response.headers.get('x-ratelimit-reset');
    };
    return {
      tree:    () => fetchJson(`${api}/git/trees/metrics?recursive=1`, true, count),
      commits: start => fetchJson(
        `${api}/commits?sha=${start || 'staging'}&per_page=${symbolDashboard.commitsPerPage}`, false, count),
      record:  path => { ++meter.records; return fetchJson(raw + path, false); }
    };
  }

  /**
   * @param {string} directory
   * @returns {Source}
   */
  function fixtureSource(directory) {
    const base = directory.replace(/\/*$/, '/');
    return {
      tree:    () => fetchJson(base + 'tree.json', true),
      commits: start => fetchJson(`${base}commits/${start || 'staging'}.json`, false),
      record:  path => { ++meter.records; return fetchJson(base + path, false); }
    };
  }

  // The summaries of commits fetched by earlier views. A commit's parents,
  // date and subject never change, so the cache cannot go stale; losing it
  // costs only API calls.
  /** @returns {Map<string, Summary>} */
  function knownCommits() {
    if (fixtures) return new Map();
    /** @param {any} summary */
    const isSummary = summary => summary !== null && typeof summary === 'object' && Array.isArray(summary.parents)
                              && summary.parents.every((/** @type {any} */ parent) => typeof parent === 'string')
                              && typeof summary.date === 'string' && Number.isFinite(Date.parse(summary.date)) && typeof summary.subject === 'string';
    try { return new Map(Object.entries(JSON.parse(localStorage.getItem(cacheKey) || '{}')).filter(([, summary]) => isSummary(summary))); }
    catch (error) { return new Map(); }
  }
  /** @param {Map<string, Summary>} known */
  function remember(known) {
    if (fixtures) return;
    try { localStorage.setItem(cacheKey, JSON.stringify(Object.fromEntries(known))); }
    catch (error) { /* storage is a convenience: without it, the next view fetches again */ }
  }

  /**
   * @param {string} title
   * @param {string} text
   * @param {boolean} isError
   */
  function showMessage(title, text, isError) {
    const message = /** @type {HTMLElement} */ (document.getElementById('message'));
    message.className = 'card' + (isError ? ' alert' : '');
    message.setAttribute('role', isError ? 'alert' : 'status');
    message.replaceChildren(element('h2', {}, title), element('p', {}, text));
    message.hidden = false;
  }

  /** @param {Difference[]} change */
  function describeChange(change) {
    /** @param {any} value */
    const shown = value => value === undefined ? 'absent' : typeof value === 'string' ? value : symbolDashboard.canonical(value);
    return change.map(({path, before, after}) => `${path}: ${shown(before)} → ${shown(after)}`).join('; ');
  }

  // Ticks ------------------------------------------------------------------

  /** @param {number} value @param {number} step @param {number} high */
  function compact(value, step, high) {
    const [divisor, suffix] = high >= 1e6 ? [1e6, 'M'] : high >= 1e3 ? [1e3, 'K'] : [1, ''];
    const decimals = Math.max(0, Math.ceil(-Math.log10(step / divisor) - 1e-9));
    return (value / divisor).toFixed(decimals) + suffix;
  }

  // Charts -----------------------------------------------------------------

  /** @type {{points: Point[], selected: number, hovered: number | null,
   *           charts: Chart[]}} */
  const state = {points: [], selected: 0, hovered: null, charts: []};

  // Fills `circle` in the series' colour, or, if `hollow`, with the surface's inside an outline in the series' colour.
  /** @param {Element} circle @param {boolean} hollow */
  function paintMarker(circle, hollow) {
    circle.setAttribute('fill', hollow ? 'var(--surface)' : 'var(--series)');
    circle.setAttribute('stroke', hollow ? 'var(--series)' : 'var(--surface)');
  }

  /** @param {number} cx @param {number} cy @param {number} r
   *  @param {boolean} hollow */
  function marker(cx, cy, r, hollow) {
    const circle = element('svg:circle', {cx, cy, r, 'stroke-width': 2});
    paintMarker(circle, hollow);
    return circle;
  }

  /**
   * Draws the points' values of one measure into `container`, one line per
   * run, or the markers alone if the measure is `markersOnly`, with a rule at
   * each change of toolchain, over the days from its first count to its last.
   * Returns the chart's functions to mark the hovered and selected points.
   *
   * @param {HTMLElement} container
   * @param {Measure} measure
   * @param {number} height
   * @param {boolean} labelBreaks
   * @returns {Chart}
   */
  function drawChart(container, measure, height, labelBreaks) {
    const points = state.points;
    const width  = Math.max(280, container.clientWidth);
    const margin = {top: labelBreaks ? 22 : 8, right: 12, bottom: 26, left: 56};
    const plotWidth  = width - margin.left - margin.right;
    const plotHeight = height - margin.top - margin.bottom;
    const values = points.map(point => measure.value(point.record));
    // A blind point has no value, so it has no marker and no selection ring. It is on the axis of time and is hovered,
    // and its rule, which says whether its run failed, is the cue
    /** @param {number} index */
    const blindLabel = index => measure.blindLabel === undefined ? null : measure.blindLabel(points[index].record);
    /** @param {number} index */
    const isBlind = index => blindLabel(index) !== null;
    // A point may lack the measure, and is then left out of the scale, the axis of time and the line
    const indicesWithCount = points.flatMap((point, index) => values[index] === null && !isBlind(index) ? [] : [index]);
    const valuesWithCount = indicesWithCount.filter(index => values[index] !== null).map(index => values[index]);
    const scale  = symbolDashboard.scaleOf(valuesWithCount.length > 0 ? valuesWithCount : [0], measure.includesZero === true);
    // Time runs along x, so points of one day are close and days with no point leave a gap
    const timesWithCount = indicesWithCount.map(index => Date.parse(points[index].date));
    const axisOfTime = symbolDashboard.timeAxis(timesWithCount, margin.left, width - margin.right, 80);
    /** @param {number} index */
    const x = index => axisOfTime.x(Date.parse(points[index].date));
    /** @param {number} index */
    const isWithinAxis = index => x(index) >= margin.left && x(index) <= width - margin.right;
    /** @param {number} value */
    const y = value => margin.top + plotHeight * (scale.high - value) / (scale.high - scale.low);

    const svg = element('svg:svg', {viewBox: `0 0 ${width} ${height}`, height, tabindex: 0, role: 'application',
                                    'aria-label': `${measure.name} at each point. The arrow keys select a point; `
                                                + 'the table below lists every value'});
    const axis = element('svg:g', {class: 'axis'});
    for (const tick of scale.ticks) {
      axis.append(element('svg:line', {x1: margin.left, x2: width - margin.right, y1: y(tick), y2: y(tick),
                                       stroke: tick === scale.low ? 'var(--baseline)' : 'var(--grid)', 'stroke-width': 1}));
      axis.append(element('svg:text', {x: margin.left - 8, y: y(tick) + 4, 'text-anchor': 'end'},
                          compact(tick, scale.step, scale.high)));
    }
    // A mark on the axis at each tick, its date centred beneath it, or ending at it where centred would overrun
    const plotBottom = margin.top + plotHeight;
    for (const tick of axisOfTime.ticks) {
      axis.append(element('svg:line', {x1: tick.x, x2: tick.x, y1: plotBottom, y2: plotBottom + tickMarkLength,
                                       stroke: 'var(--text-muted)', 'stroke-width': 1}));
      const overrunsRight = tick.x + dateLabelHalfWidth > width;
      axis.append(element('svg:text', {x: tick.x, y: height - 6, 'text-anchor': overrunsRight ? 'end' : 'middle'},
                          shortDate(new Date(tick.time).toISOString())));
    }
    svg.append(axis);

    // A rule is drawn between a point and the one before it at a break, and through a blind point. A break beside a
    // point outside the axis's days is not drawn, a blind point outside them is not, and a label too close to the
    // last one drawn is left out
    let lastDrawnLabelX = -Infinity;
    /** @param {number} at @param {string} label @param {boolean} dashed */
    const drawRule = (at, label, dashed) => {
      svg.append(element('svg:line', {x1: at, x2: at, y1: labelBreaks ? 16 : margin.top, y2: margin.top + plotHeight,
                                      stroke: 'var(--text-muted)', 'stroke-width': 1, ...(dashed ? {'stroke-dasharray': '3 3'} : {})}));
      if (!labelBreaks || at - lastDrawnLabelX < breakLabelWidth) return;
      svg.append(element('svg:text', {x: at, y: 11, 'text-anchor': 'middle', class: 'break-label'}, label));
      lastDrawnLabelX = at;
    };
    points.forEach((point, index) => {
      const isBreak = index > 0 && (measure.changeOf ? measure.changeOf(point) : point.toolchainChange);
      if (isBreak && isWithinAxis(index - 1) && isWithinAxis(index)) {
        drawRule((x(index - 1) + x(index)) / 2, measure.breakLabel ? measure.breakLabel(point) : 'toolchain', false);
      }
      if (isBlind(index) && isWithinAxis(index)) drawRule(x(index), /** @type {string} */ (blindLabel(index)), true);
    });

    /** @param {number} index */
    const hasFailedRun = index => measure.exitStatus !== undefined && (measure.exitStatus(points[index].record) ?? 0) !== 0;

    // A line runs through the points which have the measure and have the same `runOf`, by default the
    // toolchain's run; a measure may name a run of its own, as of its own toolchain or boundary. A point
    // without the measure ends the line, unless the measure joins gaps. A blind point ends it whether or not
    // the measure joins gaps.
    /** @type {string[][]} */
    const runs = [];
    /** @type {number[]} */
    const runStarts = [];
    /** @param {Point} point */
    const runOf = point => measure.runOf ? measure.runOf(point) : point.run;
    /** @type {Point | null} */
    let joined = null;
    points.forEach((point, index) => {
      if (values[index] === null) {
        if (!measure.joinsGaps || isBlind(index)) joined = null;
        return;
      }
      if (joined === null || runOf(joined) !== runOf(point)) { runs.push([]); runStarts.push(index); }
      joined = point;
      runs[runs.length - 1].push(`${x(index).toFixed(1)},${y(values[index]).toFixed(1)}`);
    });
    for (const run of measure.markersOnly ? [] : runs) {
      svg.append(element('svg:polyline', {points: run.join(' '), fill: 'none', stroke: 'var(--series)', 'stroke-width': 2,
                                          'stroke-linejoin': 'round', 'stroke-linecap': 'round'}));
    }
    // A point is marked where the points are far enough apart for every one to be, where it is a line of its own,
    // and where its run failed, so that a failed run shows on any axis. Where there is no line, every point is.
    const hasRoomForEveryMarker = measure.markersOnly || symbolDashboard.smallestGap(axisOfTime.xs) >= 14;
    const loneRunPoints = new Set(runStarts.filter((start, runIndex) => runs[runIndex].length === 1));
    points.forEach((point, index) => {
      if (values[index] === null
          || !(hasRoomForEveryMarker || loneRunPoints.has(index) || hasFailedRun(index))) return;
      svg.append(marker(x(index), y(values[index]), 4, hasFailedRun(index)));
    });

    const crosshair = element('svg:line', {y1: margin.top, y2: margin.top + plotHeight, stroke: 'var(--text-muted)',
                                           'stroke-width': 1, visibility: 'hidden'});
    const hoverDot  = marker(0, 0, 5, false);
    hoverDot.setAttribute('visibility', 'hidden');
    // The selection is a ring around the point's marker, so that the marker still shows whether its run failed
    const selectDot = element('svg:circle', {r: 8, fill: 'none', stroke: 'var(--series)', 'stroke-width': 2});
    svg.append(crosshair, selectDot, hoverDot);

    const overlay = element('svg:rect', {x: margin.left, y: 0, width: plotWidth, height, fill: 'transparent'});
    /** @param {MouseEvent} event */
    const indexAt = event => {
      const bounds = svg.getBoundingClientRect();
      const pointerX = (event.clientX - bounds.left) * width / bounds.width;
      return indicesWithCount[symbolDashboard.nearestPoint(axisOfTime.xs, pointerX)];
    };
    overlay.addEventListener('pointermove', event => hover(indexAt(event)));
    overlay.addEventListener('pointerleave', () => hover(null));
    overlay.addEventListener('click', event => select(indexAt(event)));
    svg.append(overlay);
    // The keys move among the points this chart draws, as the pointer does
    svg.addEventListener('keydown', event => {
      const before = indicesWithCount.filter(index => index < state.selected);
      const after  = indicesWithCount.filter(index => index > state.selected);
      /** @type {Record<string, number | undefined>} */
      const targets = {ArrowLeft: before[before.length - 1], ArrowRight: after[0],
                       Home: indicesWithCount[0], End: indicesWithCount[indicesWithCount.length - 1]};
      if (!(event.key in targets)) return;
      event.preventDefault();
      if (targets[event.key] !== undefined) select(/** @type {number} */ (targets[event.key]));
      hover(isWithinAxis(state.selected) ? state.selected : null, svg);
    });
    svg.addEventListener('focus', () => hover(isWithinAxis(state.selected) ? state.selected : null, svg));
    svg.addEventListener('blur', () => hover(null));
    container.replaceChildren(svg);

    return {
      svg,
      markHover(index) {
        const shown = index === null || !isWithinAxis(index) ? 'hidden' : 'visible';
        crosshair.setAttribute('visibility', shown);
        hoverDot.setAttribute('visibility', index === null || values[index] === null ? 'hidden' : 'visible');
        if (index === null || !isWithinAxis(index)) return;
        crosshair.setAttribute('x1', x(index)); crosshair.setAttribute('x2', x(index));
        if (values[index] !== null) {
          hoverDot.setAttribute('cx', x(index)); hoverDot.setAttribute('cy', y(values[index]));
          paintMarker(hoverDot, hasFailedRun(index));
        }
      },
      markSelected(index) {
        selectDot.setAttribute('visibility', values[index] === null ? 'hidden' : 'visible');
        if (values[index] !== null) { selectDot.setAttribute('cx', x(index)); selectDot.setAttribute('cy', y(values[index])); }
      },
      anchor: index => ({x: x(index) / width, y: (values[index] === null ? margin.top : y(values[index])) / height})
    };
  }

  /** @type {Measure[]} */
  const measures = [
    {name: 'Symbols',              value: record => record.executable.symbols,           unit: 'symbols'},
    {name: 'Bytes of symbol names', value: record => record.executable.symbol_name_bytes, unit: 'bytes of names'},
    {name: 'Instructions of prune\'s analysis', value: symbolDashboard.pruneInstructions, unit: 'instructions',
     exitStatus: symbolDashboard.pruneStampingExitStatus},
    {name: 'Uncalled functions',    value: symbolDashboard.uncalledCount, unit: 'uncalled functions',
     // Measured nightly, so a line joins the points which have a count; the toolchain is that of the measurement
     joinsGaps: true, runOf: point => point.record.uncalled.run,
     changeOf: point => point.record.uncalled ? point.record.uncalled.toolchainChange : null}
  ];

  // One measure for each event of the runner's setup that is plotted. The system calls are not: their total does
  // not grow with the number of tests, so they are not the setup's alone.
  /** @type {Measure[]} */
  const runnerMeasures = ['instructions'].map(event => ({
    event,
    name: `${event} of the runner's setup, per test`,
    value: record => symbolDashboard.runnerSetupPerTest(record, event),
    unit: `${event} per test`,
    exitStatus: symbolDashboard.runnerSetupExitStatus,
    // A record can lack the count, being older than it or its measurement having failed, and the line joins
    // the points which have it; it breaks at a change of the toolchain, or of the boundary of the measurement
    joinsGaps: true, runOf: point => `${point.run}/${point.setupRun}`,
    changeOf: point => point.toolchainChange || point.boundaryChange,
    breakLabel: point => point.boundaryChange ? 'boundary' : 'toolchain'
  }));

  // The setup's cost of leftover entries is plotted per entry, so a point with none has no value and is blind. A
  // record can lack the section, being older than it or its measurement having failed. The points are markers
  // alone: the quotient moves by tens between jobs even at one commit, which a line between jobs a moment apart
  // would draw as a climb, and the scale starts at zero, so that the regression of about 1,650 stands out of that.
  // A rule breaks the series at a change of the toolchain, or of the boundary of the measurement.
  /** @type {Measure} */
  const leftoverMeasure = {
    name: 'instructions of the runner\'s setup, per leftover entry',
    value: symbolDashboard.leftoverMaterialsSetupPerEntry,
    unit: 'instructions per leftover entry',
    exitStatus: symbolDashboard.leftoverMaterialsSetupFirstFailure,
    blindLabel: symbolDashboard.leftoverMaterialsSetupBlindLabel,
    markersOnly: true, includesZero: true,
    changeOf: point => point.toolchainChange || point.leftoverBoundaryChange,
    breakLabel: point => point.leftoverBoundaryChange ? 'boundary' : 'toolchain'
  };

  // The instructions of compiling the units are summed. A record can lack
  // them, being older than them or its measurement having failed, and the
  // line joins the points which have them. The line breaks where the
  // compile-cost record's toolchain changes, or its series: the units or the
  // boundary of the count. The symbol table's toolchain does not break it.
  /** @type {Measure} */
  const compileCostMeasure = {
    name: 'instructions of compiling the units',
    value: symbolDashboard.compileCostInstructions,
    unit: 'instructions',
    joinsGaps: true, runOf: point => point.record.compileCost.run,
    changeOf: point => point.record.compileCost
      ? point.record.compileCost.toolchainChange || point.record.compileCost.seriesChange
      : null,
    breakLabel: point => point.record.compileCost.toolchainChange ? 'toolchain' : 'boundary'
  };

  /**
   * @param {number} index
   * @param {Measure} measure  One whose value every point has
   * @returns {{delta: number, before: number} | null}
   */
  function changeOn(index, measure) {
    if (index === 0) return null;
    const before = /** @type {number} */ (measure.value(state.points[index - 1].record));
    const after  = /** @type {number} */ (measure.value(state.points[index].record));
    return {delta: after - before, before};
  }

  // The tooltip's lines for a record with a `leftover_materials_setup`
  /** @param {any} record */
  function leftoverTooltipLines(record) {
    const perEntry = leftoverMeasure.value(record);
    const runs = symbolDashboard.leftoverMaterialsSetupRuns(record);
    const entries = symbolDashboard.leftoverMaterialsSetupEntries(record);
    return [
      element('span', {}, perEntry === null ? 'The setup cost of leftover materials: no value, since the first run left no entries'
                                            : `The setup cost of leftover materials: ${oneDecimal.format(perEntry)} instructions per leftover entry`),
      element('span', {}, `${integer.format(runs.with_leftovers.instructions)} instructions with ${integer.format(entries)} leftover entries`
                          + ` and ${integer.format(runs.without_leftovers.instructions)} without; the runs executed`
                          + ` ${integer.format(runs.with_leftovers.tests)} tests, with exit statuses ${runs.with_leftovers.exit_status}`
                          + ` with leftovers and ${runs.without_leftovers.exit_status} without`)];
  }

  // The tooltip's lines for a record with a `compile_cost`
  /** @param {any} record */
  function compileCostTooltipLines(record) {
    const units = symbolDashboard.compileCostUnits(record);
    const compiler = record.compileCost.compiler;
    return [
      element('span', {}, `${integer.format(/** @type {number} */ (compileCostMeasure.value(record)))} instructions of compiling the units`
                          + ` with ${compiler.id} ${compiler.version}:`),
      ...Object.entries(units).map(([unit, instructions]) => element('span', {}, `${integer.format(instructions)} for ${unit}`))];
  }

  /**
   * @param {number | null} index
   * @param {Element} [from]
   */
  function hover(index, from) {
    state.hovered = index;
    for (const chart of state.charts) chart.markHover(index);
    const tooltip = /** @type {HTMLElement} */ (document.getElementById('tooltip'));
    if (index === null) { tooltip.hidden = true; return; }
    const point = state.points[index];
    const symbols = changeOn(index, measures[0]);
    tooltip.replaceChildren(
      element('strong', {}, `${integer.format(point.record.executable.symbols)} symbols`),
      element('span', {}, symbols ? `${signed(symbols.delta)} (${percent(symbols.delta, symbols.before)}) on the previous point`
                                  : 'The first point'),
      element('span', {}, `${integer.format(point.record.executable.symbol_name_bytes)} bytes of names`),
      ...(measures[2].value(point.record) === null ? []
          : [element('span', {}, `${integer.format(/** @type {number} */ (measures[2].value(point.record)))} instructions of prune's analysis`
                                 + `, after a full run with exit status ${symbolDashboard.pruneStampingExitStatus(point.record)}`)]),
      ...(runnerMeasures[0].value(point.record) === null ? []
          : [element('span', {}, 'The runner\'s setup, per test: '
                                 + runnerMeasures.map(measure => `${oneDecimal.format(/** @type {number} */ (measure.value(point.record)))} ${measure.event}`)
                                                 .join(', ')
                                 + `; the run executed ${integer.format(symbolDashboard.runnerSetupTests(point.record))} tests`
                                 + ` and had exit status ${symbolDashboard.runnerSetupExitStatus(point.record)}`)]),
      ...(symbolDashboard.leftoverMaterialsSetupEntries(point.record) === null ? [] : leftoverTooltipLines(point.record)),
      ...(symbolDashboard.compileCostUnits(point.record) === null ? [] : compileCostTooltipLines(point.record)),
      ...(measures[3].value(point.record) === null ? []
          : [element('span', {}, `${integer.format(/** @type {number} */ (measures[3].value(point.record)))} uncalled functions in Source`)]),
      element('span', {}, `${short(point.commit)} · ${longDate(point.date)}`),
      element('span', {}, point.subject));
    if (point.toolchainChange) tooltip.append(element('span', {}, `Toolchain changed: ${describeChange(point.toolchainChange)}`));
    const chart  = state.charts.find(c => c.svg === from) || state.charts.find(c => c.svg.matches(':hover')) || state.charts[0];
    const figure = /** @type {HTMLElement} */ (chart.svg.closest('figure'));
    const bounds = chart.svg.getBoundingClientRect();
    const where  = chart.anchor(index);
    const left   = figure.offsetLeft + /** @type {HTMLElement} */ (chart.svg.parentNode).offsetLeft + where.x * bounds.width;
    const top    = figure.offsetTop + /** @type {HTMLElement} */ (chart.svg.parentNode).offsetTop + where.y * bounds.height;
    tooltip.hidden = false;
    const pageWidth = document.documentElement.clientWidth;
    const wanted    = left + 16 + tooltip.offsetWidth > pageWidth ? left - 16 - tooltip.offsetWidth : left + 16;
    tooltip.style.left = `${Math.max(0, Math.min(pageWidth - tooltip.offsetWidth, wanted))}px`;
    tooltip.style.top  = `${Math.max(0, top - tooltip.offsetHeight - 12)}px`;
  }

  /** @param {number} index */
  function select(index) {
    state.selected = index;
    for (const chart of state.charts) chart.markSelected(index);
    drawMovers();
    for (const row of /** @type {NodeListOf<HTMLElement>} */ (document.querySelectorAll('#points tr[data-index]'))) {
      row.classList.toggle('selected', Number(row.dataset.index) === index);
    }
  }

  function drawCharts() {
    state.charts = [
      drawChart(/** @type {HTMLElement} */ (document.getElementById('symbols-plot')), measures[0], 260, true),
      drawChart(/** @type {HTMLElement} */ (document.getElementById('bytes-plot')),   measures[1], 180, false)
    ];
    const hasAnalysis = state.points.some(point => measures[2].value(point.record) !== null);
    /** @type {HTMLElement} */ (document.getElementById('analysis-figure')).hidden = !hasAnalysis;
    if (hasAnalysis) state.charts.push(drawChart(/** @type {HTMLElement} */ (document.getElementById('analysis-plot')), measures[2], 180, false));
    const hasRunner = state.points.some(point => runnerMeasures[0].value(point.record) !== null);
    /** @type {HTMLElement} */ (document.getElementById('runner-figure')).hidden = !hasRunner;
    if (hasRunner) {
      for (const measure of runnerMeasures) {
        state.charts.push(drawChart(/** @type {HTMLElement} */ (document.getElementById(`runner-plot-${measure.event}`)), measure, 120, true));
      }
    }
    const hasLeftover = state.points.some(point => symbolDashboard.leftoverMaterialsSetupEntries(point.record) !== null);
    /** @type {HTMLElement} */ (document.getElementById('leftover-figure')).hidden = !hasLeftover;
    if (hasLeftover) state.charts.push(drawChart(/** @type {HTMLElement} */ (document.getElementById('leftover-plot')), leftoverMeasure, 120, true));
    const hasCompileCost = state.points.some(point => compileCostMeasure.value(point.record) !== null);
    /** @type {HTMLElement} */ (document.getElementById('compile-figure')).hidden = !hasCompileCost;
    if (hasCompileCost) state.charts.push(drawChart(/** @type {HTMLElement} */ (document.getElementById('compile-plot')), compileCostMeasure, 180, true));
    const hasUncalled = state.points.some(point => measures[3].value(point.record) !== null);
    /** @type {HTMLElement} */ (document.getElementById('uncalled-figure')).hidden = !hasUncalled;
    if (hasUncalled) state.charts.push(drawChart(/** @type {HTMLElement} */ (document.getElementById('uncalled-plot')), measures[3], 180, true));
    for (const chart of state.charts) chart.markSelected(state.selected);
  }

  // Panels -----------------------------------------------------------------

  function drawTiles() {
    const points  = state.points;
    const latest  = points[points.length - 1];
    const change  = changeOn(points.length - 1, measures[0]);
    /** @param {string} label @param {string} value @param {string} detail */
    const tile = (label, value, detail) => element('div', {class: 'tile'},
      element('div', {class: 'label'}, label), element('div', {class: 'value'}, value), element('div', {class: 'detail'}, detail));
    /** @type {HTMLElement} */ (document.getElementById('tiles')).replaceChildren(
      tile('Symbols at the latest point', integer.format(latest.record.executable.symbols), `${short(latest.commit)}, ${longDate(latest.date)}`),
      tile('Change on the point before', change ? signed(change.delta) : '—',
           change ? percent(change.delta, change.before) + (latest.toolchainChange ? ', across a toolchain change' : '') : 'There is only one point'),
      tile('Points', integer.format(points.length), `${longDate(points[0].date)} to ${longDate(latest.date)}`));
  }

  // The paragraphs which say how each unit's count of instructions moved at the point `index`, against the
  // point before it with a count, or none if the point has no count or the point before is of another series
  /** @param {number} index */
  function compileCostMovers(index) {
    const units = symbolDashboard.compileCostUnits(state.points[index].record);
    const before = state.points.slice(0, index).findLast(point => symbolDashboard.compileCostUnits(point.record) !== null);
    if (units === null || before === undefined) return [];
    const compileCost = state.points[index].record.compileCost;
    if (compileCost.toolchainChange || compileCost.seriesChange) {
      return [element('p', {class: 'note'}, 'The count of compiling the units is of another toolchain or series than at '
        + `${short(before.commit)}, the point before with a count, so it is not compared.`)];
    }
    const previous = symbolDashboard.compileCostUnits(before.record);
    const moved = Object.entries(units).map(([unit, instructions]) =>
      `${unit} ${signed(instructions - previous[unit])} (${percent(instructions - previous[unit], previous[unit])})`);
    return [element('p', {}, 'Against ', link(commitUrl(before.commit), short(before.commit)),
      `, the point before with a count, the instructions of compiling the units moved: ${moved.join('; ')}.`)];
  }

  function drawMovers() {
    const panel = /** @type {HTMLElement} */ (document.getElementById('movers'));
    const index = state.selected;
    const point = state.points[index];
    panel.replaceChildren(element('h2', {}, 'What moved at ', link(commitUrl(point.commit), short(point.commit))),
                          element('p', {class: 'note'}, `${longDate(point.date)}: ${point.subject}`));
    if (index === 0) {
      panel.append(element('p', {}, 'This is the first point, so there is nothing to compare it with.'));
      return;
    }
    const previous = state.points[index - 1];
    const symbols  = /** @type {{delta: number, before: number}} */ (changeOn(index, measures[0]));
    const movers   = symbolDashboard.movers(previous.record, point.record);
    const objects  = Object.keys(point.record.defined_symbols_by_object).length;
    panel.append(element('p', {}, 'Against ', link(commitUrl(previous.commit), short(previous.commit)),
      `, the point before, the executable's symbols went from ${integer.format(symbols.before)} to `
      + `${integer.format(point.record.executable.symbols)} (${signed(symbols.delta)}, ${percent(symbols.delta, symbols.before)}). `
      + `${movers.length} of ${objects} objects changed their count of defined symbols.`));
    if (point.toolchainChange) {
      panel.append(element('p', {class: 'note'}, `The toolchain changed between these points, so the movement may be the toolchain's: `
        + describeChange(point.toolchainChange)));
    }
    panel.append(...compileCostMovers(index));
    if (movers.length === 0) return;
    /** @param {Mover[]} rows */
    const table = rows => {
      const body = rows.map(mover => element('tr', {},
        element('td', {class: 'object'}, mover.object),
        element('td', {class: 'number'}, mover.before === null ? 'absent' : integer.format(mover.before)),
        element('td', {class: 'number'}, mover.after === null ? 'absent' : integer.format(mover.after)),
        element('td', {class: 'number'}, signed(mover.delta))));
      return element('div', {class: 'scroll'}, element('table', {},
        element('thead', {}, element('tr', {}, element('th', {}, 'Object, by its source'), element('th', {class: 'number'}, 'Before'),
                                       element('th', {class: 'number'}, 'After'), element('th', {class: 'number'}, 'Change'))),
        element('tbody', {}, ...body)));
    };
    panel.append(table(movers.slice(0, topMovers)));
    if (movers.length > topMovers) {
      panel.append(element('details', {}, element('summary', {}, `The other ${movers.length - topMovers} objects that moved`),
                           table(movers.slice(topMovers))));
    }
  }

  function drawPointsTable() {
    const header = element('tr', {}, ...['Commit', 'Date', 'Symbols', 'Change', 'Bytes of names', 'Subject'].map((text, i) =>
      element('th', {class: i >= 2 && i <= 4 ? 'number' : ''}, text)));
    /** @type {HTMLElement[]} */
    const rows = [];
    state.points.forEach((point, index) => {
      if (point.toolchainChange) {
        rows.push(element('tr', {class: 'change'}, element('td', {colspan: 6}, `Toolchain changed: ${describeChange(point.toolchainChange)}`)));
      }
      const change = changeOn(index, measures[0]);
      const row = element('tr', {'data-index': index},
        element('td', {}, link(commitUrl(point.commit), short(point.commit))),
        element('td', {}, longDate(point.date)),
        element('td', {class: 'number'}, integer.format(point.record.executable.symbols)),
        element('td', {class: 'number'}, change ? signed(change.delta) : ''),
        element('td', {class: 'number'}, integer.format(point.record.executable.symbol_name_bytes)),
        element('td', {}, point.subject));
      row.tabIndex = 0;
      row.addEventListener('click', () => select(index));
      row.addEventListener('keydown', event => {
        if (event.key === 'Enter' || event.key === ' ') { event.preventDefault(); select(index); }
      });
      rows.push(row);
    });
    /** @type {HTMLElement} */ (document.getElementById('points')).replaceChildren(element('thead', {}, header), element('tbody', {}, ...rows.reverse()));
  }

  /** @param {Column} column */
  function drawProblems(column) {
    const panel = /** @type {HTMLElement} */ (document.getElementById('problems'));
    panel.hidden = column.problems.length === 0;
    panel.replaceChildren(element('h2', {}, `${column.problems.length} records not plotted`),
      element('ul', {}, ...column.problems.map(({path, problems}) => element('li', {}, element('code', {}, path), `: ${problems.join('; ')}.`))));
  }

  /** @param {Column} column */
  function showColumn(column) {
    state.points = column.points;
    drawProblems(column);
    const dashboard = /** @type {HTMLElement} */ (document.getElementById('dashboard'));
    dashboard.hidden = false;
    const hasPoints = column.points.length > 0;
    for (const node of /** @type {HTMLCollectionOf<HTMLElement>} */ (dashboard.children)) if (node.id !== 'problems' && node.id !== 'tooltip') node.hidden = !hasPoints;
    if (!hasPoints) {
      showMessage('Nothing to plot', `None of the records of ${column.name} can be plotted; the list below says why.`, true);
      return;
    }
    /** @type {HTMLElement} */ (document.getElementById('message')).hidden = true;
    const asked = parameters.get('commit');
    const found = asked ? column.points.findIndex(point => point.commit.startsWith(asked)) : -1;
    state.selected = found >= 0 ? found : column.points.length - 1;
    drawTiles();
    drawCharts();
    drawPointsTable();
    select(state.selected);
  }

  // `loaded` is the result of symbolDashboard.loadHistory, or null
  /** @param {any} loaded */
  function describeCalls(loaded) {
    const footer = /** @type {HTMLElement} */ (document.getElementById('footer'));
    /** @param {number} count @param {string} noun */
    const plural = (count, noun) => `${count} ${noun}${count === 1 ? '' : 's'}`;
    const pages  = loaded && loaded.state === 'loaded' ? loaded.pages : 0;
    const read   = `This view read ${plural(meter.records, 'record')} and ${plural(pages, 'page')} of staging's history`;
    if (fixtures) {
      footer.textContent = `Preview: ${read} from ${fixtures}, not from GitHub.`;
      return;
    }
    const remaining = meter.remaining === null ? '.' : `; ${meter.remaining} remain until ${clock(meter.reset)}.`;
    footer.textContent = `${read}, with ${plural(meter.calls, 'call')} to GitHub's API. The API allows a reader who is not `
                       + `signed in 60 calls an hour${remaining} The records come from raw.githubusercontent.com, `
                       + 'which the limit does not count.';
  }

  async function main() {
    const source = fixtures ? fixtureSource(fixtures) : githubSource();
    const known  = knownCommits();
    let loaded;
    try { loaded = await symbolDashboard.loadHistory(source, known); }
    catch (error) {
      showMessage('The records could not be loaded', /** @type {Error} */ (error).message, true);
      describeCalls(null);
      return;
    }
    finally { remember(known); }
    describeCalls(loaded);
    if (loaded.state === 'absent') {
      showMessage('No records yet', 'The metrics branch does not exist yet, so there is nothing to plot. CI creates it when it stores the first record.', false);
      return;
    }
    if (loaded.state === 'empty') {
      showMessage('No records yet', 'The metrics branch holds no records under symbol-table/, so there is nothing to plot.', false);
      return;
    }
    const columns = new Map(loaded.columns.map((/** @type {Column} */ column) => [column.name, column]));
    const chosen  = parameters.get('column') || (columns.has(defaultColumn) ? defaultColumn : loaded.columns[0].name);
    if (!columns.has(chosen)) {
      showMessage('No such column', `The metrics branch has no column ${chosen}; it has ${[...columns.keys()].join(', ')}.`, true);
      return;
    }
    const picker = /** @type {HTMLSelectElement} */ (document.getElementById('column'));
    picker.replaceChildren(...[...columns.keys()].map(name => element('option', {value: name}, name)));
    picker.value = chosen;
    picker.addEventListener('change', () => showColumn(/** @type {Column} */ (columns.get(picker.value))));
    /** @type {HTMLElement} */ (document.getElementById('filters')).hidden = columns.size < 2;
    showColumn(/** @type {Column} */ (columns.get(chosen)));
    // Only a change of width redraws, since redrawing replaces the focused
    // chart and so ends keyboard navigation.
    /** @type {number | null} */
    let pending = null, drawnWidth = /** @type {Element} */ (document.querySelector('main')).clientWidth;
    new ResizeObserver(([entry]) => {
      if (entry.contentRect.width === drawnWidth) return;
      drawnWidth = entry.contentRect.width;
      cancelAnimationFrame(/** @type {number} */ (pending));
      pending = requestAnimationFrame(() => { if (state.points.length) { drawCharts(); } });
    }).observe(/** @type {Element} */ (document.querySelector('main')));
  }

  main();
})();

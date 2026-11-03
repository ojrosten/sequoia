// What the dashboard's scripts meet outside the language: Node's `module`,
// which the selftest provides; the page's `window`, which names the exports
// of symbol_dashboard.js, and so is the global symbolDashboard that
// symbol_dashboard_page.js reads; and the DOM's conversion of an attribute's
// value to a string, which the library's types do not allow to be a number.
declare const module: {exports: Record<string, any>} | undefined;
interface Window { symbolDashboard: Record<string, any>; }
declare var symbolDashboard: Record<string, any>;
interface Element { setAttribute(qualifiedName: string, value: string | number): void; }

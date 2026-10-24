// What symbol_dashboard.js meets outside the language: Node's `module`, which
// the selftest provides, and the page's `window`, which names the script's
// exports.
declare const module: {exports: Record<string, any>} | undefined;
interface Window { symbolDashboard: Record<string, any>; }

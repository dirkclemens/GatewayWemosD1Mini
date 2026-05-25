#include <Arduino.h>
const char style_css[] PROGMEM = R"====(
:root {
    --primary: #0ea5e9;
    --primary-dk: #0369a1;
    --bg: #f1f5f9;
    --surface: #ffffff;
    --border: #e2e8f0;
    --text: #1e293b;
    --muted: #64748b;
    --ok: #22c55e;
    --warn: #f59e0b;
    --err: #ef4444;
    --nav-h: 52px;
    --radius: 8px;
}
* { box-sizing: border-box; margin: 0; padding: 0; }
body {
    font-family: system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
    background: var(--bg);
    color: var(--text);
    font-size: 14px;
    line-height: 1.5;
}

/* ── Navigation ──────────────────────────────────────────────────────── */
nav {
    position: fixed; top: 0; left: 0; right: 0;
    height: var(--nav-h);
    background: var(--surface);
    border-bottom: 1px solid var(--border);
    box-shadow: 0 1px 4px rgba(0,0,0,.07);
    z-index: 100;
    display: flex; align-items: center;
    padding: 0 12px; gap: 8px;
}
.nav-logo { width: 32px; height: 32px; flex-shrink: 0; }
.nav-title {
    font-weight: 600; font-size: 13px;
    color: var(--primary); white-space: nowrap;
    display: none;
}
.nav-tabs { display: flex; gap: 3px; flex: 1; flex-wrap: wrap; }
.tab {
    background: none; border: none; cursor: pointer;
    padding: 5px 11px; border-radius: var(--radius);
    color: var(--muted); font-size: 13px; font-weight: 500;
    transition: background .15s, color .15s;
    white-space: nowrap;
}
.tab:hover { background: var(--bg); color: var(--text); }
.tab.active { background: var(--primary); color: #fff; }
.conn-status {
    font-size: 11px; white-space: nowrap;
    padding: 3px 8px; border-radius: 20px;
    border: 1px solid var(--border);
}
.conn-ok { color: var(--ok); border-color: var(--ok); }
.conn-err { color: var(--err); border-color: var(--err); }

/* ── Pages ───────────────────────────────────────────────────────────── */
main { padding-top: calc(var(--nav-h) + 10px); }
.page { display: none; padding: 12px 14px; max-width: 1400px; margin: 0 auto; }
.page.active { display: block; }
.section-title {
    font-size: 15px; font-weight: 600;
    margin-bottom: 10px; color: var(--text);
}

/* ── Messages table ──────────────────────────────────────────────────── */
.msg-wrap { overflow-x: auto; }
.msg-table {
    width: 100%; border-collapse: collapse;
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    overflow: hidden;
}
.msg-table thead { background: #f8fafc; }
.msg-table th {
    padding: 7px 9px; text-align: left;
    font-size: 11px; font-weight: 600;
    text-transform: uppercase; letter-spacing: .04em;
    color: var(--muted);
    border-bottom: 1px solid var(--border);
}
.msg-table td { padding: 5px 9px; border-bottom: 1px solid #f1f5f9; font-size: 13px; }
.msg-table tr:hover td { background: #f8fafc; }
.msg-table td.payload { font-family: monospace; color: var(--primary-dk); }
.msg-table .cmd-set  { color: var(--ok); font-weight: 600; }
.msg-table .cmd-int  { color: var(--muted); }
.msg-table .cmd-prs  { color: var(--warn); }
.msg-controls {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-bottom: 8px;
}
.msg-controls label {
    color: var(--muted);
    font-size: 12px;
    font-weight: 600;
    text-transform: uppercase;
}
.msg-controls select {
    padding: 5px 8px;
    border: 1px solid var(--border);
    border-radius: 6px;
    background: var(--surface);
    color: var(--text);
    font-size: 12px;
}

/* ── Sensor cards ────────────────────────────────────────────────────── */
.sensor-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(230px, 1fr));
    gap: 12px;
}
.node-card {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    overflow: hidden;
    box-shadow: 0 1px 3px rgba(0,0,0,.05);
}
.node-header {
    background: var(--primary);
    color: #fff;
    padding: 8px 12px;
    display: flex; align-items: center; gap: 8px;
}
.node-id-badge {
    background: rgba(255,255,255,.25);
    border-radius: 4px;
    padding: 1px 6px;
    font-size: 11px; font-weight: 700;
    font-family: monospace;
}
.node-name { font-weight: 600; font-size: 14px; flex: 1; }
.sensor-list { padding: 2px 0; }
.sensor-row {
    display: flex; align-items: center;
    padding: 5px 12px; gap: 6px;
    border-bottom: 1px solid #f8fafc;
    font-size: 13px;
}
.sensor-row:last-child { border-bottom: none; }
.s-sid { color: var(--muted); font-size: 11px; min-width: 18px; font-family: monospace; }
.s-type { color: var(--muted); font-size: 11px; flex: 1; min-width: 60px; }
.s-val { font-weight: 600; font-family: monospace; color: var(--text); }
.s-time { color: var(--muted); font-size: 11px; margin-left: auto; white-space: nowrap; }
.empty-hint { color: var(--muted); padding: 24px; text-align: center; }

/* ── Info / status ───────────────────────────────────────────────────── */
#page-info {
    flex-direction: column;
    align-items: center;
}
#page-info.active { display: flex; }
#page-info #infoContent {
    width: 100%;
    text-align: left;
    justify-content: left;
}
#page-info #gwClientsContent {
    width: 100%;
    text-align: center;
    justify-content: center;
}
.info-status-line {
    width: 100%;
    // max-width: 920px;
    display: flex;
    justify-content: left;
    align-items: left;
    gap: 10px;
    flex-wrap: wrap;
    margin: 4px 10px 10px;
}
.info-chip {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    padding: 6px 10px;
    border-radius: 999px;
    border: 1px solid var(--border);
    background: var(--surface);
}
.info-chip .k {
    font-size: 11px;
    color: var(--muted);
    text-transform: uppercase;
    letter-spacing: .04em;
}
.info-chip .v {
    font-size: 12px;
    color: var(--text);
    font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace;
}
#infoContent table { width: 100%; max-width: 620px; border-collapse: collapse; }
#page-info #infoContent table,
#page-info #gwClientsContent table { margin: 0 auto; }
#infoContent td { padding: 5px 10px; border-bottom: 1px solid var(--border); font-size: 13px; }
#infoContent td:first-child {
    color: var(--muted); font-size: 12px;
    text-align: right; padding-right: 16px; width: 42%;
}
#page-info .action-row { justify-content: center; }

/* ── Badges ──────────────────────────────────────────────────────────── */
.badges-row { display: flex; flex-wrap: wrap; gap: 6px; margin-bottom: 10px; }
.badge {
    display: inline-block; padding: 4px 10px;
    border-radius: 20px; background: var(--surface);
    border: 1px solid var(--border); font-size: 12px;
    color: var(--muted);
}

/* ── Canvas chart ────────────────────────────────────────────────────── */
#diagChart {
    border: 1px solid var(--border);
    border-radius: var(--radius);
    background: var(--surface);
    display: block; max-width: 100%;
}

/* ── Log area ────────────────────────────────────────────────────────── */
.log-controls {
    display: flex; align-items: center; gap: 6px;
    margin: 10px 0 6px;
}
.log-section { margin-bottom: 10px; }
.log-label { font-size: 11px; font-weight: 600; text-transform: uppercase;
    letter-spacing: .05em; color: var(--muted); margin-bottom: 4px; }
.log-box {
    background: #0f172a; border-radius: var(--radius);
    padding: 8px 10px;
    font-family: 'Courier New', monospace;
    font-size: 12px; line-height: 1.6;
    min-height: 60px; max-height: 220px; overflow-y: auto;
}
.logline { display: block; padding: 1px 0; border-bottom: 1px solid rgba(255,255,255,.04); }
.logline .ts { color: #475569; }
.logline .lv { display: inline-block; min-width: 46px; font-weight: bold; }
.logline.error .lv { color: #f87171; }
.logline.warn  .lv { color: #fbbf24; }
.logline.info  .lv { color: #60a5fa; }
.logline .txt  { color: #cbd5e1; }

/* ── Buttons ─────────────────────────────────────────────────────────── */
.btn, button {
    display: inline-flex; align-items: center; gap: 4px;
    padding: 6px 13px; border: none; border-radius: 6px;
    background: var(--primary); color: #fff;
    cursor: pointer; font-size: 13px; font-weight: 500;
    text-decoration: none; white-space: nowrap;
    transition: background .15s;
}
.btn:hover, button:hover { background: var(--primary-dk); }
.btn-sm { padding: 4px 9px; font-size: 12px; }
.btn-secondary { background: #94a3b8; }
.btn-secondary:hover { background: #64748b; }
.btn-danger { background: var(--err); }
.btn-danger:hover { background: #dc2626; }
.action-row { display: flex; flex-wrap: wrap; gap: 8px; margin-top: 14px; }

/* ── Forms ───────────────────────────────────────────────────────────── */
input[type=text] {
    padding: 6px 10px;
    border: 1px solid var(--border); border-radius: 6px;
    font-size: 13px; color: var(--text);
    background: var(--surface);
}
input[type=text]:focus { outline: 2px solid var(--primary); border-color: var(--primary); }
.form-row { display: flex; gap: 8px; align-items: center; flex-wrap: wrap; margin-bottom: 10px; }

.settings-stack {
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    gap: 6px;
}
.settings-stack label {
    display: block;
}

/* ── Node names table ────────────────────────────────────────────────── */
.nodes-table {
    width: 100%; max-width: 400px;
    border-collapse: collapse;
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    overflow: hidden;
}
.nodes-table th, .nodes-table td { padding: 6px 12px; border-bottom: 1px solid var(--border); text-align: left; }
.nodes-table th { font-weight: 600; font-size: 12px; color: var(--muted); background: #f8fafc; }

/* ── Responsive ──────────────────────────────────────────────────────── */
@media (min-width: 640px) {
    .nav-title { display: block; }
}
@media (max-width: 600px) {
    .tab .lbl { display: none; }
    .msg-table th:nth-child(5),
    .msg-table td:nth-child(5),
    .msg-table th:nth-child(6),
    .msg-table td:nth-child(6) { display: none; }
}
@media (max-width: 420px) {
    .msg-table th:nth-child(4),
    .msg-table td:nth-child(4) { display: none; }
    nav { padding: 0 8px; gap: 4px; }
    .tab { padding: 5px 7px; font-size: 12px; }
}
)====";

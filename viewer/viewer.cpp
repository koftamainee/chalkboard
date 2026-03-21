#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <httplib.h>

namespace fs = std::filesystem;

static constexpr int    k_port         = 8080;
static constexpr size_t k_history_size = 16;

static std::mutex                    g_mutex;
static std::vector<std::string>      g_history;
static size_t                        g_history_index = 0;
static std::vector<httplib::DataSink*> g_sinks;
static std::atomic<bool>             g_running{true};

static std::string timestamp() {
    const auto now  = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time), "%H:%M:%S");
    return ss.str();
}

static void log(const std::string& method, const std::string& path, const int status = 0) {
    std::cout << "[" << timestamp() << "] " << method;
    if (!path.empty()) {
        std::cout << " " << path;
    }
    if (status != 0) {
        std::cout << " -> " << status;
    }
    std::cout << "\n";
}

static void cleanup_current() {
    if (!g_history.empty() && fs::exists(g_history[g_history_index])) {
        fs::remove_all(g_history[g_history_index]);
    }
}

static void signal_handler(int) {
    g_running = false;
    std::lock_guard lock(g_mutex);
    cleanup_current();
    std::cout << "\n[" << timestamp() << "] shutting down\n";
    std::exit(0);
}

static std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        return "";
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string extract_body(const std::string& html) {
    auto start = html.find("<body");
    if (start == std::string::npos) {
        return html;
    }
    start = html.find('>', start);
    if (start == std::string::npos) {
        return html;
    }
    ++start;
    const auto end = html.rfind("</body>");
    if (end == std::string::npos) {
        return html.substr(start);
    }
    return html.substr(start, end - start);
}

static std::string json_escape(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (const char c : s) {
        if (c == '"')       { r += "\\\""; }
        else if (c == '\\') { r += "\\\\"; }
        else if (c == '\n') { r += "\\n";  }
        else if (c == '\r') { r += "\\r";  }
        else                { r += c;      }
    }
    return r;
}

static void notify_clients(const std::string& event) {
    const std::string msg = "data: " + event + "\n\n";
    std::lock_guard lock(g_mutex);
    for (const auto* sink : g_sinks) {
        (void)sink->write(msg.c_str(), msg.size());
    }
}

static std::string build_artifact_json(const size_t index) {
    std::lock_guard lock(g_mutex);
    if (g_history.empty()) {
        return "";
    }
    const std::string& dir        = g_history[index];
    const std::string        index_path = dir + "/index.html";
    const std::string        html       = read_file(index_path);
    if (html.empty()) {
        return "";
    }
    const std::string body        = extract_body(html);
    const std::string name        = fs::path(dir).filename().string();
    const bool        has_prev    = index > 0;
    const bool        has_next    = index + 1 < g_history.size();

    std::ostringstream json;
    json << "{"
         << "\"body\":\""    << json_escape(body) << "\","
         << "\"name\":\""    << json_escape(name) << "\","
         << "\"index\":"     << index             << ","
         << "\"total\":"     << g_history.size()  << ","
         << "\"has_prev\":"  << (has_prev ? "true" : "false") << ","
         << "\"has_next\":"  << (has_next ? "true" : "false")
         << "}";
    return json.str();
}

static std::string shell_html() {
    return R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Viewer</title>
<script>
  window.MathJax = {
    tex: {
      inlineMath: [['\\(','\\)']],
      displayMath: [['\\[','\\]']],
      tags: 'ams'
    },
    options: { skipHtmlTags: ['script','noscript','style','textarea','pre'] }
  };
</script>
<script src="https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-chtml.js" async></script>
<style>
  :root {
    --bg:        #0f0c1f;
    --surface:   #1b1733;
    --card:      #2a244a;
    --border:    #3a2f60;
    --accent:    #8b5cf6;
    --danger:    #f87171;
    --warn:      #fbbf24;
    --success:   #34d399;
    --txt:       #e0d9ff;
    --muted:     #a5a1b8;
    --font-body: 'Georgia', 'Times New Roman', serif;
    --font-mono: 'JetBrains Mono', 'Fira Code', 'Courier New', monospace;
    --font-ui:   'Segoe UI', 'Helvetica Neue', sans-serif;
    --radius:    8px;
  }

  *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

  body {
    background: var(--bg);
    color: var(--txt);
    font-family: var(--font-body);
    font-size: 17px;
    line-height: 1.75;
    min-height: 100vh;
  }

  #statusbar {
    position: fixed;
    top: 0; left: 0; right: 0;
    height: 36px;
    background: var(--surface);
    border-bottom: 1px solid var(--border);
    display: flex;
    align-items: center;
    padding: 0 20px;
    gap: 10px;
    z-index: 100;
    font-family: var(--font-ui);
    font-size: 0.78rem;
    color: var(--muted);
  }

  #statusbar .dot {
    width: 7px; height: 7px;
    border-radius: 50%;
    background: var(--muted);
    transition: background 0.3s;
    flex-shrink: 0;
  }
  #statusbar .dot.connected { background: var(--success); }
  #statusbar .dot.error     { background: var(--danger); }

  #statusbar .label { letter-spacing: 0.05em; }

  #statusbar .artifact-name {
    margin-left: auto;
    color: var(--accent);
    font-family: var(--font-mono);
    font-size: 0.75rem;
  }

  .nav-btn {
    background: none;
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    font-family: var(--font-ui);
    font-size: 0.75rem;
    padding: 2px 8px;
    cursor: pointer;
    transition: color 0.2s, border-color 0.2s;
  }
  .nav-btn:hover:not(:disabled) { color: var(--accent); border-color: var(--accent); }
  .nav-btn:disabled { opacity: 0.3; cursor: default; }

  #history-counter {
    font-family: var(--font-mono);
    font-size: 0.72rem;
    color: var(--muted);
    min-width: 40px;
    text-align: center;
  }

  #search-bar {
    display: none;
    position: fixed;
    top: 36px; left: 0; right: 0;
    height: 38px;
    background: var(--surface);
    border-bottom: 1px solid var(--border);
    align-items: center;
    padding: 0 16px;
    gap: 10px;
    z-index: 99;
    font-family: var(--font-ui);
    font-size: 0.82rem;
  }
  #search-bar.visible { display: flex; }

  #search-input {
    flex: 1;
    background: var(--card);
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--txt);
    font-family: var(--font-mono);
    font-size: 0.82rem;
    padding: 3px 10px;
    outline: none;
  }
  #search-input:focus { border-color: var(--accent); }

  #search-count {
    color: var(--muted);
    font-size: 0.75rem;
    min-width: 60px;
  }

  .search-btn {
    background: none;
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    font-size: 0.75rem;
    padding: 2px 8px;
    cursor: pointer;
  }
  .search-btn:hover { color: var(--accent); border-color: var(--accent); }

  mark.search-hit {
    background: rgba(139, 92, 246, 0.35);
    color: inherit;
    border-radius: 2px;
    outline: none;
  }
  mark.search-hit.current {
    background: var(--accent);
    color: #fff;
  }

  #content {
    max-width: 860px;
    margin: 0 auto;
    padding: 60px 28px 80px;
    transition: padding-top 0.15s;
  }
  #content.search-open { padding-top: 96px; }

  #error-banner {
    display: none;
    background: rgba(248, 113, 113, 0.12);
    border: 1px solid var(--danger);
    border-radius: var(--radius);
    color: var(--danger);
    font-family: var(--font-ui);
    font-size: 0.85rem;
    padding: 12px 18px;
    margin-bottom: 18px;
  }
  #error-banner.visible { display: block; }

  #placeholder {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    min-height: calc(100vh - 36px);
    gap: 16px;
    color: var(--muted);
    font-family: var(--font-ui);
  }
  #placeholder .icon { font-size: 3rem; opacity: 0.3; }
  #placeholder p { font-size: 0.9rem; letter-spacing: 0.03em; }

  #content h1 {
    font-family: var(--font-ui);
    font-size: clamp(1.6rem, 4vw, 2.2rem);
    font-weight: 700;
    color: #fff;
    letter-spacing: -0.02em;
    margin-bottom: 8px;
    padding-bottom: 20px;
    border-bottom: 1px solid var(--border);
    position: relative;
  }
  #content h1::after {
    content: '';
    position: absolute;
    bottom: -1px; left: 0;
    width: 56px; height: 2px;
    background: linear-gradient(90deg, var(--accent), #34d399);
    border-radius: 2px;
  }

  #content h2 {
    font-family: var(--font-ui);
    font-size: 1.3rem;
    font-weight: 700;
    color: #fff;
    margin: 44px 0 14px;
    padding-bottom: 10px;
    border-bottom: 1px solid var(--border);
    position: relative;
  }
  #content h2::after {
    content: '';
    position: absolute;
    bottom: -1px; left: 0;
    width: 36px; height: 2px;
    background: var(--accent);
    border-radius: 2px;
  }

  #content h3 {
    font-family: var(--font-ui);
    font-size: 1.05rem;
    font-weight: 600;
    color: var(--accent);
    margin: 28px 0 10px;
  }

  #content h4 {
    font-family: var(--font-ui);
    font-size: 0.95rem;
    font-weight: 600;
    color: var(--muted);
    margin: 20px 0 8px;
    text-transform: uppercase;
    letter-spacing: 0.06em;
  }

  #content p { margin: 12px 0; color: var(--txt); }

  #content p.math {
    background: var(--card);
    border: 1px solid var(--border);
    border-left: 3px solid var(--accent);
    border-radius: var(--radius);
    padding: 18px 22px;
    margin: 18px 0;
    overflow-x: auto;
    position: relative;
  }

  .copy-latex-btn {
    position: absolute;
    top: 8px; right: 10px;
    background: none;
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--muted);
    font-family: var(--font-ui);
    font-size: 0.68rem;
    padding: 2px 7px;
    cursor: pointer;
    opacity: 0;
    transition: opacity 0.2s, color 0.2s, border-color 0.2s;
    letter-spacing: 0.04em;
  }
  #content p.math:hover .copy-latex-btn { opacity: 1; }
  .copy-latex-btn:hover { color: var(--accent); border-color: var(--accent); }
  .copy-latex-btn.copied { color: var(--success); border-color: var(--success); opacity: 1; }

  #content ul, #content ol { margin: 12px 0 12px 24px; color: var(--txt); }
  #content li { margin: 6px 0; }
  #content li::marker { color: var(--accent); }

  #content pre {
    background: var(--card);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 18px 22px;
    overflow-x: auto;
    margin: 18px 0;
  }

  #content code {
    font-family: var(--font-mono);
    font-size: 0.875rem;
    color: var(--txt);
    line-height: 1.6;
  }

  #content header { margin-bottom: 32px; }

  #content .timestamp {
    font-family: var(--font-mono);
    font-size: 0.75rem;
    color: var(--muted);
    margin-top: 6px;
    letter-spacing: 0.04em;
  }

  #content table {
    width: 100%;
    border-collapse: collapse;
    margin: 18px 0;
    font-family: var(--font-mono);
    font-size: 0.875rem;
  }

  #content th {
    background: var(--card);
    color: var(--accent);
    font-weight: 600;
    padding: 10px 16px;
    border: 1px solid var(--border);
    text-align: left;
    letter-spacing: 0.04em;
    font-size: 0.78rem;
    text-transform: uppercase;
  }

  #content td {
    padding: 9px 16px;
    border: 1px solid var(--border);
    color: var(--txt);
  }

  #content tr:hover td { background: rgba(139,92,246,0.05); }

  @keyframes fadeIn {
    from { opacity: 0; transform: translateY(8px); }
    to   { opacity: 1; transform: translateY(0); }
  }
  .fade-in { animation: fadeIn 0.25s ease forwards; }
</style>
</head>
<body>

<div id="statusbar">
  <div class="dot" id="dot"></div>
  <span class="label" id="status-label">connecting...</span>
  <button class="nav-btn" id="btn-prev" disabled>&#8592; prev</button>
  <span id="history-counter">—</span>
  <button class="nav-btn" id="btn-next" disabled>next &#8594;</button>
  <button class="nav-btn" id="btn-search" title=\"Search (Ctrl+F)\">&#128269;</button>
  <span class="artifact-name" id="artifact-name"></span>
</div>

<div id="search-bar">
  <input id="search-input" type="text" placeholder="search..." autocomplete="off" spellcheck="false" />
  <span id="search-count"></span>
  <button class="search-btn" id="btn-search-prev">&#8593;</button>
  <button class="search-btn" id="btn-search-next">&#8595;</button>
  <button class="search-btn" id="btn-search-close">&#10005;</button>
</div>

<div id="content">
  <div id="error-banner"></div>
  <div id="placeholder">
    <div class="icon">&#x2B21;</div>
    <p>Waiting for artifact...</p>
  </div>
</div>

<script>
  const dot           = document.getElementById('dot');
  const statusLabel   = document.getElementById('status-label');
  const artifactName  = document.getElementById('artifact-name');
  const content       = document.getElementById('content');
  const errorBanner   = document.getElementById('error-banner');
  const btnPrev       = document.getElementById('btn-prev');
  const btnNext       = document.getElementById('btn-next');
  const historyCounter = document.getElementById('history-counter');
  const searchBar     = document.getElementById('search-bar');
  const searchInput   = document.getElementById('search-input');
  const searchCount   = document.getElementById('search-count');
  const btnSearchPrev = document.getElementById('btn-search-prev');
  const btnSearchNext = document.getElementById('btn-search-next');
  const btnSearchClose = document.getElementById('btn-search-close');
  const btnSearchOpen = document.getElementById('btn-search');

  let g_current_index = 0;
  let g_search_hits   = [];
  let g_search_cursor = 0;

  function set_status(state, label) {
    dot.className       = 'dot ' + state;
    statusLabel.textContent = label;
  }

  function show_error(msg) {
    errorBanner.textContent = msg;
    errorBanner.classList.add('visible');
  }

  function clear_error() {
    errorBanner.classList.remove('visible');
  }

  function update_nav(data) {
    btnPrev.disabled     = !data.has_prev;
    btnNext.disabled     = !data.has_next;
    historyCounter.textContent = (data.index + 1) + ' / ' + data.total;
    g_current_index      = data.index;
  }

  function attach_copy_buttons() {
    document.querySelectorAll('#content p.math').forEach(block => {
      if (block.querySelector('.copy-latex-btn') !== null) {
        return;
      }
      const raw = block.getAttribute('data-latex') || block.textContent.trim();
      const btn = document.createElement('button');
      btn.className   = 'copy-latex-btn';
      btn.textContent = 'copy LaTeX';
      btn.addEventListener('click', async () => {
        await navigator.clipboard.writeText(raw);
        btn.textContent = 'copied!';
        btn.classList.add('copied');
        setTimeout(() => {
          btn.textContent = 'copy LaTeX';
          btn.classList.remove('copied');
        }, 1500);
      });
      block.appendChild(btn);
    });
  }

  async function load_artifact(index) {
    const url = (index === undefined) ? '/artifact' : '/artifact?index=' + index;
    try {
      const res = await fetch(url);
      if (res.status === 204) {
        return;
      }
      if (!res.ok) {
        show_error('Failed to load artifact (HTTP ' + res.status + ')');
        return;
      }
      const data = await res.json();
      clear_error();
      content.innerHTML = '<div id="error-banner"></div>' + data.body;
      content.className = 'fade-in';
      const formatted_name = data.name.replace(/^report_/, '').replaceAll('_', ' ');
      artifactName.textContent = formatted_name;
      document.title    = formatted_name;
      update_nav(data);
      attach_copy_buttons();
      if (window.MathJax) {
        await MathJax.typesetPromise([content]);
      }
      if (searchBar.classList.contains('visible')) {
        run_search(searchInput.value);
      }
    } catch(e) {
      show_error('Failed to load artifact: ' + e.message);
    }
  }

  btnPrev.addEventListener('click', () => {
    if (g_current_index > 0) {
      load_artifact(g_current_index - 1);
    }
  });

  btnNext.addEventListener('click', () => {
    load_artifact(g_current_index + 1);
  });

  function open_search() {
    searchBar.classList.add('visible');
    content.classList.add('search-open');
    searchInput.focus();
    searchInput.select();
  }

  function close_search() {
    searchBar.classList.remove('visible');
    content.classList.remove('search-open');
    clear_highlights();
    g_search_hits   = [];
    g_search_cursor = 0;
    searchCount.textContent = '';
  }

  function clear_highlights() {
    document.querySelectorAll('mark.search-hit').forEach(mark => {
      const parent = mark.parentNode;
      parent.replaceChild(document.createTextNode(mark.textContent), mark);
      parent.normalize();
    });
  }

  function highlight_in_node(node, query) {
    if (node.nodeType === Node.TEXT_NODE) {
      const text  = node.textContent;
      const lower = text.toLowerCase();
      const q     = query.toLowerCase();
      const idx   = lower.indexOf(q);
      if (idx === -1) {
        return false;
      }
      const before = document.createTextNode(text.slice(0, idx));
      const mark   = document.createElement('mark');
      mark.className   = 'search-hit';
      mark.textContent = text.slice(idx, idx + q.length);
      const after = document.createTextNode(text.slice(idx + q.length));
      const parent = node.parentNode;
      parent.insertBefore(before, node);
      parent.insertBefore(mark, node);
      parent.insertBefore(after, node);
      parent.removeChild(node);
      return true;
    }
    if (node.nodeType === Node.ELEMENT_NODE &&
        node.tagName !== 'SCRIPT' &&
        node.tagName !== 'STYLE' &&
        !node.classList.contains('copy-latex-btn')) {
      Array.from(node.childNodes).forEach(child => highlight_in_node(child, query));
    }
    return false;
  }

  function run_search(query) {
    clear_highlights();
    g_search_hits   = [];
    g_search_cursor = 0;
    if (query.length < 1) {
      searchCount.textContent = '';
      return;
    }
    highlight_in_node(content, query);
    g_search_hits = Array.from(document.querySelectorAll('mark.search-hit'));
    if (g_search_hits.length === 0) {
      searchCount.textContent = 'no results';
      return;
    }
    jump_to_hit(0);
  }

  function jump_to_hit(index) {
    g_search_hits.forEach(m => m.classList.remove('current'));
    g_search_cursor = (index + g_search_hits.length) % g_search_hits.length;
    const hit = g_search_hits[g_search_cursor];
    hit.classList.add('current');
    hit.scrollIntoView({ block: 'center', behavior: 'smooth' });
    searchCount.textContent = (g_search_cursor + 1) + ' / ' + g_search_hits.length;
  }

  searchInput.addEventListener('input', () => run_search(searchInput.value));
  searchInput.addEventListener('keydown', e => {
    if (e.key === 'Enter') {
      if (e.shiftKey) {
        jump_to_hit(g_search_cursor - 1);
      } else {
        jump_to_hit(g_search_cursor + 1);
      }
    }
    if (e.key === 'Escape') {
      close_search();
    }
  });
  btnSearchPrev.addEventListener('click', () => jump_to_hit(g_search_cursor - 1));
  btnSearchNext.addEventListener('click', () => jump_to_hit(g_search_cursor + 1));
  btnSearchClose.addEventListener('click', close_search);
  btnSearchOpen.addEventListener('click', open_search);

  document.addEventListener('keydown', e => {
    if ((e.ctrlKey || e.metaKey) && e.key === 'f') {
      e.preventDefault();
      open_search();
    }
    if (e.key === 'ArrowLeft' && !btnPrev.disabled && document.activeElement !== searchInput) {
      load_artifact(g_current_index - 1);
    }
    if (e.key === 'ArrowRight' && !btnNext.disabled && document.activeElement !== searchInput) {
      load_artifact(g_current_index + 1);
    }
  });

  function connect() {
    const es = new EventSource('/events');
    es.onopen = () => set_status('connected', 'connected');
    es.onmessage = async (e) => {
      if (e.data === 'reload') {
        set_status('connected', 'loading...');
        await load_artifact();
        set_status('connected', 'connected');
      }
    };
    es.onerror = () => {
      set_status('error', 'reconnecting...');
      es.close();
      setTimeout(connect, 2000);
    };
  }

  connect();
  load_artifact();
</script>
</body>
</html>
)";
}

int main() {
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    httplib::Server svr;

    std::cout << "[" << timestamp() << "] viewer starting on http://0.0.0.0:" << k_port << "\n";

    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        log("GET", "/", 200);
        res.set_content(shell_html(), "text/html");
    });

    svr.Get("/artifact", [](const httplib::Request& req, httplib::Response& res) {
        size_t index = 0;
        {
            std::lock_guard lock(g_mutex);
            if (g_history.empty()) {
                log("GET", "/artifact", 204);
                res.status = 204;
                return;
            }
            if (req.has_param("index")) {
                const size_t requested = std::stoul(req.get_param_value("index"));
                if (requested >= g_history.size()) {
                    log("GET", "/artifact?index=" + req.get_param_value("index"), 404);
                    res.status = 404;
                    return;
                }
                index = requested;
            } else {
                index = g_history_index;
            }
        }

        const std::string json = build_artifact_json(index);
        if (json.empty()) {
            log("GET", "/artifact", 404);
            res.status = 404;
            res.set_content("index.html not found", "text/plain");
            return;
        }

        log("GET", "/artifact", 200);
        res.set_content(json, "application/json");
    });

    svr.Get("/events", [](const httplib::Request&, httplib::Response& res) {
        log("GET", "/events (SSE client connected)", 200);
        res.set_chunked_content_provider("text/event-stream",
            [](size_t, httplib::DataSink& sink) {
                {
                    std::lock_guard lock(g_mutex);
                    g_sinks.push_back(&sink);
                }
                (void)sink.write("data: connected\n\n", 18);
                while (g_running && sink.is_writable()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                {
                    std::lock_guard lock(g_mutex);
                    std::erase(g_sinks, &sink);
                }
                std::cout << "[" << timestamp() << "] SSE client disconnected\n";
                return false;
            }
        );
    });

    svr.Post("/publish", [](const httplib::Request& req, httplib::Response& res) {
        const std::string new_dir = req.body;
        if (!fs::exists(new_dir)) {
            log("POST", "/publish -> not found: " + new_dir, 400);
            res.status = 400;
            res.set_content("artifact directory not found", "text/plain");
            return;
        }
        {
            std::lock_guard lock(g_mutex);
            if (g_history.size() >= k_history_size) {
                if (fs::exists(g_history.front())) {
                    fs::remove_all(g_history.front());
                }
                g_history.erase(g_history.begin());
            }
            g_history.push_back(new_dir);
            g_history_index = g_history.size() - 1;
        }
        log("POST", "/publish -> " + new_dir, 200);
        notify_clients("reload");
        res.set_content("ok", "text/plain");
    });

    svr.Get("/assets/(.*)", [](const httplib::Request& req, httplib::Response& res) {
        std::string asset_path;
        {
            std::lock_guard lock(g_mutex);
            if (g_history.empty()) {
                res.status = 404;
                return;
            }
            asset_path = g_history[g_history_index] + "/assets/" + req.matches[1].str();
        }
        const std::string file_content = read_file(asset_path);
        if (file_content.empty()) {
            log("GET", "/assets/" + req.matches[1].str(), 404);
            res.status = 404;
            return;
        }
        log("GET", "/assets/" + req.matches[1].str(), 200);
        res.set_content(file_content, "application/octet-stream");
    });

    std::cout << "[" << timestamp() << "] listening — open http://localhost:" << k_port << "\n";
    svr.listen("0.0.0.0", k_port);
    return 0;
}
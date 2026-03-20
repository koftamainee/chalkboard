#include <atomic>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <httplib.h>

namespace fs = std::filesystem;

static constexpr int PORT = 8080;

namespace chalkboard {
  static std::mutex g_mutex;
  static std::string g_artifact_dir;
  static std::vector<httplib::DataSink*> g_sinks;
  static std::atomic<bool> g_running{true};

  static void cleanup_artifact() {
    if (!g_artifact_dir.empty() && fs::exists(g_artifact_dir)) {
      fs::remove_all(g_artifact_dir);
      g_artifact_dir.clear();
    }
  }

  static void signal_handler(int) {
    g_running = false;
    std::lock_guard lock(g_mutex);
    cleanup_artifact();
    std::exit(0);
  }

  static std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) { return ""; }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
  }

  static std::string extract_body(const std::string& html) {
    auto start = html.find("<body");
    if (start == std::string::npos) { return html; }
    start = html.find('>', start);
    if (start == std::string::npos) { return html; }
    ++start;
    auto end = html.rfind("</body>");
    if (end == std::string::npos) { return html.substr(start); }
    return html.substr(start, end - start);
  }

  static void notify_clients() {
    std::lock_guard lock(g_mutex);
    for (auto* sink : g_sinks) {
      (void)sink->write("data: reload\n\n", 14);
    }
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
    options: {
      skipHtmlTags: ['script','noscript','style','textarea','pre']
    }
  };
</script>
<script src="https://cdn.jsdelivr.net/npm/mathjax@3/es5/tex-chtml.js" async></script>
<style>
  :root {
    --bg:      #0f0c1f;
    --surface: #1b1733;
    --card:    #2a244a;
    --border:  #3a2f60;
    --accent:  #8b5cf6;
    --danger:  #f87171;
    --warn:    #fbbf24;
    --success: #34d399;
    --txt:     #e0d9ff;
    --muted:   #a5a1b8;
    --font-body: 'Georgia', 'Times New Roman', serif;
    --font-mono: 'JetBrains Mono', 'Fira Code', 'Courier New', monospace;
    --font-ui:   'Segoe UI', 'Helvetica Neue', sans-serif;
    --radius: 8px;
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

  #content {
    max-width: 860px;
    margin: 0 auto;
    padding: 60px 28px 80px;
  }

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
  }

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
  <span class="artifact-name" id="artifact-name"></span>
</div>

<div id="content">
  <div id="placeholder">
    <div class="icon">⬡</div>
    <p>Waiting for artifact...</p>
  </div>
</div>

<script>
  const dot          = document.getElementById('dot');
  const statusLabel  = document.getElementById('status-label');
  const artifactName = document.getElementById('artifact-name');
  const content      = document.getElementById('content');

  function setStatus(state, label) {
    dot.className = 'dot ' + state;
    statusLabel.textContent = label;
  }

  async function loadArtifact() {
    try {
      const res = await fetch('/artifact');
      if (!res.ok) { return; }
      const data = await res.json();
      content.innerHTML = data.body;
      content.className = 'fade-in';
      artifactName.textContent = data.name;
      if (window.MathJax) {
        await MathJax.typesetPromise([content]);
      }
    } catch(e) {
      console.error('loadArtifact failed:', e);
    }
  }

  function connect() {
    const es = new EventSource('/events');

    es.onopen = () => setStatus('connected', 'connected');

    es.onmessage = async (e) => {
      if (e.data === 'reload') {
        setStatus('connected', 'loading...');
        await loadArtifact();
        setStatus('connected', 'connected');
      }
    };

    es.onerror = () => {
      setStatus('error', 'reconnecting...');
      es.close();
      setTimeout(connect, 2000);
    };
  }

  connect();
  loadArtifact();
</script>
</body>
</html>
)";
  }

  int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    httplib::Server svr;

    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
      res.set_content(shell_html(), "text/html");
    });

    svr.Get("/artifact", [](const httplib::Request&, httplib::Response& res) {
      std::lock_guard lock(g_mutex);
      if (g_artifact_dir.empty()) {
        res.status = 204;
        return;
      }
      std::string index_path = g_artifact_dir + "/index.html";
      std::string html = read_file(index_path);
      if (html.empty()) {
        res.status = 404;
        return;
      }
      std::string body = extract_body(html);
      std::string name = fs::path(g_artifact_dir).filename().string();

      auto escape = [](const std::string& s) {
        std::string r;
        for (char c : s) {
          if (c == '"') { r += "\\\""; }
          else if (c == '\\') { r += "\\\\"; }
          else if (c == '\n') { r += "\\n"; }
          else if (c == '\r') { r += "\\r"; }
          else { r += c; }
        }
        return r;
      };

      std::ostringstream json;
      json << "{\"body\":\"" << escape(body) << "\","
        << "\"name\":\"" << escape(name) << "\"}";

      res.set_content(json.str(), "application/json");
    });

    svr.Get("/events", [](const httplib::Request&, httplib::Response& res) {
      res.set_chunked_content_provider("text/event-stream",
                                       [](size_t, httplib::DataSink& sink) {
                                         {
                                           std::lock_guard lock(g_mutex);
                                           g_sinks.push_back(&sink);
                                         }
                                         sink.write("data: connected\n\n", 18);
                                         while (g_running && sink.is_writable()) {
                                           std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                         }
                                         {
                                           std::lock_guard lock(g_mutex);
                                           g_sinks.erase(
                                             std::remove(g_sinks.begin(), g_sinks.end(), &sink),
                                             g_sinks.end()
                                           );
                                         }
                                         return false;
                                       }
      );
    });

    svr.Post("/publish", [](const httplib::Request& req, httplib::Response& res) {
      std::string new_dir = req.body;
      if (!fs::exists(new_dir)) {
        res.status = 400;
        res.set_content("artifact directory not found", "text/plain");
        return;
      }
      {
        std::lock_guard lock(g_mutex);
        if (new_dir != g_artifact_dir) {
          cleanup_artifact();
        }
        g_artifact_dir = new_dir;
      }
      notify_clients();
      res.set_content("ok", "text/plain");
    });

    svr.Get("/assets/(.*)", [](const httplib::Request& req, httplib::Response& res) {
      std::lock_guard lock(g_mutex);
      if (g_artifact_dir.empty()) {
        res.status = 404;
        return;
      }
      std::string path = g_artifact_dir + "/assets/" + req.matches[1].str();
      std::string content = read_file(path);
      if (content.empty()) {
        res.status = 404;
        return;
      }
      res.set_content(content, "application/octet-stream");
    });

    svr.listen("0.0.0.0", PORT);
    return 0;
  }
}

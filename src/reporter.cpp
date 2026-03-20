#include <chalkboard/reporter.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace chalkboard {
    Reporter::Reporter(std::string title)
        : m_title(std::move(title)) {}

    std::string Reporter::math(const std::string& s) { return "\\(" + s + "\\)"; }
    std::string Reporter::display(const std::string& s) { return "\\[" + s + "\\]"; }

    std::string Reporter::mbf(const std::string& s) { return "\\mathbf{" + s + "}"; }
    std::string Reporter::mbb(const std::string& s) { return "\\mathbb{" + s + "}"; }
    std::string Reporter::mit(const std::string& s) { return "\\mathit{" + s + "}"; }
    std::string Reporter::mrm(const std::string& s) { return "\\mathrm{" + s + "}"; }
    std::string Reporter::mtt(const std::string& s) { return "\\mathtt{" + s + "}"; }
    std::string Reporter::txt(const std::string& s) { return "\\text{" + s + "}"; }

    std::string Reporter::N() { return mbb("N"); }
    std::string Reporter::Z() { return mbb("Z"); }
    std::string Reporter::Q() { return mbb("Q"); }
    std::string Reporter::R() { return mbb("R"); }
    std::string Reporter::C() { return mbb("C"); }
    std::string Reporter::F() { return mbb("F"); }

    std::string Reporter::ring(const std::string& field, const std::vector<std::string>& vars) {
        std::string s = field + "[";
        for (std::size_t i = 0; i < vars.size(); ++i) {
            if (i) { s += ", "; }
            s += vars[i];
        }
        return s + "]";
    }

    std::string Reporter::eq(const std::string& a, const std::string& b) { return a + " = " + b; }
    std::string Reporter::neq(const std::string& a, const std::string& b) { return a + " \\neq " + b; }
    std::string Reporter::in(const std::string& a, const std::string& b) { return a + " \\in " + b; }
    std::string Reporter::sub(const std::string& a, const std::string& b) { return a + " \\subseteq " + b; }

    std::string Reporter::cdot(const std::string& a, const std::string& b) { return a + " \\cdot " + b; }
    std::string Reporter::frac(const std::string& a, const std::string& b) { return "\\frac{" + a + "}{" + b + "}"; }
    std::string Reporter::pow(const std::string& a, const std::string& b) { return a + "^{" + b + "}"; }
    std::string Reporter::sub_script(const std::string& a, const std::string& b) { return a + "_{" + b + "}"; }
    std::string Reporter::sqrt(const std::string& a) { return "\\sqrt{" + a + "}"; }
    std::string Reporter::abs(const std::string& a) { return "\\lvert " + a + " \\rvert"; }
    std::string Reporter::norm(const std::string& a) { return "\\lVert " + a + " \\rVert"; }

    std::string Reporter::to() { return "\\to"; }
    std::string Reporter::implies() { return "\\Rightarrow"; }
    std::string Reporter::iff() { return "\\Leftrightarrow"; }
    std::string Reporter::mapsto() { return "\\mapsto"; }

    Reporter& Reporter::section(const std::string& text) {
        push(std::make_unique<HeadingBlock>(2, text));
        return *this;
    }

    Reporter& Reporter::subsection(const std::string& text) {
        push(std::make_unique<HeadingBlock>(3, text));
        return *this;
    }

    Reporter& Reporter::subsubsection(const std::string& text) {
        push(std::make_unique<HeadingBlock>(4, text));
        return *this;
    }

    Reporter& Reporter::text(const std::string& paragraph) {
        push(std::make_unique<TextBlock>(paragraph));
        return *this;
    }

    Reporter& Reporter::raw_latex(const std::string& latex) {
        push(std::make_unique<MathBlock>(latex));
        return *this;
    }

    Reporter& Reporter::add(const IReportObject& obj) {
        push(std::make_unique<RawBlock>(obj.to_html()));
        return *this;
    }

    std::string Reporter::build() const {
        std::string dir = make_artifact_dir();
        if (dir == "/tmp/report_") {
            throw std::invalid_argument("Reporter::build: title produces empty directory name");
        }
        std::filesystem::create_directories(dir);
        std::filesystem::create_directories(dir + "/assets");

        const std::string path = dir + "/index.html";
        std::ofstream out(path);
        if (!out.is_open()) {
            throw std::runtime_error("Reporter::build: cannot open file: " + path);
        }
        out << render_shell();
        return dir;
    }

    Reporter::HeadingBlock::HeadingBlock(const int level, std::string text)
        : m_level(level), m_text(std::move(text)) {}

    std::string Reporter::HeadingBlock::to_html() const {
        const std::string tag = "h" + std::to_string(m_level);
        return "<" + tag + ">" + Reporter::escape_html(m_text) + "</" + tag + ">\n";
    }

    Reporter::TextBlock::TextBlock(std::string content)
        : m_content(std::move(content)) {}

    std::string Reporter::TextBlock::to_html() const {
        return "<p>" + m_content + "</p>\n";
    }

    Reporter::MathBlock::MathBlock(std::string latex)
        : m_latex(std::move(latex)) {}

    std::string Reporter::MathBlock::to_html() const {
        return R"(<p class="math">\[)" + m_latex + "\\]</p>\n";
    }

    Reporter::RawBlock::RawBlock(std::string html)
        : m_html(std::move(html)) {}

    std::string Reporter::RawBlock::to_html() const {
        return m_html + "\n";
    }

    void Reporter::push(std::unique_ptr<IReportObject> block) {
        m_blocks.push_back(std::move(block));
    }

    std::string Reporter::make_artifact_dir() const {
        return "/tmp/report_" + sanitize_title(m_title);
    }

    std::string Reporter::escape_html(const std::string& s) {
        std::string result;
        result.reserve(s.size());
        for (const char c : s) {
            switch (c) {
            case '&':  result += "&amp;";  break;
            case '<':  result += "&lt;";   break;
            case '>':  result += "&gt;";   break;
            case '"':  result += "&quot;"; break;
            case '\'': result += "&#39;";  break;
            default:   result += c;        break;
            }
        }
        return result;
    }

    std::string Reporter::sanitize_title(const std::string& title) {
        std::string result;
        for (const char c : title) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                result += c;
            } else if (std::isspace(static_cast<unsigned char>(c))) {
                result += '_';
            }
        }
        return result;
    }

    std::string Reporter::current_timestamp() {
        const auto now = std::chrono::system_clock::now();
        const auto t = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss;
        oss << std::put_time(std::gmtime(&t), "%Y-%m-%d %H:%M:%S UTC");
        return oss.str();
    }

    std::string Reporter::render_shell() const {
        std::ostringstream html;
        html << R"(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>)" << escape_html(m_title) << R"(</title>
</head>
<body>
<header>
  <h1>)" << escape_html(m_title) << R"(</h1>
  <div class="timestamp">)" << current_timestamp() << R"(</div>
</header>
<main>
)";
        for (const auto& block : m_blocks) {
            html << block->to_html();
        }
        html << R"(</main>
</body>
</html>
)";
        return html.str();
    }
}
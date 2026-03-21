#pragma once

#include <format>
#include <memory>
#include <string>
#include <vector>

#include <chalkboard/latex_serializable.h>
#include <chalkboard/report_object.h>

namespace chalkboard {
  class Reporter {
  public:
    explicit Reporter(std::string title);

    static std::string math(const std::string& s);
    static std::string display(const std::string& s);

    static std::string mbf(const std::string& s);
    static std::string mbb(const std::string& s);
    static std::string mit(const std::string& s);
    static std::string mrm(const std::string& s);
    static std::string mtt(const std::string& s);
    static std::string txt(const std::string& s);

    static std::string N();
    static std::string Z();
    static std::string Q();
    static std::string R();
    static std::string C();
    static std::string F();

    static std::string ring(const std::string& field, const std::vector<std::string>& vars);

    static std::string eq(const std::string& a, const std::string& b);
    static std::string neq(const std::string& a, const std::string& b);
    static std::string in(const std::string& a, const std::string& b);
    static std::string sub(const std::string& a, const std::string& b);

    static std::string cdot(const std::string& a, const std::string& b);
    static std::string frac(const std::string& a, const std::string& b);
    static std::string pow(const std::string& a, const std::string& b);
    static std::string sub_script(const std::string& a, const std::string& b);
    static std::string sqrt(const std::string& a);
    static std::string abs(const std::string& a);
    static std::string norm(const std::string& a);

    static std::string to();
    static std::string implies();
    static std::string iff();
    static std::string mapsto();

    Reporter& section(const std::string& text);
    Reporter& subsection(const std::string& text);
    Reporter& subsubsection(const std::string& text);
    Reporter& text(const std::string& paragraph);

    template <typename... Args>
    Reporter& text(std::format_string<Args...> fmt, Args&&... args) {
      return text(std::format(fmt, maybe_latex(std::forward<Args>(args))...));
    }

    Reporter& raw_latex(const std::string& latex);

    Reporter& math_block(const LatexSerializable auto& obj) {
      return raw_latex(to_latex(obj));
    }

    Reporter& add(const IReportObject& obj);

    [[nodiscard]] std::string build() const;

  private:
    struct HeadingBlock : IReportObject {
      int m_level;
      std::string m_text;

      HeadingBlock(int level, std::string text);
      [[nodiscard]] std::string to_html() const override;
    };

    struct TextBlock : IReportObject {
      std::string m_content;

      explicit TextBlock(std::string content);
      [[nodiscard]] std::string to_html() const override;
    };

    struct MathBlock : IReportObject {
      std::string m_latex;

      explicit MathBlock(std::string latex);
      [[nodiscard]] std::string to_html() const override;
    };

    struct RawBlock : IReportObject {
      std::string m_html;

      explicit RawBlock(std::string html);
      [[nodiscard]] std::string to_html() const override;
    };

    void push(std::unique_ptr<IReportObject> block);
    [[nodiscard]] std::string make_artifact_dir() const;
    [[nodiscard]] std::string render_shell() const;

    static std::string escape_html(const std::string& s);
    static std::string sanitize_title(const std::string& title);
    static std::string current_timestamp();

    std::string m_title;
    std::vector<std::unique_ptr<IReportObject>> m_blocks;
  };
}

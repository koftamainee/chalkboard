#include <chalkboard/latex_list.h>

LatexList LatexList::itemize() { return LatexList(Kind::Itemize); }
LatexList LatexList::enumerate() { return LatexList(Kind::Enumerate); }

LatexList::LatexList(Kind kind) : m_kind(kind) {}

std::string LatexList::to_html() const {
  std::string tag = (m_kind == Kind::Itemize) ? "ul" : "ol";
  std::string result = "<" + tag + ">\n";
  for (const auto& i : m_items) {
    result += "<li>" + i + "</li>\n";
  }
  result += "</" + tag + ">\n";
  return result;
}
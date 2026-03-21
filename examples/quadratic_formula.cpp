#include <iostream>
#include <chalkboard/reporter.h>
#include <chalkboard/latex_list.h>
#include <chalkboard/publisher.h>

int main() {
    using R = chalkboard::Reporter;

    R reporter("Quadratic formula");

    reporter
        .section("Quadratic formula")
        .text(
            "The roots of the general quadratic {} are:",
            R::math("ax^2 + bx + c = 0, \\; a \\neq 0"))
        .raw_latex(R::display(
            R::eq("x", R::frac(
                "-b \\pm \\sqrt{b^2 - 4ac}",
                "2a"))));

    reporter.subsection("Derivation by completing the square");

    reporter.raw_latex(R::display(
        R::eq(R::pow(R::sub_script("(x", "0)"), "2"),
              R::frac("b^2 - 4ac", "4a^2"))));

    reporter.text(
        "where {} is the vertex x-coordinate {}.",
        R::math("x_0"),
        R::math(R::eq("x_0", R::frac("-b", "2a"))));

    reporter.subsection("Discriminant");

    const std::string delta_def = R::eq(
        "\\Delta",
        "b^2 - 4ac");

    reporter.text(
        "Define the discriminant {}. "
        "Its sign fully determines the nature of the roots.",
        R::math(delta_def));

    auto cases = chalkboard::LatexList::itemize()
        .item("{} — two distinct real roots {}.",
              R::math("\\Delta > 0"),
              R::math(R::in("x_{1,2}", R::R())))
        .item("{} — one repeated real root {}.",
              R::math("\\Delta = 0"),
              R::math(R::eq("x", R::frac("-b", "2a"))))
        .item("{} — two complex conjugate roots {}.",
              R::math("\\Delta < 0"),
              R::math(R::in("x_{1,2}", R::C())));

    reporter.add(cases);

    reporter.subsection("Vieta's formulas");

    reporter.text(
        "For roots {} and {}, the following relations hold "
        "regardless of the discriminant.",
        R::math("x_1"), R::math("x_2"));

    reporter
        .raw_latex(R::display(R::eq("x_1 + x_2", R::frac("-b", "a"))))
        .raw_latex(R::display(R::eq("x_1 \\cdot x_2", R::frac("c", "a"))));

    const std::string artifact_dir = reporter.build();

    const chalkboard::Publisher publisher("localhost", 8080);
    publisher.publish(artifact_dir);
}
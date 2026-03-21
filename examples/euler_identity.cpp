#include <chalkboard/reporter.h>
#include <chalkboard/latex_list.h>
#include <chalkboard/publisher.h>

int main() {
    using R = chalkboard::Reporter;

    R reporter("Eulers Identity");

    reporter
        .section("Euler's identity")
        .text(
            "Often called the most beautiful equation in mathematics, "
            "it links the five fundamental constants {}, {}, {}, {}, and {}.",
            R::math("e"), R::math("i"), R::math("\\pi"), R::math("1"), R::math("0"))
        .raw_latex(R::display("e^{i\\pi} + 1 = 0"));

    reporter.subsection("Derivation from Euler's formula");

    reporter.text(
        "Euler's formula states that {} for any real {}.",
        R::math("e^{ix} = \\cos x + i\\sin x"),
        R::math("x"));

    reporter.text(
        "Setting {} and using {} and {}, the result follows immediately.",
        R::math("x = \\pi"),
        R::math("\\cos\\pi = -1"),
        R::math("\\sin\\pi = 0"));

    const auto steps = chalkboard::LatexList::enumerate()
        .item("Substitute {}: obtain {}.",
              R::math("x = \\pi"),
              R::math("e^{i\\pi} = \\cos\\pi + i\\sin\\pi"))
        .item("Evaluate the trig values: {} and {}.",
              R::math("\\cos\\pi = -1"),
              R::math("\\sin\\pi = 0"))
        .item("Substitute: {}.",
              R::math("e^{i\\pi} = -1 + i \\cdot 0 = -1"))
        .item("Rearrange to get {}.",
              R::math("e^{i\\pi} + 1 = 0"));

    reporter.add(steps);

    reporter.subsection("Geometric interpretation");
    reporter.text(
        "In the complex plane, {} represents a rotation by {} radians. "
        "Starting from {} on the real axis and rotating by {}, "
        "we arrive at {}, giving the identity.",
        R::math("e^{i\\theta}"),
        R::math("\\theta"),
        R::math("1"),
        R::math("\\pi"),
        R::math("-1"));

    const std::string artifact_path = reporter.build();

    const chalkboard::Publisher publisher("localhost", 8080);
    publisher.publish(artifact_path);
}
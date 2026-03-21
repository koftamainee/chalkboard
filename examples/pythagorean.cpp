#include <chalkboard/task.h>
#include <chalkboard/reporter.h>

int main() {
  const chalkboard::Task task("Pythagorean theorem");

  task.build_and_publish([](chalkboard::Reporter& r) {
    using R = chalkboard::Reporter;
    r.section("Pythagorean theorem")
     .text("For a right triangle with legs {} and {} and hypotenuse {}, the following holds.",
           R::math("a"), R::math("b"), R::math("c"))
     .raw_latex(R::display("a^2 + b^2 = c^2"))
     .subsection("In terms of the hypotenuse")
     .text("Solving for {} gives:", R::math("c"))
     .raw_latex(R::display(R::eq("c", R::sqrt("a^2 + b^2"))));
  });
}

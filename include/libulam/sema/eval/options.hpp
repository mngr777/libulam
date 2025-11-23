#pragma once

namespace ulam::sema {

struct EvalOptions {
    // allows casting non-atom ref to Atom (see t3697):
    // ```
    // element E : Q {
    //   void foo() {
    //     Q& e_ref = self;       // dyn ref type is E,
    //     Atom a = (Atom) e_ref; // cast to Atom is allowed
    //   }
    // }
    // ```
    bool cast_deref_as_dyn_type{true};

    int max_loop_iterations{150}; // -1 for no limit
};

constexpr EvalOptions DefaultEvalOptions{};

} // namespace ulam::sema

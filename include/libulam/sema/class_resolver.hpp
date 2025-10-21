#pragma once
#include <libulam/sema/eval/helper.hpp>

namespace ulam {
class Class;
}

namespace ulam::sema {

class EvalEnv;
class Resolver;

class ClassResolver : EvalHelper {
public:
    ClassResolver(EvalEnv& env, Resolver& resolver, Class& cls):
        EvalHelper{env}, _resolver{resolver}, _cls{cls} {}

    bool init();
    bool resolve(bool full = false);

private:
    bool do_resolve();
    bool resolve_rest();

    bool resolve_params();
    bool init_ancestors();

    bool resolve_ancestors();
    bool resolve_props();
    bool resolve_funs();
    bool check_bitsize();

    void init_default_data();

    bool do_init_ancestors();
    void add_inherited_props();

    Resolver& _resolver;
    Class& _cls;
};

} // namespace ulam::sema

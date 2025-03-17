#include "./test_case.hpp"
#include <algorithm>
#include <cassert>
#include <exception>
#include <filesystem>
#include <iostream>
#include <set> // TEST
#include <stdexcept>
#include <string>

using Path = std::filesystem::path;

static void exit_usage(std::string name) {
    std::cout << name << " <path-to-ulam-src-root>[ <case-number>]\n";
    std::exit(-1);
}

static bool run(Path stdlib_dir, const Path& path) {
    try {
        TestCase test_case{stdlib_dir, path};
        test_case.run();
        return true;
    } catch (std::invalid_argument& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
}

static bool run(Path stdlib_dir, unsigned n, std::vector<Path> test_paths) {
    assert(n > 0);
    auto& path = test_paths[n - 1];
    std::cout << "# " << n << " " << path.filename() << "\n";
    bool ok = true;
    try {
        ok = run(stdlib_dir, path);
    } catch (std::exception& exc) {
        std::cout << "exception thrown\n";
        ok = false;
    }
    std::cout << "# " << n << " " << path.filename() << " "
              << (ok ? "OK" : "FAIL") << "\n";
    return ok;
}

int main(int argc, char** argv) {
    if (argc < 2)
        exit_usage(argv[0]);
    // ULAM paths
    const Path ulam_src_root{argv[1]};
    const Path stdlib_dir{ulam_src_root / "share" / "ulam" / "stdlib"};
    const Path test_src_dir{
        ulam_src_root / "src" / "test" / "generic" / "safe"};
    // test case number
    unsigned case_num = 0;
    if (argc > 2)
        case_num = std::stoul(argv[2]);

    std::vector<Path> test_paths;
    {
        auto it = std::filesystem::directory_iterator{test_src_dir};
        for (const auto& item : it)
            test_paths.push_back(item.path());
    }
    std::sort(test_paths.begin(), test_paths.end());

    if (case_num == 0) {
        std::set<std::string> skip = {
            "t3205_test_compiler_elementandquark_emptyquark.test", // Empty
            "t3233_test_compiler_elementandquarkarray_ew.test", // aref called by name
            "t3241_test_compiler_unarymod.test", // invalid implicit cast?
            "t3357_test_compiler_elementandquarkswclassargs_lhsconstantcompare.test", // behave()
            "t3358_test_compiler_elementandquarkswclassargs_lhsconstantlogicaland.test", // behave()
            "t3361_test_compiler_elementandquarkswclassargs_memberconstantasfunccallarg.test", // behave()
            "t3400_test_compiler_arraysizeof_lengthzeroelement.test", // Empty
            "t3401_test_compiler_unaryminofmaxofconstant_issue.test", // Empty
            "t3484_test_compiler_elementandquark_caarray_ambiguousfunc_issue.test", // Empty
            "t3494_test_compiler_divideandmodmixedtypes.test", // carrying consteval flag through props?
            "t3501_test_compiler_elementandquarkcaarray_unsignedindex_issue.test", // ambiguous funcall as intended
            "t3504_test_compiler_arraywithconstantindex.test", // wrong definitiong order, wontfix?
            "t3505_test_compiler_elementandquark_caarrayemptyquark.test", // Empty
            "t3590_test_compiler_isemptyquark.test", // Empty
            "t3631_test_compiler_element_castatomandquark_ish.test", // tmp, quark to Atom?
            "t3675_test_compiler_elementandquark_Selfatomoffordefaultelement.test", // quark self to Atoms
            "t3692_test_compiler_atomcasttoquarkselfref.test", // quark Self to Atom
            "t3693_test_compiler_atomcasttoquarkselfrefasself.test", // -"-
            "t3697_test_compiler_elementandquark_castreftoatom.test", // quark ref to Atom: use .atomof instead?
            "t3701_test_compiler_elementandquark_castclassreftoatomref.test", // quark ref to Atom&
            "t3747_test_compiler_elementinheritedquark_instanceof_withsuper.test", // quark to Atom via super.instanceof ??
            "t3774_test_compiler_transientwithfuncswrefarg_undefinedtemp_issue.test", // empty Atom ref argument for Mob.visit, TODO
            "t3779_test_compiler_transientwithfuncswrefarg_castvoidreturn_issue", // -"-
            "t3779_test_compiler_transientwithfuncswrefarg_castvoidreturn_issue.test", // -"-
            "t3788_test_compiler_localtransientcallsfuncswrefargwithself_issue.test", // UrSelf
            "t3794_test_compiler_localtransientcallsfuncswrefargwithrefcast.test", // UrSelf
            "t3795_test_compiler_localtransientcallsfuncswrefargbutnotref.test", // UrSelf
            "t3834_test_compiler_atomreffromsuperquarkrefwithcast.test", // quark element base to Atom&: use (Atom&)(Elt&)q ?
            "t3837_test_compiler_element_quarkrefatomof_issue.test", // Atom to quark element base reference: use (QPerc&)(Elt&) ?
            "t3873_test_compiler_elementinheritance_withunorderedlocaldefs_separatefilescope.test", // type defined after use, wontfix?
            "t3881_test_compiler_constantarrayinit_datamember.test", // UrSelf
            "t3882_test_compiler_constantarrayinit_immediate.test", // UrSelf
            "t3883_test_compiler_constantarrayinit_localdef.test", // UrSelf
            "t3886_test_compiler_elementandquarkswclassargs_wdefaultparamvaluefromanotherclass_reverseorder.test", // tpl param used in previous param definition, wontfix?
            "t3888_test_compiler_elementandquarkswclassargs_wdefaultparamvaluefromanotherclass_constantdm.test", // tpl const used as param default value, wontfix?
            "t3889_test_compiler_elementandquarkswclassargs_wdefaultparamvaluefromanotherclass_constantdminancestor.test", // -"-
            "t3890_test_compiler_elementandquarkswclassargs_wdefaultparamvaluefromanotherclass_localdefconstantarrayitem.test", // -"-
            "t3891_test_compiler_elementandquarkswclassargs_wdefaultparamvaluefromanotherclass_localdefconstantarrayitem.test", // -"-
            "t3899_test_compiler_constantarrayinit_datamember_dmandlocalvarinitvals.test", // UrSelf
            "t3920_test_compiler_elementandquark_conditionalis_resbehave_issue.test", // Empty
            "t3921_test_compiler_elementandquark_conditionalis_qswitch4wallportqportcell_issue.test", // Empty
            "t3930_test_compiler_string_ascii.test", // ASCII
            "t3936_test_compiler_arraylengthof_lengthzeroelement.test", // Empty
            "t3939_test_compiler_caarraylengthof.test", // alengthof, TODO
            "t3941_test_compiler_stringitem_novar.test", // ASCII
            "t3948_test_compiler_caarrayofregulararrayofstrings3D.test", // ASCII
            "t3951_test_compiler_stringascii_constant.test", // ASCII
            "t3952_test_compiler_stringascii_filescope.test", // ASCII
            "t3953_test_compiler_stringascii_constantarray_lengthof.test", // ASCII
            "t3955_test_compiler_stringascii_octal_issue.test", // ASCII
            "t3957_test_compiler_stringascii_dbldigitlength_reflengthof_cast.test", // ASCII
            "t3954_test_compiler_stringascii_constantarray_filescope.test", // ASCII
            "t3967_test_compiler_inheritedtransientdmcastasref_issue.test", // ASCII
            "t3972_test_compiler_string_dminit.test", // ASCII
            "t3973_test_compiler_stringarray_dminit_lengthof.test", // ASCII
            "t3974_test_compiler_stringarray_localinit_lengthof.test", // ASCII
            "t3986_test_compiler_atomofvirtualselfquarkSelf_issue.test", // Empty
        };
        for (unsigned n = 1; n <= test_paths.size(); ++n) {
            if (skip.count(test_paths[n - 1].filename()) > 0)
                continue;
            if (!run(stdlib_dir, n, test_paths))
                break;
        }
    } else {
        if (case_num <= test_paths.size()) {
            run(stdlib_dir, case_num, test_paths);
        } else {
            std::cout << "case not found\n";
        }
    }
}

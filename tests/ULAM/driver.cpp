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
    std::cout << "# " << std::dec << n << " " << path.filename() << "\n";
    bool ok = true;
    try {
        ok = run(stdlib_dir, path);
    } catch (std::exception& exc) {
        std::cout << "exception thrown\n";
        ok = false;
    }
    std::cout << "# " << std::dec << n << " " << path.filename() << " "
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
            "t3233_test_compiler_elementandquarkarray_ew.test", // aref called by name
            "t3241_test_compiler_unarymod.test", // invalid implicit cast?
            "t3484_test_compiler_elementandquark_caarray_ambiguousfunc_issue.test", // TODO: aref can have overloads
            "t3494_test_compiler_divideandmodmixedtypes.test", // carrying consteval flag through props?
            "t3501_test_compiler_elementandquarkcaarray_unsignedindex_issue.test", // ambiguous funcall as intended
            "t3504_test_compiler_arraywithconstantindex.test", // wrong definitiong order, wontfix?
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
            "t3834_test_compiler_atomreffromsuperquarkrefwithcast.test", // quark element base to Atom&: use (Atom&)(Elt&)q ?
            "t3837_test_compiler_element_quarkrefatomof_issue.test", // Atom to quark element base reference: use (QPerc&)(Elt&) ?
            "t3873_test_compiler_elementinheritance_withunorderedlocaldefs_separatefilescope.test", // type defined after use, wontfix?
            "t3886_test_compiler_elementandquarkswclassargs_wdefaultparamvaluefromanotherclass_reverseorder.test", // tpl param used in previous param definition, wontfix?
            "t3888_test_compiler_elementandquarkswclassargs_wdefaultparamvaluefromanotherclass_constantdm.test", // tpl const used as param default value, wontfix?
            "t3889_test_compiler_elementandquarkswclassargs_wdefaultparamvaluefromanotherclass_constantdminancestor.test", // -"-
            "t3890_test_compiler_elementandquarkswclassargs_wdefaultparamvaluefromanotherclass_localdefconstantarrayitem.test", // -"-
            "t3891_test_compiler_elementandquarkswclassargs_wdefaultparamvaluefromanotherclass_localdefconstantarrayitem.test", // -"-
            "t3939_test_compiler_caarraylengthof.test", // alengthof, TODO
            "t3972_test_compiler_string_dminit.test", // -"-
            "t3973_test_compiler_stringarray_dminit_lengthof.test", // -"-
            "t3986_test_compiler_atomofvirtualselfquarkSelf_issue.test", // Atom& to quark ref cast
            "t41005_test_compiler_tmpvarnotdeclared_customarraygencode_issue.test", // Atom& to quark ref cast
            "t41006_test_compiler_tmpvarnotdeclared_lhsfunccallgencode.test", // -"-
            "t41007_test_compiler_addstubcopytoancestorclass_customarray_issue.test", // -"-
            "t41043_test_compiler_forascond.test", // for (; (a as Type); ) {...}, TODO
            "t41046_test_compiler_switchascondcases.test", // which() as cond, TODO
            "t41050_test_compiler_controlswitch_emptyvalueemptybody.test", // -"-
            "t41051_test_compiler_element_castatomreffuncreturnvalue_issue.test", // Atom& to quark ref cast
            "t41052_test_compiler_elementandquark_castreffuncreturnvalue.test", // rvalue ref
            "t41053_test_compiler_transientandquark_castreffuncreturnvalue.test", // rvalue ref
            "t41074_test_compiler_elementandquark_customarrayonfunccallreturn.test", // assignment to xvalue
            "t41085_test_compiler_elementandquark_instanceofwithconstructor.test", // instanceof with arguments, TODO
            "t41088_test_compiler_instanceofconstructorfuncarg.test", // -"-
            "t41089_test_compiler_assigntofuncreturnvalueonlhs_evalerr.test", // assignment to rvalue
            "t41091_test_compiler_elementandquark_memberselectoninstanceofconstructorcallresult.test", // instanceof with arguments
            "t41092_test_compiler_elementandquark_constructorcallonfuncreturninstance.test", // -"-
            "t41109_test_compiler_elementandtransient_comparisonoperatoroverloadequalequal.test", // implicit `!=' operator
            "t41112_test_compiler_elementandtransient_comparisonoperatoroverloads.test", // implicit `>=' operator
            "t41153_test_compiler_elementandquark_selfcasttoatom_issue.test", // quark to Atom cast
            "t41214_test_compiler_elementwithclassparameterquarktemplate_Sfirstwdefault.test", // tpl param ised in previous param definition, wontfix?
            "t41215_test_compiler_elementandquarkswclassargs_dependentprimitivebitsizewdefault.test", // -"-
            "t41217_test_compiler_elementwithclassparameterquarktemplateanddefaultsecarg.test", // -"-
            "t41218_test_compiler_elementwithclassparameterquarktemplate_scopeparamnames.test", // -"-
            "t41223_test_compiler_elementtemplatewithinheritedclassparameterquarktemplate.test", // -"-
            "t41224_test_compiler_elementregularwithinheritedclassparameterquarktemplate.test", // -"-
            "t41226_test_compiler_elementtemplatewithinheritedclassparameterquarktemplatedependent_thegoal.test", // -"-
            "t41228_test_compiler_elementtemplatewithinheritedclassparameterquarktemplateandancestor.test", // -"-
            "t41266_test_compiler_constantclassarrayoftransients.test", // module-local constant `keyexpr_x13` addressed as `KeyExprRep.keyexpr_x13` ??
            "t41284_test_compiler_localdefquestioncolon_filescope.test", // module-local constants referenced before definition
            "t41285_test_compiler_localdefquestioncolon_usingtemplateinstanceconstant_filescope.test",
            "t41310_test_compiler_elementandquark_multibases_virtualfuncsselectwdatamembersandtypedefs.test", // using local alias to access base class, potentially ambiguous?
            "t41311_test_compiler_elementandquark_multibases_virtualfuncsselectwself.test", // -"-
            "t41315_test_compiler_elementandquark_multibases_isasbase.test", // Atom& to quark ref cast
            "t41316_test_compiler_transientandquark_multibases_virtualfuncsselectwdatamembersandtypedefs.test", // using local alias to access base class, potentially ambiguous?
            "t41318_test_compiler_elementandquark_multibasesisasbase_virtualfuncswsharedancestoranddatamembercopies.test", // Atom& to quark
            "t41320_test_compiler_elementandquark_multibases_refbasecallsoverridebaseclassvirtualfunc.test", // -"-
            "t41323_test_compiler_elementandquark_multibases_virtualfuncscalledonsharedancestoranddatamembercopies.test", // -"-
            "t41360_test_compiler_elementandquarks_multibases_atomofcasting_issue.test", // -"-
            "t41376_test_compiler_elementandquark_multibases_specificbasevariablevirtualfunccall.test", // classid magic, TODO
            "t41379_test_compiler_elementandquark_multibases_specificbasevariablevirtualfunccallusingclassidofref.test", // -"-
            "t41380_test_compiler_elementandquark_multibases_constantspecificbasevariablevirtualfunccall.test", // -"-
            "t41381_test_compiler_elementandquark_multibases_specificbasevariablevirtualfunccallusingconstantclassidofref.test", // -"-
            "t41382_test_compiler_elementandquarkswclassargs_wdefaultparamvalueandtype_insertCLASS.test", // __CLASS_*__, TODO
            "t41384_test_compiler_elementandquarkwclassargs_multibasesisasbase_virtualfuncswsharedancestoranddatamembercopies.test", // base class access args, TODO
            "t41391_test_compiler_elementandquark_multibases_virtualfuncsselectwselfandclassidconstant.test", // classid magic, TODO
            "t41392_test_compiler_elementandquark_multibases_virtualfuncsselectwselfandclassidconstant2.test", // -"-
            "t41393_test_compiler_elementandquark_multibases_virtualfuncsselectwselfSelfandclassidconstant3.test", // -"-
            "t41414_test_compiler_castingquarkreftobits_ish.test", // ref to base to Bits(Base.sizeof), TODO?? -- need "known dynamic type" flag similar to consteval for values
            "t41422_test_compiler_castingstringtobitsandbitstostring.test", // Bits to String, ??
            "t41425_test_compiler_castingstringtobooltovalidatestringindex.test", // -"-
            "t41438_test_compiler_elementtemplatewithdefaultclassparametersandquarktemplateancestorwconstantdefaults_ish.test", // tpl constant used as default tpl param value
            "t41439_test_compiler_elementtemplatewithdefaultclassparametersandquarktemplateancestorwconstantdefaults_ish.test", // -"-
            "t41444_test_compiler_elementtemplatewithdefaultclassparametersandquarktemplateancestorwmemberconstantdefaults_unseencase_ish.test", // -"-
            "t41445_test_compiler_elementtemplatewithdefaultclassparametersandquarktemplateancestorwmemberconstantdefaults_seencase_ish.test", // -"-
            "t41447_test_compiler_quarktemplatewithtypedefanddmfrombasesbaseclasstemplateclassinstance.test", // native function result used in condition
            "t41452_test_compiler_templateclasseswiththreewayreferencingtypedefs.test", // class typedef used in parent list
            "t41453_test_compiler_templateclasseswithmembertypedefbaseclasses.test", // -"-
            "t41457_test_compiler_atomreftoaccessdatamemberofbaseclass_gencode_ish.test", // quark reference to Atom
            "t41461_test_compiler_elementandquark_multibases_virtualfuncsselectwselfandclassidconstant_atomref.test", // classid magic, TODO
            "t41469_test_compiler_instanceofconstructor_ish.test", // instanceof with arguments, TODO
            "t41471_test_compiler_constantfromanotherclasssuperarg_ish.test", // class constant used as parent tpl argument
            "t41487_test_compiler_constantatomarrayofasingleconstantelement_ish.test", // instanceof with arguments, TODO
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

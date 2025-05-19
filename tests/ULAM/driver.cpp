#include "./test_case.hpp"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

constexpr char UlamPathEnv[] = "ULAM_PATH";

static const std::set<std::string> Skip = {
    "t3241_test_compiler_unarymod.test", // invalid implicit cast?
    "t3494_test_compiler_divideandmodmixedtypes.test", // carrying consteval flag through props?
    "t3501_test_compiler_elementandquarkcaarray_unsignedindex_issue.test", // ambiguous funcall as intended
    "t3504_test_compiler_arraywithconstantindex.test", // constant used before definition
    "t3631_test_compiler_element_castatomandquark_ish.test", // tmp, quark to Atom?
    "t3675_test_compiler_elementandquark_Selfatomoffordefaultelement.test", // quark self to Atoms
    "t3747_test_compiler_elementinheritedquark_instanceof_withsuper.test", // quark to Atom via super.instanceof ??
    "t3774_test_compiler_transientwithfuncswrefarg_undefinedtemp_issue.test", // empty Atom ref argument for Mob.visit -- cannot eval condition
    "t3779_test_compiler_transientwithfuncswrefarg_castvoidreturn_issue", // -"-
    "t3779_test_compiler_transientwithfuncswrefarg_castvoidreturn_issue.test", // -"-
    "t3873_test_compiler_elementinheritance_withunorderedlocaldefs_separatefilescope.test", // type defined after use
    "t3886_test_compiler_elementandquarkswclassargs_wdefaultparamvaluefromanotherclass_reverseorder.test", // tpl param used in previous param definition
    "t3888_test_compiler_elementandquarkswclassargs_wdefaultparamvaluefromanotherclass_constantdm.test", // tpl const used as param default value
    "t3889_test_compiler_elementandquarkswclassargs_wdefaultparamvaluefromanotherclass_constantdminancestor.test", // -"-
    "t3890_test_compiler_elementandquarkswclassargs_wdefaultparamvaluefromanotherclass_localdefconstantarrayitem.test", // -"-
    "t3891_test_compiler_elementandquarkswclassargs_wdefaultparamvaluefromanotherclass_localdefconstantarrayitem.test", // -"-
    "t41043_test_compiler_forascond.test", // for (; (a as Type); ) {...}, TODO
    "t41046_test_compiler_switchascondcases.test", // which() as cond, TODO
    "t41050_test_compiler_controlswitch_emptyvalueemptybody.test", // -"-
    "t41052_test_compiler_elementandquark_castreffuncreturnvalue.test", // rvalue ref
    "t41053_test_compiler_transientandquark_castreffuncreturnvalue.test", // rvalue ref
    "t41074_test_compiler_elementandquark_customarrayonfunccallreturn.test", // assignment to xvalue
    "t41089_test_compiler_assigntofuncreturnvalueonlhs_evalerr.test", // assignment to rvalue
    "t41092_test_compiler_elementandquark_constructorcallonfuncreturninstance.test", // -"-
    "t41109_test_compiler_elementandtransient_comparisonoperatoroverloadequalequal.test", // implicit `!=' operator
    "t41112_test_compiler_elementandtransient_comparisonoperatoroverloads.test", // implicit `>=' operator
    "t41129_test_compiler_elementandquark_overloadoperatorsquareeventwindow_isparse.test", // ambiguous aref/operator[] call
    "t41214_test_compiler_elementwithclassparameterquarktemplate_Sfirstwdefault.test", // tpl param used in previous param definition
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
    "t41316_test_compiler_transientandquark_multibases_virtualfuncsselectwdatamembersandtypedefs.test", // using local alias to access base class, potentially ambiguous?
    "t41376_test_compiler_elementandquark_multibases_specificbasevariablevirtualfunccall.test", // classid magic, TODO
    "t41379_test_compiler_elementandquark_multibases_specificbasevariablevirtualfunccallusingclassidofref.test", // -"-
    "t41380_test_compiler_elementandquark_multibases_constantspecificbasevariablevirtualfunccall.test", // -"-
    "t41381_test_compiler_elementandquark_multibases_specificbasevariablevirtualfunccallusingconstantclassidofref.test", // -"-
    "t41382_test_compiler_elementandquarkswclassargs_wdefaultparamvalueandtype_insertCLASS.test", // __CLASS_*__, TODO
    "t41384_test_compiler_elementandquarkwclassargs_multibasesisasbase_virtualfuncswsharedancestoranddatamembercopies.test", // base class access args, TODO
    "t41391_test_compiler_elementandquark_multibases_virtualfuncsselectwselfandclassidconstant.test", // classid magic, TODO
    "t41392_test_compiler_elementandquark_multibases_virtualfuncsselectwselfandclassidconstant2.test", // -"-
    "t41393_test_compiler_elementandquark_multibases_virtualfuncsselectwselfSelfandclassidconstant3.test", // -"-
    "t41414_test_compiler_castingquarkreftobits_ish.test", // ref to base to Bits(Base.sizeof), TODO?? -- need "known dynamic type" flag similar to consteval for values?
    "t41422_test_compiler_castingstringtobitsandbitstostring.test", // Bits to String, ??
    "t41425_test_compiler_castingstringtobooltovalidatestringindex.test", // -"-
    "t41438_test_compiler_elementtemplatewithdefaultclassparametersandquarktemplateancestorwconstantdefaults_ish.test", // tpl constant used as default tpl param value
    "t41439_test_compiler_elementtemplatewithdefaultclassparametersandquarktemplateancestorwconstantdefaults_ish.test", // -"-
    "t41444_test_compiler_elementtemplatewithdefaultclassparametersandquarktemplateancestorwmemberconstantdefaults_unseencase_ish.test", // -"-
    "t41445_test_compiler_elementtemplatewithdefaultclassparametersandquarktemplateancestorwmemberconstantdefaults_seencase_ish.test", // -"-
    "t41447_test_compiler_quarktemplatewithtypedefanddmfrombasesbaseclasstemplateclassinstance.test", // native function result used in condition
    "t41452_test_compiler_templateclasseswiththreewayreferencingtypedefs.test", // class typedef used in parent list
    "t41453_test_compiler_templateclasseswithmembertypedefbaseclasses.test", // -"-
    "t41461_test_compiler_elementandquark_multibases_virtualfuncsselectwselfandclassidconstant_atomref.test", // classid magic, TODO
    "t41471_test_compiler_constantfromanotherclasssuperarg_ish.test", // class constant used as parent tpl argument
    "t41522_test_compiler_elementinheritance_withoutlocaldefkeyword_andvalidmembertypedef_whenclassseenfirst.test", // class typedef used in parent list
    "t41524_test_compiler_elementmultiinheritance_shadowingtypedefsinhierarchy.test", // -"-
    "t41527_test_compiler_elementtemplateinheritance_funcinbasetemplateinstance_ish.test", // parent class typedef used in class tpl param list
    "t41528_test_compiler_elementtemplateinheritance_classparametertypenotinlocalsscope_ish.test", // -"-
    "t41597_test_compiler_anotherclassconstantinsquarebracketslhs_ish.test", // out-of-range error is correct?
    "t41619_test_compiler_positionofdatamemberinbaseclassofbaseclassnoref.test", // positionof base type chain, TODO
    "t41621_test_compiler_elementpositionofdatamemberusingSelf.test", // instanceof base type chain, TODO
    "t41647_test_compiler_elementinheritedquarks_multibase_templatebaseclasswithdmandfuncnotfound.test", // class typedef used in parent list
    "t41648_test_compiler_elementinheritedquarks_templatebaseclasswithrecursivetypedefnonlocalnone_issue.test", // -"-
    "t41649_test_compiler_elementinheritedquarks_templatebaseclasswithrecursivetypedefnonlocaldm_issue.test", // -"-
    "t41650_test_compiler_elementinheritedquarks_templatebaseclasswithrecursivetypedeflocals_issue.test", // -"-
    /* added while writing postfix compiler: */
    "t3450_test_compiler_minmaxsizeoffunccallreturns.test", // converting String.lengthof from Unsigned to Int(7), TODO: consteval functions?
    "t3933_test_compiler_string_lengthof.test", // similar to t3930 in SkipCheckAnswer, `Unsigned` to `Int(8)` cast fails because .lenghtof is not consteval
    "t41042_test_compiler_whileascond.test", // while-as
    "t41134_test_compiler_elementandquark_overloadequalitycomplement.test", // implicit operator!=
    "t41481_test_compiler_switchontypefromanotherclassinfuncoftemplatedsuper_ish.test", // class typedef used in parent list
};

static const std::set<std::string> SkipAnswerCheck = {
    "t3412_test_compiler_elementandquarkwargs_functoint.test", // single test using _toIntHelper to cast returned quark
    "t3521_test_compiler_elementandquarkswclassargs_wdefaultparam.test", // constant sum correctly folded
    "t3549_test_compiler_elementandquarkcaarray_withquarkinitandinheritance_localuse.test", // using result of native `aref`
    "t3652_test_compiler_elementandquarksinherited_unseentypedef_unaryclassarg_ish.test", // classid of non-const expr
    "t3714_test_compiler_bigtransient.test", // ta(551) is correct (tested in MFM)
    "t3715_test_compiler_transientwithtransientdm.test", // similar to t3714
    "t3748_test_compiler_elementinheritedquark_sizeof_withsuper.test", // Bits "u" suffix
    "t3749_test_compiler_elementinheritedquark_datamember_withsuper.test", // -"-
    "t3805_test_compiler_transientsandelements_virtualfuncsandrefargs.test", // cannot use `System.assert(false)` without evaluating native funs
    "t3811_test_compiler_transientarraywithelementarraydm_funccallrefarg.test", // b(true), c(true) are correct (tested in MFM)
    "t3812_test_compiler_transientarraywithatomarraydm_funccallrefarg.test", // similar to t3811
    "t3814_test_compiler_transientarraywithelementarrayref.test", // -"-
    "t3816_test_compiler_transientarraywithelementarrayref_typedefref.test", // -"-
    "t3817_test_compiler_transientarraywithprimitivearrayref_typedefref.test", // -"-
    "t3818_test_compiler_transientarraywithatomarrayref_typedefref.test", // -"-
    "t3819_test_compiler_transientwithelementdmwithquarkarrayref_typedefref.test", // -"-
    "t3820_test_compiler_transientwithatomdm_typedefref.test", // -"-
    "t3832_test_compiler_transientwithtransientdmwithelementarrayref_typedefref.test", // -"-
    "t3833_test_compiler_transientwithatomdm.test", // -"-
    "t3837_test_compiler_element_quarkrefatomof_issue.test", // -"-
    "t3850_test_compiler_mixedlongbinaryops.test", // mixing 32/64-bit Ints, why `mixprod32 / mixdiv64` -> `mixprod32 cast mixdiv64 / cast cast 4 cast ==`?
    "t3887_test_compiler_elementandquarkswclassargs_wdefaultparamvaluefromanotherclass_localdefshadow.test", // local `b` used instead of tpl parameter, TODO: address in compat mode
    "t3902_test_compiler_funcdef_forloopcontinue_issue.test", // `d(0)` is correct (tested) `d(5)` means `--i` is ignored?
    "t3906_test_compiler_elementandquark_atomofmemberselectarrayitem.test", // tmp, .atomof
    "t3908_test_compiler_elementandquark_atomofmemberselectarrayitemlval.test", // -"-
    "t3909_test_compiler_elementandquark_atomofmemberselectarrayitemlval_clobberingType.test", // -"-
    "t3916_test_compiler_elementandquark_memberselectoncustomarray.test", // similar to t3811, b(true) is correct?
    "t3930_test_compiler_string_ascii.test", // .lenghtof is not consteval, since string var is not const, would require constrol flow analysis
    "t3948_test_compiler_caarrayofregulararrayofstrings3D.test", // `mi_i(120)` is correct (tested)
    "t3949_test_compiler_array2Dstring_lengthof.test", // .lengthof, similar to t3930
    "t3953_test_compiler_stringascii_constantarray_lengthof.test", // -"-
    "t3954_test_compiler_stringascii_constantarray_filescope.test", // -"-
    "t3957_test_compiler_stringascii_dbldigitlength_reflengthof_cast.test", // -"-
    "t3958_test_compiler_stringasciiarray_toobig_issue.test", // '"' in string prop values are not escaped, TODO: revisit
    "t3967_test_compiler_inheritedtransientdmcastasref_issue.test", // -"-
    "t3968_test_compiler_transientwithelementdm.test", // `Foo.ta(881)` is correct (tested)
    "t3969_test_compiler_transientwithquarkdm.test", // similar to t3968
    "t3970_test_compiler_string_assign_print.test", // .lengthof, similar to t3930
    "t3973_test_compiler_stringarray_dminit_lengthof.test", // -"-
    "t3974_test_compiler_stringarray_localinit_lengthof.test", // -"-
    "t3975_test_compiler_stringarray_localinit_morethanone.test", // -"-
    "t3977_test_compiler_productandsum3264bitboundstosaturate.test", // tmp: no Unary -> Unsigned cast for +=
    "t3978_test_compiler_divideandsubtract3264bitboundstosaturate.test", // similar to t3977
    "t3984_test_compiler_stringasciiarray_itembystringdmvarlengthof_issue.test", // .lengthof, similar to t3930
    "t3985_test_compiler_stringasciiarray_variablelengthof.test", // tmp
    "t3993_test_compiler_longstringemptystring_lengthof.test", // .lengthof, similar to t3930
    "t3995_test_compiler_stringarray_assign_print.test", // -"-
    "t41005_test_compiler_tmpvarnotdeclared_customarraygencode_issue.test", // unsigned value format
    "t41006_test_compiler_tmpvarnotdeclared_lhsfunccallgencode.test", // -"-
    "t41007_test_compiler_addstubcopytoancestorclass_customarray_issue.test", // -"-
    "t41014_test_compiler_logicalorandprecedence_issue.test", // ULAM has same precedence for || and && ?
    "t41039_test_compiler_controlswitch_nonconstantcaseexpressions.test", // seems to require runtime values to work; z = 4 does happen, so ok to skip
    "t41072_test_compiler_assignto_questioncolon.test", // ULAM parses `ok ? a : b = 2` as `(ok ? a : b) = 2`?
    "t41073_test_compiler_assigntoarrayitem_questioncolon.test", // -"-
    "t41093_test_compiler_elementandquark_quarkunionstring.test", // .lenghtof, similar to t3930
    "t41131_test_compiler_elementandquark_overloadfunccallrefargsubclass_issue.test", // unsigned value format
    "t41139_test_compiler_castatomreftoatom_issue.test", // redundant cast from Stretch.instanceof to Atom?
    "t41140_test_compiler_dividewithquestioncolon_issue.test", // tmp: casts
    "t41145_test_compiler_quarkunion_dmwithunknownsizes_issue.test", // constant not in output
    "t41168_test_compiler_classdminitarrays.test", // autofilled array format: exact init expr not available at compile time
    "t41169_test_compiler_classdminitarraysinclass.test", // -"-
    "t41170_test_compiler_classdminitarrayofquarks.test", // -"-
    "t41172_test_compiler_classdminitarrays_localvar.test", // -"-
    "t41173_test_compiler_classdminitarraysinclass_localvar.test", // -"-
    "t41174_test_compiler_classdminitarrayofquarks_localvar.test", // -"-
    "t41175_test_compiler_classdminit_elementintransient.test", // `A.b(true)` is correct (tested), prop order in object map
    "t41176_test_compiler_classdminit_elementintransientwmods.test", // -"-, our object map includes non-default props
    "t41177_test_compiler_classdminitarraysintransientwmods.test", // -"-, obj map, array format
    "t41178_test_compiler_classdminitarraysinclassintransientwmods.test", // -"-
    "t41179_test_compiler_classdminitarrayofquarksintransientwmods.test", // -"-
    "t41182_test_compiler_classdminitarrayofquarksinherited_localvar.test", // array format
    "t41183_test_compiler_classdminit_inheritedtemplateString_localvar.test", // obj map format
    "t41184_test_compiler_classdminitwithconstructor_localvar.test", // -"-
    "t41185_test_compiler_classdminitarrayofquarksinquark_issue.test", // tmp
    "t41198_test_compiler_constantclasswish.test", // constant `b1 = QFoo.c_qbar.iou == 7` folded correctly
    "t41206_test_compiler_classdminitarraysnull_localvar.test", // array format
    "t41209_test_compiler_elementwithclassparameterquarktemplate.test", // object param format (is prefix in hex classid?), folding, casts
    "t41213_test_compiler_elementwithclassparameterquarktemplate_Sfirst.test", // -"-
    "t41216_test_compiler_elementwithaclassparameterquark_constantclassemptyinit.test", // -"-
    "t41220_test_compiler_elementwithaclassparameterquark_constantclassemptyinit_Sfirst.test", // -"-
    "t41229_test_compiler_elementtemplatewithinheritedclassparameterquarktemplateandancestor_assignment_ish.test", // -"-
    "t41231_test_compiler_transientconstant_assignment.test", // `Bool m_rb(true);  Int m_ri(99);` is correct
    "t41232_test_compiler_transientconstantwelementdatamember.test", // `Bool m_testb(true);  Int m_testi(77);` is correct
    "t41233_test_compiler_transientconstantwelementdatamemberinitialized.test", // similar to t41232
    "t41234_test_compiler_transientconstantwelementarraydatamember.test", // -"-
    "t41267_test_compiler_constantclassarrayoftransients_withelementdm.test", // class const `KeyExprNode.m_elfoo` is evaluated with default eval, no codegen
    "t41268_test_compiler_constantclassarrayoftransientswithelementdm_asconstreffuncarg.test", // -"-
    "t41269_test_compiler_constantclassarrayoftransients_withtransientdm.test", // obj value format: init expr not available at compile time
    "t41270_test_compiler_constantclassarrayoftransientswithtransientdm_asconstreffuncarg.test", // -"-
    "t41271_test_compiler_dmconstantclassarrayoftransientswithtransientdm_asconstrefarrayfuncarg.test", // Bits hex padding does not match
    "t41272_test_compiler_dmconstantclassarrayoftransientswithelementdm_asconstrefarrayfuncarg.test", // similar to t41269
    "t41273_test_compiler_constantclassarrayoftransients_withelementdmwstringarray.test", // -"-
    "t41274_test_compiler_constantclassarrayoftransientswithelementdm_constantmemberofconstantclass.test", // -"-
    "t41275_test_compiler_constantclassarrayoftransientsloop.test", // .lengthof folding
    "t41276_test_compiler_constantarray2Dstring_lengthof.test", // -"-
    "t41277_test_compiler_constantclasswithstringdm.test", // -"-, element hex does not match (strings)
    "t41278_test_compiler_constantquarkclasswithstringdm.test", // quark hex does not match (strings)
    "t41292_test_compiler_refcastdm_alsoancestor_issue.test", // `qrok.m == 0`: `qrok.m` is not casted to Int in answer, TODO?
    "t41302_test_compiler_elementinheritances_castingself_exactfuncmatches.test", // `Foo.Soo.Coo.cb(true)` is correct
    "t41333_test_compiler_multiinheritance_sibilingshadowdatamemberlhs.test", // deref cast omitted, TODO: always dereference via cast helper
    "t41334_test_compiler_quarkunionstringarrayDM.test", // .lenghtof folding
    "t41336_test_compiler_multiinheritance_referenceinit.test", // deref cast omitted, similar to t41333
    "t41360_test_compiler_elementandquarks_multibases_atomofcasting_issue.test", // Bool value format
    "t41365_test_compiler_elementsandquarks_multibases_virtualfuncs_nestedascond_ish.test", // redundant cast for `as` var
    "t41383_test_compiler_elementandquarkswclassargs_classidofSelf_issue.test", // TODO: classid
    "t41394_test_compiler_elementandquark_multibases_virtualfuncsselectwextrashallowbase_issue.test", // array format
    "t41395_test_compiler_transientclassdminitwithconstructor_localvar.test", // folding: .lengthof, non-const property
    "t41402_test_compiler_classidtemplatearg_ish.test", // TODO: classids don't match, use separate ID for classes
    "t41426_test_compiler_quarkunion_datamemberbitsizezero.test", // UNINITIALIZED_STRING
    "t41434_test_compiler_elementtemplatewithinheritedclassparameterquarktemplateandancestorwlocalscontext_ish.test", // Bool format, TODO: investigate props with `Type prop( value)` format
    "t41468_test_compiler_stringdatamemberuninitialized_ish.test", // UNINITIALIZED_STRING
    "t41473_test_compiler_funccallonrefinsideifconditionafterothercalls_gencode_ish.test", // `Bool m_border(false)` is correct?
    "t41478_test_compiler_classconstantofsametemplateclass_ish.test", // constant folding, consteval cast
    "t41480_test_compiler_datamemberinitusingclassconstantoftypedefofsametemplateclass.test", // object as hex formatting: why so many zeros?
    "t41483_test_compiler_constantatom.test", // Atom initialization const expr
    "t41490_test_compiler_constantatomwaitingonconstantinclassDM_ish.test", // quark prop output format (const vars)
    "t41492_test_compiler_elementandconstantatomarray_constantreffuncparamatom.test", // element hexes do not match, Atom init value
    "t41493_test_compiler_elementandconstantatomarray_funcparamatomarray.test", // -"-
    "t41494_test_compiler_elementandconstantatom_constantfuncparamatomascond.test", // -"-
    "t41495_test_compiler_elementandconstantatom_constantfuncparamatomiscondandcast.test", // -"-
    "t41496_test_compiler_elementandconstantatom_funcparamatomiscondcasttoref.test", // -"-
    "t41498_test_compiler_elementbasequarkandconstantatom_constantfuncparamatomascond.test", // -"-
    "t41499_test_compiler_elementbasequarkandconstantatom_constantfuncparamatomiscondandcast.test", // -"-
    "t41500_test_compiler_elementbasequarkandconstantatom_funcparamatomiscondandcast.test", // -"-
    "t41503_test_compiler_element_castatomfromquarkrefimmediateconstantof.test", // constantof cast
    "t41504_test_compiler_elementandquark_elementconstantoffunccallarg.test", // -"-
};

using Path = std::filesystem::path;

static void exit_usage(std::string name) {
    std::cout << name << "[{<case-number>|'t<test-number>'}]\n";
    std::exit(-1);
}

static bool run(Path stdlib_dir, const Path& path, bool single) {
    try {
        TestCase::flags_t flags = TestCase::NoFlags;
        if (!single && SkipAnswerCheck.count(path.filename()) > 0)
            flags |= TestCase::SkipAnswerCheck;
        TestCase test_case{stdlib_dir, path, flags};
        test_case.run();
        return true;
    } catch (std::invalid_argument& e) {
        std::cerr << e.what() << "\n";
        return false;
    }
}

static bool run(Path stdlib_dir, unsigned n, std::vector<Path> test_paths, bool single) {
    assert(n > 0);
    auto& path = test_paths[n - 1];
    std::cout << "# " << std::dec << n << " " << path.filename() << "\n";
    bool ok = true;
    try {
        ok = run(stdlib_dir, path, single);
    } catch (std::exception& exc) {
        std::cout << "exception thrown\n";
        ok = false;
    }
    std::cout << "# " << std::dec << n << " " << path.filename() << " "
              << (ok ? "OK" : "FAIL") << "\n";
    return ok;
}

int main(int argc, char** argv) {
    // ULAM root
    const char* ulam_path_env = std::getenv(UlamPathEnv);
    if (!ulam_path_env) {
        std::cerr << "`" << UlamPathEnv << "' is not set\n";
        std::exit(-1);
    }
    const Path ulam_src_root{ulam_path_env};
    if (argc > 2)
        exit_usage(argv[0]);

    // ULAM paths
    const Path stdlib_dir{ulam_src_root / "share" / "ulam" / "stdlib"};
    const Path test_src_dir{
        ulam_src_root / "src" / "test" / "generic" / "safe"};

    // case number/test name
    unsigned case_num = 0;
    std::string test_name;
    if (argc > 1) {
        std::string_view arg{argv[1]};
        if (arg.empty())
            exit_usage(argv[0]);
        if (arg[0] == 't') {
            test_name = arg;
        } else {
            case_num = std::stoul(arg.data());
        }
    }

    std::vector<Path> test_paths;
    {
        auto it = std::filesystem::directory_iterator{test_src_dir};
        for (const auto& item : it)
            test_paths.push_back(item.path());
    }
    std::sort(test_paths.begin(), test_paths.end());

    if (case_num != 0) {
        if (case_num <= test_paths.size()) {
            run(stdlib_dir, case_num, test_paths, true);
        } else {
            std::cout << "case not found\n";
        }

    } else if (!test_name.empty()) {
        auto it = std::find_if(test_paths.begin(), test_paths.end(), [&](const Path& path) {
            return path.filename().string().substr(0, test_name.size()) == test_name;
        });
        if (it != test_paths.end()) {
            run(stdlib_dir, std::distance(test_paths.begin(), it) + 1, test_paths, true);
        } else {
            std::cout << "test not found\n";
        }

    } else {
        for (unsigned n = 1; n <= test_paths.size(); ++n) {
            if (Skip.count(test_paths[n - 1].filename()) > 0)
                continue;
            if (!run(stdlib_dir, n, test_paths, false))
                return -1;
        }
    }
}

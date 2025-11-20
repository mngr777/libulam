#include <libulam/semantic/program.hpp>
#include <libulam/semantic/utils/strf.hpp>
#include <libulam/semantic/value/types.hpp>
#include <sstream>

namespace {
constexpr const char NoValue[] = "<no-value>";
constexpr const char InvalidStrId[] = "<invalid-str-id>";
constexpr const char Atom[] = "<Atom>";
} // namespace

namespace ulam::utils {

Strf::Strf(Ref<Program> program):
    _builtins{program->builtins()},
    _str_pool{program->str_pool()},
    _text_pool{program->text_pool()} {}

std::string Strf::str(Ref<Type> type, const Value& val) {
    return val.accept([&](const auto& v) { return str(type, v); });
}

std::string Strf::str(Ref<Type> type, const LValue& lval) {
    std::stringstream ss;
    str(ss, type, lval);
    return ss.str();
}

std::string Strf::str(Ref<Type> type, const RValue& rval) {
    std::stringstream ss;
    str(ss, type, rval);
    return ss.str();
}

void Strf::str(std::ostream& os, Ref<Type> type, const Value& val) {
    return val.accept([&](const auto& v) { str(os, type, v); });
}

void Strf::str(std::ostream& os, Ref<Type> type, const LValue& lval) {
    lval.with_rvalue([&](const RValue& rval) { str(os, type, rval); });
}

void Strf::str(std::ostream& os, Ref<Type> type, const RValue& rval) {
    if (rval.empty()) {
        os << NoValue;
        return;
    }

    type = type->deref();
    if (type->is_prim()) {
        write_prim(os, type->as_prim(), rval);
    } else if (type->is_array()) {
        write_array(os, type->as_array(), rval);
    } else if (type->is_class()) {
        write_class(os, type->as_class(), rval);
    } else if (type->is_atom()) {
        os << Atom;
    } else {
        assert(false);
    }
}

void Strf::write_prim(
    std::ostream& os, Ref<PrimType> type, const RValue& rval) {
    switch (type->bi_type_id()) {
    case IntId: {
        os << rval.get<Integer>();
    } break;
    case UnsignedId: {
        os << rval.get<Unsigned>();
    } break;
    case UnaryId:
    case BoolId: {
        os << std::hex << "0x" << rval.get<Unsigned>();
    } break;
    case BitsId: {
        os << rval.get<Bits>().hex();
    } break;
    case StringId: {
        auto str_id = rval.get<String>().id;
        if (!_text_pool.has_id(str_id)) {
            os << InvalidStrId;
            break;
        }
        auto str = _text_pool.get(str_id);
        os << '"' << str << '"';
        break;
    }
    default:
        assert(false);
    }
}

void Strf::write_array(
    std::ostream& os, Ref<ArrayType> type, const RValue& rval) {
    os << "(";
    for (array_idx_t idx = 0; idx < type->array_size(); ++idx) {
        if (idx > 0)
            os << ", ";
        auto lval = rval.array_access(idx, false);
        lval.with_rvalue(
            [&](const RValue& item) { str(os, type->item_type(), item); });
    }
    os << ")";
}

void Strf::write_class(std::ostream& os, Ref<Class> cls, const RValue& rval) {
    os << "{";
    bool is_first = true;
    for (auto prop : cls->all_props()) {
        if (!is_first)
            os << ", ";
        auto lval = rval.prop(prop);
        auto label = _str_pool.get(prop->name_id());
        os << ".";
        if (prop->cls() != cls)
            os << prop->cls()->name();
        os << label << " = ";
        lval.with_rvalue(
            [&](const RValue& prop_rval) { str(os, prop->type(), prop_rval); });
        is_first = false;
    }
    os << "}";
}

} // namespace ulam::utils

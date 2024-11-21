#pragma once
#include <cassert>
#include <cstdint>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/value.hpp>
#include <string>
#include <unordered_map>
#include <libulam/semantic/type_id_gen.hpp>

namespace ulam {

using bitsize_t = std::uint16_t;

template <
    BuiltinTypeId _TypeId,
    bitsize_t Min,
    bitsize_t Max,
    bitsize_t Default>
class PrimType : public BasicType {
    static_assert(_TypeId != BuiltinTypeId::AtomId);
    static_assert(Min > 0 == Max > 0);
    static_assert(Max > 0 == Default > 0);

public:
    static constexpr BuiltinTypeId TypeId = _TypeId;
    static constexpr bitsize_t MinSize = Min;
    static constexpr bitsize_t MaxSize = Max;
    static constexpr bitsize_t DefaultSize = Default;

    PrimType(type_id_t id, bitsize_t bitsize):
        BasicType{id}, _bitsize{bitsize} {}

    bitsize_t bitsize() const { return _bitsize; }

private:
    bitsize_t _bitsize;
};

template <typename T> class PrimTypeTpl : public TypeTpl {
public:
    Ref<Type> type(
        Diag& diag,
        ast::Ref<ast::ArgList> arg_list,
        TypeIdGen& id_gen,
        ValueList& args) {
        // TODO: more precise error locations
        if constexpr (T::MaxSize == 0) {
            if (args.size() > 0) {
                diag.emit(
                    diag::Error, arg_list->loc_id(), 1,
                    std::string(type_str()) + " does not have parameters");
                // continue
            }
            return get(id_gen, 0);
        }
        bitsize_t size = 0;
        if (args.size() == 0) {
            size = T::DefaultSize;
        } else {
            if (args.size() > 1) {
                diag.emit(
                    diag::Error, arg_list->loc_id(), 1,
                    "too many arguments for " + type_str());
                // continue
            }
            // get first arg
            auto& arg = args.front();
            if (arg.is<Integer>()) {
                auto int_val = arg.get<Integer>();
                size = (int_val < 0) ? 0 : static_cast<Unsigned>(int_val);
            } else if (arg.is<Unsigned>()) {
                size = arg.get<Unsigned>();
            } else {
                diag.emit(
                    diag::Error, arg_list->loc_id(), 1,
                    "cannot convert to bit size");
                return {};
            }
            // check, adjust and continue on error
            if (size < T::MinSize) {
                diag.emit(
                    diag::Error, arg_list->loc_id(), 1,
                    "bit size argument must be at least " +
                        std::to_string(T::MinSize));
                size = T::MinSize;
            } else if (size > T::MaxSize) {
                diag.emit(
                    diag::Error, arg_list->loc_id(), 1,
                    "bit size argument must be at most " +
                        std::to_string(T::MaxSize));
                size = T::MaxSize;
            }
        }
        return get(id_gen, size);
    }

private:
    Ref<Type> get(TypeIdGen& id_gen, bitsize_t size) {
        {
            auto it = _types.find(size);
            if (it != _types.end())
                return ref(it->second);
        }
        auto type = make<T>(id_gen.next(), size);
    }

    std::string_view type_str() { return builtin_type_str(T::TypeId); }

    std::unordered_map<bitsize_t, Ptr<T>> _types;
};

} // namespace ulam

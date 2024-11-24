#pragma once
#include <cassert>
#include <libulam/ast/nodes/params.hpp>
#include <libulam/diag.hpp>
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/type.hpp>
#include <libulam/semantic/type/builtin_type_id.hpp>
#include <libulam/semantic/type_tpl.hpp>
#include <libulam/semantic/value.hpp>
#include <string>
#include <unordered_map>

namespace ulam {

using bitsize_t = std::uint16_t;

class Program;

class PrimType : public BasicType {
public:
    PrimType(Ref<Program> program);

protected:
    Ref<Program> program() { return _program; }

private:
    Ref<Program> _program;
};

template <
    BuiltinTypeId _TypeId,
    bitsize_t Min,
    bitsize_t Max,
    bitsize_t Default>
class _PrimType : public PrimType {
    static_assert(_TypeId != NoBuiltinTypeId);
    static_assert(_TypeId != AtomId);
    static_assert(_TypeId != StringId);
    static_assert(0 < Min);
    static_assert(Min <= Max);
    static_assert(Min <= Default && Default <= Max);

public:
    static constexpr BuiltinTypeId TypeId = _TypeId;
    static constexpr bitsize_t MinSize = Min;
    static constexpr bitsize_t MaxSize = Max;
    static constexpr bitsize_t DefaultSize = Default;

    _PrimType(Ref<Program> program, bitsize_t bitsize):
        PrimType{program}, _bitsize{bitsize} {}

    BuiltinTypeId builtin_type_id() const override { return TypeId; }
    bitsize_t bitsize() const override { return _bitsize; }

private:
    bitsize_t _bitsize;
};

class PrimTypeTpl : public TypeTpl {
public:
    PrimTypeTpl(Ref<Program> program);

    Ref<Type> type(ast::Ref<ast::ArgList> arg_list, ValueList& args) override = 0;
    virtual Ref<Type> type(ast::Ref<ast::Node> node, bitsize_t bitsize) = 0;

protected:
    Diag& _diag;
};

template <typename T> class _PrimTypeTpl : public PrimTypeTpl {
    static_assert(T::MaxSize > 0);

public:
    _PrimTypeTpl(Ref<Program> program): PrimTypeTpl{program} {}

    Ref<Type> type(ast::Ref<ast::ArgList> arg_list, ValueList& args) {
        bitsize_t size = 0;
        if (args.size() == 0) {
            size = T::DefaultSize;
        } else {
            if (args.size() > 1) {
                _diag.emit(
                    diag::Error, arg_list->loc_id(), 1,
                    std::string("too many arguments for ") + type_str());
                // continue
            }
            // get first arg
            auto& arg = args.front();
            RValue* value = arg.rvalue();
            assert(value);
            if (value->is<Integer>()) {
                auto int_val = value->get<Integer>();
                size = (int_val < 0) ? 0 : static_cast<Unsigned>(int_val);
            } else if (value->is<Unsigned>()) {
                size = value->get<Unsigned>();
            } else {
                _diag.emit(
                    diag::Error, arg_list->loc_id(), 1,
                    "cannot convert to bit size");
                return {};
            }
        }
        return type(arg_list, size);
    }

    Ref<Type> type(ast::Ref<ast::Node> node, bitsize_t size) {
        // check, adjust and continue on error
        if (size < T::MinSize) {
            _diag.emit(
                diag::Error, node->loc_id(), 1,
                std::string("bit size argument must be at least ") +
                    std::to_string(T::MinSize));
            size = T::MinSize;
        } else if (size > T::MaxSize) {
            _diag.emit(
                diag::Error, node->loc_id(), 1,
                std::string("bit size argument must be at most ") +
                    std::to_string(T::MaxSize));
            size = T::MaxSize;
        }
        return get(size);
    }

private:
    Ref<Type> get(bitsize_t size) {
        {
            auto it = _types.find(size);
            if (it != _types.end())
                return ref(it->second);
        }
        auto type = make<T>(program(), size);
        auto type_ref = ref(type);
        _types[size] = std::move(type);
        return type_ref;
    }

    std::string type_str() const {
        return std::string{builtin_type_str(T::TypeId)};
    }

    std::unordered_map<bitsize_t, Ptr<T>> _types;
};

} // namespace ulam

#pragma once
#include <cassert>
#include <libulam/semantic/type.hpp>

namespace ulam {

class PhType : public Type {
public:
    explicit PhType(type_id_t id): Type{id} {}

    bool is_placeholder() const override { return true; }

    Ref<Type> prev() override {
        assert(false);
        return {};
    }

    Ref<const Type> prev() const override {
        assert(false);
        return {};
    }

    Ref<BasicType> basic() override {
        assert(false);
        return {};
    };

    Ref<const BasicType> basic() const override {
        assert(false);
        return {};
    }

    array_size_t array_size() const override {
        assert(false);
        return {};
    }

    bool is_reference() const override {
        assert(false);
        return {};
    }

    Ref<Operator> op(Op op, Ref<Type> rhs_type) override {
        assert(false);
        return {};
    }
};

} // namespace ulam

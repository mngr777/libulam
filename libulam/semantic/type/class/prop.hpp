#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/data.hpp>
#include <libulam/semantic/value/types.hpp>
#include <libulam/semantic/var/base.hpp>

namespace ulam {

class Class;
class RValue;

class Prop : public VarBase {
public:
    using VarBase::VarBase;

    RValue load(DataPtr obj) const;
    void store(DataPtr obj, RValue&& rval) const;

    bitsize_t data_off_in(Ref<Class> derived) const;
    bitsize_t data_off_in(DataPtr obj) const;

    bool has_data_off() const { return _data_off != NoBitsize; }
    bitsize_t data_off() const;
    void set_data_off(bitsize_t off);

    bool has_default_value() const;
    const RValue& default_value() const;
    void set_default_value(RValue&& rval);

private:
    bitsize_t _data_off{NoBitsize};
    RValue _default_value{};
};

} // namespace ulam

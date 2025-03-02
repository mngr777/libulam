#pragma once
#include <libulam/memory/ptr.hpp>
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

private:
    bitsize_t _data_off{NoBitsize};
};

} // namespace ulam

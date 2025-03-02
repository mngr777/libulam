#pragma once
#include <libulam/memory/ptr.hpp>
#include <libulam/semantic/value/object.hpp>
#include <libulam/semantic/value/types.hpp>
#include <libulam/semantic/var/base.hpp>

namespace ulam {

class Class;

class Prop : public VarBase {
public:
    using VarBase::VarBase;

    ObjectView obj_view(ObjectView obj_view);
    BitsView bits_view(ObjectView obj_view);

    RValue load(const ObjectView obj_view) const;
    void store(ObjectView obj_view, const RValue& rval);

    bitsize_t data_off_in(Ref<Class> derived) const;

    bool has_data_off() const { return _data_off != NoBitsize; }
    bitsize_t data_off() const;
    void set_data_off(bitsize_t off);

private:
    bitsize_t _data_off{NoBitsize};
};

} // namespace ulam

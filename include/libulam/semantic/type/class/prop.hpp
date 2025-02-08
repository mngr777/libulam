#pragma once
#include <libulam/semantic/type/class/layout.hpp>
#include <libulam/semantic/var/base.hpp>

namespace ulam {

class Prop : public VarBase {
public:
    using VarBase::VarBase;

    bool has_data_off() const { return _data_off == cls::NoDataOff; }
    cls::data_off_t data_off() const { return _data_off; }
    void set_data_off(cls::data_off_t off) { _data_off = off; }

private:
    cls::data_off_t _data_off{cls::NoDataOff};
};

} // namespace ulam

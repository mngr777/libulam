#include <libulam/semantic/value.hpp>
#include <libulam/semantic/value/object.hpp>
#include <utility>
#include <vector>

namespace ulam {

class Object::Data {
public:
    Data(unsigned size): _size{size} {}

    void set(unsigned idx, Value&& value) { std::swap(_get(idx), value); }

    Value& get(unsigned idx) { return _get(idx); }
    const Value& get(unsigned idx) const { return _get(idx); }

private:
    const Value& _get(unsigned idx) const {
        return const_cast<Data*>(this)->_get(idx);
    }

    Value& _get(unsigned idx) {
        assert(idx < _size);
        if (idx >= _values.size())
            _values.resize(idx + 1);
        return _values[idx];
    }

    unsigned _size;
    std::vector<Value> _values;
};

Object::Object(Ref<Class> cls): _cls{cls}, _data{make<Data>(1 /* TMP */)} {}

Object::~Object() {}

Value& Object::get(unsigned idx) { return _data->get(idx); }
const Value& Object::get(unsigned idx) const { return _data->get(idx); }

void Object::set(unsigned idx, Value&& value) {
    _data->set(idx, std::move(value));
}

} // namespace ulam

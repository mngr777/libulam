#include <libulam/lang/scope.hpp>
#include <libulam/lang/type.hpp>

namespace ulam {

Class::Class(Ref<ast::ClassDef> node, Kind kind): _kind(kind), _scope{} {}

Class::~Class() {}

} // namespace ulam

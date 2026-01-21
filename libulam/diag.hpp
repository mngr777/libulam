#pragma once
#include <cstdint>
#include <libulam/memory/ptr.hpp>
#include <libulam/src_man.hpp>

namespace ulam::ast {
class Node;
}

namespace ulam {

class Diag {
public:
    enum Level : std::uint8_t { Fatal = 0, Error, Warn, Notice, Debug };

    Diag(SrcMan& src_man): _src_man{src_man} {}

    template <typename... Ts> void fatal(Ts... args) {
        emit(Diag::Fatal, std::forward<Ts>(args)...);
    }

    template <typename... Ts> void error(Ts... args) {
        emit(Diag::Error, std::forward<Ts>(args)...);
    }

    template <typename... Ts> void warn(Ts... args) {
        emit(Diag::Warn, std::forward<Ts>(args)...);
    }

    template <typename... Ts> void notice(Ts... args) {
        emit(Diag::Notice, std::forward<Ts>(args)...);
    }

    template <typename... Ts> void debug(Ts... args) {
        emit(Diag::Debug, std::forward<Ts>(args)...);
    }

    void
    emit(Diag::Level lvl, Ref<const ast::Node> node, const std::string& text);

    void emit(
        Diag::Level lvl,
        loc_id_t loc_id,
        std::size_t len,
        const std::string& text);

    void emit(
        Diag::Level lvl,
        loc_id_t loc_id,
        int off,
        std::size_t len,
        const std::string& text);

private:
    SrcMan& _src_man;
    unsigned _err_num{0};
};

} // namespace ulam

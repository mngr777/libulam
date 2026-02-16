#include <filesystem>
#include <libulam/utils/file.hpp>
#include <tuple>

namespace {
using std::filesystem::exists;
}

namespace ulam::utils {

Path PathResolver::resolve(const Path& path) {
    // auto it = _mapped.find(path);
    // if (it != _mapped.end())
    //     return it->second;

    for (const auto& include_path : _include_paths) {
        auto full_path = include_path / path;
        if (exists(full_path)) {
            // std::tie(it, std::ignore) = _mapped.emplace(path, full_path);
            return full_path;
            break;
        }
    }
    // std::tie(it, std::ignore) = _mapped.emplace(path, Path{});
    // return it->second;
    return {};
}

Path find_file_rel(const Path& rel_path, const Path& path) {
    auto full_path = path.parent_path() / rel_path;
    return exists(full_path) ? full_path : Path{};
}

} // namespace ulam::utils

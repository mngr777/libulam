#pragma once
#include <libulam/types.hpp>
#include <unordered_map>

namespace ulam::utils {

class PathResolver {
public:
    explicit PathResolver(const PathList& include_paths): _include_paths{include_paths} {}

    PathResolver(PathResolver&) = delete;
    PathResolver& operator=(PathResolver&) = delete;

    Path resolve(const Path& path);

private:
    PathList _include_paths;
    // std::unordered_map<Path, Path> _mapped;
};

Path find_file_rel(const Path& rel_path, const Path& path);

} // namespace ulam::utils

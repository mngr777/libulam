#include <iostream>

#ifdef ULAM_DEBUG

# ifndef ULAM_DEBUG_PREFIX
#  define ULAM_DEBUG_PREFIX ""
# endif

# define debug() std::cerr << ULAM_DEBUG_PREFIX
#else
# define debug() while (false) std::cerr
#endif

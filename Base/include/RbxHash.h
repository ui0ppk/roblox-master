#pragma once

#ifdef _WIN32
#include <hash_map>
using stdext::hash_map;
#else
#include <ext/hash_map>
using __gnu_cxx::hash_map;
#endif

#pragma once

#include <stdint.h>

#ifdef _DEBUG
// This will catch some really nasty bugs:
#pragma warning(error: 4702)
#endif

#ifndef _DEBUG

#if _SECURE_SCL != 0
#error Define _SECURE_SCL equal to 0 in release builds!
#endif

#endif
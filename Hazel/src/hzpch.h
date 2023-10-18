#pragma once

#ifdef HZ_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include <memory>
#include <vector>
#include <string>
#include <array>
#include <unordered_map>
#include <functional>
#include <algorithm>

#include <fstream>

#include <Hazel/Core/Base.h>
#include <Hazel/Core/Log.h>
#include <Hazel/Core/Events/Event.h>

// Math
#include <Hazel/Core/Math/Mat4.h>
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>
#include <vector>
#include <functional>

#include "rhi_class.h"

namespace Dao {

	#define RHI_DELETE_PTR(x) delete x; x = nullptr;

	class RHI {

	};
}
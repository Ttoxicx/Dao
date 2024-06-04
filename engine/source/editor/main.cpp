#include <iostream>
#include <assimp/Importer.hpp>
#include "runtime/core/math/math_headers.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/render_guid_allocator.h"

namespace Dao {
	void logTest() {
		LOG_ERROR("engine is still in initial stage")
	}
}

int main() {
	auto importer = Assimp::Importer();
	try {
		importer.ReadFile("/////////////////", 0);
	}
	catch (...) {
		std::cout << "exception occur" << std::endl;
	}

	auto res = importer.GetScene();
	if (res == nullptr) {
		std::cout << "invalide scene" << std::endl;
	}

	Dao::WindowSystem window{};
	window.initialize(Dao::WindowCreateInfo{});
	while (!window.shouldClose())
	{
		window.pollEvents();
	}

	{
		std::cout << "core math test" << std::endl;
		Dao::Vector3 vec{ 1, 1, 1 };
		std::cout << "Dao::Vector3 x: " << vec.x<<" y: " << vec.y <<" z: " << vec.z << std::endl;
	}

	{
		Dao::GuidAllocator<int> allocator;
		for (int i = 0; i < 100; ++i) {
			allocator.allocGuid(i);
		}
		for (int i = 0; i < 100; ++i) {
			size_t x = 0;
			allocator.getElementGuid(i, x);
			std::cout << x << std::endl;
		}
	}
	Dao::g_runtime_global_context.startSystem();
	Dao::logTest();
	Dao::g_runtime_global_context.shutdownSystem();
	return 0;
}
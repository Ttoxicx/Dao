#include <iostream>
#include "runtime/core/math/math_headers.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/render/window_system.h"
#include "runtime/function/render/render_guid_allocator.h"
#include "runtime/core/meta/test.h"
#include "runtime/engine.h"

namespace Dao {
	namespace Test {
		void logTest() {
			LOG_ERROR("engine is still in initial stage")
		}
		void metaTest() {
			Dao::metaTest();
		}
	}
}

int main() {
	
	Dao::DaoEngine engine;

	engine.startEngine();
	engine.initialize();
	engine.clear();
	engine.shutdownEngine();

	return 0;
}
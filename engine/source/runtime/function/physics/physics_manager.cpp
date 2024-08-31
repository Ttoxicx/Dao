#include "runtime/function/physics/physics_manager.h"

#include "runtime/function/physics/physics_scene.h"

namespace Dao {
	void PhysicsManager::initialize() {

	}

	void PhysicsManager::clear() {
		m_scenes.clear();
	}

	std::weak_ptr<PhysicsScene> PhysicsManager::createPhysicalScene(const Vector3& gravity) {
		std::shared_ptr<PhysicsScene> physics_scene = std::make_shared<PhysicsScene>(gravity);
		m_scenes.push_back(physics_scene);
		return physics_scene;
	}

	void PhysicsManager::deletePhysicsScene(std::weak_ptr<PhysicsScene> physics_scene) {
		std::shared_ptr<PhysicsScene> deleted_scene = physics_scene.lock();
		auto itr = std::find(m_scenes.begin(), m_scenes.end(), deleted_scene);
		if (itr != m_scenes.end()) {
			m_scenes.erase(itr);
		}
	}
}
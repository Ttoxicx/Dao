#pragma once

#include "runtime/resource/res_type/common/world.h"

#include <filesystem>
#include <string>

namespace Dao {

	class Level;
	class PhysicsScene;

	class WorldManager {
	public:
		virtual ~WorldManager();

		void initialize();
		void clear();

		void reloadCurrentLevel();
		void saveCurrentLevel();

		void tick(float delta_time);
		
		std::weak_ptr<Level> getCurrentActiveLevel() const { return m_current_active_level; }
		std::weak_ptr<PhysicsScene> getCurrentActivePhysicsScene() const;
	
	private:
		bool loadWorld(const std::string& world_url);
		bool loadLevel(const std::string& level_url);

	private:
		bool _is_world_loaded{ false };
		std::string _current_world_url;
		std::shared_ptr<WorldRes> _current_world_resource;

		std::unordered_map<std::string, std::shared_ptr<Level>> m_loaded_levels;
		std::weak_ptr<Level> m_current_active_level;
	};
}
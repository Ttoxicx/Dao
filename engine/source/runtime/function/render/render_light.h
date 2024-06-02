#pragma once

#include "runtime/core/math/vector3.h"
#include "runtime/function/render/render_type.h"

#include <vector>

namespace Dao {
	struct PointLight
	{
		Vector3 m_position;
		//radiant flux in R, G, B channels
		Vector3 m_flux;
		//calculate the effective radius
		float calculateRadius() const {
			const float INTENSITY_CUTOFF = 1.0f;
			const float ATTENTUATION_CUTOFF = 0.05f;
			//radiant flux in unit sphere radius (W/sr)
			Vector3 intensity = m_flux / (4.0f * Math_PI);
			//max intensity among R, G, B channels
			float maxIntensity = Vector3::getMaxElement(intensity);
			float attenuation = Math::max(INTENSITY_CUTOFF, ATTENTUATION_CUTOFF * maxIntensity) / maxIntensity;
			return 1.0f / sqrtf(attenuation);
		}
	};

	struct AmbientLight {
		//radiant flux in unit area (W/(m*m))
		Vector3 m_irradiance;
	};

	struct PDirectionalLight{
		Vector3 m_direction;
		Vector3 m_color;
	};

	struct LightList {
		struct {
			Vector3 m_position;
			float m_padding;
			Vector3 m_intensity;
			float m_radius;
		};
	};

	class PointLightList :public LightList {
	public:
		void init() {}
		void shutdown() {}
		void update() {}
		std::vector<PointLight> m_lights;
		std::shared_ptr<BufferData> m_buffer;
	};
}

#pragma once

#include "runtime/core/math/vector2.h"
#include "runtime/core/math/vector3.h"
#include "runtime/core/math/vector4.h"

#include "runtime/function/render/interface/rhi.h"

#include <array>

namespace Dao {
	struct MeshVertex {

		struct VulkanMeshVertexPosition {
			Vector3 position;
		};

		struct VulkanMeshVertexVaryingEnableBlending
		{
			Vector3 normal;
			Vector3 tangent;
		};

		struct VulkanMeshVertexVarying
		{
			Vector2 texcoord;
		};

		struct VulkanMeshVertexJointBinding {
			int indices[4];
			Vector4 weights;
		};

		static std::array<RHIVertexInputBindingDescription, 3> getBindingDescriptions() {
			std::array<RHIVertexInputBindingDescription, 3> binding_descriptions{};
			
			binding_descriptions[0].binding = 0;
			binding_descriptions[0].stride = sizeof(VulkanMeshVertexPosition);
			binding_descriptions[0].inputRate = RHI_VERTEX_INPUT_RATE_VERTEX;

			binding_descriptions[1].binding = 1;
			binding_descriptions[1].stride = sizeof(VulkanMeshVertexVaryingEnableBlending);
			binding_descriptions[1].inputRate = RHI_VERTEX_INPUT_RATE_VERTEX;

			binding_descriptions[2].binding = 2;
			binding_descriptions[2].stride = sizeof(VulkanMeshVertexVarying);
			binding_descriptions[2].inputRate = RHI_VERTEX_INPUT_RATE_VERTEX;

			return binding_descriptions;
		}

		static std::array<RHIVertexInputAttributeDescription, 4> getAttributeDescription() {
			std::array<RHIVertexInputAttributeDescription, 4> attribute_descriptions{};

			attribute_descriptions[0].binding = 0;
			attribute_descriptions[0].location = 0;
			attribute_descriptions[0].format = RHI_FORMAT_R32G32B32_SFLOAT;
			attribute_descriptions[0].offset = offsetof(VulkanMeshVertexPosition, position);

			attribute_descriptions[1].binding = 1;
			attribute_descriptions[1].location = 1;
			attribute_descriptions[1].format = RHI_FORMAT_R32G32B32_SFLOAT;
			attribute_descriptions[1].offset = offsetof(VulkanMeshVertexVaryingEnableBlending, normal);

			attribute_descriptions[2].binding = 1;
			attribute_descriptions[2].location = 2;
			attribute_descriptions[2].format = RHI_FORMAT_R32G32B32_SFLOAT;
			attribute_descriptions[2].offset = offsetof(VulkanMeshVertexVaryingEnableBlending, tangent);

			attribute_descriptions[3].binding = 2;
			attribute_descriptions[3].location = 3;
			attribute_descriptions[3].format = RHI_FORMAT_R32G32_SFLOAT;
			attribute_descriptions[3].offset = offsetof(VulkanMeshVertexVarying, texcoord);
			
			return attribute_descriptions;
		}
	};
}
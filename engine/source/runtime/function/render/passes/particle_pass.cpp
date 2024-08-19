#include "runtime/function/render/passes/particle_pass.h"

#include "runtime/function/render/interface/vulkan/vulkan_rhi.h"
#include "runtime/function/render/interface/vulkan/vulkan_util.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_system.h"
#include "core/base/macro.h"

#include <particle_emit_comp.h>
#include <particle_kickoff_comp.h>
#include <particle_simulate_comp.h>
#include <particlebillboard_vert.h>
#include <particlebillboard_frag.h>

namespace Dao {

}
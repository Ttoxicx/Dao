#include "runtime/function/render/render_pass_base.h"

namespace Dao {
	void RenderPassBase::postInitialize() {}
	void RenderPassBase::setCommonInfo(RenderPassCommonInfo common_info) {
		m_rhi = common_info.m_rhi;
		m_render_resource = common_info.m_render_resource;
	}
	void RenderPassBase::preparePassData(std::shared_ptr<RenderResourceBase> render_resource) {}
	void RenderPassBase::initializeUIRenderBackend(WindowUI* window_ui) {}
}
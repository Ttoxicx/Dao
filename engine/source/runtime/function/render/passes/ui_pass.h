#pragma once

#include "runtime/function/render/render_pass.h"

namespace Dao {
	
	class WindowUI;

	struct UIPassInitInfo :RenderPassInitInfo {
		RHIRenderPass* render_pass;
	};

	class UIPass :public RenderPass {
	public:
		void initialize(const RenderPassInitInfo* init_info) override final;
		void initializeUIRenderBackend(WindowUI* window_ui) override final;
		void draw() override final;

	private:
		void createFonts();
		void destroyFonts();

	private:
		WindowUI* _window_ui;
	};
}
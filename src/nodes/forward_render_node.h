#pragma once

#include "../render_node.h"

namespace nimble
{
	class ForwardRenderNode : public SceneRenderNode
	{
	public:
		ForwardRenderNode(RenderGraph* graph);
		~ForwardRenderNode();

		void execute(const View& view) override;
		bool initialize() override;
		void shutdown() override;
		std::string name() override;
		std::string vs_template_path() override;
		std::string fs_template_path() override;

	private:
		std::shared_ptr<RenderTarget> m_color_rt;
		std::shared_ptr<RenderTarget> m_depth_rt;
		std::shared_ptr<RenderTarget> m_velocity_rt;
		RenderTargetView m_color_rtv;
		RenderTargetView m_velocity_rtv;
		RenderTargetView m_depth_rtv;
	};
}
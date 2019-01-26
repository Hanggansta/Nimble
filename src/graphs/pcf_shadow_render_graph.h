#pragma once

#include "../render_graph.h"

namespace nimble
{
	class PCFShadowRenderGraph : public ShadowRenderGraph
	{
	public:
		PCFShadowRenderGraph(Renderer* renderer);
		std::string name() override;
		bool build() override;
		void refresh() override;
		std::string sampling_source() override;
	};
}
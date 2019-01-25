#pragma once

#include "../render_graph.h"

namespace nimble
{
	class PCFRenderGraph : public RenderGraph
	{
	public:
		PCFRenderGraph(Renderer* renderer);
		std::string name() override;
		bool build() override;
		void refresh() override;
	};
}
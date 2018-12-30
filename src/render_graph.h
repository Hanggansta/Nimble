#pragma once

#include "render_node.h"
#include "view.h"

namespace nimble
{
	class RenderGraph
	{
	public:
		RenderGraph();
		~RenderGraph();

		bool initialize();
		void shutdown();
		void clear();
		void execute(const View& view);
		uint32_t id();
		bool attach_and_initialize_node(std::shared_ptr<RenderNode> node);
		std::shared_ptr<RenderNode> node_by_name(const std::string& name);
		void on_window_resized(const uint32_t& w, const uint32_t& h);

		inline uint32_t window_width() { return m_window_width; }
		inline uint32_t window_height() { return m_window_height; }
		inline virtual uint32_t actual_viewport_width() { return window_width(); }
		inline virtual uint32_t actual_viewport_height() { return window_height(); }
		inline virtual uint32_t rendered_viewport_width() { return actual_viewport_width(); }
		inline virtual uint32_t rendered_viewport_height() { return actual_viewport_height(); }

		virtual std::string name() = 0;
		virtual bool build() = 0;
		virtual void refresh() = 0;

	private:
		uint32_t m_id;
		uint32_t m_window_width;
		uint32_t m_window_height;
		std::vector<std::shared_ptr<RenderNode>> m_nodes;
	};
}
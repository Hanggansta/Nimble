#include "render_graph.h"

namespace nimble
{
	static uint32_t g_last_graph_id = 0;

	// -----------------------------------------------------------------------------------------------------------------------------------

	RenderGraph::RenderGraph() : m_id(g_last_graph_id++)
	{

	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	RenderGraph::~RenderGraph()
	{

	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	bool RenderGraph::initialize()
	{
		return build();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void RenderGraph::shutdown()
	{
		for (auto& node : m_nodes)
			node->shutdown();

		clear();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void RenderGraph::clear()
	{
		m_nodes.clear();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void RenderGraph::execute()
	{
		for (auto& node : m_nodes)
			node->execute();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	uint32_t RenderGraph::id()
	{
		return m_id;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	bool RenderGraph::attach_and_initialize_node(std::shared_ptr<RenderNode> node)
	{
		m_nodes.push_back(node);

		return node->initialize();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	std::shared_ptr<RenderNode> RenderGraph::node_by_name(const std::string& name)
	{
		for (const auto& node : m_nodes)
		{
			if (node->name() == name)
				return node;
		}

		return nullptr;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace nimble
#include "mesh.h"
#include "ogl.h"

namespace nimble
{
	// -----------------------------------------------------------------------------------------------------------------------------------

	Mesh::Mesh(const std::string& name,
		const glm::vec3& max_extents,
		const glm::vec3& min_extents,
		const std::vector<ast::SubMesh>& submeshes,
		const std::vector<std::shared_ptr<Material>>& materials,
		std::shared_ptr<VertexBuffer> vertex_buffer,
		std::shared_ptr<IndexBuffer> index_buffer,
		std::shared_ptr<VertexArray> vertex_array) : m_name(name),
									 m_submeshes(submeshes),
									 m_materials(materials),
									 m_vertex_buffer(vertex_buffer),
									 m_index_buffer(index_buffer),
									 m_vertex_array(vertex_array)
	{
		m_aabb.min = min_extents;
		m_aabb.max = max_extents;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	Mesh::~Mesh()
	{
		m_vertex_array.reset();
		m_index_buffer.reset();
		m_vertex_buffer.reset();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Mesh::bind()
	{
		m_vertex_array->bind();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Mesh::bind_material(Program* program, const uint32_t& index, int32_t& unit)
	{
		m_materials[index]->bind(program, unit);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	ast::SubMesh& Mesh::submesh(const uint32_t& index)
	{
		return m_submeshes[index];
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	uint32_t Mesh::submesh_count()
	{
		return m_submeshes.size();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	AABB Mesh::aabb()
	{
		return m_aabb;
	}

	// -----------------------------------------------------------------------------------------------------------------------------------
}
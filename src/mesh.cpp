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
		VertexBuffer* vertex_buffer,
		IndexBuffer* index_buffer,
		VertexArray* vertex_array) : m_name(name),
									 m_max_extents(max_extents),
									 m_min_extents(min_extents),
									 m_submeshes(submeshes),
									 m_materials(materials),
									 m_vertex_buffer(vertex_buffer),
									 m_index_buffer(index_buffer),
									 m_vertex_array(vertex_array)
	{

	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	Mesh::~Mesh()
	{
		NIMBLE_SAFE_DELETE(m_vertex_array);
		NIMBLE_SAFE_DELETE(m_index_buffer);
		NIMBLE_SAFE_DELETE(m_vertex_buffer);
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Mesh::bind()
	{
		m_vertex_array->bind();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	void Mesh::bind_material(Program* program, const uint32_t& index)
	{
		m_materials[index]->bind(program);
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
}
#include "resource_manager.h"
#include "logger.h"
#include "ogl.h"
#include "material.h"
#include "mesh.h"
#include "scene.h"
#include "utility.h"
#include "shader_key.h"
#include <runtime/loader.h>
#include <gtc/matrix_transform.hpp>

namespace nimble
{
	std::unordered_map<std::string, std::weak_ptr<Texture>>  ResourceManager::m_texture_cache;
	std::unordered_map<std::string, std::weak_ptr<Material>> ResourceManager::m_material_cache;
	std::unordered_map<std::string, std::weak_ptr<Mesh>>	 ResourceManager::m_mesh_cache;
	std::unordered_map<std::string, std::weak_ptr<Scene>>	 ResourceManager::m_scene_cache;
	std::unordered_map<std::string, std::weak_ptr<Shader>>	 ResourceManager::m_shader_cache;
	std::unordered_map<std::string, uint32_t>				 ResourceManager::m_vertex_func_id_map;
	std::unordered_map<std::string, uint32_t>				 ResourceManager::m_fragment_func_id_map;

	// -----------------------------------------------------------------------------------------------------------------------------------

	static uint32_t g_vertex_func_id_counter = 0;
	static uint32_t g_fragment_func_id_counter = 0;

	static const GLenum kInternalFormatTable[][4] =
	{
		{ GL_R8, GL_RG8, GL_RGB8, GL_RGBA8 },
		{ GL_R16F, GL_RG16F, GL_RGB16F, GL_RGBA16F },
		{ GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F }
	};

	static const GLenum kCompressedTable[][2] =
	{
		{ GL_COMPRESSED_RGB_S3TC_DXT1_EXT, GL_COMPRESSED_SRGB_S3TC_DXT1_EXT }, // BC1
		{ GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT }, // BC1a
		{ GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT }, // BC2
		{ GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT }, // BC3
		{ GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT }, // BC3n
		{ GL_COMPRESSED_RED_RGTC1, 0 }, // BC4
		{ GL_COMPRESSED_RG_RGTC2, 0 }, // BC5
		{ GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB, 0 }, // BC6
		{ GL_COMPRESSED_RGBA_BPTC_UNORM_ARB, GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB } // BC7
	};

	static const GLenum kFormatTable[] =
	{
		GL_RED,
		GL_RG,
		GL_RGB,
		GL_RGBA
	};

	static const GLenum kTypeTable[] =
	{
		GL_UNSIGNED_BYTE,
		GL_HALF_FLOAT,
		GL_FLOAT
	};

	static const TextureType kTextureTypeTable[] = 
	{
		TEXTURE_TYPE_ALBEDO,
		TEXTURE_TYPE_EMISSIVE,
		TEXTURE_TYPE_DISPLACEMENT,
		TEXTURE_TYPE_NORMAL,
		TEXTURE_TYPE_METAL_SPEC,
		TEXTURE_TYPE_ROUGH_SMOOTH,
		TEXTURE_TYPE_METAL_SPEC,
		TEXTURE_TYPE_ROUGH_SMOOTH,
		TEXTURE_TYPE_CUSTOM
	};

	// -----------------------------------------------------------------------------------------------------------------------------------

	void ResourceManager::shutdown()
	{
		for (auto& itr : m_mesh_cache)
			itr.second.reset();

		for (auto& itr : m_material_cache)
			itr.second.reset();

		for (auto& itr : m_texture_cache)
			itr.second.reset();
	
		for (auto& itr : m_shader_cache)
			itr.second.reset();

		for (auto& itr : m_scene_cache)
			itr.second.reset();
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	std::shared_ptr<Texture> ResourceManager::load_texture(const std::string& path, const bool& absolute, const bool& srgb, const bool& cubemap)
	{
		if (m_texture_cache.find(path) != m_texture_cache.end() && m_texture_cache[path].lock())
			return m_texture_cache[path].lock();
		else
		{
			ast::Image image;

			if (ast::load_image(absolute ? path : utility::path_for_resource("assets/" + path), image))
			{
				uint32_t type = 0;

				if (image.type == ast::PIXEL_TYPE_FLOAT16)
					type = 1;
				else if (image.type == ast::PIXEL_TYPE_FLOAT32)
					type = 2;

				if (cubemap)
				{
					if (image.array_slices != 6)
					{
						NIMBLE_LOG_ERROR("Texture does not have 6 array slices: " + path);
						return nullptr;
					}

					if (image.compression == ast::COMPRESSION_NONE)
					{
						std::shared_ptr<TextureCube> texture = std::make_shared<TextureCube>(image.data[0][0].width,
							image.data[0][0].height,
							image.array_slices,
							image.mip_slices,
							kInternalFormatTable[type][image.components - 1],
							kFormatTable[image.components - 1],
							kTypeTable[type]);

						for (uint32_t i = 0; i < image.array_slices; i++)
						{
							for (uint32_t j = 0; j < image.mip_slices; j++)
								texture->set_data(i, 0, j, image.data[i][j].data);
						}

						m_texture_cache[path] = texture;

						return texture;
					}
					else
					{
						if (kCompressedTable[image.compression - 1][(int)srgb] == 0)
						{
							NIMBLE_LOG_ERROR("No SRGB format available for this compression type: " + path);
							return nullptr;
						}

						std::shared_ptr<TextureCube> texture = std::make_shared<TextureCube>(image.data[0][0].width,
							image.data[0][0].height,
							1,
							image.mip_slices,
							kCompressedTable[image.compression - 1][(int)srgb],
							kFormatTable[image.components - 1],
							kTypeTable[type],
							true);

						for (uint32_t i = 0; i < image.array_slices; i++)
						{
							for (uint32_t j = 0; j < image.mip_slices; j++)
								texture->set_compressed_data(i, 0, j, image.data[i][j].size, image.data[i][j].data);
						}

						m_texture_cache[path] = texture;

						return texture;
					}
				}
				else
				{
					if (image.compression == ast::COMPRESSION_NONE)
					{
						std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>(image.data[0][0].width,
							image.data[0][0].height,
							image.array_slices,
							image.mip_slices,
							1,
							kInternalFormatTable[type][image.components - 1],
							kFormatTable[image.components - 1],
							kTypeTable[type]);

						for (uint32_t i = 0; i < image.array_slices; i++)
						{
							for (uint32_t j = 0; j < image.mip_slices; j++)
								texture->set_data(i, j, image.data[i][j].data);
						}

						m_texture_cache[path] = texture;

						return texture;
					}
					else
					{
						std::shared_ptr<Texture2D> texture = std::make_shared<Texture2D>(image.data[0][0].width,
							image.data[0][0].height,
							image.array_slices,
							image.mip_slices,
							1,
							kCompressedTable[image.compression - 1][(int)srgb],
							kFormatTable[image.components - 1],
							kTypeTable[type],
							true);

						for (uint32_t i = 0; i < image.array_slices; i++)
						{
							for (uint32_t j = 0; j < image.mip_slices; j++)
								texture->set_compressed_data(i, j, image.data[i][j].size, image.data[i][j].data);
						}

						m_texture_cache[path] = texture;

						return texture;
					}
				}
			}
			else
			{
				NIMBLE_LOG_ERROR("Failed to load Texture: " + path);
				return nullptr;
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	std::shared_ptr<Material> ResourceManager::load_material(const std::string& path, const bool& absolute)
	{
		if (m_material_cache.find(path) != m_material_cache.end() && m_material_cache[path].lock())
			return m_material_cache[path].lock();
		else
		{
			ast::Material ast_material;

			if (ast::load_material(absolute ? path : utility::path_for_resource("assets/" + path), ast_material))
			{
				std::shared_ptr<Material> material = std::make_shared<Material>();

				material->set_name(ast_material.name);
				material->set_metallic_workflow(ast_material.metallic_workflow);
				material->set_double_sided(ast_material.double_sided);
				material->set_vertex_shader_func(ast_material.vertex_shader_func_src);
				material->set_fragment_shader_func(ast_material.fragment_shader_func_src);
				material->set_blend_mode((BlendMode)ast_material.blend_mode);
				material->set_displacement_type((DisplacementType)ast_material.displacement_type);
				material->set_shading_model((ShadingModel)ast_material.shading_model);
				material->set_lighting_model((LightingModel)ast_material.lighting_model);


				uint32_t custom_texture_count = 0;

				for (const auto& texture_desc : ast_material.textures)
				{
					if (texture_desc.type == ast::TEXTURE_CUSTOM)
						custom_texture_count++;
					else
						material->set_surface_texture(kTextureTypeTable[texture_desc.type], load_texture(texture_desc.path, true, texture_desc.srgb));
				}

				material->set_custom_texture_count(custom_texture_count);

				// Iterate a second time to add the custom textures
				for (uint32_t i = 0; i < ast_material.textures.size(); i++)
					material->set_custom_texture(i, load_texture(ast_material.textures[i].path, true, ast_material.textures[i].srgb));

				// Build shader and program keys
				VertexShaderKey vs_key;
				FragmentShaderKey fs_key;

				vs_key.set_normal_texture(material->surface_texture(TEXTURE_TYPE_NORMAL) ? 1 : 0);
				fs_key.set_normal_texture(material->surface_texture(TEXTURE_TYPE_NORMAL) ? 1 : 0);

				uint32_t vertex_func_id = 1023;

				if (ast_material.vertex_shader_func_id.size() > 0)
				{
					if (m_vertex_func_id_map.find(ast_material.vertex_shader_func_id) != m_vertex_func_id_map.end())
						vertex_func_id = m_vertex_func_id_map[ast_material.vertex_shader_func_id];
					else
					{
						uint32_t id = g_vertex_func_id_counter++;
						m_vertex_func_id_map[ast_material.vertex_shader_func_id] = id;

						vertex_func_id = id;
					}
				}

				vs_key.set_vertex_func_id(vertex_func_id);
				
				uint32_t fragment_func_id = 1023;

				if (ast_material.fragment_shader_func_id.size() > 0)
				{
					if (m_fragment_func_id_map.find(ast_material.fragment_shader_func_id) != m_fragment_func_id_map.end())
						fragment_func_id = m_fragment_func_id_map[ast_material.fragment_shader_func_id];
					else
					{
						uint32_t id = g_fragment_func_id_counter++;
						m_fragment_func_id_map[ast_material.fragment_shader_func_id] = id;

						fragment_func_id = id;
					}
				}

				fs_key.set_fragment_func_id(fragment_func_id);
				fs_key.set_displacement_type((uint32_t)ast_material.displacement_type);
				fs_key.set_alpha_cutout(ast_material.blend_mode == ast::BLEND_MODE_MASKED ? 1 : 0);
				fs_key.set_lighting_model((uint32_t)ast_material.lighting_model);
				fs_key.set_shading_model((uint32_t)ast_material.shading_model);
				fs_key.set_albedo_texture(material->surface_texture(TEXTURE_TYPE_ALBEDO) ? 1 : 0);
				fs_key.set_roughness_texture(material->surface_texture(TEXTURE_TYPE_ROUGH_SMOOTH) ? 1 : 0);
				fs_key.set_metallic_texture(material->surface_texture(TEXTURE_TYPE_METAL_SPEC) ? 1 : 0);
				fs_key.set_emissive_texture(material->surface_texture(TEXTURE_TYPE_EMISSIVE) ? 1 : 0);
				fs_key.set_metallic_workflow(ast_material.metallic_workflow);
				fs_key.set_custom_texture_count(custom_texture_count);

				ProgramKey program_key = ProgramKey(vs_key, fs_key);

				material->set_vs_key(vs_key);
				material->set_fs_key(fs_key);
				material->set_program_key(program_key);

				m_material_cache[path] = material;

				return material;
			}
			else
			{
				NIMBLE_LOG_ERROR("Failed to load Material: " + path);
				return nullptr;
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	std::shared_ptr<Mesh> ResourceManager::load_mesh(const std::string& path)
	{
		if (m_mesh_cache.find(path) != m_mesh_cache.end() && m_mesh_cache[path].lock())
			return m_mesh_cache[path].lock();
		else
		{
			ast::Mesh ast_mesh;

			if (ast::load_mesh(utility::path_for_resource("assets/" + path), ast_mesh))
			{
				std::shared_ptr<VertexArray> vao = nullptr;
				std::shared_ptr<VertexBuffer> vbo = nullptr;
				std::shared_ptr<IndexBuffer> ibo = nullptr;

				vbo = std::make_shared<VertexBuffer>(GL_STATIC_DRAW, sizeof(ast::Vertex) * ast_mesh.vertices.size(), (void*)&ast_mesh.vertices[0]);

				if (!vbo)
					NIMBLE_LOG_ERROR("Failed to create Vertex Buffer");

				// Create index buffer.
				ibo = std::make_shared<IndexBuffer>(GL_STATIC_DRAW, sizeof(uint32_t) * ast_mesh.indices.size(), (void*)ast_mesh.indices[0]);

				if (!ibo)
					NIMBLE_LOG_ERROR("Failed to create Index Buffer");

				// Declare vertex attributes.
				VertexAttrib attribs[] =
				{
					{ 3, GL_FLOAT, false, 0 },
					{ 2, GL_FLOAT, false, offsetof(ast::Vertex, tex_coord) },
					{ 3, GL_FLOAT, false, offsetof(ast::Vertex, normal) },
					{ 3, GL_FLOAT, false, offsetof(ast::Vertex, tangent) },
					{ 3, GL_FLOAT, false, offsetof(ast::Vertex, bitangent) }
				};

				// Create vertex array.
				vao = std::make_shared<VertexArray>(vbo.get(), ibo.get(), sizeof(ast::Vertex), 5, attribs);

				if (!vao)
					NIMBLE_LOG_ERROR("Failed to create Vertex Array");

				std::vector<std::shared_ptr<Material>> materials;
				materials.resize(ast_mesh.materials.size());

				for (uint32_t i = 0; i < ast_mesh.materials.size(); i++)
					materials[i] = load_material(ast_mesh.material_paths[i], true);

				std::vector<SubMesh> submeshes;
				submeshes.resize(ast_mesh.submeshes.size());

				for (uint32_t i = 0; i < ast_mesh.submeshes.size(); i++)
				{
					submeshes[i] = { ast_mesh.submeshes[i].index_count, 
									 ast_mesh.submeshes[i].base_vertex, 
									 ast_mesh.submeshes[i].base_index, 
									 ast_mesh.submeshes[i].max_extents, 
									 ast_mesh.submeshes[i].min_extents, 
									 materials[ast_mesh.submeshes[i].material_index] 
								   };
				}

				std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(ast_mesh.name, ast_mesh.max_extents, ast_mesh.min_extents, submeshes, vbo, ibo, vao);

				m_mesh_cache[path] = mesh;

				return mesh;
			}
			else
			{
				NIMBLE_LOG_ERROR("Failed to load Mesh: " + path);
				return nullptr;
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	std::shared_ptr<Scene> ResourceManager::load_scene(const std::string& path)
	{
		if (m_scene_cache.find(path) != m_scene_cache.end() && m_scene_cache[path].lock())
			return m_scene_cache[path].lock();
		else
		{
			ast::Scene ast_scene;

			if (ast::load_scene(utility::path_for_resource("assets/" + path), ast_scene))
			{
				std::shared_ptr<Scene> scene = std::make_shared<Scene>(ast_scene.name);

				// Create entities
				for (const auto& entity : ast_scene.entities)
				{
					Entity::ID id = scene->create_entity(entity.name);

					Entity& e = scene->lookup_entity(id);

					e.set_position(entity.position);
					e.set_rotation(entity.rotation);
					e.set_scale(entity.scale);
					
					e.m_mesh = load_mesh(entity.mesh);
					
					if (entity.material_override.size() > 0)
						e.m_override_mat = load_material(entity.material_override);

					glm::mat4 H = glm::rotate(glm::mat4(1.0f), glm::radians(e.m_rotation.x), glm::vec3(0.0f, 1.0f, 0.0f));
					glm::mat4 P = glm::rotate(glm::mat4(1.0f), glm::radians(e.m_rotation.y), glm::vec3(1.0f, 0.0f, 0.0f));
					glm::mat4 B = glm::rotate(glm::mat4(1.0f), glm::radians(e.m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

					glm::mat4 R = H * P * B;
					glm::mat4 S = glm::scale(glm::mat4(1.0f), e.m_scale);
					glm::mat4 T = glm::translate(glm::mat4(1.0f), e.m_position);

					e.m_transform = T * R * S;
					e.m_prev_transform = e.m_transform;

					e.m_obb.min = e.m_mesh->aabb().min;
					e.m_obb.max = e.m_mesh->aabb().max;
					e.m_obb.position = e.m_position;

#ifdef ENABLE_SUBMESH_CULLING
					e.m_submesh_visibility.resize(e.m_mesh->submesh_count());

					for (uint32_t i = 0; i < e.m_mesh->submesh_count(); i++)
					{
						SubMesh& submesh = e.m_mesh->submesh(i);

						Sphere sphere;

						glm::vec3 center = (submesh.min_extents + submesh.max_extents) / 2.0f;

						sphere.position = center + e.m_position;
						sphere.radius = glm::length(submesh.max_extents - submesh.min_extents) / 2.0f;

						e.m_submesh_spheres.push_back(sphere);
					}
#endif
				}

				// Load camera
				Camera* camera = scene->camera();
				camera->set_position(ast_scene.camera.position);

				// Load environment
				if (ast_scene.skybox.environment_map.size() > 0)
					scene->set_environment_map(std::static_pointer_cast<TextureCube>(load_texture(ast_scene.skybox.environment_map, false, true)));

				if (ast_scene.skybox.diffuse_irradiance.size() > 0)
					scene->set_irradiance_map(std::static_pointer_cast<TextureCube>(load_texture(ast_scene.skybox.diffuse_irradiance, false, true)));
				
				if (ast_scene.skybox.specular_irradiance.size() > 0)
					scene->set_prefiltered_map(std::static_pointer_cast<TextureCube>(load_texture(ast_scene.skybox.specular_irradiance, false, true)));

				// @TODO: Load reflection probes

				// @TODO: Load GI probes

				m_scene_cache[path] = scene;

				return scene;
			}
			else
			{
				NIMBLE_LOG_ERROR("Failed to load Scene: " + path);
				return nullptr;
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------

	std::shared_ptr<Shader> ResourceManager::load_shader(const std::string& path, const uint32_t& type, std::vector<std::string> defines)
	{
		std::string define_str = "";

		for (const auto& define : defines)
		{
			define_str = define_str + "|";
			define_str = define_str + define;
		}

		std::string id = path + define_str;
		
		if (m_shader_cache.find(id) != m_shader_cache.end() && m_shader_cache[id].lock())
			return m_shader_cache[id].lock();
		else
		{
			std::string source;

			if (!utility::read_shader(utility::path_for_resource("assets/" + path), source, defines))
			{
				NIMBLE_LOG_ERROR("Failed to read shader with name '" + path);
				return nullptr;
			}
			else
			{
				std::shared_ptr<Shader> shader = std::make_shared<Shader>((GLenum)type, source);

				if (!shader->compiled())
				{
					NIMBLE_LOG_ERROR("Shader with name '" + path + "' failed to compile:\n" + source);
					return nullptr;
				}

				m_shader_cache[id] = shader;
				return shader;
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------------------------------
} // namespace nimble
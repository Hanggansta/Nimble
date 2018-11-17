#pragma once

#include <string>
#include <stdint.h>
#include <glm.hpp>
#include "macros.h"
#include "murmur_hash.h"

namespace nimble
{
	class Texture;
	class Program;
	class UniformBuffer;

	#define MAX_MATERIAL_TEXTURES 8

	static const std::string kSurfaceTextureNames[] = 
	{
		"s_Albedo",
		"s_Normal",
		"s_Metallic",
		"s_Roughness",
		"s_Specular",
		"s_Smoothness",
		"s_Displacement"
	};

	static const std::string kCustomTextureNames[] =
	{
		"s_Texture1",
		"s_Texture2",
		"s_Texture3",
		"s_Texture4",
		"s_Texture5",
		"s_Texture6",
		"s_Texture7",
		"s_Texture8"
	};

	struct MaterialUniforms
	{
		glm::vec4 albedo;
		glm::vec4 emissive;
		glm::vec4 metal_rough;
	};

	enum ShadingModel
	{
		SHADING_MODEL_STANDARD = 0,
		SHADING_MODEL_CLOTH,
		SHADING_MODEL_SUBSURFACE
	};

	enum LightingModel
	{
		LIGHTING_MODEL_LIT = 0,
		LIGHTING_MODEL_UNLIT
	};

	enum DisplacementType
	{
		DISPLACEMENT_NONE = 0,
		DISPLACEMENT_PARALLAX_OCCLUSION,
		DISPLACEMENT_TESSELLATION
	};

	enum BlendMode
	{
		BLEND_MODE_OPAQUE = 0,
		BLEND_MODE_ADDITIVE,
		BLEND_MODE_MASKED,
		BLEND_MODE_TRANSLUCENT
	};

	enum TextureType
	{
		TEXTURE_TYPE_ALBEDO,
		TEXTURE_TYPE_NORMAL,
		TEXTURE_TYPE_METAL_SPEC,
		TEXTURE_TYPE_ROUGH_SMOOTH,
		TEXTURE_TYPE_DISPLACEMENT,
		TEXTURE_TYPE_EMISSIVE,
		TEXTURE_TYPE_CUSTOM
	};

	class Material
	{
		friend class ResourceManager;

	public:
		Material();
		~Material();

		// Bind textures and material constants to pipeline.
		void bind(Program* program);

		// Property setters.
		inline void set_name(const std::string& name) { m_name = name; m_hash = NIMBLE_HASH(m_name.c_str()); }
		inline void set_metallic_workflow(bool metallic) { m_metallic_workflow = metallic; }
		inline void set_double_sided(bool double_sided) { m_double_sided = double_sided; }
		inline void set_uniform_albedo(const glm::vec4& albedo) { m_uniforms.albedo = albedo; }
		inline void set_uniform_emissive(const glm::vec3& emissive) { m_uniforms.emissive = glm::vec4(emissive, 0.0f); }
		inline void set_uniform_metallic(const float& metallic) { m_uniforms.metal_rough.x = metallic; }
		inline void set_uniform_roughness(const float& roughness) { m_uniforms.metal_rough.y = roughness; }
		inline void set_blend_mode(const BlendMode& blend) { m_blend_mode = blend; }
		inline void set_displacement_type(const DisplacementType& displacement) { m_displacement_type = displacement; }
		inline void set_shading_model(const ShadingModel& shading) { m_shading_model = shading; }
		inline void set_lighting_model(const LightingModel& lighting) { m_lighting_model = lighting; }
		inline void set_vertex_shader_func(const std::string& src) { if (src.size() > 0) { m_vertex_shader_func = src; m_has_vertex_shader_func = true; } }
		inline void set_fragment_shader_func(const std::string& src) { if (src.size() > 0) { m_fragment_shader_func = src; m_has_fragment_shader_func = true; } }
		inline void set_custom_texture_count(const uint32_t& count) { m_custom_texture_count = count; }
		inline void set_custom_texture(const uint32_t& index, Texture* texture) { if (index < m_custom_texture_count){ m_custom_textures[index] = texture; }}
		inline void set_surface_texture(TextureType type, Texture* texture) { m_surface_textures[type] = texture; }

		// Property getters
		inline std::string name() { return m_name; }
		inline uint64_t hash() { return m_hash; }
		inline bool has_vertex_shader_func() { return m_has_vertex_shader_func; }
		inline bool has_fragment_shader_func() { return m_has_fragment_shader_func; }
		inline std::string vertex_shader_func() { return m_vertex_shader_func; }
		inline std::string fragment_shader_func() { return m_fragment_shader_func; }
		inline bool is_metallic_workflow() { return m_metallic_workflow; }
		inline bool is_double_sided() { return m_double_sided; }
		inline glm::vec4 uniform_albedo() { return m_uniforms.albedo; }
		inline glm::vec3 uniform_emissive() { return glm::vec3(m_uniforms.emissive); }
		inline float uniform_metallic() { return m_uniforms.metal_rough.x; }
		inline float uniform_roughness() { return m_uniforms.metal_rough.y; }
		inline MaterialUniforms uniforms() { return m_uniforms; }
		inline BlendMode blend_mode() { return m_blend_mode; }
		inline DisplacementType displacement_type() { return m_displacement_type; }
		inline ShadingModel shading_model() { return m_shading_model; }
		inline LightingModel lighting_model() { return m_lighting_model; }
		inline uint32_t custom_texture_count() { return m_custom_texture_count; }
		inline Texture* custom_texture(const uint32_t& index) { return m_custom_textures[index]; }
		inline Texture* surface_texture(const uint32_t& index) { return m_surface_textures[index]; }

	private:
		std::string		 m_name = "";
		std::string		 m_vertex_shader_func = "";
		std::string		 m_fragment_shader_func = "";
		uint64_t		 m_hash = 0;
		bool			 m_metallic_workflow = true;
		bool			 m_double_sided = false;
		bool			 m_has_vertex_shader_func = false;
		bool			 m_has_fragment_shader_func = false;
		bool			 m_has_uniforms = false;
		uint32_t		 m_custom_texture_count = 0;
		BlendMode		 m_blend_mode = BLEND_MODE_OPAQUE;
		DisplacementType m_displacement_type = DISPLACEMENT_NONE;
		ShadingModel	 m_shading_model = SHADING_MODEL_STANDARD;
		LightingModel	 m_lighting_model = LIGHTING_MODEL_LIT;
		Texture* 		 m_custom_textures[MAX_MATERIAL_TEXTURES];
		Texture*		 m_surface_textures[MAX_MATERIAL_TEXTURES];
		MaterialUniforms m_uniforms;
	};
}
// ------------------------------------------------------------------
// DEFINITIONS ------------------------------------------------------
// ------------------------------------------------------------------

#define MAX_SHADOW_MAP_CASCADES 8
#define MAX_RELFECTION_PROBES 128
#define MAX_GI_PROBES 128
#define MAX_POINT_LIGHTS 512
#define MAX_SPOT_LIGHTS 512
#define MAX_DIRECTIONAL_LIGHTS 512
#define MAX_SHADOW_CASTING_POINT_LIGHTS 8
#define MAX_SHADOW_CASTING_SPOT_LIGHTS 8
#define MAX_SHADOW_CASTING_DIRECTIONAL_LIGHTS 8
#define MAX_BONES 100

// ------------------------------------------------------------------
// STRUCTURES -------------------------------------------------------
// ------------------------------------------------------------------

struct ShadowFrustum
{
	mat4  shadow_matrix;
	float far_plane;
};

// ------------------------------------------------------------------

struct PointLightData
{
	vec4 position_range;
	vec4 color_intensity;
	int	 shadow_map_idx;
};

// ------------------------------------------------------------------

struct SpotLightData
{
	vec4 position_cone_angle;
	vec4 direction_range;
	vec4 color_intensity;
	int	 shadow_map_idx;
};

// ------------------------------------------------------------------

struct DirectionalLightData
{
	vec4 direction;
	vec4 color_intensity;
	int	 shadow_map_idx;
};

// ------------------------------------------------------------------

struct VertexProperties
{
	vec3 Position;
	vec4 NDCFragPos;
	vec4 ScreenPosition;
	vec4 LastScreenPosition;
	vec3 Normal;
	vec2 TexCoord;

#ifdef TEXTURE_NORMAL
	vec3 Tangent;
	vec3 Bitangent;
#endif

#ifdef DISPLACEMENT_TYPE_PARALLAX_OCCLUSION
	vec3 TangentViewPos;
	vec3 TangentFragPos;
#endif
};

// ------------------------------------------------------------------

struct FragmentProperties
{
	vec3 Position;
	vec2 TexCoords;
	vec3 Normal;
	float FragDepth;
#ifdef TEXTURE_NORMAL
	vec3 Tangent;
	vec3 Bitangent;
#endif
#ifdef DISPLACEMENT_TYPE_PARALLAX_OCCLUSION
	vec3 TangentViewPos;
	vec3 TangentFragPos;
#endif
};

// ------------------------------------------------------------------

struct MaterialProperties
{
	vec4 albedo;
	vec3 normal;
	float metallic;
	float roughness;
};

// ------------------------------------------------------------------

struct PBRProperties
{
	vec3 N;
	vec3 V;
	vec3 R;
	vec3 F0;
	vec3 F;
	vec3 kS;
	vec3 kD;
	float NdotV;
};

// ------------------------------------------------------------------
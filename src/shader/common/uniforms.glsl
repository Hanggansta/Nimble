#include <structures.glsl>

// ------------------------------------------------------------------
// COMMON UNIFORM BUFFERS -------------------------------------------
// ------------------------------------------------------------------

layout (std140) uniform u_PerFrame
{ 
	mat4 		  lastViewProj;
	mat4 		  viewProj;
	mat4 		  invViewProj;
	mat4 		  invProj;
	mat4 		  invView;
	mat4 		  projMat;
	mat4 		  viewMat;
	vec4 		  viewPos;
	vec4 		  viewDir;
	vec4		  current_prev_jitter;
	int			  numCascades;
	ShadowFrustum shadowFrustums[MAX_SHADOW_FRUSTUM];
	float		  tanHalfFov;
	float		  aspectRatio;
	float		  nearPlane;
	float		  farPlane;
	int			  renderer;
	int			  current_output;
	int			  motion_blur;
	int			  max_motion_blur_samples;
	float		  velocity_scale;
	int			  viewport_width;
	int			  viewport_height;
	int			  ssao;
	int			  ssao_num_samples;
	float		  ssao_radius;
	float		  ssao_bias;
};

// ------------------------------------------------------------------

layout (std140) uniform u_PerScene
{
	PointLight 		 pointLights[MAX_POINT_LIGHTS];
	DirectionalLight directionalLight;
	int				 pointLightCount;
};

// ------------------------------------------------------------------

layout (std140) uniform u_PerEntity
{
	mat4 mvpMat;
	mat4 lastMvpMat;
	mat4 modelMat;	
	vec4 worldPos;
};

// ------------------------------------------------------------------
#include <../../common/uniforms.glsl>

// ------------------------------------------------------------------
// OUTPUTS ----------------------------------------------------------
// ------------------------------------------------------------------

out vec3 FS_OUT_FragColor;

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

in vec2 FS_IN_TexCoord;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform sampler2D s_Depth;
uniform sampler2D s_Velocity;

uniform int u_NumSamples;
uniform vec4 u_MieG;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

float mie_scattering(float cos_angle)
{    
	return u_MieG.w * (u_MieG.x / (pow(u_MieG.y - u_MieG.z * cos_angle, 1.5)));			
}

// ------------------------------------------------------------------

float attenuation(vec3 frag_pos, int shadow_map_idx, int light_idx)
{
	int start_idx = shadow_map_idx * num_cascades; // Starting from this value
	int end_idx = start_idx + num_cascades; // Less that this value

	int index = start_idx;

	// TODO: Use spheres for shadow cascades for stability and ease of checking
    vec4 clip_pos = view_proj * vec4(frag_pos, 1.0);
	clip_pos /= clip_pos.w;
	float frag_depth = clip_pos.z;
    
	// Find shadow cascade.
	for (int i = start_idx; i < (end_idx - 1); i++)
	{
		if (frag_depth > cascade_far_plane[i])
			index = i + 1;
	}
}

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
	float depth = texture(s_Depth, FS_IN_TexCoord).r;
	vec3 frag_pos = world_position_from_depth(FS_IN_TexCoord, f.FragDepth);

	vec3 direction = view_pos - frag_pos;
	float march_distance = length(direction);
	direction = normalize(direction);
	float step_size = march_distance / u_NumSamples;

	vec3 current_pos = frag_pos;

	float cos_angle = dot(directional_light_direction.xyz, direction);
	vec3 v_light = vec3(0.0);

	for (int i = 0; i < march_distance; i++)
	{
		float atten = attenuation(current_pos, 0, 0);
		v_light += atten;

		current_pos = current_pos + direction * step_size;
	}

	// Apply scattering
	v_light *= mie_scattering(cos_angle);

	// Apply light color
	v_light *= directional_light_color_intensity[0].xyz;

	// Divide by the number of samples
	v_light /= u_NumSamples;

	FS_OUT_FragColor = v_light;
}

// ------------------------------------------------------------------
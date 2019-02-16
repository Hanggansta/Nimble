#include <../../../common/uniforms.glsl>

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform int u_LightIdx;

// ------------------------------------------------------------------
// INPUT ------------------------------------------------------------
// ------------------------------------------------------------------

in vec3 PS_IN_FragPos;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec3 light_pos = point_light_position_range[u_LightIdx].xyz;
    float far_plane = point_light_position_range[u_LightIdx].w;

    float light_distance = length(PS_IN_FragPos - light_pos);
    
    // map to [0;1] range by dividing by far_plane
    light_distance = light_distance / far_plane;
    
    // write this as modified depth
    gl_FragDepth = light_distance;
}

// ------------------------------------------------------------------
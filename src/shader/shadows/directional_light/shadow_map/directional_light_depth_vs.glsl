#include <../../../common/mesh_vertex_attribs.glsl>
#include <../../../common/uniforms.glsl>

// ------------------------------------------------------------------
// MAIN  ------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
	gl_Position = view_proj *  model_mat * vec4(VS_IN_Position, 1.0f);
}

// ------------------------------------------------------------------
#include <../../common/uniforms.glsl>

// ------------------------------------------------------------------
// OUTPUTS ----------------------------------------------------------
// ------------------------------------------------------------------

out vec3 PS_OUT_FragColor;

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

in vec2 PS_IN_TexCoord;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform sampler2D s_Color;
uniform sampler2D s_History;
uniform sampler2D s_Velocity;
uniform vec2 u_PixelSize;
uniform int u_FirstFrame;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
	vec3 c;
    
    if(u_FirstFrame == 1) 
	{
        // first frame, no blending at all.
        c.xyz = texture(s_Color, PS_IN_TexCoord).xyz;
    } 
	else 
	{  
        vec3 neighbourhood[9];
        
        neighbourhood[0] = texture(s_Color, PS_IN_TexCoord + vec2(-1, -1) * u_PixelSize ).xyz;
        neighbourhood[1] = texture(s_Color, PS_IN_TexCoord + vec2(+0, -1) * u_PixelSize ).xyz;
        neighbourhood[2] = texture(s_Color, PS_IN_TexCoord + vec2(+1, -1) * u_PixelSize ).xyz;
        neighbourhood[3] = texture(s_Color, PS_IN_TexCoord + vec2(-1, +0) * u_PixelSize ).xyz;
        neighbourhood[4] = texture(s_Color, PS_IN_TexCoord + vec2(+0, +0) * u_PixelSize ).xyz;
        neighbourhood[5] = texture(s_Color, PS_IN_TexCoord + vec2(+1, +0) * u_PixelSize ).xyz;
        neighbourhood[6] = texture(s_Color, PS_IN_TexCoord + vec2(-1, +1) * u_PixelSize ).xyz;
        neighbourhood[7] = texture(s_Color, PS_IN_TexCoord + vec2(+0, +1) * u_PixelSize ).xyz;
        neighbourhood[8] = texture(s_Color, PS_IN_TexCoord + vec2(+1, +1) * u_PixelSize ).xyz;
        
        vec3 nmin = neighbourhood[0];
        vec3 nmax = neighbourhood[0]; 

        for(int i = 1; i < 9; ++i) 
		{
            nmin = min(nmin, neighbourhood[i]);
            nmax = max(nmax, neighbourhood[i]);
        }
           
        vec2 velocity = vec2(0.0);

		if (renderer == 0)
			velocity = texture(s_Velocity, PS_IN_TexCoord).rg;
		else if (renderer == 1)
		 	velocity = texture(s_Velocity, PS_IN_TexCoord).ba;

        // Remap to [-1, 1] range
		velocity = velocity * 2.0 - 1.0;

        vec2 histUv = PS_IN_TexCoord - velocity.xy;
        
        // sample from history buffer, with neighbourhood clamping.  
        vec3 histSample = clamp(texture(s_History, histUv).xyz, nmin, nmax);
        
        // blend factor
        float blend = 0.05;
        
        bvec2 a = greaterThan(histUv, vec2(1.0, 1.0));
        bvec2 b = lessThan(histUv, vec2(0.0, 0.0));
        // if history sample is outside screen, switch to aliased image as a fallback.
        blend = any(bvec2(any(a), any(b))) ? 1.0 : blend;
        
        vec3 curSample = neighbourhood[4];
        // finally, blend current and clamped history sample.
        c = mix(histSample, curSample, vec3(blend));
    }  

	PS_OUT_FragColor = c;
}

// ------------------------------------------------------------------
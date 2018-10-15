#pragma once

// Forward Renderer
#define FRAMEBUFFER_FORWARD "Forward_FBO"
#define RENDER_TARGET_FORWARD_COLOR "Forward_Color"
#define RENDER_TARGET_FORWARD_VELOCITY "Forward_Velocity"
#define RENDER_TARGET_FORWARD_DEPTH "Forward_Depth"
#define FRAMEBUFFER_FORWARD_DEPTH_PREPASS "Forward_Depth_Prepass_FBO"
#define RENDER_TARGET_FORWARD_DEPTH_PREPASS "Forward_Depth_Prepass"

// PBR
#define BRDF_LUT "BRDF_LUT"

// CSM
#define CSM_SHADOW_MAPS "CSMShadowMaps"

// Hi-Z
#define RENDER_TARGET_HiZ "HIZ_RT"

// Deferred Renderer
#define FRAMEBUFFER_DEFERRED "Deferred_FBO"
#define RENDER_TARGET_DEFERRED_COLOR "Deferred_Color"
#define GBUFFER_FBO "GBuffer_FBO"
#define RENDER_TARGET_GBUFFER_RT0 "GBuffer_RT0"
#define RENDER_TARGET_GBUFFER_RT1 "GBuffer_RT1"
#define RENDER_TARGET_GBUFFER_RT2 "GBuffer_RT2"
#define RENDER_TARGET_GBUFFER_RT3 "GBuffer_RT3"
#define RENDER_TARGET_GBUFFER_DEPTH "GBuffer_DEPTH"

// Motion Blur
#define FRAMEBUFFER_MOTION_BLUR "Motion_Blur_FBO"
#define RENDER_TARGET_MOTION_BLUR "Motion_Blur_RT"

// SSAO
#define FRAMEBUFFER_SSAO "SSAO_FBO"
#define FRAMEBUFFER_SSAO_BLUR "SSAO_BLUR_FBO"
#define RENDER_TARGET_SSAO "SSAO_RT"
#define RENDER_TARGET_SSAO_BLUR "SSAO_BLUR_RT"

// Tone Mapping
#define RENDER_TARGET_TONE_MAPPING "TONE_MAPPING_RT"
#define FRAMEBUFFER_TONE_MAPPING "TONE_MAPPING_FBO"

// Bloom
#define RENDER_TARGET_BLOOM2_1 "BLOOM2_RT_1"
#define RENDER_TARGET_BLOOM2_2 "BLOOM2_RT_2"
#define RENDER_TARGET_BLOOM4_1 "BLOOM4_RT_1"
#define RENDER_TARGET_BLOOM4_2 "BLOOM4_RT_2"
#define RENDER_TARGET_BLOOM8_1 "BLOOM8_RT_1"
#define RENDER_TARGET_BLOOM8_2 "BLOOM8_RT_2"
#define RENDER_TARGET_BLOOM16_1 "BLOOM16_RT_1"
#define RENDER_TARGET_BLOOM16_2 "BLOOM16_RT_2"
#define FRAMEBUFFER_BLOOM2_1 "BLOOM2_FBO_1"
#define FRAMEBUFFER_BLOOM2_2 "BLOOM2_FBO_2"
#define FRAMEBUFFER_BLOOM4_1 "BLOOM4_FBO_1"
#define FRAMEBUFFER_BLOOM4_2 "BLOOM4_FBO_2"
#define FRAMEBUFFER_BLOOM8_1 "BLOOM8_FBO_1"
#define FRAMEBUFFER_BLOOM8_2 "BLOOM8_FBO_2"
#define FRAMEBUFFER_BLOOM16_1 "BLOOM16_FBO_1"
#define FRAMEBUFFER_BLOOM16_2 "BLOOM16_FBO_2"
#define RENDER_TARGET_BRIGHT_PASS "BRIGHT_PASS_RT"
#define FRAMEBUFFER_BRIGHT_PASS "BRIGHT_PASS_FBO"
#define RENDER_TARGET_BLOOM_COMPOSITE "BLOOM_COMPOSITE_RT"
#define FRAMEBUFFER_BLOOM_COMPOSITE "BLOOM_COMPOSITE_FBO"

// TAA
#define RENDER_TARGET_TAA "TAA_RT"
#define FRAMEBUFFER_TAA "TAA_FBO"
#define RENDER_TARGET_TAA_HIST "TAA_HIST_RT"
#define FRAMEBUFFER_TAA_HIST "TAA_HIST_FBO"

// Depth Of Field
#define RENDER_TARGET_DOF_COC "DOF_COC_RT"
#define FRAMEBUFFER_DOF_COC "DOF_COC_FBO"

#define RENDER_TARGET_DOF_COLOR4 "DOF_COLOR4_RT"
#define RENDER_TARGET_DOF_COC4 "DOF_COC4_RT"
#define RENDER_TARGET_DOF_MUL_COC_FAR4 "DOF_MUL_COC_FAR4_RT"
#define FRAMEBUFFER_DOF_DOWNSAMPLE "DOF_DOWNSAMPLE_FBO"

#define RENDER_TARGET_NEAR_COC_MAX_X4 "DOF_NEAR_COC_MAX_X4_RT"
#define FRAMEBUFFER_DOF_NEAR_COC_MAX_X4 "DOF_NEAR_COC_MAX_X4_FBO"

#define RENDER_TARGET_NEAR_COC_MAX4 "DOF_NEAR_COC_MAX4_RT"
#define FRAMEBUFFER_DOF_NEAR_COC_MAX4 "DOF_NEAR_COC_MAX4_FBO"

#define RENDER_TARGET_NEAR_COC_BLUR_X4 "DOF_NEAR_COC_BLUR_X4_RT"
#define FRAMEBUFFER_NEAR_COC_BLUR_X4 "DOF_NEAR_COC_BLUR_X4_FBO"

#define RENDER_TARGET_NEAR_COC_BLUR4 "DOF_NEAR_COC_BLUR4_RT"
#define FRAMEBUFFER_NEAR_COC_BLUR4 "DOF_NEAR_COC_BLUR4_FBO"

#define RENDER_TARGET_NEAR_DOF4 "DOF_NEAR_DOF4_RT"
#define RENDER_TARGET_FAR_DOF4 "DOF_FAR_DOF4_RT"
#define FRAMEBUFFER_DOF4 "DOF4_FBO"

#define RENDER_TARGET_NEAR_FILL_DOF4 "DOF_NEAR_FILL_DOF4_RT"
#define RENDER_TARGET_FAR_FILL_DOF4 "DOF_FAR_FILL_DOF4_RT"
#define FRAMEBUFFER_DOF_FILL4 "DOF_FILL4_FBO"

#define RENDER_TARGET_DOF_COMPOSITE "DOF_COMPOSITE_RT"
#define FRAMEBUFFER_DOF_COMPOSITE "DOF_COMPOSITE_FBO"

// SSR
#define RENDER_TARGET_SSR "SSR_RT"
#define FRAMEBUFFER_SSR "SSR_FBO"

// Composition pass
#define SHOW_FORWARD_COLOR 0
#define SHOW_FORWARD_DEPTH 1
#define SHOW_DEFERRED_COLOR 2
#define SHOW_GBUFFER_ALBEDO 3
#define SHOW_GBUFFER_NORMALS 4
#define SHOW_GBUFFER_ROUGHNESS 5
#define SHOW_GBUFFER_METALNESS 6
#define SHOW_GBUFFER_VELOCITY 7
#define SHOW_GBUFFER_EMISSIVE 8
#define SHOW_GBUFFER_DISPLACEMENT 9
#define SHOW_GBUFFER_DEPTH 10
#define SHOW_SHADOW_MAPS 11
#define SHOW_SSAO 20
#define SHOW_SSAO_BLUR 21
#define SHOW_BRIGHT_PASS 22
#define SHOW_SSR 23
#pragma once

#include <glm.hpp>
#include <stdint.h>
#include <memory>
#include "uniforms.h"

namespace nimble
{
struct RenderTargetView;
class RenderGraph;
class Scene;

enum ViewType
{
    VIEW_STANDARD,
    VIEW_DIRECTIONAL_LIGHT,
    VIEW_SPOT_LIGHT,
    VIEW_POINT_LIGHT
};

struct View
{
    bool                         enabled;
    bool                         culling;
    uint32_t                     cull_idx;
    uint32_t                     uniform_idx;
    glm::vec3                    direction;
    glm::vec3                    up;
    glm::vec3                    right;
    glm::vec3                    position;
    glm::mat4                    view_mat;
    glm::mat4                    projection_mat;
    glm::mat4                    vp_mat;
    glm::mat4                    prev_vp_mat;
    glm::mat4                    inv_view_mat;
    glm::mat4                    inv_projection_mat;
    glm::mat4                    inv_vp_mat;
    glm::vec4                    jitter;
    float                        fov;
    float                        ratio;
    float                        far_plane;
    float                        near_plane;
    Scene*                       scene;
    std::shared_ptr<RenderGraph> graph;
    RenderTargetView*            dest_render_target_view;
    ViewType                     type;

    // Directional Light related payload
    uint32_t      num_cascade_views;
    ShadowFrustum shadow_frustums[MAX_SHADOW_MAP_CASCADES * MAX_SHADOW_CASTING_DIRECTIONAL_LIGHTS];
    View*         cascade_views[MAX_SHADOW_MAP_CASCADES * MAX_SHADOW_CASTING_DIRECTIONAL_LIGHTS];

    // Optional payload
    uint32_t light_index;

    View()
    {
        scene                   = nullptr;
        dest_render_target_view = nullptr;

        for (uint32_t i = 0; i < MAX_SHADOW_MAP_CASCADES; i++)
            cascade_views[i] = nullptr;
    }
};
} // namespace nimble

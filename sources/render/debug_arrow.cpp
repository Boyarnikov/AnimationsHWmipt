#include <vector>
#include <render/material.h>
#include <render/mesh.h>
#include <render/shader.h>
#include <render/direction_light.h>
#include "debug_arrow.h"

static void add_triangle(vec3 a, vec3 b, vec3 c, std::vector<uint>& indices, std::vector<vec3>& vert, std::vector<vec3>& normal)
{
    uint k = vert.size();
    vec3 n = normalize(cross(b - a, c - a));
    indices.push_back(k);
    indices.push_back(k + 2);
    indices.push_back(k + 1);
    vert.push_back(a);
    vert.push_back(b);
    vert.push_back(c);
    normal.push_back(n);
    normal.push_back(n);
    normal.push_back(n);
}

struct DebugArrow
{
    MaterialPtr arrowMaterial;
    MeshPtr arrowMesh;

    std::vector<mat4> instancesTm;
    std::vector<vec4> instancesColor;

    DebugArrow()
    {
        arrowMaterial = make_material("arrow", ROOT_PATH"sources/shaders/arrow_vs.glsl", ROOT_PATH"sources/shaders/arrow_ps.glsl");
        std::vector<uint> indices;
        std::vector<vec3> vert;
        std::vector<vec3> normal;
        vec3 c = vec3(0, 1, 0);
        const int N = 4;
        vec3 p[N];
        for (int i = 0; i < N; i++)
        {
            float a1 = ((float)(i) / N) * 2 * PI;
            float a2 = ((float)(i + 1) / N) * 2 * PI;
            vec3 p1 = p[i] = vec3(cos(a1), 0, sin(a1));
            vec3 p2 = vec3(cos(a2), 0, sin(a2));
            add_triangle(p2, p1, c, indices, vert, normal);
        }

        add_triangle(p[0], p[1], p[2], indices, vert, normal);
        add_triangle(p[0], p[2], p[3], indices, vert, normal);
        arrowMesh = make_mesh(indices, vert, normal);
    }
};

static std::unique_ptr<DebugArrow> arrowRender;

void create_arrow_render()
{
    arrowRender = std::make_unique<DebugArrow>();
}

static mat4 directionMatrix(vec3 from, vec3 to)
{
    from = normalize(from);
    to = normalize(to);
    quat q(from, to);
    return toMat4(q);
}

static void add_arrow(DebugArrow& renderer, const vec3& from, const vec3& to, vec3 color, float size)
{
    vec3 d = to - from;
    mat4 t = translate(mat4(1.f), from);
    float len = length(d);
    if (len < 0.01f)
        return;
    mat4 s = scale(mat4(1.f), vec3(size, len, size));

    mat4 r = directionMatrix(vec3(0, 1, 0), d);
    renderer.instancesTm.emplace_back(t * r * s);
    renderer.instancesColor.emplace_back(vec4(color, 1.f));
}

void draw_arrow(const mat4& transform, const vec3& from, const vec3& to, vec3 color, float size)
{
    if (arrowRender)
    {
        add_arrow(*arrowRender, transform * vec4(from, 1), transform * vec4(to, 1), color, size);
    }   
}
void draw_arrow(const vec3& from, const vec3& to, vec3 color, float size)
{
    if (arrowRender)
    {
        add_arrow(*arrowRender, from, to, color, size);
    }   
}

void render_arrows(const mat4& cameraProjView, vec3 cameraPosition, const DirectionLight& light)
{
    if (!arrowRender)
        return;

    DebugArrow& renderer = *arrowRender;
    if (renderer.instancesTm.empty())
        return;
    const auto& shader = renderer.arrowMaterial->get_shader();

    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);

    shader.use();

    shader.set_mat4x4("ViewProjection", cameraProjView);
    shader.set_vec3("CameraPosition", cameraPosition);
    shader.set_vec3("LightDirection", glm::normalize(light.lightDirection));
    shader.set_vec3("AmbientLight", light.ambient);
    shader.set_vec3("SunLight", light.lightColor);

    const int N = 128;
    int instancesCount = renderer.instancesTm.size();
    for (int i = 0; i < instancesCount; i += N)
    {
        int count = min(N, instancesCount - i);
        shader.set_mat4x4("ArrowTm", std::span(renderer.instancesTm.data() + i, count));
        shader.set_vec4("ArrowColor", std::span(renderer.instancesColor.data() + i, count));
        render(renderer.arrowMesh, count);
    }

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    renderer.instancesTm.clear();
    renderer.instancesColor.clear();
}
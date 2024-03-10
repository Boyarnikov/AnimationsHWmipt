
#include <render/direction_light.h>
#include <render/material.h>
#include <render/mesh.h>
#include <render/debug_arrow.h>
#include "camera.h"
#include <application.h>

struct UserCamera
{
  glm::mat4 transform;
  mat4x4 projection;
  ArcballCamera arcballCamera;
};

struct Character
{
  glm::mat4 transform;
  MeshPtr mesh;
  MaterialPtr material;
};

struct Scene
{
  DirectionLight light;

  UserCamera userCamera;

  std::vector<Character> characters;

};

static std::unique_ptr<Scene> scene;

void game_init()
{
  scene = std::make_unique<Scene>();
  scene->light.lightDirection = glm::normalize(glm::vec3(-1, -1, 0));
  scene->light.lightColor = glm::vec3(1.f);
  scene->light.ambient = glm::vec3(0.2f);

  scene->userCamera.projection = glm::perspective(90.f * DegToRad, get_aspect_ratio(), 0.01f, 500.f);

  ArcballCamera &cam = scene->userCamera.arcballCamera;
  cam.curZoom = cam.targetZoom = 0.5f;
  cam.maxdistance = 5.f;
  cam.distance = cam.curZoom * cam.maxdistance;
  cam.lerpStrength = 10.f;
  cam.mouseSensitivity = 0.5f;
  cam.wheelSensitivity = 0.05f;
  cam.targetPosition = glm::vec3(0.f, 1.f, 0.f);
  cam.targetRotation = cam.curRotation = glm::vec2(DegToRad * -90.f, DegToRad * -30.f);
  cam.rotationEnable = false;

  scene->userCamera.transform = calculate_transform(scene->userCamera.arcballCamera);

  input.onMouseButtonEvent += [](const SDL_MouseButtonEvent &e) { arccam_mouse_click_handler(e, scene->userCamera.arcballCamera); };
  input.onMouseMotionEvent += [](const SDL_MouseMotionEvent &e) { arccam_mouse_move_handler(e, scene->userCamera.arcballCamera); };
  input.onMouseWheelEvent += [](const SDL_MouseWheelEvent &e) { arccam_mouse_wheel_handler(e, scene->userCamera.arcballCamera); };


  auto material = make_material("character", ROOT_PATH"sources/shaders/character_vs.glsl", ROOT_PATH"sources/shaders/character_ps.glsl");
  std::fflush(stdout);
  material->set_property("mainTex", create_texture2d(ROOT_PATH"resources/MotusMan_v55/MCG_diff.jpg"));

  scene->characters.emplace_back(Character{
    glm::identity<glm::mat4>(),
    load_mesh(ROOT_PATH"resources/MotusMan_v55/MotusMan_v55.fbx", 0),
    std::move(material)
  });

  create_arrow_render();

  std::fflush(stdout);
}


void game_update()
{
  arcball_camera_update(
    scene->userCamera.arcballCamera,
    scene->userCamera.transform,
    get_delta_time());
}

void render_character(const Character &character, const mat4 &cameraProjView, vec3 cameraPosition, const DirectionLight &light)
{
  const Material &material = *character.material;
  const Shader &shader = material.get_shader();

  shader.use();
  material.bind_uniforms_to_shader();
  shader.set_mat4x4("Transform", character.transform);
  shader.set_mat4x4("ViewProjection", cameraProjView);
  shader.set_vec3("CameraPosition", cameraPosition);
  shader.set_vec3("LightDirection", glm::normalize(light.lightDirection));
  shader.set_vec3("AmbientLight", light.ambient);
  shader.set_vec3("SunLight", light.lightColor);

  render(character.mesh);

  for (const auto& bone : character.mesh->bones)
  {
      const auto& parent = character.mesh->bones[bone.parentIndex];
      float distance = glm::length(parent.bindPose[3] - bone.bindPose[3]);
      draw_arrow(bone.bindPose, vec3(0), vec3(0.01f, 0, 0), vec3(1, 0, 0), 0.01f);
      draw_arrow(bone.bindPose, vec3(0), vec3(0, 0.01f, 0), vec3(0, 1, 0), 0.01f);
      draw_arrow(bone.bindPose, vec3(0), vec3(0, 0, 0.01f), vec3(0, 0, 1), 0.01f);
      draw_arrow(parent.bindPose[3], bone.bindPose[3], vec3(1, 1, 1), 0.01f);
  }
}

void game_render()
{
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  const float grayColor = 0.3f;
  glClearColor(grayColor, grayColor, grayColor, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


  const mat4 &projection = scene->userCamera.projection;
  const glm::mat4 &transform = scene->userCamera.transform;
  mat4 projView = projection * inverse(transform);

  for (const Character& character : scene->characters)
  {
      render_character(character, projView, glm::vec3(transform[3]), scene->light);
  }

  render_arrows(projView, glm::vec3(transform[3]), scene->light);
}
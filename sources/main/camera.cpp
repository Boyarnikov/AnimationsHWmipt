#include "camera.h"
#include <input.h>
#include <application.h>


glm::mat4 calculate_transform(const ArcballCamera &camera)
{
  float y = camera.curRotation.y;
  float x = camera.curRotation.x ;
  vec3 direction = vec3(cos(x) * cos(y), sin(y), sin(x) * cos(y));

  glm::vec3 position = camera.targetPosition - camera.distance * direction;
  glm::vec3 rotation = glm::vec3(PI * 0.5f - x, -y, 0);
  glm::vec3 scale = glm::vec3(1.f);

  return glm::translate(glm::mat4(1.f), position) * glm::scale(glm::yawPitchRoll(rotation.x, rotation.y, rotation.z), scale);
}


void arccam_mouse_move_handler(
  const SDL_MouseMotionEvent &e,
  ArcballCamera &arcballCamera)
{
  if (arcballCamera.rotationEnable)
  {
    float const pixToRad = DegToRad * arcballCamera.mouseSensitivity;
    arcballCamera.targetRotation += vec2(-e.xrel, -e.yrel) * pixToRad;
  }
}

void arccam_mouse_click_handler(
  const SDL_MouseButtonEvent &e,
  ArcballCamera &arcballCamera)
{
  if (e.button == SDL_BUTTON_RIGHT)
  {
    arcballCamera.rotationEnable = e.type == SDL_MOUSEBUTTONDOWN;
  }
}

void arccam_mouse_wheel_handler(
  const SDL_MouseWheelEvent &e,
  ArcballCamera &arcballCamera)
{
  arcballCamera.targetZoom = glm::clamp(arcballCamera.targetZoom - float(e.y) * arcballCamera.wheelSensitivity, 0.f, 1.f);
}


void arcball_camera_update(
  ArcballCamera &arcballCamera,
  glm::mat4 &transform,
  float dt)
{
  float lerpFactor = arcballCamera.lerpStrength * dt;

  arcballCamera.curZoom = lerp(arcballCamera.curZoom, arcballCamera.targetZoom, lerpFactor);
  arcballCamera.curRotation = lerp(arcballCamera.curRotation, arcballCamera.targetRotation, lerpFactor);
  arcballCamera.distance = arcballCamera.maxdistance * arcballCamera.curZoom;
  transform = calculate_transform(arcballCamera);
}

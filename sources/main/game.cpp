
#include <render/direction_light.h>
#include <render/material.h>
#include <render/mesh.h>
#include <render/scene.h>
#include <render/debug_arrow.h>
#include "camera.h"
#include <application.h>
#include <imgui/imgui.h>
#include "ImGuizmo.h"

#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/blending_job.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"

struct UserCamera
{
  glm::mat4 transform;
  mat4x4 projection;
  ArcballCamera arcballCamera;
};

struct AnimationLayer
{
    // Constructor, default initialization.
    AnimationLayer(const SkeletonPtr& skeleton, AnimationPtr animation) : weight(1.f), animation(animation)
    {
        locals.resize(skeleton->num_soa_joints());

        // Allocates a context that matches animation requirements.
        context = std::make_shared<ozz::animation::SamplingJob::Context>(skeleton->num_joints());
    }

    // Playback animation parameters
    float ratio = 0;
    bool pause = false;
    bool loop = true;
    float animPLaybackSpeed = 1.0f;

    // Blending weight for the layer.
    float weight;

    // Runtime animation.
    AnimationPtr animation;

    // Sampling context.
    std::shared_ptr<ozz::animation::SamplingJob::Context> context;

    // Buffer of local transforms as sampled from animation_.
    std::vector<ozz::math::SoaTransform> locals;
};


struct Character
{
  glm::mat4 transform;
  MeshPtr mesh;
  MaterialPtr material;
  // Runtime skeleton.
  SkeletonPtr skeleton_;

  // Sampling context.
  std::shared_ptr<ozz::animation::SamplingJob::Context> context_;

  // Buffer of local transforms as sampled from animation_.
  std::vector<ozz::math::SoaTransform> locals_;

  // Buffer of model space matrices.
  std::vector<ozz::math::Float4x4> models_;

  std::vector<AnimationLayer> layers;

  AnimationPtr currentAnimation;
  float ratio = 0;
  bool pause = false;
  bool loop = true;
  float animPLaybackSpeed = 1.0f;
};

struct Scene
{
  DirectionLight light;

  UserCamera userCamera;

  std::vector<Character> characters;

};

static std::unique_ptr<Scene> scene;

static std::vector<std::string> animationList;

#include <filesystem>
static std::vector<std::string> scan_animations(const char* path)
{
    std::vector<std::string> animations;
    for (auto& p : std::filesystem::recursive_directory_iterator(path))
    {
        auto filePath = p.path();
        if (p.is_regular_file() && filePath.extension() == ".fbx")
            animations.push_back(filePath.string());
    }
    return animations;
}

void game_init()
{
  animationList = scan_animations(ROOT_PATH"resources/Animations");
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

  input.onMouseButtonEvent += [](const SDL_MouseButtonEvent& e) { arccam_mouse_click_handler(e, scene->userCamera.arcballCamera); };
      
  input.onMouseMotionEvent += [](const SDL_MouseMotionEvent &e) { arccam_mouse_move_handler(e, scene->userCamera.arcballCamera); };
  input.onMouseWheelEvent += [](const SDL_MouseWheelEvent &e) { arccam_mouse_wheel_handler(e, scene->userCamera.arcballCamera); };


  auto material = make_material("character", ROOT_PATH"sources/shaders/character_vs.glsl", ROOT_PATH"sources/shaders/character_ps.glsl");
  std::fflush(stdout);
  material->set_property("mainTex", create_texture2d(ROOT_PATH"resources/MotusMan_v55/MCG_diff.jpg"));
  SceneAsset sceneAsset = load_scene(ROOT_PATH"resources/MotusMan_v55/MotusMan_v55.fbx",
      SceneAsset::LoadScene::Meshes | SceneAsset::LoadScene::Skeleton);

  Character& character = scene->characters.emplace_back(Character{
      glm::identity<glm::mat4>(),
      sceneAsset.meshes[0],
      std::move(material),
      sceneAsset.skeleton });

  // Skeleton and animation needs to match.
  // assert (character.skeleton_->num_joints() != character.animation_.num_tracks());

  // Allocates runtime buffers.
  const int num_soa_joints = character.skeleton_->num_soa_joints();
  character.locals_.resize(num_soa_joints);
  const int num_joints = character.skeleton_->num_joints();
  character.models_.resize(num_joints);

  // Allocates a context that matches animation requirements.
  character.context_ = std::make_shared<ozz::animation::SamplingJob::Context>(num_joints);

  create_arrow_render();

  std::fflush(stdout);
}


void game_update()
{
  arcball_camera_update(
    scene->userCamera.arcballCamera,
    scene->userCamera.transform,
    get_delta_time());
  for (Character& character : scene->characters)
  {
      if (!character.layers.empty())
      {
          for (AnimationLayer& layer : character.layers)
          {
              if (!layer.pause)
              {
                  if (layer.loop)
                  {
                      if (layer.animPLaybackSpeed < 0)
                      {
                          if (layer.ratio < 0)
                              layer.ratio = 1.0f;
                      }
                      else
                      {
                          if (layer.ratio >= 1.0f)
                              layer.ratio = 0.0f;
                      }
                  }
                  layer.ratio += get_delta_time() * layer.animPLaybackSpeed / layer.animation->duration();
              }

              // Samples optimized animation at t = animation_time_.
              ozz::animation::SamplingJob sampling_job;
              sampling_job.animation = layer.animation.get();
              sampling_job.context = layer.context.get();
              sampling_job.ratio = layer.ratio;
              sampling_job.output = ozz::make_span(layer.locals);
              if (!sampling_job.Run())
              {
                  debug_error("sampling_job failed");
              }
          }

          // Prepares blending layers.
          int numLayer = character.layers.size();
          std::vector<ozz::animation::BlendingJob::Layer> layers(numLayer);

          for (int i = 0; i < numLayer; ++i)
          {
              layers[i].transform = ozz::make_span(character.layers[i].locals);
              layers[i].weight = character.layers[i].weight;
          }

          // Setups blending job.
          ozz::animation::BlendingJob blend_job;
          blend_job.threshold = 0.1;
          blend_job.layers = ozz::make_span(layers);
          blend_job.rest_pose = character.skeleton_->joint_rest_poses();
          blend_job.output = ozz::make_span(character.locals_);

          // Blends.
          if (!blend_job.Run())
          {
              debug_error("blend_job failed");
              continue;
          }
      }
      else if (character.currentAnimation)
      {
          if (!character.pause)
          {
              if (character.loop)
              {
                  if (character.animPLaybackSpeed < 0)
                  {
                      if (character.ratio < 0)
                          character.ratio = 1.0f;
                  }
                  else
                  {
                      if (character.ratio >= 1.0f)
                          character.ratio = 0.0f;
                  }
              }
              character.ratio += get_delta_time() * character.animPLaybackSpeed / character.currentAnimation->duration();
          }
             
          // Samples optimized animation at t = animation_time_.
          ozz::animation::SamplingJob sampling_job;
          sampling_job.animation = character.currentAnimation.get();
          sampling_job.context = character.context_.get();
          sampling_job.ratio = character.ratio;
          sampling_job.output = ozz::make_span(character.locals_);
          if (!sampling_job.Run())
          {
              continue;
          }
      }
      else
      {
          auto restPose = character.skeleton_->joint_rest_poses();
          std::copy(restPose.begin(), restPose.end(), character.locals_.begin());
      }
      ozz::animation::LocalToModelJob ltm_job;
      ltm_job.skeleton = character.skeleton_.get();
      ltm_job.input = ozz::make_span(character.locals_);
      ltm_job.output = ozz::make_span(character.models_);
      if (!ltm_job.Run())
      {
          continue;
      }
  }
}

static glm::mat4 to_glm(const ozz::math::Float4x4& tm)
{
    glm::mat4 result;
    memcpy(glm::value_ptr(result), &tm.cols[0], sizeof(glm::mat4));
    return result;
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

  size_t boneNumber = character.mesh->bones.size();
  std::vector<mat4> bones(boneNumber);

  //const RuntimeSkeleton& skeleton = character.skeleton;
  //size_t nodeCount = skeleton.ref->nodeCount;

  //for (size_t i = 0; i < nodeCount; i++)
  //{
  //    auto it = character.mesh->boneNamesMap.find(skeleton.ref->names[i]);
  //    if (it != character.mesh->boneNamesMap.end())
  //    {
  //        int boneIdx = it->second;
  //        bones[boneIdx] = skeleton.globalTm[i] * character.mesh->bones[boneIdx].invBindPose;
  //    }
  //}

  const auto& skeleton = *character.skeleton_;
  size_t nodeCount = skeleton.num_joints();
  for (size_t i = 0; i < nodeCount; i++)
  {
      auto it = character.mesh->boneNamesMap.find(skeleton.joint_names()[i]);
      if (it != character.mesh->boneNamesMap.end())
      {
          int boneIdx = it->second;
          bones[boneIdx] = to_glm(character.models_[i]) * character.mesh->bones[boneIdx].invBindPose;
      }
  }

  shader.set_mat4x4("Bones", bones);


  render(character.mesh);

  //for (size_t i = 2; i < nodeCount; i++)
  //{
  //    glm::vec3 offset{ 0,0,0 };
  //    for (size_t j = i; j < nodeCount; j++)
  //    {
  //        if (skeleton.ref->parent[j] == int(i))
  //        {
  //            offset = glm::vec3(skeleton.localTm[j][3]);
  //            draw_arrow(skeleton.globalTm[i], vec3(0), offset, vec3(1.0f, 1.0f, 1.0f), 0.01f);
  //        }
  //    }
  //}

  for (size_t i = 0; i < nodeCount; i++)
  {
      for (size_t j = i; j < nodeCount; j++)
      {
          if (skeleton.joint_parents()[j] == int(i))
          {
              alignas(16) glm::vec3 offset;
              ozz::math::Store3Ptr(character.models_[i].cols[3] - character.models_[j].cols[3], glm::value_ptr(offset));

              glm::mat4 globTm = to_glm(character.models_[j]);
              offset = inverse(globTm) * glm::vec4(offset, .0f);
              draw_arrow(character.transform * to_glm(character.models_[j]), vec3(0), offset, vec3(0, 0.5f, 0), 0.01f);
          }
      }
  }
}

void render_imguizmo(ImGuizmo::OPERATION& mCurrentGizmoOperation, ImGuizmo::MODE& mCurrentGizmoMode)
{
    if (ImGui::Begin("gizmo window"))
    {
        //if (ImGui::IsKeyPressed('Q'))
        //    mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        //if (ImGui::IsKeyPressed('E'))
        //    mCurrentGizmoOperation = ImGuizmo::ROTATE;
        //if (ImGui::IsKeyPressed('R')) // r Key
        //    mCurrentGizmoOperation = ImGuizmo::SCALE;
        if (ImGui::RadioButton("Translate", mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
            mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Rotate", mCurrentGizmoOperation == ImGuizmo::ROTATE))
            mCurrentGizmoOperation = ImGuizmo::ROTATE;
        ImGui::SameLine();
        if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
            mCurrentGizmoOperation = ImGuizmo::SCALE;

        if (mCurrentGizmoOperation != ImGuizmo::SCALE)
        {
            if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
                mCurrentGizmoMode = ImGuizmo::LOCAL;
            ImGui::SameLine();
            if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
                mCurrentGizmoMode = ImGuizmo::WORLD;
        }
    }
    ImGui::End();
}
static AnimationPtr animation_list_combo()
{
    std::vector<const char*> animations(animationList.size() + 1);
    animations[0] = "None";
    for (size_t i = 0; i < animationList.size(); i++)
        animations[i + 1] = animationList[i].c_str();
    static int item = 0;
    if (ImGui::Combo("", &item, animations.data(), animations.size()))
    {
        if (item > 0)
        {
            SceneAsset sceneAsset = load_scene(animations[item],
                SceneAsset::LoadScene::Skeleton | SceneAsset::LoadScene::Animation);
            if (!sceneAsset.animations.empty())
                return sceneAsset.animations[0];
        }
    }
    return nullptr;
}
void imgui_render()
{
    ImGuizmo::BeginFrame();
    for (Character& character : scene->characters)
    {
         const auto &skeleton = *character.skeleton_;
         size_t nodeCount = skeleton.num_joints();

         if (ImGui::Begin("Skeleton view"))
         {
           for (size_t i = 0; i < nodeCount; i++)
           {
             ImGui::Text("%d) %s", int(i), skeleton.joint_names()[i]);
           }
         }
         ImGui::End();

        if (ImGui::Begin("Animation list"))
        {
            if (ImGui::Button("Play animation"))
                ImGui::OpenPopup("Select animation to play");
            if (ImGui::BeginPopup("Select animation to play"))
            {
                if (AnimationPtr animation = animation_list_combo())
                {
                    character.currentAnimation = animation;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            if (ImGui::TreeNode("controller"))
            {
                ImGui::SliderFloat("Animation time", &character.ratio, 0.0f, 1.0f);
                ImGui::Checkbox("Pause", &character.pause);
                ImGui::Checkbox("Loop", &character.loop);
                ImGui::SliderFloat("Playback speed", &character.animPLaybackSpeed, -3.0f, 3.0f);
                ImGui::TreePop();
            }

            if (ImGui::Button("Add layer"))
                ImGui::OpenPopup("Add layer to play");
            if (ImGui::BeginPopup("Add layer to play"))
            {
                if (AnimationPtr animation = animation_list_combo())
                {
                    character.layers.emplace_back(character.skeleton_, animation);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            for (size_t i = 0; i < character.layers.size(); i++)
            {
                AnimationLayer& layer = character.layers[i];
                ImGui::PushID(i);
                ImGui::Text("name: %s", layer.animation->name());
                ImGui::Text("duration: %f", layer.animation->duration());
                ImGui::SliderFloat("Weight", &layer.weight, 0.0f, 1.0f);

                if (ImGui::TreeNode("controller"))
                {
                    ImGui::SliderFloat("Animation time", &layer.ratio, 0.0f, 1.0f);
                    ImGui::Checkbox("Pause", &layer.pause);
                    ImGui::Checkbox("Loop", &layer.loop);
                    ImGui::SliderFloat("Playback speed", &layer.animPLaybackSpeed, -3.0f, 3.0f);
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }

        ImGui::End();


        static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
        static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::WORLD);
        render_imguizmo(mCurrentGizmoOperation, mCurrentGizmoMode);
        const glm::mat4& projection = scene->userCamera.projection;
        const glm::mat4& transform = scene->userCamera.transform;
        mat4 cameraView = inverse(transform);
        ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

        glm::mat4 globNodeTm = character.transform;

        ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(projection), mCurrentGizmoOperation, mCurrentGizmoMode,
            glm::value_ptr(globNodeTm));

        character.transform = globNodeTm;

        break;
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

  for (Character& character : scene->characters)
  {
      render_character(character, projView, glm::vec3(transform[3]), scene->light);
  }

  render_arrows(projView, glm::vec3(transform[3]), scene->light);
}

void close_game()
{
    scene.reset();
}
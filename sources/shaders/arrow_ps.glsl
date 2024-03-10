#version 330

uniform vec3 CameraPosition;
uniform vec3 LightDirection;
uniform vec3 AmbientLight;
uniform vec3 SunLight;

struct VsOutput
{
  vec3 EyespaceNormal;
  vec3 WorldPosition;
  vec3 Color;
};

in VsOutput vsOutput;
out vec4 FragColor;

vec3 LightedColor(
  vec3 color,
  float shininess,
  float metallness,
  vec3 world_position,
  vec3 world_normal,
  vec3 light_dir,
  vec3 camera_pos)
{
  vec3 W = normalize(camera_pos - world_position);
  vec3 E = reflect(light_dir, world_normal);
  float df = max(0.0, dot(world_normal, -light_dir));
  float sf = max(0.0, dot(E, W));
  sf = pow(sf, shininess);
  return color * (AmbientLight + df * SunLight) + vec3(1,1,1) * sf * metallness;
}
void main()
{
  float shininess = 40;
  float metallness = 0;
  vec3 color = LightedColor(vsOutput.Color, shininess, metallness,
    vsOutput.WorldPosition, vsOutput.EyespaceNormal, LightDirection, CameraPosition);
  color = vsOutput.Color;
  FragColor = vec4(color, 1.0);
}
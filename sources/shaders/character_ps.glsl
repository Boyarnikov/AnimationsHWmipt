#version 450

struct VsOutput
{
  vec3 EyespaceNormal;
  vec3 WorldPosition;
  vec2 UV;
};

uniform vec3 CameraPosition;
uniform vec3 LightDirection;
uniform vec3 AmbientLight;
uniform vec3 SunLight;

in VsOutput vsOutput;
in vec3 boneColors;
out vec4 FragColor;

uniform sampler2D mainTex;

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
  float shininess = 1.3;
  float metallness = 0.4;
  vec3 color = texture(mainTex, vsOutput.UV).rgb ;
  color = LightedColor(color, shininess, metallness, vsOutput.WorldPosition, vsOutput.EyespaceNormal, LightDirection, CameraPosition);

  FragColor = vec4(color, 1.0);
}

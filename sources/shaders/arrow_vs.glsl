#version 330

struct VsOutput
{
  vec3 EyespaceNormal;
  vec3 WorldPosition;
  vec3 Color;
};


uniform mat4 ViewProjection;
const int N = 128;
uniform mat4 ArrowTm[N];
uniform vec4 ArrowColor[N];


layout(location = 0)in vec3 Position;
layout(location = 1)in vec3 Normal;

out VsOutput vsOutput;

void main()
{
  vec4 worldPos = ArrowTm[gl_InstanceID] * vec4(Position, 1);
  vsOutput.WorldPosition = worldPos.xyz;
  gl_Position = ViewProjection * worldPos;
  mat3 ModelNorm = mat3(ArrowTm[gl_InstanceID]);
  ModelNorm[0] = normalize(ModelNorm[0]);
  ModelNorm[1] = normalize(ModelNorm[1]);
  ModelNorm[2] = normalize(ModelNorm[2]);
  vsOutput.EyespaceNormal = normalize(ModelNorm * Normal);

  vsOutput.Color = ArrowColor[gl_InstanceID].rgb;
}
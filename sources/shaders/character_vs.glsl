#version 450

struct VsOutput
{
  vec3 EyespaceNormal;
  vec3 WorldPosition;
  vec2 UV;
};

uniform mat4 Transform;
uniform mat4 ViewProjection;

const int N = 128;
uniform mat4 Bones[N];

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 UV;
layout(location = 3) in vec4 BoneWeights;
layout(location = 4) in uvec4 BoneIndex;

out VsOutput vsOutput;
out vec3 boneColors;

vec3 get_random_color(uint x)
{
  x += 1u;
  vec3 col = vec3(1.61803398875);
  col = fract(col) * vec3(x,x,x);
  col = fract(col) * vec3(1,x,x);
  col = fract(col) * vec3(1,1,x);
  //col = vec3(phi*i, phi*i*i, phi*i*i*i); // has precision issues
  return fract(col);
}

void main()
{

  mat4 BoneTransform = mat4(0);

  for (int i = 0; i < 4; i++)
  {
    BoneTransform += Bones[BoneIndex[i]] * BoneWeights[i];
  }

  BoneTransform = Transform * BoneTransform;
  vec3 VertexPosition = (BoneTransform * vec4(Position, 1)).xyz;

  vsOutput.EyespaceNormal = normalize((BoneTransform * vec4(Normal, 0)).xyz);

  gl_Position = ViewProjection * vec4(VertexPosition, 1);
  vsOutput.WorldPosition = VertexPosition;

  vsOutput.UV = UV;

  boneColors = vec3(0);

  for (int  i = 0; i < 4; i++)
  {
     boneColors += get_random_color(BoneIndex[i]) * BoneWeights[i]; 
  }
}

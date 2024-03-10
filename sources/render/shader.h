#pragma once
#include "3dmath.h"
#include <vector>
#include <string>
#include <memory>
#include "glad/glad.h"
#include <span>

struct ShaderUniform
{
  std::string name;
  unsigned int type;
  int shaderLocation;
};


class Shader
{
public:

	using ShaderSources = std::vector<std::pair<GLuint, std::string>>;

	const std::string name;
	const ShaderSources shaderSources; //for hotreload
	GLuint program;
  std::vector<ShaderUniform> uniforms;

	Shader(const std::string &shader_name, GLuint shader_program, ShaderSources sources):
		name(shader_name),
		shaderSources(sources),
		program(shader_program)
	{}

	void use() const
	{
		glUseProgram(program);
	}

	int get_uniform_location(const char *name)
	{
		return glGetUniformLocation(program, name);
	}
	void set_mat3x3(const char*name, const mat3 &matrix, bool transpose = false) const
	{
		glUniformMatrix3fv(glGetUniformLocation(program, name), 1, transpose, glm::value_ptr(matrix));
	}
	void set_mat3x3(int uniform_location, const mat3 &matrix, bool transpose = false) const
	{
		glUniformMatrix3fv(uniform_location, 1, transpose, glm::value_ptr(matrix));
	}

	void set_mat4x4(const char *name, const mat4 matrix, bool transpose = false) const
	{
		set_mat4x4(glGetUniformLocation(program, name), matrix, transpose);
	}
	void set_mat4x4(int uniform_location, const mat4 matrix, bool transpose = false) const
	{
		glUniformMatrix4fv(uniform_location, 1, transpose, glm::value_ptr(matrix));
	}
	void set_mat4x4(const char* name, const std::span<mat4>& matrix, bool transpose = false) const
	{
		glUniformMatrix4fv(glGetUniformLocation(program, name), matrix.size(), transpose, glm::value_ptr(matrix[0]));
	}
	void set_float(const char *name, const float &v) const
	{
		set_float(glGetUniformLocation(program, name), v);
  }
	void set_float(int uniform_location, const float &v) const
	{
		glUniform1fv(uniform_location, 1, &v);
  }
	void set_int(const char *name, int v) const
	{
		set_int(glGetUniformLocation(program, name), v);
  }
	void set_int(int uniform_location, int v) const
	{
		glUniform1i(uniform_location, v);
  }

	void set_vec2(const char*name, const vec2 &v) const
	{
		set_vec2(glGetUniformLocation(program, name), v);
  }
	void set_vec2(int uniform_location, const vec2 &v) const
	{
		glUniform2fv(uniform_location, 1, glm::value_ptr(v));
  }

	void set_vec3(const char*name, const vec3 &v) const
	{
		set_vec3(glGetUniformLocation(program, name), v);
  }
	void set_vec3(int uniform_location, const vec3 &v) const
	{
		glUniform3fv(uniform_location, 1, glm::value_ptr(v));
  }

	void set_vec4(const char*name, const vec4 &v) const
	{
		set_vec4(glGetUniformLocation(program, name), v);
  }
	void set_vec4(int uniform_location, const vec4 &v) const
	{
		glUniform4fv(uniform_location, 1, glm::value_ptr(v));
	}
	void set_vec4(const char* name, const std::span<vec4>& v) const
	{
		glUniform4fv(glGetUniformLocation(program, name), v.size(), glm::value_ptr(v[0]));
	}
};

using ShaderPtr = std::shared_ptr<Shader>;

ShaderPtr compile_shader(const char *name, const char *vs_path, const char *ps_path);

void recompile_all_shaders();

Shader::Shader(const char* vp, const char *fp)
{
	vert_path = vp;
	frag_path = fp;
	program = link_opengl_program(compile_opengl_shader(vert_path, GL_VERTEX_SHADER), compile_opengl_shader(frag_path, GL_FRAGMENT_SHADER));
}

void Shader::recompile()
{
	GLuint tmp = link_opengl_program(compile_opengl_shader(vert_path, GL_VERTEX_SHADER), compile_opengl_shader(frag_path, GL_FRAGMENT_SHADER));
	glDeleteProgram(program);
	program = tmp;
}
void Shader::set_uniform(const char* attr, vec2 v)
{
	glUniform2f(glGetUniformLocation(program, attr), v.x, v.y);
}
void Shader::set_uniform(const char* attr, vec3 v)
{
	glUniform3f(glGetUniformLocation(program, attr), v.x, v.y, v.z);
}
void Shader::set_uniform(const char* attr, vec4 v)
{
	glUniform4f(glGetUniformLocation(program, attr), v.x, v.y, v.z, v.w);	
}
void Shader::set_uniform(const char* attr, int v)
{
	glUniform1i(glGetUniformLocation(program, attr), v);
}
void Shader::set_uniform(const char* attr, uint v)
{
	glUniform1ui(glGetUniformLocation(program, attr), v);
}
void Shader::set_uniform(const char* attr, float v)
{
	glUniform1f(glGetUniformLocation(program, attr), v);
}
void Shader::set_uniform(const char* attr, mat4 v)
{
	glUniformMatrix4fv(glGetUniformLocation(program, attr), 1, GL_FALSE, &v[0][0]);
}
void Shader::set_uniform(const char* attr, Light l)
{
	assert(strlen(attr) < 64);
	char buf[64];
	sprintf(buf, "%s.position", attr);
	set_uniform(buf, glm::vec4(l.position.x, l.position.y, l.position.z, l.position.w));
	sprintf(buf, "%s.ambient", attr);
	set_uniform(buf, glm::vec3(l.ambient.x, l.ambient.y, l.ambient.z));
	sprintf(buf, "%s.diffuse", attr);
	set_uniform(buf, glm::vec3(l.diffuse.x, l.diffuse.y, l.diffuse.z));
	sprintf(buf, "%s.specular", attr);
	set_uniform(buf, glm::vec3(l.specular.x, l.specular.y, l.specular.z));
}
void Shader::set_uniform(const char* attr, Material m)
{
	// TODO: assert that string length will never be larger than buffer size
	assert(strlen(attr) < 64);
	char buf[64];
	sprintf(buf, "%s.ambient", attr);
	set_uniform(buf, glm::vec3(m.ambient.x, m.ambient.y, m.ambient.z));
	sprintf(buf, "%s.diffuse", attr);
	set_uniform(buf, glm::vec3(m.diffuse.x, m.diffuse.y, m.diffuse.z));
	sprintf(buf, "%s.ambient", attr);
	set_uniform(buf, glm::vec3(m.ambient.x, m.ambient.y, m.ambient.z));
	sprintf(buf, "%s.shine", attr);
	set_uniform(buf, m.shine);
}

void Shader::use()
{
	glUseProgram(program);
}

// NOTE: 2d textures only
void Shader::set_texture(const char* attr, GLuint tex, GLuint slot)
{
	assert(slot < 8);
	switch (slot) {
	case 0: glActiveTexture(GL_TEXTURE0); break;
	case 1: glActiveTexture(GL_TEXTURE1); break;
	case 2: glActiveTexture(GL_TEXTURE2); break;
	case 3: glActiveTexture(GL_TEXTURE3); break;
	case 4: glActiveTexture(GL_TEXTURE4); break;
	case 5: glActiveTexture(GL_TEXTURE5); break;
	case 6: glActiveTexture(GL_TEXTURE6); break;
	case 7: glActiveTexture(GL_TEXTURE7); break;
	case 8: glActiveTexture(GL_TEXTURE8); break;
	default: assert(0);
	}
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1i(glGetUniformLocation(program, attr), slot);
}

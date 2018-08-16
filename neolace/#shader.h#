struct Shader {
	const char* vert_path;
	const char* frag_path;
	GLuint program;
	Shader(const char* vp, const char *fp);
	void set_texture(const char*, GLuint, GLuint);
	void set_uniform(const char*, glm::vec2);
	void set_uniform(const char*, glm::vec3);
	void set_uniform(const char*, glm::vec4);
	void set_uniform(const char*, int);
	void set_uniform(const char*, uint);
	void set_uniform(const char*, float);
	void set_uniform(const char*, glm::mat4);
	void set_uniform(const char*, Light);
	void set_uniform(const char*, Material);
	void recompile();
	void use();
};

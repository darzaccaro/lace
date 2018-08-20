#define MAX_VAO_ARRAY_SIZE 256
struct Vao {
	GLuint id;
	GLuint num_indices;
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	
	Vao(const char* path_to_obj_file, Shader *shaders[], uint num_shaders);
	void bind();
	void draw(Shader *shader);

};

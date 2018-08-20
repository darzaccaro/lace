// NOTE: A model must have vertices, uvs, and normals!
Vao::Vao(const char* obj_path, Shader *shaders[], uint num_shaders) {
	std::vector<uint> vertex_indices, uv_indices, normal_indices;
	std::vector<glm::vec3> tmp_vertices;
	std::vector<glm::vec2> tmp_uvs;
	std::vector<glm::vec3> tmp_normals;

	FILE *fp = fopen(obj_path, "r");
	assert(fp);

	char line_prefix[256];
	for (;;) {
		int res = fscanf(fp, "%s", line_prefix);
		if (res == EOF) break;
		if (strcmp(line_prefix, "v") == 0) {
			glm::vec3 v;
			fscanf(fp, "%f %f %f\n", &v.x, &v.y, &v.z);
			tmp_vertices.push_back(v);
		}
		else if (strcmp(line_prefix, "vt") == 0) {
			glm::vec2 uv;
			fscanf(fp, "%f %f\n", &uv.x, &uv.y);
			tmp_uvs.push_back(uv);
		}
		else if (strcmp(line_prefix, "vn") == 0) {
			glm::vec3 n;
			fscanf(fp, "%f %f %f\n", &n.x, &n.y, &n.z);
			/* bool unique = true;
						for (auto nml : normals) {
							if (nml == n) unique = false;
						}
						if (unique) tmp_normals.push_back(n);
			*/
			tmp_normals.push_back(n);
		}
		else if (strcmp(line_prefix, "f") == 0) {
			uint vertex_index[3], uv_index[3], normal_index[3];
			int matches = fscanf(fp, "%d/%d/%d %d/%d/%d %d/%d/%d\n",
				&vertex_index[0], &uv_index[0], &normal_index[0],
				&vertex_index[1], &uv_index[1], &normal_index[1],
				&vertex_index[2], &uv_index[2], &normal_index[2]);
			assert(matches == 9);
			for (int i = 0; i < 3; i++) {
				vertex_indices.push_back(vertex_index[i]);
				uv_indices.push_back(uv_index[i]);
				normal_indices.push_back(normal_index[i]);
			}

		}
		else {
			char trash[1000];
			fgets(trash, 1000, fp);
		}
	}

	for (uint i = 0; i < vertex_indices.size(); i++) {
		uint vertex_index = vertex_indices[i];
		uint uv_index = uv_indices[i];
		uint normal_index = normal_indices[i];

		// .obj file format indexes from 1 instead of 0, which is why we subtract 1
		glm::vec3 vertex = tmp_vertices[vertex_index - 1];
		glm::vec2 uv = tmp_uvs[uv_index - 1];
		glm::vec3 normal = tmp_normals[normal_index - 1];
		vertices.push_back(vertex);
		uvs.push_back(uv);
		normals.push_back(normal);
	}
	/*


	int i = 0;
	for (auto v: vertices) {
		vertex_array[i] = v.x;
		vertex_array[i + 1] = v.y;
		vertex_array[i + 2] = v.z;
		i += 3;
	}
	assert(i < MAX_VAO_ARRAY_SIZE);
	vertex_array_size = (i - 3) * sizeof(float);

	num_indices = i / 3;
		
	i = 0;
	for (auto v: uvs) {
		uv_array[i] = v.x;
		uv_array[i + 1] = v.y;
		i += 2;
	}	
	assert(i < MAX_VAO_ARRAY_SIZE);
	uv_array_size = (i - 2) * sizeof(float);
		
	// TODO: why are there more normals than vertices??? our indices are wrong
	i = 0;
	for (auto v: tmp_normals) {
		normal_array[i] = v.x;
		normal_array[i + 1] = v.y;
		normal_array[i + 2] = v.z;
		i += 3;
	}
	assert(i < MAX_VAO_ARRAY_SIZE);
	normal_array_size = (i - 3) * sizeof(float);
	*/	
	glGenVertexArrays(1, &id);
	glBindVertexArray(id);

	GLuint vertex_buffer, uv_buffer, normal_buffer;
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
	for (int i = 0; i < num_shaders; i++) {
		GLuint attr_position = glGetAttribLocation(shaders[i]->program, "position");
		glEnableVertexAttribArray(attr_position);
		glVertexAttribPointer(attr_position, 3, GL_FLOAT, GL_FALSE, 0, (void*)0); // does this depend on the bound buffer? YES!!!!!! it does, wow!
	}

	glGenBuffers(1, &uv_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
	for (int i = 0; i < num_shaders; i++) {
		GLuint attr_texcoord = glGetAttribLocation(shaders[i]->program, "texcoord");
		glEnableVertexAttribArray(attr_texcoord);
		glVertexAttribPointer(attr_texcoord, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	}

	glGenBuffers(1, &normal_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, normal_buffer);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
	for (int i = 0; i < num_shaders; i++) {
		GLuint attr_norml = glGetAttribLocation(shaders[i]->program, "norml");
		glEnableVertexAttribArray(attr_norml);
		glVertexAttribPointer(attr_norml, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	}
		
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

}


void Vao::bind()
{
	glBindVertexArray(id);
}

void Vao::draw(Shader *s)
{	
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

void draw_sun_and_cubes(Renderer *r, Camera *c, uint frame_count) {
	glBindFramebuffer(GL_FRAMEBUFFER, r->framebuffers[MSAA]); // todo: msaa in the post_fbo??

	static mat4 model_a;
	if (frame_count == 0) {
		model_a = mat4(1.0f);
	}
	c->.update(window, window_width, window_height, keystate, fullscreen);
	light.position = vec4(c->.position, 1.0f); // if w == 0.0, this is a directional light
	static bool did_once = false;
	if (!did_once) {
		//c->.position = vec3(271.979553, -4.139664, 113.563263);
		//c->.position = vec3(0.0f, 4.0f, 124.0f);
		c->.position = vec3(-453.821167, 865.267334, -2387.491455);
		c->.front = vec3(0.199234, -0.390731, 0.898685);
		//c->.front = vec3(-0.034846, 0.990268, -0.134740); // TODO: why doesn't this work???
		did_once = true;
	}
	c->.position += 0.5f * c->.front;
	glDisable(GL_DEPTH_TEST);
	shaders[SN_BACKGROUND]->use();
	glBindVertexArray(quad_vao);// TODO: implement as vao.bind()
	shaders[SN_BACKGROUND]->set_uniform("resolution", vec2((float)window_width, (float)window_height));
	shaders[SN_BACKGROUND]->set_uniform("frame_count", frame_count);
	shaders[SN_BACKGROUND]->set_uniform("scene", scene);
	glDrawArrays(GL_TRIANGLES, 0, 6);// TODO: implement as vao.draw()

		
	glEnable(GL_DEPTH_TEST); // todo: renderer.enable(GL_DEPTH_TEST); might be overengineering
	glClear(GL_DEPTH_BUFFER_BIT); // TODO: renderer.clear(GL_COLOR_BUIFFER_BIT | GL_DEPTH_BUFFER_TEST), etc...
	shaders[SN_CUBE]->use();
	shaders[SN_CUBE]->set_uniform("material", default_material);
	shaders[SN_CUBE]->set_uniform("light", light);
	shaders[SN_CUBE]->set_uniform("resolution", vec2((float)window_width, (float)window_height));
	shaders[SN_CUBE]->set_uniform("frame_count", frame_count);
	shaders[SN_CUBE]->set_uniform("scene", scene);

	shaders[SN_CUBE]->set_uniform("offset", 0);
	shaders[SN_CUBE]->set_uniform("model", model_a);
	shaders[SN_CUBE]->set_uniform("view", c->.view);
	shaders[SN_CUBE]->set_uniform("projection", c->.projection);

	float t = float(frame_count * 0.02);
	static int b = 0;
	if ((frame_count % 10) == 0) {
		b++;
	}

	{
		mat4 rotm = rotate(model_a, radians(45.f), vec3(1.0f, 0.0f, 1.0f));
		for (int i = 0; i < 64; i++) {
			float row = i;
			
			for (int ii = 0; ii < 16; ii++) {
				float row = i; float col = ii;
				float z;
				if (mod(t, sin(row * 10.0f)) <= 2.0f) z = -4.0f * row * col;
				else z = -4.0f;
				
				mat4 m = translate(rotm, vec3(row * 8.0f * sin(row), col * 8.0f * sin(col), z));

				m = scale(m, vec3(row * 2.0f + 1.0f, (col * 2.0f) + 2.0f, 10.0f + 40.0f  * abs(sin(4.0f)) * abs(sin(row)) * abs(cos(col))));
								
								
				shaders[SN_CUBE]->set_uniform("model", m);
				shaders[SN_CUBE]->set_uniform("brightness", 0.4f);
				cube_vao.draw();
			}
		}
			
	}
	/*
	  {
	  mat4 m = model_a;
	  m = translate(m, vec3(0.0f, -2.0f, 0.0f));
	  m = scale(m, vec3(128.0f, 0.0, 128.0f));
	  shaders[SN_CUBE]->set_uniform("model", m);
	  shaders[SN_CUBE]->set_uniform("material", other_material);
	  shaders[SN_CUBE]->set_uniform("brightness", 0.5f);
	  plane_vao.draw();
	  }
	*/
	{
		mat4 m = model_a;
		m = translate(m, vec3(0.0f, 0.0f, -128.0f)); //TODO is our z-axis reversed somehow???
		m = scale(m, vec3(128.0f, 128.0f, 128.0f));
		shaders[SN_CUBE]->set_uniform("model", m);
		shaders[SN_CUBE]->set_uniform("material", other_material);
		shaders[SN_CUBE]->set_uniform("brightness", 0.8f);
		sphere_vao.draw();
	}
}

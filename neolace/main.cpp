#include <SDL.h>
#include <GL/glew.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <map>
#include <vector>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
using namespace glm;

#pragma warning(disable: 4996)

#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))

#define KILOBYTES(n) ((unsigned long)(n) * 1024)
#define MEGABYTES(n) ((unsigned long)KILOBYTES(n) * 1024)
#define GIGABYTES(n) ((unsigned long)MEGABYTES(n) * 1024)
#define TERABYTES(n) ((unsigned long)GIGABYTES(n) * 1024)

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

struct Light {
	vec4 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct Material {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shine;
};

// TODO finish compressing this
void read_file(const char* path, char** data, long* size);

// TODO inline these functions in the shader constructor?
GLuint compile_opengl_shader(const char* path, GLenum type);
GLuint link_opengl_program(GLuint vert, GLuint frag);

#include "noise.h"
#include "camera.h"
#include "shader.h"
#include "vao.h"

int main(int argc, char *argv[]) {
	SDL_Window *window = NULL;
	SDL_GLContext glcontext;
	GLenum err;
	SDL_version compiled, linked;
	const char *title = "neolace demo";
	i32 window_width = 1280;
	i32 window_height = 720;
	u32 current_time = 0;
	u32 last_time = 0;
	bool fullscreen = false;
	u32 frame_count = 0;
	u32 scene = 0;

	assert(SDL_Init(SDL_INIT_VIDEO) >= 0);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	SDL_GL_SetSwapInterval(1);
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, 0, window_width, window_height, SDL_WINDOW_OPENGL | SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS);
	assert(window);

	glcontext = SDL_GL_CreateContext(window);
	assert(glcontext);

	err = glewInit();
	if (GLEW_OK != err) {
		printf("Error: %s\n", glewGetErrorString(err));
	}
	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	printf("Compiled against SDL version %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch);
	printf("Linked against SDL version %d.%d.%d\n", linked.major, linked.minor, linked.patch);
	printf("Using OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("Using GLEW version: %s\n", glewGetString(GLEW_VERSION));
	printf("Running %s demo from: %s\n", title, SDL_GetBasePath());

	glEnable(GL_MULTISAMPLE); // TODO: do we have to do this after we bind our fbo?
	glDepthFunc(GL_LEQUAL);

	glViewport(0, 0, window_width, window_height);
	glClearColor(0.2456f, 0.5432f, 0.23f, 1.0f);

	enum Shader_Name { SN_BACKGROUND, SN_OBJECT, SN_POST, SN_FXAA, SN_ADD_TEXTURES, SN_BRIGHTNESS_MASK, SN_BLUR, SN_MIRROR, SN_MAX };
	Shader *shaders[SN_MAX];
	shaders[SN_BACKGROUND] = &Shader("background.vert", "background.frag");
	shaders[SN_OBJECT] = &Shader("cube.vert", "cube.frag");
	shaders[SN_POST] = &Shader("quad.vert", "post.frag");
	shaders[SN_FXAA] = &Shader("quad.vert", "fxaa.frag");
	shaders[SN_ADD_TEXTURES] = &Shader("quad.vert", "add_textures.frag");
	shaders[SN_BRIGHTNESS_MASK] = &Shader("quad.vert", "brightness_mask.frag");
	shaders[SN_BLUR] = &Shader("quad.vert", "blur.frag");
	shaders[SN_MIRROR] = &Shader("quad.vert", "mirror.frag");

	// TODO: fix sphere's origin to be at center! m,
	auto oj_vao = Vao("models/oj.obj", shaders, SN_MAX);
	auto sphere_vao = Vao("models/sphere.obj", shaders, SN_MAX); 
	auto cube_vao = Vao("models/cube.obj", shaders, SN_MAX);
	auto plane_vao = Vao("models/plane.obj", shaders, SN_MAX);
	// TODO: export a plane from blender

	GLuint quad_vao; // NOTE: a quad is different from a plane!
	GLfloat quad_data[] = {
			       // position        //uv
			       -1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
			       1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
			       -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,

			       -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
			       1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
			       1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
	};
	glGenVertexArrays(1, &quad_vao);
	glBindVertexArray(quad_vao);
	{ // todo add normals
		GLuint position_vbo;
		glGenBuffers(1, &position_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_data), quad_data, GL_STATIC_DRAW);
		for (Shader* s : shaders) {
			glEnableVertexAttribArray(glGetAttribLocation(s->program, "position"));
			glVertexAttribPointer(glGetAttribLocation(s->program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
		}

		GLuint texcoord_vbo;
		glGenBuffers(1, &texcoord_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, texcoord_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quad_data), quad_data, GL_STATIC_DRAW);
		for (Shader* s : shaders) {
			glEnableVertexAttribArray(glGetAttribLocation(s->program, "texcoord"));
			glVertexAttribPointer(glGetAttribLocation(s->program, "texcoord"), 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); // automatically do these for each shader!!!!!!!!!
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}



	GLuint post_fbo, post_fbo_depth_buffer;
	glGenFramebuffers(1, &post_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, post_fbo);
	GLuint post_texture;
	glGenTextures(1, &post_texture);
	glBindTexture(GL_TEXTURE_2D, post_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, post_texture, 0);
	glGenRenderbuffers(1, &post_fbo_depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, post_fbo_depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_width, window_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, post_fbo_depth_buffer);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GLuint mirror_fbo, mirror_fbo_depth_buffer;
	glGenFramebuffers(1, &mirror_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, mirror_fbo);
	GLuint mirror_texture;
	glGenTextures(1, &mirror_texture);
	glBindTexture(GL_TEXTURE_2D, mirror_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirror_texture, 0);
	glGenRenderbuffers(1, &mirror_fbo_depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, mirror_fbo_depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_width, window_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mirror_fbo_depth_buffer);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GLuint brightness_mask_fbo, brightness_mask_fbo_depth_buffer;
	glGenFramebuffers(1, &brightness_mask_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, brightness_mask_fbo);
	GLuint brightness_mask_texture;
	glGenTextures(1, &brightness_mask_texture);
	glBindTexture(GL_TEXTURE_2D, brightness_mask_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brightness_mask_texture, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glGenRenderbuffers(1, &brightness_mask_fbo_depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, brightness_mask_fbo_depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_width, window_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, brightness_mask_fbo_depth_buffer);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GLuint blur_fbos[2], blur_fbo_depth_buffers[2], blur_textures[2];
	glGenFramebuffers(2, blur_fbos);
	glGenTextures(2, blur_textures);
	glGenRenderbuffers(2, blur_fbo_depth_buffers);
	for (int i = 0; i < 2; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, blur_fbos[i]);
	
		glBindTexture(GL_TEXTURE_2D, blur_textures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blur_textures[i], 0);

		glBindRenderbuffer(GL_RENDERBUFFER, blur_fbo_depth_buffers[i]);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_width, window_height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, blur_fbo_depth_buffers[i]);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	GLuint msaa_fbo, msaa_texture, msaa_rbo;
	glGenTextures(1, &msaa_texture);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, msaa_texture);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, window_width, window_height, GL_TRUE);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
	glGenFramebuffers(1, &msaa_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, msaa_texture, 0);
	// TODO: error check fbo status
	glGenRenderbuffers(1, &msaa_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, msaa_rbo);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, window_width, window_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, msaa_rbo);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	auto camera = Camera(window_width, window_height);
	Light light;
	light.position = vec4(3.0f, 3.0f, 8.0f, 1.0f);
	light.ambient = vec3(0.2f, 0.2f, 0.2f);
	light.diffuse = vec3(0.5f, 0.5f, 0.5f);
	light.specular = vec3(1.0f, 1.0f, 1.0f);

	// TODO: read materials from blender!
	Material default_material;
	default_material.ambient = vec3(1.0f, 0.5f, 0.31f);
	default_material.diffuse = vec3(1.0f, 0.5f, 0.31f);
	default_material.specular = vec3(0.5f, 0.5f, 0.5f);
	default_material.shine = 64;

	Material other_material;
	other_material.ambient = vec3(0.4f, 0.2f, 0.31f);
	other_material.diffuse = vec3(0.4f, 0.2f, 0.31f);
	other_material.specular = vec3(0.25f, 0.25f, 0.25f);
	other_material.shine = 8;

	for (;;) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_KEYDOWN) {
				SDL_Keycode key = event.key.keysym.sym;
				if (key == SDLK_PAUSE) goto cleanup;
				else if (key == SDLK_F11) {
					fullscreen = !fullscreen;
					u32 flag = (fullscreen) ? SDL_WINDOW_FULLSCREEN : 0;
					SDL_SetWindowFullscreen(window, flag);
					SDL_GetWindowSize(window, &window_width, &window_height);
					glViewport(0, 0, window_width, window_height);
					if (fullscreen) SDL_ShowCursor(0);
					else SDL_ShowCursor(1);

				}
				else if (key == SDLK_r) {
					u32 tmp;
					SDL_Delay(100);
					for (Shader* s: shaders) {
						s->recompile();
					}
					SDL_Delay(100);
				}
				else if (key == SDLK_0) scene = 0;
				else if (key == SDLK_1) scene = 1;
				else if (key == SDLK_2) scene = 2;
				else if (key == SDLK_3) scene = 3;
				else if (key == SDLK_4) scene = 4;
				else if (key == SDLK_5) scene = 5;
				else if (key == SDLK_6) scene = 6;
				else if (key == SDLK_7) scene = 7;
				else if (key == SDLK_8) scene = 8;
				else if (key == SDLK_9) scene = 9;
			}
		}

		do {
			current_time = SDL_GetTicks();
		} while (current_time - last_time < 16);
		const u8* keystate = SDL_GetKeyboardState(NULL);

		// RENDER

		// post processing ----------------------------------

		static mat4 model_a;
		static bool did_once = false;
		static int b = 0;
		bool render_mirror;
		if (scene == 1) {
			render_mirror = true;

			glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo); // todo: msaa in the post_fbo??

			if (frame_count == 0) {
				model_a = mat4(1.0f);
			}
			camera.update(window, window_width, window_height, keystate, fullscreen);
			light.position = vec4(camera.position, 1.0f); // if w == 0.0, this is a directional light

			if (!did_once) {
				//camera.position = vec3(271.979553, -4.139664, 113.563263);
				//camera.position = vec3(0.0f, 4.0f, 124.0f);
				camera.position = vec3(-453.821167, 865.267334, -2387.491455);
				camera.front = vec3(0.199234, -0.390731, 0.898685);
				//camera.front = vec3(-0.034846, 0.990268, -0.134740); // TODO: why doesn't this work???
				did_once = true;
			}
			camera.position += 0.5f * camera.front;
			glDisable(GL_DEPTH_TEST);
			shaders[SN_BACKGROUND]->use();
			glBindVertexArray(quad_vao);// TODO: implement as vao.bind()
			shaders[SN_BACKGROUND]->set_uniform("resolution", vec2((float)window_width, (float)window_height));
			shaders[SN_BACKGROUND]->set_uniform("frame_count", frame_count);
			shaders[SN_BACKGROUND]->set_uniform("scene", scene);
			glDrawArrays(GL_TRIANGLES, 0, 6);// TODO: implement as vao.draw()

		
			glEnable(GL_DEPTH_TEST); // todo: renderer.enable(GL_DEPTH_TEST); might be overengineering
			glClear(GL_DEPTH_BUFFER_BIT); // TODO: renderer.clear(GL_COLOR_BUIFFER_BIT | GL_DEPTH_BUFFER_TEST), etc...
			shaders[SN_OBJECT]->use();
			shaders[SN_OBJECT]->set_uniform("material", default_material);
			shaders[SN_OBJECT]->set_uniform("light", light);
			shaders[SN_OBJECT]->set_uniform("resolution", vec2((float)window_width, (float)window_height));
			shaders[SN_OBJECT]->set_uniform("frame_count", frame_count);
			shaders[SN_OBJECT]->set_uniform("scene", scene);

			shaders[SN_OBJECT]->set_uniform("offset", 0);
			shaders[SN_OBJECT]->set_uniform("model", model_a);
			shaders[SN_OBJECT]->set_uniform("view", camera.view);
			shaders[SN_OBJECT]->set_uniform("projection", camera.projection);

			float t = float(frame_count * 0.02);

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
								
								
						shaders[SN_OBJECT]->set_uniform("model", m);
						shaders[SN_OBJECT]->set_uniform("brightness", 0.4f);
						cube_vao.draw();
					}
				}
			
			}
			
			{
				mat4 m = model_a;
				m = translate(m, vec3(0.0f, -2.0f, 0.0f));
				m = scale(m, vec3(128.0f, 0.0, 128.0f));
				shaders[SN_OBJECT]->set_uniform("model", m);
				shaders[SN_OBJECT]->set_uniform("material", other_material);
				shaders[SN_OBJECT]->set_uniform("brightness", 0.5f);
				plane_vao.draw();
			}
			
			{
				mat4 m = model_a;
				m = translate(m, vec3(0.0f, 0.0f, -128.0f)); //TODO is our z-axis reversed somehow???
				m = scale(m, vec3(128.0f, 128.0f, 128.0f));
				shaders[SN_OBJECT]->set_uniform("model", m);
				shaders[SN_OBJECT]->set_uniform("material", other_material);
				shaders[SN_OBJECT]->set_uniform("brightness", 0.8f);
				// TODO: set texture
				sphere_vao.draw();
				
			}

		} else if (scene == 0) {
			// render msaa to post texture
			render_mirror = false; // todo: render_state struct???

			glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo); // todo: msaa in the post_fbo??

			if (frame_count == 0) {
				model_a = mat4(1.0f);
			}
			camera.update(window, window_width, window_height, keystate, fullscreen);
			light.position = vec4(camera.position, 1.0f); // if w == 0.0, this is a directional light

			if (!did_once) {
				//camera.position = vec3(271.979553, -4.139664, 113.563263);
				//camera.position = vec3(0.0f, 4.0f, 124.0f);
				camera.position = vec3(-453.821167, 865.267334, -2387.491455);
				camera.front = vec3(0.199234, -0.390731, 0.898685);
				//camera.front = vec3(-0.034846, 0.990268, -0.134740); // TODO: why doesn't this work???
				did_once = true;
			}
			camera.position += 0.5f * camera.front;
			glDisable(GL_DEPTH_TEST);
			shaders[SN_BACKGROUND]->use();
			glBindVertexArray(quad_vao);// TODO: implement as vao.bind()
			shaders[SN_BACKGROUND]->set_uniform("resolution", vec2((float)window_width, (float)window_height));
			shaders[SN_BACKGROUND]->set_uniform("frame_count", frame_count);
			shaders[SN_BACKGROUND]->set_uniform("scene", scene);
			glDrawArrays(GL_TRIANGLES, 0, 6);// TODO: implement as vao.draw()

		
			glEnable(GL_DEPTH_TEST); // todo: renderer.enable(GL_DEPTH_TEST); might be overengineering
			glClear(GL_DEPTH_BUFFER_BIT); // TODO: renderer.clear(GL_COLOR_BUIFFER_BIT | GL_DEPTH_BUFFER_TEST), etc...
			shaders[SN_OBJECT]->use();
			shaders[SN_OBJECT]->set_uniform("material", default_material);
			shaders[SN_OBJECT]->set_uniform("light", light);
			shaders[SN_OBJECT]->set_uniform("resolution", vec2((float)window_width, (float)window_height));
			shaders[SN_OBJECT]->set_uniform("frame_count", frame_count);
			shaders[SN_OBJECT]->set_uniform("scene", scene);

			shaders[SN_OBJECT]->set_uniform("offset", 0);
			shaders[SN_OBJECT]->set_uniform("model", model_a);
			shaders[SN_OBJECT]->set_uniform("view", camera.view);
			shaders[SN_OBJECT]->set_uniform("projection", camera.projection);

			float t = float(frame_count * 0.02);

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
								
								
						shaders[SN_OBJECT]->set_uniform("model", m);
						shaders[SN_OBJECT]->set_uniform("brightness", 0.4f);
						cube_vao.draw();
					}
				}
			
			}
			/*
			  {
			  mat4 m = model_a;
			  m = translate(m, vec3(0.0f, -2.0f, 0.0f));
			  m = scale(m, vec3(128.0f, 0.0, 128.0f));
			  shaders[SN_OBJECT]->set_uniform("model", m);
			  shaders[SN_OBJECT]->set_uniform("material", other_material);
			  shaders[SN_OBJECT]->set_uniform("brightness", 0.5f);
			  plane_vao.draw();
			  }
			*/
			{
				mat4 m = model_a;
				m = translate(m, vec3(0.0f, 0.0f, -128.0f)); //TODO is our z-axis reversed somehow???
				m = scale(m, vec3(128.0f, 128.0f, 128.0f));
				shaders[SN_OBJECT]->set_uniform("model", m);
				shaders[SN_OBJECT]->set_uniform("material", other_material);
				shaders[SN_OBJECT]->set_uniform("brightness", 0.2f);
				oj_vao.draw();
			}
		}
		// PostFX
		glActiveTexture(GL_TEXTURE0);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, msaa_fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, post_fbo);
		glBlitFramebuffer(0, 0, window_width, window_height, 0, 0, window_width, window_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		glDisable(GL_DEPTH_TEST);
		glBindVertexArray(quad_vao);
		// render brightness mask from post texture to brightness texture
		glBindFramebuffer(GL_FRAMEBUFFER, brightness_mask_fbo);
		shaders[SN_BRIGHTNESS_MASK]->use();
		shaders[SN_BRIGHTNESS_MASK]->set_texture("tex0", post_texture, 0);
		shaders[SN_BRIGHTNESS_MASK]->set_uniform("resolution", vec2(window_width, window_height));
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// TODO pingpong between 2 color attachments instead!!!
		glBindFramebuffer(GL_FRAMEBUFFER, blur_fbos[0]);
		shaders[SN_BLUR]->use();
		shaders[SN_BLUR]->set_texture("tex0", brightness_mask_texture, 0);
		shaders[SN_BLUR]->set_uniform("horizontal", true);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindFramebuffer(GL_FRAMEBUFFER, blur_fbos[1]);
		shaders[SN_BLUR]->set_texture("tex0", blur_textures[0], 0);
		shaders[SN_BLUR]->set_uniform("horizontal", false);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// render add brightness to post and render to screen
		glBindFramebuffer(GL_FRAMEBUFFER, mirror_fbo);
		shaders[SN_ADD_TEXTURES]->use();
		shaders[SN_ADD_TEXTURES]->set_texture("tex0", post_texture, 0);
		shaders[SN_ADD_TEXTURES]->set_texture("tex1", blur_textures[1], 1);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// TODO: a final color pass with pow(color, 2.0), color correction, gamma correction, etc...
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		shaders[SN_MIRROR]->use();
		shaders[SN_MIRROR]->set_texture("tex0", mirror_texture, 0);
		shaders[SN_MIRROR]->set_uniform("resolution", vec2(float(window_width), float(window_height)));
		if (render_mirror) {
			shaders[SN_MIRROR]->set_uniform("num_splits", 4);
		} else {
			shaders[SN_MIRROR]->set_uniform("num_splits", 0);
		}
		//shaders[SN_MIRROR]->set_uniform("cuts", 4);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// renderer.unbind(RO_Shader);
		// renderer.unbind(RO_Vao);
		// renderer.unbind(RO_Texture);
		// renderer.unbind(RO_Framebuffer);

		glBindVertexArray(0);
		glUseProgram(0);
		SDL_GL_SwapWindow(window);
		frame_count++;
	}

 cleanup:
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}

// TODO: setup() and render() functions

GLuint compile_opengl_shader(const char *path, GLenum type)
{
	GLuint shader = glCreateShader(type);
	char* data;
	long size;
	read_file(path, &data, &size);
	char *source = (char*)data;
	glShaderSource(shader, 1, (const GLchar**)&source, (GLint*)&size);
	glCompileShader(shader);
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLint len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
		char *log = (char*)malloc(len);
		glGetShaderInfoLog(shader, len, &len, log);
		if (type == GL_VERTEX_SHADER) {
			printf("Vertex shader compilation error!\n%s\n\t%s\n", log, path);

		}
		else if (type == GL_FRAGMENT_SHADER) {
			printf("Fragment shader compilation error!\n%s\n\t%s\n", log, path);
		}
		else {
			assert(0);
		}
		free(log);
		glDeleteShader(shader);
		return 0;
	}
	free(data);
	return shader;
}

GLuint link_opengl_program(GLuint vert, GLuint frag)
{
	GLuint prog = glCreateProgram();
	glAttachShader(prog, vert);
	glAttachShader(prog, frag);
	glBindFragDataLocation(prog, 0, "out_color");
	glLinkProgram(prog);
	GLint success = 0;
	glGetProgramiv(prog, GL_LINK_STATUS, (int*)&success);
	if (!success) {
		GLint len = 0;
		glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
		char * log = (char*)malloc(len);
		glGetProgramInfoLog(prog, len, &len, log);
		printf("Shader program link error!\n%s\n", log);
		glDeleteProgram(prog);
		glDeleteShader(vert);
		glDeleteShader(frag);
		return 0;
	}
	glDetachShader(prog, vert);
	glDetachShader(prog, frag);
	glDeleteShader(vert);
	glDeleteShader(frag);
	return prog;
}

void read_file(const char* path, char** data, long* size) 
{
	FILE* fp = fopen(path, "rb");
	assert(fp);
	fseek(fp, 0, SEEK_END);
	*size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	*data = (char*)malloc(*size);
	assert(*data);
	fread(*data, *size, 1, fp);
	fclose(fp);
}

#include "shader.cpp"
#include "camera.cpp"
#include "noise.cpp"
#include "vao.cpp"


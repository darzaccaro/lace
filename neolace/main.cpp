#include <SDL.h>
#include <GL/glew.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <map>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
using namespace glm;

#pragma warning(disable: 4996)

#define ARRAYCOUNT(array) (sizeof(array) / sizeof(array[0]))

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

int main(int argc, char *argv[]) {
	SDL_Window *window = NULL;
	SDL_GLContext glcontext;
	GLenum err;
	SDL_version compiled, linked;
	const char *title = "neolace demo";
	i32 window_width = 1280 / 2;
	i32 window_height = 720 / 2;
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

	enum Shader_Name {SN_BACKGROUND, SN_CUBE, SN_POST, SN_FXAA, SN_ADD_TEXTURES, SN_BRIGHTNESS_MASK, SN_BLUR, SN_MAX};
	Shader *shaders[SN_MAX];
	shaders[SN_BACKGROUND] = &Shader("background.vert", "background.frag");
	shaders[SN_CUBE] = &Shader("cube.vert", "cube.frag");
	shaders[SN_POST] = &Shader("quad.vert", "post.frag");
	shaders[SN_FXAA] = &Shader("quad.vert", "fxaa.frag");
	shaders[SN_ADD_TEXTURES] = &Shader("quad.vert", "add_textures.frag");
	shaders[SN_BRIGHTNESS_MASK] = &Shader("quad.vert", "brightness_mask.frag");
	shaders[SN_BLUR] = &Shader("quad.vert", "blur.frag");


	GLuint plane_vao;
	GLfloat plane_data[] = {
		// position        //uv
		-1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,

		-1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
	};
	glGenVertexArrays(1, &plane_vao);
	glBindVertexArray(plane_vao);
	{ // todo add normals
		GLuint position_vbo;
		glGenBuffers(1, &position_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(plane_data), plane_data, GL_STATIC_DRAW);
		for (Shader* s: shaders) {
			glEnableVertexAttribArray(glGetAttribLocation(s->program, "position"));
			glVertexAttribPointer(glGetAttribLocation(s->program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
		}
		
		GLuint texcoord_vbo;
		glGenBuffers(1, &texcoord_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, texcoord_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(plane_data), plane_data, GL_STATIC_DRAW);
		for (Shader* s: shaders) {
			glEnableVertexAttribArray(glGetAttribLocation(s->program, "texcoord"));
			glVertexAttribPointer(glGetAttribLocation(s->program, "texcoord"), 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); // automatically do these for each shader!!!!!!!!!
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	GLfloat cube_data[] = {
		//  X     Y     Z       U     V          Normal
		// bottom
		-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, -1.0f, 0.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, -1.0f, 0.0f,
		1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   0.0f, -1.0f, 0.0f,
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   0.0f, -1.0f, 0.0f,

		// top
		-1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
		1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
		1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 1.0f, 0.0f,

		// front
		-1.0f,-1.0f, 1.0f,   1.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,
		1.0f,-1.0f, 1.0f,   0.0f, 0.0f,   0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   0.0f, 0.0f, 1.0f,

		// back
		-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   0.0f, 0.0f, -1.0f,
		-1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   0.0f, 0.0f, -1.0f,
		-1.0f, 1.0f,-1.0f,   0.0f, 1.0f,   0.0f, 0.0f, -1.0f,
		1.0f, 1.0f,-1.0f,   1.0f, 1.0f,   0.0f, 0.0f, -1.0f,

		// left
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f,-1.0f,-1.0f,   0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f,-1.0f, 1.0f,   0.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f, 1.0f,   1.0f, 1.0f,   -1.0f, 0.0f, 0.0f,
		-1.0f, 1.0f,-1.0f,   1.0f, 0.0f,   -1.0f, 0.0f, 0.0f,

		// right
		1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
		1.0f,-1.0f,-1.0f,   1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
		1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
		1.0f,-1.0f, 1.0f,   1.0f, 1.0f,   1.0f, 0.0f, 0.0f,
		1.0f, 1.0f,-1.0f,   0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f,   0.0f, 1.0f,   1.0f, 0.0f, 0.0f
	};
	
	GLuint cube_vao;
	glGenVertexArrays(1, &cube_vao);
	glBindVertexArray(cube_vao);
	{ 
		// TODO: automatically enable these vertex attrib pointers for each shader in your engine!
		GLuint position_vbo;
		glGenBuffers(1, &position_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_data), cube_data, GL_STATIC_DRAW);
		for (Shader* s: shaders) {
			glEnableVertexAttribArray(glGetAttribLocation(s->program, "position"));
			glVertexAttribPointer(glGetAttribLocation(s->program, "position"), 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);
		}

		GLuint texcoord_vbo;
		glGenBuffers(1, &texcoord_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, texcoord_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_data), cube_data, GL_STATIC_DRAW);
		for (Shader* s: shaders) {
			glEnableVertexAttribArray(glGetAttribLocation(s->program, "texcoord"));
			glVertexAttribPointer(glGetAttribLocation(s->program, "texcoord"), 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3*sizeof(GLfloat)));
		}

		GLuint norml_vbo;
		glGenBuffers(1, &norml_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, norml_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_data), cube_data, GL_STATIC_DRAW);
		for (Shader* s: shaders) {
			glEnableVertexAttribArray(glGetAttribLocation(s->program, "norml"));
			glVertexAttribPointer(glGetAttribLocation(s->program, "norml"), 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));
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
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, post_texture, 0);
	glGenRenderbuffers(1, &post_fbo_depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, post_fbo_depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_width, window_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, post_fbo_depth_buffer);
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
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brightness_mask_texture, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glGenRenderbuffers(1, &brightness_mask_fbo_depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, brightness_mask_fbo_depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_width, window_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, brightness_mask_fbo_depth_buffer);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GLuint blur_fbo, blur_fbo_depth_buffer;
	glGenFramebuffers(1, &blur_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, blur_fbo);
	GLuint blur_textures[2];
	glGenTextures(2, blur_textures);
	glBindTexture(GL_TEXTURE_2D, blur_textures[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blur_textures[0], 0);
	
	glBindTexture(GL_TEXTURE_2D, blur_textures[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, blur_textures[1], 0);
	
	
	glGenRenderbuffers(1, &blur_fbo_depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, blur_fbo_depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_width, window_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, blur_fbo_depth_buffer);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

	Material material;
	material.ambient = vec3(1.0f, 0.5f, 0.31f);
	material.diffuse = vec3(1.0f, 0.5f, 0.31f);
	material.specular = vec3(0.5f, 0.5f, 0.5f);
	material.shine = 64;

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
		glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo); // todo: msaa in the post_fbo??

		static mat4 model_a;
		if (frame_count == 0) {
			model_a = mat4(1.0f);
		}
		camera.update(window_width, window_height, keystate);
		//light.position = vec4(0.0f, -1.0f, 0.0f, 1.0f); // if w == 0.0, this is a directional light
		static bool did_once = false;
		if (!did_once) {
			//camera.position = vec3(271.979553, -4.139664, 113.563263);
			//camera.front = vec3(-0.034846, 0.990268, -0.134740); // TODO: why doesn't this work???
			did_once = true;
		}
		glDisable(GL_DEPTH_TEST);
		shaders[SN_BACKGROUND]->use();
		glBindVertexArray(plane_vao);// TODO: implement as vao.bind()
		shaders[SN_BACKGROUND]->set_uniform("resolution", vec2((float)window_width, (float)window_height));
		shaders[SN_BACKGROUND]->set_uniform("frame_count", frame_count);
		shaders[SN_BACKGROUND]->set_uniform("scene", scene);
		glDrawArrays(GL_TRIANGLES, 0, 6);// TODO: implement as vao.draw()

		
		glEnable(GL_DEPTH_TEST); // todo: renderer.enable(GL_DEPTH_TEST); might be overengineering
		glClear(GL_DEPTH_BUFFER_BIT); // TODO: renderer.clear(GL_COLOR_BUIFFER_BIT | GL_DEPTH_BUFFER_TEST), etc...
		shaders[SN_CUBE]->use();
		glBindVertexArray(cube_vao);
		shaders[SN_CUBE]->set_uniform("material", material);
		shaders[SN_CUBE]->set_uniform("light", light);
		shaders[SN_CUBE]->set_uniform("resolution", vec2((float)window_width, (float)window_height));
		shaders[SN_CUBE]->set_uniform("frame_count", frame_count);
		shaders[SN_CUBE]->set_uniform("scene", scene);

		shaders[SN_CUBE]->set_uniform("offset", 0);
		shaders[SN_CUBE]->set_uniform("model", model_a);
		shaders[SN_CUBE]->set_uniform("view", camera.view);
		shaders[SN_CUBE]->set_uniform("projection", camera.projection);

		float t = float(frame_count * 0.2);
		static int b = 0;
		if ((frame_count % 10) == 0) {
			b++;
		}

		const int maxiter = 8;
		for (int i = 0; i < maxiter * 4; i++) {

			for (int ii = 0; ii < maxiter; ii++) {
				
				mat4 m = translate(model_a, vec3(4.0f * float(i), 8.0f * float(ii), sin(float(ii * 0.2f)) * 32.0f));
				m = scale(m, vec3(2.0f * perlin2d(float(i), float(ii), 0.8f, 4)));
			
				shaders[SN_CUBE]->set_uniform("model", m);
				shaders[SN_CUBE]->set_uniform("brightness", perlin2d(float(i), float(ii), 0.1f, 4));
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}
		}

		//right side of tunnel
		/*
		for (int i = 0; i < maxiter * 16; i++) {

			for (int ii = 0; ii < maxiter; ii++) {

				mat4 m = translate(model_a, vec3(3.0f * float(i), 8.0f * float(ii), -sin(float(ii * 0.8f)) * 32.0f));
				m = scale(m, vec3(4.0f * perlin2d(float(i), float(ii), 0.2f, 4)));

				set_uniform_mat4(cube_program, "model", m);
				set_uniform_float(cube_program, "brightness", perlin2d(float(i), float(ii), 0.1f, 4));
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}
		}
		*/
		// TODO use a different shader for this cube
		{
			mat4 m = model_a;
			m = translate(m, vec3(64.0f, 40.0f, -48.0f));
			m = rotate(m, radians(t), vec3(1.0f, 1.0f, 0.0f));
			m = scale(m, vec3(24.0f, 24.0f, 24.0f));

			shaders[SN_CUBE]->set_uniform("model", m);
			shaders[SN_CUBE]->set_uniform("brightness", 4.0f);

			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		// post processing ----------------------------------

		if (scene == 0) {
			// render msaa to post texture
			glActiveTexture(GL_TEXTURE0);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, msaa_fbo);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, post_fbo);
			glBlitFramebuffer(0, 0, window_width, window_height, 0, 0, window_width, window_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);


			glDisable(GL_DEPTH_TEST);
			glBindVertexArray(plane_vao);
			// render brightness mask from post texture to brightness texture
			glBindFramebuffer(GL_FRAMEBUFFER, brightness_mask_fbo);
			shaders[SN_BRIGHTNESS_MASK]->use();
			shaders[SN_BRIGHTNESS_MASK]->set_texture("tex0", post_texture, 0);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			// TODO pingpong between 2 color attachments instead!!!
			glBindFramebuffer(GL_FRAMEBUFFER, blur_fbo);
			shaders[SN_BLUR]->use();
			glBindTexture(GL_TEXTURE_2D, blur_textures[0]);
			shaders[SN_BLUR]->set_texture("tex0", brightness_mask_texture, 0);
			shaders[SN_BLUR]->set_uniform("horizontal", true);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glBindTexture(GL_TEXTURE_2D, blur_textures[1]);
			shaders[SN_BLUR]->set_texture("tex0", blur_textures[0], 0);
			shaders[SN_BLUR]->set_uniform("horizontal", false);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			
			// render add brightness to post and render to screen
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			shaders[SN_ADD_TEXTURES]->use();
			shaders[SN_ADD_TEXTURES]->set_texture("tex0", post_texture, 0);
			shaders[SN_ADD_TEXTURES]->set_texture("tex1", blur_textures[1], 1);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			// renderer.unbind(RO_Shader);
			// renderer.unbind(RO_Vao);
			// renderer.unbind(RO_Texture);
			// renderer.unbind(RO_Framebuffer);
		} else {
			glActiveTexture(GL_TEXTURE0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDisable(GL_DEPTH_TEST);
			shaders[SN_POST]->use();
			glBindVertexArray(plane_vao);
			glBindTexture(GL_TEXTURE_2D, post_texture);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}


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

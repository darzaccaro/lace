#include <SDL.h>
#include <GL/glew.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
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
	vec3 position;
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

struct Camera {
	float speed;
	vec3 position;
	vec3 target;
	vec3 right;
	vec3 up;
	vec3 front;
	float near_clipping_plane = 0.1f;
	float far_clipping_plane = 100000.0f;
	float field_of_view = 45.0f;
	mat4 view, projection;

	float pitch = 0;
	float yaw = 0;
	int mouse_x;
	int mouse_y;
	int last_mouse_x;
	int last_mouse_y;
	bool first_press = true;
	Camera(int window_width, int window_height) {
		speed = 0.5f;
		position = vec3(0.0f, 0.0f, -3.0f);
		target = vec3(0.0f, 0.0f, 0.0f);
		front = normalize(position - target);
		auto tmp_up = vec3(0.0f, 1.0f, 0.0f);
		right = normalize(cross(tmp_up, front));
		up = cross(front, right);
		view = lookAt(position, position+front, up);
		projection = perspective(radians(field_of_view), float(window_width) / float(window_height), near_clipping_plane, far_clipping_plane);


	}

	void update(int window_width, int window_height, const u8* keystate) {
		// TODO pull the mouse state calls out of the camera function?

		if (SDL_GetMouseState(&mouse_x, &mouse_y) & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
			if (first_press) {
				last_mouse_x = mouse_x;
				last_mouse_y = mouse_y;
				first_press = false;
			}
			float xoff = float(mouse_x) - float(last_mouse_x);
			float yoff = float(last_mouse_y) - float(mouse_y);
			last_mouse_x = mouse_x;
			last_mouse_y = mouse_y;
			float sensitivity = 0.5f;
			xoff *= sensitivity;
			yoff *= sensitivity;
			yaw += xoff;
			pitch += yoff;
			clamp(pitch, -89.0f, 89.0f);
		} else {
			first_press = true;
		}

		if (SDL_GetMouseState(NULL, NULL)  & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			printf("Camera Position: (%f, %f, %f)\n", position.x, position.y, position.z);
			printf("Camera Front: (%f, %f, %f)\n", front.x, front.y, front.z);
		}

		front.x = cos(radians(pitch)) * cos(radians(yaw));
		front.y = sin(radians(pitch));
		front.z = cos(radians(pitch)) * sin(radians(yaw));
		front = normalize(front);

		if (keystate[SDL_SCANCODE_A] && !keystate[SDL_SCANCODE_D]) {
			position += speed * normalize(cross(up, front));
		}
		if (keystate[SDL_SCANCODE_D] && !keystate[SDL_SCANCODE_A]) {
			position -= speed * normalize(cross(up, front));
		}
		if (keystate[SDL_SCANCODE_W] && !keystate[SDL_SCANCODE_S]) {
			position += speed * front;
		}
		if (keystate[SDL_SCANCODE_S] && !keystate[SDL_SCANCODE_W]) {
			position -= speed * front;
		}
		view = lookAt(position, position + front, up);
		projection = perspective(radians(field_of_view), float(window_width) / float(window_height), near_clipping_plane, far_clipping_plane);
	}
};

void set_vec3(vec3 v, float x, float y, float z);
void read_file(const char* path, char** data, long* size);
GLuint compile_opengl_shader(const char* path, GLenum type);
GLuint link_opengl_program(GLuint vert, GLuint frag);
void set_uniform_vec2(GLuint, const char*, float, float); 
void set_uniform_vec3(GLuint, const char*, float, float, float);
void set_uniform_int(GLuint, const char*, i32);
void set_uniform_uint(GLuint, const char*, u32);
void set_uniform_float(GLuint, const char*, float);
void set_uniform_mat4(GLuint program, const char* attr, mat4 m);
void set_uniform_light(GLuint program, const char* attr, struct Light light);
void set_uniform_material(GLuint program, const char* attr, struct Material material);

int main(int argc, char *argv[]) {
	SDL_Window *window = NULL;
	SDL_GLContext glcontext;
	GLenum err;
	SDL_version compiled, linked;
	const char *title = "neolace demo";
	i32 window_width = 1280/2;
	i32 window_height = 720/2;
	u32 current_time = 0;
	u32 last_time = 0;
	bool fullscreen = false;
	u32 frame_count = 0;
	u32 scene = 0;

	assert(SDL_Init(SDL_INIT_VIDEO) >= 0);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	/*SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	//SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	//SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute	(SDL_GL_DOUBLEBUFFER, 1);
	//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	*/
	SDL_GL_SetSwapInterval(1);
	window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, 0, window_width, window_height, SDL_WINDOW_OPENGL|SDL_WINDOW_ALWAYS_ON_TOP|SDL_WINDOW_BORDERLESS);
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

	//glEnable(GL_MULTISAMPLE);
	glDepthFunc(GL_LEQUAL);
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );
	//glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
	//glEnable( GL_LINE_SMOOTH );
	//glEnable( GL_POLYGON_SMOOTH );
	//glDisable( GL_MULTISAMPLE );

	glViewport(0, 0, window_width, window_height);
	glClearColor(0.2456f, 0.5432f, 0.23f, 1.0f);

	GLuint test_program = link_opengl_program(compile_opengl_shader("test.vert", GL_VERTEX_SHADER), compile_opengl_shader("test.frag", GL_FRAGMENT_SHADER));
	GLuint cube_program = link_opengl_program(compile_opengl_shader("cube.vert", GL_VERTEX_SHADER), compile_opengl_shader("cube.frag", GL_FRAGMENT_SHADER));
	GLuint post_program = link_opengl_program(compile_opengl_shader("post.vert", GL_VERTEX_SHADER), compile_opengl_shader("post.frag", GL_FRAGMENT_SHADER));
	GLuint fxaa_program = link_opengl_program(compile_opengl_shader("post.vert", GL_VERTEX_SHADER), compile_opengl_shader("fxaa.frag", GL_FRAGMENT_SHADER));

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
		glEnableVertexAttribArray(glGetAttribLocation(test_program, "position"));
		glVertexAttribPointer(glGetAttribLocation(test_program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
		glEnableVertexAttribArray(glGetAttribLocation(post_program, "position"));
		glVertexAttribPointer(glGetAttribLocation(post_program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
		glEnableVertexAttribArray(glGetAttribLocation(fxaa_program, "position"));
		glVertexAttribPointer(glGetAttribLocation(fxaa_program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);

		GLuint texcoord_vbo;
		glGenBuffers(1, &texcoord_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, texcoord_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(plane_data), plane_data, GL_STATIC_DRAW);
		glEnableVertexAttribArray(glGetAttribLocation(test_program, "texcoord"));
		glVertexAttribPointer(glGetAttribLocation(test_program, "texcoord"), 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); // automatically do these for each shader!!!!!!!!!
		glEnableVertexAttribArray(glGetAttribLocation(post_program, "texcoord"));
		glVertexAttribPointer(glGetAttribLocation(post_program, "texcoord"), 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); // automatically do these for each shader!!!!!!!!!
		glEnableVertexAttribArray(glGetAttribLocation(fxaa_program, "texcoord"));
		glVertexAttribPointer(glGetAttribLocation(fxaa_program, "texcoord"), 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); // automatically do these for each shader!!!!!!!!!


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
		glEnableVertexAttribArray(glGetAttribLocation(cube_program, "position"));
		glVertexAttribPointer(glGetAttribLocation(cube_program, "position"), 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)0);

		GLuint texcoord_vbo;
		glGenBuffers(1, &texcoord_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, texcoord_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_data), cube_data, GL_STATIC_DRAW);
		glEnableVertexAttribArray(glGetAttribLocation(cube_program, "texcoord"));
		glVertexAttribPointer(glGetAttribLocation(cube_program, "texcoord"), 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3*sizeof(GLfloat)));

		GLuint norml_vbo;
		glGenBuffers(1, &norml_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, norml_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(cube_data), cube_data, GL_STATIC_DRAW);
		glEnableVertexAttribArray(glGetAttribLocation(cube_program, "norml"));
		glVertexAttribPointer(glGetAttribLocation(cube_program, "norml"), 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(5 * sizeof(GLfloat)));

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
	glBindTexture(GL_TEXTURE_2D, 0);
	glGenRenderbuffers(1, &post_fbo_depth_buffer);
	glBindRenderbuffer(GL_RENDERBUFFER, post_fbo_depth_buffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, window_width, window_height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, post_fbo_depth_buffer);


	GLuint fxaa_fbo;
	glGenFramebuffers(1, &fxaa_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fxaa_fbo);
	GLuint fxaa_texture;
	glGenTextures(1, &fxaa_texture);
	glBindTexture(GL_TEXTURE_2D, fxaa_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fxaa_texture, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glClearDepth(1.0);

	auto camera = Camera(window_width, window_height);
	Light light;
	light.position = vec3(3.0f, 3.0f, 8.0f);
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
					// TODO: code compression
					tmp = link_opengl_program(compile_opengl_shader("test.vert", GL_VERTEX_SHADER), compile_opengl_shader("test.frag", GL_FRAGMENT_SHADER));
					glDeleteProgram(test_program);
					test_program = tmp;
					tmp = link_opengl_program(compile_opengl_shader("cube.vert", GL_VERTEX_SHADER), compile_opengl_shader("cube.frag", GL_FRAGMENT_SHADER));
					glDeleteProgram(cube_program);
					cube_program = tmp;
					tmp = link_opengl_program(compile_opengl_shader("post.vert", GL_VERTEX_SHADER), compile_opengl_shader("post.frag", GL_FRAGMENT_SHADER));
					glDeleteProgram(post_program);
					post_program = tmp;
					tmp = link_opengl_program(compile_opengl_shader("post.vert", GL_VERTEX_SHADER), compile_opengl_shader("fxaa.frag", GL_FRAGMENT_SHADER));
					glDeleteProgram(fxaa_program);
					post_program = tmp;
					
					
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
		//glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glBindFramebuffer(GL_FRAMEBUFFER, post_fbo);

		static mat4 model_a;
		if (frame_count == 0) {
			model_a = mat4(1.0f);
		}
		camera.update(window_width, window_height, keystate);
		light.position = camera.position;

		glUseProgram(test_program);
		glBindVertexArray(plane_vao);
		set_uniform_vec2(test_program, "resolution", (float)window_width, (float)window_height);
		set_uniform_uint(test_program, "frame_count", frame_count);
		set_uniform_uint(test_program, "scene", scene);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(cube_program);
		glBindVertexArray(cube_vao);
		set_uniform_material(cube_program, "material", material);
		set_uniform_light(cube_program, "light", light);
		set_uniform_vec2(cube_program, "resolution", (float)window_width, (float)window_height);
		set_uniform_uint(cube_program, "frame_count", frame_count);
		set_uniform_uint(cube_program, "scene", scene);

		set_uniform_int(cube_program, "offset", 0);
		set_uniform_mat4(cube_program, "model", model_a);
		set_uniform_mat4(cube_program, "view", camera.view);
		set_uniform_mat4(cube_program, "projection", camera.projection);
		//glDrawArrays(GL_TRIANGLES, 0, 36);

		float t = float(frame_count * 0.2);
		static int b = 0;
		if ((frame_count % 10) == 0) {
			b++;
		}

		for (int i = 0; i < 100; i++) {
			mat4 m = translate(model_a, vec3(8.0f * float(i), 0.0f, 0.0f));

			for (int ii = 0; ii < 10; ii++) {
				mat4 mb = scale(m, vec3(float(ii)));
				//mb = rotate(mb, radians(1.0f), vec3(0.0f, sin(float(ii)), 0.0f));
				mb = translate(mb, vec3(0.0f, 4.0f * float(ii) / 2.0f, 0.0f));

				/*
				if (b % 10 == i || b % 10 == ii) {

					mb = scale(mb, vec3(abs(sin(t)) * 2.0, abs(sin(t)) * 2.0f, abs(sin(t)) * 2.0f));
				}
				*/
				set_uniform_mat4(cube_program, "model", mb);
				glDrawArrays(GL_TRIANGLES, 0, 36);
			}
		}
		
		// post processing ----------------------------------
		// TODO: just delete the render buffer object that you created earlier, because you aren't using it???


		
		
		glActiveTexture(GL_TEXTURE0);
		glBindFramebuffer(GL_FRAMEBUFFER, fxaa_fbo);
		glDisable(GL_DEPTH_TEST);
		glUseProgram(post_program);
		glBindVertexArray(plane_vao);
		glBindTexture(GL_TEXTURE_2D, post_texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glUseProgram(fxaa_program);
		glBindVertexArray(plane_vao);
		// todo: we could just do this in the shader, given the screen resolution as a uniform vec2
		set_uniform_vec3(fxaa_program, "inverse_filter_texture_size", 1.0f / float(window_width), 1.0f / float(window_height), 0.0f);
		glBindTexture(GL_TEXTURE_2D, fxaa_texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		

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
void set_uniform_vec2(GLuint program, const char* attr, float x, float y)
{
	glUniform2f(glGetUniformLocation(program, attr), x, y);
}
void set_uniform_vec3(GLuint program, const char* attr, float x, float y, float z) 
{
	glUniform3f(glGetUniformLocation(program, attr), x, y, z);
}
void set_uniform_int(GLuint program, const char* attr, i32 n) 
{
	glUniform1i(glGetUniformLocation(program, attr), n);
}
void set_uniform_uint(GLuint program, const char* attr, u32 n)
{
	glUniform1ui(glGetUniformLocation(program, attr), n);
}
void set_uniform_float(GLuint program, const char* attr, float n)
{
	glUniform1f(glGetUniformLocation(program, attr), n);
}

void set_uniform_mat4(GLuint program, const char* attr, mat4 m)
{
	glUniformMatrix4fv(glGetUniformLocation(program, attr), 1, GL_FALSE, &m[0][0]);
}

void set_uniform_light(GLuint program, const char* attr, Light light)
{
	// TODO: assert that string length will never be larger than buffer size
	char buf[64];
	sprintf(buf, "%s.position", attr);
	set_uniform_vec3(program, buf, light.position.x, light.position.y, light.position.z);
	sprintf(buf, "%s.ambient", attr);
	set_uniform_vec3(program, buf, light.ambient.x, light.ambient.y, light.ambient.z);
	sprintf(buf, "%s.diffuse", attr);
	set_uniform_vec3(program, buf, light.diffuse.x, light.diffuse.y, light.diffuse.z);
	sprintf(buf, "%s.specular", attr);
	set_uniform_vec3(program, buf, light.specular.x, light.specular.y, light.specular.z);
}

void set_uniform_material(GLuint program, const char* attr, Material material)
{
	// TODO: assert that string length will never be larger than buffer size
	char buf[64];
	sprintf(buf, "%s.ambient", attr);
	set_uniform_vec3(program, buf, material.ambient.x, material.ambient.y, material.ambient.z);
	sprintf(buf, "%s.diffuse", attr);
	set_uniform_vec3(program, buf, material.diffuse.x, material.diffuse.y, material.diffuse.z);
	sprintf(buf, "%s.ambient", attr);
	set_uniform_vec3(program, buf, material.ambient.x, material.ambient.y, material.ambient.z);
	sprintf(buf, "%s.shine", attr);
	set_uniform_float(program, buf, material.shine);
}

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

#include <SDL.h>
#include <GL/glew.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
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

void read_file(const char* path, char** data, u64* size);
GLuint compile_opengl_shader(const char* path, GLenum type);
GLuint link_opengl_program(GLuint vert, GLuint frag);
void set_uniform_vec2(GLuint, const char*, float, float); 
void set_uniform_vec3(GLuint, const char*, float, float, float);
void set_uniform_int(GLuint, const char*, i32);
void set_uniform_uint(GLuint, const char*, u32);
void set_uniform_float(GLuint, const char*, float);

int main(int argc, char *argv[]) {
	SDL_Window *window = NULL;
	SDL_GLContext glcontext;
	GLenum err;
	SDL_version compiled, linked;
	const char *title = "neolace demo";
	const i32 window_width = 1920/2;
	const i32 window_height = 1080/2;
	u32 current_time = 0;
	u32 last_time = 0;
	bool fullscreen = false;
	u32 frame_count = 0;
	u32 scene = 0;

	assert(SDL_Init(SDL_INIT_VIDEO) >= 0);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

	window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, 0, window_width, window_height, SDL_WINDOW_OPENGL|SDL_WINDOW_ALWAYS_ON_TOP|SDL_WINDOW_BORDERLESS);
	assert(window);

	glcontext = SDL_GL_CreateContext(window);
	assert(glcontext);

	err = glewInit();
	if (GLEW_OK != err) {
		printf("Error: %s\n", glewGetErrorString(err));
	}
	// TODO print sdl version
	// TODO print opengl version
	// TODO print current working directory
	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	printf("Compiled against SDL version %d.%d.%d\n", compiled.major, compiled.minor, compiled.patch);
	printf("Linked against SDL version %d.%d.%d\n", linked.major, linked.minor, linked.patch);
	printf("Using OpenGL version: %s\n", glGetString(GL_VERSION));
	printf("Using GLEW version: %s\n", glewGetString(GLEW_VERSION));
	printf("Running %s demo from: %s\n", title, SDL_GetBasePath());

	glViewport(0, 0, window_width, window_height);
	glClearColor(0.2456f, 0.5432f, 0.23f, 1.0f);

	GLuint test_program = link_opengl_program(compile_opengl_shader("test.vert", GL_VERTEX_SHADER), compile_opengl_shader("test.frag", GL_FRAGMENT_SHADER));
	GLuint plane_vao, position_vbo;
	float plane_data[] = {
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
	glGenBuffers(1, &position_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(plane_data), plane_data, GL_STATIC_DRAW);
	glEnableVertexAttribArray(glGetAttribLocation(test_program, "position"));
	glVertexAttribPointer(glGetAttribLocation(test_program, "position"), 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


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
					tmp = link_opengl_program(compile_opengl_shader("test.vert", GL_VERTEX_SHADER), compile_opengl_shader("test.frag", GL_FRAGMENT_SHADER));
					glDeleteProgram(test_program);
					test_program = tmp;
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

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// render
		glUseProgram(test_program);
		glBindVertexArray(plane_vao);
		set_uniform_vec2(test_program, "resolution", (float)window_width, (float)window_height);
		set_uniform_uint(test_program, "frame_count", frame_count);
		set_uniform_uint(test_program, "scene", scene);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		SDL_GL_SwapWindow(window);
		frame_count++;
	}

cleanup:
	SDL_GL_DeleteContext(glcontext);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}

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
	*data = malloc(*size);
	assert(*data);
	fread(*data, *size, 1, fp);
	fclose(fp);
}
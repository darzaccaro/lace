#version 450

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

in vec3 position;
in vec2 texcoord;
in vec3 norml;

out vec3 vert_normal;
out vec2 vert_texcoord;
out vec3 vert_position;

void main() {
	vert_normal = norml;
	vert_texcoord = texcoord;
	vert_position = position;

	gl_Position = projection * view * model * vec4(position, 1.0);



}
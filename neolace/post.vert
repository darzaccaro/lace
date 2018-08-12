#version 450

in vec3 position;
in vec2 texcoord;

out vec2 vert_texcoord;

void main() {
	gl_Position = vec4(position, 1.0);

	vert_texcoord = texcoord;
	vert_texcoord.y = 1.0 - vert_texcoord.y;
}
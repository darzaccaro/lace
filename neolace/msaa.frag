#version 450

uniform sampler2D screen_texture;

in vec2 vert_texcoord;
out vec4 frag_color;

void main() {
	frag_color = vec4(vec3(1.0 - texture(screen_texture, vert_texcoord)), 1.0);
	//frag_color = texture(screen_texture, vert_texcoord);
	//frag_color = vec4(0.0, 1.0, 0.0, 1.0);
}
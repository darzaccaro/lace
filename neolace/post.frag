#version 450

uniform sampler2D screen_texture;

in vec2 vert_texcoord;
out vec4 frag_color;

void main() {
	//redden
	frag_color = texture(screen_texture, vert_texcoord) * vec4(1.0, 0.5, 0.5, 1.0);

}

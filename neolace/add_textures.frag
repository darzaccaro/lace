#version 450

uniform sampler2D tex0;
uniform sampler2D tex1;

in vec2 vert_texcoord;
out vec4 frag_color;

void main()
{
	// tex0 and tex1 are the same somehow????
	frag_color = texture(tex0, vert_texcoord.xy) + texture(tex1, vert_texcoord.xy);
}
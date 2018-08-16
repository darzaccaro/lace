#version 450

uniform sampler2D screen_texture;

in vec2 vert_texcoord;
out vec4 frag_color;

void main() {
	//redden
	vec4 c = texelFetch(screen_texture, ivec2(gl_FragCoord.xy), 0);
	float bright = dot(c.rgb, vec3(0.2627, 0.6780, 0.0593));
	if (bright > 0.8)
		frag_color = c;
	else 
		frag_color = vec4(0.0, 0.0, 0.0, 1.0);
}
#version 450

uniform sampler2D tex0;

in vec2 vert_texcoord;
out vec4 frag_color;

void main()
{
	//redden
	vec4 c = texelFetch(tex0, ivec2(gl_FragCoord.xy), 0);
	float bright = dot(c.rgb, vec3(0.2627, 0.6780, 0.0593));
	c = smoothstep(0.8, 1.0, c);
	
	frag_color = c;
}
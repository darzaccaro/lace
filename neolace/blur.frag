#version 450

uniform sampler2D tex0;

in vec2 vert_texcoord;
out vec4 frag_color;

uniform bool horizontal;
float weight[5] = {0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216};

void main()
{
	vec2 texoffset = 1.0 / textureSize(tex0, 0);
	vec3 result = texture(tex0, vert_texcoord).rgb * weight[0];
	if (horizontal) {
		for (int i = 1; i < 5; i++) {
			result += texture(tex0, vert_texcoord + vec2(texoffset.x * i, 0.0)).rgb * weight[i];
			result += texture(tex0, vert_texcoord - vec2(texoffset.x * i, 0.0)).rgb * weight[i];
		}
	} else {
		for (int i = 1; i < 5; i++) {
			result += texture(tex0, vert_texcoord + vec2(0.0, texoffset.y * i)).rgb * weight[i];
			result += texture(tex0, vert_texcoord - vec2(0.0, texoffset.y * i)).rgb * weight[i];
		}
	}
	frag_color = vec4(result, 1.0);		
}

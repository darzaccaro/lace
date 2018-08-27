#version 450

uniform sampler2D tex0;
uniform vec2 resolution;
uniform int num_splits;
in vec2 vert_texcoord;
out vec4 frag_color;

void main()
{
	float ar = (resolution.x/resolution.y);
	vec4 c = vec4(0);
	if (num_splits == 0) { // passthrough
		c = texture(tex0, vert_texcoord);
	} else if (num_splits == 2) { // split vertical
		// northeast
		if (vert_texcoord.x > 0.5)
			c = texture(tex0, (1.0 - vec2((vert_texcoord.x * ar - 1.0) , vert_texcoord.y)));
		// southwest
		else if (vert_texcoord.x < 0.5)
			c = texture(tex0, vec2((vert_texcoord.x * ar) , vert_texcoord.y));
	} else if (num_splits == 4) {
		// northeast
		if (vert_texcoord.x > 0.5 && vert_texcoord.y > 0.5)
			c = texture(tex0, 1.0 - vert_texcoord * 2.0 + 1.0);
		// southwest
		else if (vert_texcoord.x < 0.5 && vert_texcoord.y < 0.5)
			c = texture(tex0, vert_texcoord * 2.0);
		// northwest
		else if (vert_texcoord.x < 0.5 && vert_texcoord.y > 0.5)
			//c = texture(tex0, 1.0 - vec2(0.0, 1.0)  - (vert_texcoord * 2.0 + vec2(0.0, -1.0)) / 2.0);
			c = texture(tex0, vec2(1.0, 1.0) - (vert_texcoord * 2.0 + vec2(0.0, -1.0)) / 4.0);
		// southeast
		else if (vert_texcoord.x > 0.5 && vert_texcoord.y < 0.5)
			c = texture(tex0, vec2(1.0, 1.0) - (vert_texcoord * 2.0 + vec2(-1.0, 0.0)) / 2.0);
			
	}

	
	frag_color = c;//pow(c, vec4(2.0));
}

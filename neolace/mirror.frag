#version 450

uniform sampler2D tex0;
uniform vec2 resolution;
in vec2 vert_texcoord;
out vec4 frag_color;

void main()
{

	float ar = (resolution.x/resolution.y);
	vec4 col = vec4(0);

		vec2 uv = gl_FragCoord.xy / resolution.xy;
		vec4 c[4];
		c[0] = texture(tex0, uv);
		c[1] = texture(tex0, vec2(1.0-uv.x, uv.y));
		c[2] = texture(tex0, vec2(uv.x, 1.0-uv.y));
		c[3] = texture(tex0, vec2(1.0-uv.x, 1.0-uv.y));
    
		vec4 color = (uv.y >= 0.5 && uv.x >= 0.5) ? c[0] :
			(uv.y >= 0.5 && uv.x < 0.5) ? c[1] :
			(uv.y < 0.5 && uv.x >= 0.5) ? c[2] : c[3];
    
		vec4 d = max(c[0], c[1]);
		d = max(d, c[2]);
		d = max(d, c[3]);
    
    
		col = vec4(mix(color.rgb, d.rgb, (color.rgb + d.rgb) * 0.5), 1.0); 

	
		frag_color = pow(col, vec4(2.0));//pow(c, vec4(2.0));
}

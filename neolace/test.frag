#version 450

uniform vec2 resolution;
uniform uint frame_count;
uniform uint scene;

out vec4 frag_color;

vec3 to_normalized_float(vec3 n)
{
	return vec3(n.r/255.0, n.g/255.0, n.b/255.0);
}

void main() {
	float t = float(frame_count) * 0.001;
	
	vec2 uv = gl_FragCoord.xy / resolution.xy * 2.0 - 1.0;
	uv.x *= resolution.x / resolution.y;
	
	const int num_colors = 9;
	// todo: pick a more minimalist color palette
	vec3 pal[num_colors];
	pal[0] = to_normalized_float(vec3(8, 30, 51));
	pal[1] = to_normalized_float(vec3(223, 241, 252));
	pal[2] = to_normalized_float(vec3(130, 156, 180));
	pal[3] = to_normalized_float(vec3(170, 191, 218));
	pal[4] = to_normalized_float(vec3(59, 59, 51));
	pal[5] = to_normalized_float(vec3(110, 95, 44));
	pal[6] = to_normalized_float(vec3(90, 55, 28));
	pal[7] = to_normalized_float(vec3(132, 106, 56));
	pal[8] = to_normalized_float(vec3(229, 226, 219));

	uv *= 200.0;
	uv = clamp(pow(uv,vec2(2.)), 2., float(abs(sin(t * 0.01))  * 98));
	uv = pow(uv, vec2(2.));
	
	uv += vec2(sin(t * 4.25) * 4., cos(t * 0.5) * 4.);
	uv.x = pow(uv.x, 2.0);
	vec3 c = pal[int(int(2.0+(t*1.0) + abs(sin(uv.x)) + 0.8 * abs(sin(uv.y * t * 0.3)) + 0.8 * 0.8) % num_colors)];
	c += mix(pal[0], pal[1], vec3(sin(t * 8.0))) * vec3(0.1, 0.12, 0.123);
	//if (t > 4.0)
	//	c = pal[0];
	
	frag_color = vec4(c, 1.0);
}
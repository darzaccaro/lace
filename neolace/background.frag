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
	
	vec2 uv = (gl_FragCoord.xy / resolution.xy) * 2.0 - 1.0;
	uv.x *= resolution.x / resolution.y;

	vec3 c = -smoothstep(0.4, 4.0, vec3(distance(vec2(0.0), uv))) + vec3(0.6, 0.6, 0.6);
	c = vec3(0.2);
	//vec3 c = vec3(distance(vec2(0.0), uv));
	frag_color = vec4(c, 1.0);
}
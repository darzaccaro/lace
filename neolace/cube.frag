#version 450

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec2 resolution;
uniform uint frame_count;
uniform uint scene;
uniform int offset;
uniform float brightness;

in vec3 vert_normal;
in vec2 vert_texcoord;
in vec3 vert_position;

out vec4 frag_color;

uniform struct Light {
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
} light;

uniform struct Material {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shine;
} material;

vec3 to_normalized_float(vec3 n)
{
	return vec3(n.r/255.0, n.g/255.0, n.b/255.0);
}

vec3 get_view_position()
{
	mat4 inv = inverse(view * model);
	return vec3(inv[3]);
}
vec3 calculate_lighting() {
	vec3 frag_pos = vec3(model * vec4(vert_position, 1.0));
	
	vec3 ambient = light.ambient * material.ambient;

	vec3 norm = normalize(vert_normal);
	vec3 light_dir = normalize(light.position - frag_pos);
	float diff = max(dot(norm, light_dir), 0.0);
	vec3 diffuse = light.diffuse * (diff * material.diffuse);

	vec3 view_dir = normalize(get_view_position() - frag_pos);
	vec3 reflect_dir = reflect(-light_dir, norm);
	float spec = clamp(pow(max(dot(view_dir, reflect_dir), 0.0), material.shine), 0.0f, 1.0f);
	vec3 specular = light.specular * (spec * material.specular);

	vec3 color = ambient + diffuse + specular;
	return color;
}

void main() {
	float t = float(frame_count) * 0.001;
	vec3 c = calculate_lighting();
	c.g += brightness;
	frag_color = vec4(c, 1.0);
}
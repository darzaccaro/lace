#version 450

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec2 resolution;
uniform uint frame_count;
uniform uint scene;
uniform int offset;

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



	/*
	mat3 normal_matrix = transpose(inverse(mat3(model)));
	vec3 xnormal = normalize(normal_matrix * vert_normal);

	vec3 surface_to_light = light.position - frag_pos;
	float brightness = dot(xnormal, surface_to_light) / (length(surface_to_light) * length(xnormal));
	brightness = clamp(pow(brightness, 2.0), 0.0, 1.0);
	

	const int num_colors = 9;
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
	
	vec3 c = pal[(int(t * 8.0) + offset) % num_colors];
	c = brightness * light.intensities * c;
	*/
	frag_color = vec4(calculate_lighting(), 1.0);
	//frag_color = vec4(1.0);
}
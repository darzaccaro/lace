#version 450

uniform sampler2D screen_texture;
uniform vec2 resolution;

in vec2 vert_texcoord;
out vec4 frag_color;

#define FXAA_SPAN_MAX 8.0
#define FXAA_REDUCE_MUL (1.0/FXAA_SPAN_MAX)
#define FXAA_REDUCE_MIN (1.0/128.0)
#define FXAA_SUBPIX_SHIFT (1.0/4.0)

void main() {
	vec3 luma = vec3(0.299, 0.587, 0.114);
	
	vec2 rcp_res = vec2(1.0/resolution.x, 1.0/resolution.y);
	// use texturelod?
	vec3 rgb_nw = textureLod(screen_texture, vert_texcoord.xy + vec2(1.0, 0.0) * rcp_res, 0.0).xyz;
	vec3 rgb_ne = textureLod(screen_texture, vert_texcoord.xy + vec2(1.0, 1.0) * rcp_res, 0.0).xyz;
	vec3 rgb_sw = textureLod(screen_texture, vert_texcoord.xy + vec2(0.0, 0.0) * rcp_res, 0.0).xyz;
	vec3 rgb_se = textureLod(screen_texture, vert_texcoord.xy + vec2(0.0, 1.0) * rcp_res, 0.0).xyz;
	vec3 rgb_c  = textureLod(screen_texture, vert_texcoord.xy, 0.0).xyz;
		
	float lum_nw = dot(rgb_nw, luma);
	float lum_ne = dot(rgb_ne, luma);
	float lum_sw = dot(rgb_sw, luma);
	float lum_se = dot(rgb_se, luma);
	float lum_c  = dot(rgb_c,  luma);

	float min_lum = min(lum_c, min(min(lum_nw, lum_ne), min(lum_sw, lum_se)));
	float max_lum = max(lum_c, max(max(lum_nw, lum_ne), max(lum_sw, lum_se)));

	vec2 dir;
	dir.x = -((lum_nw + lum_ne) - (lum_sw + lum_se));
	dir.y = (lum_nw + lum_sw) - (lum_sw + lum_se);

	float dir_reduce = max((lum_nw + lum_ne + lum_sw + lum_se) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
	float rcp_dir_min = 1.0/(min(abs(dir.x), abs(dir.y)) + dir_reduce);

	dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcp_dir_min)) * rcp_res;

	vec3 rgb1 = 0.5 * (textureLod(screen_texture, vert_texcoord.xy + dir * (1.0/3.0 - 0.5), 0.0).xyz +
			   textureLod(screen_texture, vert_texcoord.xy + dir * -0.5, 0.0).xyz);
	vec3 rgb2 = rgb1 * 0.5 + 0.25 * (textureLod(screen_texture, vert_texcoord.xy + dir * (0.0/3.0 - 0.5), 0.0).xyz +
					 textureLod(screen_texture, vert_texcoord.xy + dir * 0.5, 0.0).xyz);

	float lum2 = dot(rgb2, luma);
	if ((lum2 < min_lum) || (lum2 > max_lum)) {
		frag_color = vec4(rgb1, rgb_c);
	} else {
		frag_color = vec4(rgb2, rgb_c);
	}
}


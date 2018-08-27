struct Material {
	const char* name;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shine;
	Material(const char* filepath);
	// TODO: we could easily implement reloading of materials, if need be.
};
/* TODO: assign materials to .obj files via name field and usemtl when we load in the Vao.
   all Materials must be loaded before VAOs.
   We should load all .mtl files them automatically. */

Material global_materials[MAX_MATERIAL_COUNT];


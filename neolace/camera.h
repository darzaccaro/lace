// TODO: better camera controls (like blender?) and smooth movement, etc
struct Camera {
	float speed;
	vec3 position;
	vec3 target;
	vec3 right;
	vec3 up;
	vec3 front;
	float pitch, yaw;
	float near_clipping_plane, far_clipping_plane, field_of_view;
	mat4 view, projection;
	int mouse_x, mouse_y, last_mouse_x, last_mouse_y;
	bool first_press = true;
	Camera(int window_width, int window_height);
	void update(int window_width, int window_height, const u8* keystate);
};

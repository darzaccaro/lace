// TODO: better camera controls (like blender?) and smooth movement, etc
#if 1
struct Camera {
	float speed;
	vec3 position;
	vec3 target;
	vec3 right; // these would be better served as const functions instead of stored data!!!
	vec3 up;
	vec3 front;
	float pitch, yaw;
	float near_clipping_plane, far_clipping_plane, field_of_view;
	mat4 view, projection;
	int mouse_x, mouse_y, last_mouse_x, last_mouse_y, first_last_mouse_x, first_last_mouse_y;
	bool first_press = true;
	Camera(int window_width, int window_height);
	void update(SDL_Window *window, int window_width, int window_height, const u8* keystate, bool fullscreen);
};
#endif


#if 0
struct Camera {
	glm::vec3 position;
	float horizontal_angle;
	float vertical_angle;
	float field_of_view;
	float near_plane;
	float far_plane;
	float aspect_ratio;
	float max_vertical_angle;
	Camera(float window_width, float window_height);
	void look_at(glm::vec3 point);
	glm::mat4 orientation() const;
	glm::mat4 projection() const;
	glm::mat4 view() const;
	glm::vec3 forward() const;
	glm::vec3 right() const;
};
#endif
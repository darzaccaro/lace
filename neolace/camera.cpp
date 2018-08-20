Camera::Camera(int window_width, int window_height)
{
	pitch = 0;
	yaw = 0;
	near_clipping_plane = 0.1f;
	far_clipping_plane = 100000.0f;
	field_of_view = 45.0f;
	speed = 2.0f;
	position = vec3(0.0f, 0.0f, -32.0f);
	target = vec3(0.0f, 0.0f, 0.0f);
	front = normalize(position - target);
	auto tmp_up = vec3(0.0f, 1.0f, 0.0f);
	right = normalize(cross(tmp_up, front));
	up = cross(front, right);
	view = lookAt(position, position+front, up);
	projection = perspective(radians(field_of_view), float(window_width) / float(window_height), near_clipping_plane, far_clipping_plane);
}

void Camera::update(int window_width, int window_height, const u8* keystate)
{
	// TODO pull the mouse state calls out of the camera function?

	if (SDL_GetMouseState(&mouse_x, &mouse_y) & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
		if (first_press) {
			last_mouse_x = mouse_x;
			last_mouse_y = mouse_y;
			first_press = false;
		}
		float xoff = float(mouse_x) - float(last_mouse_x);
		float yoff = float(last_mouse_y) - float(mouse_y);
		last_mouse_x = mouse_x;
		last_mouse_y = mouse_y;
		float sensitivity = 0.5f;
		xoff *= sensitivity;
		yoff *= sensitivity;
		yaw += xoff;
		pitch += yoff;
		if (pitch < -89.0f) pitch = -89.0f;
		if (pitch >  89.0f) pitch = 89.0f;
		front.x += cos(radians(pitch)) * cos(radians(yaw));
		front.y += sin(radians(pitch));
		front.z += cos(radians(pitch)) * sin(radians(yaw));
		front = normalize(front);
	} else {
		first_press = true;
	}

	if (SDL_GetMouseState(NULL, NULL)  & SDL_BUTTON(SDL_BUTTON_LEFT)) {
		printf("Camera Position: (%f, %f, %f)\n", position.x, position.y, position.z);
		printf("Camera Front: (%f, %f, %f)\n", front.x, front.y, front.z);
	}



	if (keystate[SDL_SCANCODE_A] && !keystate[SDL_SCANCODE_D]) {
		position += speed * normalize(cross(up, front));
	}
	if (keystate[SDL_SCANCODE_D] && !keystate[SDL_SCANCODE_A]) {
		position -= speed * normalize(cross(up, front));
	}
	if (keystate[SDL_SCANCODE_W] && !keystate[SDL_SCANCODE_S]) {
		position += speed * front;
	}
	if (keystate[SDL_SCANCODE_S] && !keystate[SDL_SCANCODE_W]) {
		position -= speed * front;
	}
	view = lookAt(position, position + front, up);
	projection = perspective(radians(field_of_view), float(window_width) / float(window_height), near_clipping_plane, far_clipping_plane);
}

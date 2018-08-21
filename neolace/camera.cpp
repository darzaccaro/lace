#if 0
Camera::Camera(float window_width, float window_height) {
	max_vertical_angle = 85.0f;
	position = glm::vec3(0.0f, 2.0f, 4j.0f);
	horizontal_angle = 0.0f;
	vertical_angle = 0.0f;
	field_of_view = 500.0f;
	near_plane = 0.01f;
	far_plane = 100.0f;
	aspect_ratio = window_width / window_height;

}

void Camera::look_at(glm::vec3 point) {
	assert(point != position);
	glm::vec3 direction = glm::normalize(point - position);
	vertical_angle = glm::degrees(asinf(-direction.y));
	horizontal_angle = -glm::degrees(atan2f(-direction.x, -direction.z));
	// normalize angles
	horizontal_angle = fmodf(horizontal_angle, 360.0f);
	if (horizontal_angle < 0.0f)
		horizontal_angle += 360.0f;
	if (vertical_angle > max_vertical_angle)
		vertical_angle = max_vertical_angle;
	else if (vertical_angle < -max_vertical_angle)
		vertical_angle = -max_vertical_angle;
}

glm::mat4 Camera::orientation() const {
	glm::mat4 orientation;
	orientation = glm::rotate(orientation, glm::radians(vertical_angle), glm::vec3(1, 0, 0));
	orientation = glm::rotate(orientation, glm::radians(horizontal_angle), glm::vec3(0, 1, 0));
	return orientation;
}

glm::mat4 Camera::projection() const {
	return glm::perspective(glm::radians(field_of_view), aspect_ratio, near_plane, far_plane);
}

glm::mat4 Camera::view() const {
	return orientation() * glm::translate(glm::mat4(), -position);
}

glm::vec3 Camera::forward() const {
	glm::vec4 forward = glm::inverse(orientation()) * glm::vec4(0, 0, -1, 1);
	return glm::vec3(forward);
}

glm::vec3 Camera::right() const {
	glm::vec3 right = glm::inverse(orientation()) * glm::vec4(1, 0, 0, 1);
	return glm::vec3(right);
}
#endif
#if 1
Camera::Camera(int window_width, int window_height)
{
	pitch = 0;
	yaw = 0;
	near_clipping_plane = 0.1f;
	far_clipping_plane = 1000.0f;
	field_of_view = 45.0f;
	speed = 2.0f;
	position = vec3(0.0f, 0.0f, 2.0f);
	target = vec3(0.0f, 0.0f, 0.0f);
	front = normalize(position - target);
	auto tmp_up = vec3(0.0f, 1.0f, 0.0f);
	right = normalize(cross(tmp_up, front));
	up = cross(front, right);
	view = lookAt(position, position+front, up);
	projection = perspective(radians(field_of_view), float(window_width) / float(window_height), near_clipping_plane, far_clipping_plane);
}

void Camera::update(SDL_Window *window, int window_width, int window_height, const u8* keystate, bool fullscreen)
{
	// TODO pull the mouse state calls out of the camera function?

	if (SDL_GetMouseState(&mouse_x, &mouse_y) & SDL_BUTTON(SDL_BUTTON_RIGHT)) {
		if (first_press) {
			if (!fullscreen) {
				SDL_ShowCursor(0);
			}
			first_last_mouse_x = last_mouse_x = mouse_x;
			first_last_mouse_y = last_mouse_y = mouse_y;
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
		if (!first_press) {
			if (!fullscreen) {
				SDL_ShowCursor(1);
			}
			SDL_WarpMouseInWindow(window, first_last_mouse_x, first_last_mouse_y);
		}
		first_press = true;
	}

	if (SDL_GetMouseState(NULL, NULL)  & SDL_BUTTON(SDL_BUTTON_LEFT)) {
		printf("Camera Position: (%f, %f, %f)\n", position.x, position.y, position.z);
		printf("Camera Front: (%f, %f, %f)\n", front.x, front.y, front.z);
	}

	if (keystate[SDL_SCANCODE_A] && !keystate[SDL_SCANCODE_D]) {
		position += speed * normalize(cross(up, front));
	} else if (keystate[SDL_SCANCODE_D] && !keystate[SDL_SCANCODE_A]) {
		position -= speed * normalize(cross(up, front));
	} if (keystate[SDL_SCANCODE_LSHIFT] && keystate[SDL_SCANCODE_W] && !keystate[SDL_SCANCODE_S]) {
		position += speed * up;
	} else if (keystate[SDL_SCANCODE_W] && !keystate[SDL_SCANCODE_S]) {
		position += speed * front;
	} if (keystate[SDL_SCANCODE_LSHIFT] && keystate[SDL_SCANCODE_S] && !keystate[SDL_SCANCODE_W]) {
		position -= speed * up;
	} else if (keystate[SDL_SCANCODE_S] && !keystate[SDL_SCANCODE_W]) {
		position -= speed * front;
	}

	if (keystate[SDL_SCANCODE_Q] && !keystate[SDL_SCANCODE_E]) {
		
	}
	else if (keystate[SDL_SCANCODE_E] && !keystate[SDL_SCANCODE_Q]) {

	}

	view = lookAt(position, position + front, up);
	projection = perspective(radians(field_of_view), float(window_width) / float(window_height), near_clipping_plane, far_clipping_plane);
}
#endif
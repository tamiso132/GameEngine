#include "camera.h"
#include "glm/ext/matrix_transform.hpp"
glm::mat4x4 Camera::get_projection() { return glm::mat4x4(); }

glm::mat4x4 Camera::get_view() {
  glm::mat4 view;
  view = glm::lookAt(_camPos, _camPos + _camFront, _camUp);

  return view;
}

void Camera::process_input(SDL_Event *event, float deltaTime, int mouse_delta_x,
                           int mouse_delta_y, const uint8_t *keystate) {

  //   if (event->type == SDL_MOUSEMOTION)
  glm::vec3 direction;

  float sensitivity = 0.1f;

  mouse_delta_x *= sensitivity;
  mouse_delta_y *= sensitivity;

  _yaw += mouse_delta_x;
  _pitch += mouse_delta_y * -1;

  if (_pitch > 89.0f)
    _pitch = 89.0f;
  if (_pitch < -89.0f)
    _pitch = -89.0f;

  direction.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
  direction.y = sin(glm::radians(_pitch));
  direction.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));

  _camFront = glm::normalize(direction);
  //  }

  //   if (event->type == SDL_KEYDOWN)
  const float camera_speed = 2.5f * deltaTime;
  // Handle key events
  if (keystate[SDL_SCANCODE_W]) {
    auto d = camera_speed * _camFront;
    // printf("speed x: %f\n", d.x);
    // printf("speed y: %f\n", d.y);
    // printf("speed z: %f\n", d.z);
    // printf("\n\n");
    _camPos += camera_speed * _camFront;
  }
  if (keystate[SDL_SCANCODE_S]) {
    _camPos -= camera_speed * _camFront;
  }
  if (keystate[SDL_SCANCODE_A]) {
    _camPos -= glm::normalize(glm::cross(_camFront, _camUp)) * camera_speed;
  }
  if (keystate[SDL_SCANCODE_D]) {
    _camPos += glm::normalize(glm::cross(_camFront, _camUp)) * camera_speed;
  }
}

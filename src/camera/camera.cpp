#include "camera.h"
#include "glm/ext/matrix_transform.hpp"
#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>

glm::mat4x4 Camera::get_projection() { return ci_projection; }

glm::mat4x4 Camera::get_view() {
    glm::mat4 view;
    view = glm::lookAt(_camPos, _camPos + _camFront, _camUp);

    return view;
}

glm::vec3 Camera::get_camera_position() { return _camPos; }

void Camera::create_frustum() {
    Frustum frustum;

    float aspect = ((float)ci_extent.width) / ((float)ci_extent.height);

    const float     farHalfHeight = ci_far * tanf(ci_fovy * 0.5f);
    const float     farHalfWidth  = farHalfHeight * aspect;
    const glm::vec3 frontMultFar  = ci_far * _camFront;
    glm::vec3       camRight      = glm::cross(_camFront, _camUp);

    glm::vec3 camPosNorm = glm::normalize(_camPos);

    ci_frustum.nearFace.normal   = glm::normalize(_camPos + ci_near * _camFront);
    ci_frustum.nearFace.distance = glm::dot(ci_frustum.nearFace.normal, _camFront);

    ci_frustum.farFace.normal   = glm::normalize(_camPos + frontMultFar);
    ci_frustum.farFace.distance = glm::dot(ci_frustum.farFace.normal, -_camFront);

    ci_frustum.rightFace.normal   = camPosNorm;
    ci_frustum.rightFace.distance = glm::dot(camPosNorm, glm::cross(frontMultFar - camRight * farHalfWidth, _camFront));

    ci_frustum.leftFace.normal   = camPosNorm;
    ci_frustum.leftFace.distance = glm::dot(camPosNorm, glm::cross(_camUp, frontMultFar + camRight * farHalfHeight));

    ci_frustum.topFace.normal   = camPosNorm;
    ci_frustum.topFace.distance = glm::dot(camPosNorm, glm::cross(camRight, frontMultFar - _camUp * farHalfHeight));

    ci_frustum.bottomFace.normal   = camPosNorm;
    ci_frustum.bottomFace.distance = glm::dot(camPosNorm, glm::cross(frontMultFar + _camUp * farHalfHeight, camRight));
}

void Camera::process_input(SDL_Event *event, float deltaTime, int mouse_delta_x, int mouse_delta_y, const uint8_t *keystate, uint8_t focusWindow) {

    if (!focusWindow)
        return;
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

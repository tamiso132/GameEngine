#pragma once

#include <SDL2/SDL.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>

class Camera {
  public:
    Camera() {
        glm::mat4x4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
        projection[1][1] *= -1;
        ci_projection = projection;
    }

    glm::mat4x4 get_projection();
    glm::mat4x4 get_view();
    void        process_input(SDL_Event *event, float deltaTime, int mouse_delta_x, int mouse_delta_y, const uint8_t *keycode, uint8_t focusWindow);
    glm::vec3   get_camera_position();

  private:
    glm::mat4x4 ci_projection;

    glm::vec3 _camPos   = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 _camFront = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 _camUp    = glm::vec3(0.0f, 1.0f, 0.0f);

    float _yaw   = 0;
    float _pitch = 0;
};

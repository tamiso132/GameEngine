#pragma once

#include <glm/glm.hpp>
#include <SDL2/SDL.h>
class Camera
{
public:
    glm::mat4x4 get_projection();
    glm::mat4x4 get_view();
    void process_input(SDL_Event *event, float deltaTime, int mouse_delta_x, int mouse_delta_y);

private:
    glm::vec3 _camPos = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 _camFront = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 _camUp = glm::vec3(0.0f, 1.0f, 0.0f);

    float _yaw = 0;
    float _pitch = 0;
};

#pragma once

#include <SDL2/SDL.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

struct Plane {
    glm::vec3 normal   = {0.f, 1.f, 0.f}; // unit vector
    float     distance = 0.f;             // Distance with origin

    Plane() = default;

    Plane(const glm::vec3 &p1, const glm::vec3 &norm) : normal(glm::normalize(norm)), distance(glm::dot(normal, p1)) {}

    float getSignedDistanceToPlane(const glm::vec3 &point) const { return glm::dot(normal, point) - distance; }
};

struct Frustum {
    Plane topFace;
    Plane bottomFace;

    Plane rightFace;
    Plane leftFace;

    Plane farFace;
    Plane nearFace;
};

class Camera {
  public:
    Camera(VkExtent2D windowExtent) {
        ci_extent    = windowExtent;
        float aspect = ((float)ci_extent.width) / ((float)ci_extent.height);

        glm::mat4x4 projection = glm::perspective(ci_fovy, aspect, ci_near, ci_far);
        projection[1][1] *= -1;
        ci_projection = projection;
    }

    Camera() = default;

    /*Functions*/

    glm::mat4x4 get_projection();
    glm::mat4x4 get_view();
    void        process_input(SDL_Event *event, float deltaTime, int mouse_delta_x, int mouse_delta_y, const uint8_t *keycode, uint8_t focusWindow);
    glm::vec3   get_camera_position();

  private:
    void create_frustum();

    /*Variables*/
  public:
    Frustum ci_frustum;

  private:
    VkExtent2D  ci_extent;
    glm::mat4x4 ci_projection;

    glm::vec3 _camPos   = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 _camFront = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 _camUp    = glm::vec3(0.0f, 1.0f, 0.0f);

    float _yaw   = 0;
    float _pitch = 0;

    /*Camera Settings*/

    float ci_fovy = glm::radians(70.f);
    float ci_near = 0.1f;
    float ci_far  = 200.0f;
};

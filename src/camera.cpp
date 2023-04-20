#ifndef GLLIB_CAMERA_CPP
#define GLLIB_CAMERA_CPP

#include "camera.hpp"

namespace gllib {

namespace defaults {
    static constexpr float yaw = -90.0f;
    static constexpr float pitch = 0.0f;
    static constexpr float speed = 1.0f;
    static constexpr float maxFov = 90.0f;
    static constexpr float sensitivity = 0.1f;
    static constexpr glm::vec3 worldUp = {0.0f, 1.0f, 0.0f};
}

camera::camera(float fov, float near, float far, uint32_t screenWidth, uint32_t screenHeight, glm::vec3 pos)
    : fov(fov), near(near), far(far),
      aspectRatio((float)screenWidth / (float)screenHeight),
      pos(pos), yaw(defaults::yaw), pitch(defaults::pitch),
      speed(defaults::speed), sensitivity(defaults::sensitivity)
{
    front = glm::vec3(0.0f, 0.0f, -1.0f);
    up = defaults::worldUp;
    right = glm::normalize(glm::cross(front,up));
    
    update();
}

void camera::update_proj()
{
    proj = glm::perspective(fov, aspectRatio, near, far);
}

void camera::update_view()
{
    view = glm::lookAt(pos, pos + front, up);
}

void camera::update_aspect(float aspectRatio)
{
    this->aspectRatio = aspectRatio;
    update();
}

void camera::update()
{
    update_vectors();
    update_proj();
    update_view();
}

void camera::move(movement direction, float deltaTime)
{
    float velocity = speed * deltaTime;
    switch (direction) {
        case movement::forward:
            pos += front * velocity;
            break;
        case movement::backward:
            pos -= front * velocity;
            break;
        case movement::left:
            pos -= right * velocity;
            break;
        case movement::right:
            pos += right * velocity;
            break;
        case movement::up:
            pos += up * velocity;
            break;
        case movement::down:
            pos -= up * velocity;
            break;
    }
}

void camera::rotate(float xoffset, float yoffset, float deltaTime)
{
    if (firstMouse) {
        firstMouse = false;
        return;
    }

    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch -= yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    this->front = glm::normalize(front);

    // update();
}

void camera::update_vectors()
{
    right = glm::normalize(glm::cross(front, defaults::worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void camera::zoom(float yoffset)
{
    if (fov >= 1.0f && fov <= defaults::maxFov)
        fov -= yoffset;
    if (fov <= 1.0f)
        fov = 1.0f;
    if (fov >= defaults::maxFov)
        fov = defaults::maxFov;

    update_proj();
}

glm::mat4 camera::get_proj() {
    return proj;
}
glm::mat4 camera::get_view() {
    return view;
}
glm::mat4 camera::get_combined() {
    return proj * view;
}
glm::vec3 camera::get_pos() {
    return pos;
}

void camera::setFirstMouse() {
    firstMouse = true;
}

} // namespace gllib

#endif // GLLIB_CAMERA_CPP
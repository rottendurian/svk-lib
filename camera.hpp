#ifndef GLLIB_CAMERA_HPP
#define GLLIB_CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdint.h>

namespace gllib {

#undef near
#undef far

class camera {
public:
    enum class movement;
    camera(float fov, float near, float far, uint32_t screenWidth, uint32_t screenHeight, glm::vec3 pos);
    void update_proj();
    void update_view();
    void update_aspect(float aspectRatio);
    void update();
    void move(movement direction, float deltaTime);
    void rotate(float xoffset, float yoffset,float deltaTime);
    void update_vectors();
    void zoom(float yoffset);
    void setFirstMouse();
    
    glm::mat4 get_proj();
    glm::mat4 get_view();
    glm::mat4 get_combined();
    glm::vec3 get_pos();

    enum class movement {
        forward,
        backward,
        left,
        right,
        up,
        down,
    };

private:
    bool firstMouse = true;

    float fov;
    float near;
    float far;
    float aspectRatio;

    float speed;
    float sensitivity;

    float yaw;
    float pitch;

    glm::vec3 pos;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;

    glm::mat4 proj;
    glm::mat4 view;
};




} // gllib

#endif // GLLIB_CAMERA_HPP
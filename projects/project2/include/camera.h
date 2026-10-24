#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

class Camera {
public:
    Camera(glm::vec3 position, glm::quat rotation);

    glm::vec3 position() const;
    void setPosition(glm::vec3 position);
    glm::quat rotation() const;
    void setRotation(glm::quat rotation);
    glm::vec3 getFront() const;
    glm::vec3 getRight() const;
    glm::vec3 getUp() const;

private:
    glm::vec3 m_position;
    glm::quat m_rotation;
};

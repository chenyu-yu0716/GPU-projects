#include <camera.h>

#include <glm/geometric.hpp>

Camera::Camera(glm::vec3 position, glm::quat rotation)
    : m_position(position)
    , m_rotation(glm::normalize(rotation)) {}

glm::vec3 Camera::position() const {
    return m_position;
}

void Camera::setPosition(glm::vec3 position) {
    m_position = position;
}

glm::quat Camera::rotation() const {
    return m_rotation;
}

void Camera::setRotation(glm::quat rotation) {
    m_rotation = glm::normalize(rotation);
}

glm::vec3 Camera::getFront() const {
    return glm::normalize(m_rotation * glm::vec3(0, 0, -1));
}

glm::vec3 Camera::getRight() const {
    return glm::normalize(m_rotation * glm::vec3(1, 0, 0));
}

glm::vec3 Camera::getUp() const {
    return glm::normalize(m_rotation * glm::vec3(0, 1, 0));
}

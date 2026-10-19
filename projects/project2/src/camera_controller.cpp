#include <camera_controller.h>

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

namespace {
constexpr glm::vec3 worldUp{0.0f, 1.0f, 0.0f};
}

CameraController::CameraController(glm::vec3 target)
    : m_target(target) {}

void CameraController::lookAt(Camera& camera) const {
    glm::vec3 front = m_target - camera.position();
    if (front.x * front.x + front.z * front.z < 1e-8f) {
        front.x = -1e-3f;
    }
    front = glm::normalize(front);
    glm::vec3 const right = glm::normalize(glm::cross(front, worldUp));
    glm::vec3 const up = glm::cross(right, front);
    glm::mat3 const rotationMatrix{right, up, -front};
    camera.setRotation(glm::quat_cast(rotationMatrix));
}

glm::vec3 CameraController::horizontalForward(Camera const& camera) const {
    glm::vec3 direction = camera.getFront();
    direction.y = 0;
    if (glm::dot(direction, direction) < 1e-8f) {
        return {-1, 0, 0};
    }
    return glm::normalize(direction);
}

glm::vec3 CameraController::horizontalRight(Camera const& camera) const {
    glm::vec3 direction = camera.getRight();
    direction.y = 0;
    if (glm::dot(direction, direction) < 1e-8f) {
        glm::vec3 const horizontal = horizontalForward(camera);
        return {-horizontal.z, 0, horizontal.x};
    }
    return glm::normalize(direction);
}

bool CameraController::move(Camera& camera, glm::vec3 direction, float distance) const {
    float const directionLengthSquared = glm::dot(direction, direction);
    if (directionLengthSquared == 0 || distance == 0) {
        return false;
    }
    camera.setPosition(camera.position() + glm::normalize(direction) * distance);
    lookAt(camera);
    return true;
}

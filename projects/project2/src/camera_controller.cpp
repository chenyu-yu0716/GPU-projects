#include <camera_controller.h>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

namespace {
constexpr glm::vec3 worldUp{0.0f, 1.0f, 0.0f};
constexpr float maxPitch = glm::radians(85.0f);
constexpr float minDistance = 0.5f;

glm::vec3 offsetFromAngles(float radius, float pitch, float yaw) {
    float const horizontal = radius * glm::cos(pitch);
    return {horizontal * glm::cos(yaw), radius * glm::sin(pitch), horizontal * glm::sin(yaw)};
}
} // namespace

CameraController::CameraController(glm::vec3 target)
    : m_target(target) {}

void CameraController::lookAt(Camera& camera) const {
    glm::vec3 offset = camera.position() - m_target;
    if (glm::dot(offset, offset) < 1e-12f) {
        offset = {minDistance, 0.0f, 0.0f};
    }

    float const radius = glm::max(glm::length(offset), minDistance);
    float const horizontal = glm::length(glm::vec2{offset.x, offset.z});
    // Keep the view direction away from the poles where front and worldUp are parallel.
    float const pitch = glm::clamp(glm::atan(offset.y, horizontal), -maxPitch, maxPitch);
    float const yaw = horizontal > 1e-6f ? glm::atan(offset.z, offset.x) : 0.0f;

    glm::vec3 const constrainedOffset = offsetFromAngles(radius, pitch, yaw);
    camera.setPosition(m_target + constrainedOffset);

    glm::vec3 const front = -glm::normalize(constrainedOffset);
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

    glm::vec3 const previousPosition = camera.position();
    glm::vec3 offset = camera.position() - m_target;
    if (glm::length(offset) < minDistance || glm::length(glm::vec2{offset.x, offset.z}) < 1e-6f) {
        lookAt(camera);
        offset = camera.position() - m_target;
    }

    float const radius = glm::length(offset);
    float const horizontal = glm::length(glm::vec2{offset.x, offset.z});
    float const pitch = glm::atan(offset.y, horizontal);
    float const yaw = glm::atan(offset.z, offset.x);
    glm::vec3 const displacement = glm::normalize(direction) * distance;

    // Resolve world-space movement along the orbit's radial, pitch, and yaw directions.
    glm::vec3 const radial = offset / radius;
    glm::vec3 const pitchTangent{-glm::sin(pitch) * glm::cos(yaw), glm::cos(pitch), -glm::sin(pitch) * glm::sin(yaw)};
    glm::vec3 const yawTangent{-glm::sin(yaw), 0.0f, glm::cos(yaw)};

    float const nextRadius = glm::max(radius + glm::dot(displacement, radial), minDistance);
    float const nextPitch = glm::clamp(pitch + glm::dot(displacement, pitchTangent) / radius, -maxPitch, maxPitch);
    float const nextYaw = yaw + glm::dot(displacement, yawTangent) / horizontal;
    glm::vec3 const nextPosition = m_target + offsetFromAngles(nextRadius, nextPitch, nextYaw);
    if (glm::dot(nextPosition - previousPosition, nextPosition - previousPosition) < 1e-12f) {
        return false;
    }

    camera.setPosition(nextPosition);
    lookAt(camera);
    return true;
}

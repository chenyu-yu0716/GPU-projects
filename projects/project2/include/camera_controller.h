#pragma once

#include <camera.h>

class CameraController {
public:
    explicit CameraController(glm::vec3 target);

    void lookAt(Camera& camera) const;
    glm::vec3 horizontalForward(Camera const& camera) const;
    glm::vec3 horizontalRight(Camera const& camera) const;
    bool move(Camera& camera, glm::vec3 direction, float distance) const;

private:
    glm::vec3 m_target;
};

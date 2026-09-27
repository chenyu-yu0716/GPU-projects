#pragma once

#include <utility>

struct GLFWwindow;

namespace gfx {
class GraphicsContext {
public:
    GraphicsContext(std::pair<int, int> const& minimumVersion);

    ~GraphicsContext();

    void init(GLFWwindow* window);

    int getMajorVersion() const noexcept;

    int getMinorVersion() const noexcept;

    void restoreDepthStates();

    void restoreStencilStates();

private:
    std::pair<int, int> m_version;

    bool checkVersion(GLFWwindow* window);

    bool registerDebugOutput();
};
} // namespace gfx
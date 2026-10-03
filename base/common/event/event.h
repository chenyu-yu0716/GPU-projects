#pragma once

#include <string>
#include <type_traits>

class Event {
public:
    enum class Type {
        None,
        WindowClose,
        WindowResize,
        WindowFramebufferResize,
        WindowIconify,
        KeyPress,
        KeyRelease,
        KeyType,
        MouseMove,
        MouseScroll,
        MouseButtonPress,
        MouseButtonRelease,
        MouseButtonHold,
    };

    enum class Category {
        None = 0,
        Input = 1 << 0,
        Window = 1 << 1,
        Keyboard = 1 << 2,
        Mouse = 1 << 3,
    };

public:
    bool isHandled{ false };

public:
    virtual ~Event() = default;

    bool isInCategory(Category category) const noexcept {
        using T = std::underlying_type_t<Category>;
        return static_cast<T>(getCategoryBitmask()) & static_cast<T>(category);
    }

    virtual Type getType() const noexcept = 0;

    virtual Category getCategoryBitmask() const noexcept = 0;

    virtual std::string getInfo() const = 0;
};

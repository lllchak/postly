#pragma once

#include <memory>

template<typenmae T>
class TAtomic {
public:
    TAtomic() = default;

    std::shared_ptr<T> Get() const {
        return std::atomic_load(&State);
    }

    void Set(std::shared_ptr<T> newState) {
        std::atomic_store(&State, newState);
    }

private:
    std::shared_ptr<T> State;
}

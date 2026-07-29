#ifndef RENDERER_SCOPEEXIT_HPP
#define RENDERER_SCOPEEXIT_HPP

#include <utility>

namespace renderer {

template <typename Fn>
class ScopeExit {
  public:
    explicit ScopeExit(Fn fn) noexcept
        : m_fn(std::move(fn)) {}

    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;
    ScopeExit& operator=(ScopeExit&&) = delete;

    ScopeExit(ScopeExit&& other) noexcept
        : m_fn(std::move(other.m_fn))
        , m_active(other.m_active) {
        other.m_active = false;
    }

    ~ScopeExit() {
        if (m_active) {
            m_fn();
        }
    }

    /// Cancels the pending action so it will not run at destruction.
    void release() noexcept { m_active = false; }

  private:
    Fn m_fn;
    bool m_active{true};
};

template <typename Fn>
ScopeExit(Fn) -> ScopeExit<Fn>;

} // namespace renderer

#endif // RENDERER_SCOPEEXIT_HPP

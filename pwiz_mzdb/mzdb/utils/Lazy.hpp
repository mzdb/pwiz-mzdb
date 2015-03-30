#ifndef LAZY_HPP
#define LAZY_HPP

#include <utility>
#include <type_traits>

namespace mzdb {

namespace detail {

template<class Func>
struct Lazy {
    typedef typename std::result_of<Func()>::type result_type;

    Lazy(): func_(nullptr), initialized_(false) {}
    explicit Lazy(Func&& f) : func_(std::move(f)), initialized_(false) {}
    explicit Lazy(Func& f)  : func_(f), initialized_(false) {}

    const result_type& operator()() const {
        return const_cast<Lazy&>(*this)();
    }

    result_type& operator()() {
        if (!initialized_) {
            value_ = func_();
            initialized_ = true;
        }
        return value_;
    }

private:
    bool initialized_;
    result_type value_;
    Func func_;
};

}

/// Using lambdas
template<class Func>
detail::Lazy<typename std::remove_reference<Func>::type>
lazy(Func&& fun) {
    return detail::Lazy<typename std::remove_reference<Func>::type>(
                std::forward<Func>(fun)
                );
}

} // end mzdb

#endif // LAZY_HPP

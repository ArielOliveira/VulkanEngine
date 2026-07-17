#ifndef UTILS_PASSKEY_HPP
#define UTILS_PASSKEY_HPP

namespace Utils {
    template <typename T>
    class PassKey {
            friend T;
        private:
            PassKey() = default;
    };
}
#endif
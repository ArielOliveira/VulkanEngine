#ifndef RENDERER_HPP
#define RENDERER_HPP

namespace Graphics {
    class Renderer {
        public:
            static Renderer& getInstance();

            Renderer(const Renderer&) = delete;
            Renderer(Renderer&&) noexcept = delete;

            Renderer& operator=(const Renderer&) = delete;
            Renderer& operator=(Renderer&&) noexcept = delete;

            ~Renderer();
        private:
            Renderer();

    };
}

#endif
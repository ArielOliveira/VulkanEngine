#ifndef COMMAND_BUFFER_HPP
#define COMMAND_BUFFER_HPP

#include <graphics/core.hpp>
#include <graphics/models.hpp>
#include <vector>

using std::vector;

namespace Graphics {
    class CommandBuffer {
        public:
            CommandBuffer(const vk::raii::CommandPool& commandPool, const vk::raii::Queue* queue, vk::CommandBufferLevel level, uint32_t count = 1);
            CommandBuffer(const vk::raii::CommandPool& commandPool, const vk::raii::Queue* queue, vk::CommandBufferUsageFlags usageFlags, vk::CommandBufferLevel level, uint32_t count = 1);
            CommandBuffer(CommandBuffer &&rhs) = default;
            CommandBuffer(std::nullptr_t) noexcept;
            ~CommandBuffer();

            CommandBuffer& operator=(const CommandBuffer&) = delete;
            CommandBuffer& operator=(CommandBuffer&&) noexcept = default;
            CommandBuffer& operator=(std::nullptr_t) noexcept;

            const vk::raii::CommandBuffer& operator[](int index) const;

            static CommandBuffer singleTimeTransferCommand();
            static CommandBuffer singleTimeGraphicsCommand();

            void addBarrier(const vk::DependencyInfo& info, uint32_t index = 0) const;
            void blit(const vk::BlitImageInfo2& info, uint32_t index = 0) const;
            void copyBufferToImage(const vk::CopyBufferToImageInfo2 &info, uint32_t index = 0) const;
            void dispatch(uint32_t index = 0, bool waitBeforeDispatch = false) const;
            void dispatch(const vk::SubmitInfo &info, const vk::raii::Fence &fence, uint32_t index = 0, bool waitBeforeDispatch = false) const;
        private:
            const vk::raii::Queue* queue;

            vector<vk::raii::CommandBuffer> commands;
    };
}

#endif
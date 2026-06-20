#include <graphics/commandBuffer.hpp>

namespace Graphics {
    CommandBuffer::CommandBuffer(const vk::raii::CommandPool& commandPool, const vk::raii::Queue* queue, vk::CommandBufferLevel level, uint32_t count) {
        this->queue = queue;

        vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool, .level = level, .commandBufferCount = count};
        commands = vk::raii::CommandBuffers(Core::getInstance().getDevice(), allocInfo);
        
        for (auto &command : commands)
            command.begin({});
    }

    CommandBuffer::CommandBuffer(const vk::raii::CommandPool& commandPool, const vk::raii::Queue* queue, vk::CommandBufferUsageFlags usageFlags, vk::CommandBufferLevel level, uint32_t count) {
        this->queue = queue;
        
        vk::CommandBufferAllocateInfo allocInfo{.commandPool = commandPool, .level = level, .commandBufferCount = count};
        commands = vk::raii::CommandBuffers(Core::getInstance().getDevice(), allocInfo);

        vk::CommandBufferBeginInfo beginInfo{.flags = usageFlags};
        
        for (auto &command : commands)
            command.begin(beginInfo);
    }

    CommandBuffer CommandBuffer::singleTimeTransferCommand() {
        const Core& core = Core::getInstance();

        return std::move(CommandBuffer(core.getTransferCommandPool(), &core.getTransferQueue(), 
                             vk::CommandBufferUsageFlagBits::eOneTimeSubmit, vk::CommandBufferLevel::ePrimary));
    }
    
    CommandBuffer CommandBuffer::singleTimeGraphicsCommand() {
        const Core& core = Core::getInstance();

        return std::move(CommandBuffer(core.getGraphicsCommandPool(), &core.getGraphicsQueue(), 
                             vk::CommandBufferUsageFlagBits::eOneTimeSubmit, vk::CommandBufferLevel::ePrimary));
    }

    CommandBuffer::CommandBuffer(std::nullptr_t) noexcept {}

    CommandBuffer::~CommandBuffer() {
        for (auto &command : commands) {
            command.release();
            command = nullptr;
        }

        commands.clear();
    }

    CommandBuffer& CommandBuffer::operator=(std::nullptr_t) noexcept {
        for (auto &command : commands) {
            command.release();
            command = nullptr;
        }

        commands.clear();

        return *this;
    }

    void CommandBuffer::addBarrier(const vk::DependencyInfo& info, uint32_t index) const { commands[index].pipelineBarrier2(info); }
    void CommandBuffer::blit(const vk::BlitImageInfo2& info, uint32_t index) const { commands[index].blitImage2(info); }
    void CommandBuffer::copyBufferToImage(const vk::CopyBufferToImageInfo2 &info, uint32_t index) const { commands[index].copyBufferToImage2(info); }

    void CommandBuffer::dispatch(uint32_t index) const {
        commands[index].end();

        vk::SubmitInfo info { 
            .commandBufferCount = 1, 
            .pCommandBuffers = &*commands[index]
        };

        queue->submit(info, nullptr);
        queue->waitIdle();
    }

    void CommandBuffer::dispatch(const vk::SubmitInfo &info, const vk::raii::Fence &fence, uint32_t index) const {
        commands[index].end();

        queue->submit(info, *fence);
    }
}
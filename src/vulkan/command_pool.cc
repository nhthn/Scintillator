#include "vulkan/command_pool.h"

#include "vulkan/device.h"
#include "vulkan/pipeline.h"
#include "vulkan/swapchain.h"

namespace scin {

namespace vk {

CommandPool::CommandPool(std::shared_ptr<Device> device) :
    device_(device),
    command_pool_(VK_NULL_HANDLE) {
}

CommandPool::~CommandPool() {
    Destroy();
}

bool CommandPool::Create() {
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = device_->graphics_family_index();
    pool_info.flags = 0;
    return (vkCreateCommandPool(device_->get(), &pool_info, nullptr,
            &command_pool_) == VK_SUCCESS);
}

void CommandPool::Destroy() {
    if (command_pool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_->get(), command_pool_, nullptr);
        command_pool_ = VK_NULL_HANDLE;
    }
}

bool CommandPool::CreateCommandBuffers(Swapchain* swapchain,
        Pipeline* pipeline) {
    command_buffers_.resize(swapchain->image_count());

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool_;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = static_cast<uint32_t>(
            command_buffers_.size());

    if (vkAllocateCommandBuffers(device_->get(), &alloc_info,
            command_buffers_.data()) != VK_SUCCESS) {
        return false;
    }

    for (size_t i = 0; i < command_buffers_.size(); ++i) {
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        begin_info.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(command_buffers_[i], &begin_info)
                != VK_SUCCESS) {
            return false;
        }

        VkRenderPassBeginInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = pipeline->render_pass();
        render_pass_info.framebuffer = swapchain->framebuffer(i);
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = swapchain->extent();
        VkClearValue clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clear_color;

        vkCmdBeginRenderPass(command_buffers_[i], &render_pass_info,
                VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(command_buffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline->get());
        vkCmdDraw(command_buffers_[i], 3, 1, 0, 0);
        vkCmdEndRenderPass(command_buffers_[i]);
        if (vkEndCommandBuffer(command_buffers_[i]) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

}    // namespace vk

}    // namespace scin
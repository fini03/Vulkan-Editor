#include "vulkan_base.h"

void VulkanRenderPass::createRenderPass(VulkanContext* context, VkFormat format) {
	VkAttachmentDescription colorAttachment{
		.format = format,
    	.samples = VK_SAMPLE_COUNT_1_BIT,
    	.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    	.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    	.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    	.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    	.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    	.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

    VkAttachmentReference colorAttachmentRef{
	    .attachment = 0,
	    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass{
	    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
	    .colorAttachmentCount = 1,
	    .pColorAttachments = &colorAttachmentRef,
    };

    // We need 2 dependencies not sure why yet
    std::array<VkSubpassDependency, 2> dependencies{};

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dependencies[0].dependencyFlags = 0;

    dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].dstSubpass = 0;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].srcAccessMask = 0;
    dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    dependencies[1].dependencyFlags = 0;

    VkRenderPassCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
	    .attachmentCount = 1,
	    .pAttachments = &colorAttachment,
	    .subpassCount = 1,
	    .pSubpasses = &subpass,
	    .dependencyCount = static_cast<uint32_t>(dependencies.size()),
	    .pDependencies = dependencies.data()
    };

    if (vkCreateRenderPass(context->device, &createInfo, nullptr, &this->renderPass) != VK_SUCCESS) {
    	throw std::runtime_error("Failed to create render pass!");
    }
}

void VulkanRenderPass::destroyRenderpass(VulkanContext* context) {
	vkDestroyRenderPass(context->device, renderPass, 0);
}

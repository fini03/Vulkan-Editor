#include "vulkan_utils.h"

uint32_t findMemoryType(VulkanContext* context, uint32_t typeFilter, VkMemoryPropertyFlags memoryProperties) {
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(context->physicalDevice, &deviceMemoryProperties);

	for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; ++i) {
		// Check if required memory type is allowed
		if ((typeFilter & (1 << i)) != 0) {
			// Check if required properties are satisfied
			if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties) {
				// Return this memory type index
				return i;
			}
		}
	}

	// No matching avaialble memroy type found
	assert(false);
	return UINT32_MAX;
}

void VulkanBuffer::createBuffer(VulkanContext* context, uint64_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties) {
	VkBufferCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage
	};

	if (vkCreateBuffer(context->device, &createInfo, 0, &this->buffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(context->device, this->buffer, &memoryRequirements);

	uint32_t memoryIndex = findMemoryType(context, memoryRequirements.memoryTypeBits, memoryProperties);
	assert(memoryIndex != UINT32_MAX);

	VkMemoryAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = memoryIndex
	};

	vkAllocateMemory(context->device, &allocateInfo, 0, &this->memory);
	vkBindBufferMemory(context->device, this->buffer, this->memory, 0);
}

void VulkanBuffer::uploadDataToBuffer(VulkanContext* context, void* data, size_t size) {
	VulkanQueue* queue = &context->graphicsQueue;
	VkCommandPool commandPool = {};
	VkCommandBuffer commandBuffer = {};
	VulkanBuffer stagingBuffer = {};

	stagingBuffer.createBuffer(
		context,
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	vkMapMemory(context->device, stagingBuffer.memory, 0, size, 0, &mapped);
	memcpy(mapped, data, size);
	vkUnmapMemory(context->device, stagingBuffer.memory);

	{
		VkCommandPoolCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			.queueFamilyIndex = queue->familyIndex
		};

		vkCreateCommandPool(context->device, &createInfo, 0, &commandPool);
	}

	{
		VkCommandBufferAllocateInfo allocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		vkAllocateCommandBuffers(context->device, &allocateInfo, &commandBuffer);
	}

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy region = { 0, 0, size };
	vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, this->buffer, 1, &region);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer
	};

	vkQueueSubmit(queue->queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue->queue);

	vkDestroyCommandPool(context->device, commandPool, 0);
	stagingBuffer.destroyBuffer(context);
}

void VulkanBuffer::destroyBuffer(VulkanContext* context) {
	vkDestroyBuffer(context->device, this->buffer, 0);
	vkFreeMemory(context->device, this->memory, 0);
}

void VulkanImage::createImage(
	VulkanContext* context,
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageUsageFlags usage
) {
	{
		VkImageCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = format,
			.extent = { width, height, 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = usage,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
		};

		vkCreateImage(context->device, &createInfo, 0, &this->image);
	}

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(context->device, this->image, &memoryRequirements);
	VkMemoryAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = findMemoryType(context, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};

	vkAllocateMemory(context->device, &allocateInfo, 0, &this->memory);
	vkBindImageMemory(context->device, this->image, this->memory, 0);

	VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	if(format == VK_FORMAT_D32_SFLOAT) {
		aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	{
		VkImageViewCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = this->image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = format,
			.subresourceRange = { aspect, 0, 1, 0, 1 }
		};

		vkCreateImageView(context->device, &createInfo, 0, &this->view);
	}
}

void VulkanImage::createImageView(
	VulkanContext* context,
	VkFormat format,
	VkImageAspectFlags aspectFlags
) {
	VkImageViewCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
	    .viewType = VK_IMAGE_VIEW_TYPE_2D,
	    .format = format,
	    .subresourceRange = { aspectFlags, 0, 1, 0, 1 }
	};

    if (vkCreateImageView(context->device, &createInfo, nullptr, &this->view) != VK_SUCCESS) {
    	throw std::runtime_error("failed to create texture image view!");
    }
}

void VulkanImage::uploadDataToImage(
	VulkanContext* context,
	void* data,
	size_t size,
	uint32_t width,
	uint32_t height,
	VkImageLayout finalLayout,
	VkAccessFlags dstAccessMask
) {
	// Upload with staging buffer
	VulkanQueue* queue = &context->graphicsQueue;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VulkanBuffer stagingBuffer;
	stagingBuffer.createBuffer(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void* mappedMemory;
	vkMapMemory(context->device, stagingBuffer.memory, 0, size, 0, &mappedMemory);
	memcpy(mappedMemory, data, size);
	vkUnmapMemory(context->device, stagingBuffer.memory);

	{
		VkCommandPoolCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			.queueFamilyIndex = queue->familyIndex
		};

		vkCreateCommandPool(context->device, &createInfo, 0, &commandPool);
	}

	{
		VkCommandBufferAllocateInfo allocateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = commandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1
		};

		vkAllocateCommandBuffers(context->device, &allocateInfo, &commandBuffer);
	}

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	{
		VkImageMemoryBarrier imageBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = this->image,
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		};

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, &imageBarrier);
	}

	VkBufferImageCopy region = {
		.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
		.imageExtent = {width, height, 1}

	};

	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.buffer, this->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	{
		VkImageMemoryBarrier imageBarrier = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_NONE,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = finalLayout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = this->image,
			.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
		};

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, 0, 0, 0, 1, &imageBarrier);
	}

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer
	};

	vkQueueSubmit(queue->queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue->queue);

	vkDestroyCommandPool(context->device, commandPool, 0);
	stagingBuffer.destroyBuffer(context);
}

void VulkanImage::destroyImage(VulkanContext* context) {
	vkDestroyImageView(context->device, this->view, 0);
	vkDestroyImage(context->device, this->image, 0);
	vkFreeMemory(context->device, this->memory, 0);
}

#pragma once
#include "vulkan_editor/vulkan_base.h"

class VulkanBuffer {
public:
	VkBuffer buffer;
	VkDeviceMemory memory;
	void* mapped;

	// Methods
	void createBuffer(
		VulkanContext* context,
		uint64_t size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags memoryProperties
	);
	void uploadDataToBuffer(
		VulkanContext* context,
		void* data,
		size_t size
	);
	void destroyBuffer(VulkanContext* context);
};

class VulkanImage {
public:
	VkImage image;
	VkImageView view;
	VkDeviceMemory memory;

	// Methods
	void createImage(
		VulkanContext* context,
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageUsageFlags usage
	);

	void createImageView(
		VulkanContext* context,
		VkFormat format,
		VkImageAspectFlags aspectFlags
	);

	void uploadDataToImage(
		VulkanContext* context,
		void* data,
		size_t size,
		uint32_t width,
		uint32_t height,
		VkImageLayout finalLayout,
		VkAccessFlags dstAccessMask
	);
	void destroyImage(VulkanContext* context);
};

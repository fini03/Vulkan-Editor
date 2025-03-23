#pragma once
#include "imgui.h"
#include "vulkan_editor/vulkan_base.h"
#include <fstream>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
//#include "node.h"
#include "swapchain.h"
#include "imgui-node-editor/imgui_node_editor.h"

namespace ed = ax::NodeEditor;

class VertexDataNode {
public:
    virtual void generateVertexBindings() = 0;
};

class ColorDataNode {
public:
    virtual void generateColorBindings() = 0;
};

class TextureDataNode {
public:
    virtual void generateTextureBindings() = 0;
};

class ModelNode : public Node, public VertexDataNode, public ColorDataNode, public TextureDataNode {
public:
	std::ofstream outFile;
	size_t attributesCount = 0;
    ModelNode(int id) : Node(id) {
    	outputPins.push_back({ ed::PinId(id * 10 + 1), PinType::VertexOutput });
     	outputPins.push_back({ ed::PinId(id * 10 + 2), PinType::ColorOutput });
        outputPins.push_back({ ed::PinId(id * 10 + 3), PinType::TextureOutput });
        attributesCount = outputPins.size();
    }

    ~ModelNode() override { }

    void generateVertexBindings() override {
    	outFile.open("Vertex.h", std::ios::app);
        if (outFile.is_open()) {
            outFile << "        attributeDescriptions[0].binding = 0;\n"
                    << "        attributeDescriptions[0].location = 0;\n"
                    << "        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;\n"
                    << "        attributeDescriptions[0].offset = offsetof(Vertex, pos);\n\n";
            outFile.close();
        } else {
            std::cerr << "Error opening file for writing.\n";
        }
    }

    void generateColorBindings() override {
    	outFile.open("Vertex.h", std::ios::app);
        if (outFile.is_open()) {
            outFile << "        attributeDescriptions[1].binding = 0;\n"
                    << "        attributeDescriptions[1].location = 1;\n"
                    << "        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;\n"
                    << "        attributeDescriptions[1].offset = offsetof(Vertex, color);\n\n";
            outFile.close();
        } else {
            std::cerr << "Error opening file for writing.\n";
        }
    }

    void generateTextureBindings() override {
    	outFile.open("Vertex.h", std::ios::app);
        if (outFile.is_open()) {
            outFile << "        attributeDescriptions[2].binding = 0;\n"
                    << "        attributeDescriptions[2].location = 2;\n"
                    << "        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;\n"
                    << "        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);\n\n";
            outFile.close();
        } else {
            std::cerr << "Error opening file for writing.\n";
        }
    }

    void generateVertexStructFilePart1() {
    	outFile.open("Vertex.h", std::ios::app);
        if (!outFile.is_open()) {
            std::cerr << "Error opening file for writing.\n";
            return;
        }

        outFile << "struct Vertex {\n"
                << "    glm::vec3 pos;\n"
                << "    glm::vec3 color;\n"
                << "    glm::vec2 texCoord;\n\n"

                << "    static VkVertexInputBindingDescription getBindingDescription() {\n"
                << "        VkVertexInputBindingDescription bindingDescription{};\n"
                << "        bindingDescription.binding = 0;\n"
                << "        bindingDescription.stride = sizeof(Vertex);\n"
                << "        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;\n\n"
                << "        return bindingDescription;\n"
                << "    }\n\n"

                << "    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {\n"
                << "        std::vector<VkVertexInputAttributeDescription>attributeDescriptions(" << attributesCount << ");\n\n";

                outFile.close();
    }

    void generateVertexStructFilePart2() {
    	outFile.open("Vertex.h", std::ios::app);
	    if (!outFile.is_open()) {
	        std::cerr << "Error opening file for writing.\n";
	        return;
	    }

        outFile << "        return attributeDescriptions;\n"
                << "    }\n\n"

                << "    bool operator==(const Vertex& other) const {\n"
                << "        return pos == other.pos && color == other.color && texCoord == other.texCoord;\n"
                << "    }\n"
                << "};\n\n"

                << "namespace std {\n"
                << "    template<> struct hash<Vertex> {\n"
                << "        size_t operator()(Vertex const& vertex) const {\n"
                << "            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);\n"
                << "        }\n"
                << "    };\n"
                << "}\n";

        outFile.close();
        std::cout << "Vertex struct successfully written to Vertex.h\n";
    }

    void generateModel() {
        outFile.open("Vertex.h", std::ios::app);
        if (!outFile.is_open()) {
            std::cerr << "Error opening file for writing.\n";
            return;
        }

        outFile << R"(
class VulkanTutorial {
	public:
    void MemCopy(VkDevice device, void* source, VmaAllocationInfo& allocInfo, VkDeviceSize size) {
        memcpy(allocInfo.pMappedData, source, size);
    }

    VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = commandPool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            return commandBuffer;
        }

        void endSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphicsQueue);

            vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        }

    void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator
        , VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties
        , VmaAllocationCreateFlags vmaFlags, VkBuffer& buffer
        , VmaAllocation& allocation, VmaAllocationInfo* allocationInfo = nullptr) {

        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = vmaFlags;
        vmaCreateBuffer(vmaAllocator, &bufferInfo, &allocInfo, &buffer, &allocation, allocationInfo);
    }

    void copyBuffer(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkBuffer srcBuffer
        , VkBuffer dstBuffer, VkDeviceSize size) {

        VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
    }

    void destroyBuffer(VkDevice device, VmaAllocator vmaAllocator, VkBuffer buffer, VmaAllocation& allocation) {

        vmaDestroyBuffer(vmaAllocator, buffer, allocation);
    }

    void transitionImageLayout(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool
            , VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {

            VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            } else {
                throw std::invalid_argument("unsupported layout transition!");
            }

            vkCmdPipelineBarrier(
                commandBuffer,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
        }

        void copyBufferToImage(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool
            , VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {

            VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, commandPool);

            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {
                width,
                height,
                1
            };

            vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            endSingleTimeCommands(device, graphicsQueue, commandPool, commandBuffer);
        }

        void createImage(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator, uint32_t width, uint32_t height
            , VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties
            , VkImage& image, VmaAllocation& imageAllocation) {

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            allocInfo.priority = 1.0f;
            vmaCreateImage(vmaAllocator, &imageInfo, &allocInfo, &image, &imageAllocation, nullptr);
        }

        VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VkImageView imageView;
            if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image view!");
            }

            return imageView;
        }

        void createTextureImageView(VkDevice device, Texture& texture) {
            texture.m_textureImageView = createImageView(device, texture.m_textureImage, VK_FORMAT_R8G8B8A8_SRGB
                                            , VK_IMAGE_ASPECT_COLOR_BIT);
        }

    void createTextureImage(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator
          , VkQueue graphicsQueue, VkCommandPool commandPool, const std::string& TEXTURE_PATH, Texture& texture) {

          int texWidth, texHeight, texChannels;
          stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
          VkDeviceSize imageSize = texWidth * texHeight * 4;

          if (!pixels) {
              throw std::runtime_error("failed to load texture image!");
          }

          VkBuffer stagingBuffer;
          VkDeviceMemory stagingBufferMemory;
          VmaAllocation stagingBufferAllocation;
          VmaAllocationInfo allocInfo;
          createBuffer(physicalDevice, device, vmaAllocator, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
              , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
              , VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
              , stagingBuffer, stagingBufferAllocation, &allocInfo);

          MemCopy(device, pixels, allocInfo, imageSize);

          stbi_image_free(pixels);

          createImage(physicalDevice, device, vmaAllocator, texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB
              , VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
              , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture.m_textureImage, texture.m_textureImageAllocation);

          transitionImageLayout(device, graphicsQueue, commandPool, texture.m_textureImage, VK_FORMAT_R8G8B8A8_SRGB
              , VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
              copyBufferToImage(device, graphicsQueue, commandPool, stagingBuffer, texture.m_textureImage
                  , static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
          transitionImageLayout(device, graphicsQueue, commandPool, texture.m_textureImage, VK_FORMAT_R8G8B8A8_SRGB
              , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

          destroyBuffer(device, vmaAllocator, stagingBuffer, stagingBufferAllocation);
      }

      void createTextureSampler(VkPhysicalDevice physicalDevice, VkDevice device, Texture &texture) {
          VkPhysicalDeviceProperties properties{};
          vkGetPhysicalDeviceProperties(physicalDevice, &properties);

          VkSamplerCreateInfo samplerInfo{};
          samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
          samplerInfo.magFilter = VK_FILTER_LINEAR;
          samplerInfo.minFilter = VK_FILTER_LINEAR;
          samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
          samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
          samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
          samplerInfo.anisotropyEnable = VK_TRUE;
          samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
          samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
          samplerInfo.unnormalizedCoordinates = VK_FALSE;
          samplerInfo.compareEnable = VK_FALSE;
          samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
          samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

          if (vkCreateSampler(device, &samplerInfo, nullptr, &texture.m_textureSampler) != VK_SUCCESS) {
              throw std::runtime_error("failed to create texture sampler!");
          }
      }


      void updateUniformBuffer(uint32_t currentImage, SwapChain& swapChain, std::vector<Object>& objects ) {
          static auto startTime = std::chrono::high_resolution_clock::now();
          auto currentTime = std::chrono::high_resolution_clock::now();
          float dt = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		startTime = currentTime;

		for( auto& object : objects ) {
	        object.m_ubo.model = glm::rotate(object.m_ubo.model, dt * 1.0f * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	        object.m_ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			object.m_ubo.proj = glm::perspective(glm::radians(45.0f), swapChain.m_swapChainExtent.width / (float) swapChain.m_swapChainExtent.height, 0.1f, 10.0f);
	        object.m_ubo.proj[1][1] *= -1;

	        memcpy(object.m_uniformBuffers.m_uniformBuffersMapped[currentImage], &object.m_ubo, sizeof(object.m_ubo));
		}
      }

      void loadModel( Geometry& geometry, const std::string& MODEL_PATH) {
          tinyobj::attrib_t attrib;
          std::vector<tinyobj::shape_t> shapes;
          std::vector<tinyobj::material_t> materials;
          std::string warn, err;

          if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
              throw std::runtime_error(warn + err);
          }

          std::unordered_map<Vertex, uint32_t> uniqueVertices{};

          for (const auto& shape : shapes) {
              for (const auto& index : shape.mesh.indices) {
                  Vertex vertex{};

                  vertex.pos = {
                      attrib.vertices[3 * index.vertex_index + 0],
                      attrib.vertices[3 * index.vertex_index + 1],
                      attrib.vertices[3 * index.vertex_index + 2]
                  };

                  vertex.texCoord = {
                      attrib.texcoords[2 * index.texcoord_index + 0],
                      1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                  };

                  vertex.color = {1.0f, 1.0f, 1.0f};

                  if (uniqueVertices.count(vertex) == 0) {
                      uniqueVertices[vertex] = static_cast<uint32_t>(geometry.m_vertices.size());
                      geometry.m_vertices.push_back(vertex);
                  }

                  geometry.m_indices.push_back(uniqueVertices[vertex]);
              }
          }
      }

      void createVertexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator
          , VkQueue graphicsQueue, VkCommandPool commandPool, Geometry& geometry) {

          VkDeviceSize bufferSize = sizeof(geometry.m_vertices[0]) * geometry.m_vertices.size();

          VkBuffer stagingBuffer;
          VmaAllocation stagingBufferAllocation;
          VmaAllocationInfo allocInfo;
          createBuffer(physicalDevice, device, vmaAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
              , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
              , VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
              , stagingBuffer, stagingBufferAllocation, &allocInfo);

          MemCopy(device, geometry.m_vertices.data(), allocInfo, bufferSize);

          createBuffer(physicalDevice, device, vmaAllocator, bufferSize
              , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
              , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, geometry.m_vertexBuffer
              , geometry.m_vertexBufferAllocation);

          copyBuffer(device, graphicsQueue, commandPool, stagingBuffer, geometry.m_vertexBuffer, bufferSize);

          destroyBuffer(device, vmaAllocator, stagingBuffer, stagingBufferAllocation);
      }

      void createIndexBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator
          , VkQueue graphicsQueue, VkCommandPool commandPool, Geometry& geometry) {

          VkDeviceSize bufferSize = sizeof(geometry.m_indices[0]) * geometry.m_indices.size();

          VkBuffer stagingBuffer;
          VmaAllocation stagingBufferAllocation;
          VmaAllocationInfo allocInfo;
          createBuffer(physicalDevice, device, vmaAllocator, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
              , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
              , VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
              , stagingBuffer, stagingBufferAllocation, &allocInfo);

          MemCopy(device, geometry.m_indices.data(), allocInfo, bufferSize);

          createBuffer(physicalDevice, device, vmaAllocator, bufferSize
              , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
              , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0
              , geometry.m_indexBuffer, geometry.m_indexBufferAllocation);

          copyBuffer(device, graphicsQueue, commandPool, stagingBuffer, geometry.m_indexBuffer, bufferSize);

          destroyBuffer(device, vmaAllocator, stagingBuffer, stagingBufferAllocation);
      }

      void createUniformBuffers(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator& vmaAllocator
              , UniformBuffers &uniformBuffers) {

          VkDeviceSize bufferSize = sizeof(UniformBufferObject);

          uniformBuffers.m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
          uniformBuffers.m_uniformBuffersAllocation.resize(MAX_FRAMES_IN_FLIGHT);
          uniformBuffers.m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

          for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
              VmaAllocationInfo allocInfo;
              createBuffer(physicalDevice, device, vmaAllocator, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                  , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                  , VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT
                  , uniformBuffers.m_uniformBuffers[i]
                  , uniformBuffers.m_uniformBuffersAllocation[i], &allocInfo);

              uniformBuffers.m_uniformBuffersMapped[i] = allocInfo.pMappedData;
          }
      }

      void createDescriptorSets(VkDevice device, Texture& texture
          , VkDescriptorSetLayout descriptorSetLayout, UniformBuffers& uniformBuffers, VkDescriptorPool descriptorPool
          , std::vector<VkDescriptorSet>& descriptorSets) {

          std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
          VkDescriptorSetAllocateInfo allocInfo{};
          allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
          allocInfo.descriptorPool = descriptorPool;
          allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
          allocInfo.pSetLayouts = layouts.data();

          descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
          if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
              throw std::runtime_error("failed to allocate descriptor sets!");
          }

          for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
              VkDescriptorBufferInfo bufferInfo{};
              bufferInfo.buffer = uniformBuffers.m_uniformBuffers[i];
              bufferInfo.offset = 0;
              bufferInfo.range = sizeof(UniformBufferObject);

              VkDescriptorImageInfo imageInfo{};
              imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
              imageInfo.imageView = texture.m_textureImageView;
              imageInfo.sampler = texture.m_textureSampler;

              std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

              descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
              descriptorWrites[0].dstSet = descriptorSets[i];
              descriptorWrites[0].dstBinding = 0;
              descriptorWrites[0].dstArrayElement = 0;
              descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
              descriptorWrites[0].descriptorCount = 1;
              descriptorWrites[0].pBufferInfo = &bufferInfo;

              descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
              descriptorWrites[1].dstSet = descriptorSets[i];
              descriptorWrites[1].dstBinding = 1;
              descriptorWrites[1].dstArrayElement = 0;
              descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
              descriptorWrites[1].descriptorCount = 1;
              descriptorWrites[1].pImageInfo = &imageInfo;

              vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
          }
      }


    void createObject(VkPhysicalDevice physicalDevice, VkDevice device, VmaAllocator vmaAllocator,
    VkQueue graphicsQueue, VkCommandPool commandPool, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout,
    glm::mat4&& model, std::string modelPath, std::string texturePath, std::vector<Object>& objects) {
        Object object{{model}};
        createTextureImage(physicalDevice, device, vmaAllocator, graphicsQueue, commandPool, texturePath, object.m_texture);
        createTextureImageView(device, object.m_texture);
        createTextureSampler(physicalDevice, device, object.m_texture);
        loadModel(object.m_geometry, modelPath);
        createVertexBuffer(physicalDevice, device, vmaAllocator, graphicsQueue, commandPool, object.m_geometry);
        createIndexBuffer(physicalDevice, device, vmaAllocator, graphicsQueue, commandPool, object.m_geometry);
        createUniformBuffers(physicalDevice, device, vmaAllocator, object.m_uniformBuffers);
        createDescriptorSets(device, object.m_texture, descriptorSetLayout, object.m_uniformBuffers, descriptorPool, object.m_descriptorSets);
        objects.push_back(object);
    }
    )";

        outFile.close();

        SwapchainNode swapchain{id};
        swapchain.generateSwapchain();
    }


    void render() const override {
	    ed::BeginNode(this->id);
	    ImGui::Text("Model");

	    // Draw Pins
		ed::BeginPin(this->id * 10 + 1, ed::PinKind::Output);
        ImGui::Text("*vertex_data");
        ed::EndPin();

        ed::BeginPin(this->id * 10 + 2, ed::PinKind::Output);
        ImGui::Text("*color_data");
        ed::EndPin();

        ed::BeginPin(this->id * 10 + 3, ed::PinKind::Output);
        ImGui::Text("*texture_data");
        ed::EndPin();

	    ed::EndNode();

		for (auto& link : links) {
        	ed::Link(link.id, link.startPin, link.endPin);
    	}

    }
};

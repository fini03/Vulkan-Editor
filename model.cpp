#include "model.h"
#include "vulkan_editor/vulkan_base.h"
#include <vulkan/vulkan_core.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include "libs/tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "libs/stb_image.h"

VkSampler textureSampler = {};

void createTextureSampler(VulkanContext* context) {
    VkSamplerCreateInfo createInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    createInfo.magFilter = VK_FILTER_NEAREST;
    createInfo.minFilter = VK_FILTER_NEAREST;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = createInfo.addressModeU;
    createInfo.addressModeW = createInfo.addressModeU;
    createInfo.mipLodBias = 0.0f;
    createInfo.maxAnisotropy = 1.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = 1.0f;
    vkCreateSampler(context->device, &createInfo, 0, &textureSampler);
}

void loadImage(VulkanContext* context, VulkanImage& textureImage, const char* filename) {
   	int width, height, channels;
   	uint8_t* data = stbi_load(filename, &width, &height, &channels, 4);
   	if(!data) {
  		std::cerr << "Failed to load image data" << std::endl;
   	}

    textureImage.createImage(
  		context,
      	width,
       	height,
       	VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
    );

    textureImage.uploadDataToImage(
    	context,
        data,
        width * height * 4,
        width,
        height,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
    );
    stbi_image_free(data);
}

void Model::createModel(VulkanContext* context, const char* modelPath, const char* texturePath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath)) {
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
            	uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

   	VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
    vertexBuffer.createBuffer(
    	context,
     	vertexBufferSize,
      	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    vertexBuffer.uploadDataToBuffer(context, vertices.data(), vertexBufferSize);

    VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
    indexBuffer.createBuffer(
    	context,
      	indexBufferSize,
      	VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

	indexBuffer.uploadDataToBuffer(context, indices.data(), indexBufferSize);

	if(!attrib.texcoords.empty()) {
		loadImage(context, textureImage, texturePath);
	}

	bindingDescription = Vertex::getBindingDescription();
    attributeDescriptions = Vertex::getAttributeDescriptions();
}

void Model::destroyModel(VulkanContext* context) {
	vkDestroySampler(context->device, textureSampler, nullptr);
 	this->textureImage.destroyImage(context);

    this->vertexBuffer.destroyBuffer(context);
    this->indexBuffer.destroyBuffer(context);

    *this = {};
}

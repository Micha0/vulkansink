#include "vulkan-test.h"
#include "utils/log.h"
#include "utils/fs_android.h"
#include "graphics/wsi.h"
#include "vulkandebug.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <set>
#include <chrono>
#include <thread>

namespace {

    //configuration
    std::vector<const char*> enabledExtensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
    //        "VK_EXT_swapchain_colorspace"
    };

    std::vector<const char*> validationLayers = {
            "VK_LAYER_LUNARG_core_validation",
            "VK_LAYER_LUNARG_swapchain",
            "VK_LAYER_LUNARG_parameter_validation"
    };

    std::vector<const char*> enabledDeviceExtensions = {
            "VK_KHR_swapchain",
    };

    //private vars
    const int MAX_FRAMES_IN_FLIGHT = 2;
    size_t currentFrame = 0;
    bool framebufferResized = false;

    VkInstance instance;
    VkDebugReportCallbackEXT debugReportCallback;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkPhysicalDeviceFeatures deviceFeatures;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    bool debugEnabled = true;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objectType,
        uint64_t object,
        size_t location,
        int32_t messageCode,
        const char* pLayerPrefix,
        const char*  pMessage,
        void* pUserData
    )
    {
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
            __android_log_print(ANDROID_LOG_ERROR,
                                "VkDebug",
                                "ERROR: [%s] Code %i : %s",
                                pLayerPrefix, messageCode, pMessage);
        }
        else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
            __android_log_print(ANDROID_LOG_WARN,
                                "VkDebug",
                                "WARNING: [%s] Code %i : %s",
                                pLayerPrefix, messageCode, pMessage);
        }
        else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        {
            __android_log_print(ANDROID_LOG_WARN,
                                "VkDebug",
                                "PERFORMANCE WARNING: [%s] Code %i : %s",
                                pLayerPrefix, messageCode, pMessage);
        }
        else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
        {
            __android_log_print(ANDROID_LOG_INFO,
                                "VkDebug", "INFO: [%s] Code %i : %s",
                                pLayerPrefix, messageCode, pMessage);
        }
        else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
        {
            __android_log_print(ANDROID_LOG_VERBOSE,
                                "VkDebug", "DEBUG: [%s] Code %i : %s",
                                pLayerPrefix, messageCode, pMessage);
        }

        return VK_FALSE;
    }

    VkResult CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* createInfo, VkDebugReportCallbackEXT* callback)
    {
        auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");

        if (func != nullptr)
        {
            return func(instance, createInfo, nullptr, callback);
        }
        else
        {
            LOGE("Could not find extension %s\n", "vkCreateDebugReportCallbackEXT");
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void DestroyDebugReportCallback(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* allocators)
    {
        auto func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
        if (func != nullptr)
        {
            func(instance, callback, allocators);
        }
    }

    struct QueueFamilyIndices {
        int graphicsFamily = -1;
        int presentFamily = -2;

        bool isComplete()
        {
            return graphicsFamily >= 0 && presentFamily >= 0;
        }
    };

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;

        unsigned int queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (queueFamily.queueCount > 0)
            {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    indices.graphicsFamily = i;
                }
                if (presentSupport)
                {
                    indices.presentFamily = i;
                }
            }

            if (indices.isComplete())
            {
                break;
            }

            ++i;
        }

        return indices;
    }

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        unsigned int formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }
        LOGI("Found %d Supported Formats: ", formatCount);
        for (const auto& format : details.formats)
        {
            LOGI("\t%s", Vulkan::VkFormatToString(format.format));
        }

        unsigned int presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices = findQueueFamilies(device);
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);

        return indices.isComplete() && !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
        //We have all the choices, pick our favorite: 32bits sRGB
        if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
        {
            return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        }

        //We have limited choices, let's see if we can find our favorite: 32bit SRGB
        for (const auto& availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormat;
            }
        }

        //Our favorite is not there .. let's go with the first
        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
    {
        VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

        for (const auto& availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return availablePresentMode;
            }
            else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                bestMode = availablePresentMode;
            }
        }

        return bestMode;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            VkExtent2D actualExtent = {WSI::GetWidth(), WSI::GetHeight()};

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

    bool createSwapChain()
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swapChainCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR
            , .surface = surface
            , .minImageCount = imageCount
            , .imageFormat = surfaceFormat.format
            , .imageColorSpace = surfaceFormat.colorSpace
            , .imageExtent = extent
            , .imageArrayLayers = 1
            , .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
            , .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE
            , .queueFamilyIndexCount = 0
            , .pQueueFamilyIndices = nullptr
            , .preTransform = swapChainSupport.capabilities.currentTransform
            , .compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
            , .presentMode = presentMode
            , .clipped = VK_TRUE
            , .oldSwapchain = VK_NULL_HANDLE
        };

        if (vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapChain) != VK_SUCCESS) {
            return false;
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        if (imageCount == 0)
        {
            return false;
        }

        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;

        return true;
    }

    bool createImageViews()
    {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            VkImageViewCreateInfo createInfo = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
                , .image = swapChainImages[i]
                    , .viewType = VK_IMAGE_VIEW_TYPE_2D
                    , .format = swapChainImageFormat
                    , .components = {
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                    }
                    , .subresourceRange = {
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    }
            };

            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
            {
                return false;
            }
        }

        return true;
    }

    bool createShaderModule(VkShaderModule& shaderModule, const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = code.size(),
            .pCode = reinterpret_cast<const uint32_t*>(code.data())
        };


        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            return false;
        }

        return true;
    }

    bool createRenderPass() {
        VkAttachmentDescription colorAttachment = {
            .format = swapChainImageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };

        VkAttachmentReference colorAttachmentRef = {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };

        VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentRef
        };

        VkSubpassDependency dependency = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        };

        VkRenderPassCreateInfo renderPassInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments = &colorAttachment,
            .subpassCount = 1,
            .pSubpasses = &subpass,
            .dependencyCount = 1,
            .pDependencies = &dependency,
        };

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            LOGE("failed to create render pass!");
            return false;
        }

        return true;
    }

    bool createGraphicsPipeline()
    {
        auto vertShaderFile = Vfs::Open<Vfs::AndroidFile>("shaders/tutorial4.vert.spv");
        auto fragShaderFile = Vfs::Open<Vfs::AndroidFile>("shaders/tutorial4.frag.spv");

        auto vertShaderCode = vertShaderFile->ToBuffer<char>();
        auto fragShaderCode = fragShaderFile->ToBuffer<char>();

        VkShaderModule vertShaderModule;
        VkShaderModule fragShaderModule;

        if (!createShaderModule(vertShaderModule, vertShaderCode)) {
            LOGE("Unable to create vertex shader module");
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
            return false;
        }

        if (!createShaderModule(fragShaderModule, fragShaderCode)) {
            LOGE("Unable to create fragment shader module");
            vkDestroyShaderModule(device, fragShaderModule, nullptr);
            return false;
        }

        //Vertex Shader stage
        VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertShaderModule,
            .pName = "main",
        };

        //Fragment shader stage
        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragShaderModule,
            .pName = "main",
        };

        //Combine the shader stages
        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        //Vertex input
        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 0,
            .pVertexBindingDescriptions = nullptr,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions = nullptr,
        };

        //Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        };

        //viewport
        VkViewport viewport = {
            .x = 0.0f,
            .y = 0.0f,
            .width = (float)swapChainExtent.width,
            .height = (float)swapChainExtent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        VkRect2D scissor = {
            .offset = {0, 0},
            .extent = swapChainExtent
        };

        VkPipelineViewportStateCreateInfo viewportState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissor,
        };

        //Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.0f,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f, // Optional
            .depthBiasClamp = 0.0f, // Optional
            .depthBiasSlopeFactor = 0.0f, // Optional
        };

        //Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .sampleShadingEnable = VK_FALSE,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .minSampleShading = 1.0f, // Optional
            .pSampleMask = nullptr, // Optional
            .alphaToCoverageEnable = VK_FALSE, // Optional
            .alphaToOneEnable = VK_FALSE, // Optional
        };

        //skip depth and stencil

        //Color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
            .colorBlendOp = VK_BLEND_OP_ADD, // Optional
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
            .alphaBlendOp = VK_BLEND_OP_ADD, // Optional
        };

        VkPipelineColorBlendStateCreateInfo colorBlending = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY, // Optional
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment,
            .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}, // Optional
        };

        //Dynamic state
        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_LINE_WIDTH
        };

        VkPipelineDynamicStateCreateInfo dynamicState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates = dynamicStates,
        };

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 0, // Optional
            .pSetLayouts = nullptr, // Optional
            .pushConstantRangeCount = 0, // Optional
            .pPushConstantRanges = nullptr, // Optional
        };

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            LOGE("failed to create pipeline layout!");
            return false;
        }

        VkGraphicsPipelineCreateInfo pipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = 2,
            .pStages = shaderStages,

            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = nullptr, // Optional
            .pColorBlendState = &colorBlending,
            .pDynamicState = nullptr, // Optional

            .layout = pipelineLayout,

            .renderPass = renderPass,
            .subpass = 0,

            .basePipelineHandle = VK_NULL_HANDLE, // Optional
            .basePipelineIndex = -1, // Optional
        };

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            LOGE("failed to create graphics pipeline!");
            return false;
        }

        //cleanup
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        return true;
    }

    bool createFramebuffers()
    {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++)
        {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = renderPass,
                .attachmentCount = 1,
                .pAttachments = attachments,
                .width = swapChainExtent.width,
                .height = swapChainExtent.height,
                .layers = 1,
            };

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                LOGE("failed to create framebuffer!");
                return false;
            }
        }
        return true;
    }

    bool createCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = (uint32_t)queueFamilyIndices.graphicsFamily,
            .flags = 0,
        };

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            LOGE("failed to create command pool!");
            return false;
        }

        return true;
    }

    bool createCommandBuffers()
    {
        commandBuffers.resize(swapChainFramebuffers.size());

        VkCommandBufferAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = (uint32_t) commandBuffers.size(),
        };

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            LOGE("failed to allocate command buffers!");
            return false;
        }

        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

        for (size_t i = 0; i < commandBuffers.size(); i++)
        {
            VkCommandBufferBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
                .pInheritanceInfo = nullptr, // Optional
            };

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
            {
                LOGE("failed to begin recording command buffer!");
                return false;
            }

            VkRenderPassBeginInfo renderPassInfo = {
                .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass = renderPass,
                .framebuffer = swapChainFramebuffers[i],
                .renderArea = {
                    .offset = {0, 0},
                    .extent = swapChainExtent,
                },
                .clearValueCount = 1,
                .pClearValues = &clearColor,
            };

            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
            vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
            vkCmdEndRenderPass(commandBuffers[i]);

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
            {
                LOGE("failed to record command buffer!");
                return false;
            }
        }

        return true;
    }

    bool createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VkFenceCreateInfo fenceInfo = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) !=
                VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) !=
                VK_SUCCESS || vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

                LOGE("failed to create sync objects!");
                return false;
            }
        }

        return true;
    }

    void cleanupSwapChain() {

        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        //vkDestroyCommandPool(device, commandPool, nullptr);

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    bool recreateSwapChain() {
        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        if (!createSwapChain())
        {
            LOGE("Failed to create the swap chain!");
            return false;
        }

        if (!createImageViews())
        {
            LOGE("Failed to create the image views");
            return false;
        }

        //Render passes
        if (!createRenderPass())
        {
            LOGE("Failed to create the render pass!");
            return false;
        }

        //Graphics pipeline
        if (!createGraphicsPipeline())
        {
            LOGE("Failed to create the graphics pipeline");
            return false;
        }

        //Framebuffers
        if (!createFramebuffers())
        {
            LOGE("Failed to create the framebuffers");
            return false;
        }

        //Command Pool
        if (!createCommandPool())
        {
            LOGE("failed to create a commandpool");

            return false;
        }

        //command buffer
        if (!createCommandBuffers())
        {
            LOGE("failed to create a commandbuffer");

            return false;
        }

        return true;
    }

    bool createDeviceRelatives()
    {
        //Physical
        unsigned int deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        LOGI("Found %d Devices", deviceCount);
        for (const auto& dev : devices)
        {
            if (isDeviceSuitable(dev))
            {
                physicalDevice = dev;
            }
        }

        if (!physicalDevice)
        {
            LOGE("Unable to find a physical device!");
            return false;
        }

        //Device
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<int> uniqueQueueFamilies {indices.graphicsFamily, indices.presentFamily, indices.presentFamily};

        float queuePriority[] = {
                1.0f
        };
        for (int queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {
                    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
                    , .pNext = nullptr
                    , .flags = 0
                    , .queueFamilyIndex = (uint32_t)queueFamily
                    , .queueCount = 1
                    , .pQueuePriorities = queuePriority
            };

            queueCreateInfos.push_back(queueCreateInfo);
        }

        unsigned int deviceExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, nullptr);
        std::vector<VkExtensionProperties> availableDeviceExtensions(deviceExtensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &deviceExtensionCount, availableDeviceExtensions.data());

        LOGI("Found %d Device Extensions: ", deviceExtensionCount);
        for (const auto& devExt : availableDeviceExtensions)
        {
            LOGI("\t%s", devExt.extensionName);
        }

        VkDeviceCreateInfo deviceCreateInfo = {
                .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
                , .pNext = nullptr
                , .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size())
                , .pQueueCreateInfos = queueCreateInfos.data()
                , .pEnabledFeatures = &deviceFeatures
                , .enabledExtensionCount = (uint32_t)enabledDeviceExtensions.size()
                , .ppEnabledExtensionNames = enabledDeviceExtensions.data()
                , .enabledLayerCount = (uint32_t)validationLayers.size()
                , .ppEnabledLayerNames = validationLayers.data()
        };

        if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
        {
            LOGE("Failed to create logic device!");
            return false;
        }

        vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
        vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);

        return true;
    }

    bool createFromSurface(ANativeWindow* window)
    {
        //Surface
        WSI::Initialize(instance, window);
        surface = WSI::GetSurface();

        if (!createDeviceRelatives())
        {
            LOGE("Failed to create the device relatives!");
            return false;
        }

        if (!createSwapChain())
        {
            LOGE("Failed to create the swap chain!");
            return false;
        }

        if (!createImageViews())
        {
            LOGE("Failed to create the image views");
            return false;
        }

        //Render passes
        if (!createRenderPass())
        {
            LOGE("Failed to create the render pass!");
            return false;
        }

        //Graphics pipeline
        if (!createGraphicsPipeline())
        {
            LOGE("Failed to create the graphics pipeline");
            return false;
        }

        //Framebuffers
        if (!createFramebuffers())
        {
            LOGE("Failed to create the framebuffers");
            return false;
        }

        //Command Pool
        if (!createCommandPool())
        {
            LOGE("failed to create a commandpool");

            return false;
        }

        //command buffer
        if (!createCommandBuffers())
        {
            LOGE("failed to create a commandbuffer");

            return false;
        }

        //semaphores + fences
        if (!createSyncObjects())
        {
            LOGE("failed to create semaphores and/or fences");

            return false;
        }

        return true;
    }

    void cleanupFromSurface()
    {
        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        vkDestroyDevice(device, nullptr);

        WSI::Destroy(instance);

        surface = 0;
    }
}

bool Vulkan::Initialize(ANativeWindow* window) {
    unsigned int extensionCount = 0;
    unsigned int availableLayerCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> supportedExtensions(extensionCount);

    if (debugEnabled) {
        enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    enabledExtensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);

    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, supportedExtensions.data());

    LOGI("Vulkan: %d extensions supported! Extensions:", extensionCount);
    for (const auto &ext : supportedExtensions) {
        LOGI("\t%s", ext.extensionName);
    }

    vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(availableLayerCount);
    vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data());

    LOGI("Vulkan: %d layers available! Layers:", availableLayerCount);
    for (const auto &lyr : availableLayers) {
        LOGI("\t%s", lyr.layerName);
    }

    VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "VulkanSink",
            .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
            .pEngineName = "Hardcore Homebuilt",
            .engineVersion = VK_MAKE_VERSION(0, 0, 0),
            .apiVersion = VK_API_VERSION_1_0
    };

    VkInstanceCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &appInfo, .enabledExtensionCount = (uint32_t) enabledExtensions.size(), .ppEnabledExtensionNames = enabledExtensions.data(), .enabledLayerCount = (uint32_t) validationLayers.size(), .ppEnabledLayerNames = validationLayers.data()
    };

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        LOGE("Failed to create instance\n");
        return false;
    }

    if (debugEnabled) {
        VkDebugReportCallbackCreateInfoEXT callbackInfo = {
                .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
                .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                         VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                .pfnCallback = debugCallback
        };

        if (CreateDebugReportCallbackEXT(instance, &callbackInfo, &debugReportCallback) !=
            VK_SUCCESS) {
            LOGE("Unable to setup debug report callback");
            return false;
        }
    }

    if (!createFromSurface(window))
    {
        LOGE("Unable to initialize in the surface step");
        return false;
    }

    return true;
}

void Vulkan::Draw() {
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(),
                          imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS) {
        LOGE("failed to acquire swap chain image!");
        return;
    }

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = waitSemaphores,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffers[imageIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = signalSemaphores,
    };

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        LOGE("failed to submit draw command buffer!");
        return;
    }

    VkSwapchainKHR swapChains[] = {swapChain};

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,

        .waitSemaphoreCount = 1,
        .pWaitSemaphores = signalSemaphores,

        .swapchainCount = 1,
        .pSwapchains = swapChains,
        .pImageIndices = &imageIndex,
        .pResults = nullptr, // Optional
    };

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        LOGE("failed to present swap chain image!");
        return;
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Vulkan::ReleaseSurface()
{
    LOGI("Vulkan::ReleaseSurface");
    cleanupFromSurface();
}

void Vulkan::ReAcquireSurface(ANativeWindow* window)
{
    LOGI("Vulkan::ReAcquireSurface");
    framebufferResized = true;

    if (!createFromSurface(window))
    {
        LOGE("Unable to initialize in the surface step");
    }
}

void Vulkan::Destroy()
{
    cleanupFromSurface();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    if (debugEnabled) {
        DestroyDebugReportCallback(instance, debugReportCallback, nullptr);
    }


	vkDestroyInstance(instance, nullptr);
}

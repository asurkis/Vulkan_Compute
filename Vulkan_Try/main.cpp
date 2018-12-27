#include "stdafx.h"
#include "Const.h"

#include "vert.spv"
#include "frag.spv"
#include "comp.spv"

bool shouldResize = false;

struct Particle
{
    float pos[2];
    float vel[2];
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_CLOSE:
        PostQuitMessage(0);
        break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
            PostQuitMessage(0);
        break;

    case WM_SIZE:
        shouldResize = true;
        break;

    default:
        return DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    return 0;
}

HWND window = NULL;
HINSTANCE hInstance = NULL;

int windowWidth = 800;
int windowHeight = 800;

void InitWindows()
{
    hInstance = GetModuleHandle(NULL);

    WNDCLASS wc = {};
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.hCursor = LoadCursor(hInstance, IDC_ARROW);
    wc.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wc.hInstance = hInstance;
    wc.lpfnWndProc = &WndProc;
    wc.lpszClassName = WINDOW_CLASS_NAME;
    wc.style = CS_HREDRAW | CS_VREDRAW;

    ATOM result = RegisterClass(&wc);
    assert(result);

    RECT rect = {};
    rect.left = (GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2;
    rect.top = (GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2;
    rect.right = rect.left + windowWidth;
    rect.bottom = rect.top + windowHeight;

    DWORD dwStyle = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&rect, dwStyle, FALSE);

    window = CreateWindow(WINDOW_CLASS_NAME, _T("WindowName1"),
                          dwStyle, rect.left, rect.top,
                          rect.right - rect.left,
                          rect.bottom - rect.top,
                          HWND_DESKTOP, NULL, hInstance, NULL);
    assert(window);
}

VkInstance instance = NULL;

void InitInstance()
{
    VkResult result;

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_API_VERSION_1_0;
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pApplicationName = "ApplicationName1";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "EngineName1";

    const char *const extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#ifndef NDEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = sizeof extensions / sizeof extensions[0];
    instanceInfo.ppEnabledExtensionNames = extensions;
#ifndef NDEBUG
    instanceInfo.enabledLayerCount = sizeof layers / sizeof layers[0];
    instanceInfo.ppEnabledLayerNames = layers;
#endif
    // instanceInfo.flags = 0;

    result = vkCreateInstance(&instanceInfo, NULL, &instance);
    assert(!result);
}

#ifndef NDEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                   VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                   const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                   void *pUserData)
{
    fprintf(stderr, "[vk] %s\n", pCallbackData->pMessage);
    return VK_FALSE;
}

VkDebugUtilsMessengerEXT debugCallback;
#endif

void InitDebugCallback()
{
#ifndef NDEBUG
    VkDebugUtilsMessengerCreateInfoEXT callbackInfo = {};
    callbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    callbackInfo.messageSeverity = 0
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

    callbackInfo.messageType = 0
        | VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

    callbackInfo.pfnUserCallback = &VulkanDebugCallback;

    auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    assert(fn);
    fn(instance, &callbackInfo, NULL, &debugCallback);
#endif
}

void DisposeDebugCallback()
{
#ifndef NDEBUG
    auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    assert(fn);
    fn(instance, debugCallback, NULL);
#endif
}

VkSurfaceKHR surface;

void InitSurface()
{
    VkResult result;

    VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hwnd = window;
    surfaceInfo.hinstance = hInstance;

    result = vkCreateWin32SurfaceKHR(instance, &surfaceInfo, NULL, &surface);
    assert(!result);
}

VkPhysicalDevice physicalDevice = NULL;
VkPhysicalDeviceMemoryProperties memoryProperties = {};
uint32_t queueFamily = 0;
VkDevice device = NULL;
VkQueue queue = NULL;

void InitDevice()
{
    VkResult result;
    uint32_t physicalDeviceCount;
    result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL);
    assert(!result);
    assert(0 != physicalDeviceCount);
    VkPhysicalDevice *physicalDevices = new VkPhysicalDevice[physicalDeviceCount];
    result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices);
    assert(!result);

    for (uint32_t i = 0; i < physicalDeviceCount; i++)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);
        if (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU != deviceProperties.deviceType)
            continue;

        uint32_t queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, NULL);
        assert(0 != queueFamilyCount);
        VkQueueFamilyProperties *properties = new VkQueueFamilyProperties[queueFamilyCount];
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, properties);

        uint32_t queueFamily = ~0;

        for (uint32_t j = 0; j < queueFamilyCount; j++)
        {
            VkBool32 presentSupported;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[i], j, surface, &presentSupported);
            if (!presentSupported) continue;
            if (VK_QUEUE_COMPUTE_BIT != (VK_QUEUE_COMPUTE_BIT & properties[j].queueFlags)) continue;
            if (VK_QUEUE_GRAPHICS_BIT != (VK_QUEUE_GRAPHICS_BIT & properties[j].queueFlags)) continue;

            if (!~queueFamily || properties[queueFamily].queueCount < properties[j].queueCount)
                queueFamily = j;
        }

        delete[] properties;

        if (!~queueFamily) continue;

        ::queueFamily = queueFamily;
        physicalDevice = physicalDevices[i];
        break;
    }
    delete[] physicalDevices;

    assert(physicalDevice);

    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    float priorities[] = { 0.0f };

    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = queueFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = priorities;

    VkPhysicalDeviceFeatures features = {};
    // vkGetPhysicalDeviceFeatures(physicalDevice, &features);

    const char *const extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.enabledExtensionCount = sizeof extensions / sizeof extensions[0];
    deviceInfo.ppEnabledExtensionNames = extensions;
#ifndef NDEBUG
    deviceInfo.enabledLayerCount = sizeof layers / sizeof layers[0];
    deviceInfo.ppEnabledLayerNames = layers;
#endif
    deviceInfo.pEnabledFeatures = &features;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;

    result = vkCreateDevice(physicalDevice, &deviceInfo, NULL, &device);
    assert(!result);

    vkGetDeviceQueue(device, queueFamily, 0, &queue);
}

VkCommandPool commandPool;

void InitCommandPool()
{
    VkResult result;

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamily;

    result = vkCreateCommandPool(device, &poolInfo, NULL, &commandPool);
    assert(!result);
}

VkSwapchainKHR swapchain = NULL;
uint32_t swapchainImageCount = 0;
VkImage *swapchainImages = NULL;
VkImageView *swapchainImageViews = NULL;

void InitSwapchain()
{
    VkResult result;

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &caps);

    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = surface;
    swapchainInfo.minImageCount = caps.minImageCount;
    swapchainInfo.imageFormat = SWAPCHAIN_IMAGE_FORMAT;
    swapchainInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swapchainInfo.imageExtent = caps.currentExtent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 1;
    swapchainInfo.pQueueFamilyIndices = &queueFamily;
    swapchainInfo.preTransform = caps.currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    // swapchainInfo.clipped = VK_FALSE;

    result = vkCreateSwapchainKHR(device, &swapchainInfo, NULL, &swapchain);
    assert(!result);

    result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL);
    assert(!result);
    assert(0 != swapchainImageCount);
    swapchainImages = new VkImage[swapchainImageCount];
    result = vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages);
    assert(!result);

    swapchainImageViews = new VkImageView[swapchainImageCount];
    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = SWAPCHAIN_IMAGE_FORMAT;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;

        result = vkCreateImageView(device, &viewInfo, NULL, &swapchainImageViews[i]);
        assert(!result);
    }
}

VkShaderModule vertModule = NULL;
VkShaderModule fragModule = NULL;
VkShaderModule compModule = NULL;

void InitShaders()
{
    VkResult result;

    VkShaderModuleCreateInfo shaderInfo = {};
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    shaderInfo.codeSize = sizeof vertBytecode;
    shaderInfo.pCode = vertBytecode;
    result = vkCreateShaderModule(device, &shaderInfo, NULL, &vertModule);
    assert(!result);

    shaderInfo.codeSize = sizeof fragBytecode;
    shaderInfo.pCode = fragBytecode;
    result = vkCreateShaderModule(device, &shaderInfo, NULL, &fragModule);
    assert(!result);

    shaderInfo.codeSize = sizeof compBytecode;
    shaderInfo.pCode = compBytecode;
    result = vkCreateShaderModule(device, &shaderInfo, NULL, &compModule);
    assert(!result);
}

VkBuffer particleBuffer = NULL;
VkDeviceMemory particleBufferMemory = NULL;
VkBuffer uniformBuffer = NULL;
VkDeviceMemory uniformBufferMemory = NULL;

uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((typeFilter & (1 << i)) && properties == (properties & memoryProperties.memoryTypes[i].propertyFlags))
            return i;
    }
    assert(false);
    return ~0;
}

void CreateOneBuffer(VkBuffer *pBuffer, VkDeviceMemory *pMemory,
                     VkDeviceSize size, VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags memoryProperties)
{
    VkResult result;

    VkBufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = size;
    createInfo.usage = usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    result = vkCreateBuffer(device, &createInfo, NULL, pBuffer);
    assert(!result);

    VkMemoryRequirements reqs;
    vkGetBufferMemoryRequirements(device, *pBuffer, &reqs);

    VkMemoryAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = reqs.size;
    allocateInfo.memoryTypeIndex = FindMemoryType(reqs.memoryTypeBits, memoryProperties);

    result = vkAllocateMemory(device, &allocateInfo, NULL, pMemory);
    assert(!result);

    vkBindBufferMemory(device, *pBuffer, *pMemory, 0);
}

void FillParticleBuffer()
{
    VkResult result;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    CreateOneBuffer(&stagingBuffer, &stagingBufferMemory,
                    PARTICLE_COUNT * sizeof(Particle),
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    0
                    | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *data;
    result = vkMapMemory(device, stagingBufferMemory, 0, VK_WHOLE_SIZE, 0, &data);
    assert(!result);
    float *floatPtr = (float *)data;
    for (int i = 0; i < PARTICLE_COUNT; i++)
    {
        floatPtr[4 * i + 0] = 2.0f * rand() / RAND_MAX - 1.0f;
        floatPtr[4 * i + 1] = 2.0f * rand() / RAND_MAX - 1.0f;
        floatPtr[4 * i + 2] = 0.0f;
        floatPtr[4 * i + 3] = 0.0f;
    }
    vkUnmapMemory(device, stagingBufferMemory);

    VkCommandBufferAllocateInfo cmdInfo = {};
    cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdInfo.commandPool = commandPool;
    cmdInfo.commandBufferCount = 1;

    VkCommandBuffer cmdBuf;
    result = vkAllocateCommandBuffers(device, &cmdInfo, &cmdBuf);
    assert(!result);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    result = vkBeginCommandBuffer(cmdBuf, &beginInfo);
    assert(!result);
    VkBufferCopy copyRegion = {};
    copyRegion.dstOffset = 0;
    copyRegion.srcOffset = 0;
    copyRegion.size = PARTICLE_COUNT * sizeof(Particle);
    vkCmdCopyBuffer(cmdBuf, stagingBuffer, particleBuffer, 1, &copyRegion);
    result = vkEndCommandBuffer(cmdBuf);
    assert(!result);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    result = vkQueueSubmit(queue, 1, &submitInfo, NULL);
    assert(!result);
    result = vkQueueWaitIdle(queue);
    assert(!result);

    vkFreeCommandBuffers(device, commandPool, 1, &cmdBuf);
    vkDestroyBuffer(device, stagingBuffer, NULL);
    vkFreeMemory(device, stagingBufferMemory, NULL);

}

void InitBuffers()
{
    CreateOneBuffer(&particleBuffer, &particleBufferMemory,
                    PARTICLE_COUNT * sizeof(Particle),
                    0
                    | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                    | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                    | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    FillParticleBuffer();

    CreateOneBuffer(&uniformBuffer, &uniformBufferMemory, 4 * sizeof(float),
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    0
                    | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                    | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

VkDescriptorPool descriptorPool;

void InitDescriptorPool()
{
    VkResult result;

    VkDescriptorPoolSize poolSizes[2];

    poolSizes[0] = {};
    poolSizes[0].descriptorCount = 1;
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    poolSizes[1] = {};
    poolSizes[1].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = swapchainImageCount;

    result = vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptorPool);
    assert(!result);
}

VkDescriptorSetLayout *descriptorSetLayouts;
VkDescriptorSet *descriptorSets;

void InitDescriptors()
{
    VkResult result;

    VkDescriptorSetLayoutBinding bindings[2];

    bindings[0] = {};
    bindings[0].binding = 0;
    bindings[0].descriptorCount = 1;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1] = {};
    bindings[1].binding = 1;
    bindings[1].descriptorCount = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings;

    descriptorSetLayouts = new VkDescriptorSetLayout[swapchainImageCount];
    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        result = vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &descriptorSetLayouts[i]);
        assert(!result);
    }

    VkDescriptorSetAllocateInfo setInfo = {};
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setInfo.descriptorPool = descriptorPool;
    setInfo.descriptorSetCount = swapchainImageCount;
    setInfo.pSetLayouts = descriptorSetLayouts;

    descriptorSets = new VkDescriptorSet[swapchainImageCount];
    result = vkAllocateDescriptorSets(device, &setInfo, descriptorSets);
    assert(!result);

    VkDescriptorBufferInfo bufferInfos[2];

    bufferInfos[0] = {};
    bufferInfos[0].buffer = uniformBuffer;
    bufferInfos[0].offset = 0;
    bufferInfos[0].range = VK_WHOLE_SIZE;

    bufferInfos[1] = {};
    bufferInfos[1].buffer = particleBuffer;
    bufferInfos[1].offset = 0;
    bufferInfos[1].range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet descriptorWrites[2];

    descriptorWrites[0] = {};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = 0;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfos[0];

    descriptorWrites[1] = {};
    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = 0;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &bufferInfos[1];

    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[1].dstSet = descriptorSets[i];
        vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, NULL);
    }
}

VkPipelineLayout pipelineLayout = NULL;

void InitPipelineLayout()
{
    VkResult result;

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = swapchainImageCount;
    layoutInfo.pSetLayouts = descriptorSetLayouts;

    result = vkCreatePipelineLayout(device, &layoutInfo, NULL, &pipelineLayout);
    assert(!result);
}

VkRenderPass renderPass = NULL;

void InitRenderPass()
{
    VkResult result;

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = SWAPCHAIN_IMAGE_FORMAT;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;


    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = 0
        | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
        | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    result = vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass);
    assert(!result);
}

VkPipeline graphicsPipeline = NULL;

void InitGraphicsPipeline()
{
    VkResult result;

    VkPipelineShaderStageCreateInfo graphicsStageInfos[2];

    graphicsStageInfos[0] = {};
    graphicsStageInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    graphicsStageInfos[0].pName = "main";
    graphicsStageInfos[0].module = vertModule;
    graphicsStageInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;

    graphicsStageInfos[1] = {};
    graphicsStageInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    graphicsStageInfos[1].pName = "main";
    graphicsStageInfos[1].module = fragModule;
    graphicsStageInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkVertexInputAttributeDescription vertexAttributeDescription = {};
    vertexAttributeDescription.binding = 0;
    vertexAttributeDescription.location = 0;
    vertexAttributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
    vertexAttributeDescription.offset = offsetof(Particle, pos);

    VkVertexInputBindingDescription vertexBindingDescription = {};
    vertexBindingDescription.binding = 0;
    vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexBindingDescription.stride = sizeof(Particle);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexAttributeDescriptionCount = 1;
    vertexInputInfo.pVertexAttributeDescriptions = &vertexAttributeDescription;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; // TODO: change
    // inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float)windowWidth;
    viewport.height = (float)windowHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = windowWidth;
    scissor.extent.height = windowHeight;

    VkPipelineViewportStateCreateInfo viewportInfo = {};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizerInfo = {};
    rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL; // TODO: change
    rasterizerInfo.lineWidth = 1.0f;
    // rasterizerInfo.depthClampEnable = VK_FALSE;
    // rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
    // // rasterizerInfo.cullMode = VK_CULL_MODE_NONE;
    // // rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    // rasterizerInfo.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
    multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    // multisampleInfo.sampleShadingEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState blendAttachment = {};
    blendAttachment.colorWriteMask = 0
        | VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;
    // blendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo blendInfo = {};
    blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendInfo.attachmentCount = 1;
    blendInfo.pAttachments = &blendAttachment;
    // blendInfo.logicOpEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = graphicsStageInfos;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportInfo;
    pipelineInfo.pRasterizationState = &rasterizerInfo;
    pipelineInfo.pMultisampleState = &multisampleInfo;
    pipelineInfo.pColorBlendState = &blendInfo;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    result = vkCreateGraphicsPipelines(device, NULL, 1, &pipelineInfo, NULL, &graphicsPipeline);
    assert(!result);
}

VkPipeline computePipeline = NULL;

void InitComputePipeline()
{
    VkResult result;

    VkComputePipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipelineInfo.stage.pName = "main";
    pipelineInfo.stage.module = compModule;
    pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;

    result = vkCreateComputePipelines(device, NULL, 1, &pipelineInfo, NULL, &computePipeline);
    assert(!result);
}

VkFramebuffer *framebuffers;

void InitFramebuffers()
{
    VkResult result;

    framebuffers = new VkFramebuffer[swapchainImageCount];

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = windowWidth;
    framebufferInfo.height = windowHeight;
    framebufferInfo.layers = 1;

    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        framebufferInfo.pAttachments = &swapchainImageViews[i];
        result = vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffers[i]);
        assert(!result);
    }
}

VkCommandBuffer *commandBuffers;

void InitCommandBuffers()
{
    VkResult result;

    commandBuffers = new VkCommandBuffer[swapchainImageCount];
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandBufferCount = swapchainImageCount;
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    result = vkAllocateCommandBuffers(device, &allocateInfo, commandBuffers);
    assert(!result);

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent.width = windowWidth;
    renderPassBeginInfo.renderArea.extent.height = windowHeight;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;

    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        renderPassBeginInfo.framebuffer = framebuffers[i];

        result = vkBeginCommandBuffer(commandBuffers[i], &commandBufferBeginInfo);
        assert(!result);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSets[i], 0, NULL);
        vkCmdDispatch(commandBuffers[i], PARTICLE_COUNT / 64, 1, 1);

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &particleBuffer, &offset);
        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, NULL);
        vkCmdDraw(commandBuffers[i], PARTICLE_COUNT, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffers[i]);

        result = vkEndCommandBuffer(commandBuffers[i]);
        assert(!result);
    }
}

VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
VkFence fences[MAX_FRAMES_IN_FLIGHT];

void InitSync()
{
    VkResult result;

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        result = vkCreateSemaphore(device, &semaphoreInfo, NULL, &imageAvailableSemaphores[i]);
        assert(!result);

        result = vkCreateSemaphore(device, &semaphoreInfo, NULL, &renderFinishedSemaphores[i]);
        assert(!result);

        result = vkCreateFence(device, &fenceInfo, NULL, &fences[i]);
        assert(!result);
    }
}

void DisposeSwapchain()
{
    vkFreeCommandBuffers(device, commandPool, swapchainImageCount, commandBuffers);
    for (uint32_t i = 0; i < swapchainImageCount; i++)
        vkDestroyFramebuffer(device, framebuffers[i], NULL);
    delete[] framebuffers;
    vkDestroyPipeline(device, graphicsPipeline, NULL);
    vkDestroyPipelineLayout(device, pipelineLayout, NULL);
    vkDestroyRenderPass(device, renderPass, NULL);
    for (uint32_t i = 0; i < swapchainImageCount; i++)
        vkDestroyImageView(device, swapchainImageViews[i], NULL);
    delete[] swapchainImageViews;
    delete[] swapchainImages;
    vkDestroySwapchainKHR(device, swapchain, NULL);
}

void RecreateSwapchain()
{
    RECT rect;
    GetClientRect(window, &rect);
    windowWidth = rect.right - rect.left;
    windowHeight = rect.bottom - rect.top;

    if (0 == windowWidth || 0 == windowHeight)
        return;

    vkDeviceWaitIdle(device);
    DisposeSwapchain();

    InitSwapchain();
    InitRenderPass();
    InitPipelineLayout();
    InitGraphicsPipeline();
    InitFramebuffers();
    InitCommandBuffers();

    shouldResize = false;
}

clock_t frameTime[FRAME_TIME_COUNT];
size_t currentFrameTime = 0;

float CalcFrameTime()
{
    frameTime[currentFrameTime] = clock();
    currentFrameTime++;
    clock_t delta = frameTime[currentFrameTime - 1] - frameTime[currentFrameTime % FRAME_TIME_COUNT];
    currentFrameTime %= FRAME_TIME_COUNT;
    return (float)delta / (float)(CLOCKS_PER_SEC * FRAME_TIME_COUNT);
}

size_t currentFlightFrame = 0;

void Frame()
{
    VkResult result;

    if (shouldResize)
    {
        RecreateSwapchain();
        return;
    }

    float avgFrameTime = CalcFrameTime();
    char buf[256];
    sprintf_s(buf, "framerate: %.1f", 1.0f / avgFrameTime);
    SetWindowTextA(window, buf);

    result = vkWaitForFences(device, 1, &fences[currentFlightFrame], VK_TRUE, UINT64_MAX);
    assert(!result);

    void *data;
    result = vkMapMemory(device, uniformBufferMemory, 0, VK_WHOLE_SIZE, 0, &data);
    assert(!result);
    float *floatPtr = (float *)data;
    floatPtr[0] = (float)windowWidth;
    floatPtr[1] = (float)windowHeight;
    floatPtr[2] = avgFrameTime;
    vkUnmapMemory(device, uniformBufferMemory);

    uint32_t imageIndex;
    result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphores[currentFlightFrame], NULL, &imageIndex);
    assert(!result || VK_ERROR_OUT_OF_DATE_KHR == result || VK_SUBOPTIMAL_KHR == result);
    if (VK_ERROR_OUT_OF_DATE_KHR == result)
    {
        shouldResize = true;
        return;
    }

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFlightFrame];
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFlightFrame];

    result = vkResetFences(device, 1, &fences[currentFlightFrame]);
    assert(!result);

    result = vkQueueSubmit(queue, 1, &submitInfo, fences[currentFlightFrame]);
    assert(!result);

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFlightFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(queue, &presentInfo);
    assert(!result || VK_ERROR_OUT_OF_DATE_KHR == result || VK_SUBOPTIMAL_KHR == result);

    // result = vkQueueWaitIdle(queue);
    // assert(!result);
    currentFlightFrame = (currentFlightFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

int main()
{
    InitWindows();
    InitInstance();
    InitDebugCallback();
    InitSurface();
    InitDevice();
    InitCommandPool();
    InitSwapchain();
    InitShaders();
    InitBuffers();
    InitDescriptorPool();
    InitDescriptors();
    InitPipelineLayout();
    InitRenderPass();
    InitGraphicsPipeline();
    InitComputePipeline();
    InitFramebuffers();
    InitCommandBuffers();
    InitSync();

    ShowWindow(window, SW_SHOW);

    bool shouldRun = true;
    while (shouldRun)
    {
        Frame();

        MSG msg;
        while (shouldRun && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) shouldRun = false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    vkDeviceWaitIdle(device);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], NULL);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], NULL);
        vkDestroyFence(device, fences[i], NULL);
    }
    DisposeSwapchain();
    vkDestroyPipeline(device, computePipeline, NULL);
    vkFreeDescriptorSets(device, descriptorPool, swapchainImageCount, descriptorSets);
    delete[] descriptorSets;
    for (uint32_t i = 0; i < swapchainImageCount; i++)
        vkDestroyDescriptorSetLayout(device, descriptorSetLayouts[i], NULL);
    delete[] descriptorSetLayouts;
    vkDestroyCommandPool(device, commandPool, NULL);
    vkDestroyDescriptorPool(device, descriptorPool, NULL);
    vkDestroyBuffer(device, uniformBuffer, NULL);
    vkFreeMemory(device, uniformBufferMemory, NULL);
    vkDestroyBuffer(device, particleBuffer, NULL);
    vkFreeMemory(device, particleBufferMemory, NULL);
    vkDestroyShaderModule(device, vertModule, NULL);
    vkDestroyShaderModule(device, fragModule, NULL);
    vkDestroyShaderModule(device, compModule, NULL);
    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    DisposeDebugCallback();
    vkDestroyInstance(instance, NULL);
    DestroyWindow(window);
    UnregisterClass(WINDOW_CLASS_NAME, hInstance);
}

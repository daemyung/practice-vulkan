// MIT License
//
// Copyright (c) 2024 Daemyung Jang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cassert>
#include <array>
#include <vector>
#include <iomanip>

#include "VkRenderer.h"
#include "VkUtil.h"
#include "AndroidOut.h"

using namespace std;

VkRenderer::VkRenderer(ANativeWindow *window) {
    // ================================================================================
    // 1. VkInstance 생성
    // ================================================================================
    VkApplicationInfo applicationInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Practice Vulkan",
        .applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0),
        .apiVersion = VK_MAKE_API_VERSION(0, 1, 3, 0)
    };

    uint32_t instanceLayerCount;
    VK_CHECK_ERROR(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));

    vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
    VK_CHECK_ERROR(vkEnumerateInstanceLayerProperties(&instanceLayerCount,
                                                      instanceLayerProperties.data()));

    vector<const char *> instanceLayerNames;
    for (const auto &properties: instanceLayerProperties) {
        instanceLayerNames.push_back(properties.layerName);
    }

    uint32_t instanceExtensionCount;
    VK_CHECK_ERROR(vkEnumerateInstanceExtensionProperties(nullptr,
                                                          &instanceExtensionCount,
                                                          nullptr));

    vector<VkExtensionProperties> instanceExtensionProperties(instanceExtensionCount);
    VK_CHECK_ERROR(vkEnumerateInstanceExtensionProperties(nullptr,
                                                          &instanceExtensionCount,
                                                          instanceExtensionProperties.data()));

    vector<const char *> instanceExtensionNames;
    for (const auto &properties: instanceExtensionProperties) {
        if (properties.extensionName == string("VK_KHR_surface") ||
            properties.extensionName == string("VK_KHR_android_surface")) {
            instanceExtensionNames.push_back(properties.extensionName);
        }
    }
    assert(instanceExtensionNames.size() == 2);

    VkInstanceCreateInfo instanceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = static_cast<uint32_t>(instanceLayerNames.size()),
        .ppEnabledLayerNames = instanceLayerNames.data(),
        .enabledExtensionCount = static_cast<uint32_t>(instanceExtensionNames.size()),
        .ppEnabledExtensionNames = instanceExtensionNames.data()
    };

    VK_CHECK_ERROR(vkCreateInstance(&instanceCreateInfo, nullptr, &mInstance));

    // ================================================================================
    // 2. VkPhysicalDevice 선택
    // ================================================================================
    uint32_t physicalDeviceCount;
    VK_CHECK_ERROR(vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, nullptr));

    vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    VK_CHECK_ERROR(vkEnumeratePhysicalDevices(mInstance,
                                              &physicalDeviceCount,
                                              physicalDevices.data()));

    mPhysicalDevice = physicalDevices[0];

    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(mPhysicalDevice, &physicalDeviceProperties);

    aout << "Selected Physical Device Information ↓" << endl;
    aout << setw(16) << left << " - Device Name: "
         << string_view(physicalDeviceProperties.deviceName) << endl;
    aout << setw(16) << left << " - Device Type: "
         << vkToString(physicalDeviceProperties.deviceType) << endl;
    aout << std::hex;
    aout << setw(16) << left << " - Device ID: " << physicalDeviceProperties.deviceID << endl;
    aout << setw(16) << left << " - Vendor ID: " << physicalDeviceProperties.vendorID << endl;
    aout << std::dec;
    aout << setw(16) << left << " - API Version: "
         << VK_API_VERSION_MAJOR(physicalDeviceProperties.apiVersion) << "."
         << VK_API_VERSION_MINOR(physicalDeviceProperties.apiVersion);
    aout << setw(16) << left << " - Driver Version: "
         << VK_API_VERSION_MAJOR(physicalDeviceProperties.driverVersion) << "."
         << VK_API_VERSION_MINOR(physicalDeviceProperties.driverVersion);

    // ================================================================================
    // 3. VkDevice 생성
    // ================================================================================
    uint32_t queueFamilyPropertiesCount;
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyPropertiesCount, nullptr);

    vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertiesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyPropertiesCount,
                                             queueFamilyProperties.data());

    for (mQueueFamilyIndex = 0;
         mQueueFamilyIndex != queueFamilyPropertiesCount; ++mQueueFamilyIndex) {
        if (queueFamilyProperties[mQueueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            break;
        }
    }

    const vector<float> queuePriorities{1.0};
    VkDeviceQueueCreateInfo deviceQueueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = mQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = queuePriorities.data()
    };

    uint32_t deviceExtensionCount;
    VK_CHECK_ERROR(vkEnumerateDeviceExtensionProperties(mPhysicalDevice,
                                                        nullptr,
                                                        &deviceExtensionCount,
                                                        nullptr));

    vector<VkExtensionProperties> deviceExtensionProperties(deviceExtensionCount);
    VK_CHECK_ERROR(vkEnumerateDeviceExtensionProperties(mPhysicalDevice,
                                                        nullptr,
                                                        &deviceExtensionCount,
                                                        deviceExtensionProperties.data()));

    vector<const char *> deviceExtensionNames;
    for (const auto &properties: deviceExtensionProperties) {
        if (properties.extensionName == string("VK_KHR_swapchain")) {
            deviceExtensionNames.push_back(properties.extensionName);
        }
    }
    assert(deviceExtensionNames.size() == 1);

    VkDeviceCreateInfo deviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensionNames.size()),
        .ppEnabledExtensionNames = deviceExtensionNames.data()
    };

    VK_CHECK_ERROR(vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mDevice));
    vkGetDeviceQueue(mDevice, mQueueFamilyIndex, 0, &mQueue);

    // ================================================================================
    // 4. VkSurface 생성
    // ================================================================================
    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
        .window = window
    };

    VK_CHECK_ERROR(vkCreateAndroidSurfaceKHR(mInstance, &surfaceCreateInfo, nullptr, &mSurface));

    VkBool32 supported;
    VK_CHECK_ERROR(vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice,
                                                        mQueueFamilyIndex,
                                                        mSurface,
                                                        &supported));
    assert(supported);

    // ================================================================================
    // 5. VkSwapchain 생성
    // ================================================================================
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VK_CHECK_ERROR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mPhysicalDevice,
                                                             mSurface,
                                                             &surfaceCapabilities));

    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR;
    for (auto i = 0; i <= 4; ++i) {
        if (auto flag = 0x1u << i; surfaceCapabilities.supportedUsageFlags & flag) {
            compositeAlpha = static_cast<VkCompositeAlphaFlagBitsKHR>(flag);
            break;
        }
    }
    assert(compositeAlpha != VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR);

    uint32_t surfaceFormatCount = 0;
    VK_CHECK_ERROR(vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice,
                                                        mSurface,
                                                        &surfaceFormatCount,
                                                        nullptr));

    vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    VK_CHECK_ERROR(vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice,
                                                        mSurface,
                                                        &surfaceFormatCount,
                                                        surfaceFormats.data()));

    uint32_t surfaceFormatIndex = VK_FORMAT_MAX_ENUM;
    for (auto i = 0; i != surfaceFormatCount; ++i) {
        if (surfaceFormats[i].format == VK_FORMAT_R8G8B8A8_UNORM) {
            surfaceFormatIndex = i;
            break;
        }
    }
    assert(surfaceFormatIndex != VK_FORMAT_MAX_ENUM);

    uint32_t presentModeCount;
    VK_CHECK_ERROR(vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice,
                                                             mSurface,
                                                             &presentModeCount,
                                                             nullptr));

    vector<VkPresentModeKHR> presentModes(presentModeCount);
    VK_CHECK_ERROR(vkGetPhysicalDeviceSurfacePresentModesKHR(mPhysicalDevice,
                                                             mSurface,
                                                             &presentModeCount,
                                                             presentModes.data()));

    uint32_t presentModeIndex = VK_PRESENT_MODE_MAX_ENUM_KHR;
    for (auto i = 0; i != presentModeCount; ++i) {
        if (presentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
            presentModeIndex = i;
            break;
        }
    }
    assert(presentModeIndex != VK_PRESENT_MODE_MAX_ENUM_KHR);

    VkSwapchainCreateInfoKHR swapchainCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = mSurface,
        .minImageCount = surfaceCapabilities.minImageCount,
        .imageFormat = surfaceFormats[surfaceFormatIndex].format,
        .imageColorSpace = surfaceFormats[surfaceFormatIndex].colorSpace,
        .imageExtent = surfaceCapabilities.currentExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = compositeAlpha,
        .presentMode = presentModes[presentModeIndex]
    };

    VK_CHECK_ERROR(vkCreateSwapchainKHR(mDevice, &swapchainCreateInfo, nullptr, &mSwapchain));

    uint32_t swapchainImageCount;
    VK_CHECK_ERROR(vkGetSwapchainImagesKHR(mDevice, mSwapchain, &swapchainImageCount, nullptr));

    mSwapchainImages.resize(swapchainImageCount);
    VK_CHECK_ERROR(vkGetSwapchainImagesKHR(mDevice,
                                           mSwapchain,
                                           &swapchainImageCount,
                                           mSwapchainImages.data()));

    // ================================================================================
    // 6. VkCommandPool 생성
    // ================================================================================
    VkCommandPoolCreateInfo commandPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = mQueueFamilyIndex
    };

    VK_CHECK_ERROR(vkCreateCommandPool(mDevice, &commandPoolCreateInfo, nullptr, &mCommandPool));
}

VkRenderer::~VkRenderer() {
    vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
    vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);
    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    vkDestroyDevice(mDevice, nullptr);
    vkDestroyInstance(mInstance, nullptr);
}
#include <stdio.h>
#include <iostream>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>

#define ESP_UTILS_LOG_TAG "UserMatDebugAllocator"
#include "esp_lib_utils.h"

class UserMatDebugAllocator : public cv::MatAllocator {
public:
    UserMatDebugAllocator()
    {
        m_defaultAllocator = cv::Mat::getDefaultAllocator();
        m_stdAllocator = cv::Mat::getStdAllocator();
        std::cout << "UserMatDebugAllocator: constructor" << std::endl;
    }

    ~UserMatDebugAllocator() override
    {
        std::cout << "UserMatDebugAllocator: destructor" << std::endl;
    }

    cv::UMatData *allocate(int dims, const int *sizes, int type,
                           void *data, size_t *step, cv::AccessFlag flags, cv::UMatUsageFlags usageFlags) const override
    {
        std::cout << "UserMatDebugAllocator: allocate with heap_caps_malloc (SPIRAM only)" << std::endl;

        // Calculate data type size
        int typeSize = CV_ELEM_SIZE(type);

        // Calculate total memory size and step
        size_t totalSize = typeSize;
        for (int i = 0; i < dims; i++) {
            totalSize *= sizes[i];
        }

        // Use heap_caps_malloc to allocate memory, force SPIRAM usage
        void *ptr = heap_caps_malloc(totalSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (ptr == nullptr) {
            std::cout << "UserMatDebugAllocator: heap_caps_malloc failed for size " << totalSize << " bytes" << std::endl;
            std::cout << "UserMatDebugAllocator: SPIRAM allocation failed, but we won't fallback to default allocator" << std::endl;
            return nullptr;
        }

        std::cout << "UserMatDebugAllocator: allocated " << totalSize << " bytes in SPIRAM at " << ptr << std::endl;

        // Calculate step
        if (step != nullptr) {
            step[dims - 1] = typeSize;
            for (int i = dims - 2; i >= 0; i--) {
                step[i] = step[i + 1] * sizes[i + 1];
            }
        }

        // Create UMatData object using our allocated memory
        cv::UMatData *ret = m_defaultAllocator->allocate(dims, sizes, type, ptr, step, flags, usageFlags);
        if (nullptr != ret) {
            ret->currAllocator = this;
            // Store our allocated memory pointer for later deallocation
            ret->userdata = ptr;
        } else {
            // If creation fails, free our allocated memory
            heap_caps_free(ptr);
        }

        return ret;
    }

    bool allocate(cv::UMatData *data, cv::AccessFlag accessflags, cv::UMatUsageFlags usageFlags) const override
    {
        std::cout << "UserMatDebugAllocator: allocate2 (SPIRAM only)" << std::endl;
        // For existing UMatData, we still use the default allocator to manage metadata
        // But the actual data is already in SPIRAM
        return m_defaultAllocator->allocate(data, accessflags, usageFlags);
    }

    void deallocate(cv::UMatData *data) const override
    {
        std::cout << "UserMatDebugAllocator: deallocate with heap_caps_free" << std::endl;

        // Check if this is memory allocated by us
        if (data->userdata != nullptr) {
            void *ptr = data->userdata;
            std::cout << "UserMatDebugAllocator: freeing memory at " << ptr << std::endl;
            heap_caps_free(ptr);
            data->userdata = nullptr;
        }

        // Call the default allocator's deallocate to clean up the UMatData object itself
        m_defaultAllocator->deallocate(data);
    }

    void map(cv::UMatData *data, cv::AccessFlag accessflags) const override
    {
        std::cout << "UserMatDebugAllocator: map" << std::endl;
        return m_defaultAllocator->map(data, accessflags);
    }

    void unmap(cv::UMatData *data) const override
    {
        std::cout << "UserMatDebugAllocator: unmap" << std::endl;
        if ((data->urefcount == 0) && (data->refcount == 0)) {
            deallocate(data);
        }
    }

    void download(cv::UMatData *data, void *dst, int dims, const size_t sz[],
                  const size_t srcofs[], const size_t srcstep[],
                  const size_t dststep[]) const override
    {
        std::cout << "UserMatDebugAllocator: download" << std::endl;
        return m_defaultAllocator->download(data, dst, dims, sz, srcofs, srcstep, dststep);
    }

    void upload(cv::UMatData *data, const void *src, int dims, const size_t sz[],
                const size_t dstofs[], const size_t dststep[],
                const size_t srcstep[]) const override
    {
        std::cout << "UserMatDebugAllocator: upload" << std::endl;
        return m_defaultAllocator->upload(data, src, dims, sz, dstofs, dststep, srcstep);
    }

    void copy(cv::UMatData *srcdata, cv::UMatData *dstdata, int dims, const size_t sz[],
              const size_t srcofs[], const size_t srcstep[],
              const size_t dstofs[], const size_t dststep[], bool sync) const override
    {
        std::cout << "UserMatDebugAllocator: copy" << std::endl;
        return m_defaultAllocator->copy(srcdata, dstdata, dims, sz, srcofs, srcstep, dstofs, dststep, sync);
    }

    cv::BufferPoolController *getBufferPoolController(const char *id = NULL) const override
    {
        std::cout << "UserMatDebugAllocator: getBufferPoolController" << std::endl;
        return m_defaultAllocator->getBufferPoolController(id);
    }

    cv::MatAllocator *m_defaultAllocator;
    cv::MatAllocator *m_stdAllocator;
};

extern "C" void app_main(void)
{
    std::cout << "test start" << std::endl;

    UserMatDebugAllocator userAllocator;
    cv::Mat::setDefaultAllocator(&userAllocator);
    std::cout << "Before allocate" << std::endl;
    esp_utils_mem_print_info();
    {
        cv::Mat mat(1000, 1000, CV_8UC3);
        mat.at<cv::Vec3b>(500, 500) = cv::Vec3b(255, 0, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        std::cout << "After allocate" << std::endl;
        esp_utils_mem_print_info();
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    std::cout << "After delay" << std::endl;
    esp_utils_mem_print_info();
    std::cout << "test end" << std::endl;
}

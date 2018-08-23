
#include <android/native_window.h>
#include <vulkan/vulkan.h>

namespace WSI
{
	uint32_t GetWidth();
	uint32_t GetHeight();

	bool Initialize(VkInstance instance, ANativeWindow* window);
	void Destroy(VkInstance instance);


	VkSurfaceKHR GetSurface();
}


#include <android/native_window.h>

namespace Vulkan
{
	void ReleaseSurface();
	void ReAcquireSurface(ANativeWindow* window);

	bool Initialize(ANativeWindow* window);
	void Draw();
	void Destroy();
}
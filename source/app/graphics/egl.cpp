#include "egl.h"
#include "utils/log.h"
#include <EGL/egl.h>
#include <cassert>
#include <android/native_window.h>

namespace {
	EGLint width = 0, height = 0;
	EGLint format = 0;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;
    EGLDisplay display;
}

namespace EGL
{
	EGLint GetWidth()
	{
		return width;
	}
	EGLint GetHeight()
	{
		return height;
	}
	EGLint GetFormat()
	{
		return format;
	}
	EGLConfig GetConfig()
	{
		return config;
	}
	EGLSurface GetSurface()
	{
		return surface;
	}
	EGLContext GetContext()
	{
		return context;
	}

	bool Initialize(ANativeWindow* window)
	{
		const EGLint attribs[] = {
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_BLUE_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_RED_SIZE, 8,
			EGL_NONE
		};
		EGLint numConfigs;

		display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

		eglInitialize(display, 0, 0);

		eglChooseConfig(display, attribs, nullptr, 0, &numConfigs);
		EGLConfig* supportedConfigs = new EGLConfig[numConfigs];

		assert(supportedConfigs);
		if (!numConfigs)
		{
			LOGE("EGL: no supported config found");
			return false;
		}
		
		eglChooseConfig(display, attribs, supportedConfigs, numConfigs, &numConfigs);
		assert(numConfigs);
		if (!numConfigs)
		{
			LOGE("EGL: Unable to find a configuration");
			return false;
		}

		auto i = 0;
		for (; i < numConfigs; i++)
		{
			auto& cfg = supportedConfigs[i];
			EGLint r, g, b, d;
			if (eglGetConfigAttrib(display, cfg, EGL_RED_SIZE, &r)   &&
				eglGetConfigAttrib(display, cfg, EGL_GREEN_SIZE, &g) &&
				eglGetConfigAttrib(display, cfg, EGL_BLUE_SIZE, &b)  &&
				eglGetConfigAttrib(display, cfg, EGL_DEPTH_SIZE, &d) &&
				r == attribs[3] && g == attribs[5] && b == attribs[7] && d == 0 ) {

				config = supportedConfigs[i];
				break;
			}
		}
		if (i == numConfigs)
		{
			config = supportedConfigs[0];
		}

		/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
		 * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
		 * As soon as we picked a EGLConfig, we can safely reconfigure the
		 * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
		static const EGLint contextAttribs[] = {
				EGL_CONTEXT_CLIENT_VERSION,	2,
				EGL_NONE
		};
		eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
        ANativeWindow_setBuffersGeometry(window, 0, 0, format);
		surface = eglCreateWindowSurface(display, config, window, NULL);
		context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);

		if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE)
		{
			LOGE("EGL: Unable to MakeCurrent");
			return false;
		}

		eglQuerySurface(display, surface, EGL_WIDTH, &width);
		eglQuerySurface(display, surface, EGL_HEIGHT, &height);

		return true;
	}

    void Destroy()
    {
        if (display != EGL_NO_DISPLAY)
        {
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (context != EGL_NO_CONTEXT)
            {
                eglDestroyContext(display, context);
            }
            if (surface != EGL_NO_SURFACE)
            {
                eglDestroySurface(display, surface);
            }
            eglTerminate(display);
        }

        display = EGL_NO_DISPLAY;
        context = EGL_NO_CONTEXT;
        surface = EGL_NO_SURFACE;
    }

    void Swap()
    {
		eglSwapBuffers(display, surface);    	
    }
}
/*
This file belongs to RendererLib.
See LICENSE file in root folder.
*/
#pragma once

#include "OpenGLDefines.hpp"

namespace gl_renderer
{
	namespace gl
	{
#define GL_LIB_BASE_FUNCTION( fun ) extern PFN_gl##fun fun;
#define GL_LIB_FUNCTION( fun ) extern PFN_gl##fun fun;
#define GL_LIB_FUNCTION_EXT( fun, ext, name ) extern PFN_gl##fun fun##_##ext;
#define GL_LIB_FUNCTION_VSN( fun, version ) extern PFN_gl##fun fun##_##version;
#include "OpenGLFunctionsList.inl"
	}

#if RENDERLIB_WIN32
	namespace wgl
	{
#define WGL_LIB_BASE_FUNCTION( fun ) extern PFN_wgl##fun fun;
#define WGL_LIB_FUNCTION( fun ) extern PFN_wgl##fun fun;
#define WGL_LIB_FUNCTION_EXT( fun, ext, name ) extern PFN_wgl##fun fun;
#include "OpenGLFunctionsList.inl"
	}
#elif RENDERLIB_XLIB
#else
#endif
}

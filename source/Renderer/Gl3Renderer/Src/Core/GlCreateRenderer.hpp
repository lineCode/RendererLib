/**
*\file
*	GlCreateRenderer.h
*\author
*	Sylvain Doremus
*/
#pragma once

#include <Core/Renderer.hpp>

#if defined( _WIN32 ) && !defined( Gl3Renderer_STATIC )
#	ifdef Gl3Renderer_EXPORTS
#		define Gl3Renderer_API __declspec( dllexport )
#	else
#		define Gl3Renderer_API __declspec( dllimport )
#	endif
#else
#	define Gl3Renderer_API
#endif

extern "C"
{
	/**
	*\~french
	*\brief
	*	Crée un renderer OpenGL.
	*\param[in] configuration
	*	La configuration de création.
	*\~english
	*\brief
	*	Creates an OpenGL renderer.
	*\param[in] configuration
	*	The creation options.
	*/
	Gl3Renderer_API renderer::Renderer * createRenderer( renderer::Renderer::Configuration const & configuration );
}

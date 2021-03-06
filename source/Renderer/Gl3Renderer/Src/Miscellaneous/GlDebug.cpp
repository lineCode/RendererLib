#include "Miscellaneous/GlDebug.hpp"

#include <cstdint>
#include <iostream>
#include <sstream>

#if RENDERLIB_WIN32
#   include <Windows.h>
#elif RENDERLIB_XLIB
#	include <X11/Xlib.h>
#	include <GL/glx.h>
#endif

#include <GL/gl.h>

//*************************************************************************************************

namespace gl_api
{
	/**
	*\brief
	*	Récupère une fonction dans la dll OpenGL actuellement chargée
	*\param[in] p_strName
	*	Le nom de la fonction
	*\param[in] p_func
	*	Reçoit la fonction
	*\return
	*	true si la fonction a été trouvée, false sinon
	*/
	template< typename Func >
	bool getFunction( std::string const & p_strName, Func & p_func )
	{
#if RENDERLIB_WIN32
		p_func = reinterpret_cast< Func >( wglGetProcAddress( p_strName.c_str() ) );
#elif RENDERLIB_XLIB
		p_func = reinterpret_cast< Func >( glXGetProcAddress( ( GLubyte const * )p_strName.c_str() ) );
#endif
		return p_func != NULL;
	}

#define makeGlExtension( x )	static const std::string x = "GL_"#x;

	makeGlExtension( ARB_debug_output )
	makeGlExtension( AMDX_debug_output )
}

//*************************************************************************************************

namespace gl_renderer
{
	namespace
	{
		std::string getErrorName( uint32_t code, uint32_t category )
		{
			static uint32_t constexpr InvalidEnum = 0x0500;
			static uint32_t constexpr InvalidValue = 0x0501;
			static uint32_t constexpr InvalidOperation = 0x0502;
			static uint32_t constexpr StackOverflow = 0x0503;
			static uint32_t constexpr StackUnderflow = 0x0504;
			static uint32_t constexpr OutOfMemory = 0x0505;
			static uint32_t constexpr InvalidFramebufferOperation = 0x0506;

			static std::map< uint32_t, std::string > const errors
			{
				{ InvalidEnum, "Invalid Enum" },
				{ InvalidValue, "Invalid Value" },
				{ InvalidOperation, "Invalid Operation" },
				{ StackOverflow, "Stack Overflow" },
				{ StackUnderflow, "Stack Underflow" },
				{ OutOfMemory, "Out of memory" },
				{ InvalidFramebufferOperation, "Invalid frame buffer operation" },
			};

			if ( category == GL_DEBUG_CATEGORY_API_ERROR_AMD
				|| category == GL_DEBUG_TYPE_ERROR )
			{
				auto it = errors.find( code );

				if ( it == errors.end() )
				{
					return "UNKNOWN_ERROR";
				}

				return it->second;
			}
			else
			{
				return std::string{};
			}
		}
	}
	using PFNGLDEBUGPROC = void ( CALLBACK * )( uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int length, const char * message, void * userParam );
	using PFNGLDEBUGAMDPROC = void ( CALLBACK * )( uint32_t id, uint32_t category, uint32_t severity, int length, const char* message, void* userParam );
	using PFNGLDEBUGMESSAGECALLBACK = void ( CALLBACK * )( PFNGLDEBUGPROC callback, void * userParam );
	using PFNGLDEBUGMESSAGECALLBACKAMD = void ( CALLBACK * )( PFNGLDEBUGAMDPROC callback, void * userParam );

	PFNGLDEBUGMESSAGECALLBACK glDebugMessageCallback = nullptr;
	PFNGLDEBUGMESSAGECALLBACKAMD glDebugMessageCallbackAMD = nullptr;

	void loadDebugFunctions()
	{
		auto ext = ( char const * )glGetString( GL_EXTENSIONS );
		std::string extensions = ext ? ext : "";

		if ( extensions.find( gl_api::ARB_debug_output ) != std::string::npos )
		{
			if ( !gl_api::getFunction( "glDebugMessageCallback", glDebugMessageCallback ) )
			{
				if ( !gl_api::getFunction( "glDebugMessageCallbackARB", glDebugMessageCallback ) )
				{
					renderer::Logger::logWarning( "Unable to retrieve function glDebugMessageCallback" );
				}
			}
		}
		else if ( extensions.find( gl_api::AMDX_debug_output ) != std::string::npos )
		{
			if ( !gl_api::getFunction( "glDebugMessageCallbackAMD", glDebugMessageCallbackAMD ) )
			{
				renderer::Logger::logWarning( "Unable to retrieve function glDebugMessageCallbackAMD" );
			}
		}
	}

	void initialiseDebugFunctions()
	{
#if !defined( NDEBUG )
		if ( glDebugMessageCallback )
		{
			glDebugMessageCallback( PFNGLDEBUGPROC( &callbackDebugLog ), nullptr );
			glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
		}
		else if ( glDebugMessageCallbackAMD )
		{
			glDebugMessageCallbackAMD( PFNGLDEBUGAMDPROC( &callbackDebugLogAMD ), nullptr );
			glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
		}
#endif
	}

	void CALLBACK callbackDebugLog( uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int length, const char * message, void * userParam )
	{
		std::locale loc{ "C" };
		std::stringstream stream;
		stream.imbue( loc );
		stream << "OpenGL Debug\n";

		switch ( source )
		{
		case GL_DEBUG_SOURCE_API:				stream << "    Source: OpenGL\n";			break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:		stream << "    Source: Windows\n";			break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:	stream << "    Source: Shader compiler\n";	break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:		stream << "    Source: Third party\n";		break;
		case GL_DEBUG_SOURCE_APPLICATION:		stream << "    Source: Application\n";		break;
		case GL_DEBUG_SOURCE_OTHER:				stream << "    Source: Other\n";			break;
		}

		switch ( type )
		{
		case GL_DEBUG_TYPE_ERROR:				stream << "    Type: Error\n";					break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:	stream << "    Type: Deprecated behavior\n";	break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:	stream << "    Type: Undefined behavior\n";		break;
		case GL_DEBUG_TYPE_PORTABILITY:			stream << "    Type: Portability\n";			break;
		case GL_DEBUG_TYPE_PERFORMANCE:			stream << "    Type: Performance\n";			break;
		case GL_DEBUG_TYPE_OTHER:				stream << "    Type: Other\n";					break;
		}

		stream << "    ID: 0x" << std::hex << id << " (" << getErrorName( id, type ) << ")\n";

		switch ( severity )
		{
		case GL_DEBUG_SEVERITY_HIGH:			stream << "    Severity: High\n";			break;
		case GL_DEBUG_SEVERITY_MEDIUM:			stream << "    Severity: Medium\n";			break;
		case GL_DEBUG_SEVERITY_LOW:				stream << "    Severity: Low\n";			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:	stream << "    Severity: Notification\n";	break;
		}

		stream << "    Message: " << message;

		switch ( severity )
		{
		case GL_DEBUG_SEVERITY_HIGH:
			renderer::Logger::logError( stream.str() );
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			renderer::Logger::logWarning( stream.str() );
			break;
		case GL_DEBUG_SEVERITY_LOW:
			renderer::Logger::logInfo( stream.str() );
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			break;
		default:
			renderer::Logger::logDebug( stream.str() );
			break;
		}
	}

	void CALLBACK callbackDebugLogAMD( uint32_t id, uint32_t category, uint32_t severity, int length, const char* message, void* userParam )
	{
		std::locale loc{ "C" };
		std::stringstream stream;
		stream.imbue( loc );
		stream << "OpenGL Debug\n";

		switch ( category )
		{
		case GL_DEBUG_CATEGORY_API_ERROR_AMD:			stream << "    Category: OpenGL\n";					break;
		case GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD:		stream << "    Category: Windows\n";				break;
		case GL_DEBUG_CATEGORY_DEPRECATION_AMD:			stream << "    Category: Deprecated behavior\n";	break;
		case GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD:	stream << "    Category: Undefined behavior\n";		break;
		case GL_DEBUG_CATEGORY_PERFORMANCE_AMD:			stream << "    Category: Performance\n";			break;
		case GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD:		stream << "    Category: Shader compiler\n";		break;
		case GL_DEBUG_CATEGORY_APPLICATION_AMD:			stream << "    Category: Application\n";			break;
		case GL_DEBUG_CATEGORY_OTHER_AMD:				stream << "    Category: Other\n";					break;
		}

		stream << "    ID: 0x" << std::hex << id << " (" << getErrorName( id, category ) << ")\n";

		switch ( severity )
		{
		case GL_DEBUG_SEVERITY_HIGH:					stream << "    Severity: High\n";					break;
		case GL_DEBUG_SEVERITY_MEDIUM:					stream << "    Severity: Medium\n";					break;
		case GL_DEBUG_SEVERITY_LOW:						stream << "    Severity: Low\n";					break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:			stream << "    Severity: Notification\n";			break;
		}

		stream << "    Message: " << message;

		switch ( severity )
		{
		case GL_DEBUG_SEVERITY_HIGH:
			renderer::Logger::logError( stream.str() );
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			renderer::Logger::logWarning( stream.str() );
			break;
		case GL_DEBUG_SEVERITY_LOW:
			renderer::Logger::logInfo( stream.str() );
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			break;
		default:
			renderer::Logger::logDebug( stream.str() );
			break;
		}
	}

	bool glCheckError( std::string const & text )
	{
		bool result = true;
		uint32_t errorCode = gl::GetError();

		if ( errorCode )
		{
			std::locale loc{ "C" };
			std::stringstream stream;
			stream.imbue( loc );
			stream << "OpenGL Error, on function: " << text << std::endl;
			stream << "  ID: 0x" << std::hex << errorCode << " (" << getErrorName( errorCode, GL_DEBUG_TYPE_ERROR ) << ")" << std::endl;
			renderer::Logger::logError( stream.str() );
			errorCode = gl::GetError();
			result = false;
		}

		return result;
	}

	//*************************************************************************************************
}

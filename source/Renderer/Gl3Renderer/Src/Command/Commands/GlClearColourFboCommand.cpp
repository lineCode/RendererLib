/*
This file belongs to GlRenderer.
See LICENSE file in root folder.
*/
#include "GlClearColourFboCommand.hpp"

#include "Core/GlDevice.hpp"
#include "Image/GlTextureView.hpp"
#include "Image/GlTexture.hpp"
#include "RenderPass/GlFrameBuffer.hpp"

namespace gl_renderer
{
	ClearColourFboCommand::ClearColourFboCommand( Device const & device
		, renderer::TextureView const & image
		, renderer::ClearColorValue const & colour )
		: m_device{ device }
		, m_image{ static_cast< TextureView const & >( image ) }
		, m_colour{ colour }
		, m_internal{ getInternal( m_image.getFormat() ) }
		, m_format{ getFormat( m_internal ) }
		, m_type{ getType( m_internal ) }
	{
	}

	void ClearColourFboCommand::apply()const
	{
		glLogCommand( "ClearColourFboCommand" );
		auto & image = static_cast< Texture const & >( m_image.getTexture() );
		auto target = GL_TEXTURE_2D;

		if ( image.getSamplesCount() > renderer::SampleCountFlag::e1 )
		{
			target = GL_TEXTURE_2D_MULTISAMPLE;
		}

		glLogCall( gl::BindFramebuffer
			, GL_FRAMEBUFFER
			, m_device.getBlitDstFbo() );
		GLenum point = getAttachmentPoint( m_image );
		glLogCall( gl::FramebufferTexture2D
			, GL_FRAMEBUFFER
			, point
			, target
			, image.getImage()
			, m_image.getSubResourceRange().baseMipLevel );
		gl::DrawBuffers( 1, &point );
		glLogCall( gl::ClearBufferfv
			, GL_CLEAR_TARGET_COLOR
			, 0u
			, m_colour.float32.data() );
		glLogCall( gl::BindFramebuffer
			, GL_FRAMEBUFFER
			, 0u );
	}

	CommandPtr ClearColourFboCommand::clone()const
	{
		return std::make_unique< ClearColourFboCommand >( *this );
	}
}

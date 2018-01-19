#include "Texture.h"

#include <Buffer/StagingBuffer.hpp>
#include <Core/Device.hpp>
#include <Image/Sampler.hpp>
#include <Image/Texture.hpp>

namespace render
{
	Texture::Texture( renderer::Device const & device
		, renderer::WrapMode wrapS
		, renderer::WrapMode wrapT
		, renderer::WrapMode wrapR
		, renderer::Filter minFilter
		, renderer::Filter magFilter )
		: m_texture{ device.createTexture() }
		, m_sampler{ device.createSampler( wrapS
			, wrapT
			, wrapR
			, minFilter
			, magFilter ) }
	{
	}

	void Texture::image( renderer::PixelFormat format
		, renderer::UIVec2 const & size
		, ByteArray const & data
		, renderer::StagingBuffer const & stagingBuffer
		, renderer::CommandBuffer const & commandBuffer )
	{
		m_format = format;
		m_size = size;
		m_texture->setImage( m_format
			, m_size
			, renderer::ImageUsageFlag::eTransferDst | renderer::ImageUsageFlag::eSampled
			, m_format == renderer::PixelFormat::eR8G8B8
				? renderer::ImageTiling::eLinear
				: renderer::ImageTiling::eOptimal );
		stagingBuffer.copyTextureData( commandBuffer
			, data
			, *m_texture );
	}
}
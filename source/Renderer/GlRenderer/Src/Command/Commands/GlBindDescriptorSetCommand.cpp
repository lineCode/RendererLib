/*
This file belongs to GlRenderer.
See LICENSE file in root folder.
*/
#include "GlBindDescriptorSetCommand.hpp"

#include "Buffer/GlBuffer.hpp"
#include "Buffer/GlBufferView.hpp"
#include "Descriptor/GlDescriptorSet.hpp"
#include "Pipeline/GlPipelineLayout.hpp"
#include "Image/GlSampler.hpp"
#include "Image/GlTexture.hpp"
#include "Image/GlTextureView.hpp"
#include "Buffer/GlUniformBuffer.hpp"

#include <Descriptor/DescriptorSetLayoutBinding.hpp>

namespace gl_renderer
{
	namespace
	{
		renderer::TextureView const & getView( renderer::WriteDescriptorSet const & write, uint32_t index )
		{
			assert( index < write.imageInfo.size() );
			return write.imageInfo[index].imageView.value().get();
		}

		renderer::Sampler const & getSampler( renderer::WriteDescriptorSet const & write, uint32_t index )
		{
			assert( index < write.imageInfo.size() );
			return write.imageInfo[index].sampler.value().get();
		}

		renderer::BufferBase const & getBuffer( renderer::WriteDescriptorSet const & write, uint32_t index )
		{
			assert( index < write.bufferInfo.size() );
			return write.bufferInfo[index].buffer.get();
		}

		void bindCombinedSampler( renderer::WriteDescriptorSet const & write )
		{
			for ( auto i = 0u; i < write.imageInfo.size(); ++i )
			{
				uint32_t bindingIndex = write.dstBinding + write.dstArrayElement + i;
				auto & view = getView( write, i );
				auto & sampler = getSampler( write, i );
				glLogCall( gl::ActiveTexture
					, GlTextureUnit( GL_TEXTURE0 + bindingIndex ) );
				glLogCall( gl::BindTexture
					, convert( view.getType() )
					, static_cast< TextureView const & >( view ).getImage() );
				glLogCall( gl::BindSampler
					, bindingIndex
					, static_cast< Sampler const & >( sampler ).getSampler() );
			}
		}

		void bindSampler( renderer::WriteDescriptorSet const & write )
		{
			for ( auto i = 0u; i < write.imageInfo.size(); ++i )
			{
				uint32_t bindingIndex = write.dstBinding + write.dstArrayElement + i;
				auto & sampler = getSampler( write, i );
				glLogCall( gl::BindSampler
					, bindingIndex
					, static_cast< Sampler const & >( sampler ).getSampler() );
			}
		}

		void bindSampledTexture( renderer::WriteDescriptorSet const & write )
		{
			for ( auto i = 0u; i < write.imageInfo.size(); ++i )
			{
				uint32_t bindingIndex = write.dstBinding + write.dstArrayElement + i;
				auto & view = getView( write, i );
				glLogCall( gl::ActiveTexture
					, GlTextureUnit( GL_TEXTURE0 + bindingIndex ) );
				glLogCall( gl::BindTexture
					, convert( view.getType() )
					, static_cast< TextureView const & >( view ).getImage() );
			}
		}

		void bindStorageTexture( renderer::WriteDescriptorSet const & write )
		{
			for ( auto i = 0u; i < write.imageInfo.size(); ++i )
			{
				uint32_t bindingIndex = write.dstBinding + write.dstArrayElement + i;
				auto & view = getView( write, i );
				auto & range = view.getSubResourceRange();
				glLogCall( gl::ActiveTexture
					, GlTextureUnit( GL_TEXTURE0 + bindingIndex ) );
				glLogCall( gl::BindImageTexture
					, bindingIndex
					, static_cast< TextureView const & >( view ).getImage()
					, range.baseMipLevel
					, range.layerCount
					, range.baseArrayLayer
					, GL_ACCESS_TYPE_READ_WRITE
					, getInternal( view.getFormat() ) );
			}
		}

		void bindUniformBuffer( renderer::WriteDescriptorSet const & write )
		{
			for ( auto i = 0u; i < write.bufferInfo.size(); ++i )
			{
				uint32_t bindingIndex = write.dstBinding + write.dstArrayElement + i;
				auto & buffer = getBuffer( write, i );
				glLogCall( gl::BindBufferRange
					, GL_BUFFER_TARGET_UNIFORM
					, bindingIndex
					, static_cast< Buffer const & >( buffer ).getBuffer()
					, GLintptr( write.bufferInfo[i].offset )
					, GLsizeiptr( write.bufferInfo[i].range ) );
			}
		}

		void bindStorageBuffer( renderer::WriteDescriptorSet const & write )
		{
			for ( auto i = 0u; i < write.bufferInfo.size(); ++i )
			{
				uint32_t bindingIndex = write.dstBinding + write.dstArrayElement + i;
				auto & buffer = getBuffer( write, i );
				glLogCall( gl::BindBufferRange
					, GL_BUFFER_TARGET_SHADER_STORAGE
					, bindingIndex
					, static_cast< Buffer const & >( buffer ).getBuffer()
					, GLintptr( write.bufferInfo[i].offset )
					, GLsizeiptr( write.bufferInfo[i].range ) );
			}
		}

		void bindTexelBuffer( renderer::WriteDescriptorSet const & write )
		{
			for ( auto i = 0u; i < write.bufferInfo.size(); ++i )
			{
				uint32_t bindingIndex = write.dstBinding + write.dstArrayElement + i;
				auto & buffer = getBuffer( write, i );
				glLogCall( gl::ActiveTexture
					, GlTextureUnit( GL_TEXTURE0 + bindingIndex ) );
				glLogCall( gl::BindTexture
					, GL_BUFFER_TARGET_TEXTURE
					, static_cast< BufferView const & >( write.texelBufferView[i].get() ).getImage() );
			}
		}

		void bindDynamicUniformBuffer( renderer::WriteDescriptorSet const & write, uint32_t offset )
		{
			for ( auto i = 0u; i < write.bufferInfo.size(); ++i )
			{
				uint32_t bindingIndex = write.dstBinding + write.dstArrayElement + i;
				auto & buffer = getBuffer( write, i );
				glLogCall( gl::BindBufferRange
					, GL_BUFFER_TARGET_UNIFORM
					, bindingIndex
					, static_cast< Buffer const & >( buffer ).getBuffer()
					, GLintptr( write.bufferInfo[i].offset + offset )
					, GLsizeiptr( write.bufferInfo[i].range ) );
			}
		}

		void bindDynamicStorageBuffer( renderer::WriteDescriptorSet const & write, uint32_t offset )
		{
			for ( auto i = 0u; i < write.bufferInfo.size(); ++i )
			{
				uint32_t bindingIndex = write.dstBinding + write.dstArrayElement + i;
				auto & buffer = getBuffer( write, i );
				glLogCall( gl::BindBufferRange
					, GL_BUFFER_TARGET_SHADER_STORAGE
					, bindingIndex
					, static_cast< Buffer const & >( buffer ).getBuffer()
					, GLintptr( write.bufferInfo[i].offset + offset )
					, GLsizeiptr( write.bufferInfo[i].range ) );
			}
		}

		void bindDynamicBuffers( renderer::WriteDescriptorSetArray const & writes
			, renderer::UInt32Array const & offsets )
		{
			for ( auto i = 0u; i < offsets.size(); ++i )
			{
				auto & write = writes[i];

				switch ( write.descriptorType )
				{
				case renderer::DescriptorType::eUniformBufferDynamic:
					bindDynamicUniformBuffer( write, offsets[i] );
					break;

				case renderer::DescriptorType::eStorageBufferDynamic:
					bindDynamicStorageBuffer( write, offsets[i] );
					break;

				default:
					assert( false && "Unsupported dynamic descriptor type" );
					throw std::runtime_error{ "Unsupported dynamic descriptor type" };
					break;
				}
			}
		}
	}

	BindDescriptorSetCommand::BindDescriptorSetCommand( renderer::DescriptorSet const & descriptorSet
		, renderer::PipelineLayout const & layout
		, renderer::UInt32Array const & dynamicOffsets
		, renderer::PipelineBindPoint bindingPoint )
		: m_descriptorSet{ static_cast< DescriptorSet const & >( descriptorSet ) }
		, m_layout{ static_cast< PipelineLayout const & >( layout ) }
		, m_bindingPoint{ bindingPoint }
		, m_dynamicOffsets{ dynamicOffsets }
	{
		assert( m_descriptorSet.getDynamicBuffers().size() == m_dynamicOffsets.size()
			&& "Dynamic descriptors and dynamic offsets sizes must match." );
	}

	void BindDescriptorSetCommand::apply()const
	{
		glLogCommand( "BindDescriptorSetCommand" );
		for ( auto & write : m_descriptorSet.getCombinedTextureSamplers() )
		{
			bindCombinedSampler( write );
		}

		for ( auto & write : m_descriptorSet.getSamplers() )
		{
			bindSampler( write );
		}

		for ( auto & write : m_descriptorSet.getSampledTextures() )
		{
			bindSampledTexture( write );
		}

		for ( auto & write : m_descriptorSet.getStorageTextures() )
		{
			bindStorageTexture( write );
		}

		for ( auto & write : m_descriptorSet.getUniformBuffers() )
		{
			bindUniformBuffer( write );
		}

		for ( auto & write : m_descriptorSet.getStorageBuffers() )
		{
			bindStorageBuffer( write );
		}

		for ( auto & write : m_descriptorSet.getTexelBuffers() )
		{
			bindTexelBuffer( write );
		}

		bindDynamicBuffers( m_descriptorSet.getDynamicBuffers(), m_dynamicOffsets );
	}

	CommandPtr BindDescriptorSetCommand::clone()const
	{
		return std::make_unique< BindDescriptorSetCommand >( *this );
	}
}

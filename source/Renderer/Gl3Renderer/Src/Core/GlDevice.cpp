/*
This file belongs to RendererLib.
See LICENSE file in root folder.
*/
#include "Core/GlDevice.hpp"

#include "Buffer/GlBuffer.hpp"
#include "Buffer/GlBufferView.hpp"
#include "Buffer/GlGeometryBuffers.hpp"
#include "Buffer/GlUniformBuffer.hpp"
#include "Command/GlCommandPool.hpp"
#include "Command/GlQueue.hpp"
#include "Core/GlConnection.hpp"
#include "Core/GlContext.hpp"
#include "Core/GlDummyIndexBuffer.hpp"
#include "Core/GlRenderer.hpp"
#include "Core/GlSwapChain.hpp"
#include "Descriptor/GlDescriptorPool.hpp"
#include "Descriptor/GlDescriptorSetLayout.hpp"
#include "Image/GlSampler.hpp"
#include "Image/GlTexture.hpp"
#include "Image/GlTextureView.hpp"
#include "Miscellaneous/GlDeviceMemory.hpp"
#include "Miscellaneous/GlQueryPool.hpp"
#include "Pipeline/GlPipelineLayout.hpp"
#include "RenderPass/GlRenderPass.hpp"
#include "Shader/GlShaderModule.hpp"
#include "Sync/GlEvent.hpp"
#include "Sync/GlFence.hpp"
#include "Sync/GlSemaphore.hpp"

#include <Image/ImageSubresource.hpp>
#include <Image/SubresourceLayout.hpp>
#include <RenderPass/RenderPassCreateInfo.hpp>

#include <algorithm>
#include <iostream>

namespace gl_renderer
{
	namespace
	{
		void doApply( renderer::ColourBlendState const & state )
		{
			if ( state.logicOpEnable )
			{
				glLogCall( gl::LogicOp, convert( state.logicOp ) );
			}

			auto & blendConstants = state.blendConstants;
			glLogCall( gl::BlendColor
				, blendConstants[0]
				, blendConstants[1]
				, blendConstants[2]
				, blendConstants[3] );

			bool blend = false;

			if ( gl::BlendEquationSeparatei_40 )
			{
				uint32_t buf = 0;

				for ( auto & blendState : state.attachs )
				{
					if ( blendState.blendEnable )
					{
						blend = true;
						glLogCall( gl::BlendEquationSeparatei_40
							, buf
							, convert( blendState.colorBlendOp )
							, convert( blendState.alphaBlendOp ) );
						glLogCall( gl::BlendFuncSeparatei_40
							, buf
							, convert( blendState.srcColorBlendFactor )
							, convert( blendState.dstColorBlendFactor )
							, convert( blendState.srcAlphaBlendFactor )
							, convert( blendState.dstAlphaBlendFactor ) );
					}

					++buf;
				}
			}
			else
			{
				auto count = std::count_if( state.attachs.begin()
					, state.attachs.end()
					, []( renderer::ColourBlendStateAttachment const & attach )
					{
						return attach.blendEnable;
					} );
				blend = count > 0;

				if ( count > 1 )
				{
					renderer::Logger::logWarning( "Separate blend equations are not available." );
				}

				if ( blend )
				{
					auto it = std::find_if( state.attachs.begin()
						, state.attachs.end()
						, []( renderer::ColourBlendStateAttachment const & attach )
						{
							return attach.blendEnable;
						} );
					glLogCall( gl::BlendEquationSeparate
						, convert( it->colorBlendOp )
						, convert( it->alphaBlendOp ) );
					glLogCall( gl::BlendFuncSeparate
						, convert( it->srcColorBlendFactor )
						, convert( it->dstColorBlendFactor )
						, convert( it->srcAlphaBlendFactor )
						, convert( it->dstAlphaBlendFactor ) );
				}
			}

			if ( blend )
			{
				glLogCall( gl::Enable, GL_BLEND );
			}
			else
			{
				glLogCall( gl::Disable, GL_BLEND );
			}
		}

		void doApply( renderer::RasterisationState const & state )
		{
			if ( state.cullMode != renderer::CullModeFlag::eNone )
			{
				glLogCall( gl::Enable, GL_CULL_FACE );
				glLogCall( gl::CullFace, convert( state.cullMode ) );
				glLogCall( gl::FrontFace, convert( state.frontFace ) );
			}
			else
			{
				glLogCall( gl::Disable, GL_CULL_FACE );
			}

			glLogCall( gl::PolygonMode
				, GL_CULL_MODE_FRONT_AND_BACK
				, convert( state.polygonMode ) );

			if ( state.depthBiasEnable )
			{
				switch ( state.polygonMode )
				{
				case renderer::PolygonMode::eFill:
					glLogCall( gl::Enable, GL_POLYGON_OFFSET_FILL );
					break;

				case renderer::PolygonMode::eLine:
					glLogCall( gl::Enable, GL_POLYGON_OFFSET_LINE );
					break;

				case renderer::PolygonMode::ePoint:
					glLogCall( gl::Enable, GL_POLYGON_OFFSET_POINT );
					break;
				}

				glLogCall( gl::PolygonOffsetClampEXT, state.depthBiasConstantFactor
					, state.depthBiasSlopeFactor
					, state.depthBiasClamp );
			}
			else
			{
				switch ( state.polygonMode )
				{
				case renderer::PolygonMode::eFill:
					glLogCall( gl::Disable, GL_POLYGON_OFFSET_FILL );
					break;

				case renderer::PolygonMode::eLine:
					glLogCall( gl::Disable, GL_POLYGON_OFFSET_LINE );
					break;

				case renderer::PolygonMode::ePoint:
					glLogCall( gl::Disable, GL_POLYGON_OFFSET_POINT );
					break;
				}
			}

			if ( state.depthClampEnable )
			{
				glLogCall( gl::Enable, GL_DEPTH_CLAMP );
			}
			else
			{
				glLogCall( gl::Disable, GL_DEPTH_CLAMP );
			}

			if ( state.rasteriserDiscardEnable )
			{
				glLogCall( gl::Enable, GL_RASTERIZER_DISCARD );
			}
			else
			{
				glLogCall( gl::Disable, GL_RASTERIZER_DISCARD );
			}

			glLogCall( gl::LineWidth, state.lineWidth );
		}

		void doApply( renderer::MultisampleState const & state )
		{
			if ( state.rasterisationSamples != renderer::SampleCountFlag::e1 )
			{
				glLogCall( gl::Enable, GL_MULTISAMPLE );

				if ( state.alphaToCoverageEnable )
				{
					glLogCall( gl::Enable, GL_SAMPLE_ALPHA_TO_COVERAGE );
				}
				else
				{
					glLogCall( gl::Disable, GL_SAMPLE_ALPHA_TO_COVERAGE );
				}

				if ( state.alphaToOneEnable )
				{
					glLogCall( gl::Enable, GL_SAMPLE_ALPHA_TO_ONE );
				}
				else
				{
					glLogCall( gl::Disable, GL_SAMPLE_ALPHA_TO_ONE );
				}
			}
			else
			{
				glLogCall( gl::Disable, GL_MULTISAMPLE );
			}
		}

		void doApply( renderer::DepthStencilState const & state )
		{
			if ( state.depthWriteEnable )
			{
				glLogCall( gl::DepthMask, GL_TRUE );
			}
			else
			{
				glLogCall( gl::DepthMask, GL_FALSE );
			}

			if ( state.depthTestEnable )
			{
				glLogCall( gl::Enable, GL_DEPTH_TEST );
				glLogCall( gl::DepthFunc, convert( state.depthCompareOp ) );
			}
			else
			{
				glLogCall( gl::Disable, GL_DEPTH_TEST );
			}

			if ( state.stencilTestEnable )
			{
				glLogCall( gl::Enable, GL_STENCIL_TEST );

				glLogCall( gl::StencilMaskSeparate
					, GL_CULL_MODE_BACK
					, state.back.writeMask );
				glLogCall( gl::StencilFuncSeparate
					, GL_CULL_MODE_BACK
					, convert( state.back.compareOp )
					, state.back.reference
					, state.back.compareMask );
				glLogCall( gl::StencilOpSeparate
					, GL_CULL_MODE_BACK
					, convert( state.back.failOp )
					, convert( state.back.depthFailOp )
					, convert( state.back.passOp ) );
				glLogCall( gl::StencilMaskSeparate
					, GL_CULL_MODE_FRONT
					, state.front.writeMask );
				glLogCall( gl::StencilFuncSeparate
					, GL_CULL_MODE_FRONT
					, convert( state.front.compareOp )
					, state.front.reference
					, state.front.compareMask );
				glLogCall( gl::StencilOpSeparate
					, GL_CULL_MODE_FRONT
					, convert( state.front.failOp )
					, convert( state.front.depthFailOp )
					, convert( state.front.passOp ) );
			}
			else
			{
				glLogCall( gl::Disable, GL_STENCIL_TEST );
			}

			if ( state.depthBoundsTestEnable )
			{
				glLogCall( gl::Enable, GL_DEPTH_CLAMP );
				glLogCall( gl::DepthRange, state.minDepthBounds, state.maxDepthBounds );
			}
			else
			{
				glLogCall( gl::Disable, GL_DEPTH_CLAMP );
			}
		}

		void doApply( renderer::TessellationState const & state )
		{
			if ( state.patchControlPoints )
			{
				glLogCall( gl::PatchParameteri_ARB, GL_PATCH_VERTICES, int( state.patchControlPoints ) );
			}
		}

		void doApply( renderer::InputAssemblyState const & state )
		{
			if ( state.topology == renderer::PrimitiveTopology::ePointList )
			{
				glLogCall( gl::Enable, GL_PROGRAM_POINT_SIZE );
			}
			else
			{
				glLogCall( gl::Disable, GL_PROGRAM_POINT_SIZE );
			}

			if ( state.primitiveRestartEnable )
			{
				glLogCall( gl::Enable, GL_PRIMITIVE_RESTART );
			}
			else
			{
				glLogCall( gl::Disable, GL_PRIMITIVE_RESTART );
			}
		}
	}

	Device::Device( renderer::Renderer const & renderer
		, PhysicalDevice const & gpu
		, renderer::ConnectionPtr && connection )
		: renderer::Device{ renderer, gpu, *connection }
		, m_context{ Context::create( gpu, std::move( connection ) ) }
		, m_rsState{}
	{
		enable();
		//glLogCall( gl::ClipControl, GL_UPPER_LEFT, GL_ZERO_TO_ONE );
		glLogCall( gl::Enable, GL_TEXTURE_CUBE_MAP_SEAMLESS );
		initialiseDebugFunctions();
		disable();

		m_timestampPeriod = 1;
		m_presentQueue = std::make_unique< Queue >( *this );
		m_computeQueue = std::make_unique< Queue >( *this );
		m_graphicsQueue = std::make_unique< Queue >( *this );
		m_presentCommandPool = std::make_unique< CommandPool >( *this, 0u );
		m_computeCommandPool = std::make_unique< CommandPool >( *this, 0u );
		m_graphicsCommandPool = std::make_unique< CommandPool >( *this, 0u );

		enable();
		doApply( m_cbState );
		doApply( m_dsState );
		doApply( m_msState );
		doApply( m_rsState );
		doApply( m_iaState );

		if ( m_gpu.getFeatures().tessellationShader )
		{
			doApply( m_tsState );
		}

		m_dummyIndexed.indexBuffer = renderer::makeBuffer< uint32_t >( *this
			, sizeof( dummyIndex ) / sizeof( dummyIndex[0] )
			, renderer::BufferTarget::eIndexBuffer
			, renderer::MemoryPropertyFlag::eHostVisible );
		if ( auto * buffer = m_dummyIndexed.indexBuffer->lock( 0u
			, sizeof( dummyIndex ) / sizeof( dummyIndex[0] )
			, renderer::MemoryMapFlag::eWrite ) )
		{
			std::memcpy( buffer, dummyIndex, sizeof( dummyIndex ) );
			m_dummyIndexed.indexBuffer->unlock();
		}
		auto & indexBuffer = static_cast< Buffer const & >( m_dummyIndexed.indexBuffer->getBuffer() );

		m_dummyIndexed.geometryBuffers = std::make_unique< GeometryBuffers >( VboBindings{}
			, BufferObjectBinding{ indexBuffer.getBuffer(), 0u, &indexBuffer }
			, renderer::VertexInputState{}
			, renderer::IndexType::eUInt32 );
		m_dummyIndexed.geometryBuffers->initialise();

		gl::GenFramebuffers( 2, m_blitFbos );
		disable();
	}

	Device::~Device()
	{
		enable();
		gl::DeleteFramebuffers( 2, m_blitFbos );
		m_dummyIndexed.geometryBuffers.reset();
		m_dummyIndexed.indexBuffer.reset();
		disable();

		m_graphicsCommandPool.reset();
		m_graphicsQueue.reset();
		m_presentCommandPool.reset();
		m_presentQueue.reset();
		m_computeCommandPool.reset();
		m_computeQueue.reset();
	}

	renderer::RenderPassPtr Device::createRenderPass( renderer::RenderPassCreateInfo createInfo )const
	{
		return std::make_unique< RenderPass >( *this, std::move( createInfo ) );
	}

	renderer::PipelineLayoutPtr Device::createPipelineLayout( renderer::DescriptorSetLayoutCRefArray const & setLayouts
		, renderer::PushConstantRangeCRefArray const & pushConstantRanges )const
	{
		return std::make_unique< PipelineLayout >( *this
			, setLayouts
			, pushConstantRanges );
	}

	renderer::DescriptorSetLayoutPtr Device::createDescriptorSetLayout( renderer::DescriptorSetLayoutBindingArray && bindings )const
	{
		return std::make_unique< DescriptorSetLayout >( *this, std::move( bindings ) );
	}

	renderer::DescriptorPoolPtr Device::createDescriptorPool( renderer::DescriptorPoolCreateFlags flags
		, uint32_t maxSets
		, renderer::DescriptorPoolSizeArray poolSizes )const
	{
		return std::make_unique< DescriptorPool >( *this, flags, maxSets, poolSizes );
	}

	renderer::DeviceMemoryPtr Device::allocateMemory( renderer::MemoryRequirements const & requirements
		, renderer::MemoryPropertyFlags flags )const
	{
		return std::make_unique< DeviceMemory >( *this
			, requirements
			, flags );
	}

	renderer::TexturePtr Device::createTexture( renderer::ImageCreateInfo const & createInfo )const
	{
		return std::make_unique< Texture >( *this, createInfo );
	}

	void Device::getImageSubresourceLayout( renderer::Texture const & image
		, renderer::ImageSubresource const & subresource
		, renderer::SubresourceLayout & layout )const
	{
		auto & gltex = static_cast< Texture const & >( image );
		auto target = convert( gltex.getType(), gltex.getLayerCount(), gltex.getFlags() );
		glLogCall( gl::BindTexture
			, target
			, gltex.getImage() );
		int w = 0;
		int h = 0;
		int d = 0;
		int red = 0;
		int green = 0;
		int blue = 0;
		int alpha = 0;
		int depth = 0;
		int stencil = 0;
		gl::GetTexLevelParameteriv( target, subresource.mipLevel, GL_TEXTURE_WIDTH, &w );
		gl::GetTexLevelParameteriv( target, subresource.mipLevel, GL_TEXTURE_HEIGHT, &h );
		gl::GetTexLevelParameteriv( target, subresource.mipLevel, GL_TEXTURE_DEPTH, &d );
		gl::GetTexLevelParameteriv( target, subresource.mipLevel, GL_TEXTURE_RED_SIZE, &red );
		gl::GetTexLevelParameteriv( target, subresource.mipLevel, GL_TEXTURE_GREEN_SIZE, &green );
		gl::GetTexLevelParameteriv( target, subresource.mipLevel, GL_TEXTURE_BLUE_SIZE, &blue );
		gl::GetTexLevelParameteriv( target, subresource.mipLevel, GL_TEXTURE_ALPHA_SIZE, &alpha );
		gl::GetTexLevelParameteriv( target, subresource.mipLevel, GL_TEXTURE_DEPTH_SIZE, &depth );
		gl::GetTexLevelParameteriv( target, subresource.mipLevel, GL_TEXTURE_STENCIL_SIZE, &stencil );
		layout.rowPitch = 0u;
		layout.arrayPitch = 0u;
		layout.depthPitch = 0u;
		layout.size = std::max( w, 1 )  * std::max( d, 1 ) * std::max( h, 1 ) * ( red + green + blue + alpha + depth + stencil );
		layout.offset = 0;
		glLogCall( gl::BindTexture
			, target
			, 0 );
	}

	renderer::SamplerPtr Device::createSampler( renderer::SamplerCreateInfo const & createInfo )const
	{
		return std::make_unique< Sampler >( *this, createInfo );
	}

	renderer::BufferBasePtr Device::createBuffer( uint32_t size
		, renderer::BufferTargets target )const
	{
		return std::make_unique< Buffer >( *this
			, size
			, target );
	}

	renderer::BufferViewPtr Device::createBufferView( renderer::BufferBase const & buffer
		, renderer::Format format
		, uint32_t offset
		, uint32_t range )const
	{
		return std::make_unique< BufferView >( *this
			, static_cast< Buffer const & >( buffer )
			, format
			, offset
			, range );
	}

	renderer::UniformBufferBasePtr Device::createUniformBuffer( uint32_t count
		, uint32_t size
		, renderer::BufferTargets target
		, renderer::MemoryPropertyFlags memoryFlags )const
	{
		return std::make_unique< UniformBuffer >( *this
			, count
			, size
			, target
			, memoryFlags );
	}

	renderer::SwapChainPtr Device::createSwapChain( renderer::Extent2D const & size )const
	{
		renderer::SwapChainPtr result;

		try
		{
			result = std::make_unique< SwapChain >( *this, size );
		}
		catch ( std::exception & exc )
		{
			renderer::Logger::logError( std::string{ "Could not create the swap chain:\n" } + exc.what() );
		}
		catch ( ... )
		{
			renderer::Logger::logError( "Could not create the swap chain:\nUnknown error" );
		}

		return result;
	}

	renderer::SemaphorePtr Device::createSemaphore()const
	{
		return std::make_unique< Semaphore >( *this );
	}

	renderer::FencePtr Device::createFence( renderer::FenceCreateFlags flags )const
	{
		return std::make_unique< Fence >( *this, flags );
	}

	renderer::EventPtr Device::createEvent()const
	{
		return std::make_unique< Event >( *this );
	}

	renderer::CommandPoolPtr Device::createCommandPool( uint32_t queueFamilyIndex
		, renderer::CommandPoolCreateFlags const & flags )const
	{
		return std::make_unique< CommandPool >( *this
			, queueFamilyIndex
			, flags );
	}

	renderer::ShaderModulePtr Device::createShaderModule( renderer::ShaderStageFlag stage )const
	{
		return std::make_shared< ShaderModule >( *this, stage );
	}

	renderer::QueryPoolPtr Device::createQueryPool( renderer::QueryType type
		, uint32_t count
		, renderer::QueryPipelineStatisticFlags pipelineStatistics )const
	{
		return std::make_unique< QueryPool >( *this
			, type
			, count
			, pipelineStatistics );
	}

	void Device::waitIdle()const
	{
		glLogCall( gl::Finish );
	}

	void Device::swapBuffers()const
	{
		m_context->swapBuffers();
	}

	void Device::doEnable()const
	{
		m_context->setCurrent();
	}

	void Device::doDisable()const
	{
		m_context->endCurrent();
	}
}

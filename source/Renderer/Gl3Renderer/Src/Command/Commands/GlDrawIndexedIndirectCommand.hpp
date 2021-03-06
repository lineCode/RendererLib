/*
This file belongs to RendererLib.
See LICENSE file in root folder
*/
#pragma once

#include "GlCommandBase.hpp"

namespace gl_renderer
{
	class DrawIndexedIndirectCommand
		: public CommandBase
	{
	public:
		DrawIndexedIndirectCommand( Device const & device
			, renderer::BufferBase const & buffer
			, uint32_t offset
			, uint32_t drawCount
			, uint32_t stride
			, renderer::PrimitiveTopology mode
			, renderer::IndexType type );

		void apply()const override;
		CommandPtr clone()const override;

	private:
		Device const & m_device;
		Buffer const & m_buffer;
		uint32_t m_offset;
		uint32_t m_drawCount;
		uint32_t m_stride;
		GlPrimitiveTopology m_mode;
		GlIndexType m_type;
	};
}

#include <stdafx.h>

// Include
#include "RenderDeviceOGL.h"

// General
#include "TextureOGL.h"

// Additional
#include "ShaderOGL.h"
#include "TextureOGLTranslate.h"

#include __PACK_BEGIN
struct BLPHeader
{
	uint8 magic[4];
	uint32 type;

	uint8 compression;  // Compression: 1 for uncompressed, 2 for DXTC, 3 (cataclysm) for plain A8R8G8B8 m_DiffuseTextures
	uint8 alpha_depth;  // Alpha channel bit depth: 0, 1, 4 or 8
	uint8 alpha_type;   // 0, 1, 7, or 8
	uint8 has_mips;     // 0 = no mips, 1 = has mips

	uint32 width;
	uint32 height;
	uint32 mipOffsets[16];
	uint32 mipSizes[16];
};
#include __PACK_END


GLsizei GLTextureCompressedSize(GLenum _format, int _width, int _height, int _depth)
{
	switch (_format)
	{
	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		return std::max(_width / 4, 1) * std::max(_height / 4, 1) * _depth * 8;
	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
		return std::max(_width / 4, 1) * std::max(_height / 4, 1) * _depth * 16;
	default:
		_ASSERT(false);
	}
}

TextureOGL::TextureOGL(RenderDeviceOGL* _device)
	: m_TextureWidth(0)
	, m_TextureHeight(0)
	, m_TextureDepth(0)
	, m_CPUAccess(CPUAccess::None)
	, m_bDynamic(false)
	, m_bGenerateMipmaps(false)
	, m_BPP(0)
	, m_Pitch(0)
	, m_bIsTransparent(false)
	, m_bIsDirty(false)
	, m_TextureType(0)
{
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &m_GLObj);
}

// 2D Texture
TextureOGL::TextureOGL(RenderDeviceOGL* _device, uint16_t width, uint16_t height, uint16_t slices, const TextureFormat& format, CPUAccess cpuAccess)
	: m_TextureWidth(width)
	, m_TextureHeight(height)
	, m_BPP(0)
	, m_TextureFormat(format)
	, m_CPUAccess(cpuAccess)
	, m_bGenerateMipmaps(false)
	, m_bIsTransparent(true)
	, m_bIsDirty(false)
	, m_TextureType(GL_TEXTURE_2D)
{
	m_TextureDepth = glm::max<uint16_t>(slices, 1);

	m_TextureDimension = Dimension::Texture2D;
	if (m_TextureDepth > 1)
	{
		m_TextureDimension = Dimension::Texture2DArray;
	}

	glGenTextures(1, &m_GLObj);
	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, m_GLObj);
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

		/*float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);*/

		GLenum internalFormat = TranslateTextureInternalFormat(format);
		GLenum inputFormat = TranslateTextureInputFormat(format);
		GLenum inputType = TranslateTextureInputType(format);

		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_TextureWidth, m_TextureHeight, 0, inputFormat, inputType, NULL);
		OGLCheckError();

		// Sampler state
	}
	glBindTexture(GL_TEXTURE_2D, 0);
}

// CUBE Texture
TextureOGL::TextureOGL(RenderDeviceOGL* _device, uint16_t size, uint16_t count, const TextureFormat& format, CPUAccess cpuAccess)
{
	m_TextureDimension = Texture::Dimension::TextureCube;
	m_TextureWidth = m_TextureHeight = size;

	glGenTextures(1, &m_GLObj);

	glActiveTexture(GL_TEXTURE15);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_GLObj);

	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// Sampler state

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

TextureOGL::~TextureOGL()
{
	if (m_GLObj != 0)
	{
		glDeleteTextures(1, &m_GLObj);
		m_GLObj = 0;
	}
}

bool TextureOGL::LoadTexture2D(cstring fileName)
{
	std::shared_ptr<IFile> f = GetManager<IFilesManager>()->Open(fileName);

	// Read data
	BLPHeader header;
	f->readBytes(&header, sizeof(BLPHeader));

	if (header.width & (header.width - 1)) return false;
	if (header.height & (header.height - 1)) return false;

	assert1(header.magic[0] == 'B' && header.magic[1] == 'L' && header.magic[2] == 'P' && header.magic[3] == '2');
	assert1(header.type == 1);
	
	m_TextureType = GL_TEXTURE_2D;
	m_TextureWidth = header.width;
	m_TextureHeight = header.height;
	m_TextureDepth = 1;
	m_bGenerateMipmaps = false;

	glActiveTexture(GL_TEXTURE15);
	glBindTexture(GL_TEXTURE_2D, m_GLObj);

	uint8 mipmax = header.has_mips ? 16 : 1;
	switch (header.compression)
	{
	case 1:
	{
		GLenum internalFormat = GL_RGBA8;
		GLenum inputFormat = GL_RGBA;
		GLenum inputType = GL_UNSIGNED_BYTE;

		unsigned int pal[256];
		f->readBytes(pal, 1024);

		unsigned char* buf = new unsigned char[header.mipSizes[0]];
		unsigned int* buf2 = new unsigned int[header.width * header.height];
		unsigned int* p;
		unsigned char* c, *a;

		//bool hasalpha = (header.alpha_depth != 0);
		//_texture->createTexture(R_TextureTypes::Tex2D, header.width, header.height, 1, R_TextureFormats::RGBA8, header.has_mips, false, false, false);

		for (int i = 0; i < mipmax; i++)
		{
			if (header.mipOffsets[i] && header.mipSizes[i])
			{
				f->seek(header.mipOffsets[i]);
				f->readBytes(buf, header.mipSizes[i]);

				int cnt = 0;
				p = buf2;
				c = buf;
				a = buf + header.width * header.height;
				for (uint32 y = 0; y < header.height; y++)
				{
					for (uint32 x = 0; x < header.width; x++)
					{
						unsigned int k = pal[*c++];
						k = ((k & 0x00FF0000) >> 16) | ((k & 0x0000FF00)) | ((k & 0x000000FF) << 16);
						int alpha;

						if (header.alpha_depth == 8)
						{
							alpha = (*a++);
						}
						else if (header.alpha_depth == 1)
						{
							alpha = (*a & (1 << cnt++)) ? 0xff : 0;
							if (cnt == 8)
							{
								cnt = 0;
								a++;
							}
						}
						else if (header.alpha_depth == 0)
						{
							alpha = 0xff;
						}


						k |= alpha << 24;
						*p++ = k;
					}
				}

				glTexImage2D(GL_TEXTURE_2D, i, internalFormat, m_TextureWidth, m_TextureHeight, 0, inputFormat, inputType, buf);
				OGLCheckError();
			}
			else
			{
				break;
			}
		}

		delete[] buf2;
		delete[] buf;
	}
	break;
	case 2:
	{
		GLenum internalFormat;
		GLenum inputFormat = GL_RGBA;
		GLenum inputType = GL_UNSIGNED_BYTE;

		if (header.alpha_type == 0)
		{
			internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		}
		else if (header.alpha_type == 1)
		{
			internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		}
		else if (header.alpha_type == 7)
		{
			internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		}
		else
		{
			_ASSERT(false);
		}

		//_texture->createTexture(R_TextureTypes::Tex2D, header.width, header.height, 1, format, header.has_mips, false, true, false);

		uint8* buf = new uint8[header.mipSizes[0]];
		for (uint8 i = 0; i < mipmax; i++)
		{
			if (header.mipOffsets[i])
			{
				assert1(header.mipSizes[i] > 0);

				f->seek(header.mipOffsets[i]);
				f->readBytes(buf, header.mipSizes[i]);

				uint16 width = std::max(m_TextureWidth >> i, 1), height = std::max(m_TextureHeight >> i, 1);
				glCompressedTexImage2D(GL_TEXTURE_2D, i, internalFormat, width, height, 0, GLTextureCompressedSize(internalFormat, width, height, m_TextureDepth), buf);
				OGLCheckError();
			}
			else
			{
				break;
			}
		}

		delete[] buf;
	}
	break;
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}

bool TextureOGL::LoadTextureCube(cstring fileName)
{
	m_TextureType = GL_TEXTURE_CUBE_MAP;

	return false;
}

void TextureOGL::GenerateMipMaps()
{
	if (m_bGenerateMipmaps && (m_TextureType != GL_TEXTURE_CUBE_MAP))
	{
		glGenerateMipmap(m_TextureType);
	}
}

std::shared_ptr<Texture> TextureOGL::GetFace(CubeFace face) const
{
	return std::static_pointer_cast<Texture>(std::const_pointer_cast<TextureOGL>(shared_from_this()));
}

std::shared_ptr<Texture> TextureOGL::GetSlice(uint32 slice) const
{
	return std::static_pointer_cast<Texture>(std::const_pointer_cast<TextureOGL>(shared_from_this()));
}

uint16_t TextureOGL::GetWidth() const
{
	return m_TextureWidth;
}

uint16_t TextureOGL::GetHeight() const
{
	return m_TextureHeight;
}

uint16_t TextureOGL::GetDepth() const
{
	return m_TextureDepth;
}

uint8_t TextureOGL::GetBPP() const
{
	return m_BPP;
}

bool TextureOGL::IsTransparent() const
{
	return m_bIsTransparent;
}

// Resize

void TextureOGL::Resize2D(uint16_t width, uint16_t height)
{
	Log::Error("TextureOGL::Resize2D Not implemented!");
}

void TextureOGL::ResizeCube(uint16_t size)
{
	Log::Error("TextureOGL::ResizeCube Not implemented!");
}

void TextureOGL::Resize(uint16_t width, uint16_t height, uint16_t depth)
{
	switch (m_TextureDimension)
	{
	case Dimension::Texture2D:
	case Dimension::Texture2DArray:
		Resize2D(width, height);
		break;
	case Texture::Dimension::TextureCube:
		ResizeCube(width);
		break;
	default:
		std::exception("Unknown texture dimension.");
	}
}

void TextureOGL::Plot(glm::ivec2 coord, const uint8_t* pixel, size_t size)
{
	assert(m_BPP > 0 && m_BPP % 8 == 0);
	assert(coord.s < m_TextureWidth && coord.t < m_TextureHeight && size == (m_BPP / 8));

	uint8_t bytesPerPixel = (m_BPP / 8);
	uint32_t stride = m_TextureWidth * bytesPerPixel;
	uint32_t index = (coord.s * bytesPerPixel) + (coord.t * stride);

	for (uint32 i = 0; i < size; ++i)
	{
		m_Buffer[index + i] = *(pixel + i);
	}

	m_bIsDirty = true;
}

void TextureOGL::FetchPixel(glm::ivec2 coord, uint8_t*& pixel, size_t size)
{
	assert(m_BPP > 0 && m_BPP % 8 == 0);
	assert(coord.s < m_TextureWidth && coord.t < m_TextureHeight && size == (m_BPP / 8));

	uint8_t bytesPerPixel = (m_BPP / 8);
	uint32_t stride = m_TextureWidth * bytesPerPixel;
	uint32_t index = (coord.s * bytesPerPixel) + (coord.t * stride);
	pixel = &m_Buffer[index];
}

void TextureOGL::Copy(std::shared_ptr<Texture> other)
{
	std::shared_ptr<TextureOGL> srcTexture = std::dynamic_pointer_cast<TextureOGL>(other);

	if (srcTexture && srcTexture.get() != this)
	{
		if (m_TextureDimension == srcTexture->m_TextureDimension &&
			m_TextureWidth == srcTexture->m_TextureWidth &&
			m_TextureHeight == srcTexture->m_TextureHeight
		)
		{
			switch (m_TextureDimension)
			{
			case Texture::Dimension::Texture2D:
			case Texture::Dimension::Texture2DArray:
				throw std::exception("Not implemented!");
				//m_pDeviceContext->CopyResource(m_pTexture2D, srcTexture->m_pTexture2D);
				break;
			case Texture::Dimension::TextureCube:
				throw std::exception("Not implemented!");
				//m_pDeviceContext->CopyResource(m_pTexture3D, srcTexture->m_pTexture3D);
				break;
			}
		}
		else
		{
			Log::Error("Incompatible source texture.");
		}
	}

	/*if (((int)m_CPUAccess & (int)CPUAccess::Read) != 0 && m_pTexture2D)
	{
		D3D11_MAPPED_SUBRESOURCE mappedResource;

		// Copy the texture data from the texture resource
		if (FAILED(m_pDeviceContext->Map(m_pTexture2D, 0, D3D11_MAP_READ, 0, &mappedResource)))
		{
			Log::Error("Failed to map texture resource for reading.");
		}

		memcpy_s(m_Buffer.data(), m_Buffer.size(), mappedResource.pData, m_Buffer.size());

		m_pDeviceContext->Unmap(m_pTexture2D, 0);
	}*/
}

void TextureOGL::Clear(ClearFlags clearFlags, cvec4 color, float depth, uint8_t stencil)
{
	/*if (m_pRenderTargetView && ((int)clearFlags & (int)ClearFlags::Color) != 0)
	{
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, glm::value_ptr(color));
	}

	{
		UINT flags = 0;
		flags |= ((int)clearFlags & (int)ClearFlags::Depth) != 0 ? D3D11_CLEAR_DEPTH : 0;
		flags |= ((int)clearFlags & (int)ClearFlags::Stencil) != 0 ? D3D11_CLEAR_STENCIL : 0;
		if (m_pDepthStencilView && flags > 0)
		{
			m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, flags, depth, stencil);
		}
	}*/
}

void TextureOGL::Bind(uint32_t ID, std::weak_ptr<Shader> shader, ShaderParameter::Type parameterType)
{
	if (m_bIsDirty)
	{
		/*if (m_bDynamic && m_pTexture2D)
		{
			D3D11_MAPPED_SUBRESOURCE mappedResource;

			// Copy the texture data to the texture resource
			HRESULT hr = m_pDeviceContext->Map(m_pTexture2D, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			if (FAILED(hr))
			{
				Log::Error("Failed to map texture resource for writing.");
			}

			memcpy_s(mappedResource.pData, m_Buffer.size(), m_Buffer.data(), m_Buffer.size());

			m_pDeviceContext->Unmap(m_pTexture2D, 0);

			if (m_bGenerateMipmaps)
			{
				m_pDeviceContext->GenerateMips(m_pShaderResourceView);
			}
		}*/
		m_bIsDirty = false;
	}

	std::shared_ptr<ShaderOGL> pShader = std::dynamic_pointer_cast<ShaderOGL>(shader.lock());
	_ASSERT(pShader != NULL);

	if (pShader->GetType() != Shader::ShaderType::PixelShader)
	{
		return;
	}

	glProgramUniform1i(pShader->GetGLObject(), ID, ID);

	glActiveTexture(GL_TEXTURE0 + ID);
	glBindTexture(m_TextureType, m_GLObj);

}

void TextureOGL::UnBind(uint32_t ID, std::weak_ptr<Shader> shader, ShaderParameter::Type parameterType)
{
	std::shared_ptr<ShaderOGL> pShader = std::dynamic_pointer_cast<ShaderOGL>(shader.lock());
	_ASSERT(pShader != NULL);

	glProgramUniform1i(pShader->GetGLObject(), ID, 0);

	glActiveTexture(GL_TEXTURE0 + ID);
	glBindTexture(m_TextureType, 0);
}

uint32 TextureOGL::GetGLObject()
{
	return m_GLObj;
}
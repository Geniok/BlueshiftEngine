#include "Precompiled.h"
#include "Render/Render.h"
#include "RenderInternal.h"

BE_NAMESPACE_BEGIN

static const char   cubemap_postfix[6][3] = { "px", "nx", "py", "ny", "pz", "nz" };
static const char   camera_cubemap_postfix[6][8] = { "rt", "lf", "up", "dn", "ft", "bk" };

int Texture::MemRequired(bool includingMipmaps) const {
    int numMipmaps = includingMipmaps ? Image::MaxMipMapLevels(width, height, depth) : 1;
    int size = Image::MemRequired(width, height, depth, numMipmaps, format);
    return size;
}

void Texture::Create(Renderer::TextureType type, const Image &srcImage, int flags) {
    Purge();

    this->type = type;
    this->textureHandle = glr.CreateTexture(type);
    this->flags = flags;

    /*if (flags & HighPriority) {
        glr.PrioritizeTexture(texture, 1.0f);
    } else if (flags & LowPriority) {
        glr.PrioritizeTexture(texture, 0.0f);
    }*/

    Upload(&srcImage);
}

void Texture::CreateEmpty(Renderer::TextureType type, int width, int height, int depth, int numSlices, Image::Format format, int flags) {
    Purge();

    this->type = type;
    this->textureHandle = glr.CreateTexture(type);
    this->flags = flags;

    Image image;
    image.InitFromMemory(width, height, depth, type == Renderer::TextureCubeMap ? 6 : numSlices, 1, format, nullptr, 0);
    Upload(&image);
}

void Texture::CreateFromBuffer(Image::Format format, Renderer::Handle bufferHandle) {
    Purge();

    this->type = Renderer::TextureBuffer;
    this->textureHandle = glr.CreateTexture(Renderer::TextureBuffer);
    this->flags = 0;
    this->format = format;

    glr.BindBuffer(Renderer::TexelBuffer, bufferHandle);

    glr.SelectTextureUnit(0);
    glr.BindTexture(textureHandle);
    glr.SetTextureImageBuffer(format, false, bufferHandle);

    glr.BindBuffer(Renderer::TexelBuffer, Renderer::NullBuffer);
}

// Indirection Cube map : cubic coord -> VCM coord
void Texture::CreateIndirectionCubemap(int size, int vcmWidth, int vcmHeight, int flags) {
    Image cubeImage;
    cubeImage.CreateCube(size, 1, Image::LA_16_16, nullptr, Image::LinearFlag);

    word *dstPtr = (word *)cubeImage.GetPixels();

    int vcmFaceWidth = vcmWidth / 3;
    int vcmFaceHeight = vcmHeight / 2;

    // VCM layout
    // ----------------
    // | PX | PY | PZ |
    // |----+----+----| vcmHeight
    // | NX | NY | NZ |
    // ----------------
    //     vcmWidth

    for (int i = Renderer::PositiveX; i <= Renderer::NegativeZ; i++) {
        for (int y = 0; y < size; y++) {
            Vec2 vcmSt;
            vcmSt.y = ((float)y / (float)(size - 1)) * vcmFaceHeight;
            vcmSt.y = (vcmSt.y + vcmFaceHeight * (i & 1)) / vcmHeight;
            vcmSt.y = vcmSt.y * 65535.0f;

            for (int x = 0; x < size; x++) {
                vcmSt.x = ((float)x / (float)(size - 1)) * vcmFaceWidth;
                vcmSt.x = (vcmSt.x + vcmFaceWidth * (i >> 1)) / vcmWidth;
                vcmSt.x = vcmSt.x * 65535.0f;

                *dstPtr++ = (word)(vcmSt.x);
                *dstPtr++ = (word)(vcmSt.y);
            }
        }
    }

    Create(Renderer::TextureCubeMap, cubeImage, Clamp | NoMipmaps | HighQuality | flags);
}

/*
-------------------------------------------------------------------------------

    default map

-------------------------------------------------------------------------------
*/

void Texture::CreateDefaultTexture(int size, int flags) {
    Image image;
    image.Create2D(size, size, 1, Image::RGB_8_8_8, nullptr, Image::SRGBFlag);
    byte *dst = image.GetPixels();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (x == 0 || x == size-1 || y == 0 || y == size-1) {
                dst[3 * (y*size + x) + 0] = (byte)(0);
                dst[3 * (y*size + x) + 1] = (byte)(0);
                dst[3 * (y*size + x) + 2] = (byte)(0);
            } else {
                dst[3 * (y*size + x) + 0] = (byte)(255);
                dst[3 * (y*size + x) + 1] = (byte)(0);
                dst[3 * (y*size + x) + 2] = (byte)(255);
            }
        }
    }

    Create(Renderer::Texture2D, image, Texture::HighQuality | flags);
}

/*
-------------------------------------------------------------------------------

    zero clamp map

-------------------------------------------------------------------------------
*/

void Texture::CreateZeroClampTexture(int size, int flags) {
    Image image;
    image.Create2D(size, size, 1, Image::LA_8_8, nullptr, Image::LinearFlag);
    byte *dst = image.GetPixels();

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            byte c = (byte)((x==0 || x==size-1 || y==0 || y==size-1) ? 0 : 255);
            dst[(y*size + x) * 2 + 0] = c;
            dst[(y*size + x) * 2 + 1] = c;
        }
    }

    Create(Renderer::Texture2D, image, Texture::ZeroClamp | Texture::HighQuality | flags);
}

/*
-------------------------------------------------------------------------------

    flat normal map

-------------------------------------------------------------------------------
*/

void Texture::CreateFlatNormalTexture(int size, int flags) {
    Image image;
    image.Create2D(size, size, 1, Image::RGB_8_8_8, nullptr, Image::LinearFlag);
    byte *dst = image.GetPixels();
        
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            dst[(y*size + x)*3 + 0] = 127;
            dst[(y*size + x)*3 + 1] = 127;
            dst[(y*size + x)*3 + 2] = 255;
        }
    }

    Create(Renderer::Texture2D, image, Texture::NormalMap | Texture::NoScaleDown | flags);
}

/*
-------------------------------------------------------------------------------

    default cube map

-------------------------------------------------------------------------------
*/

void Texture::CreateDefaultCubeMapTexture(int size, int flags) {
    Image image;
    image.CreateCube(size, 1, Image::L_8, nullptr, Image::SRGBFlag);
    byte *dst = image.GetPixels();

    int faceSize = image.GetSliceSize();

    for (int i = 0; i < 6; i++) {
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                dst[y*size + x] = (byte)((x==0 || x==size-1 || y==0 || y==size-1) ? 255 : 0);
            }
        }

        dst += faceSize;
    }

    Create(Renderer::TextureCubeMap, image, Texture::Clamp | Texture::NoCompression | Texture::NoScaleDown | flags);
}

/*
-------------------------------------------------------------------------------

    black cube map

-------------------------------------------------------------------------------
*/

void Texture::CreateBlackCubeMapTexture(int size, int flags) {
    Image image;
    image.CreateCube(size, 1, Image::L_8, nullptr, Image::SRGBFlag);
    byte *dst = image.GetPixels();

    int facesize = image.GetSliceSize();

    for (int i = 0; i < 6; i++) {
        memset(dst, 0, facesize);
        dst += facesize;
    }

    Create(Renderer::TextureCubeMap, image, Texture::Clamp | Texture::NoCompression | Texture::NoScaleDown | flags);
}

/*
-------------------------------------------------------------------------------

    Normalization cube map

-------------------------------------------------------------------------------
*/

void Texture::CreateNormalizationCubeMapTexture(int size, int flags) {
    Image image;
    image.CreateCube(size, 1, Image::RGB_8_8_8, nullptr, Image::LinearFlag);
    byte *dst = image.GetPixels();

    int facesize = image.GetSliceSize();
    Vec3 vec;

    for (int i = 0; i < 6; i++) {
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                R_CubeCoord(size, i, x, y, vec);
                vec.Normalize();
                dst[3 * (y * size + x) + 0] = (byte)(128 + 127 * vec[0]);
                dst[3 * (y * size + x) + 1] = (byte)(128 + 127 * vec[1]);
                dst[3 * (y * size + x) + 2] = (byte)(128 + 127 * vec[2]);
            }
        }

        dst += facesize;
    }

    Create(Renderer::TextureCubeMap, image, Texture::Clamp | Texture::NoMipmaps | Texture::NoCompression | Texture::NoScaleDown | flags);
}

/*
-------------------------------------------------------------------------------

    Cubic normal cube map

-------------------------------------------------------------------------------
*/

void Texture::CreateCubicNormalCubeMapTexture(int size, int flags) {
    char faceNormal[6][3];
    
    faceNormal[0][0] = 127;
    faceNormal[0][1] = 0;
    faceNormal[0][2] = 0;

    faceNormal[1][0] = -128;
    faceNormal[1][1] = 0;
    faceNormal[1][2] = 0;

    faceNormal[2][0] = 0;
    faceNormal[2][1] = 127;
    faceNormal[2][2] = 0;

    faceNormal[3][0] = 0;
    faceNormal[3][1] = -128;
    faceNormal[3][2] = 0;

    faceNormal[4][0] = 0;
    faceNormal[4][1] = 0;
    faceNormal[4][2] = 127;

    faceNormal[5][0] = 0;
    faceNormal[5][1] = 0;
    faceNormal[5][2] = -128;

    Image image;
    image.CreateCube(size, 1, Image::RGB_SNORM_8_8_8, nullptr, Image::LinearFlag);
    char *dst = (char *)image.GetPixels();

    int facesize = image.GetSliceSize();

    for (int i = 0; i < 6; i++) {
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                dst[3 * (y * size + x) + 0] = faceNormal[i][0];
                dst[3 * (y * size + x) + 1] = faceNormal[i][1];
                dst[3 * (y * size + x) + 2] = faceNormal[i][2];
            }
        }

        dst += facesize;
    }

    Create(Renderer::TextureCubeMap, image, Texture::Nearest | Texture::Clamp | Texture::NoMipmaps | Texture::NoCompression | Texture::NoScaleDown | flags);
}

/*
-------------------------------------------------------------------------------

    Attenuation map

-------------------------------------------------------------------------------
*/

void Texture::CreateAttenuationTexture(int size, int flags) {
    Image image;
    image.Create2D(size, size, 1, Image::LA_8_8, nullptr, Image::LinearFlag);
    byte *dst = image.GetPixels();

    float centerx = size / 2.0;
    float centery = size / 2.0;
    float radius_squared = Square(size / 2.0);

    for (int s = 0; s < size; s++) {
        for (int t = 0; t < size; t++) {
            float dist_squared = Square(s - centerx) + Square(t - centery);
            if (dist_squared < radius_squared) {
                float falloff = 1.0f - dist_squared / radius_squared;
                byte value = Square(falloff) * 255.0;
                dst[(t * size + s) * 2] = value;
                dst[(t * size + s) * 2 + 1] = value;
            } else {
                dst[(t * size + s) * 2] = 0;
                dst[(t * size + s) * 2 + 1] = 0;
            }
        }
    }

    Create(Renderer::Texture2D, image, Texture::Clamp | Texture::NoMipmaps | Texture::NoCompression | Texture::NoScaleDown | flags);
}

/*
-------------------------------------------------------------------------------

    Exponent map 

-------------------------------------------------------------------------------
*/

void Texture::CreateExponentTexture(int flags) {
    Image image;
    image.Create2D(256, 256, 1, Image::L_8, nullptr, Image::LinearFlag);
    byte *dst = image.GetPixels();

    for (int t = 0; t < 256; t++) {
        for (int s = 0; s < 256; s++) {
            int c = Math::Pow(s/255.0, t+1) * 255;
            dst[t*256 + s] = c;
        }
    }

    Create(Renderer::Texture2D, image, Texture::Nearest | Texture::NoMipmaps | Texture::Clamp | Texture::HighQuality | flags);
}

/*
-------------------------------------------------------------------------------

    Fog map

-------------------------------------------------------------------------------
*/

void Texture::CreateFogTexture(int flags) {
    Image image;
    image.Create2D(256, 1, 1, Image::A_8, nullptr, Image::LinearFlag);
    byte *dst = image.GetPixels();

    for (int s = 0; s < 256; s++) {
        int c = sqrt(s / 255.0) * 255;
        dst[s] = Min(c, 255);
    }

    Create(Renderer::Texture2D, image, Texture::NoMipmaps | Texture::Clamp | Texture::HighQuality | flags);
}

/*
-------------------------------------------------------------------------------

    Fog enter map

-------------------------------------------------------------------------------
*/

void Texture::CreateFogEnterTexture(int flags) {
    Image image;
    image.Create2D(256, 1, 1, Image::A_8, nullptr, Image::LinearFlag);
    byte *dst = image.GetPixels();

    for (int s = 0; s < 256; s++) {
        int c = sqrt(s / 255.0) * 255;
        dst[s] = Min(c, 255);
    }

    Create(Renderer::Texture2D, image, Texture::NoMipmaps | Texture::Clamp | Texture::HighQuality | flags);
}

/*
-------------------------------------------------------------------------------

    Random rotation matrix map

-------------------------------------------------------------------------------
*/

void Texture::CreateRandomRotMatTexture(int size, int flags) {
    Image image;
    image.Create2D(size, size, 1, Image::RGBA_8_8_8_8, nullptr, Image::LinearFlag);
    byte *dst = image.GetPixels();
    float s, c;

    Random random;
    random.SetSeed(23119);
    
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            Math::SinCos(random.RandomFloat() * Math::TwoPi, s, c);
                
            // cos, sin 값만 저장
            dst[4 * (y * 64 + x) + 0] = ClampByte((+c + 1.0f) * 0.5f * 255 + 0.5f);
            dst[4 * (y * 64 + x) + 1] = ClampByte((-s + 1.0f) * 0.5f * 255 + 0.5f);
            dst[4 * (y * 64 + x) + 2] = ClampByte((+s + 1.0f) * 0.5f * 255 + 0.5f);
            dst[4 * (y * 64 + x) + 3] = ClampByte((+c + 1.0f) * 0.5f * 255 + 0.5f);
        }
    }

    Create(Renderer::Texture2D, image, Texture::Nearest | Texture::Repeat | Texture::NoMipmaps | Texture::NoCompression | Texture::NoScaleDown | flags);
}

/*
-------------------------------------------------------------------------------

    Random direction map for SSAO

-------------------------------------------------------------------------------
*/

void Texture::CreateRandomDir4x4Texture(int flags) {
    Image image;
    image.Create2D(4, 4, 1, Image::RGB_8_8_8, nullptr, Image::LinearFlag);
    byte *dst = image.GetPixels();
    Vec3 dir;

    Random random;
    random.SetSeed(810202);

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            dir.x = random.RandomFloat();
            dir.y = random.RandomFloat();
            dir.z = random.RandomFloat();
            dir.Normalize();

            dir = (dir + Vec3(1.0f, 1.0f, 1.0f)) * 0.5f;

            dst[3 * (y * 4 + x) + 0] = ClampByte(dir.x * 255.0f + 0.5f);
            dst[3 * (y * 4 + x) + 1] = ClampByte(dir.y * 255.0f + 0.5f);
            dst[3 * (y * 4 + x) + 2] = ClampByte(dir.z * 255.0f + 0.5f);
        }
    }

    Create(Renderer::Texture2D, image, Texture::Nearest | Texture::Repeat | Texture::NoMipmaps | Texture::NoCompression | Texture::NoScaleDown | flags);
}

static Renderer::AddressMode TextureFlagsToAddressMode(int flags) {
    if (flags & (Texture::ClampToBorder | Texture::ZeroClamp)) {
        return Renderer::ClampToBorder;
    } else if (flags & Texture::Clamp) {
        return Renderer::Clamp;
    } else if (flags & Texture::MirroredRepeat) {
        return Renderer::MirroredRepeat;
    } else {
        return Renderer::Repeat;
    }
}

void Texture::Upload(const Image *srcImage) {
    Image::Format srcFormat = srcImage->GetFormat();
    Image::Format forceFormat = Image::Format::UnknownFormat;
    Image tmpImage;

    if (type == Renderer::TextureRectangle) {
        flags |= (NoMipmaps | Clamp | NoScaleDown);
    }

    if (srcImage->IsEmpty()) {
        flags |= (NoScaleDown | NoCompression);
        forceFormat = srcImage->GetFormat();
    }

    bool useNormalMap   = (flags & NormalMap) ? true : false;
    bool useCompression = !(flags & NoCompression) ? TextureManager::texture_useCompression.GetBool() : false;
    bool useNPOT        = (flags & NonPowerOfTwo) ? true : false;
    bool useSRGB        = ((flags & SRGB) && TextureManager::texture_sRGB.GetBool()) ? true : false;

    Image::Format dstFormat;
    if (forceFormat != Image::Format::UnknownFormat) {
        dstFormat = forceFormat;
    } else {
        glr.AdjustTextureFormat(type, useCompression, useNormalMap, srcImage->GetFormat(), &dstFormat);
    }
    
    this->srcWidth      = srcImage->GetWidth();
    this->srcHeight     = srcImage->GetHeight();
    this->srcDepth      = srcImage->GetDepth();
    this->numSlices     = srcImage->NumSlices();
    
    int dstWidth, dstHeight, dstDepth;
    glr.AdjustTextureSize(type, useNPOT, srcWidth, srcHeight, srcDepth, &dstWidth, &dstHeight, &dstDepth);

    // scale down mip level 적용
    int mipLevel = !(flags & NoScaleDown) ? TextureManager::texture_mipLevel.GetInteger() : 0;
    if (mipLevel > 0) {
        dstWidth = Max(dstWidth >> mipLevel, 1);
        dstHeight = Max(dstHeight >> mipLevel, 1);
        dstDepth = Max(dstDepth >> mipLevel, 1);
    }
    
    // depth 텍스쳐는 텍셀을 정의할 수 없다
    if (!srcImage->IsEmpty() && Image::IsDepthFormat(srcImage->GetFormat())) {
        BE_FATALERROR(L"Texture::Upload: Couldn't upload texel data of depth texture");
    }
    
    Image scaledImage;
    
    if (!srcImage->IsEmpty()) {
        if (srcWidth != dstWidth || srcHeight != dstHeight) {
            srcImage->Resize(dstWidth, dstHeight, Image::Bicubic, scaledImage);
            srcImage = &scaledImage;
        }
    }

    this->format        = dstFormat;
    this->width         = dstWidth;
    this->height        = dstHeight;
    this->depth         = dstDepth;
    
    this->hasMipmaps    = !(flags & NoMipmaps);
    this->permanence    = !!(flags & Permanence);
    this->addressMode   = TextureFlagsToAddressMode(flags);

    glr.BindTexture(textureHandle);

    if (flags & ZeroClamp) {
        glr.SetTextureBorderColor(Color4(0.0f, 0.0f, 0.0f, 0.0f));
    }

    if (flags & Shadow) {
        glr.SetTextureBorderColor(Color4(1.0f, 1.0f, 1.0f, 1.0f));
        glr.SetTextureShadowFunc(true);
    }

    glr.SetTextureImage(type, srcImage, dstFormat, hasMipmaps, useSRGB);

    glr.SetTextureAddressMode(addressMode);

    if (hasMipmaps) {
        glr.SetTextureFilter((flags & Nearest) ? Renderer::NearestMipmapNearest : textureManager.textureFilter);	
    } else {
        glr.SetTextureFilter((flags & Nearest) ? Renderer::Nearest : Renderer::Linear);
    }

    if (hasMipmaps && !(flags & Nearest)) {
        glr.SetTextureAnisotropy(textureManager.textureAnisotropy);
    }
}

void Texture::Update2D(int xoffset, int yoffset, int width, int height, Image::Format format, const byte *data) {
    glr.SetTextureSubImage2D(0, xoffset, yoffset, width, height, format, data);	
}

void Texture::Update3D(int xoffset, int yoffset, int zoffset, int width, int height, int depth, Image::Format format, const byte *data) {
    glr.SetTextureSubImage3D(0, xoffset, yoffset, zoffset, width, height, depth, format, data);
}

void Texture::UpdateCubemap(int face, int xoffset, int yoffset, int width, int height, Image::Format format, const byte *data) {
    glr.SetTextureSubImageCube((Renderer::CubeMapFace)face, 0, xoffset, yoffset, width, height, format, data);
}

void Texture::UpdateRect(int xoffset, int yoffset, int width, int height, Image::Format format, const byte *data) {
    glr.SetTextureSubImageRect(xoffset, yoffset, width, height, format, data);
}

void Texture::GetTexels2D(Image::Format format, void *pixels) const {
    glr.GetTextureImage2D(0, format, pixels);
}

void Texture::GetTexels3D(Image::Format format, void *pixels) const {
    glr.GetTextureImage3D(0, format, pixels);
}

void Texture::GetTexelsCubemap(int face, Image::Format format, void *pixels) const {
    glr.GetTextureImageCube((Renderer::CubeMapFace)face, 0, format, pixels);
}

void Texture::GetTexelsRect(Image::Format format, void *pixels) const {
    glr.GetTextureImageRect(format, pixels);
}

void Texture::Purge() {
    if (textureHandle != Renderer::NullTexture) {
        glr.DeleteTexture(textureHandle);
    }

    textureHandle = Renderer::NullTexture;
}

bool Texture::Load(const char *filename, int flags) {
    flags |= LoadedFromFile;

    if (flags & (CubeMap | CameraCubeMap)) {
        Str name = filename;
        name.StripFileExtension();
        Image images[6];

        for (int i = 0; i < 6; i++) {
            Str filename2 = name + "_" + ((flags & CameraCubeMap) ? camera_cubemap_postfix[i] : cubemap_postfix[i]);
            BE_LOG(L"Loading texture '%hs'...\n", filename2.c_str());

            images[i].Load(filename2.c_str());
            
            if (images[i].IsEmpty()) {
                BE_WARNLOG(L"Couldn't load texture \"%hs\"\n", filename2.c_str());
                return false;
            }
        }

        Image cubeImage;
        cubeImage.CreateCubeFromMultipleImages(images);
        Create(Renderer::TextureCubeMap, cubeImage, flags);
    } else {
        BE_LOG(L"Loading texture '%hs'...\n", filename);

        Image image;
        image.Load(filename);
        
        if (image.IsEmpty()) {
            BE_WARNLOG(L"Couldn't load texture \"%hs\"\n", filename);
            return false;
        }

        Create(Renderer::Texture2D, image, flags);
    }

    return true;
}

bool Texture::Reload() {
    if (!(flags & LoadedFromFile)) {
        return false;
    }

    Str _hashName = this->hashName;

    const Str textureInfoPath = _hashName + ".texinfo";
    int _flags = textureManager.LoadTextureInfo(textureInfoPath);

    bool ret = Load(_hashName, _flags);
    return ret;
}

void Texture::Bind() const {
    glr.BindTexture(textureHandle);
}

//--------------------------------------------------------------------------------------------------

void TextureGenerator_CubicDiffuseIrradianceMap::Generate(Texture *texture) const {
    if (texture->GetType() != Renderer::TextureCubeMap) {
        BE_ERRLOG(L"TextureGenerator_CubicDiffuseIrradianceMap::Generate: src texture must be cube map\n");
        return;
    }
}

void TextureGenerator_CubicSpecularIrradianceMap::Generate(Texture *texture) const {
    if (texture->GetType() != Renderer::TextureCubeMap) {
        BE_ERRLOG(L"TextureGenerator_CubicSpecularIrradianceMap::Generate: src texture must be cube map\n");
        return;
    }
}

BE_NAMESPACE_END

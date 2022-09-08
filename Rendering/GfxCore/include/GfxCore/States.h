#pragma once

#include <boost/functional/hash_fwd.hpp>

namespace RBX
{
namespace Graphics
{

template<typename T> struct StateHasher
{
    std::size_t operator()(const T& state) const
    {
        return state.getHashId();
    }
};

class RasterizerState
{
public:
    enum CullMode
	{
        Cull_None,
        Cull_Back,
        Cull_Front,

        Cull_Count
	};

    RasterizerState(CullMode cullMode, int depthBias = 0)
		: cullMode(cullMode)
        , depthBias(depthBias)
    {
	}

	CullMode getCullMode() const { return cullMode; }
	int getDepthBias() const { return depthBias; }

    bool operator==(const RasterizerState& other) const
	{
        return cullMode == other.cullMode && depthBias == other.depthBias;
	}

    bool operator!=(const RasterizerState& other) const
	{
        return !(*this == other);
	}

    size_t getHashId() const
    {
        size_t seed = 0;
        boost::hash_combine(seed, cullMode);
        boost::hash_combine(seed, depthBias);
        return seed;
    }

private:
    CullMode cullMode;
    int depthBias;
};

class BlendState
{
public:
    enum Mode
	{
        Mode_None,
        Mode_Additive,
        Mode_Multiplicative,
        Mode_AlphaBlend,
        Mode_PremultipliedAlphaBlend,
        Mode_Count
	};

	enum Factor
	{
		Factor_One,
		Factor_Zero,
		Factor_DstColor,
		Factor_SrcAlpha,
		Factor_InvSrcAlpha,
		Factor_DstAlpha,
		Factor_InvDstAlpha,
		Factor_Count
	};

    enum ColorMask
	{
        Color_None = 0,
        Color_R = 1 << 0,
        Color_G = 1 << 1,
        Color_B = 1 << 2,
        Color_A = 1 << 3,
        Color_All = Color_R | Color_G | Color_B | Color_A
	};

    BlendState(Mode mode, unsigned int colorMask = Color_All)
	{
        struct Translation 
        {
            Factor src;
            Factor dst;
        };

        static const Translation translation[Mode_Count] =
        {
            { Factor_One, Factor_Zero },
            { Factor_One, Factor_One },
            { Factor_DstColor, Factor_Zero }, 
            { Factor_SrcAlpha, Factor_InvSrcAlpha }, 
            { Factor_One, Factor_InvSrcAlpha }, 
        };
           
        construct(translation[mode].src, translation[mode].dst, translation[mode].src, translation[mode].dst, colorMask);
	}
	
	BlendState(Factor colorSrcIn, Factor colorDstIn, Factor alphaSrcIn, Factor alphaDstIn, unsigned int colorMask = Color_All)
	{
		construct(colorSrcIn, colorDstIn, alphaSrcIn, alphaDstIn, colorMask);
	}

	BlendState(Factor src, Factor dst, unsigned int colorMask = Color_All)
	{
		construct(src, dst, src, dst, colorMask);
	}
	
	bool blendingNeeded() const { return colorSrc != Factor_One || colorDst != Factor_Zero || alphaSrc != Factor_One || alphaDst != Factor_Zero; }
	bool separateAlphaBlend() const { return colorSrc != alphaSrc || colorDst != alphaDst; }
	unsigned int getColorMask() const { return colorMask; }

	unsigned int getColorSrc() const { return colorSrc; }
	unsigned int getColorDst() const { return colorDst; }
	unsigned int getAlphaSrc() const { return alphaSrc; }
	unsigned int getAlphaDst() const { return alphaDst; }

	bool operator==(const BlendState& other) const
	{
		return colorSrc == other.colorSrc && colorDst == other.colorDst && alphaSrc == other.alphaSrc && alphaDst == other.alphaDst && colorMask == other.colorMask;
	}

	bool operator!=(const BlendState& other) const
	{
        return !(*this == other);
	}

    size_t getHashId() const
    {
        size_t seed = 0;
        boost::hash_combine(seed, colorSrc);
        boost::hash_combine(seed, colorDst);
		boost::hash_combine(seed, alphaSrc);
		boost::hash_combine(seed, alphaDst);
		boost::hash_combine(seed, colorMask);
        return seed;
    }

private:

    void construct(Factor colorSrcIn, Factor colorDstIn, Factor alphaSrcIn, Factor alphaDstIn, unsigned int colorMask)
    {
        this->colorSrc = colorSrcIn;
        this->colorDst = colorDstIn;
        this->alphaSrc = alphaSrcIn;
        this->alphaDst = alphaDstIn;
        this->colorMask = colorMask;
    }

    unsigned int colorMask;

	Factor colorSrc;
	Factor colorDst;
	Factor alphaSrc;
	Factor alphaDst;
};

class DepthState
{
public:
    enum Function
	{
        Function_Always,
        Function_Less,
        Function_LessEqual,

        Function_Count
	};

    enum StencilMode
	{
        Stencil_None,
        Stencil_IsNotZero,
        Stencil_UpdateZFail,

        Stencil_Count
	};

    DepthState(Function function, bool write, StencilMode stencilMode = Stencil_None)
		: function(function)
        , write(write)
        , stencilMode(stencilMode)
	{
	}

	Function getFunction() const { return function; }
	bool getWrite() const { return write; }
	StencilMode getStencilMode() const { return stencilMode; }

	bool operator==(const DepthState& other) const
	{
		return function == other.function && write == other.write && stencilMode == other.stencilMode;
	}

	bool operator!=(const DepthState& other) const
	{
        return !(*this == other);
	}

    size_t getHashId() const
    {
        size_t seed = 0;
        boost::hash_combine(seed, function);
        boost::hash_combine(seed, write);
        boost::hash_combine(seed, stencilMode);
        return seed;
    }

private:
	Function function;
    bool write;
    StencilMode stencilMode;
};

class SamplerState
{
public:
    enum Filter
	{
        Filter_Point,
        Filter_Linear,
        Filter_Anisotropic,

        Filter_Count
	};

    enum Address
	{
        Address_Wrap,
        Address_Clamp,

        Address_Count
	};

    SamplerState(Filter filter, Address address = Address_Wrap, unsigned int anisotropy = 0)
		: filter(filter)
        , address(address)
		, anisotropy(anisotropy)
	{
	}

	Filter getFilter() const { return filter; }
	Address getAddress() const { return address; }
	unsigned int getAnisotropy() const { return anisotropy; }

	bool operator==(const SamplerState& other) const
	{
		return filter == other.filter && address == other.address && anisotropy == other.anisotropy;
	}

	bool operator!=(const SamplerState& other) const
	{
		return !(*this == other);
	}

    size_t getHashId() const
    {
        size_t seed = 0;
        boost::hash_combine(seed, filter);
        boost::hash_combine(seed, address);
        boost::hash_combine(seed, anisotropy);
        return seed;
    }

private:
    Filter filter;
    Address address;
    unsigned int anisotropy;
};

}
}

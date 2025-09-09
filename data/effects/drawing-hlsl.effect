Texture2D image : register(t0);
Texture2D image1 : register(t1);
Texture2D motionMap : register(t2);

SamplerState def_sampler : register(s0);

cbuffer Constants : register(b0)
{
	float texelWidth;
	float texelHeight;
	int kernelSize;

	float strength;
	float motionThreshold;

	bool useLog;
	float scalingFactor;
	float highThreshold;
	float lowThreshold;
};

struct VertInOut {
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

#define TARGET SV_Target

#include "drawing-pixelshaders.effect"

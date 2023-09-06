#define MAX_POINT_LIGHT_NUM 1000

struct PointLightData
{
	float4 pos;
	float4 color;
};

cbuffer LightBufferObject : register(b0)
{
	PointLightData pointLightData[MAX_POINT_LIGHT_NUM];
	uint pointLightNum;
	float3 eye;
}

Texture2D<float4> albedoColorTexture: register(t0);
Texture2D<float4> normalTexture: register(t1);
Texture2D<float4> worldPositionTexture: register(t2);
Texture2D<float> depthBuffer: register(t3);

SamplerState smp: register(s0);

struct VSOutput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOOD;
};
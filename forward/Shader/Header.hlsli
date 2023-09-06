

cbuffer ConstantBufferObject : register(b0)
{
	matrix model;
	matrix view;
	matrix proj;
	float3 eye;
}

struct VSOutput
{
	float4 pos : SV_POSITION;
	float4 norm : NORMAL;
	float3 ray : RAY;
};
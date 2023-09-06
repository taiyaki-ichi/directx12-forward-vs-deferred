
#define MAX_MODEL_NUM 1000

cbuffer ConstantBufferObject : register(b0)
{
	matrix model[MAX_MODEL_NUM];
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
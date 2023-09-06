#define MAX_MODEL_NUM 1000

cbuffer ConstantBufferObject : register(b0)
{
	matrix model[MAX_MODEL_NUM];
	matrix view;
	matrix proj;
}

struct VSOutput
{
	float4 pos : SV_POSITION;
	float4 normal : NORMAL;
	float4 worldPosition : POSITION;
};

struct PSOutput
{
	float4 albedoColor : SV_TARGET0;
	float4 normal : SV_TARGET1;
	float4 worldPosition : SV_TARGET2;
};
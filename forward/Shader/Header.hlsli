
#define MAX_MODEL_NUM 1000
#define MAX_POINT_LIGHT_NUM 1000

cbuffer ConstantBufferObject : register(b0)
{
	matrix model[MAX_MODEL_NUM];
	matrix view;
	matrix proj;
	float3 eye;
}

struct PointLightData
{
	float4 pos;
	float4 color;
};

cbuffer LightBufferObject : register(b1)
{
	PointLightData pointLightData[MAX_POINT_LIGHT_NUM];
	uint pointLightNum;
}

struct VSOutput
{
	float4 pos : SV_POSITION;
	float4 norm : NORMAL;
	float3 ray : RAY;
	float4 worldPos : POSITION;
};
#include"Header.hlsli"

float4 main(float4 pos : POSITION, float4 normal : NORMAL) : SV_POSITION
{
	return mul(mul(proj,view),mul(model,pos));
}
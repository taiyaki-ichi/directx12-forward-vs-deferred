#include"GeometryHeader.hlsli"

VSOutput main(float4 pos : POSITION, float4 normal : NORMAL, uint instanceID : SV_InstanceId)
{
	VSOutput output;
	output.pos = mul(mul(proj, view), mul(model[instanceID], pos));
	output.normal = normalize(mul(model[instanceID], float4(normal.xyz, 0.f)));
	output.worldPosition = mul(model[instanceID], pos);
	return output;
}
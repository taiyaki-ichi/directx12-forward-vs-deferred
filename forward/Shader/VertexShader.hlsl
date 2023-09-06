#include"Header.hlsli"

VSOutput main(float4 pos : POSITION, float4 norm : NORMAL, uint instanceID : SV_InstanceId)
{
	VSOutput output;
	output.pos = mul(mul(proj, view), mul(model[instanceID], pos));
	output.norm = normalize(mul(model[instanceID], float4(norm.xyz, 0.f)));
	output.ray = normalize(mul(model[instanceID], pos) - eye);
	output.worldPos = mul(model[instanceID], pos);
	return output;
}
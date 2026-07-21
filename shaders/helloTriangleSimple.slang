struct VSInput {
    float3 inPosition;
    float3 inColor;
    float2 inTexCoord;
};

struct VSOutput {
    float4 pos : SV_Position;
    float3 color;
    float2 texCoord;
};

struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

ConstantBuffer<UniformBuffer> ubo;

float4 objectToWorld(float4 localPos) {
    return mul(ubo.proj, mul(ubo.view, mul(ubo.model, localPos)));
}

[shader("vertex")]
VSOutput vertMain(VSInput input, uint vid : SV_VertexID) {
    VSOutput output;
    output.pos      = objectToWorld(float4(input.inPosition, 1.0));
    output.color    = float3(input.inColor);
    output.texCoord = input.inTexCoord;
    return output;
}

[shader("fragment")]
float4 fragMain(VSOutput vertIn) : SV_TARGET {
    // return texture.Sample(vertIn.texCoord) * float4(vertIn.color, 1.0f);
    return float4(vertIn.texCoord, 0.0f, 1.0f);

    //return float4(vertIn.color, 1.0f);
}
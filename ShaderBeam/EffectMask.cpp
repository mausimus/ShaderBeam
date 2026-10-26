/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"

#include "EffectMask.h"
#include "Helpers.h"

namespace ShaderBeam
{

namespace
{
const char s_maskShader[] = R"(
cbuffer Params : register(b0)
{
    float4 box;   // the effect area's rectangle: left, top, right, bottom, in pixels
    float4 shape; // corner radius in pixels, curvature x, curvature y, unused
};

Texture2D<float4> captured : register(t0);

float4 VSmain(uint id : SV_VertexID) : SV_Position
{
    // one triangle over the whole output; the scissor rectangle limits it to the effect area
    float2 uv = float2((id << 1) & 2, id & 2);
    return float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
}

// Roughly how many pixels a point is inside the effect area's edge (negative outside).
float EdgeDistance(float2 p)
{
    float2 halfSize = (box.zw - box.xy) * 0.5;
    float2 q        = (p - (box.xy + box.zw) * 0.5) / halfSize; // -1..1 across the rectangle
    float2 k        = shape.yz;
    float2 curved   = q * (1 + k * dot(q, q)) / (1 + k);         // edges at curved = +-1
    float2 inside   = (1 - abs(curved)) * halfSize;
    float  radius   = shape.x;
    if(inside.x < radius && inside.y < radius)
        return radius - length(radius - inside);
    return min(inside.x, inside.y);
}

float4 PSmain(float4 position : SV_Position) : SV_Target
{
    float coverage = saturate(EdgeDistance(position.xy) + 0.5);
    if(coverage >= 1)
        discard;
    // blended over the shader's output by alpha: the captured image wherever the pixel is outside
    return float4(captured.Load(int3(position.xy, 0)).rgb, 1 - coverage);
}
)";

winrt::com_ptr<ID3DBlob> Compile(const char* entryPoint, const char* target)
{
    winrt::com_ptr<ID3DBlob> code;
    winrt::com_ptr<ID3DBlob> errors;
    const UINT               flags = D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_ENABLE_STRICTNESS;
    if(FAILED(D3DCompile(s_maskShader, sizeof(s_maskShader) - 1, "EffectMask", nullptr, nullptr, entryPoint, target, flags, 0, code.put(), errors.put())))
        throw std::runtime_error(std::string("Unable to compile the effect area mask:\n") + (errors ? static_cast<const char*>(errors->GetBufferPointer()) : ""));
    return code;
}
} // namespace

void EffectMask::Create(const RenderContext& renderContext)
{
    const auto& device = renderContext.device;

    const auto vertexCode = Compile("VSmain", "vs_5_0");
    const auto pixelCode  = Compile("PSmain", "ps_5_0");
    THROW(device->CreateVertexShader(vertexCode->GetBufferPointer(), vertexCode->GetBufferSize(), nullptr, m_vertexShader.put()), "Unable to create mask vertex shader");
    THROW(device->CreatePixelShader(pixelCode->GetBufferPointer(), pixelCode->GetBufferSize(), nullptr, m_pixelShader.put()), "Unable to create mask pixel shader");

    D3D11_BUFFER_DESC bufferDesc {};
    bufferDesc.ByteWidth      = 8 * sizeof(float);
    bufferDesc.Usage          = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    THROW(device->CreateBuffer(&bufferDesc, nullptr, m_parameters.put()), "Unable to create mask constant buffer");

    // color = captured * alpha + shader output * (1 - alpha); the output's alpha stays as it is
    D3D11_BLEND_DESC blendDesc {};
    auto&            target      = blendDesc.RenderTarget[0];
    target.BlendEnable           = TRUE;
    target.SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    target.DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    target.BlendOp               = D3D11_BLEND_OP_ADD;
    target.SrcBlendAlpha         = D3D11_BLEND_ZERO;
    target.DestBlendAlpha        = D3D11_BLEND_ONE;
    target.BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    target.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    THROW(device->CreateBlendState(&blendDesc, m_blendState.put()), "Unable to create mask blend state");
}

void EffectMask::Destroy()
{
    m_blendState   = nullptr;
    m_parameters   = nullptr;
    m_pixelShader  = nullptr;
    m_vertexShader = nullptr;
}

void EffectMask::Render(const RenderContext& renderContext, const EffectArea& area)
{
    if(area.IsRectangle() || !m_pixelShader)
        return;

    const auto& context = renderContext.deviceContext;
    const float width   = static_cast<float>(renderContext.options.outputWidth);
    const float height  = static_cast<float>(renderContext.options.outputHeight);
    const float parameters[8] { area.left * width, area.top * height, area.right * width, area.bottom * height, area.radius * height, area.curvatureX, area.curvatureY, 0.0f };

    D3D11_MAPPED_SUBRESOURCE mapped {};
    if(FAILED(context->Map(m_parameters.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
        return;
    memcpy(mapped.pData, parameters, sizeof(parameters));
    context->Unmap(m_parameters.get(), 0);

    ID3D11RenderTargetView* targets[1] = { renderContext.outputTargetView.get() };
    context->OMSetRenderTargets(1, targets, nullptr);
    context->OMSetBlendState(m_blendState.get(), nullptr, 0xffffffff);
    context->VSSetShader(m_vertexShader.get(), nullptr, 0);
    context->PSSetShader(m_pixelShader.get(), nullptr, 0);
    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11Buffer* buffers[1] = { m_parameters.get() };
    context->PSSetConstantBuffers(0, 1, buffers);
    ID3D11ShaderResourceView* views[1] = { renderContext.inputTextureViews[renderContext.inputSlots[0]].get() };
    context->PSSetShaderResources(0, 1, views);

    context->Draw(3, 0);

    ID3D11ShaderResourceView* noViews[1] = { nullptr };
    context->PSSetShaderResources(0, 1, noViews);
    ID3D11Buffer* noBuffers[1] = { nullptr };
    context->PSSetConstantBuffers(0, 1, noBuffers);
    context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}
} // namespace ShaderBeam

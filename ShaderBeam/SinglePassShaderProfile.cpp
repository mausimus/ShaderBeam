/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"

#include "SinglePassShaderProfile.h"
#include "Helpers.h"

namespace ShaderBeam
{

void SinglePassShaderProfile::SetShader(const wchar_t* filename, D3D10_SHADER_MACRO* macros, const RenderContext& renderContext)
{
    ID3DBlob* vertexBlob = nullptr;
    ID3DBlob* pixelBlob  = nullptr;
    ID3DBlob* errorBlob  = nullptr;
    UINT      flags      = D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_ENABLE_STRICTNESS;
    HRESULT   hr;

    hr = D3DCompileFromFile(filename, macros, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSmain", "vs_5_0", flags, 0, &vertexBlob, &errorBlob);
    if(FAILED(hr))
    {
        char* msg = NULL;
        if(errorBlob)
        {
            msg = (char*)errorBlob->GetBufferPointer();
            OutputDebugStringA(msg);
        }
        throw std::runtime_error("Unable to compile vertex shader:\n" + Helpers::WCharToString(filename) + "\n" + (msg ? msg : ""));
    }
    THROW(renderContext.device->CreateVertexShader(vertexBlob->GetBufferPointer(), vertexBlob->GetBufferSize(), NULL, m_vertexShader.put()), "Unable to create vertex shader");

    hr = D3DCompileFromFile(filename, macros, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSmain", "ps_5_0", flags, 0, &pixelBlob, &errorBlob);
    if(FAILED(hr))
    {
        char* msg = NULL;
        if(errorBlob)
        {
            msg = (char*)errorBlob->GetBufferPointer();
            OutputDebugStringA(msg);
        }
        throw std::runtime_error("Unable to compile pixel shader from\n" + Helpers::WCharToString(filename) + "\n" + (msg ? msg : ""));
    }

    THROW(renderContext.device->CreatePixelShader(pixelBlob->GetBufferPointer(), pixelBlob->GetBufferSize(), NULL, m_pixelShader.put()), "Unable to create pixel shader");

    vertexBlob->Release();
    pixelBlob->Release();
}

void SinglePassShaderProfile::SetParameterBuffer(void* data, int size, const RenderContext& renderContext)
{
    m_parametersBuffer = data;
    m_parametersSize   = size;

    D3D11_BUFFER_DESC constantBufferDesc {};
    constantBufferDesc.ByteWidth      = (size + 0xf) & 0xfffffff0;
    constantBufferDesc.Usage          = D3D11_USAGE_DYNAMIC;
    constantBufferDesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    THROW(renderContext.device->CreateBuffer(&constantBufferDesc, nullptr, m_constantBuffer.put()), "Unable to create constant buffer");
}

void SinglePassShaderProfile::CreatePipeline(const RenderContext& renderContext)
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter             = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU           = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressV           = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressW           = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.BorderColor[0]     = 0.0f;
    samplerDesc.BorderColor[1]     = 0.0f;
    samplerDesc.BorderColor[2]     = 0.0f;
    samplerDesc.BorderColor[3]     = 0.0f;
    samplerDesc.ComparisonFunc     = D3D11_COMPARISON_NEVER;
    renderContext.device->CreateSamplerState(&samplerDesc, m_samplerState.put());
}

void SinglePassShaderProfile::RenderPipeline(const RenderContext& renderContext)
{
    renderContext.deviceContext->VSSetShader(m_vertexShader.get(), NULL, 0);
    renderContext.deviceContext->PSSetShader(m_pixelShader.get(), NULL, 0);

    renderContext.deviceContext->IASetInputLayout(NULL);
    renderContext.deviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ID3D11Buffer* buffer[1] = { m_constantBuffer.get() };
    renderContext.deviceContext->VSSetConstantBuffers(0, 1, buffer);
    renderContext.deviceContext->PSSetConstantBuffers(0, 1, buffer);

    ID3D11ShaderResourceView* localResources[MAX_INPUTS];
    for(int slot = 0; slot < renderContext.inputSlots.size(); slot++)
        localResources[slot] = renderContext.inputTextureViews[renderContext.inputSlots[slot]].get();

    renderContext.deviceContext->PSSetShaderResources(2, (UINT)renderContext.inputSlots.size(), localResources);

    ID3D11SamplerState* samplers[1] = { m_samplerState.get() };
    renderContext.deviceContext->PSSetSamplers(2, 1, samplers);

    renderContext.deviceContext->Draw(3, 0);

    ID3D11ShaderResourceView* nullv[] = { nullptr };
    renderContext.deviceContext->PSSetShaderResources(2, 1, nullv);

    ID3D11Buffer* nullb[1] = { nullptr };
    renderContext.deviceContext->VSSetConstantBuffers(0, 1, nullb);
    renderContext.deviceContext->PSSetConstantBuffers(0, 1, nullb);

    ID3D11SamplerState* nulls[1] = { nullptr };
    renderContext.deviceContext->PSSetSamplers(2, 1, nulls);
}

void SinglePassShaderProfile::UpdateParameters(const RenderContext& renderContext)
{
    D3D11_MAPPED_SUBRESOURCE mappedSubresource;
    renderContext.deviceContext->Map(m_constantBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
    memcpy(mappedSubresource.pData, m_parametersBuffer, m_parametersSize);
    renderContext.deviceContext->Unmap(m_constantBuffer.get(), 0);
}

void SinglePassShaderProfile::Destroy()
{
    m_constantBuffer = nullptr;
    m_samplerState   = nullptr;
    m_pixelShader    = nullptr;
    m_vertexShader   = nullptr;
}
} // namespace ShaderBeam
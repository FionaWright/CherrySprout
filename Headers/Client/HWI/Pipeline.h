//
// Created by fiona on 25/09/2025.
//

#ifndef PT_SHADER_H
#define PT_SHADER_H

class Pipeline
{
public:
    void InitGraphics(ID3D12Device* device, const char* vs, const char* ps, D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc, const std::vector<std::string>& compileArgs = {});

    void InitCompute(ID3D12Device* device, const char* cs, ID3D12RootSignature* rootSig, const std::vector<std::string>& compileArgs = {});
    void InitCompute(ID3D12Device* device, const char* cs, D3D12_COMPUTE_PIPELINE_STATE_DESC& desc, const std::vector<std::string>& compileArgs = {});

    ID3D12PipelineState* GetPSO() const { return m_pso.Get(); }

private:
    ComPtr<ID3D12PipelineState> m_pso;
};


#endif //PT_SHADER_H
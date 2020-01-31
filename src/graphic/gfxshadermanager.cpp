#include "graphic/gfxshadermanager.h"

#include "graphic/dx12utils.h"

#include "tmp/shaders/shaderheadersautogen.h"

void GfxShader::Initialize(const AutoGenerated::AutoGeneratedShaderData& data)
{
    bbeProfileFunction();

    assert(data.m_ShaderPermutation != (ShaderPermutation)0xDEADBEEF);
    assert(data.m_ByteCodeArray != nullptr);
    assert(data.m_ByteCodeSize > 0);

    m_ShaderPermutation = data.m_ShaderPermutation;

    DX12_CALL(D3DCreateBlob(data.m_ByteCodeSize, &m_ShaderBlob));
    memcpy(m_ShaderBlob->GetBufferPointer(), data.m_ByteCodeArray, data.m_ByteCodeSize);

    boost::hash_combine(m_Hash, m_ShaderPermutation);
    boost::hash_range(m_Hash, data.m_ByteCodeArray, data.m_ByteCodeArray + data.m_ByteCodeSize);
}

void GfxShaderManager::Initialize()
{
    bbeProfileFunction();

    tf::Taskflow tf;
    tf.parallel_for(AutoGenerated::gs_AllShadersData, AutoGenerated::gs_AllShadersData + _countof(AutoGenerated::gs_AllShadersData), [&](const AutoGenerated::AutoGeneratedShaderData& data)
        {
            const uint32_t shaderIdx = static_cast<uint32_t>(data.m_ShaderPermutation);

            m_AllShaders[shaderIdx].Initialize(data);
        });

    System::GetInstance().GetTasksExecutor().run(tf).wait();
}

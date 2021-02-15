#include <graphic/visual.h>
#include <graphic/pch.h>

#include <system/imguimanager.h>

ForwardDeclareSerializerFunctions(Visual);

template <typename Archive>
void Visual::Serialize(Archive& archive)
{

}

void Visual::UpdateIMGUI()
{
    ScopedIMGUIID scopedID{ this };

    StaticString<256> nameBuffer = m_Name.c_str();

    if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.capacity()))
        m_Name = nameBuffer.c_str();

    StaticString<36> objectIDStr = ToString(m_ObjectID).c_str();
    ImGui::InputText("ObjectID", objectIDStr.data(), objectIDStr.size(), ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("World Position", (float*)&m_WorldPosition);
    if (ImGui::InputFloat4("World Rotation (Quaternion)", (float*)&m_Rotation))
    {
        m_Rotation.Normalize();
    }
    ImGui::InputFloat3("Scale", (float*)&m_Scale);

    auto TextureSelector = [](GfxTexture*& destTex, GfxTexture* defaultTex)
    {
        ScopedIMGUIID scopedID{ destTex };

        if (ImGui::Button("Browse..."))
        {
            auto FileSelectionFinalizer = [&](const std::string& filePath)
            {
                GfxTexture* tex = g_GfxResourceManager.Get<GfxTexture>(filePath, [&](GfxTexture* tex) { destTex = tex; });
                if (tex)
                {
                    g_GfxManager.AddGraphicCommand([&, tex]() { destTex = tex; });
                }
            };

            g_IMGUIManager.RegisterFileDialog("Visual: Select Texture", ".dds,.hdr,.png,.jpg", FileSelectionFinalizer);
        }
        ImGui::SameLine();

        // reset back to default texture
        if (ImGui::Button("Reset"))
        {
            g_GfxManager.AddGraphicCommand([&]() { destTex = defaultTex; });
        }

        destTex->UpdateIMGUI();
    };

    if (ImGui::CollapsingHeader("Diffuse Texture"))
    {
        TextureSelector(m_DiffuseTexture, &GfxDefaultAssets::Checkerboard);
    }
    if (ImGui::CollapsingHeader("Normal Texture"))
    {
        TextureSelector(m_NormalTexture, &GfxDefaultAssets::FlatNormal);
    }
    if (ImGui::CollapsingHeader("PBR (Occlusion, Roughness, Metalness)"))
    {
        ImGui::Checkbox("Use Global PBR consts", &m_UseGlobalPBRConsts);
        TextureSelector(m_ORMTexture, &GfxDefaultAssets::Yellow2D);
    }
}

bool Visual::IsValid()
{
    bool result = true;
    result &= m_ObjectID != ID_InvalidObject;
    result &= m_Mesh != nullptr;
    result &= m_DiffuseTexture != nullptr;
    result &= m_NormalTexture != nullptr;
    result &= m_ORMTexture != nullptr;
    result &= m_Scale.IsValid();
    result &= m_WorldPosition.IsValid();
    result &= m_Rotation.IsValid();

    return result;
}

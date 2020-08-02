#include <graphic/visual.h>

#include <system/imguimanager.h>

void Visual::UpdatePropertiesIMGUI()
{
    char tempBuffer[256]{};
    strcpy(tempBuffer, m_Name.c_str());

    if (ImGui::InputText("Name", tempBuffer, sizeof(tempBuffer)))
        m_Name = tempBuffer;

    StaticString<36> objectIDStr = ToString(m_ObjectID).c_str();
    ImGui::InputText("ObjectID", objectIDStr.data(), objectIDStr.size(), ImGuiInputTextFlags_ReadOnly);
    ImGui::InputFloat3("World Position", (float*)&m_WorldPosition);
    if (ImGui::InputFloat4("World Rotation (Quaternion)", (float*)&m_Rotation))
    {
        m_Rotation.Normalize();
    }
    ImGui::InputFloat3("Scale", (float*)&m_Scale);
}

bool Visual::IsValid()
{
    bool result = true;
    result &= m_ObjectID != ID_InvalidObject;
    result &= m_Mesh != nullptr;
    result &= m_AlbedoTexture != nullptr;
    result &= m_NormalTexture != nullptr;
    result &= m_ORMTexture != nullptr;
    result &= m_Scale.IsValid();
    result &= m_WorldPosition.IsValid();
    result &= m_Rotation.IsValid();

    return result;
}

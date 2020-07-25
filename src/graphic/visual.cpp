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
    ImGui::InputFloat4("World Rotation (Quaternion)", (float*)&m_Rotation);


}

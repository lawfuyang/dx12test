#include <system/commandmanager.h>

void CommandManager::AddCommand(const std::function<void()>& newCmd)
{
    bbeAutoLock(m_CommandsLock);
    m_PendingCommands.push_back(newCmd);
}

void CommandManager::ConsumeCommandsMT(tf::Subflow& subFlow)
{
    subFlow.emplace([&](tf::Subflow& sf)
        {
            {
                bbeAutoLock(m_CommandsLock);
                m_ExecutingCommands.clear();
                m_ExecutingCommands.swap(m_PendingCommands);
            }

            for (const std::function<void()>& cmd : m_ExecutingCommands)
            {
                ADD_TF_TASK(sf, cmd());
            }
        });
}

void CommandManager::ConsumeCommandsST()
{
    bbeProfileFunction();

    {
        bbeAutoLock(m_CommandsLock);
        m_ExecutingCommands.clear();
        m_ExecutingCommands.swap(m_PendingCommands);
    }

    for (const std::function<void()>& cmd : m_ExecutingCommands)
    {
        cmd();
    }
}

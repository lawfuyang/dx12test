#include <system/commandmanager.h>

static const uint32_t MaxCommands = 256;

void CommandManager::Initialize()
{
    m_PendingCommands.set_capacity(MaxCommands);
    m_ExecutingCommands.set_capacity(MaxCommands);
}

void CommandManager::AddCommand(const std::function<void()>& newCmd)
{
    assert(m_PendingCommands.size() < MaxCommands);

    bbeAutoLock(m_CommandsLock);
    m_PendingCommands.push_back(newCmd);
}

void CommandManager::ConsumeAllCommandsMT(tf::Subflow& subFlow)
{
    subFlow.emplace([&](tf::Subflow& sf)
        {
            {
                bbeAutoLock(m_CommandsLock);
                m_ExecutingCommands.swap(m_PendingCommands);
            }

            tf::Task clearTask = ADD_TF_TASK(sf, m_ExecutingCommands.clear());
            for (const std::function<void()>& cmd : m_ExecutingCommands)
            {
                clearTask.succeed(ADD_TF_TASK(sf, cmd()));
            }
        });
}

void CommandManager::ConsumeAllCommandsST()
{
    bbeProfileFunction();

    {
        bbeAutoLock(m_CommandsLock);
        m_ExecutingCommands.swap(m_PendingCommands);
    }

    for (const std::function<void()>& cmd : m_ExecutingCommands)
    {
        cmd();
    }
    m_ExecutingCommands.clear();
}

bool CommandManager::ConsumeOneCommand()
{
    bbeProfileFunction();

    bbeAutoLock(m_CommandsLock);

    if (m_PendingCommands.empty())
        return false;

    m_PendingCommands.front()();
    m_PendingCommands.pop_front();

    return true;
}

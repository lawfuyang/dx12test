#pragma once

class CommandManager
{
public:
    void AddCommand(const std::function<void()>& newCmd);
    void ConsumeCommandsMT(tf::Subflow& sf);
    void ConsumeCommandsST();

private:
    std::mutex m_CommandsLock;
    std::vector<std::function<void()>> m_PendingCommands;
    std::vector<std::function<void()>> m_ExecutingCommands;
};

#pragma once

class CommandManager
{
public:
    void Initialize();

    void AddCommand(const std::function<void()>& newCmd);
    void ConsumeAllCommandsMT(tf::Subflow& sf);
    void ConsumeAllCommandsST();
    void ConsumeOneCommand();

private:
    std::mutex m_CommandsLock;
    CircularBuffer<std::function<void()>> m_PendingCommands;
    CircularBuffer<std::function<void()>> m_ExecutingCommands;
};

#pragma once

class GfxRenderer
{
    DeclareSingletonFunctions(GfxRenderer);

public:
    void Initialize();
    void ShutDown();

private:
};

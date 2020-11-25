
void AddValidPermutations(Shader& newShader, GfxShaderType shaderType, json shaderPermsForTypeJSON)
{
    // get all permutation macro define strings
    std::vector<std::string> allDefines;
    for (const json permutationJSON : shaderPermsForTypeJSON.at("Permutations"))
    {
        allDefines.push_back(permutationJSON.get<std::string>());
    }

    struct RuleSet
    {
        PermutationRule m_Rule;
        uint32_t m_AffectedBits = 0;
    };
    std::vector<RuleSet> ruleSets;

    // get all permutation rules
    for (const json ruleJSON : shaderPermsForTypeJSON["Rules"])
    {
        PermutationRule rule = StringToPermutationRule(ruleJSON.at("Rule"));

        uint32_t affectedBits = 0;
        for (const json affectedBitsJSON : ruleJSON.at("AffectedBits"))
            affectedBits |= (1 << affectedBitsJSON.get<uint32_t>());

        ruleSets.push_back({ rule, affectedBits });
    }

    // get full mask for all permutations. (Filled with '1's)
    uint32_t allPermutationsMask = 0;
    for (uint32_t i = 0; i < allDefines.size(); ++i)
        allPermutationsMask = (allPermutationsMask << 1) | 1;

    // iterate from '1', to full mask of all permutations
    // '1' because we already have the base permutation
    for (uint32_t i = 1; i <= allPermutationsMask; ++i)
    {
        // run through every rule
        bool allRulesPassed = true;
        for (const RuleSet& ruleSet : ruleSets)
        {
            if (!allRulesPassed)
                break;

            // only apply the rules for affected permutations
            if ((i & ruleSet.m_AffectedBits) != ruleSet.m_AffectedBits)
                continue;

            switch (ruleSet.m_Rule)
            {
            case AnyBitSet:
                allRulesPassed &= ((i & allPermutationsMask) != 0);
                break;

            case AllBitsSet:
                allRulesPassed &= (i == allPermutationsMask);
                break;

            case OnlyOneBitSet:
                allRulesPassed &= (std::bitset<NbShaderBits>{ i }.count() == 1);
                break;

            case MaxOneBitSet:
                allRulesPassed &= (std::bitset<NbShaderBits>{ i }.count() <= 1);
                break;
            }
        }

        // if all rules passed, this specific permutation bit mask is valid. Add it to be compiled
        if (allRulesPassed)
        {
            // Run through array of permutation names and append to get full shader name
            std::string name = newShader.m_Name;
            std::vector<std::string> defines;
            RunOnAllBits(i, [&](uint32_t bit)
                {
                    defines.push_back(allDefines[bit]);
                    name += allDefines[bit];
                });

            const std::string shaderObjCodeFileDir = StringFormat("%s%s_%s.h", g_GlobalDirs.m_ShadersTmpDir.c_str(), EnumToString(shaderType), name.c_str());
            newShader.m_Permutations[shaderType].push_back({ i, name, shaderObjCodeFileDir, defines });

        }
    }
}

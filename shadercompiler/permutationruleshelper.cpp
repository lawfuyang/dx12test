
void AddValidPermutations(PermutationsProcessingContext& context)
{
    // get full mask for all permutations. (Filled with '1's)
    uint32_t allPermutationsMask = 0;
    for (uint32_t i = 0; i < context.m_AllPermutationsDefines.size(); ++i)
        allPermutationsMask = (allPermutationsMask << 1) | 1;

    // iterate from '0', to full mask of all permutations
    for (uint32_t i = 1; i <= allPermutationsMask; ++i)
    {
        // run through every rule
        bool allRulesPassed = true;
        for (const PermutationsProcessingContext::RuleProperty& rule : context.m_RuleProperties)
        {
            if (!allRulesPassed)
                break;

            switch (rule.m_Rule)
            {
            case AnyBitSet: 
                allRulesPassed &= ((i & allPermutationsMask) != 0);
                break;

            case AllBitsSet:
                allRulesPassed &= (i == allPermutationsMask);
                break;

            case OnlyOneBitSet:
                allRulesPassed &= (std::bitset<32>{ i }.count() == 1);
                break;

            case MaxOneBitSet:
                allRulesPassed &= (std::bitset<32>{ i }.count() <= 1);
                break;
            }
        }

        // if all rules passed, this specific permutation bit mask is valid. Add it to be compiled
        if (allRulesPassed)
        {
            // Run through array of permutation names and append to get full shader name
            std::string name = context.m_Shader.m_Name;
            std::vector<std::string> defines;
            RunOnAllBits(i, [&](uint32_t bit)
                {
                    defines.push_back(context.m_AllPermutationsDefines[bit]);
                    name += context.m_AllPermutationsDefines[bit];
                });

            context.m_Shader.m_Permutations[context.m_Type].push_back({ i, name, defines });

        }
    }
}

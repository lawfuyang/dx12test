
//BBE_OPTIMIZE_OFF;

static void StaticStringUnitTest_Construction()
{
    {
        StaticString<5> stringTest;
        assert(stringTest->empty());
        assert(stringTest->capacity() >= 5);
        assert(stringTest->length() == 0);
    }

    {
        StaticString<32> stringTest2("ABCD");
        assert(stringTest2 == "ABCD");
        assert(stringTest2->empty() == false);
        assert(stringTest2->capacity() >= 32);
        assert(stringTest2->length() == 4);
    }

    {
        StaticString<32> stringTest3("");
        assert(stringTest3 == "");
        assert(stringTest3->empty());
        assert(stringTest3->capacity() >= 32);
        assert(stringTest3->length() == 0);
    }

    {
        StaticString<32> stringTest4(nullptr);
        assert(stringTest4 == "");
        assert(stringTest4->empty());
        assert(stringTest4->capacity() >= 32);
        assert(stringTest4->length() == 0);
    }

    {
        StaticString<8> stringTest5("Ubi");
        StaticString<8> stringTest6{ stringTest5 };
        assert(stringTest6 == "Ubi");
        assert(stringTest6->capacity() >= 8);
        assert(stringTest6->length() == 3);
    }
}

static void StaticStringUnitTest_Comparison()
{
    StaticString<64> stringTestU("Ubi");
    StaticString<64> stringTestS("Soft");

    // Append
    {
        StaticString<64> stringTestUS;
        stringTestUS += stringTestU;
        stringTestUS += stringTestS;

        assert(stringTestUS == "UbiSoft");
    }

    // Start With
    {
        assert(stringTestU->find("U") == 0);
        assert(stringTestU->find("Ubi") == 0);
        assert(stringTestU->find("Ubis") != 0);
        assert(stringTestU->find("u") != 0);
        assert(stringTestU->find("uBI") != 0);
        assert(stringTestU->find("UBIS") != 0);
    }

    // Clear
    {
        StaticString<64> stringTestUCopy{ stringTestU };
        stringTestUCopy->clear();

        assert(stringTestUCopy->empty());
        assert(stringTestUCopy->capacity() >= 64);
        assert(stringTestUCopy->length() == 0);
    }

    // Lower Upper case
    {
        StaticString<64> stringTestUCopy{ stringTestU };
        StaticString<64> stringTestSCopy{ stringTestS };

        std::transform(stringTestUCopy->begin(), stringTestUCopy->end(), stringTestUCopy->begin(), [](char c) { return std::toupper(c); });
        assert(stringTestUCopy == "UBI");

        std::transform(stringTestUCopy->begin(), stringTestUCopy->end(), stringTestUCopy->begin(), [](char c) { return std::tolower(c); });
        assert(stringTestUCopy == "ubi");

        std::transform(stringTestSCopy->begin(), stringTestSCopy->end(), stringTestSCopy->begin(), [](char c) { return std::tolower(c); });
        assert(stringTestSCopy == "soft");
    }

    // Operator overloads
    {
        StaticString<64> stringTestUCopy{ stringTestU };
        StaticString<64> stringTestSCopy{ stringTestS };

        assert((stringTestUCopy < stringTestSCopy) == false);

        assert(stringTestUCopy != stringTestSCopy);
        assert(stringTestUCopy == stringTestUCopy);

        assert(stringTestUCopy != "");
        assert(stringTestUCopy == stringTestUCopy);
        assert(stringTestUCopy != stringTestSCopy);
        assert(stringTestUCopy != nullptr);

        assert(stringTestUCopy[0] == 'U');
        stringTestUCopy[0] = 'V';
        assert(stringTestUCopy == "Vbi");

        const StaticString<64> stringTest(stringTestUCopy);
        assert(stringTest[0] == 'V');
    }

    // Format
    {
        StaticString<64> stringTest("Ubi");
        stringTest = StringFormat("Soft:%i", 2018).c_str();

        assert(stringTest == "Soft:2018");
        assert(stringTest->length() == 9);
        assert(stringTest->capacity() >= 64);

        StaticWString<64> stringTest2(L"Ubi");
        stringTest2 = MakeWStrFromStr(StringFormat("Soft:%i", 2018)).c_str();

        assert(stringTest2 == L"Soft:2018");
        assert(stringTest2->length() == 9);
        assert(stringTest2->capacity() >= 64);
    }

    // Format Append
    {
        StaticString<64> stringTest("Ubi");
        stringTest += StringFormat("Soft:%i", 2018).c_str();

        assert(stringTest == "UbiSoft:2018");
        assert(stringTest->length() == 12);
        assert(stringTest->capacity() >= 64);

        StaticWString<64> stringTest2(L"Ubi");
        stringTest2 += MakeWStrFromStr(StringFormat("Soft:%i", 2018)).c_str();

        assert(stringTest2 == L"UbiSoft:2018");
        assert(stringTest2->length() == 12);
        assert(stringTest2->capacity() >= 64);
    }
}

void StaticStringUnitTests_RunAllTests()
{
    StaticStringUnitTest_Construction();
    StaticStringUnitTest_Comparison();
}

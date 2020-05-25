#pragma once

class Serializer
{
public:
    DeclareSingletonFunctions(Serializer);

    enum ArchiveType { Binary, JSON };

    template <typename T>
    void Read(ArchiveType archiveType, const char* fileName, T&& var)
    {
        if (archiveType == Binary)
            ReadInternal<cereal::BinaryInputArchive>(fileName, std::forward<T>(var));
        else
            ReadInternal<cereal::JSONInputArchive>(fileName, std::forward<T>(var));
    }

    template <typename T>
    void Write(ArchiveType archiveType, const char* fileName, T&& var)
    {
        if (archiveType == Binary)
            WriteInternal<cereal::BinaryOutputArchive>(fileName, std::forward<T>(var));
        else
            WriteInternal<cereal::JSONOutputArchive>(fileName, std::forward<T>(var));
    }

    template <typename CerealArchiveType>
    static bool IsReading() { return std::is_base_of_v<cereal::detail::InputArchiveBase, CerealArchiveType>; }

    template <typename CerealArchiveType>
    static bool IsWriting() { return std::is_base_of_v<cereal::detail::OutputArchiveBase, CerealArchiveType>; }

    template <typename CerealArchiveType>
    static bool IsBinaryArchive() { return std::is_same_v<CerealArchiveType, cereal::BinaryInputArchive> || std::is_same_v<CerealArchiveType, cereal::BinaryOutputArchive>; }

    template <typename CerealArchiveType>
    static bool IsJSONArchive() { return std::is_same_v<CerealArchiveType, cereal::JSONInputArchive> || std::is_same_v<CerealArchiveType, cereal::JSONOutputArchive>; }

private:
    template <typename CerealArchiveType>
    const char* GetFileExtention()
    {
        return IsBinaryArchive<CerealArchiveType>() ? ".bin" : ".json";
    }

    template <typename CerealArchiveType, typename T>
    void ReadInternal(const char* fileName, T&& var)
    {
        if (std::ifstream stream{ (m_AssetsDir + fileName + GetFileExtention<CerealArchiveType>()).c_str() })
        {
            CerealArchiveType{ stream }(std::forward<T>(var));
        }
    }

    template <typename CerealArchiveType, typename T>
    void WriteInternal(const char* fileName, T&& var)
    {
        std::ofstream stream{ (m_AssetsDir + fileName + GetFileExtention<CerealArchiveType>()).c_str() };
        CerealArchiveType{ stream }(std::forward<T>(var));
    }

    const std::string& m_AssetsDir = GetAssetsDirectory();
};
#define g_Serializer Serializer::GetInstance()

// helper Serialization functions for math types
namespace cereal
{
    // 2D vector types
    template <typename Archive, typename VectorType, traits::EnableIf<
                                                                         std::is_same_v<VectorType, bbeVector2>  ||
                                                                         std::is_same_v<VectorType, bbeVector2I> ||
                                                                         std::is_same_v<VectorType, bbeVector2U>
                                                                     > = traits::sfinae>
    void Serialize(Archive& ar, VectorType& vec)
    {
        ar(vec.x);
        ar(vec.y);
    }

    // 3D vector types
    template <typename Archive, typename VectorType, traits::EnableIf<
                                                                         std::is_same_v<VectorType, bbeVector3>  ||
                                                                         std::is_same_v<VectorType, bbeVector3I> ||
                                                                         std::is_same_v<VectorType, bbeVector3U>
                                                                     > = traits::sfinae>
    void Serialize(Archive& ar, VectorType& vec)
    {
        ar(vec.x);
        ar(vec.y);
        ar(vec.z);
    }

    // 4D vector types
    template <typename Archive, typename VectorType, traits::EnableIf<
                                                                         std::is_same_v<VectorType, bbeVector4>    ||
                                                                         std::is_same_v<VectorType, bbeVector4I>   ||
                                                                         std::is_same_v<VectorType, bbeVector4U>   ||
                                                                         std::is_same_v<VectorType, bbePlane>      ||
                                                                         std::is_same_v<VectorType, bbeQuaternion> ||
                                                                         std::is_same_v<VectorType, bbeColor>
                                                                     > = traits::sfinae>
    void Serialize(Archive& ar, VectorType& vec)
    {
        ar(vec.x);
        ar(vec.y);
        ar(vec.z);
        ar(vec.w);
    }

    // Matrix type
    template <typename Archive>
    void Serialize(Archive& ar, bbeMatrix& mat)
    {
        ar(mat.m);
    }
}

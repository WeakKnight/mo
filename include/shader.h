#pragma once
#include "common.h"

struct UniformInfo
{
    std::string name;
    int size;
    int location;
};

class Shader
{
public:
    void Init(std::string name, std::string vertCode, std::string fragmentCode, std::string geomCode = "");
    void Use();

    void SetInt(std::string location, int value);
    void SetBool(std::string location, bool value);
    void SetFloat(std::string location, float value);
    void SetVec2(std::string location, glm::vec2 value);
    void SetVec3(std::string location, glm::vec3 value);
    void SetVec4(std::string location, glm::vec4 value);
    void SetVec2Array(std::string location, const std::vector<glm::vec2> value);
    void SetVec3Array(std::string location, const std::vector<glm::vec3> value);
    void SetVec4Array(std::string location, const std::vector<glm::vec4> value);
    void SetMat2(std::string location, glm::mat2 value);
    void SetMat3(std::string location, glm::mat3 value);
    void SetMat4(std::string location, glm::mat4 value);
    void SetUniformBlock(const std::string& location, int index);

    int GetUniform(std::string name);

    std::string GetName()
    {
        return name;
    }

    void SetName(const std::string newName)
    {
        name = newName;
    }

private:
    unsigned int ID;
    std::string name = "";
    std::map<std::string, UniformInfo> uniforms;
    std::map<std::string, int> uniformBlockIndexMap;
};
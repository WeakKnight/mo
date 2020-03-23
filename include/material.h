#pragma once
#include "common.h"

class Shader;
class Texture;

enum class MATERIAL_EXTENSION
{
	MODEL_VIEW_PROJECTION,
	TIME,
	AMBIENT,
	NEAREST_LIGHT,
	CAMERA
};

enum class MATERIAL_PASS
{
	FORWARD,
	DEFERRED,
	NONE
};

class Material
{
public:
	Material(Shader* shader);

	void SetTextureProperty(const std::string& propName, Texture* texValue);
	void SetMatrix4(const std::string& propName, glm::mat4& matrix);
	void SetFloat(const std::string& propName, float value);
	void SetInt(const std::string& propName, int value);
	void SetVector3(const std::string& propName, glm::vec3& value);
	void SetUniformBlock(const std::string& location, int index);

	Texture* GetDisplacementTexture()
	{
		return textureProperties["displacementMap"];
	}

	Texture* GetNormalTexture()
	{
		return textureProperties["normalMap"];
	}

	float GetTillingX()
	{
		return floatProperties["tillingX"];
	}

	float GetTillingY()
	{
		return floatProperties["tillingY"];
	}

	void Use();

	void AddExtension(MATERIAL_EXTENSION extension)
	{
		extensions.push_back(extension);
	}

	std::vector<MATERIAL_EXTENSION> GetExtensions()
	{
		return extensions;
	}

	MATERIAL_PASS GetPass() const
	{
		return pass;
	}

	void SetPass(MATERIAL_PASS pass)
	{
		this->pass = pass;
	}

	bool HasTess() const;

public:
	std::string GetName()
	{
		return name;
	}

	void SetName(const std::string newName)
	{
		name = newName;
	}

private:

	MATERIAL_PASS pass = MATERIAL_PASS::NONE;

	std::string name = "";

	std::vector<MATERIAL_EXTENSION> extensions;
	std::map<std::string, Texture*> textureProperties;
	
	std::map<std::string, float> floatProperties;
	std::map<std::string, glm::vec3> vec3Properties;
	std::map<std::string, int> intProperties;
	std::map<std::string, glm::mat4> mat4Properties;
	std::map<std::string, int> uniformProperties;

	Shader* shader;
};


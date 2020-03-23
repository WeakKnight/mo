#include "material.h"
#include "shader.h"
#include "texture.h"

Material::Material(Shader* shader)
{
	this->shader = shader;
}

void Material::SetTextureProperty(const std::string& propName, Texture* texValue)
{
	textureProperties[propName] = texValue;
}

void Material::SetMatrix4(const std::string& propName, glm::mat4& matrix)
{
	mat4Properties[propName] = matrix;
}

void Material::SetVector3(const std::string& propName, glm::vec3& value)
{
	vec3Properties[propName] = value;
;	// shader->SetVec3(propName, value);
}

void Material::SetFloat(const std::string& propName, float value)
{
	floatProperties[propName] = value;
}

void Material::SetInt(const std::string& propName, int value)
{
	intProperties[propName] = value;
}

void Material::SetUniformBlock(const std::string& location, int index)
{
	uniformProperties[location] = index;
}

void Material::Use()
{
	shader->Use();

	int unitIndex = 0;
	for (auto& item : textureProperties)
	{
		shader->SetInt(item.first, unitIndex);
		unitIndex += 1;
	}

	for (auto& item : vec3Properties)
	{
		shader->SetVec3(item.first, item.second);
	}

	for (auto& item : floatProperties)
	{
		shader->SetFloat(item.first, item.second);
	}

	for (auto& item : intProperties)
	{
		shader->SetInt(item.first, item.second);
	}

	for (auto& item : mat4Properties)
	{
		shader->SetMat4(item.first, item.second);
	}

	for (auto& item : uniformProperties)
	{
		shader->SetUniformBlock(item.first, item.second);
	}

	/*
	Bind Textures
	*/
	unitIndex = 0;
	for (auto& item : textureProperties)
	{
		item.second->Bind(unitIndex);
		unitIndex += 1;
	}
}

bool Material::HasTess() const
{
	return shader->GetHasTess();
}
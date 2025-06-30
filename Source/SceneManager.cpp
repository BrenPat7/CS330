///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
/*** There is no limit to the number of object materials that can ***/
/*** be defined. Refer to the code in the OpenGL Sample for help ***/
	OBJECT_MATERIAL plasticMaterial;
	plasticMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	plasticMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	plasticMaterial.shininess = 21.0;
	plasticMaterial.tag = "plastic";
	m_objectMaterials.push_back(plasticMaterial);
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.6f, 0.5f, 0.2f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.2f, 0.2f);
	woodMaterial.shininess = 1.0;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	metalMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.8f);
	metalMaterial.shininess = 8.0;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	glassMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.8f);
	glassMaterial.shininess = 10.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);
	OBJECT_MATERIAL tileMaterial;
	tileMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	tileMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);
	tileMaterial.shininess = 6.0;
	tileMaterial.tag = "tile";
	m_objectMaterials.push_back(tileMaterial);
	OBJECT_MATERIAL stoneMaterial;
	stoneMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	stoneMaterial.specularColor = glm::vec3(0.73f, 0.3f, 0.3f);
	stoneMaterial.shininess = 6.0;
	stoneMaterial.tag = "stone";
	m_objectMaterials.push_back(stoneMaterial);
}
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);
	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to five point light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help ***/
	//Directional Light
	m_pShaderManager->setVec3Value("directionalLight.direction", 0.2f, 5.2f, 0.5f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.15f, 0.15f, 0.15f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 1.0f, 0.9f, 0.40f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);
	// point light 1
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 12.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.35f, 0.35f, 0.35f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.25f, 0.25f, 0.25f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
	m_pShaderManager->setVec3Value("pointLights[1].position", 0.0f, 8.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.0f, 0.0f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.6f, 0.6f, 0.65f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.10f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.05f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	SetupSceneLights();

	DefineObjectMaterials();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	
	//add the textures to the scene
	CreateGLTexture("textures/greyplastic.jpg", "plasticd_texture");
	CreateGLTexture("textures/greenplastic.jpg", "plasticc_texture");
	CreateGLTexture("textures/blueplastic.jpg", "plasticb_texture");
	CreateGLTexture("textures/Redplastic.jpg", "plastic_texture");
	CreateGLTexture("textures/sand.png", "sand_texture");
	CreateGLTexture("textures/brick.jpg", "brick_texture");
	CreateGLTexture("textures/whitecloth.jpg", "cloth_texture");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();       // for the cube.
	m_basicMeshes->LoadSphereMesh();    // for the sphere.
	m_basicMeshes->LoadCylinderMesh();  // for the cylinder.
	m_basicMeshes->LoadPyramid4Mesh();   // for the pyramid.
	m_basicMeshes->LoadConeMesh();		// for the cone.
	m_basicMeshes->LoadTorusMesh();    // for the torus.
}
/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{



	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	SetShaderMaterial("default");           
	SetShaderTexture("plane_texture");       
	SetTextureUVScale(1.0f, 1.0f);           
	
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderMaterial("stone");
	SetShaderTexture("sand_texture");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	SetShaderMaterial("default");            
	SetShaderTexture("plane_texture");      
	SetTextureUVScale(1.0f, 1.0f);           

	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 9.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// set the color values into the shader
	SetShaderColor(0.54f, 0.81f, 0.94f, 1.0f);// Light blue color for the plane.
	SetShaderMaterial("stone");
	SetShaderTexture("brick_texture");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	

	//For the torus.
	scaleXYZ = glm::vec3(0.5f, 0.5f, 0.25f);// Size of the Torus.
	YrotationDegrees = 0.0f;//
	XrotationDegrees = 90.0f;// Rotation of the Torus.
	ZrotationDegrees = 0.0f;//
	positionXYZ = glm::vec3(0.0f, 6.0f, 0.0f);// Position of the Torus.

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);// Set the transformations for the Torus.
	SetShaderTexture("plasticd_texture"); // Set the texture for the Torus.
	SetShaderMaterial("plastic");
	SetTextureUVScale(1.0f, 1.0f); // Set the texture UV scale for the Torus.
	m_basicMeshes->DrawTorusMesh();// Draw the Torus.
	//Mobile Arm Horizontal
	scaleXYZ = glm::vec3(0.10f, -2.05f, 0.10f); // Size of the string. Making it thin and tall.

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(0.0f, 6.25f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f); // Light gray color for the string.
	SetShaderTexture("plasticd_texture"); // Set the texture for the Torus.
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawCylinderMesh();// Draw the string.
	//Mobile Arm Vertical 1
	scaleXYZ = glm::vec3(0.10f, -0.35f, 0.10f); // Size of the string. Making it thin and tall.

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 6.25f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f); // Light gray color for the string.
	SetShaderTexture("plasticd_texture"); // Set the texture for the Torus.
	SetShaderMaterial("plastic");
	m_basicMeshes->DrawCylinderMesh();// Draw the string.
	
	//Mobile Arm Vertical 2
	scaleXYZ = glm::vec3(0.10f, -3.35f, 0.10f); // Size of the string. Making it thin and tall.

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(2.05f, 6.25f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f); // Light gray color for the string.
	SetShaderTexture("plasticd_texture"); // Set the texture for the Torus.
	SetShaderMaterial("plastic"); //
	m_basicMeshes->DrawCylinderMesh();// Draw the string.
	
	//Sphere Joint
	scaleXYZ = glm::vec3(0.10f, 0.10f, 0.10f);// Size of the sphere
	positionXYZ = glm::vec3(0.00f, 6.25f, 0.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);// Set the transformations for the sphere.
	SetShaderTexture("plasticd_texture"); // Set the texture for the sphere.
	SetShaderMaterial("plastic");
	SetTextureUVScale(1.0f, 1.0f); // Set the texture UV scale for the sphere.
	m_basicMeshes->DrawSphereMesh(); // Draw the sphere.
	
	//Sphere Joint 2
	scaleXYZ = glm::vec3(0.10f, 0.10f, 0.10f);// Size of the sphere
	positionXYZ = glm::vec3(2.05f, 6.25f, 0.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);// Set the transformations for the sphere.
	SetShaderTexture("plasticd_texture"); // Set the texture for the sphere.
	SetShaderMaterial("plastic");
	SetTextureUVScale(1.0f, 1.0f); // Set the texture UV scale for the sphere.
	m_basicMeshes->DrawSphereMesh(); // Draw the sphere.

	//String 1
	scaleXYZ = glm::vec3(0.02f, 0.65f, 0.02f); // Size of the string. Making it thin and tall.

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.525f, 5.30f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f); // Light gray color for the string.
	m_basicMeshes->DrawCylinderMesh();// Draw the string.
	//String 2
	scaleXYZ = glm::vec3(0.02f, 0.65f, 0.02f);  //Makes it thin and tall like a string.

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-0.525f, 5.30f, 0.0f);// Position of the string in the scene.
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();// Draw the string.
	//string 3
	scaleXYZ = glm::vec3(0.02f, 0.65f, 0.02f); //Makes it thin and tall like a string.

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 5.30f, 0.50f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();
	//String 4
	scaleXYZ = glm::vec3(0.02f, 0.65f, 0.02f); //Makes it thin and tall like a string.

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 5.30f, -0.50f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);
	m_basicMeshes->DrawCylinderMesh();


	//Toy Pyramid
	scaleXYZ = glm::vec3(0.31f, 0.31f, 0.31f);//Size of the pyramid.
	YrotationDegrees = 0.0f;
	XrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.525f, 5.25f, 0.0f);// Position of the pyramid in the scene.
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);// Set the transformations for the pyramid.
	SetShaderTexture("plasticb_texture"); // Set the texture for the pyramid.
	SetShaderMaterial("plastic");
	SetTextureUVScale(0.10f, 0.10f); // Set the texture UV scale for the pyramid.
	m_basicMeshes->DrawPyramid4Mesh(); // Draw the pyramid.

	//Toy Sphere
	scaleXYZ = glm::vec3(0.23f, 0.23f, 0.23f);// Size of the sphere
	positionXYZ = glm::vec3(0.0f, 5.25f, -0.50f);// Position of the sphere in the scene.
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);// Set the transformations for the sphere.
	SetShaderTexture("plasticc_texture"); // Set the texture for the sphere.
	SetShaderMaterial("plastic");
	SetTextureUVScale(0.20f, 0.20f); // Set the texture UV scale for the sphere.
	m_basicMeshes->DrawSphereMesh(); // Draw the sphere.

	//Toy Cube
	scaleXYZ = glm::vec3(0.28f, 0.28f, 0.28f);// Size of the box.
	YrotationDegrees = 0.0f;//
	XrotationDegrees = 0.0f;//
	ZrotationDegrees = 0.0f;//
	positionXYZ = glm::vec3(0.0f, 5.35f, 0.50f);// Position of the box in the scene.

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);// Set the transformations for the cube..
	SetShaderTexture("plastic_texture"); // Set the texture for the box.
	SetShaderMaterial("plastic");
	SetTextureUVScale(0.10f, 0.10f); // Set the texture UV scale for the box.
	m_basicMeshes->DrawBoxMesh();// Draw the box..


	//Star
	scaleXYZ = glm::vec3(0.18f, 0.25f, 0.08f);//Size of the one side.
	YrotationDegrees = 0.0f;
	XrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-0.525f, 5.35f, 0.0f);// Position of the pyramid in the scene.
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);// Set the transformations for the pyramid.
	SetShaderTexture("plasticb_texture"); // Set the texture for the pyramid.
	SetShaderMaterial("plastic");
	SetTextureUVScale(0.10f, 0.10f); // Set the texture UV scale for the pyramid.
	m_basicMeshes->DrawPyramid4Mesh(); // Draw the pyramid.
	// Side 2
	scaleXYZ = glm::vec3(0.18f, 0.25f, 0.08f);//Size of the another side.
	YrotationDegrees = 0.0f;
	XrotationDegrees = 65.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-0.525f, 5.26f, 0.10f);// Position of the pyramid in the scene.
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);// Set the transformations for the pyramid.
	SetShaderTexture("plasticb_texture"); // Set the texture for the pyramid.
	SetShaderMaterial("plastic");
	SetTextureUVScale(0.10f, 0.10f); // Set the texture UV scale for the pyramid.
	m_basicMeshes->DrawPyramid4Mesh(); // Draw the pyramid.
	// Side 3
	scaleXYZ = glm::vec3(0.18f, 0.25f, 0.08f);//Size of the a third side.
	YrotationDegrees = 0.0f;
	XrotationDegrees = -65.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-0.525f, 5.26f, -0.10f);// Position of the pyramid in the scene.
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);// Set the transformations for the pyramid.
	SetShaderTexture("plasticb_texture"); // Set the texture for the pyramid.
	SetShaderMaterial("plastic");
	SetTextureUVScale(0.10f, 0.10f); // Set the texture UV scale for the pyramid.
	m_basicMeshes->DrawPyramid4Mesh(); // Draw the pyramid.
	// Side 4
	scaleXYZ = glm::vec3(0.18f, 0.25f, 0.08f);//Size of the fourth side.
	YrotationDegrees = 0.0f;
	XrotationDegrees = 145.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-0.525f, 5.15f, 0.05f);// Position of the pyramid in the scene.
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);// Set the transformations for the pyramid.
	SetShaderTexture("plasticb_texture"); // Set the texture for the pyramid.
	SetShaderMaterial("plastic");
	SetTextureUVScale(0.10f, 0.10f); // Set the texture UV scale for the pyramid.
	m_basicMeshes->DrawPyramid4Mesh(); // Draw the pyramid.
	// Side 5
	scaleXYZ = glm::vec3(0.18f, 0.25f, 0.08f);//Size of the final side..
	YrotationDegrees = 0.0f;
	XrotationDegrees = -145.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-0.525f, 5.15f, -0.05f);// Position of the pyramid in the scene.
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);// Set the transformations for the pyramid.
	SetShaderTexture("plasticb_texture"); // Set the texture for the pyramid.
	SetShaderMaterial("plastic");
	SetTextureUVScale(0.10f, 0.10f); // Set the texture UV scale for the pyramid.
	m_basicMeshes->DrawPyramid4Mesh(); // Draw the pyramid.

	//Feet of Bassinet
	scaleXYZ = glm::vec3(0.1f, 1.72f, 0.1f);// Size of the cylinder.
	YrotationDegrees = 0.0f;//
	XrotationDegrees = 0.0f;//
	ZrotationDegrees = 0.0f;//
	positionXYZ = glm::vec3(-1.5f, 0.55f, 1.1f);// Position of the cylinder in the scene.

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);// Set the transformations for the cylinder..
	SetShaderTexture("plasticd_texture"); // Set the texture for the cylinder.
	SetShaderMaterial("plastic");
	SetTextureUVScale(1.0f, 1.0f); // Set the texture UV scale for the cylinder.
	m_basicMeshes->DrawCylinderMesh();// Draw the cylinder..

	//leg 2

	scaleXYZ = glm::vec3(0.1f, 1.72f, 0.1f);// Size of the cylinder.
	YrotationDegrees = 0.0f;//
	XrotationDegrees = 0.0f;//
	ZrotationDegrees = 0.0f;//
	positionXYZ = glm::vec3(1.5f, 0.55f, 1.1f);// Position of the cylinder in the scene.

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);// Set the transformations for the cylinder..
	SetShaderTexture("plasticd_texture"); // Set the texture for the cylinder.
	SetShaderMaterial("plastic");
	SetTextureUVScale(1.0f, 1.0f); // Set the texture UV scale for the cylinder.
	m_basicMeshes->DrawCylinderMesh();// Draw the cylinder..
	//leg 4
	
	scaleXYZ = glm::vec3(0.1f, 1.72f, 0.1f);// Size of the cylinder.
	YrotationDegrees = 0.0f;//
	XrotationDegrees = 0.0f;//
	ZrotationDegrees = 0.0f;//
	positionXYZ = glm::vec3(-1.5f, 0.55f, -1.1f);// Position of the cylinder in the scene.

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);// Set the transformations for the cylinder..
	SetShaderTexture("plasticd_texture"); // Set the texture for the cylinder.
	SetShaderMaterial("plastic");
	SetTextureUVScale(1.0f, 1.0f); // Set the texture UV scale for the cylinder.
	m_basicMeshes->DrawCylinderMesh();// Draw the box..



	//Bassinet Border
	int numSpheres = 10;
	float arcRadius = 1.0f;
	float sphereRadius = 0.15f;
	float railHeight = 2.95f;
	float boxLengthX = 2.0f;
	float boxLengthZ = 1.0f;
	float thetaStep = glm::half_pi<float>() / numSpheres;

	float railHeightFloor = 1.95f;

	// Reusable settings
	glm::vec3 scale(sphereRadius);
	SetShaderMaterial("plastic");
	SetShaderTexture("plasticd_texture");
	SetTextureUVScale(0.3f, 0.2f);

	// === CORNERS ===
	// Top-Left Corner: θ from 180° to 270° (π to 3π/2)
	for (int i = 0; i <= numSpheres; ++i) {
		float theta = glm::pi<float>() + i * thetaStep;
		float x = arcRadius * cos(theta);
		float z = arcRadius * sin(theta);
		glm::vec3 pos = glm::vec3(-boxLengthX / 2, railHeight, -boxLengthZ / 2) + glm::vec3(x, 0.0f, z); //equation for top-left corner position
		SetTransformations(scale, 0.0f, 0.0f, 0.0f, pos);
		m_basicMeshes->DrawSphereMesh();
	}

	// Top-Right Corner: θ from 270° to 360° (3π/2 to 2π)
	for (int i = 0; i <= numSpheres; ++i) {
		float theta = glm::pi<float>() * 1.5f + i * thetaStep;
		float x = arcRadius * cos(theta);
		float z = arcRadius * sin(theta);
		glm::vec3 pos = glm::vec3(boxLengthX / 2, railHeight, -boxLengthZ / 2) + glm::vec3(x, 0.0f, z); //equation for top-right corner position
		SetTransformations(scale, 0.0f, 0.0f, 0.0f, pos);
		m_basicMeshes->DrawSphereMesh();
	}

	// Bottom-Right Corner: θ from 0° to 90° (0 to π/2)
	for (int i = 0; i <= numSpheres; ++i) {
		float theta = 0.0f + i * thetaStep;
		float x = arcRadius * cos(theta);
		float z = arcRadius * sin(theta);
		glm::vec3 pos = glm::vec3(boxLengthX / 2, railHeight, boxLengthZ / 2) + glm::vec3(x, 0.0f, z);
		SetTransformations(scale, 0.0f, 0.0f, 0.0f, pos);
		m_basicMeshes->DrawSphereMesh();
	}

	// Bottom-Left Corner: θ from 90° to 180° (π/2 to π)
	for (int i = 0; i <= numSpheres; ++i) {
		float theta = glm::half_pi<float>() + i * thetaStep;
		float x = arcRadius * cos(theta);
		float z = arcRadius * sin(theta);
		glm::vec3 pos = glm::vec3(-boxLengthX / 2, railHeight, boxLengthZ / 2) + glm::vec3(x, 0.0f, z); //equation for bottom-left corner position
		SetTransformations(scale, 0.0f, 0.0f, 0.0f, pos);
		m_basicMeshes->DrawSphereMesh();
	}

	// === WALLS ===
	glm::vec3 wallScaleX(boxLengthX, 0.1f, 0.1f);
	glm::vec3 wallScaleZ(0.1f, 0.1f, boxLengthZ);

	// Top wall
	SetTransformations(wallScaleX, 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, railHeight, -boxLengthZ / 2 - arcRadius));//equation for top wall position
	m_basicMeshes->DrawBoxMesh();

	// Bottom wall
	SetTransformations(wallScaleX, 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, railHeight, boxLengthZ / 2 + arcRadius));//equation for bottom wall position
	m_basicMeshes->DrawBoxMesh();

	// Left wall
	SetTransformations(wallScaleZ, 0.0f, 0.0f, 0.0f, glm::vec3(-boxLengthX / 2 - arcRadius, railHeight, 0.0f)); //equation for left wall position
	m_basicMeshes->DrawBoxMesh();

	// Right wall
	SetTransformations(wallScaleZ, 0.0f, 0.0f, 0.0f, glm::vec3(boxLengthX / 2 + arcRadius, railHeight, 0.0f)); //equation for right wall position
	m_basicMeshes->DrawBoxMesh();

	// === SECOND BORDER (renamed variables) ===
	int    numSpheresFl = 6;
	float  arcRadiusFl = 0.5f;
	float  sphereRadiusFl = 0.15f;
	float  boxLengthXFl = 1.65f;
	float  boxLengthZFl = 0.9f;
	float  thetaStepFl = glm::half_pi<float>() / numSpheresFl; // Step size for the angle in radians
	float  railHeightFloorFl = 2.15f;

	// Reusable settings for this border
	glm::vec3 scaleFl(sphereRadiusFl);
	SetShaderMaterial("plastic");
	SetShaderTexture("plasticd_texture");

	// === CORNERS ===
	// Top-Left: θ from π to 3π/2
	for (int i = 0; i <= numSpheresFl; ++i) {
		float theta = glm::pi<float>() + i * thetaStepFl;
		float x = arcRadiusFl * cos(theta);
		float z = arcRadiusFl * sin(theta);
		glm::vec3 pos = glm::vec3(-boxLengthXFl / 2, railHeightFloorFl, -boxLengthZFl / 2) //equation for top-left corner position
			+ glm::vec3(x, 0.0f, z);
		SetTransformations(scaleFl, 0, 0, 0, pos);
		m_basicMeshes->DrawSphereMesh();
	}

	// Top-Right: θ from 3π/2 to 2π
	for (int i = 0; i <= numSpheresFl; ++i) {
		float theta = glm::pi<float>() * 1.5f + i * thetaStepFl;
		float x = arcRadiusFl * cos(theta);
		float z = arcRadiusFl * sin(theta);
		glm::vec3 pos = glm::vec3(boxLengthXFl / 2, railHeightFloorFl, -boxLengthZFl / 2) //equation for top-right corner position
			+ glm::vec3(x, 0.0f, z);
		SetTransformations(scaleFl, 0, 0, 0, pos);
		m_basicMeshes->DrawSphereMesh();
	}

	// Bottom-Right: θ from 0 to π/2
	for (int i = 0; i <= numSpheresFl; ++i) {
		float theta = i * thetaStepFl;
		float x = arcRadiusFl * cos(theta);
		float z = arcRadiusFl * sin(theta);
		glm::vec3 pos = glm::vec3(boxLengthXFl / 2, railHeightFloorFl, boxLengthZFl / 2) //equation for bottom-right corner position
			+ glm::vec3(x, 0.0f, z);
		SetTransformations(scaleFl, 0, 0, 0, pos);
		m_basicMeshes->DrawSphereMesh();
	}

	// Bottom-Left: θ from π/2 to π
	for (int i = 0; i <= numSpheresFl; ++i) {
		float theta = glm::half_pi<float>() + i * thetaStepFl; // π/2 to π
		float x = arcRadiusFl * cos(theta); // Calculate x position using the arc radius and angle
		float z = arcRadiusFl * sin(theta); // Calculate z position using the arc radius and angle
		glm::vec3 pos = glm::vec3(-boxLengthXFl / 2, railHeightFloorFl, boxLengthZFl / 2) //equation for bottom-left corner position
			+ glm::vec3(x, 0.0f, z);
		SetTransformations(scaleFl, 0, 0, 0, pos);
		m_basicMeshes->DrawSphereMesh();
	}

	// === WALLS ===
	glm::vec3 wallScaleXFl(boxLengthXFl, 0.1f, 0.1f); // Scale for the walls
	glm::vec3 wallScaleZFl(0.1f, 0.1f, boxLengthZFl); // Scale for the walls

	// Top wall
	SetTransformations(
		wallScaleXFl,
		0, 0, 0,
		glm::vec3(0.0f, railHeightFloorFl, -boxLengthZFl / 2 - arcRadiusFl) //equation for top wall position
	);
	m_basicMeshes->DrawBoxMesh();

	// Bottom wall
	SetTransformations(
		wallScaleXFl,
		0, 0, 0,
		glm::vec3(0.0f, railHeightFloorFl, boxLengthZFl / 2 + arcRadiusFl) //equation for bottom wall position
	);
	m_basicMeshes->DrawBoxMesh();

	// Left wall
	SetTransformations(
		wallScaleZFl,
		0, 0, 0,
		glm::vec3(-boxLengthXFl / 2 - arcRadiusFl, railHeightFloorFl, 0.0f) //equation for left wall position
	);
	m_basicMeshes->DrawBoxMesh();

	// Right wall
	SetTransformations(
		wallScaleZFl,
		0, 0, 0,
		glm::vec3(boxLengthXFl / 2 + arcRadiusFl, railHeightFloorFl, 0.0f) //equation for right wall position
	);
	m_basicMeshes->DrawBoxMesh();

	//Bassinet Floor and Side walls
	float  panelThickness = 0.18f;
	float  panelRise = railHeight - railHeightFloor;           // vertical span
	float  spanX = boxLengthX + 2 * arcRadius -1;              // full length front/back
	float  spanZ = boxLengthZ + 2 * arcRadius - 1;              // full length left/right
	float  midY = (railHeight + railHeightFloor) * 0.5f;  // midpoint Y for positioning

	// precompute rotation angles (in degrees)
	// SetTransformations takes degrees for X/Y/Z rotations
	float angleX = glm::degrees(atan(panelRise / spanX));
	float angleZ = glm::degrees(atan(panelRise / spanZ));

	// one for X-spanning panels, one for Z-spanning
	glm::vec3 panelScaleX(spanX, panelRise, panelThickness);
	glm::vec3 panelScaleZ(panelThickness, panelRise, spanZ);

	// switch to a plain white material
	SetShaderMaterial("plastic");
	SetShaderTexture("cloth_texture");  

	// --- FRONT PANEL (between the two front rails) ---
	SetTransformations(
		panelScaleX,
		-angleX,  // tilt “up” toward +Y
		0.0f,
		0.0f,
		glm::vec3(
			0.0f,
			midY,
			-boxLengthZ / 2 - arcRadius + panelThickness * 0.5f //equation for front panel position
		)
	);
	m_basicMeshes->DrawBoxMesh();

	// --- BACK PANEL ---
	SetTransformations(
		panelScaleX,
		angleX,  // tilt “up” toward +Y
		0.0f,
		0.0f,
		glm::vec3(
			0.0f,
			midY,
			boxLengthZ / 2 + arcRadius - panelThickness * 0.5f // equation for back panel position
		)
	);
	m_basicMeshes->DrawBoxMesh();

	// --- LEFT PANEL ---
	SetTransformations(
		panelScaleZ,
		0.0f,
		0.0f,
		angleZ,  // tilt around Z-axis so it rises toward +Y
		glm::vec3(
			-boxLengthX / 2 - arcRadius + panelThickness * 0.5f,
			midY,
			0.0f
		)
	);
	m_basicMeshes->DrawBoxMesh();

	// --- RIGHT PANEL ---
	SetTransformations(
		panelScaleZ,
		0.0f,
		0.0f,
		-angleZ,  // opposite tilt direction
		glm::vec3(
			boxLengthX / 2 + arcRadius - panelThickness * 0.5f,
			midY,
			0.0f
		)
	);
	m_basicMeshes->DrawBoxMesh();
	//Cloth at bottom of bassinet
	scaleXYZ = glm::vec3(1.3f, 1.2f, 1.0f);// Size of the box.
	YrotationDegrees = 0.0f;//
	XrotationDegrees = 0.0f;//
	ZrotationDegrees = 0.0f;//
	positionXYZ = glm::vec3(0.0f, 2.05f, 0.0f);// Position of the box in the scene.

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);// Set the transformations for the cube..
	SetShaderTexture("cloth_texture"); // Set the texture for the box.
	SetShaderMaterial("stone");
	SetTextureUVScale(1.0f, 1.0f); // Set the texture UV scale for the box.
	m_basicMeshes->DrawPlaneMesh();// Draw the box..

	//Cloth at bottom of bassinet
	scaleXYZ = glm::vec3(0.1f, 1.7f, 0.1f);// Size of the box.
	YrotationDegrees = 90.0f;//
	XrotationDegrees = 0.0f;//
	ZrotationDegrees = 0.0f;//
	positionXYZ = glm::vec3(1.40f, 0.65f, -1.0f);// Position of the box in the scene.

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);// Set the transformations for the cube..
	SetShaderTexture("plasticd_texture"); // Set the texture for the box.
	SetShaderMaterial("plastic");
	SetTextureUVScale(1.0f, 1.0f); // Set the texture UV scale for the box.
	m_basicMeshes->DrawCylinderMesh();// Draw the box..

	scaleXYZ = glm::vec3(0.1f, 1.7f, 0.1f);// Size of the box.
	YrotationDegrees = 90.0f;//
	XrotationDegrees = 0.0f;//
	ZrotationDegrees = 0.0f;//
	positionXYZ = glm::vec3(1.40f, 0.65f, -1.0f);// Position of the box in the scene.

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);// Set the transformations for the cube..
	SetShaderTexture("plasticd_texture"); // Set the texture for the box.
	SetShaderMaterial("plastic");
	SetTextureUVScale(1.0f, 1.0f); // Set the texture UV scale for the box.
	m_basicMeshes->DrawCylinderMesh();// Draw the box..

	{
	glm::vec3 scaleXYZ = glm::vec3(16.0f, 2.3f, 6.2f);      // width, height, depth
	float      Xrotation = 0.0f;
	float      Yrotation = 0.0f;
	float      Zrotation = 0.0f;
	glm::vec3 positionXYZ = glm::vec3(5.0f, 0.3f, -8.0f);     // lift it half-height above ground

	SetTransformations(scaleXYZ, Xrotation, Yrotation, Zrotation, positionXYZ);
	SetShaderTexture("plastic_texture");
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();   // seat as a flattened box
}

// ——— Couch Backrest ———
{
		glm::vec3 scaleXYZ = glm::vec3(13.0f, 7.8f, 1.6f);      //couch backrest size
	float      Xrotation = 0.0f;
	float      Yrotation = 0.0f;
	float      Zrotation = 0.0f;
	glm::vec3 positionXYZ = glm::vec3(5.0f, 0.8f, -9.4f);    // behind the seat

	SetTransformations(scaleXYZ, Xrotation, Yrotation, Zrotation, positionXYZ);
	SetShaderTexture("plastic_texture");
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();   // backrest
}

// ——— Left Armrest ———
{
	glm::vec3 scaleXYZ = glm::vec3(3.2f, 1.5f, 3.2f);      // radius-height-radius
	float      Xrotation = 0.0f;
	float      Yrotation = 0.0f;
	float      Zrotation = 90.0f;                          // rotate cylinder to lie along X
	glm::vec3 positionXYZ = glm::vec3(-1.4f, 0.65f, -8.0f);    // at left edge

	SetTransformations(scaleXYZ, Xrotation, Yrotation, Zrotation, positionXYZ);
	SetShaderTexture("plastic_texture");
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh(); // left armrest
}

// ——— Right Armrest ———
{
	glm::vec3 scaleXYZ = glm::vec3(3.2f, 1.5f, 3.2f);
	float      Xrotation = 0.0f;
	float      Yrotation = 0.0f;
	float      Zrotation = 90.0f;
	glm::vec3 positionXYZ = glm::vec3(12.8f, 0.65f, -8.0f);    // at right edge

	SetTransformations(scaleXYZ, Xrotation, Yrotation, Zrotation, positionXYZ);
	SetShaderTexture("plastic_texture");
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawCylinderMesh(); // right armrest
}
{
	glm::vec3 scaleXYZ = glm::vec3(2.5f, 0.3f, 1.2f);
	float      Xrotation = 72.0f;
	float      Yrotation = 0.0f;
	float      Zrotation = 0.0f;
	glm::vec3 positionXYZ = glm::vec3(1.3f, 2.65f, -8.0f);    // at right edge

	SetTransformations(scaleXYZ, Xrotation, Yrotation, Zrotation, positionXYZ);
	SetShaderTexture("plasticb_texture");
	SetShaderMaterial("wood");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawSphereMesh(); // right armrest
	//couch
}
}

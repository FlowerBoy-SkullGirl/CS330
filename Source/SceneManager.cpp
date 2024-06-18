///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

//GLFW
#include "GLFW/glfw3.h"

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	// Used for the layered texture sampler
	const char* g_Texture2ValueName = "objectTexture2";
	// Used for specular mapping
	const char* g_SpecularMapName = "specularMap";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseTexture2Name = "bUseTexture2";
	const char* g_UseLightingName = "bUseLighting";
	const char* g_UseSpecularMap = "bUseSpecularMap";

	// Determine whether an error has been reported
	bool g_errorEncountered = false;
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
	m_loadedTextures = 0;
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

	if (!bFound && !g_errorEncountered) {
		g_errorEncountered = true;
		std::cout << "Could not find texture: " << tag << "!" << std::endl;
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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
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
 *  SetShaderTexture()
 *
 *  This method is used for setting the second texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture2(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTexture2Name, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_Texture2ValueName, textureID);
	}
}

/***********************************************************
 *  SetShaderSpecularMap()
 *
 *  This method is used for setting the second texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderSpecularMap(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseSpecularMap, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_SpecularMapName, textureID);
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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
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

 /***********************************************************
  *  DefineObjectMaterials()
  *
  * Create the materials used to define object reflective properties 
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{

	//Define a wood material for the desk
	OBJECT_MATERIAL wood_mat;
	wood_mat.tag = "wood";
	wood_mat.ambientColor = glm::vec3(0.1f, 0.1f, 0.0f);
	wood_mat.ambientStrength = 0.3f;
	wood_mat.diffuseColor = glm::vec3(0.7f, 0.7f, 0.6f);
	wood_mat.specularColor = glm::vec3(0.5f, 0.5f, 0.4f);
	wood_mat.shininess = 32.0f;
	
	m_objectMaterials.push_back(wood_mat);

	//Define a glass material for the displays
	OBJECT_MATERIAL glass_mat;
	glass_mat.tag = "glass";
	glass_mat.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	glass_mat.ambientStrength = 0.3f;
	glass_mat.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	glass_mat.specularColor = glm::vec3(0.8f, 0.8f, 0.8f);
	glass_mat.shininess = 64.0f;

	m_objectMaterials.push_back(glass_mat);

	//Define a plastic material for the peripherals
	OBJECT_MATERIAL plastic_mat;
	plastic_mat.tag = "plastic";
	plastic_mat.ambientColor = glm::vec3(0.0f, 0.1f, 0.1f);
	plastic_mat.ambientStrength = 0.3f;
	plastic_mat.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f);
	plastic_mat.specularColor = glm::vec3(0.2f, 0.6f, 0.4f);
	plastic_mat.shininess = 16.0f;

	m_objectMaterials.push_back(plastic_mat);

	//Define a soft plastic material for the trackball body
	OBJECT_MATERIAL track_mat;
	track_mat.tag = "trackball";
	track_mat.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	track_mat.ambientStrength = 0.3f;
	track_mat.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	track_mat.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);
	track_mat.shininess = 48.0f;

	m_objectMaterials.push_back(track_mat);

	//Define a shiny "marble" material for the trackball ball
	OBJECT_MATERIAL ball_mat;
	ball_mat.tag = "ballball";
	ball_mat.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	ball_mat.ambientStrength = 0.3f;
	ball_mat.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	ball_mat.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);
	ball_mat.shininess = 128.0f;

	m_objectMaterials.push_back(ball_mat);

	//Define an aluminium material for the metal elements in the scene
	OBJECT_MATERIAL alum_mat;
	alum_mat.tag = "aluminum";
	alum_mat.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);
	alum_mat.ambientStrength = 0.1f;
	alum_mat.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	alum_mat.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	alum_mat.shininess = 64.0f;

	m_objectMaterials.push_back(alum_mat);

	//Define a plastic material for the CRT display body
	OBJECT_MATERIAL crt_mat;
	crt_mat.tag = "crt";
	crt_mat.ambientColor = glm::vec3(0.6f, 0.6f, 0.6f);
	crt_mat.ambientStrength = 0.3f;
	crt_mat.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);
	crt_mat.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);
	crt_mat.shininess = 12.0f;

	m_objectMaterials.push_back(crt_mat);
}

/***********************************************************
  *  enableLighting()
  *
  *  Enable lighting and initialize state of lights
  * 
  * ***********************************************************/
void SceneManager::enableLighting()
{
	//Enable the lighting flag in the shader
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	//Initialize all lights as disabled
	m_pShaderManager->setIntValue("lightSources[0].enabled", false);
	m_pShaderManager->setIntValue("lightSources[1].enabled", false);
	m_pShaderManager->setIntValue("lightSources[2].enabled", false);
	m_pShaderManager->setIntValue("lightSources[3].enabled", false);
}

 /***********************************************************
  *  SetUpSceneLights()
  *
  *  Define the properties of the lights in the scene
  ***********************************************************/
void SceneManager::SetUpSceneLights()
{
	enableLighting();

	// A point light behind and to the right of the camera to simulate my ceiling light
	m_pShaderManager->setIntValue("lightSources[0].enabled", true);
	m_pShaderManager->setVec3Value("lightSources[0].position", 10.0f, 20.0f, 12.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 1.0f, 0.95f, 0.9f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 1.0f, 0.95f, 0.55f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 2.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.005f);

	// A point light behind the scene to simulate my desk lamp
	m_pShaderManager->setIntValue("lightSources[1].enabled", true);
	m_pShaderManager->setVec3Value("lightSources[1].position", 0.0f, 3.0f, -12.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 1.0f, 0.95f, 0.55f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 1.0f, 0.95f, 0.55f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 1.0f, 0.95f, 0.55f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 2.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f);
}

 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.                                 ***/
	// Simply exit the program if textures fail to load
	if (!CreateGLTexture("textures/keyboard-filament.png", "keyboard"))
		exit(1);
	if (!CreateGLTexture("textures/dark-wood.png", "desk"))
		exit(1);
	if (!CreateGLTexture("textures/keycap.png", "keycap"))
		exit(1);
	if (!CreateGLTexture("textures/deskmat.png", "deskmat"))
		exit(1);
	if (!CreateGLTexture("textures/deskmat_specularmap.png", "deskmat_specmap"))
		exit(1);
	if (!CreateGLTexture("textures/Hackerlambda1440p.png", "lambda_wallpaper"))
		exit(1);

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
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
	// Initialize the materials, scenes, and textures used in the scene
	DefineObjectMaterials();
	SetUpSceneLights();
	LoadSceneTextures();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadPrismMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	drawDesk();
	drawWalls();
	drawKeyboard();
	drawTrackball();

	//Draw objects with transparency last
	// Contains light logic
	drawPrimaryMonitor();
	drawSecondaryMonitor();
	drawTower();
}

void SceneManager::drawDesk()
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

	// Set the desk texture
	SetShaderTexture("desk");
	SetShaderTexture2("deskmat");
	SetShaderSpecularMap("deskmat_specmap");

	SetShaderMaterial("wood");


	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();

	// Disable the second texture layer and specular mapping after drawing the shape
	m_pShaderManager->setIntValue(g_UseTexture2Name, false);
	m_pShaderManager->setIntValue(g_UseSpecularMap, false);
}

void SceneManager::drawKeyboard()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	//Create the boxes that will be used as the base of the keyboard
	
	/* Main box*/
	//The main box is long in the x dimension, not very high, and less long in the z dimension
	scaleXYZ = glm::vec3(5.0f, 1.0f, 2.5f);

	//Rotate
	// The main box is rotated slighly along the z axis
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 15.0f;

	// The X Y Z positions will be set in variables so that they may be referenced by the smaller boxes that will be keycaps
	float mainBX = -5.0f;
	float mainBY = 1.0f;
	float mainBZ = 5.0f;
	positionXYZ = glm::vec3(mainBX, mainBY, mainBZ);

	// Apply changes

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Green color
	SetShaderColor(0, 1, 0, 1);

	// Set the texture of the keyboard
	SetShaderTexture("keyboard");

	SetShaderMaterial("plastic");

	//Draw the box
	m_basicMeshes->DrawBoxMesh();

	/****Divider***/
	/*Second box*/

	/****************************************************************/

	//The secondary box is short in the x dimension, not very high, and slightly long in the z dimension
	//In combination with the rotations described below, this effectively produces those dimensions from the same perspective as the main box
	scaleXYZ = glm::vec3(1.5f, 3.0f, 1.0f);

	//Rotate
	// The main box is rotated slighly along the x axis and y axis
	XrotationDegrees = -75.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = -15.0f;

	// The X Y Z positions will be set in variables so that they may be referenced by the smaller boxes that will be keycaps
	float secondBX = 2.0f + mainBX;
	float secondBY = 0.0f + mainBY;
	float secondBZ = 1.5f + mainBZ;
	positionXYZ = glm::vec3(secondBX, secondBY, secondBZ);

	// Apply changes

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Green color
	SetShaderColor(0, 1, 0, 1);

	// Set the texture of the keyboard
	SetShaderTexture("keyboard");

	SetShaderMaterial("plastic");

	//Draw the box
	m_basicMeshes->DrawBoxMesh();

	/****Divider***/

	/*Third box*/

	/****************************************************************/

	//The third box is short in the x dimension, not very high, and slightly long in the z dimension, shorter than the main box
	scaleXYZ = glm::vec3(1.5f, 0.75f, 1.75f);

	//Rotate
	// The third box has little to no rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// The X Y Z positions will be set in variables so that they may be referenced by the smaller boxes that will be keycaps
	float thirdBX = -3.0f + mainBX;
	float thirdBY = -0.75f + mainBY;
	float thirdBZ = 0.0f + mainBZ;
	positionXYZ = glm::vec3(thirdBX, thirdBY, thirdBZ);

	// Apply changes

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Green color
	SetShaderColor(0, 1, 0, 1);

	// Set the texture of the keyboard
	SetShaderTexture("keyboard");

	SetShaderMaterial("plastic");

	//Draw the box
	m_basicMeshes->DrawBoxMesh();

	/****Divider***/
	// Front Casing for main box


	/* The casing for the keyboard will consist of boxes that are scaled in the y dimension and one other dimension to create a bridge in the gaps between the main boxes*/
	scaleXYZ = glm::vec3(5.0f, 2.0f, 0.1f);

	//Rotate
	// There should be little rotation in the piece of casing attached to the main box, only matching the main box
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 15.0f;

	// The X Y Z positions will be relevant to the main box
	positionXYZ = glm::vec3(0.0f + mainBX, -1.0f + mainBY, 1.25f + mainBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Green color
	SetShaderColor(0, 1, 0, 1);

	// Set the texture of the keyboard
	SetShaderTexture("keyboard");

	SetShaderMaterial("plastic");

	//Draw the box
	m_basicMeshes->DrawBoxMesh();

	/****Divider***/

	// Back Casing for main box
	// Same size and rotations as previous box, but with a different z dimension pos

	scaleXYZ = glm::vec3(5.0f, 2.0f, 0.1f);

	//Rotate
	// There should be little rotation in the piece of casing attached to the main box, only matching the main box
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 15.0f;

	// The X Y Z positions will be relevant to the main box
	positionXYZ = glm::vec3(0.0f + mainBX, -1.0f + mainBY, -1.25f + mainBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Green color
	SetShaderColor(0, 1, 0, 1);

	// Set the texture of the keyboard
	SetShaderTexture("keyboard");

	SetShaderMaterial("plastic");

	//Draw the box
	m_basicMeshes->DrawBoxMesh();


	/****Divider***/

	// Casing around minimum x dimension of second box

	scaleXYZ = glm::vec3(2.0f, 2.0f, 0.1f);

	//Rotate
	// This casing must be rotated about the y dimension to be at the same angle as the second box
	XrotationDegrees = 0.0f;
	YrotationDegrees = -70.0f;
	ZrotationDegrees = 0.0f;

	// The X Y Z positions will be relevant to the second box
	positionXYZ = glm::vec3(-0.5f + secondBX, -1.0f + secondBY, 0.5f + secondBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Green color
	SetShaderColor(0, 1, 0, 1);

	// Set the texture of the keyboard
	SetShaderTexture("keyboard");

	SetShaderMaterial("plastic");

	//Draw the box
	m_basicMeshes->DrawBoxMesh();


	/****Divider***/

	// Side Casing for main box

	scaleXYZ = glm::vec3(0.1f, 2.0f, 3.5f);

	//Rotate
	// We will make this box orthogonal by using scaling rather than rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// The X Y Z positions will be relevant to the main box
	positionXYZ = glm::vec3(2.3f + mainBX, 0.0f + mainBY, 0.5f + mainBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Green color
	SetShaderColor(0, 1, 0, 1);

	// Set the texture of the keyboard
	SetShaderTexture("keyboard");

	SetShaderMaterial("plastic");

	//Draw the box
	m_basicMeshes->DrawBoxMesh();


	/****Divider***/



	//**Begin keycap section**//

	/*COLUMN 1*/

	//All keycaps will be same size
	float keycapX = 0.5f;
	float keycapY = keycapX / 2.0f;
	float keycapZ = keycapX;

	scaleXYZ = glm::vec3(keycapX, keycapY, keycapZ);

	//Rotate
	// Most keycaps will be rotated only to make them parallel to the box they are attached to 
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 15.0f;

	//Distance between keycaps should be mostly uniform
	float distanceKeycapsXZ = 0.75f;
	float negDistanceKeycapsXZ = (-1.0f) * distanceKeycapsXZ;

	// The X Y Z positions will be relevant to the main box
	positionXYZ = glm::vec3(0.0f + mainBX, 0.5f + mainBY, distanceKeycapsXZ + mainBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Off-white color
	SetShaderColor(0.9f, 1.0f, 1.0f, 1);
	// Set the texture of the keyboard
	SetShaderTexture("keycap");
	SetShaderMaterial("plastic");

	//Draw the box
	m_basicMeshes->DrawBoxMesh();


	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3(0.0f + mainBX, 0.5f + mainBY, 0.0f + mainBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3(0.0f + mainBX, 0.5f + mainBY, negDistanceKeycapsXZ + mainBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	/*COLUMN 2*/

	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3(negDistanceKeycapsXZ + mainBX, 0.35f + mainBY, distanceKeycapsXZ + mainBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3(negDistanceKeycapsXZ + mainBX, 0.35f + mainBY, 0.0f + mainBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3(negDistanceKeycapsXZ + mainBX, 0.35f + mainBY, negDistanceKeycapsXZ + mainBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	
	/*COLUMN 3*/

	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3((2.0f * negDistanceKeycapsXZ) + mainBX, 0.2f + mainBY, negDistanceKeycapsXZ + mainBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3((2.0f * negDistanceKeycapsXZ) + mainBX, 0.2f + mainBY, 0.0f + mainBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3((2.0f * negDistanceKeycapsXZ) + mainBX, 0.2f + mainBY, distanceKeycapsXZ + mainBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	/*COLUMN 4*/
	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3(distanceKeycapsXZ + mainBX, 0.65f + mainBY, distanceKeycapsXZ + mainBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3(distanceKeycapsXZ + mainBX, 0.65f + mainBY, 0.0f + mainBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3(distanceKeycapsXZ + mainBX, 0.65f + mainBY, negDistanceKeycapsXZ + mainBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	/*DIVIDER*/
	/* New base will be on the second box for the thumb cluster keys */
	// Therefore, we will change the rotation of the keycaps to match this base, and the relative offset will be secondBX/Y/Z instead of mainBX/Y/Z

	//Keycap will be orthogonal in the x dimension to the second box due to the strange rotation of the second box
	XrotationDegrees = 15.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = -15.0f;

	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3(0.0f + secondBX, 0.35f + secondBY, distanceKeycapsXZ + secondBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3((distanceKeycapsXZ / 2.0f) + secondBX, 0.075f + secondBY, (2.0f * distanceKeycapsXZ) + secondBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3(distanceKeycapsXZ + secondBX, 0.1f + secondBY, (distanceKeycapsXZ / 2.0f) + secondBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3((1.5f * distanceKeycapsXZ) + secondBX, -0.1f + secondBY, (1.5f * distanceKeycapsXZ) + secondBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	/*DIVIDER*/
	/* New base will be on the third box for the pinky column keys */
	// Therefore, we will change the rotation of the keycaps to match this base, and the relative offset will be thirdBX/Y/Z instead of mainBX/Y/Z

	//Keycaps, like the third box, will have no rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//An offset for the reduced size of the third box in comparison to the main box will assist in the alignment of the keys 
	float thirdOffset = 0.25f;

	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3(thirdOffset + thirdBX, 0.35f + thirdBY, distanceKeycapsXZ - thirdOffset + thirdBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3(thirdOffset + thirdBX, 0.35f + thirdBY, negDistanceKeycapsXZ + thirdOffset + thirdBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();

	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3(negDistanceKeycapsXZ + thirdOffset + thirdBX, 0.35f + thirdBY, distanceKeycapsXZ - thirdOffset + thirdBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();


	//Only position will be explicitly changed until new base is required
	positionXYZ = glm::vec3(negDistanceKeycapsXZ + thirdOffset + thirdBX, 0.35f + thirdBY, negDistanceKeycapsXZ + thirdOffset + thirdBZ);

	//Apply changes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	m_basicMeshes->DrawBoxMesh();



}

void SceneManager::drawWalls()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Create a wall taller than all other objects in the scene
	scaleXYZ = glm::vec3(50.0f, 1.0f, 25.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// Position the wall global left
	positionXYZ = glm::vec3(-25.0f, 0.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0.9, 0.55f, 1);

	// Set the wood material
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


	// Second wall will be of similar size
	scaleXYZ = glm::vec3(50.0f, 1.0f, 50.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position the wall behind the scene
	positionXYZ = glm::vec3(0.0f, 0.0f, -15.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 0.9, 0.55f, 1);

	// Set the wood material
	SetShaderMaterial("wood");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
}

void SceneManager::drawPrimaryMonitor()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Scale of the monitor
	scaleXYZ = glm::vec3(10.0f, 1.0f, 5.0f);

	// Rotate to sit upright and parallel with the front edge of the desk
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position behind and above the keyboard 
	float displayX = 0.0f;
	float displayY = 8.0f;
	float displayZ = -7.5f;
	positionXYZ = glm::vec3(displayX, displayY, displayZ);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	SetShaderColor(1, 1, 1, 1);

	// Use the glass material
	SetShaderTexture("lambda_wallpaper");
	SetShaderMaterial("glass");

	// Draw the desired plane
	m_basicMeshes->DrawPlaneMesh();

	// Disable textures for the legs
	m_pShaderManager->setIntValue(g_UseTextureName, false);

	// Draw the glass in front of the panel
	// Scale of the monitor
	scaleXYZ = glm::vec3(10.0f, 1.0f, 5.0f);

	// Rotate to sit upright and parallel with the front edge of the desk
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position just before the display 
	positionXYZ = glm::vec3(displayX, displayY, displayZ + 0.05f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Make it transparent so the specular effect is felt more strongly
	// and the panel behind is still visible
	SetShaderColor(1, 1, 1, 0.1f);

	// Use the glass material
	SetShaderMaterial("glass");

	// Draw the desired plane
	m_basicMeshes->DrawPlaneMesh();

	// Reset the alpha channel
	SetShaderColor(1, 1, 1, 1);

	// Draw the monitor stand by using 3 prisms, two as legs and one as a column
	// Column will be scaled "upright" in world space (in the y axis)
	scaleXYZ = glm::vec3(1.0f, 9.75f, 1.0f);

	// Rotate to have the tip of the prism face the opposite direction (about the y axis)
	XrotationDegrees = 0.0f;
	YrotationDegrees = 180.0f;
	ZrotationDegrees = 0.0f;

	// Position behind and below the display glass 
	positionXYZ = glm::vec3(displayX, displayY - 3.0f, displayZ - 0.51f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Use the metal material
	SetShaderMaterial("aluminum");

	// Draw the desired prism
	m_basicMeshes->DrawPrismMesh();

	// Each leg will be scaled in the y dimension and slightly higher in the x dimension
	scaleXYZ = glm::vec3(1.5f, 7.5f, 0.25f);

	// Rotate first about the x axis to lay almost "flat" in world space, 
	// then about the y axis to angle out from the column
	XrotationDegrees = 100.0f;
	YrotationDegrees = 65.0f;
	ZrotationDegrees = 0.0f;

	// Position behind and below the display glass 
	positionXYZ = glm::vec3(displayX + 3.25f, displayY - 7.25f, displayZ + 1.75f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Use the metal material
	SetShaderMaterial("aluminum");

	// Draw the desired prism
	m_basicMeshes->DrawPrismMesh();

	// Each leg will be scaled in the y dimension and slightly higher in the x dimension
	scaleXYZ = glm::vec3(1.5f, 7.5f, 0.25f);

	// Rotate first about the x axis to lay almost "flat" in world space, 
	// then about the y axis to angle out from the column
	XrotationDegrees = 100.0f;
	YrotationDegrees = -65.0f;
	ZrotationDegrees = 0.0f;

	// Position behind and below the display glass 
	positionXYZ = glm::vec3(displayX - 3.25f, displayY - 7.25f, displayZ + 1.75f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Use the metal material
	SetShaderMaterial("aluminum");

	// Draw the desired prism
	m_basicMeshes->DrawPrismMesh();

}

void SceneManager::drawSecondaryMonitor()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// The shape of the monitor can be drawn with some boxes
	scaleXYZ = glm::vec3(10.0f, 10.0f, 3.0f);

	//Rotate slightly about the x axis for a tilt and then 
	//about the y axis to angle towards the center of the desk
	XrotationDegrees = 10.0f;
	YrotationDegrees = 245.0f;
	ZrotationDegrees = 0.0f;

	// World left of every other object
	float displayX = -15.0f;
	float displayY = 7.0f;
	float displayZ = 0.0f;
	positionXYZ = glm::vec3(displayX, displayY, displayZ);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Use the crt casing material
	SetShaderMaterial("crt");
	//Ensure color is set to default
	SetShaderColor(1, 1, 1, 1);

	// Draw the desired Box
	m_basicMeshes->DrawBoxMesh();


	// Draw the electron gun casing behind the first box
	scaleXYZ = glm::vec3(9.0f, 8.0f, 8.0f);

	//Rotate slightly about the x axis for a tilt and then 
	//about the y axis to angle towards the center of the desk
	XrotationDegrees = 10.0f;
	YrotationDegrees = 245.0f;
	ZrotationDegrees = 0.0f;

	// Position behind the main casing
	positionXYZ = glm::vec3(displayX - 2.5f, displayY - 1.5f, displayZ - 1.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Use the trackball material to get the desired properties
	SetShaderMaterial("trackball");
	SetShaderColor(0.3f, 0.3f, 0.3f, 1);

	// Draw the desired Box
	m_basicMeshes->DrawBoxMesh();

	// Draw the base of the display using a cylinder
	scaleXYZ = glm::vec3(5.0f, 1.0f, 5.0f);

	// Only rotate about the y axis so the base is parallel to the desk
	XrotationDegrees = 0.0f;
	YrotationDegrees = 245.0f;
	ZrotationDegrees = 0.0f;

	// Position behind the main casing
	positionXYZ = glm::vec3(displayX - 2.5f, displayY - 7.0f, displayZ - 1.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Use the trackball material to get the desired properties
	SetShaderMaterial("trackball");
	SetShaderColor(0.3f, 0.3f, 0.3f, 1);

	// Draw the desired Box
	m_basicMeshes->DrawCylinderMesh();

	// Draw a glass Box that is scaled 4:3 in the x and y dimensions to match the crt aspect
	scaleXYZ = glm::vec3(8.0f, 6.0f, 2.0f);

	//Rotate slightly about the x axis for a tilt and then 
	//about the y axis to angle towards the center of the desk
	XrotationDegrees = 10.0f;
	YrotationDegrees = 245.0f;
	ZrotationDegrees = 0.0f;

	// Position in front of the main casing
	positionXYZ = glm::vec3(displayX + 0.9f, displayY, displayZ + 0.495f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Use the glass material to get the desired properties
	SetShaderMaterial("glass");
	SetShaderColor(0.2f, 0.2f, 0.2f, 1);
	SetShaderTexture("lambda_wallpaper");

	// Draw the desired Box
	m_basicMeshes->DrawBoxMesh();

	//Reset alpha channel
	SetShaderColor(1, 1, 1, 1);

	// Draw a second glass Box that is scaled 4:3 in the x and y dimensions to match the crt aspect
	// then rotate about the y axis to match the body
	// This box will be in front of the other display to add a more glassy effect
	scaleXYZ = glm::vec3(8.0f, 6.0f, 2.0f);

	//Rotate slightly about the x axis for a tilt and then 
	//about the y axis to angle towards the center of the desk
	XrotationDegrees = 10.0f;
	YrotationDegrees = 245.0f;
	ZrotationDegrees = 0.0f;

	// Position in front of the main casing
	positionXYZ = glm::vec3(displayX + 1.0f, displayY, displayZ + 0.5f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Use the glass material to get the desired properties
	SetShaderMaterial("glass");
	SetShaderColor(0.2f, 0.2f, 0.2f, 0.2f);

	// Draw the desired Box
	m_basicMeshes->DrawBoxMesh();

	//Reset alpha channel
	SetShaderColor(1, 1, 1, 1);
}

/* updateRGBlight()
 *
 * Changes the color value of the rgb light inside the desktop tower
 */
void SceneManager::updateRGBlight()
{
	//Calculate values for R, G, and B
	float colorR = glm::sin((float) glfwGetTime());
	float colorG = glm::cos((float) glfwGetTime());
	float colorB = glm::sin((float) glfwGetTime()) / 2.0f;
	// A point light imitating RGB lighting, value should change over time
	m_pShaderManager->setIntValue("lightSources[2].enabled", true);
	m_pShaderManager->setVec3Value("lightSources[2].position", 11.5f, 4.5f, 5.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", colorR * 0.3f, colorG * 0.3f, colorB * 0.3f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", colorR * 0.3f, colorG * 0.3f, colorB * 0.3f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", colorR * 0.3f, colorG * 0.3f, colorB * 0.3f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 2.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.005f);

	//Set the color of the cylinder equal to the average between the light and white
	colorR = (2.0f + colorR) / 3.0f;
	colorG = (2.0f + colorG) / 3.0f;
	colorB = (2.0f + colorB) / 3.0f;
	SetShaderColor(colorR, colorG, colorB, 1);
}

void SceneManager::drawTower()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Represent the desktop tower case with 4 planes, one of which will have transparency to allow RGB
	// light through
	
	// Front panel is tall (y dimension) after being rotated
	float panelScale = 2.25f;
	scaleXYZ = glm::vec3(panelScale, 1.0f, panelScale * 2.0f);

	// Rotate about the x axis to sit upright
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Position to world right (high x) of all other objects
	float frontPanelX = 12.5f;
	float frontPanelY = 4.5f;
	float frontPanelZ = 7.5f;
	positionXYZ = glm::vec3(frontPanelX, frontPanelY, frontPanelZ);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Set a default color
	SetShaderColor(0.3f, 0.3f, 0.3f, 1);

	// Give the case a metal material
	SetShaderMaterial("aluminum");

	m_basicMeshes->DrawPlaneMesh();

	// Back panel will be roughly twice as long as the front, but same height
	scaleXYZ = glm::vec3(panelScale * 2.0f, 1.0f, panelScale * 2.0f);

	// Rotate about the x axis to sit upright and about the y axis to be orthogonal to the front panel
	XrotationDegrees = 90.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	//Position on the right side (high x) of the front panel
	positionXYZ = glm::vec3(frontPanelX + panelScale, frontPanelY, frontPanelZ - (2.0f * panelScale));

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	m_basicMeshes->DrawPlaneMesh();

	//Rear panel will be identical to the front panel, but further back in the z dimension
	scaleXYZ = glm::vec3(panelScale, 1.0f, panelScale * 2.0f);

	// Rotate about the x axis to sit upright
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Position behind (low z)the front panel
	positionXYZ = glm::vec3(frontPanelX, frontPanelY, frontPanelZ - (4.0f * panelScale));

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	m_basicMeshes->DrawPlaneMesh();

	// Top of the case will not be rotated about the x axis, but will be the same size as the front panel
	scaleXYZ = glm::vec3(panelScale, 1.0f, panelScale * 2.0f);

	// No rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Position above and central to all other panels
	positionXYZ = glm::vec3(frontPanelX, frontPanelY + (panelScale * 2.0f), frontPanelZ - (2.0f * panelScale));

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	m_basicMeshes->DrawPlaneMesh();

	//Bottom of the case will be identical to the top, but with low y position
	scaleXYZ = glm::vec3(panelScale, 1.0f, panelScale * 2.0f);

	// No rotation
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	//Position below and central to all other panels
	positionXYZ = glm::vec3(frontPanelX, frontPanelY - (panelScale * 1.95f), frontPanelZ - (2.0f * panelScale));

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	m_basicMeshes->DrawPlaneMesh();


	//Create a cylinder to act as the RGB fan for the PC
	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);

	// Rotate about the z axis to sit against the case back panel
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	//Position below and central to all other panels
	positionXYZ = glm::vec3(frontPanelX + panelScale - 0.1f, frontPanelY, frontPanelZ - (2.0f * panelScale));

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Update the values for the rgb light inside the tower
	updateRGBlight();

	// We want high reflectiveness, so we will recycle the trackball material
	SetShaderMaterial("ballball");

	m_basicMeshes->DrawCylinderMesh();

	//Reset colors
	SetShaderColor(1, 1, 1, 1);

	// Draw the glass panel after all other parts of the desktop for the alpha channel to be drawn properly
	// Side panel will be made of transparent glass material and will be world left (low x) all other panels
	scaleXYZ = glm::vec3(panelScale * 2.0f, 1.0f, panelScale * 2.0f);

	// Rotate about the x axis to sit upright and about the y axis to be orthogonal to the front panel
	XrotationDegrees = 90.0f;
	YrotationDegrees = 270.0f;
	ZrotationDegrees = 0.0f;

	//Position on the left side (low x) of the front panel
	positionXYZ = glm::vec3(frontPanelX - panelScale, frontPanelY, frontPanelZ - (2.0f * panelScale));

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	//Add an alpha transparency and use glass mat
	SetShaderColor(1, 1, 1, 0.1f);
	SetShaderMaterial("glass");

	m_basicMeshes->DrawPlaneMesh();

	//Reset Shader color and alpha channel
	SetShaderColor(1, 1, 1, 1);


}

void SceneManager::drawTrackball()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// Draw the sphere used as the ball of the trackball

	scaleXYZ = glm::vec3(1.0f, 1.0f, 1.0f);

	// To the right(high x) of the keyboard
	float trackballX = 4.0f;
	float trackballY = 1.0f;
	float trackballZ = 5.0f;
	positionXYZ = glm::vec3(trackballX, trackballY, trackballZ);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Use the trackball ball material
	SetShaderMaterial("ballball");

	// Set a default color
	SetShaderColor(0.5f, 0.5f, 0.5f, 1);

	m_basicMeshes->DrawSphereMesh();

	// Draw the trackball receptacle

	scaleXYZ = glm::vec3(1.5f, 1.0f, 1.5f);

	// To the right(high x) of the keyboard
	positionXYZ = glm::vec3(trackballX, trackballY - 1.0f, trackballZ + 0.2f);

	XrotationDegrees = -12.5f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);


	// Use the trackball material
	SetShaderMaterial("trackball");

	// Set a darker color than the ball
	SetShaderColor(0.3f, 0.3f, 0.3f, 1);

	m_basicMeshes->DrawCylinderMesh();


	// The body of the trackball under the receptacle
	scaleXYZ = glm::vec3(1.5f, 1.0f, 2.25f);

	// To the right(high x) of the keyboard
	positionXYZ = glm::vec3(trackballX, trackballY - 1.25f, trackballZ + 0.5f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);


	// Use the trackball material
	SetShaderMaterial("trackball");

	m_basicMeshes->DrawCylinderMesh();


	// Draw the palm rest

	// Less round than the ball, more elongated in the z dimension
	scaleXYZ = glm::vec3(1.25f, 0.75f, 2.0f);

	// Closer to the camera (high z) than the ball
	positionXYZ = glm::vec3(trackballX, trackballY - 0.25f, trackballZ + 1.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Use the trackball material
	SetShaderMaterial("trackball");

	m_basicMeshes->DrawSphereMesh();


	// Draw the flare near the palm rest
	scaleXYZ = glm::vec3(2.0f, 0.5f, 1.25f);

	// Further back (high z)than each other component of the trackball
	positionXYZ = glm::vec3(trackballX, trackballY - 1.0f, trackballZ + 1.75f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Use the trackball body material
	SetShaderMaterial("trackball");

	m_basicMeshes->DrawCylinderMesh();

}

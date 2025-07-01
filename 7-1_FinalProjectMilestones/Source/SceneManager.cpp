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
// LAST REVISED: June 20, 2025
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
#include <iostream>
#include "ViewManager.h"

// declaration of global uniform-names
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
 *  Constructor: initialize pointers and reset texture array.
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
    m_pShaderManager = pShaderManager;
    m_basicMeshes = new ShapeMeshes();

    // Initialize texture ID array and counter
    for (int i = 0; i < 16; i++)
    {
        m_textureIDs[i].tag = "/0";
        m_textureIDs[i].ID = -1;
    }
    m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  Destructor: free meshes and textures.
 ***********************************************************/
SceneManager::~SceneManager()
{
    // Destroy loaded OpenGL textures
    DestroyGLTextures();

    // Free the ShapeMeshes pointer
    delete m_basicMeshes;
    m_basicMeshes = nullptr;

    m_pShaderManager = nullptr;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  Load a single texture from file (JPG/PNG/etc),
 *  generate mipmaps, and store its GLuint in m_textureIDs.
 *
 *  Returns true if loaded successfully.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
    int width = 0, height = 0, colorChannels = 0;
    GLuint textureID = 0;

    stbi_set_flip_vertically_on_load(true);
    unsigned char* image = stbi_load(filename, &width, &height, &colorChannels, 0);
    if (!image)
    {
        std::cout << "Could not load image: " << filename << std::endl;
        return false;
    }

    std::cout << "Successfully loaded image: " << filename
        << ", width: " << width
        << ", height: " << height
        << ", channels: " << colorChannels << std::endl;

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (colorChannels == 3)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    else if (colorChannels == 4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    else
    {
        std::cout << "Unsupported channels: " << colorChannels << " in " << filename << std::endl;
        stbi_image_free(image);
        return false;
    }

    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(image);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_textureIDs[m_loadedTextures].ID = textureID;
    m_textureIDs[m_loadedTextures].tag = tag;
    m_loadedTextures++;

    return true;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  Bind each loaded texture to GL_TEXTUREi.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
    }
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  Delete all GPU textures we created.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
    for (int i = 0; i < m_loadedTextures; i++)
    {
        if (m_textureIDs[i].ID != (uint32_t)-1)
        {
            GLuint tex = m_textureIDs[i].ID;
            glDeleteTextures(1, &tex);
            m_textureIDs[i].ID = -1;
            m_textureIDs[i].tag = "/0";
        }
    }
    m_loadedTextures = 0;
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  Return the texture-unit index (0..15) for a given tag,
 *  or -1 if not found.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
    for (int i = 0; i < m_loadedTextures; ++i)
    {
        if (m_textureIDs[i].tag == tag)
            return i;
    }
    return -1;
}

/***********************************************************
 *  SetTransformations()
 *
 *  Build a model-matrix from scale/rotation/translation
 *  and upload it to the shader uniform "model".
 ***********************************************************/
void SceneManager::SetTransformations(
    glm::vec3 scaleXYZ,
    float      XrotationDegrees,
    float      YrotationDegrees,
    float      ZrotationDegrees,
    glm::vec3 positionXYZ)
{
    glm::mat4 model =
        glm::translate(positionXYZ) *
        glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0, 0, 1)) *
        glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0, 1, 0)) *
        glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1, 0, 0)) *
        glm::scale(scaleXYZ);

    if (m_pShaderManager)
        m_pShaderManager->setMat4Value(g_ModelName, model);
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  Switch shader to "texture mapping" mode and bind the
 *  sampler to the correct texture slot.
 ***********************************************************/
void SceneManager::SetShaderTexture(std::string tag, float scale)
{
    if (m_pShaderManager)
    {
        m_pShaderManager->setIntValue(g_UseTextureName, true);
        int slot = FindTextureSlot(tag);
        m_pShaderManager->setSampler2DValue(g_TextureValueName, slot);
        m_pShaderManager->setVec2Value("UVscale", glm::vec2(scale));
    }
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
    m_pShaderManager->setBoolValue(g_UseLightingName, true);

    m_pShaderManager->setVec3Value("dirLight.direction", glm::vec3(-0.5f, -1.0f, -0.3f));
    m_pShaderManager->setVec3Value("dirLight.ambient", glm::vec3(0.1f));
    m_pShaderManager->setVec3Value("dirLight.diffuse", glm::vec3(0.8f));
    m_pShaderManager->setVec3Value("dirLight.specular", glm::vec3(1.0f));

    m_pShaderManager->setVec3Value("pointLight.position", glm::vec3(0.0f, 10.0f, 0.0f));
    m_pShaderManager->setVec3Value("pointLight.ambient", glm::vec3(0.05f));
    m_pShaderManager->setVec3Value("pointLight.diffuse", glm::vec3(0.5f));
    m_pShaderManager->setVec3Value("pointLight.specular", glm::vec3(0.7f));
    m_pShaderManager->setFloatValue("pointLight.constant", 1.0f);
    m_pShaderManager->setFloatValue("pointLight.linear", 0.09f);
    m_pShaderManager->setFloatValue("pointLight.quadratic", 0.032f);
}

/***********************************************************
 *  LoadSceneTextures()
 *
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
    CreateGLTexture("textures/wood.jpg", "wood");
    CreateGLTexture("textures/white_wood.jpg", "whiteWood");
    CreateGLTexture("textures/concrete.png", "concrete");
    CreateGLTexture("textures/green.png", "green");
    CreateGLTexture("textures/gray.png", "gray");
    CreateGLTexture("textures/black.png", "black");

    BindGLTextures();
}

/***********************************************************
 *  PrepareScene()
 *
 *  Called once at startup: load textures + meshes + lights.
 ***********************************************************/
void SceneManager::PrepareScene()
{
    LoadSceneTextures();
    m_basicMeshes->LoadPlaneMesh();
    m_basicMeshes->LoadBoxMesh();
    m_basicMeshes->LoadCylinderMesh();
    SetupSceneLights();
}

/***********************************************************
 *  RenderScene()
 *
 *  Called every frame: draw ground + kitchen island.
 ***********************************************************/
void SceneManager::RenderScene()
{
    glm::vec3 scale, position;
    float rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f;

    // --------- Ground Plane (Wood Texture, Phong Lit) ---------
    scale = glm::vec3(50.0f, 1.0f, 50.0f);
    position = glm::vec3(0.0f, -0.5f, 0.0f);
    SetTransformations(scale, rotX, rotY, rotZ, position);
    SetShaderTexture("wood", 10.0f);
    m_basicMeshes->DrawPlaneMesh();

    // --------- Kitchen Island Base (White Wood) ---------
    scale = glm::vec3(10.0f, 4.0f, 4.0f);
    position = glm::vec3(0.0f, 2.0f, 0.0f);
    SetTransformations(scale, rotX, rotY, rotZ, position);
    SetShaderTexture("whiteWood", 2.0f);
    m_basicMeshes->DrawBoxMesh();

    // --------- Kitchen Island Countertop (Concrete) ---------
    scale = glm::vec3(11.0f, 0.3f, 5.0f);
    position = glm::vec3(0.0f, 4.3f, 0.0f);
    SetTransformations(scale, rotX, rotY, rotZ, position);
    SetShaderTexture("concrete", 2.0f);
    m_basicMeshes->DrawBoxMesh();

    // --- Laptop (black) ---
    // Base
    SetTransformations(glm::vec3(1.05f, 0.06f, 0.7f), 0, 16, 0, glm::vec3(-1.4f, 2.49f, -0.45f));
    SetShaderTexture("black");
    SetTextureUVScale(1.0f, 1.0f);
    m_basicMeshes->DrawBoxMesh();
    // Screen
    SetTransformations(glm::vec3(1.05f, 0.75f, 0.06f), -90, 16, 0, glm::vec3(-1.4f, 2.87f, -0.05f));
    SetShaderTexture("black");
    m_basicMeshes->DrawBoxMesh();

    // --- Water Bottle (gray) ---
    SetTransformations(glm::vec3(0.13f, 0.45f, 0.13f), 0, 0, 0, glm::vec3(0, 2.57f, -0.17f));
    SetShaderTexture("gray");
    SetTextureUVScale(1.0f, 1.0f);
    m_basicMeshes->DrawCylinderMesh();
    // Cap
    SetTransformations(glm::vec3(0.15f, 0.04f, 0.15f), 0, 0, 0, glm::vec3(0, 2.81f, -0.17f));
    SetShaderTexture("black");
    m_basicMeshes->DrawCylinderMesh();

    // --- Green Shoebox (green, blocky, like a LEGO brick) ---
    SetTransformations(glm::vec3(0.7f, 1.1f, 0.35f), 0, -13, 0, glm::vec3(1.5f, 2.92f, 0.7f));
    SetShaderTexture("green");
    SetTextureUVScale(1.0f, 1.0f);
    m_basicMeshes->DrawBoxMesh();
}

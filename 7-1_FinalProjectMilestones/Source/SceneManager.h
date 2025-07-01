///////////////////////////////////////////////////////////////////////////////
// scenemanager.h
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//  Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ShaderManager.h"
#include "ShapeMeshes.h"

#include <string>
#include <vector>
#include <glm/glm.hpp>   // for glm::vec3

/***********************************************************
 *  SceneManager
 *
 *  This class contains the code for preparing and rendering
 *  3D scenes, including the shader settings.
 ***********************************************************/
class SceneManager
{
public:
    // constructor
    SceneManager(ShaderManager* pShaderManager);
    // destructor
    ~SceneManager();

    struct TEXTURE_INFO
    {
        std::string tag;
        uint32_t    ID;   // holds the OpenGL GLuint
    };

    struct OBJECT_MATERIAL
    {
        glm::vec3 diffuseColor;
        glm::vec3 specularColor;
        float     shininess;
        std::string tag;
    };

private:
    // pointer to shader manager object
    ShaderManager* m_pShaderManager;
    // pointer to basic shapes object
    ShapeMeshes* m_basicMeshes;

    // total number of loaded textures
    int            m_loadedTextures;
    // loaded textures info (up to 16)
    TEXTURE_INFO   m_textureIDs[16];

    // defined object materials (if used)
    std::vector<OBJECT_MATERIAL> m_objectMaterials;

    // ---------------------------------------------------
    // Texture-Loading Helpers
    // ---------------------------------------------------

    // load texture image from 'filename' and tag it with 'tag'
    bool CreateGLTexture(const char* filename, std::string tag);

    // bind all loaded OpenGL textures to their texture units
    void BindGLTextures();

    // free (delete) all loaded OpenGL textures
    void DestroyGLTextures();

    // return the GLuint ID of a texture by tag, or -1 if not found
    int FindTextureID(std::string tag);

    // return the texture slot index (0..15) by tag, or -1 if not found
    int FindTextureSlot(std::string tag);

    // ---------------------------------------------------
    // Material-Lookup Helper (stubbed out if not used)
    // ---------------------------------------------------
    bool FindMaterial(std::string tag, OBJECT_MATERIAL& material);

    // ---------------------------------------------------
    // Shader-Transform & Material Setters
    // ---------------------------------------------------

    // set the model matrix via the "model" uniform
    void SetTransformations(
        glm::vec3 scaleXYZ,
        float      XrotationDegrees,
        float      YrotationDegrees,
        float      ZrotationDegrees,
        glm::vec3 positionXYZ);

    // set a solid color in the shader ( turns off texture use )
    void SetShaderColor(
        float redColorValue,
        float greenColorValue,
        float blueColorValue,
        float alphaValue);

    // set a texture in the shader ( turns on texture use )
    void SetShaderTexture(std::string textureTag, float scale = 1.0f);

    // set the UV scale uniform for tiling textures
    void SetTextureUVScale(float u, float v);

    // set material properties (diffuse, specular, shininess)
    void SetShaderMaterial(std::string materialTag);

    // ---------------------------------------------------
    // Scene-Building Helpers
    // ---------------------------------------------------

    // load six textures from disk:
    //   - textures/wood.jpg       tagged "wood"
    //   - textures/white_wood.jpg tagged "whiteWood"
    //   - textures/concrete.png   tagged "concrete"
    //   - textures/green.png      tagged "green"
    //   - textures/gray.png       tagged "gray"
    //   - textures/black.png      tagged "black"
    void LoadSceneTextures();

public:
    // called once at startup: load textures + meshes
    void PrepareScene();

    // called each frame: draw the scene
    void RenderScene();
};

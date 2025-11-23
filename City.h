#pragma once

#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Mesh.h"
#include "Texture.h"
#include "Material.h"

struct Building {
    glm::vec3 position;
    glm::vec3 scale;
    float rotation;
    int meshIndex;
    Texture* texture;
    Material* material;
};

class City {
public:
    City();
    ~City();

    void CreateCity();
    void RenderCity(GLuint uniformModel, GLuint uniformSpecularIntensity, GLuint uniformShininess, float deltaTime);
    void SetTextures(Texture* brick, Texture* dirt);
    void SetMaterials(Material* shiny, Material* dull);

    std::vector<Mesh*>& GetMeshList() { return meshList; }

private:
    void CreateMeshes();
    void CreateBuildings();
    void CreateFloorAndWalls();

    std::vector<Mesh*> meshList;
    std::vector<Building> buildings;

    Texture* brickTexture;
    Texture* dirtTexture;
    Material* shinyMaterial;
    Material* dullMaterial;

    int pyramidMeshIndex;
    int floorMeshIndex;
    int wallMeshIndex;

    // Animation variables
    float currAngle;
    bool direction;
    float triOffset;
    float triMaxOffset;
    float triIncrement;
    bool sizeDirection;
    float curSize;
    float maxSize;
    float minSize;

    void calcAverageNormals(unsigned int* indices, unsigned int indiceCount,
        GLfloat* vertices, unsigned int verticeCount,
        unsigned int vLength, unsigned int normalOffset);
};
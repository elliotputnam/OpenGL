#include "City.h"

City::City() : 
    brickTexture(nullptr), 
    dirtTexture(nullptr),
    shinyMaterial(nullptr),
    dullMaterial(nullptr),
    pyramidMeshIndex(-1),
    floorMeshIndex(-1),
    wallMeshIndex(-1),
    currAngle(0.0f),
    direction(true),
    triOffset(0.0f),
    triMaxOffset(0.4f),
    triIncrement(0.001f),
    sizeDirection(true),
    curSize(0.3f),
    maxSize(0.4f),
    minSize(0.2f)
{
}

City::~City() {
    for (auto mesh : meshList) {
        delete mesh;
    }
    meshList.clear();
}

void City::SetTextures(Texture* brick, Texture* dirt) {
    brickTexture = brick;
    dirtTexture = dirt;
}

void City::SetMaterials(Material* shiny, Material* dull) {
    shinyMaterial = shiny;
    dullMaterial = dull;
}

void City::calcAverageNormals(unsigned int* indices, unsigned int indiceCount,
                              GLfloat* vertices, unsigned int verticeCount,
                              unsigned int vLength, unsigned int normalOffset) {
    for (size_t i = 0; i < indiceCount; i += 3) {
        unsigned int in0 = indices[i] * vLength;
        unsigned int in1 = indices[i + 1] * vLength;
        unsigned int in2 = indices[i + 2] * vLength;
        
        glm::vec3 v1(vertices[in1] - vertices[in0], 
                     vertices[in1 + 1] - vertices[in0 + 1], 
                     vertices[in1 + 2] - vertices[in0 + 2]);
        glm::vec3 v2(vertices[in2] - vertices[in0], 
                     vertices[in2 + 1] - vertices[in0 + 1], 
                     vertices[in2 + 2] - vertices[in0 + 2]);
        glm::vec3 normal = glm::cross(v1, v2);
        normal = glm::normalize(normal);
        
        in0 += normalOffset; in1 += normalOffset; in2 += normalOffset;
        vertices[in0] += normal.x; vertices[in0 + 1] += normal.y; vertices[in0 + 2] += normal.z;
        vertices[in1] += normal.x; vertices[in1 + 1] += normal.y; vertices[in1 + 2] += normal.z;
        vertices[in2] += normal.x; vertices[in2 + 1] += normal.y; vertices[in2 + 2] += normal.z;
    }
    
    for (size_t i = 0; i < verticeCount / vLength; i++) {
        unsigned int nOffset = i * vLength + normalOffset;
        glm::vec3 vec(vertices[nOffset], vertices[nOffset + 1], vertices[nOffset + 2]);
        vec = glm::normalize(vec);
        vertices[nOffset] = vec.x; vertices[nOffset + 1] = vec.y; vertices[nOffset + 2] = vec.z;
    }
}

void City::CreateMeshes() {
    // Pyramid mesh
    unsigned int pyramidIndices[] = { 0, 3, 1, 1, 3, 2, 2, 3, 0, 0, 1, 2 };
    GLfloat pyramidVertices[] = {
        -1.0f, -1.0f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 1.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, -1.0f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f
    };
    calcAverageNormals(pyramidIndices, 12, pyramidVertices, 32, 8, 5);
    
    Mesh* pyramidMesh = new Mesh();
    pyramidMesh->CreateMesh(pyramidVertices, pyramidIndices, 32, 12);
    pyramidMeshIndex = meshList.size();
    meshList.push_back(pyramidMesh);
    
    // Floor mesh
    unsigned int floorIndices[] = { 0, 2, 1, 1, 2, 3 };
    GLfloat floorVertices[] = {
        -10.f, 0.0f, -10.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,
        10.0f, 0.0f, -10.0f, 10.0f, 0.0f, 0.0f, -1.0f, 0.0f,
        -10.0f, 0.0f, 10.0f, 0.0f, 10.0f, 0.0f, -1.0f, 0.0f,
        10.0f, 0.0f, 10.0f, 10.0f, 10.0f, 0.0f, -1.0f, 0.0f
    };
    
    Mesh* floorMesh = new Mesh();
    floorMesh->CreateMesh(floorVertices, floorIndices, 32, 6);
    floorMeshIndex = meshList.size();
    meshList.push_back(floorMesh);
    
    // Wall mesh
    unsigned int wallIndices[] = { 0, 2, 1, 1, 2, 3 };
    GLfloat wallVertices[] = {
        -10.0f, 0.0f, -10.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,
        10.0f, 0.0f, -10.0f, 10.0f, 0.0f, 0.0f, -1.0f, 0.0f,
        -10.0f, 5.0f, -10.0f, 0.0f, 10.0f, 0.0f, -1.0f, 0.0f,
        10.0f, 5.0f, -10.0f, 10.0f, 10.0f, 0.0f, -1.0f, 0.0f
    };
    
    Mesh* wallMesh = new Mesh();
    wallMesh->CreateMesh(wallVertices, wallIndices, 32, 6);
    wallMeshIndex = meshList.size();
    meshList.push_back(wallMesh);
}

void City::CreateBuildings() {
    // Rotating pyramid
    buildings.push_back({
        glm::vec3(3.0f, 0.0f, -2.5f),
        glm::vec3(0.4f, 0.4f, 0.4f),
        0.0f, // Will be animated
        pyramidMeshIndex,
        brickTexture,
        shinyMaterial
    });
    
    // Moving pyramid
    buildings.push_back({
        glm::vec3(-triOffset + 1.0f, 0.0f, -2.5f),
        glm::vec3(0.4f, 0.4f, 0.4f),
        0.0f,
        pyramidMeshIndex,
        dirtTexture,
        dullMaterial
    });
    
    // Resizing pyramid
    buildings.push_back({
        glm::vec3(-1.0f, 0.0f, -2.5f),
        glm::vec3(curSize, curSize, curSize),
        0.0f,
        pyramidMeshIndex,
        brickTexture,
        shinyMaterial
    });
    
    // Static pyramid
    buildings.push_back({
        glm::vec3(-3.0f, 0.0f, -2.5f),
        glm::vec3(0.4f, 0.4f, 0.4f),
        0.0f,
        pyramidMeshIndex,
        brickTexture,
        shinyMaterial
    });
}

void City::CreateFloorAndWalls() {
    // Floor
    buildings.push_back({
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(0.4f, 0.4f, 0.4f),
        0.0f,
        floorMeshIndex,
        dirtTexture,
        dullMaterial
    });
    
    // North wall
    buildings.push_back({
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(0.4f, 0.4f, 0.4f),
        0.0f,
        wallMeshIndex,
        brickTexture,
        dullMaterial
    });
    
    // West wall
    buildings.push_back({
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(0.4f, 0.4f, 0.4f),
        90.0f,
        wallMeshIndex,
        brickTexture,
        dullMaterial
    });
    
    // South wall
    buildings.push_back({
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(0.4f, 0.4f, 0.4f),
        180.0f,
        wallMeshIndex,
        brickTexture,
        dullMaterial
    });
    
    // East wall
    buildings.push_back({
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(0.4f, 0.4f, 0.4f),
        -90.0f,
        wallMeshIndex,
        brickTexture,
        dullMaterial
    });
}

void City::CreateCity() {
    CreateMeshes();
    CreateBuildings();
    CreateFloorAndWalls();
}

void City::RenderCity(GLuint uniformModel, GLuint uniformSpecularIntensity, GLuint uniformShininess, float deltaTime) {
    const float toRadians = 3.14159265f / 180.0f;
    
    // Update animations
    if (direction) { triOffset += triIncrement; }
    else { triOffset -= triIncrement; }
    if (abs(triOffset) >= triMaxOffset) { direction = !direction; }
    
    currAngle += 0.5f;
    if (currAngle >= 360) { currAngle = 0.0f; }
    
    if (sizeDirection) { curSize += 0.001f; }
    else { curSize -= 0.001f; }
    if (curSize >= maxSize || curSize <= minSize) { sizeDirection = !sizeDirection; }
    
    // Update dynamic buildings
    buildings[1].position.x = -triOffset + 1.0f;  // Moving pyramid
    buildings[2].scale = glm::vec3(curSize, curSize, curSize);  // Growing pyramid
    
    // Render buildings
    for (size_t i = 0; i < buildings.size(); i++) {
        const auto& building = buildings[i];
        glm::mat4 model(1.0f);
        model = glm::translate(model, building.position);
        
        // Apply rotation animation
        if (i == 0) {
            model = glm::rotate(model, currAngle * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        else if (building.rotation != 0.0f) {
            model = glm::rotate(model, building.rotation * toRadians, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        
        model = glm::scale(model, building.scale);
        
        glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
        
        if (building.material) {
            building.material->UseMaterial(uniformSpecularIntensity, uniformShininess);
        }
        
        if (building.texture) {
            building.texture->UseTexture();
        }
        
        meshList[building.meshIndex]->RenderMesh();
    }
}
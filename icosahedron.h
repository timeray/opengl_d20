#pragma once

#include <glad/gl.h>


typedef struct {
    GLfloat x, y, z;
    GLfloat r, g, b;
    //normal
    GLfloat n[3];
    // texture
    GLfloat t_x, t_y;
} Vertex;


typedef struct {
    size_t id;
    GLfloat x;
    GLfloat y;
    int neighbour1;
    int neighbour2;
} IcosahedronVertexTexturePosition;


// Store 20 icosahedron faces, 3 vertices per face
extern Vertex gIcosahedronMesh[20 * 3];

void initIcosahedronMeshFromVertices(void);
size_t getIcosahedronFaceIndex(size_t);

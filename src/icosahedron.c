#include <stdbool.h>

#include <cglm/cglm.h>

#include "icosahedron.h"

// Golden ratio (1 + sqrt(5)) / 2
#define GR 1.6180339887498948482045868343656

const GLfloat DEFAULT_VERTEX_COLOR[] = {0.8f, 0.8f, 0.8f};

static bool g_is_initialized = false;
Vertex gIcosahedronMesh[20 * 3];

// Face index to dice value
static size_t gIcosahedronFaceToValue[20] = {
    12, 2, 15, 18, 5, 
    10, 20, 8, 19, 9, 
    1, 11, 13, 3, 6, 
    16, 17, 7, 14, 4
};


// Dice value-1 to face index
static size_t gIcosahedronValueToFace[20] = {
    10, 1, 13, 19, 4, 14, 17, 7, 9, 5, 11, 0, 12, 18, 2, 15, 16, 3, 8, 6
};


// Orientation vertex for each triangle of the mesh
static size_t gIcosahedronOrientationVertexIndex[20] = {
    2, 0, 1, 2, 0, 1, 1, 2, 1, 2, 1, 0, 0, 2, 0, 0, 2, 1, 0, 2
};


// Set icosahedron vertices
// order is important for texturing
static Vertex gVertices[] = {
    // positions
    { 0.0f,  1.0f,    GR},  // 0
    {  -GR,  0.0f,  1.0f},  // 1
    {   GR,  0.0f, -1.0f},  // 2
    { 0.0f, -1.0f,   -GR},  // 3
    { 0.0f,  1.0f,   -GR},  // 4    
    {-1.0f,    GR,  0.0f},  // 5
    { 1.0f,   -GR,  0.0f},  // 6
    { 0.0f, -1.0f,    GR},  // 7
    {-1.0f,   -GR,  0.0f},  // 8
    {  -GR,  0.0f, -1.0f},  // 9
    {   GR,  0.0f,  1.0f},  // 10
    { 1.0f,    GR,  0.0f},  // 11
};


IcosahedronVertexTexturePosition gVerticesTexturePositions[] = {
    { 0, 0.4287109375f,  0.787109375f, -1, -1},
    { 1, 0.2392578125f,  0.787109375f, -1, -1},
    { 2, 0.6181640625f,  0.458984375f, -1, -1},
    { 3,    0.5234375f,  0.294921875f, -1, -1},
    { 4, 0.4287109375f,  0.458984375f, -1, -1},
    { 5,  0.333984375f,  0.623046875f, -1, -1},
    { 6,  0.712890625f,  0.294921875f, -1, -1},
    { 7,  0.333984375f, 0.9501953125f,  0,  1},
    { 7,  0.333984375f, 0.9501953125f,  0, 10},
    { 7,  0.333984375f, 0.9501953125f,  1,  8},
    { 7, 0.8076171875f, 0.1318359375f,  6,  8},
    { 7, 0.8076171875f, 0.1318359375f,  6, 10},
    { 8,   0.14453125f, 0.9501953125f,  1,  7},
    { 8,   0.14453125f, 0.9501953125f,  1,  9},
    { 8, 0.6181640625f, 0.1318359375f,  3,  6},
    { 8, 0.6181640625f, 0.1318359375f,  3,  9},
    { 8, 0.6181640625f, 0.1318359375f,  6,  7},
    { 9,   0.05078125f,  0.787109375f,  1,  8},
    { 9, 0.1455078125f,  0.623046875f,  1,  5},
    { 9,  0.240234375f,  0.458984375f,  4,  5},
    { 9, 0.3349609375f,  0.294921875f,  3,  4},
    { 9, 0.4287109375f, 0.1318359375f,  3,  8},
    {10, 0.5224609375f, 0.9501953125f,  0,  7},
    {10,    0.6171875f,  0.787109375f,  0, 11},
    {10, 0.7119140625f,  0.623046875f,  2, 11},
    {10,  0.806640625f,  0.458984375f,  2,  6},
    {10, 0.9013671875f,  0.294921875f,  6,  7},
    {11,    0.5234375f,  0.623046875f, -1, -1},
};


// Get face index (0-19) from dice value (1-20)
size_t getIcosahedronFaceIndex(size_t dice_value) {
    return gIcosahedronValueToFace[dice_value - 1];
}

size_t getOrientationVertexIndex(size_t face_index) {
    return gIcosahedronOrientationVertexIndex[face_index];
}


static GLfloat getDistance(Vertex v1, Vertex v2) {
    return sqrtf(powf(v1.x - v2.x, 2.0) + powf(v1.y - v2.y, 2.0) + powf(v1.z - v2.z, 2.0));
}


static bool almostEqual(GLfloat v1, GLfloat v2) {
    if (fabs(v1 - v2) < 0.001) {
        return true;
    }
    else {
        return false;
    }
}


void initIcosahedronMeshFromVertices(void) {
    if (g_is_initialized) {
        return;
    }

    const size_t n_vertices = sizeof(gVertices) / sizeof(Vertex);
    const size_t n_triangles = sizeof(gIcosahedronMesh) / sizeof(Vertex);

    // Setup default color
    for (size_t i = 0; i < n_vertices; ++i) {
        gVertices[i].r = DEFAULT_VERTEX_COLOR[0];
        gVertices[i].g = DEFAULT_VERTEX_COLOR[1];
        gVertices[i].b = DEFAULT_VERTEX_COLOR[2];
    }
    
    // Find mesh, normals and texture mapping
    size_t count = 0;
    size_t n = 0;
    for (size_t i = 0; i < n_vertices - 2; ++i) {
        for (size_t j = i + 1; j < n_vertices - 1; ++j) {
            for (size_t k = j + 1; k < n_vertices; ++k) {
                // All distances between vertices that form triangle in D20 should be equal to 2.0
                if (!almostEqual(getDistance(gVertices[i], gVertices[j]), 2.0f)) {
                    continue;
                }
                if (!almostEqual(getDistance(gVertices[i], gVertices[k]), 2.0f)) {
                    continue;
                }
                if (!almostEqual(getDistance(gVertices[j], gVertices[k]), 2.0f)) {
                    continue;
                }

                assert(count < n_triangles);                

                Vertex p1 = gVertices[i];
                Vertex p2 = gVertices[j];
                Vertex p3 = gVertices[k];

                // Calculate the direction of the normal of the plane relative to origin
                // to get the proper winding order (for face culling)
                vec3 v1 = { p1.x - p2.x, p1.y - p2.y, p1.z - p2.z };
                vec3 v2 = { p1.x - p3.x, p1.y - p3.y, p1.z - p3.z };

                vec3 n;
                glm_vec3_crossn(v1, v2, n);

                // Constant of the plane equation
                float c = p1.x * n[0] + p1.y * n[1] + p1.z * n[2];

                printf("[%d] ijk = %zu,%zu,%zu -> %zu,%zu,%zu\n",
                       (int)(count / 3),
                       i, j, k,
                       c > 0.0f ? i : j, c > 0.0f ? j : i, k);

                // Save original vertices to form triangle
                if (c > 0.0f) {
                    gIcosahedronMesh[count] = p1;
                    gIcosahedronMesh[count + 1] = p2;
                    gIcosahedronMesh[count + 2] = p3;
                } else {
                    gIcosahedronMesh[count] = p2;
                    gIcosahedronMesh[count + 1] = p1;
                    gIcosahedronMesh[count + 2] = p3;
                }

                // Save normal vector in each vertex of a new triangle
                // Reverse normal if it has wrong direction
                for (size_t v = 0; v < 3; ++v) {
                    for (size_t norm_i = 0; norm_i < 3; ++norm_i) {
                        gIcosahedronMesh[count + v].n[norm_i] = n[norm_i] * (c > 0.0f ? 1.0 : -1.0);
                    }
                }

                // Save texture
                // Find matching between new triangle and texture using texture array
                size_t indices[3];
                if (c > 0.0f) {
                    indices[0] = i;
                    indices[1] = j;
                    indices[2] = k;
                } else {
                    indices[0] = j;
                    indices[1] = i;
                    indices[2] = k;
                }
                int first, second;
                size_t n_vtex_positions = sizeof(gVerticesTexturePositions) / sizeof(IcosahedronVertexTexturePosition);
                for (size_t v = 0; v < 3; ++v) {
                    first = (v == 0) ? indices[1] : indices[0];
                    second = (v == 2) ? indices[1] : indices[2];
                    for (size_t t = 0; t < n_vtex_positions; ++t) {
                        IcosahedronVertexTexturePosition t_pos = gVerticesTexturePositions[t];
                        if (t_pos.id == indices[v]) {
                            // If only one position for vertex is available
                            // or neighbouring ids match
                            if ((t_pos.neighbour1 == -1)
                                || ((t_pos.neighbour1 == first) && (t_pos.neighbour2 == second))
                                || ((t_pos.neighbour1 == second) && (t_pos.neighbour2 == first))) {
                                gIcosahedronMesh[count + v].t_x = t_pos.x;
                                gIcosahedronMesh[count + v].t_y = t_pos.y;
                                break;
                            }
                        }

                        assert(t != (n_vtex_positions - 1));  // should break before that
                    }
                }

                // Offset counter by 3 vertices
                count += 3;
            }
        }
    }

    // Print the result
    for (size_t v = 0; v < 20 * 3; ++v) {
        printf("[%zu] (x=%.1f,y=%.1f,z=%.1f), (n1=%.2f,n2=%.2f,n3=%.2f), (tx=%.2f, ty=%.2f)\n",
            v, gIcosahedronMesh[v].x, gIcosahedronMesh[v].y, gIcosahedronMesh[v].z,
            gIcosahedronMesh[v].n[0], gIcosahedronMesh[v].n[1], gIcosahedronMesh[v].n[2],
            gIcosahedronMesh[v].t_x, gIcosahedronMesh[v].t_y);
    }

    g_is_initialized = true;
}

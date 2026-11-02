#include <M_Mesh.h>
#include <M_Window.h>
#include <sstream>

Vector3D M_Mesh::s_CamPos   = { 0.0f, 0.0f, -3.5f };
Vector3D M_Mesh::s_CamRight = { 1.0f, 0.0f,  0.0f };
Vector3D M_Mesh::s_CamUp    = { 0.0f, 1.0f,  0.0f };
Vector3D M_Mesh::s_CamFwd   = { 0.0f, 0.0f,  1.0f };
FLOAT    M_Mesh::s_FocalLen = 1.7320508f;
FLOAT    M_Mesh::s_NearClip = 0.1f;
FLOAT    M_Mesh::s_FarClip  = 500.0f;
FLOAT    M_Mesh::s_ProjQ    = 1.00020004f;
FLOAT    M_Mesh::s_ProjZNearQ = 0.100020004f;
UINT     M_Mesh::s_NextMeshID = 0;
Vector3D M_Mesh::s_SunDirection = Vector3D(0.45f, 0.65f, -0.60f).Normalized();
FLOAT    M_Mesh::s_AmbientIntensity = 1.0f;
std::mutex             M_Mesh::s_CacheMutex;
uMap<String, MeshData> M_Mesh::s_MeshCache;

std::mutex             M_Mesh::s_StreamMutex;
Array<M_Mesh::StreamedMeshJob> M_Mesh::s_CompletedStreamQueue;
std::atomic<UINT>      M_Mesh::s_PendingStreamCount{ 0 };
std::atomic<UINT>      M_Mesh::s_CompletedStreamCount{ 0 };

UINT M_Mesh::Next_MeshID() {
    return ++s_NextMeshID;
}

void M_Mesh::Set_CameraView(
    const Vector3D &pos,
    const Vector3D &right,
    const Vector3D &up,
    const Vector3D &forward,
    FLOAT           fovDegrees,
    FLOAT           nearClip,
    FLOAT           farClip)
{
    s_CamPos   = pos;
    s_CamRight = right;
    s_CamUp    = up;
    s_CamFwd   = forward;

    FLOAT halfFov = U_Math::Clamp_MinMax(fovDegrees, 20.0f, 140.0f) * 0.5f * U_Math::DegToRad;
    s_FocalLen    = U_Math::Radians_Cos(halfFov) / U_Math::Radians_Sin(halfFov);
    s_NearClip    = U_Math::Clamp_Min(nearClip, 0.01f);
    s_FarClip     = U_Math::Clamp_Min(farClip, s_NearClip + 1.0f);
    s_ProjQ       = s_FarClip / (s_FarClip - s_NearClip);
    s_ProjZNearQ  = s_NearClip * s_ProjQ;
}

MeshData M_Mesh::Create_Triangle(const COLOR &color) {
    return Create_Triangle(color, color, color);
}

MeshData M_Mesh::Create_Triangle(const COLOR &topColor, const COLOR &rightColor, const COLOR &leftColor) {
    MeshData mesh;
    mesh.Vertices = {
        { {  0.0f,  0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.5f, 0.0f }, topColor   }, // Top
        { {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f }, rightColor }, // Bottom Right
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f }, leftColor  }  // Bottom Left
    };
    mesh.Indices = {
        0, 1, 2, // Front face
        0, 2, 1  // Back face
    };
    mesh.MeshID = Next_MeshID();
    for (const auto &v : mesh.Vertices) mesh.LocalBounds.Encompass(v.Position);
    return mesh;
}

MeshData M_Mesh::Create_Quad(const COLOR &color) {
    MeshData mesh;
    mesh.Vertices = {
        { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f }, color }, // Top Left
        { {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f }, color }, // Top Right
        { {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f }, color }, // Bottom Right
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f }, color }  // Bottom Left
    };
    mesh.Indices = {
        0, 1, 2, 2, 3, 0, // Front face
        0, 2, 1, 2, 0, 3  // Back face
    };
    mesh.MeshID = Next_MeshID();
    for (const auto &v : mesh.Vertices) mesh.LocalBounds.Encompass(v.Position);
    return mesh;
}

MeshData M_Mesh::Create_Cube(const COLOR &color) {
    MeshData mesh;
    mesh.Vertices = {
        // Front Face (-Z)
        { { -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 0.0f }, color },
        { {  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 0.0f }, color },
        { {  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 1.0f, 1.0f }, color },
        { { -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }, { 0.0f, 1.0f }, color },

        // Back Face (+Z)
        { {  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 0.0f }, color },
        { { -0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 0.0f }, color },
        { { -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 1.0f, 1.0f }, color },
        { {  0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }, { 0.0f, 1.0f }, color },

        // Left Face (-X)
        { { -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f }, color },
        { { -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f }, color },
        { { -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f }, color },
        { { -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f }, color },

        // Right Face (+X)
        { {  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 0.0f }, color },
        { {  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 0.0f }, color },
        { {  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }, { 1.0f, 1.0f }, color },
        { {  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }, { 0.0f, 1.0f }, color },

        // Top Face (+Y)
        { { -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 0.0f }, color },
        { {  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 0.0f }, color },
        { {  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 1.0f, 1.0f }, color },
        { { -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }, { 0.0f, 1.0f }, color },

        // Bottom Face (-Y)
        { { -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 0.0f }, color },
        { {  0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 0.0f }, color },
        { {  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 1.0f, 1.0f }, color },
        { { -0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }, { 0.0f, 1.0f }, color }
    };

    mesh.Indices.reserve(36);
    for (UINT face = 0; face < 6; ++face) {
        UINT base = face * 4;
        mesh.Indices.push_back(base + 0);
        mesh.Indices.push_back(base + 1);
        mesh.Indices.push_back(base + 2);
        mesh.Indices.push_back(base + 2);
        mesh.Indices.push_back(base + 3);
        mesh.Indices.push_back(base + 0);
    }
    mesh.MeshID = Next_MeshID();
    for (const auto &v : mesh.Vertices) mesh.LocalBounds.Encompass(v.Position);
    return mesh;
}

MeshData M_Mesh::Create_Ground(FLOAT width, FLOAT depth, const COLOR &color, FLOAT uvTiling) {
    MeshData mesh;
    FLOAT hW = width * 0.5f;
    FLOAT hD = depth * 0.5f;

    mesh.Vertices = {
        { { -hW, 0.0f, -hD }, { 0.0f, 1.0f, 0.0f }, { 0.0f,     0.0f     }, color }, // Top-Left
        { {  hW, 0.0f, -hD }, { 0.0f, 1.0f, 0.0f }, { uvTiling, 0.0f     }, color }, // Top-Right
        { {  hW, 0.0f,  hD }, { 0.0f, 1.0f, 0.0f }, { uvTiling, uvTiling }, color }, // Bottom-Right
        { { -hW, 0.0f,  hD }, { 0.0f, 1.0f, 0.0f }, { 0.0f,     uvTiling }, color }  // Bottom-Left
    };

    mesh.Indices = {
        0, 1, 2, 2, 3, 0, // Front (+Y facing up)
        0, 2, 1, 2, 0, 3  // Back (-Y facing down)
    };

    mesh.MeshID = Next_MeshID();
    for (const auto &v : mesh.Vertices) mesh.LocalBounds.Encompass(v.Position);
    return mesh;
}

MeshData M_Mesh::Create(PrimitiveType type, const COLOR &color) {
    switch (type) {
        case Triangle: return Create_Triangle(color);
        case Quad:     return Create_Quad(color);
        case Cube:
        default:       return Create_Cube(color);
    }
}

MeshData M_Mesh::Load_OBJ(const FilePath &path, const COLOR &defaultColor) {
    String key = path.string();
    {
        std::lock_guard<std::mutex> lock(s_CacheMutex);
        auto it = s_MeshCache.find(key);
        if (it != s_MeshCache.end()) {
            return it->second;
        }
    }

    if (!M_File::Find(path)) {
        LOG(Error, LogLevel::Developer, "M_Mesh::Load_OBJ -> File [{}] not found! Returning empty mesh.", key);
        return MeshData{};
    }

    String text = M_File::Open(path);
    if (text.empty()) {
        LOG(Error, LogLevel::Developer, "M_Mesh::Load_OBJ -> File [{}] is empty! Returning empty mesh.", key);
        return MeshData{};
    }

    Array<Vector3D> positions;
    Array<Vector3D> normals;
    Array<Vector2D> uvs;
    MeshData        mesh;

    positions.reserve(256);
    normals.reserve(256);
    uvs.reserve(256);

    std::istringstream stream(text);
    String line;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ls(line);
        String prefix;
        ls >> prefix;

        if (prefix == "v") {
            Vector3D p{};
            ls >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (prefix == "vt") {
            Vector2D uv{};
            ls >> uv.x >> uv.y;
            uv.y = 1.0f - uv.y; // Convert OpenGL bottom-left V to top-left UV
            uvs.push_back(uv);
        } else if (prefix == "vn") {
            Vector3D n{};
            ls >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (prefix == "f") {
            Array<UINT> faceIndices;
            String vertToken;
            while (ls >> vertToken) {
                INT vIdx = 0, vtIdx = 0, vnIdx = 0;
                size_t s1 = vertToken.find('/');
                if (s1 == String::npos) {
                    vIdx = std::atoi(vertToken.c_str());
                } else {
                    vIdx = std::atoi(vertToken.substr(0, s1).c_str());
                    size_t s2 = vertToken.find('/', s1 + 1);
                    if (s2 == String::npos) {
                        vtIdx = std::atoi(vertToken.substr(s1 + 1).c_str());
                    } else {
                        if (s2 > s1 + 1) vtIdx = std::atoi(vertToken.substr(s1 + 1, s2 - s1 - 1).c_str());
                        vnIdx = std::atoi(vertToken.substr(s2 + 1).c_str());
                    }
                }

                // Handle OBJ 1-based and relative negative indices (used by Blender exports)
                if (vIdx < 0)  vIdx  = static_cast<INT>(positions.size()) + vIdx + 1;
                if (vtIdx < 0) vtIdx = static_cast<INT>(uvs.size()) + vtIdx + 1;
                if (vnIdx < 0) vnIdx = static_cast<INT>(normals.size()) + vnIdx + 1;

                Vertex vert{};
                vert.Color = defaultColor;
                if (vIdx > 0 && static_cast<size_t>(vIdx) <= positions.size()) vert.Position = positions[vIdx - 1];
                if (vtIdx > 0 && static_cast<size_t>(vtIdx) <= uvs.size())     vert.UV       = uvs[vtIdx - 1];
                if (vnIdx > 0 && static_cast<size_t>(vnIdx) <= normals.size()) vert.Normal   = normals[vnIdx - 1];

                faceIndices.push_back(static_cast<UINT>(mesh.Vertices.size()));
                mesh.Vertices.push_back(vert);
            }

            // Fan-triangulate N-gon face (works for triangles, quads, etc.)
            for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
                mesh.Indices.push_back(faceIndices[0]);
                mesh.Indices.push_back(faceIndices[i]);
                mesh.Indices.push_back(faceIndices[i + 1]);
            }
        }
    }

    if (!mesh.Is_Valid()) {
        LOG(Error, LogLevel::Developer, "M_Mesh::Load_OBJ -> [{}] had no valid geometry! Returning empty mesh.", key);
        return MeshData{};
    }

    FLOAT maxDistSq = 0.0f;
    for (const auto &v : mesh.Vertices) {
        FLOAT distSq = v.Position.x * v.Position.x + v.Position.y * v.Position.y + v.Position.z * v.Position.z;
        if (distSq > maxDistSq) maxDistSq = distSq;
        mesh.LocalBounds.Encompass(v.Position);
    }
    mesh.BoundingRadius = (maxDistSq > 0.0f) ? std::sqrt(maxDistSq) : 1.0f;
    mesh.FilePath       = key;

    mesh.MeshID = Next_MeshID();
    {
        std::lock_guard<std::mutex> lock(s_CacheMutex);
        s_MeshCache[key] = mesh;
    }

    LOG(Info, LogLevel::Core, "M_Mesh::Load_OBJ -> Loaded [{}] ({} Vertices, {} Triangles)",
        key, mesh.Get_VertexCount(), mesh.Get_TriangleCount());
    return mesh;
}

MeshData M_Mesh::Load(const FilePath &path, const COLOR &defaultColor) {
    String key = path.string();
    {
        std::lock_guard<std::mutex> lock(s_CacheMutex);
        auto it = s_MeshCache.find(key);
        if (it != s_MeshCache.end()) {
            return it->second;
        }
    }

    MeshData mesh = Load_OBJ(path, defaultColor);

    if (mesh.Is_Valid()) {
        std::lock_guard<std::mutex> lock(s_CacheMutex);
        s_MeshCache[key] = mesh;
    }
    return mesh;
}

void M_Mesh::Stream(
    const FilePath &path,
    Function<void(MeshData)> onLoaded,
    const COLOR &defaultColor)
{
    String key = path.string();

    // 1. Instant O(1) cache lookup if already parsed & cached
    {
        std::lock_guard<std::mutex> lock(s_CacheMutex);
        auto it = s_MeshCache.find(key);
        if (it != s_MeshCache.end()) {
            MeshData cached = it->second;
            {
                std::lock_guard<std::mutex> sLock(s_StreamMutex);
                s_CompletedStreamQueue.push_back({ key, std::move(cached), std::move(onLoaded) });
            }
            ++s_CompletedStreamCount;
            return;
        }
    }

    // 2. Schedule background worker thread to read & parse file without stalling the main thread
    ++s_PendingStreamCount;
    M_Threads::Run([path, key, onLoaded, defaultColor]() {
        MeshData loaded = Load(path, defaultColor);
        {
            std::lock_guard<std::mutex> lock(s_StreamMutex);
            s_CompletedStreamQueue.push_back({ key, std::move(loaded), std::move(onLoaded) });
        }
        ++s_CompletedStreamCount;
        if (s_PendingStreamCount > 0) {
            --s_PendingStreamCount;
        }
    });
}

void M_Mesh::Tick_Streaming(UINT budgetPerTick) {
    if (budgetPerTick == 0) return;
    if (s_CompletedStreamCount.load(std::memory_order_relaxed) == 0) return;

    Array<StreamedMeshJob> jobsToDispatch;
    {
        std::lock_guard<std::mutex> lock(s_StreamMutex);
        if (s_CompletedStreamQueue.empty()) return;

        size_t count = std::min(static_cast<size_t>(budgetPerTick), s_CompletedStreamQueue.size());
        jobsToDispatch.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            jobsToDispatch.push_back(std::move(s_CompletedStreamQueue[i]));
        }
        s_CompletedStreamQueue.erase(s_CompletedStreamQueue.begin(), s_CompletedStreamQueue.begin() + count);
        s_CompletedStreamCount.store(static_cast<UINT>(s_CompletedStreamQueue.size()), std::memory_order_relaxed);
    }

    for (auto &job : jobsToDispatch) {
        if (job.Callback) {
            job.Callback(std::move(job.Mesh));
        }
    }
}

void M_Mesh::Shutdown() {
    {
        std::lock_guard<std::mutex> lock(s_StreamMutex);
        s_CompletedStreamQueue.clear();
        s_PendingStreamCount = 0;
    }
    {
        std::lock_guard<std::mutex> lock(s_CacheMutex);
        s_MeshCache.clear();
    }
}

DrawConstants M_Mesh::Build_DrawConstants(
    const Vector3D &pos,
    const Vector3D &rotDeg,
    const Vector3D &size,
    const COLOR    &color,
    FLOAT           transparency,
    FLOAT           aspectRatio,
    BOOL            bFlipClipY,
    BOOL            bEnable3DShading)
{
    DrawConstants dc{};

    FLOAT mv00, mv10, mv20;
    FLOAT mv01, mv11, mv21;
    FLOAT mv02, mv12, mv22;
    FLOAT r00 = 1.0f, r10 = 0.0f, r20 = 0.0f;
    FLOAT r01 = 0.0f, r11 = 1.0f, r21 = 0.0f;
    FLOAT r02 = 0.0f, r12 = 0.0f, r22 = 1.0f;

    if (rotDeg.x == 0.0f && rotDeg.y == 0.0f && rotDeg.z == 0.0f) {
        // Fast Path: Unrotated identity matrix (skips 6 trig calls & 36 multiplications)
        mv00 = s_CamRight.x * size.x; mv10 = s_CamUp.x * size.x; mv20 = s_CamFwd.x * size.x;
        mv01 = s_CamRight.y * size.y; mv11 = s_CamUp.y * size.y; mv21 = s_CamFwd.y * size.y;
        mv02 = s_CamRight.z * size.z; mv12 = s_CamUp.z * size.z; mv22 = s_CamFwd.z * size.z;
    } else {
        // 1. Rotation angles (Pitch = X, Yaw = Y, Roll = Z) in radians
        FLOAT rx = rotDeg.x * U_Math::DegToRad;
        FLOAT ry = rotDeg.y * U_Math::DegToRad;
        FLOAT rz = rotDeg.z * U_Math::DegToRad;

        FLOAT cx = U_Math::Radians_Cos(rx), sx = U_Math::Radians_Sin(rx);
        FLOAT cy = U_Math::Radians_Cos(ry), sy = U_Math::Radians_Sin(ry);
        FLOAT cz = U_Math::Radians_Cos(rz), sz = U_Math::Radians_Sin(rz);

        // Rotation matrix R = Ry * Rx * Rz (3x3 basis columns)
        r00 = cy * cz + sy * sx * sz;
        r10 = cx * sz;
        r20 = -sy * cz + cy * sx * sz;

        r01 = -cy * sz + sy * sx * cz;
        r11 = cx * cz;
        r21 = sy * sz + cy * sx * cz;

        r02 = sy * cx;
        r12 = -sx;
        r22 = cy * cx;

        // 2. World Model columns (M = R * Scale) & Camera View Transform (from active E_Camera)
        FLOAT m00 = r00 * size.x, m10 = r10 * size.x, m20 = r20 * size.x;
        FLOAT m01 = r01 * size.y, m11 = r11 * size.y, m21 = r21 * size.y;
        FLOAT m02 = r02 * size.z, m12 = r12 * size.z, m22 = r22 * size.z;

        // Model-View = View * Model (project world columns onto cached Camera Right, Up, Forward)
        mv00 = s_CamRight.x * m00 + s_CamRight.y * m10 + s_CamRight.z * m20;
        mv10 = s_CamUp.x    * m00 + s_CamUp.y    * m10 + s_CamUp.z    * m20;
        mv20 = s_CamFwd.x   * m00 + s_CamFwd.y   * m10 + s_CamFwd.z   * m20;

        mv01 = s_CamRight.x * m01 + s_CamRight.y * m11 + s_CamRight.z * m21;
        mv11 = s_CamUp.x    * m01 + s_CamUp.y    * m11 + s_CamUp.z    * m21;
        mv21 = s_CamFwd.x   * m01 + s_CamFwd.y   * m11 + s_CamFwd.z   * m21;

        mv02 = s_CamRight.x * m02 + s_CamRight.y * m12 + s_CamRight.z * m22;
        mv12 = s_CamUp.x    * m02 + s_CamUp.y    * m12 + s_CamUp.z    * m22;
        mv22 = s_CamFwd.x   * m02 + s_CamFwd.y   * m12 + s_CamFwd.z   * m22;
    }

    FLOAT relX = pos.x - s_CamPos.x;
    FLOAT relY = pos.y - s_CamPos.y;
    FLOAT relZ = pos.z - s_CamPos.z;

    FLOAT mv03 = s_CamRight.x * relX + s_CamRight.y * relY + s_CamRight.z * relZ;
    FLOAT mv13 = s_CamUp.x    * relX + s_CamUp.y    * relY + s_CamUp.z    * relZ;
    FLOAT mv23 = s_CamFwd.x   * relX + s_CamFwd.y   * relY + s_CamFwd.z   * relZ;

    // 3. Perspective Projection (from precomputed camera projection parameters, [0, 1] depth)
    if (aspectRatio <= 0.001f) aspectRatio = 16.0f / 9.0f;
    FLOAT xScale = s_FocalLen / aspectRatio;
    FLOAT yScale = bFlipClipY ? -s_FocalLen : s_FocalLen;
    FLOAT q      = s_ProjQ;
    FLOAT znq    = s_ProjZNearQ;

    // Column-major 4x4 MVP = Proj * ModelView
    dc.MVP[0]  = mv00 * xScale;
    dc.MVP[1]  = mv10 * yScale;
    dc.MVP[2]  = mv20 * q;
    dc.MVP[3]  = mv20;

    dc.MVP[4]  = mv01 * xScale;
    dc.MVP[5]  = mv11 * yScale;
    dc.MVP[6]  = mv21 * q;
    dc.MVP[7]  = mv21;

    dc.MVP[8]  = mv02 * xScale;
    dc.MVP[9]  = mv12 * yScale;
    dc.MVP[10] = mv22 * q;
    dc.MVP[11] = mv22;

    dc.MVP[12] = mv03 * xScale;
    dc.MVP[13] = mv13 * yScale;
    dc.MVP[14] = mv23 * q - znq;
    dc.MVP[15] = mv23;

    // 4. Tint & Alpha Transparency (multiplications instead of float divisions)
    constexpr FLOAT INV_255 = 1.0f / 255.0f;
    FLOAT clampedAlpha = U_Math::Clamp_MinMax(transparency, 0.0f, 1.0f);
    dc.Tint[0] = static_cast<FLOAT>(color.r) * INV_255;
    dc.Tint[1] = static_cast<FLOAT>(color.g) * INV_255;
    dc.Tint[2] = static_cast<FLOAT>(color.b) * INV_255;
    dc.Tint[3] = (static_cast<FLOAT>(color.a) * INV_255) * clampedAlpha;

    // 5. Transform directional light into local model space (R^T * L_world)
    const FLOAT lx = s_SunDirection.x, ly = s_SunDirection.y, lz = s_SunDirection.z;
    dc.Light[0] = r00 * lx + r10 * ly + r20 * lz;
    dc.Light[1] = r01 * lx + r11 * ly + r21 * lz;
    dc.Light[2] = r02 * lx + r12 * ly + r22 * lz;
    dc.Light[3] = bEnable3DShading ? s_AmbientIntensity : 0.0f;

    return dc;
}

DrawConstants M_Mesh::Build_UIConstants(
    const Vector2D &pos,
    const Vector2D &size,
    const COLOR    &color,
    FLOAT           transparency,
    BOOL            bFlipClipY)
{
    DrawConstants dc{};
    Vector2D px = M_Window::Get_PixelSize();
    FLOAT screenW = px.x > 0.0f ? px.x : 1280.0f;
    FLOAT screenH = px.y > 0.0f ? px.y : 720.0f;

    FLOAT cx = pos.x + size.x * 0.5f;
    FLOAT cy = pos.y + size.y * 0.5f;

    FLOAT scaleX = (size.x / screenW) * 2.0f;
    FLOAT scaleY = (size.y / screenH) * 2.0f;

    FLOAT transX = (cx / screenW) * 2.0f - 1.0f;
    FLOAT transY = bFlipClipY ? ((cy / screenH) * 2.0f - 1.0f) : (1.0f - (cy / screenH) * 2.0f);

    dc.MVP[0]  = scaleX;
    dc.MVP[5]  = bFlipClipY ? -scaleY : scaleY;
    dc.MVP[10] = 0.0f;
    dc.MVP[12] = transX;
    dc.MVP[13] = transY;
    dc.MVP[15] = 1.0f;

    constexpr FLOAT INV_255 = 1.0f / 255.0f;
    FLOAT clampedAlpha = U_Math::Clamp_MinMax(transparency, 0.0f, 1.0f);
    dc.Tint[0] = static_cast<FLOAT>(color.r) * INV_255;
    dc.Tint[1] = static_cast<FLOAT>(color.g) * INV_255;
    dc.Tint[2] = static_cast<FLOAT>(color.b) * INV_255;
    dc.Tint[3] = (static_cast<FLOAT>(color.a) * INV_255) * clampedAlpha;

    dc.Light[0] = 1.0f;
    dc.Light[1] = 1.0f;
    dc.Light[2] = 1.0f;
    dc.Light[3] = 0.0f; // Unlit 2D

    return dc;
}

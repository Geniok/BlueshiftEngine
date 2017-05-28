﻿// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Render/Render.h"
#include "RenderInternal.h"

BE_NAMESPACE_BEGIN

ParticleMesh::ParticleMesh() {
    totalVerts = 0;
    totalIndexes = 0;
}

void ParticleMesh::Clear() {
    totalVerts = 0;
    totalIndexes = 0;

    surfaces.SetCount(0, false);

    PrepareNextSurf();
}

void ParticleMesh::PrepareNextSurf() {
    PrtMeshSurf newSurf;

    newSurf.numVerts = 0;
    newSurf.numIndexes = 0;

    memset(&newSurf.vertexCache, 0, sizeof(newSurf.vertexCache));
    memset(&newSurf.indexCache, 0, sizeof(newSurf.indexCache));

    if (surfaces.Count() > 0) {
        newSurf.material = currentSurf->material;
    } else {
        newSurf.material = materialManager.defaultMaterial;
    }

    surfaces.Append(newSurf);
    currentSurf = &surfaces.Last();
}

void ParticleMesh::DrawQuad(const VertexGeneric *verts, const Material *material) {
    if (!verts || !material) {
        return;
    }

    if (material != currentSurf->material) {
        if (currentSurf->numVerts > 0) {
            PrepareNextSurf();
        }
        currentSurf->material = material;
    }

    totalVerts += 4;
    totalIndexes += 6;

    currentSurf->numVerts += 4;
    currentSurf->numIndexes += 6;

    // Cache 4 vertices in the dynamic vertex buffer
    BufferCache vertexCache;
    bufferCacheManager.AllocVertex(4, sizeof(VertexGeneric), verts, &vertexCache);

    if (!bufferCacheManager.IsCached(&currentSurf->vertexCache)) {
        currentSurf->vertexCache = vertexCache;
    } else {
        currentSurf->vertexCache.bytes += vertexCache.bytes;
    }
}

int ParticleMesh::CountDrawingVerts(const ParticleSystem::Stage &stage, const Particle *stageParticles) const {
    int numVerts = 0;
    
    int trailCount = (stage.moduleFlags & BIT(ParticleSystem::TrailsModuleBit)) ? stage.trailsModule.count : 0;

    for (int particleIndex = 0; particleIndex < stage.standardModule.count; particleIndex++) {
        int particleSize = sizeof(Particle) + sizeof(Particle::Trail) * trailCount;
        Particle *particle = (Particle *)((byte *)stageParticles + particleIndex * particleSize);
        
        if (particle->alive) {
            if (stage.standardModule.orientation == ParticleSystem::StandardModule::Aimed ||
                stage.standardModule.orientation == ParticleSystem::StandardModule::AimedZ) {
                numVerts += 4 * (trailCount);
            } else {
                numVerts += 4 * (1 + trailCount);
            }
        }
    }

    return numVerts;
}

static Mat3 ComputeParticleAxis(ParticleSystem::StandardModule::Orientation orientation, const Mat3 &modelAxis, const Mat3 &viewAxis) {
    Mat3 worldAxis; // forward, left, up

    switch (orientation) {
    case ParticleSystem::StandardModule::View:
        worldAxis[0] = -viewAxis[0];
        worldAxis[1] = -viewAxis[1];
        worldAxis[2] = viewAxis[2];
        break;
    case ParticleSystem::StandardModule::ViewZ:
        worldAxis[0] = Vec3(-viewAxis[1].y, viewAxis[1].x, 0); // = FromCross(-viewAxis[1], unitZ)
        worldAxis[1] = -viewAxis[1];
        worldAxis[2] = Vec3::unitZ;
        break;
    case ParticleSystem::StandardModule::X:
        worldAxis[0] = Vec3::unitX;
        worldAxis[1] = Vec3::unitY;
        worldAxis[2] = Vec3::unitZ;
        break;
    case ParticleSystem::StandardModule::Y:
        worldAxis[0] = Vec3::unitY;
        worldAxis[1] = -Vec3::unitX;
        worldAxis[2] = Vec3::unitZ;
        break;
    case ParticleSystem::StandardModule::Z:
        worldAxis[0] = Vec3::unitZ;
        worldAxis[1] = Vec3::unitY;
        worldAxis[2] = -Vec3::unitX;
        break;
    default:
        assert(0);
        break;
    }

    return modelAxis.TransposedMul(worldAxis);
}

void ParticleMesh::Draw(const ParticleSystem *particleSystem, const Array<Particle *> &stageParticles, const SceneEntity *entity, const SceneView *view) {
    Vec3 worldPos[Particle::MaxTrails + 1];
    Vec3 cameraDir[Particle::MaxTrails + 1];
    Vec3 tangentDir[Particle::MaxTrails + 1];
    Mat3 localAxis;
    Vec3 rtv, upv;

    for (int stageIndex = 0; stageIndex < particleSystem->stages.Count(); stageIndex++) {
        const ParticleSystem::Stage &stage = particleSystem->stages[stageIndex];

        if (stage.skipRender) {
            continue;
        }

        int numVerts = CountDrawingVerts(stage, stageParticles[stageIndex]);
        if (numVerts > 0) {
            // number of indices for the quad that consist of two triangles
            int numIndexes = numVerts * 3 / 2;

            //if (stage.standardModule.material != currentSurf->material) { // FIXME
                if (currentSurf->numVerts > 0) {
                    PrepareNextSurf();
                }
                currentSurf->material = stage.standardModule.material;
            //}

            totalVerts += numVerts;
            totalIndexes += numIndexes;

            currentSurf->numVerts += numVerts;
            currentSurf->numIndexes += numIndexes;

            float s1 = 0.0f;
            float t1 = 0.0f;
            float s2 = 1.0f;
            float t2 = 1.0f;

            float16_t hs1 = F32toF16(s1);
            float16_t ht1 = F32toF16(t1);
            float16_t hs2 = F32toF16(s2);
            float16_t ht2 = F32toF16(t2);

            if (stage.standardModule.orientation != ParticleSystem::StandardModule::Aimed &&
                stage.standardModule.orientation != ParticleSystem::StandardModule::AimedZ) {
                localAxis = ComputeParticleAxis(stage.standardModule.orientation, entity->parms.axis, view->parms.axis);
            }

            // Cache vertices
            BufferCache vertexCache;
            bufferCacheManager.AllocVertex(numVerts, sizeof(VertexGeneric), nullptr, &vertexCache);
            VertexGeneric *vertexPointer = (VertexGeneric *)bufferCacheManager.MapVertexBuffer(&vertexCache);
           
            int trailCount = (stage.moduleFlags & BIT(ParticleSystem::TrailsModuleBit)) ? stage.trailsModule.count : 0;

            for (int particleIndex = 0; particleIndex < stage.standardModule.count; particleIndex++) {
                int pivotCount = trailCount + 1;

                int particleSize = sizeof(Particle) + sizeof(Particle::Trail) * trailCount;
                Particle *particle = (Particle *)((byte *)stageParticles[stageIndex] + particleIndex * particleSize);

                if (!particle->alive) {
                    continue;
                }

                uint32_t color = particle->trails[0].color.ToUInt32();

                if (stage.standardModule.orientation == ParticleSystem::StandardModule::Aimed ||
                    stage.standardModule.orientation == ParticleSystem::StandardModule::AimedZ) {
                    // Compute world position of all particle pivots including trails
                    for (int pivotIndex = 0; pivotIndex < pivotCount; pivotIndex++) {
                        const Particle::Trail *trail = &particle->trails[pivotIndex];

                        worldPos[pivotIndex] = entity->modelMatrix * trail->position;
                    }

                    // Compute cameraDir/tangentDir of all particle pivots including trails
                    for (int pivotIndex = 0; pivotIndex < pivotCount; pivotIndex++) {
                        const Particle::Trail *trail = &particle->trails[pivotIndex];

                        if (pivotIndex == 0) {
                            cameraDir[pivotIndex] = view->parms.origin - (worldPos[pivotIndex + 1] + worldPos[pivotIndex]) * 0.5f;
                            tangentDir[pivotIndex] = worldPos[pivotIndex + 1] - worldPos[pivotIndex];
                        } else if (pivotIndex == trailCount) {
                            cameraDir[pivotIndex] = view->parms.origin - (worldPos[pivotIndex] + worldPos[pivotIndex - 1]) * 0.5f;
                            tangentDir[pivotIndex] = worldPos[pivotIndex] - worldPos[pivotIndex - 1];
                        } else {
                            cameraDir[pivotIndex] = view->parms.origin - worldPos[pivotIndex];
                            tangentDir[pivotIndex] = worldPos[pivotIndex + 1] - worldPos[pivotIndex - 1];
                        }

                        if (stage.standardModule.orientation == ParticleSystem::StandardModule::AimedZ) {
                            cameraDir[pivotIndex].x = 0;
                            cameraDir[pivotIndex].y = 0;
                        }

                        cameraDir[pivotIndex].Normalize();
                        tangentDir[pivotIndex].Normalize();
                    }

                    for (int quadIndex = 0; quadIndex < trailCount; quadIndex++) {
                        const Particle::Trail *trail = &particle->trails[quadIndex];
                        
                        ht1 = F32toF16((float)quadIndex / trailCount);
                        ht2 = F32toF16((float)(quadIndex + 1) / trailCount);

                        rtv.SetFromCross(cameraDir[quadIndex], tangentDir[quadIndex]);
                        rtv.Normalize();
                        rtv = entity->parms.axis.TransposedMulVec(rtv);
                        rtv *= particle->trails[quadIndex].size * 0.5f;

                        vertexPointer->xyz = trail[0].position - rtv;
                        vertexPointer->st[0] = hs1;
                        vertexPointer->st[1] = ht1;
                        *reinterpret_cast<uint32_t *>(vertexPointer->color) = color;
                        vertexPointer++;

                        vertexPointer->xyz = trail[0].position + rtv;
                        vertexPointer->st[0] = hs2;
                        vertexPointer->st[1] = ht1;
                        *reinterpret_cast<uint32_t *>(vertexPointer->color) = color;
                        vertexPointer++;

                        rtv.SetFromCross(cameraDir[quadIndex + 1], tangentDir[quadIndex + 1]);
                        rtv.Normalize();
                        rtv = entity->parms.axis.TransposedMulVec(rtv);
                        rtv *= particle->trails[quadIndex + 1].size * 0.5f;

                        vertexPointer->xyz = trail[1].position - rtv;
                        vertexPointer->st[0] = hs1;
                        vertexPointer->st[1] = ht2;
                        *reinterpret_cast<uint32_t *>(vertexPointer->color) = color;
                        vertexPointer++;

                        vertexPointer->xyz = trail[1].position + rtv;
                        vertexPointer->st[0] = hs2;
                        vertexPointer->st[1] = ht2;
                        *reinterpret_cast<uint32_t *>(vertexPointer->color) = color;
                        vertexPointer++;
                    }
                } else {
                    for (int quadIndex = 0; quadIndex < pivotCount; quadIndex++) {
                        const Particle::Trail *trail = &particle->trails[quadIndex];
                        
                        Vec3 rt = localAxis[1];
                        Vec3 up = localAxis[2];

                        if (trail->angle != 0) {
                            Rotation rotation(Vec3::origin, localAxis[0], trail->angle);
                            rt = rotation.RotatePoint(rt);
                            up = rotation.RotatePoint(up);
                        }

                        const float halfSize = trail->size * 0.5f;

                        rtv = rt * halfSize * trail->aspectRatio;
                        upv = up * halfSize;

                        vertexPointer->xyz = trail->position + upv - rtv;
                        vertexPointer->st[0] = hs1;
                        vertexPointer->st[1] = ht1;
                        *reinterpret_cast<uint32_t *>(vertexPointer->color) = color;
                        vertexPointer++;

                        vertexPointer->xyz = trail->position + upv + rtv;
                        vertexPointer->st[0] = hs2;
                        vertexPointer->st[1] = ht1;
                        *reinterpret_cast<uint32_t *>(vertexPointer->color) = color;
                        vertexPointer++;

                        vertexPointer->xyz = trail->position - upv - rtv;
                        vertexPointer->st[0] = hs1;
                        vertexPointer->st[1] = ht2;
                        *reinterpret_cast<uint32_t *>(vertexPointer->color) = color;
                        vertexPointer++;

                        vertexPointer->xyz = trail->position - upv + rtv;
                        vertexPointer->st[0] = hs2;
                        vertexPointer->st[1] = ht2;
                        *reinterpret_cast<uint32_t *>(vertexPointer->color) = color;
                        vertexPointer++;
                    }
                }
            }

            bufferCacheManager.UnmapVertexBuffer(&vertexCache);

            currentSurf->vertexCache = vertexCache;
        }
    }
}

void ParticleMesh::CacheIndexes() {
    if (totalIndexes <= 0) {
        return;
    }

    assert(totalIndexes % 6 == 0);
    static const TriIndex quadTrisIndexes[6] = { 0, 2, 1, 1, 2, 3 };

    // Cache all indices in the dynamic index buffer
    BufferCache indexCache;
    bufferCacheManager.AllocIndex(totalIndexes, sizeof(TriIndex), nullptr, &indexCache);
    TriIndex *indexPointer = (TriIndex *)bufferCacheManager.MapIndexBuffer(&indexCache);

    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        PrtMeshSurf *surf = &surfaces[surfaceIndex];

        int startIndex = surf->vertexCache.offset / sizeof(VertexGeneric);

        for (int index = 0; index < surf->numIndexes; index += 6) {
            *indexPointer++ = startIndex + quadTrisIndexes[0];
            *indexPointer++ = startIndex + quadTrisIndexes[1];
            *indexPointer++ = startIndex + quadTrisIndexes[2];
            *indexPointer++ = startIndex + quadTrisIndexes[3];
            *indexPointer++ = startIndex + quadTrisIndexes[4];
            *indexPointer++ = startIndex + quadTrisIndexes[5];

            startIndex += 4;
        }
    }
    bufferCacheManager.UnmapIndexBuffer(&indexCache);

    // Set index cache info for each surfaces
    int offset = indexCache.offset;
    for (int surfaceIndex = 0; surfaceIndex < surfaces.Count(); surfaceIndex++) {
        PrtMeshSurf *surf = &surfaces[surfaceIndex];

        surf->indexCache = indexCache;
        surf->indexCache.offset = offset;
        surf->indexCache.bytes = sizeof(TriIndex) * surf->numIndexes;

        offset += surf->indexCache.bytes;
    }
}

BE_NAMESPACE_END
#include "physics-system.hpp"
#include "../components/collider.hpp"
#include "../components/rigidbody.hpp"
#include "../ecs/transform.hpp"
#include "jolt-debug-renderer.hpp"
#include <glm/gtc/quaternion.hpp>

// Jolt Headers
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>

// Assimp for mesh loading (for mesh colliders)
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>

namespace our {
    // Class that determines if an object layer can collide with a broadphase layer
    class PhysicsSystem::ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
            // NON_MOVING collides with PLAYER and ENEMY (not with attacks or other static)
            if (inLayer1 == Layers::NON_MOVING) {
                return inLayer2 == JPH::BroadPhaseLayer(Layers::PLAYER) || 
                       inLayer2 == JPH::BroadPhaseLayer(Layers::ENEMY);
            }
            // PLAYER collides with NON_MOVING and ENEMY (NOT with PLAYER_ATTACK - own bullets)
            if (inLayer1 == Layers::PLAYER) {
                return inLayer2 == JPH::BroadPhaseLayer(Layers::NON_MOVING) || 
                       inLayer2 == JPH::BroadPhaseLayer(Layers::ENEMY);
            }
            // PLAYER_ATTACK collides only with ENEMY and NON_MOVING (walls)
            if (inLayer1 == Layers::PLAYER_ATTACK) {
                return inLayer2 == JPH::BroadPhaseLayer(Layers::ENEMY) ||
                       inLayer2 == JPH::BroadPhaseLayer(Layers::NON_MOVING);
            }
            // ENEMY collides with everything except other enemies
            if (inLayer1 == Layers::ENEMY) {
                return inLayer2 != JPH::BroadPhaseLayer(Layers::ENEMY);
            }
            return false;
        }
    };

    // Class that determines if two object layers can collide
    class PhysicsSystem::ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
            // NON_MOVING (walls/ground) collides with PLAYER and ENEMY
            if (inObject1 == Layers::NON_MOVING) {
                return inObject2 == Layers::PLAYER || inObject2 == Layers::ENEMY;
            }
            // PLAYER collides with NON_MOVING and ENEMY (NOT with own attacks)
            if (inObject1 == Layers::PLAYER) {
                return inObject2 == Layers::NON_MOVING || inObject2 == Layers::ENEMY;
            }
            // PLAYER_ATTACK (bullets) collides only with ENEMY and walls
            if (inObject1 == Layers::PLAYER_ATTACK) {
                return inObject2 == Layers::ENEMY || inObject2 == Layers::NON_MOVING;
            }
            // ENEMY collides with PLAYER, PLAYER_ATTACK, and NON_MOVING (not other enemies)
            if (inObject1 == Layers::ENEMY) {
                return inObject2 == Layers::PLAYER || 
                       inObject2 == Layers::PLAYER_ATTACK || 
                       inObject2 == Layers::NON_MOVING;
            }
            return false;
        }
    };

    // Class that maps object layers to broadphase layers
    class PhysicsSystem::BPLayerInterfaceImpl : public JPH::BroadPhaseLayerInterface {
        JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
    public:
        BPLayerInterfaceImpl() {
            mObjectToBroadPhase[Layers::NON_MOVING] = JPH::BroadPhaseLayer(Layers::NON_MOVING);
            mObjectToBroadPhase[Layers::PLAYER] = JPH::BroadPhaseLayer(Layers::PLAYER);
            mObjectToBroadPhase[Layers::PLAYER_ATTACK] = JPH::BroadPhaseLayer(Layers::PLAYER_ATTACK);
            mObjectToBroadPhase[Layers::ENEMY] = JPH::BroadPhaseLayer(Layers::ENEMY);
        }
        virtual JPH::uint GetNumBroadPhaseLayers() const override { return Layers::NUM_LAYERS; }
        virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
            return mObjectToBroadPhase[inLayer];
        }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
        virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override { return "Layer"; }
#endif
    };

    // CONTACT LISTENER (For collision/trigger detection)
    class PhysicsSystem::ContactListenerImpl : public JPH::ContactListener {
    public:
        virtual JPH::ValidateResult OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, 
            JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override {
            // Allow all contacts
            return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
        }

        virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, 
            const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override {
            std::cout << "Collision: Body " << inBody1.GetID().GetIndex() << " hit Body " << inBody2.GetID().GetIndex() << std::endl;
        }

        virtual void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, 
            const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override {
            // Contact is continuing
        }

        virtual void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override {
            // Contact ended
        }
    };

    // Helper function to create mesh shape from file using Assimp
    JPH::Ref<JPH::ShapeSettings> PhysicsSystem::createMeshShape(const std::string& meshPath, const JPH::Vec3& scale, const glm::quat& rotation) {
        Assimp::Importer importer;
        
        const aiScene* scene = importer.ReadFile(meshPath,
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_OptimizeMeshes |
            aiProcess_OptimizeGraph |
            aiProcess_PreTransformVertices  // Bake FBX node transforms into vertices (matches visual mesh loading)
        );

        if (!scene || !scene->mMeshes || scene->mNumMeshes == 0) {
            std::cout << "Failed to load mesh for collider: " << meshPath << std::endl;
            std::cout << "Assimp error: " << importer.GetErrorString() << std::endl;
            // Return a unit box as fallback
            return new JPH::BoxShapeSettings(scale);
        }

        // Collect all vertices and triangles from all meshes in the scene
        JPH::VertexList vertices;
        JPH::IndexedTriangleList triangles;
        
        // Create rotation matrix from quaternion for transforming vertices
        glm::mat3 rotMat = glm::mat3_cast(rotation);
        
        uint32_t vertexOffset = 0;

        for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
            aiMesh* mesh = scene->mMeshes[m];

            // Add vertices (apply rotation THEN scale)
            for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
                aiVector3D& v = mesh->mVertices[i];
                
                // First convert to glm vec3
                glm::vec3 vertex(v.x, v.y, v.z);
                
                // Apply rotation first
                vertex = rotMat * vertex;
                
                // Then apply scale
                vertices.push_back(JPH::Float3(
                    vertex.x * scale.GetX(),
                    vertex.y * scale.GetY(),
                    vertex.z * scale.GetZ()
                ));
            }

            // Add triangles
            for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
                aiFace& face = mesh->mFaces[i];
                if (face.mNumIndices == 3) {
                    triangles.push_back(JPH::IndexedTriangle(
                        vertexOffset + face.mIndices[0],
                        vertexOffset + face.mIndices[1],
                        vertexOffset + face.mIndices[2]
                    ));
                }
            }

            vertexOffset += mesh->mNumVertices;
        }

        if (vertices.empty() || triangles.empty()) {
            std::cout << "Mesh has no valid geometry for collider: " << meshPath << std::endl;
            return new JPH::BoxShapeSettings(scale);
        }

        std::cout << "Created mesh collider from: " << meshPath << std::endl;
        std::cout << "  -> Vertices: " << vertices.size() << ", Triangles: " << triangles.size() << std::endl;
        std::cout << "  -> Rotation applied: (" << rotation.x << ", " << rotation.y << ", " << rotation.z << ", " << rotation.w << ")" << std::endl;

        return new JPH::MeshShapeSettings(vertices, triangles);
    }

    

    // MAIN SYSTEM IMPLEMENTATION
    void PhysicsSystem::initialize() {
        // 1. Initialize Jolt Factory
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();

        // 2. Allocators
        tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024); // 10 MB
        jobSystem = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

        // 3. Create Layer Interfaces
        bpLayerInterface = new BPLayerInterfaceImpl();
        objectVsBroadPhaseLayerFilter = new ObjectVsBroadPhaseLayerFilterImpl();
        objectLayerPairFilter = new ObjectLayerPairFilterImpl();

        // 4. Create System
        physicsSystem = new JPH::PhysicsSystem();
        physicsSystem->Init(1024, 0, 1024, 1024, *bpLayerInterface, *objectVsBroadPhaseLayerFilter, *objectLayerPairFilter);
        
        bodyInterface = &physicsSystem->GetBodyInterface();

        // 5. Set Contact Listener
        contactListener = new GameContactListener();
        physicsSystem->SetContactListener(contactListener);

        // 6. Initialize Debug Renderer
        debugRenderer = new JoltDebugRenderer();
        debugRenderer->Initialize();
    }

    void PhysicsSystem::cleanup() {
        // Clean up Jolt objects
        if (debugRenderer) {
            debugRenderer->Cleanup();
            delete debugRenderer;
        }
        physicsSystem->SetContactListener(nullptr);
        delete contactListener;
        delete physicsSystem;
        delete jobSystem;
        delete tempAllocator;
        delete bpLayerInterface;
        delete objectVsBroadPhaseLayerFilter;
        delete objectLayerPairFilter;
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

    void PhysicsSystem::update(World* world, float deltaTime) {
        if(!physicsSystem) return;

        // Warmup period: create bodies but don't simulate physics yet
        // This ensures all colliders are loaded before simulation starts
        bool isWarmingUp = (warmupFrames < WARMUP_FRAME_COUNT);
        if (isWarmingUp) {
            warmupFrames++;
            std::cout << "Physics warmup frame " << warmupFrames << "/" << WARMUP_FRAME_COUNT << std::endl;
        }

        //CREATE BODIES
        for(auto entity : world->getEntities()) {
            // Needs at least a collider to be a physics object
            if(!entity->getComponent<ColliderComponent>()) continue;

            auto collider = entity->getComponent<ColliderComponent>();

            // Check if body already exists
            if (!collider->runtimeBodyID.IsInvalid()) continue;

            // Determine if this is static or dynamic
            RigidBodyComponent* rb = entity->getComponent<RigidBodyComponent>();
            
            // --- GET WORLD TRANSFORM ---
            // Use world transform to properly inherit parent's position, rotation, and scale
            glm::mat4 worldMatrix = entity->getLocalToWorldMatrix();
            
            // Extract world position
            glm::vec3 worldPosition = glm::vec3(worldMatrix[3]);
            
            // Extract world scale
            glm::vec3 worldScale;
            worldScale.x = glm::length(glm::vec3(worldMatrix[0]));
            worldScale.y = glm::length(glm::vec3(worldMatrix[1]));
            worldScale.z = glm::length(glm::vec3(worldMatrix[2]));
            
            // Extract world rotation matrix (remove scale)
            glm::mat3 rotationMatrix;
            rotationMatrix[0] = glm::vec3(worldMatrix[0]) / worldScale.x;
            rotationMatrix[1] = glm::vec3(worldMatrix[1]) / worldScale.y;
            rotationMatrix[2] = glm::vec3(worldMatrix[2]) / worldScale.z;
            glm::quat worldRotation = glm::quat_cast(rotationMatrix);

            // --- SHAPE CREATION ---
            JPH::Ref<JPH::ShapeSettings> shapeSettings;
            JPH::Vec3 scale = JPH::Vec3(worldScale.x, worldScale.y, worldScale.z);
            JPH::Vec3 offset = JPH::Vec3(collider->offset.x, collider->offset.y, collider->offset.z) * scale;


            if (collider->type == ColliderType::BOX) {
                JPH::Vec3 scaled = JPH::Vec3(collider->size.x, collider->size.y, collider->size.z) * scale;
                shapeSettings = new JPH::BoxShapeSettings(scaled);
            }
            else if (collider->type == ColliderType::SPHERE) {
                float radius = collider->size.x * std::max(scale.GetX(), std::max(scale.GetY(), scale.GetZ()));
                shapeSettings = new JPH::SphereShapeSettings(radius);
            }
            else if (collider->type == ColliderType::CAPSULE) {
                float radius = collider->size.x * std::max(scale.GetX(), scale.GetZ());
                float height = collider->size.y * scale.GetY(); 
                shapeSettings = new JPH::CapsuleShapeSettings(height, radius);
            }
            else if (collider->type == ColliderType::MESH) {
                // Load mesh from file for collision
                if (collider->collisionMeshPath.empty()) {
                    std::cout << "  -> Mesh collider missing mesh path, using box fallback" << std::endl;
                    shapeSettings = new JPH::BoxShapeSettings(scale);
                } else {
                    std::cout << "  -> Creating mesh collider from: " << collider->collisionMeshPath << std::endl;
                    // Pass rotation to bake into mesh vertices (mesh colliders are static, so rotation is baked)
                    shapeSettings = createMeshShape(collider->collisionMeshPath, scale, worldRotation);
                }
            }

            // Apply offset (skip for mesh colliders - they have geometry baked in)
            if (collider->type != ColliderType::MESH) {
                shapeSettings = new JPH::RotatedTranslatedShapeSettings(
                        offset, 
                        JPH::Quat::sIdentity(), // Rotation offset do later (maybe?)
                        shapeSettings
                    );
            }

            JPH::ShapeSettings::ShapeResult result = shapeSettings->Create();

            if (result.HasError()) {
                std::cout << "Jolt Shape Error: " << result.GetError() << std::endl;
                continue;
            }

            JPH::ShapeRefC shape = result.Get();

            // --- BODY CREATION ---
            // Use world position and rotation (already extracted above)
            JPH::Vec3 pos = JPH::Vec3(worldPosition.x, worldPosition.y, worldPosition.z);
            
            // For mesh colliders, rotation is already baked into the vertices, so use identity rotation
            // For other colliders, apply the world rotation to the body
            JPH::Quat rot = (collider->type == ColliderType::MESH) 
                ? JPH::Quat::sIdentity() 
                : JPH::Quat(worldRotation.x, worldRotation.y, worldRotation.z, worldRotation.w);

            // Determine motion type
            JPH::EMotionType motionType = JPH::EMotionType::Static; // Default to static
            if (rb) {
                if (rb->type == RigidbodyType::DYNAMIC) motionType = JPH::EMotionType::Dynamic;
                else if (rb->type == RigidbodyType::KINEMATIC) motionType = JPH::EMotionType::Kinematic;
                else motionType = JPH::EMotionType::Static;
            }
            
            // IMPORTANT: Mesh colliders can ONLY be static in Jolt
            if (collider->type == ColliderType::MESH && motionType != JPH::EMotionType::Static) {
                std::cout << "Warning: Mesh colliders must be static. Forcing static for entity: " << entity->name << std::endl;
                motionType = JPH::EMotionType::Static;
            }
            
            JPH::ObjectLayer layer = (motionType == JPH::EMotionType::Static) ? Layers::NON_MOVING : Layers::PLAYER;
            if(entity->timeRemaining != 0)
                layer = Layers::PLAYER_ATTACK;
            else if(entity->name == "enemy")
                layer = Layers::ENEMY;

            JPH::BodyCreationSettings bodySettings(shape, pos, rot, motionType, layer);
            
            // Apply RigidBody properties if component exists
            if (rb) {
                // 1. APPLY GRAVITY
                rb->useGravity = false;
                bodySettings.mGravityFactor = rb->useGravity ? 1.0f : 0.0f;

                // 2. APPLY MASS (only for Dynamic bodies)
                if (motionType == JPH::EMotionType::Dynamic) {
                    bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
                    float finalMass = rb->mass > 0.001f ? rb->mass : 1.0f;
                    bodySettings.mMassPropertiesOverride.mMass = finalMass;

                    bodySettings.mAllowedDOFs = JPH::EAllowedDOFs::All;
                    bodySettings.mAllowedDOFs &= ~JPH::EAllowedDOFs::RotationX;
                    bodySettings.mAllowedDOFs &= ~JPH::EAllowedDOFs::RotationZ;
                }
            }

            bodySettings.mUserData = (JPH::uint64)entity;
            bodySettings.mIsSensor = collider->isTrigger;

            bodySettings.mLinearVelocity = rb->impulseVector;

            // Create and Add
            JPH::BodyID bodyID = bodyInterface->CreateAndAddBody(bodySettings, JPH::EActivation::Activate);
            
            // Store the ID in both components
            collider->runtimeBodyID = bodyID;
            if (rb) {
                rb->runtimeBodyID = bodyID;
            }

            // If body is supposed to spwan with movement (bullet)
            //bodyInterface->AddImpulse(rb->runtimeBodyID, rb->impulseVector);
        }

        // UPDATE SIMULATION
        physicsSystem->Update(deltaTime, 1, tempAllocator, jobSystem);


        // SYNC TRANSFORMS WITH PHYSICS
        for(auto entity : world->getEntities()) {
            if(!entity->getComponent<RigidBodyComponent>()) continue;
            auto rb = entity->getComponent<RigidBodyComponent>();

            if (rb->runtimeBodyID.IsInvalid()) continue;
            if (isWarmingUp){
                rb->useGravity = false; // Disable gravity during warmup
                    if (warmupFrames == WARMUP_FRAME_COUNT) {
                        rb->useGravity = true; // Re-enable gravity after warmup
                        JPH::BodyInterface& bodyInterface = physicsSystem->GetBodyInterface();
                        bodyInterface.SetGravityFactor(rb->runtimeBodyID, 1.0f); 
                        bodyInterface.ActivateBody(rb->runtimeBodyID);
                        //std::cout << "Physics warmup complete. Enabling gravity." << std::endl;
                    }
            }

            // Only sync Dynamic bodies (Static/Kinematic are controlled by transform)
            if (bodyInterface->GetMotionType(rb->runtimeBodyID) == JPH::EMotionType::Dynamic) {
                JPH::RVec3 pos = bodyInterface->GetPosition(rb->runtimeBodyID);
                // JPH::Quat rot = bodyInterface->GetRotation(rb->runtimeBodyID);

                entity->localTransform.position = glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
                // entity->localTransform.rotation = ... // TODO: Convert Jolt Quat to glm::quat
            }
        }
    }

    RaycastHit PhysicsSystem::Raycast(glm::vec3 origin, glm::vec3 direction, float maxDistance, const JPH::ObjectLayerFilter& layerFilter) {
        RaycastHit hitResult;
        
        JPH::RVec3 start = JPH::RVec3(origin.x, origin.y, origin.z);
        JPH::Vec3 dir = JPH::Vec3(direction.x, direction.y, direction.z) * maxDistance;
        JPH::RRayCast ray(start, dir);

        JPH::RayCastResult result;

        //std::cout << "shooting ray" << std::endl;


        bool hit = physicsSystem->GetNarrowPhaseQuery().CastRay(
            ray, 
            result, 
            JPH::BroadPhaseLayerFilter(), 
            layerFilter, 
            JPH::BodyFilter()
        );


        if(debugRenderer) {
            if (hit) {
                hitResult.hasHit = true;
                hitResult.distance = result.mFraction * maxDistance;
                
                JPH::RVec3 hitPos = ray.GetPointOnRay(result.mFraction);
                hitResult.position = glm::vec3(hitPos.GetX(), hitPos.GetY(), hitPos.GetZ());

                debugRenderer->DrawLine(start, hitPos, JPH::Color::sGreen);

                // 5. Lock so it doesn't get deleted while reading
                JPH::BodyLockRead lock(physicsSystem->GetBodyLockInterface(), result.mBodyID);
                if (lock.Succeeded()) {
                    const JPH::Body& body = lock.GetBody();
                    
                    // Retrieve Entity Pointer
                    hitResult.entity = reinterpret_cast<Entity*>(body.GetUserData());
                }
            } else
                debugRenderer->DrawLine(start, start + dir, JPH::Color::sRed);
        }

        return hitResult;
    }
}
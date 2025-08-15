#include <btBulletDynamicsCommon.h>

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/Triple.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Arguments.h>
//#include <Corrade/Utility/Debug.h>
#include <Corrade/Utility/DebugStl.h>
#include <Corrade/Utility/Format.h>

#include <Magnum/Timeline.h>
#include <Magnum/ImageView.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Quaternion.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/GenerateIndices.h>
#include <Magnum/Platform/Sdl2Application.h>
// #include <Magnum/Primitives/Icosphere.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/Trade/PhongMaterialData.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Trade/TextureData.h>
#include <Magnum/Trade/LightData.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/DebugTools/FrameProfiler.h>
#include <Magnum/BulletIntegration/Integration.h>
#include <Magnum/BulletIntegration/MotionState.h>
#include <Magnum/BulletIntegration/DebugDraw.h>

#include <Magnum/Ui/Anchor.h>
#include <Magnum/Ui/Application.h>
// #include <Magnum/Ui/BaseLayer.h> /* for DebugLayer style names */
// #include <Magnum/Ui/EventLayer.h> /* for DebugLayer style names */
// #include <Magnum/Ui/DebugLayerGL.h>
// #include <Magnum/Ui/Event.h>
#include <Magnum/Ui/Label.h>
// #include <Magnum/Ui/NodeFlags.h>
#include <Magnum/Ui/SnapLayouter.h>
#include <Magnum/Ui/Style.h>
// #include <Magnum/Ui/Style.hpp> /* for DebugLayer style names */
// #include <Magnum/Ui/TextLayer.h> /* for DebugLayer style names */
#include <Magnum/Ui/TextProperties.h>
#include <Magnum/Ui/UserInterfaceGL.h>

// #include <MagnumPlugins/GltfImporter/GltfImporter.h>

#include <vector>

#ifdef BT_USE_DOUBLE_PRECISION
#error sorry, this example does not support Bullet with double precision enabled
#endif

#include "Object3D.h"
#include "ColoredDrawable.h"
#include "TexturedDrawable.h"
#include "RigidBody.h"
#include "Player.h"
#include "globals.h"

struct LightInfo {
    Magnum::Trade::LightData data;
    Magnum::Matrix4 transformation;
};

//typedef Containers::Array<LightInfo> LightVector; _lights;
typedef std::vector<LightInfo> LightVector; // TODO possibly convert to using Containers::Array<>

struct InstanceData {
    Magnum::Matrix4 transformationMatrix;
    Magnum::Matrix3x3 normalMatrix;
    Magnum::Color3 color;
};

// static int bulletDebugCounter;

static void myContactStartedCallback(btPersistentManifold* const &manifold)
{
    // Magnum::Debug{} << bulletDebugCounter++ << "myContactStartedCallback(" << manifold << "); body0 =" << manifold->getBody0() << "; body1 =" << manifold->getBody1();
}

static void myContactEndedCallback(btPersistentManifold* const &manifold)
{
    // Magnum::Debug{} << bulletDebugCounter++ << "myContactEndedCallback(" << manifold << "); body0 =" << manifold->getBody0() << "; body1 =" << manifold->getBody1();
}

static bool myContactProcessedCallback(btManifoldPoint &cp, void *vbody0, void *vbody1)
{
    btCollisionObject * const obj0 = static_cast<btCollisionObject*>(vbody0);
    btCollisionObject * const obj1 = static_cast<btCollisionObject*>(vbody1);
    // Magnum::Debug{} << bulletDebugCounter++ << "myContactProcessedCallback(" << &cp << "," << obj0 << "," << obj1 << ")";

    btRigidBody * player = nullptr;
    if (obj0->getUserIndex() == PhysicsUserIndex::Player)
    {
        player = dynamic_cast<btRigidBody*>(obj1);
    }
    else if (obj1->getUserIndex() == PhysicsUserIndex::Player)
    {
        player = dynamic_cast<btRigidBody*>(obj0);
    }
    if (player)
    {
        const btVector3 contactNormal = cp.m_normalWorldOnB;
        const btScalar verticalComponent = contactNormal.dot(btVector3(0, 1, 0));
        const btVector3 vel = player->getLinearVelocity();

        // TODO test suppressing "popping" up over edges too forcefully (due to player capsule's rounded bottom)
        #if 0
        if (verticalComponent > 0.2 && vel.y() > 0.05)
        {
            // If capsule is walking off a ledge and you want to suppress vertical pop:
            btVector3 modifiedNormal = contactNormal;
            modifiedNormal.setY(0.0); // Remove vertical component
            modifiedNormal.normalize();
            cp.m_normalWorldOnB = modifiedNormal;
        }
        #endif
        if (std::abs(verticalComponent) < 0.4f)
        {
            // This is a mostly horizontal contact (wall) — reduce friction
            cp.m_combinedFriction = 0.0f;
        }
    }

    // cp.m_combinedFriction = 0.5f;
    // cp.m_combinedRollingFriction = 0.1f;
    // cp.m_combinedRestitution = 0.8f;

    btRigidBody * toLaunch = nullptr;
    if (obj0->getUserIndex() == PhysicsUserIndex::JumpPad)
    {
        toLaunch = dynamic_cast<btRigidBody*>(obj1);
    }
    else if (obj1->getUserIndex() == PhysicsUserIndex::JumpPad)
    {
        toLaunch = dynamic_cast<btRigidBody*>(obj0);
    }
    if (toLaunch)
    {
        btVector3 vel = toLaunch->getLinearVelocity();
        vel.setY(10.0);
        toLaunch->setLinearVelocity(vel);
    }

    return true;
}

class ViewerExample: public Magnum::Platform::Application {
    public:
        explicit ViewerExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void viewportEvent(ViewportEvent& event) override;
        // void tickEvent() override;
        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;
        void pointerPressEvent(PointerEvent& event) override;
        void pointerReleaseEvent(PointerEvent& event) override;
        void pointerMoveEvent(PointerMoveEvent& event) override;
        void scrollEvent(ScrollEvent& event) override;

        Magnum::GL::Mesh _sphereMesh{Magnum::NoCreate};
        Magnum::Shaders::PhongGL _coloredShader;
        Magnum::Shaders::PhongGL _texturedShader{Magnum::Shaders::PhongGL::Configuration{}
            .setFlags(Magnum::Shaders::PhongGL::Flag::DiffuseTexture)};
        Magnum::Containers::Array<Magnum::Containers::Optional<Magnum::GL::Mesh>> _meshes;
        Magnum::Containers::Array<Magnum::Containers::Optional<Magnum::GL::Texture2D>> _textures;
        LightVector _lights;

        Magnum::Containers::Array<Magnum::Containers::Array<Magnum::Vector3>> _physicsMeshVertices;
        Magnum::Containers::Array<Magnum::Containers::Array<Magnum::UnsignedInt>> _physicsMeshIndices;
        Magnum::Containers::Array<Magnum::Containers::Pointer<btTriangleIndexVertexArray>> _physicsTriMeshes;
        Magnum::Containers::Array<Magnum::Containers::Pointer<btBvhTriangleMeshShape>> _physicsShapes;

        Magnum::BulletIntegration::DebugDraw _debugDraw{Magnum::NoCreate};
        Magnum::Containers::Array<InstanceData> _boxInstanceData, _sphereInstanceData;

        btDbvtBroadphase _bBroadphase;
        btDefaultCollisionConfiguration _bCollisionConfig;
        btCollisionDispatcher _bDispatcher{&_bCollisionConfig};
        btSequentialImpulseConstraintSolver _bSolver;
        btDiscreteDynamicsWorld _bWorld{&_bDispatcher, &_bBroadphase, &_bSolver, &_bCollisionConfig};
        btSphereShape _bSphereShape{BALL_RADIUS};

        Scene3D _scene;
        Object3D _cameraObject;
        Player *_player{nullptr};
        Magnum::SceneGraph::Camera3D* _camera;
        Magnum::SceneGraph::DrawableGroup3D _drawables;
        Magnum::Vector3 _cameraPosition{3.0, 3.0, 15.0};
        Magnum::Float _cameraPitch{0.0};
        Magnum::Float _cameraYaw{0.0};
        Magnum::Timeline _timeline;
        Magnum::Ui::UserInterfaceGL _ui{Magnum::NoCreate};
        Magnum::Ui::Label _fpsLabel{Magnum::NoCreate, _ui};

        Magnum::DebugTools::FrameProfilerGL _profiler;
        Magnum::Float _fpsUpdateTimer{0.0};
        bool _physicsDebugDrawing{true};
};

static const char GLTF_FILE_PATH[] = "../data/test_scene_torus.glb";
// static const char GLTF_FILE_PATH[] = "../data/test_scene.gltf";

constexpr const Magnum::Float WidgetHeight = 36.0f;
constexpr const Magnum::Float LabelHeight = 24.0f;
constexpr const Magnum::Vector2 LabelSize{72.0f, LabelHeight};

static const Magnum::Float CAMERA_MOVE_SPEED = 50.0; // units per second

ViewerExample::ViewerExample(const Arguments& arguments):
    Magnum::Platform::Application{arguments, Configuration{}
        .setTitle("Magnum Viewer Example")
        .setWindowFlags(Configuration::WindowFlag::Resizable)}
{
    // set up UI
    _ui.create(*this, Magnum::Ui::McssDarkStyle{});
    Magnum::Ui::NodeHandle root = _ui.createNode({}, _ui.size());
    {
        _fpsLabel = Magnum::Ui::Label(
            Magnum::Ui::snap(_ui, Magnum::Ui::Snap::TopLeft|Magnum::Ui::Snap::Inside, root, LabelSize),
            "FPS:",
            Magnum::Text::Alignment::MiddleLeft,
            Magnum::Ui::LabelStyle::Default
        );

        /*Magnum::Ui::NodeHandle labels2 = Magnum::Ui::label(
            Magnum::Ui::snap(_ui, Magnum::Ui::Snap::BottomLeft|Magnum::Ui::Snap::InsideX, _fpsLabel, LabelSize),
            "CPU %:",
            Magnum::Text::Alignment::MiddleLeft,
            Magnum::Ui::LabelStyle::Default
        );*/
    }

    // set up framerate tracker
    _profiler.setup(
        Magnum::DebugTools::FrameProfilerGL::Value::FrameTime|
        Magnum::DebugTools::FrameProfilerGL::Value::CpuDuration|(
            #ifndef MAGNUM_TARGET_GLES
            Magnum::GL::Context::current().isExtensionSupported<Magnum::GL::Extensions::ARB::timer_query>()
            #elif !defined(MAGNUM_TARGET_WEBGL)
            Magnum::GL::Context::current().isExtensionSupported<Magnum::GL::Extensions::EXT::disjoint_timer_query>()
            #else
            Magnum::GL::Context::current().isExtensionSupported<Magnum::GL::Extensions::EXT::disjoint_timer_query_webgl2>()
            #endif
            ?
                Magnum::DebugTools::FrameProfilerGL::Value::GpuDuration : Magnum::DebugTools::FrameProfilerGL::Values{}),
        50
    );

    // OpenGL settings for the UI to work properly
    Magnum::GL::Renderer::setClearColor(Magnum::Color3::fromLinearRgbInt(0x22272e));
    Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);
    Magnum::GL::Renderer::setBlendFunction(Magnum::GL::Renderer::BlendFunction::One, Magnum::GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    // OpenGL settings for Bullet debug rendering
    Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::DepthTest);
    Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);
    Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::PolygonOffsetFill);
    Magnum::GL::Renderer::setPolygonOffset(2.0f, 0.5f);

    _debugDraw = Magnum::BulletIntegration::DebugDraw{};
    _debugDraw.setMode(Magnum::BulletIntegration::DebugDraw::Mode::DrawWireframe);
    _bWorld.setGravity({0.0f, -9.8f, 0.0f});
    _bWorld.setDebugDrawer(&_debugDraw);

    _cameraObject
        .setParent(&_scene);
    (*(_camera = new Magnum::SceneGraph::Camera3D{_cameraObject}))
        .setAspectRatioPolicy(Magnum::SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(
            Magnum::Matrix4::perspectiveProjection(Magnum::Deg(35.0f), 1.0f, 0.01f, 1000.0f)
        )
        .setViewport(Magnum::GL::defaultFramebuffer.viewport().size());

    // _sphereMesh = Magnum::MeshTools::compile(Magnum::Primitives::icosphereSolid(3));
    _sphereMesh = Magnum::MeshTools::compile(Magnum::Primitives::uvSphereSolid(32, 64));

    /* Setup renderer and shader defaults */
    Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::DepthTest);
    Magnum::GL::Renderer::enable(Magnum::GL::Renderer::Feature::FaceCulling);
    _coloredShader
        .setAmbientColor(Magnum::Color3::fromLinearRgbInt(0x111111))
        .setSpecularColor(Magnum::Color3::fromLinearRgbInt(0xffffff))
        .setShininess(80.0f);
    _texturedShader
        .setAmbientColor(Magnum::Color3::fromLinearRgbInt(0x111111))
        .setSpecularColor(Magnum::Color3::fromLinearRgbInt(0x111111))
        .setShininess(80.0f);

    /* Load a scene importer plugin */
    Magnum::PluginManager::Manager<Magnum::Trade::AbstractImporter> manager;
    Magnum::Containers::Pointer<Magnum::Trade::AbstractImporter> importer = manager.loadAndInstantiate("GltfImporter");

    if(!importer || !importer->openFile(GLTF_FILE_PATH))
        std::exit(1);

    /* Load all textures. Textures that fail to load will be NullOpt. */
    _textures = Magnum::Containers::Array<Magnum::Containers::Optional<Magnum::GL::Texture2D>>{
        importer->textureCount()};
    for(Magnum::UnsignedInt i = 0; i != importer->textureCount(); ++i) {
        Magnum::Containers::Optional<Magnum::Trade::TextureData> textureData =
            importer->texture(i);
        if(!textureData || textureData->type() != Magnum::Trade::TextureType::Texture2D) {
            Magnum::Warning{} << "Cannot load texture" << i
                << importer->textureName(i);
            continue;
        }

        Magnum::Containers::Optional<Magnum::Trade::ImageData2D> imageData =
            importer->image2D(textureData->image());
        if(!imageData || imageData->isCompressed()) {
            Magnum::Warning{} << "Cannot load image" << textureData->image()
                << importer->image2DName(textureData->image());
            continue;
        }

        (*(_textures[i] = Magnum::GL::Texture2D{}))
            .setMagnificationFilter(textureData->magnificationFilter())
            .setMinificationFilter(textureData->minificationFilter(),
                                   textureData->mipmapFilter())
            .setWrapping(textureData->wrapping().xy())
            .setStorage(Magnum::Math::log2(imageData->size().max()) + 1,
                Magnum::GL::textureFormat(imageData->format()), imageData->size())
            .setSubImage(0, {}, *imageData)
            .generateMipmap();
    }

    /* Load all materials. Materials that fail to load will be NullOpt. Only a
       temporary array as the material attributes will be stored directly in
       drawables later. */
    Magnum::Containers::Array<Magnum::Containers::Optional<Magnum::Trade::PhongMaterialData>> materials{
        importer->materialCount()};
    for(Magnum::UnsignedInt i = 0; i != importer->materialCount(); ++i) {
        Magnum::Containers::Optional<Magnum::Trade::MaterialData> materialData;
        if(!(materialData = importer->material(i))) {
            Magnum::Warning{} << "Cannot load material" << i
                << importer->materialName(i);
            continue;
        }

        materials[i] = std::move(*materialData).as<Magnum::Trade::PhongMaterialData>();
    }

    /* Load all meshes. Meshes that fail to load will be NullOpt. Generate
       normals if not present. */
    _meshes = Magnum::Containers::Array<Magnum::Containers::Optional<Magnum::GL::Mesh>>{importer->meshCount()};
    _physicsMeshVertices = Magnum::Containers::Array<Magnum::Containers::Array<Magnum::Vector3>>{importer->meshCount()};
    _physicsMeshIndices = Magnum::Containers::Array<Magnum::Containers::Array<Magnum::UnsignedInt>>{importer->meshCount()};
    _physicsTriMeshes = Magnum::Containers::Array<Magnum::Containers::Pointer<btTriangleIndexVertexArray>>{importer->meshCount()};
    for(Magnum::UnsignedInt i = 0; i != importer->meshCount(); ++i) {
        Magnum::Containers::Optional<Magnum::Trade::MeshData> meshData;
        if(!(meshData = importer->mesh(i))) {
            Magnum::Warning{} << "Cannot load mesh" << i << importer->meshName(i);
            continue;
        }

        Magnum::MeshTools::CompileFlags flags;
        if(!meshData->hasAttribute(Magnum::Trade::MeshAttribute::Normal))
            flags |= Magnum::MeshTools::CompileFlag::GenerateFlatNormals;
        _meshes[i] = Magnum::MeshTools::compile(*meshData, flags);

        // make a Bullet physics friendly version of the mesh data
        const Magnum::Trade::MeshData triangleMesh = Magnum::MeshTools::generateIndices(*meshData);
        Magnum::Containers::Array<Magnum::Vector3> &physicsMeshVertices = _physicsMeshVertices[i];
        Magnum::Containers::Array<Magnum::UnsignedInt> &physicsMeshIndices = _physicsMeshIndices[i];
        Magnum::Containers::Pointer<btTriangleIndexVertexArray> &triMesh = _physicsTriMeshes[i];
        physicsMeshVertices = triangleMesh.positions3DAsArray();
        physicsMeshIndices = triangleMesh.indicesAsArray();

        {
            btIndexedMesh bulletMesh;
            bulletMesh.m_numTriangles = physicsMeshIndices.size()/3;
            bulletMesh.m_triangleIndexBase = reinterpret_cast<const unsigned char *>(physicsMeshIndices.data());
            bulletMesh.m_triangleIndexStride = 3 * sizeof(Magnum::UnsignedInt);
            bulletMesh.m_numVertices = physicsMeshVertices.size();
            bulletMesh.m_vertexBase = reinterpret_cast<const unsigned char *>(physicsMeshVertices.data());
            bulletMesh.m_vertexStride = sizeof(Magnum::Vector3);
            bulletMesh.m_indexType = PHY_INTEGER;
            bulletMesh.m_vertexType = PHY_FLOAT;

            triMesh.emplace();
            triMesh->addIndexedMesh(bulletMesh, PHY_INTEGER);
        }
    }

    // Assume there's a scene
    CORRADE_INTERNAL_ASSERT(importer->defaultScene() != -1);

    /* Load the scene */
    Magnum::Containers::Optional<Magnum::Trade::SceneData> scene;
    if(!(scene = importer->scene(importer->defaultScene())) ||
       !scene->is3D() ||
       !scene->hasField(Magnum::Trade::SceneField::Parent) ||
       !scene->hasField(Magnum::Trade::SceneField::Mesh))
    {
        Magnum::Fatal{} << "Cannot load scene" << importer->defaultScene()
            << importer->sceneName(importer->defaultScene());
    }

    /* Allocate objects that are part of the hierarchy */
    Magnum::Containers::Array<Object3D*> objects{std::size_t(scene->mappingBound())};
    Magnum::Containers::Array<Magnum::Vector3> scales{std::size_t(scene->mappingBound())};
    Magnum::Containers::Array<Magnum::Containers::Pair<Magnum::UnsignedInt, Magnum::Int>> parents
        = scene->parentsAsArray();
    for(const Magnum::Containers::Pair<Magnum::UnsignedInt, Magnum::Int>& parent: parents)
        objects[parent.first()] = new Object3D{};

    /* Assign parent references */
    for(const Magnum::Containers::Pair<Magnum::UnsignedInt, Magnum::Int>& parent: parents)
        objects[parent.first()]->setParent(parent.second() == -1 ?
            &_scene : objects[parent.second()]);

    /* Set transformations. Objects that are not part of the hierarchy are
       ignored, objects that have no transformation entry retain an identity
       transformation. */
    for(const Magnum::Containers::Pair<Magnum::UnsignedInt, Corrade::Containers::Triple<Magnum::Vector3, Magnum::Quaternion, Magnum::Vector3>>& transformation:
        scene->translationsRotationsScalings3DAsArray())
    {
        //Utility::Debug{} << "transformation.first() = " << transformation.first() << "; " << objects[transformation.first()] << "; " << transformation.second();
        if(Object3D* object = objects[transformation.first()]) {
            object->setTransformation(
                Magnum::Matrix4::translation(transformation.second().first())*
                Magnum::Matrix4{transformation.second().second().toMatrix()});
            scales[transformation.first()] = transformation.second().third();
        }
    }

    /* Add drawables for objects that have a mesh, again ignoring objects that
       are not part of the hierarchy. There can be multiple mesh assignments
       for one object, simply add one drawable for each. */
    _physicsShapes = Magnum::Containers::Array<Magnum::Containers::Pointer<btBvhTriangleMeshShape>>{std::size_t(scene->mappingBound())};
    for(const Magnum::Containers::Pair<Magnum::UnsignedInt, Magnum::Containers::Pair<Magnum::UnsignedInt, Magnum::Int>>&
        meshMaterial: scene->meshesMaterialsAsArray())
    {
        const Magnum::UnsignedInt meshIndex = meshMaterial.second().first();
        const Magnum::UnsignedInt objectId = meshMaterial.first();
        const Magnum::Int materialId = meshMaterial.second().second();
        Object3D* const object = objects[objectId];
        Magnum::Containers::Optional<Magnum::GL::Mesh>& mesh = _meshes[meshIndex];
        if(!object || !mesh) continue;

        // create Bullet "shape" for this object
        {
            Magnum::Containers::Pointer<btTriangleIndexVertexArray> &triMesh = _physicsTriMeshes[meshIndex];
            Magnum::Containers::Pointer<btBvhTriangleMeshShape> &shape = _physicsShapes[meshIndex];

            shape.emplace(triMesh.get(), true, true);
            shape->setLocalScaling(btVector3(scales[objectId]));
        }

        /* Material not available / not loaded, use a default material */
        if(materialId == -1 || !materials[materialId]) {
            new RigidBody{*object, 0.0f, _physicsShapes[meshIndex].get(), _bWorld};
            new ColoredDrawable{*object, scales[objectId], _coloredShader, *mesh, Magnum::Color3::fromLinearRgbInt(0xffffff), _drawables};

        /* Textured material, if the texture loaded correctly */
        } else if(materials[materialId]->hasAttribute(
                Magnum::Trade::MaterialAttribute::DiffuseTexture
            ) && _textures[materials[materialId]->diffuseTexture()])
        {
            new RigidBody{*object, 0.0f, _physicsShapes[meshIndex].get(), _bWorld};
            new TexturedDrawable{*object, _texturedShader, *mesh,
                *_textures[materials[materialId]->diffuseTexture()],
                _drawables};

        /* Color-only material */
        } else {
            /* Assign a RigidBody and a drawable feature to the object */
            new RigidBody{*object, 0.0f, _physicsShapes[meshIndex].get(), _bWorld};
            new ColoredDrawable{*object, scales[objectId], _coloredShader, *mesh, materials[materialId]->diffuseColor(), _drawables};
        }
    }

    for(const Magnum::Containers::Pair<Magnum::UnsignedInt, Magnum::UnsignedInt> &lightPair : scene->lightsAsArray())
    {
        //Utility::Debug{} << "lightPair: " << lightPair.first() << " " << lightPair.second();

        if(Object3D * const object = objects[lightPair.first()])
        {
            Magnum::Containers::Optional<Magnum::Trade::LightData> data = importer->light(lightPair.second());
            if (!data)
                continue;
            _lights.emplace_back(LightInfo{
                .data = *Magnum::Utility::move(data),
                .transformation = object->absoluteTransformationMatrix()
            });
            //Utility::Debug{} << "light using transformation: " << _lights.back().transformation;
        }
    }

    // gContactAddedCallback = myContactAddedCallback;
    gContactStartedCallback = myContactStartedCallback;
    gContactEndedCallback = myContactEndedCallback;
    gContactProcessedCallback = myContactProcessedCallback;

    setCursor(Cursor::HiddenLocked);
    // setSwapInterval(1);
    // setMinimalLoopPeriod(8.0_msec); // max 120Hz?
    _timeline.start();

    // add a jump pad
    {
        btTransform jumpPadTransform;
        jumpPadTransform.setIdentity();
        jumpPadTransform.setOrigin(btVector3(2, 0.25, 0)); // jump pad position

        btCollisionShape * const shape = new btBoxShape(btVector3(1.0f, 0.25f, 1.0f)); // jump pad size
        btVector3 bInertia(0.0f, 0.0f, 0.0f);
        btDefaultMotionState * const motionState = new btDefaultMotionState(jumpPadTransform);
        const btRigidBody::btRigidBodyConstructionInfo rbInfo{0, motionState, shape, bInertia};

        btRigidBody * const jumpPadBody = new btRigidBody(rbInfo);
        jumpPadBody->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE); // doesn't collide with anything!
        // jumpPadBody->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK); // for contact added callback to be called
        jumpPadBody->setUserIndex(PhysicsUserIndex::JumpPad);

        _bWorld.addCollisionObject(jumpPadBody);
    }

    // add player
    _player = new Player(&_scene, _coloredShader, _drawables, _bWorld);
}

void ViewerExample::drawEvent() {
    const Magnum::Float timeDelta = _timeline.previousFrameDuration();
    const Magnum::Float cameraMoveSpeedScalar = CAMERA_MOVE_SPEED*timeDelta;
    const Magnum::Float playerMoveSpeedScalar = PLAYER_WALK_SPEED*timeDelta;

    // player avatar movement
    {
        Magnum::Vector3 dir(Magnum::Math::ZeroInit);

        // determine the walking direction
        const bool iPressed = isKeyPressed(Sdl2Application::Key::I);
        const bool kPressed = isKeyPressed(Sdl2Application::Key::K);
        const bool jPressed = isKeyPressed(Sdl2Application::Key::J);
        const bool lPressed = isKeyPressed(Sdl2Application::Key::L);
        const bool jmpPressed = isKeyPressed(Sdl2Application::Key::RightShift);
        if (iPressed && !kPressed)
            dir.z() = -1.0;
        else if (!iPressed && kPressed)
            dir.z() = 1.0;
        if (jPressed && !lPressed)
            dir.x() = -1.0;
        else if (!jPressed && lPressed)
            dir.x() = 1.0;
        if (!dir.isZero())
        {
            dir = dir.normalized();
            dir = Magnum::Matrix4::rotationY(Magnum::Deg(_cameraYaw)).transformPoint(dir);
            _player->walkInDirection(dir);
        }
        if (jmpPressed && _player->isOnGround())
        {
            _player->jump();
        }
    }

    _bWorld.stepSimulation(timeDelta, 10, 1.0f / 240.0f); // TODO move this to tickEvent()?
    // _bWorld.performDiscreteCollisionDetection();

    Magnum::Vector3 vel;
    {
        const bool wPressed = isKeyPressed(Sdl2Application::Key::W);
        const bool sPressed = isKeyPressed(Sdl2Application::Key::S);
        const bool aPressed = isKeyPressed(Sdl2Application::Key::A);
        const bool dPressed = isKeyPressed(Sdl2Application::Key::D);
        const bool spacePressed = isKeyPressed(Sdl2Application::Key::Space);
        const bool ctrlPressed = isKeyPressed(Sdl2Application::Key::LeftCtrl);
        if (wPressed && !sPressed)
            vel.z() = -1.0;
        else if (!wPressed && sPressed)
            vel.z() = 1.0;
        if (aPressed && !dPressed)
            vel.x() = -1.0;
        else if (!aPressed && dPressed)
            vel.x() = 1.0;
        if (spacePressed && !ctrlPressed)
            vel.y() = 1.0;
        else if (!spacePressed && ctrlPressed)
            vel.y() = -1.0;
    }
    if (!vel.isZero())
        vel = vel.normalized()*cameraMoveSpeedScalar;

    const Magnum::Matrix4 horizontalRotationMatrix = Magnum::Matrix4::rotationY(Magnum::Deg(_cameraYaw));
    const Magnum::Matrix4 verticalRotationMatrix = Magnum::Matrix4::rotationX(Magnum::Deg(_cameraPitch));
    const Magnum::Matrix4 totalRotationMatrix = horizontalRotationMatrix*verticalRotationMatrix;

    // _cameraPosition += horizontalRotationMatrix.transformVector(vel); // keeps you at the same altitude
    _cameraPosition += totalRotationMatrix.transformVector(vel); // flies around like an airplane

    // const Magnum::Matrix4 overallCameraTransform = Magnum::Matrix4::translation(_player->absoluteTransformation().translation())*totalRotationMatrix*Magnum::Matrix4::translation(Magnum::Vector3(0.0, 0.0, 5.0)); // for 3rd person camera
    const Magnum::Matrix4 overallCameraTransform = Magnum::Matrix4::translation(_cameraPosition)*totalRotationMatrix; // for free camera
    _cameraObject.setTransformation(overallCameraTransform);

    static const Magnum::Float FPS_UPDATE_INTERVAL = 0.250;
    _fpsUpdateTimer += timeDelta;
    if (_fpsUpdateTimer > FPS_UPDATE_INTERVAL && _profiler.isMeasurementAvailable(Magnum::DebugTools::FrameProfilerGL::Value::FrameTime))
    {
        _fpsUpdateTimer = Magnum::Math::fmod(_fpsUpdateTimer, FPS_UPDATE_INTERVAL);
        const Magnum::Float frameTime = _profiler.frameTimeMean();
        const Magnum::Float fps = 1000000000.0/frameTime;
        _fpsLabel.setText(Magnum::Utility::format("FPS: {:.3f}", fps));
    }

    Magnum::GL::defaultFramebuffer.clear(Magnum::GL::FramebufferClear::Color|Magnum::GL::FramebufferClear::Depth);

    {
        // TODO refactor this code so that the calculations are performed once per camera movement

        // Prepare light positions, colors, and ranges
        Magnum::Containers::Array<Magnum::Vector4> lightPositions{Magnum::NoInit, _lights.size()};
        Magnum::Containers::Array<Magnum::Color3> lightColors{Magnum::NoInit, _lights.size()};
        Magnum::Containers::Array<Magnum::Float> lightRanges{Magnum::NoInit, _lights.size()};

        for (std::size_t i = 0; i < _lights.size(); ++i) {
            const LightInfo &light = _lights[i];
            Magnum::Vector3 position = light.transformation.translation();

            lightPositions[i] = Magnum::Vector4{_camera->cameraMatrix().transformPoint(position), light.data.type() == Magnum::Trade::LightType::Directional ? 0.0f : 1.0f};

            lightColors[i] = light.data.color() * light.data.intensity() * 0.01f; // Blender likes to make absurdly huge intensities for some reason, I think it uses different units?

            //lightRanges[i] = light.range; // TODO import this
            lightRanges[i] = Magnum::Constants::inf();
        }

        _coloredShader
            .setLightPositions(lightPositions)
            .setLightColors(lightColors)
            .setLightRanges(lightRanges);
        _texturedShader
            .setLightPositions(lightPositions)
            .setLightColors(lightColors)
            .setLightRanges(lightRanges);
    }

    _profiler.beginFrame();
    _camera->draw(_drawables);
    if (_physicsDebugDrawing)
    {
        Magnum::GL::Renderer::setDepthFunction(Magnum::GL::Renderer::DepthFunction::LessOrEqual);
        _debugDraw.setTransformationProjectionMatrix(_camera->projectionMatrix()*_camera->cameraMatrix());
        _bWorld.debugDrawWorld();
        Magnum::GL::Renderer::setDepthFunction(Magnum::GL::Renderer::DepthFunction::Less);
    }
    _ui.draw();
    _profiler.endFrame();

    swapBuffers();
    redraw(); // HACK trigger another redraw immediately, so that the walking logic will run (should be doing this in tickEvent() or something else...)
    _timeline.nextFrame();
}

void ViewerExample::viewportEvent(ViewportEvent& event) {
    Magnum::GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
    _camera->setViewport(event.windowSize());
    _ui.setSize(event);
}

/*void ViewerExample::tickEvent()
{
}*/

void ViewerExample::keyPressEvent(KeyEvent& event)
{
    if (event.key() == Sdl2Application::Key::Esc)
    {
        exit();
    }
    else if (event.key() == Sdl2Application::Key::Tab)
    {
        setCursor((cursor() == Cursor::Arrow) ? Cursor::HiddenLocked : Cursor::Arrow);
    }
    else if (event.key() == Sdl2Application::Key::X)
    {
        _physicsDebugDrawing = !_physicsDebugDrawing;
    }
}

void ViewerExample::keyReleaseEvent(KeyEvent& event)
{
    (void)event;
}

static void launchBall(Scene3D &scene, Magnum::Shaders::PhongGL &shader, Magnum::GL::Mesh &mesh, Magnum::SceneGraph::DrawableGroup3D &group, btDynamicsWorld &world, btSphereShape &shape, const Magnum::Vector3 &pos, const Magnum::Vector3 &dir)
{
    Object3D * const sphere = new Object3D{&scene};
    sphere->translate(pos);
    RigidBody * const body = new RigidBody{*sphere, BALL_MASS, &shape, world};
    new ColoredDrawable{*sphere, BALL_SCALE, shader, mesh, Magnum::Color3::fromLinearRgbInt(0xffffff), group};

    static const Magnum::Float SPEED = 25.0;
    body->rigidBody().setDamping(0.0, 0.2); // linear, angular
    // body->rigidBody().setRestitution(0.8);
    body->rigidBody().setLinearVelocity(btVector3{dir*SPEED});
}

void ViewerExample::pointerPressEvent(PointerEvent& event) {
    if(!event.isPrimary())
        return;
    if (!(event.pointer() & (Pointer::MouseLeft|Pointer::Finger)))
        return;

    // const Magnum::Matrix4 playerTrans = _player->absoluteTransformation();
    const Magnum::Matrix4 cameraTrans = _cameraObject.absoluteTransformation();
    // const Magnum::Vector3 position = playerTrans.translation() + direction*(PLAYER_RADIUS*1.5+BALL_RADIUS);
    const Magnum::Vector3 position = cameraTrans.translation();
    const Magnum::Vector3 direction = cameraTrans.rotation()*Magnum::Vector3(0.0, 0.0, -1.0);

    launchBall(_scene, _coloredShader, _sphereMesh, _drawables, _bWorld, _bSphereShape, position, direction);

    event.setAccepted();
}

void ViewerExample::pointerReleaseEvent(PointerEvent& event) {
    if(!event.isPrimary())
        return;
    // if (event.pointer() & (Pointer::MouseLeft|Pointer::Finger)) {}
}

void ViewerExample::scrollEvent(ScrollEvent& event) {
    if(!event.offset().y())
        return;
}

void ViewerExample::pointerMoveEvent(PointerMoveEvent& event) {
    if(!event.isPrimary())
        return;
    if (cursor() != Cursor::HiddenLocked)
        return;

    const Magnum::Vector2 currentPosition = event.position();
    const Magnum::Vector2 delta = event.relativePosition();

    if(delta.isZero())
        return;

    _cameraYaw  -= delta.x() * 0.1f;
    _cameraPitch -= delta.y() * 0.1f;
    _cameraPitch = Magnum::Math::clamp(_cameraPitch, -89.0f, 89.0f);

    // redraw();
}

MAGNUM_APPLICATION_MAIN(ViewerExample)

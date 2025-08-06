#include <btBulletDynamicsCommon.h>

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
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
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/GenerateIndices.h>
#include <Magnum/Platform/Sdl2Application.h>
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

typedef Magnum::SceneGraph::Object<Magnum::SceneGraph::MatrixTransformation3D> Object3D;
typedef Magnum::SceneGraph::Scene<Magnum::SceneGraph::MatrixTransformation3D> Scene3D;

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

class ViewerExample: public Magnum::Platform::Application {
    public:
        explicit ViewerExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void viewportEvent(ViewportEvent& event) override;
        // void tickEvent() override;
        void focusEvent(FocusEvent& event) override;
        void blurEvent(FocusEvent& event) override;
        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;
        void pointerPressEvent(PointerEvent& event) override;
        void pointerReleaseEvent(PointerEvent& event) override;
        void pointerMoveEvent(PointerMoveEvent& event) override;
        void scrollEvent(ScrollEvent& event) override;

        Magnum::Shaders::PhongGL _coloredShader;
        Magnum::Shaders::PhongGL _texturedShader{Magnum::Shaders::PhongGL::Configuration{}
            .setFlags(Magnum::Shaders::PhongGL::Flag::DiffuseTexture)};
        Magnum::Containers::Array<Magnum::Containers::Optional<Magnum::GL::Mesh>> _meshes;
        Magnum::Containers::Array<Magnum::Containers::Optional<Magnum::GL::Texture2D>> _textures;
        LightVector _lights;

        Magnum::BulletIntegration::DebugDraw _debugDraw{Magnum::NoCreate};
        Magnum::Containers::Array<InstanceData> _boxInstanceData, _sphereInstanceData;

        btDbvtBroadphase _bBroadphase;
        btDefaultCollisionConfiguration _bCollisionConfig;
        btCollisionDispatcher _bDispatcher{&_bCollisionConfig};
        btSequentialImpulseConstraintSolver _bSolver;
        btDiscreteDynamicsWorld _bWorld{&_bDispatcher, &_bBroadphase, &_bSolver, &_bCollisionConfig};
        btSphereShape _bSphereShape{0.25f};

        Scene3D _scene;
        Object3D _cameraObject;
        Magnum::SceneGraph::Camera3D* _camera;
        Magnum::SceneGraph::DrawableGroup3D _drawables;
        Magnum::Vector3 _cameraPosition;
        Magnum::Float _cameraPitch;
        Magnum::Float _cameraYaw;
        Magnum::Timeline _timeline;

        Magnum::Ui::UserInterfaceGL _ui{Magnum::NoCreate};
        Magnum::Ui::Label _fpsLabel{Magnum::NoCreate, _ui};

        Magnum::DebugTools::FrameProfilerGL _profiler;
        Magnum::Float _fpsUpdateTimer;
};

class ColoredDrawable: public Magnum::SceneGraph::Drawable3D {
    public:
        explicit ColoredDrawable(Object3D& object, Magnum::Shaders::PhongGL& shader, Magnum::GL::Mesh& mesh, LightVector &lights, const Magnum::Color4& color, Magnum::SceneGraph::DrawableGroup3D& group): Magnum::SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _lights(lights), _color{color} {}

    private:
        void draw(const Magnum::Matrix4& transformationMatrix, Magnum::SceneGraph::Camera3D& camera) override;

        Magnum::Shaders::PhongGL& _shader;
        Magnum::GL::Mesh& _mesh;
        LightVector &_lights;
        Magnum::Color4 _color;
};

class TexturedDrawable: public Magnum::SceneGraph::Drawable3D {
    public:
        explicit TexturedDrawable(Object3D& object, Magnum::Shaders::PhongGL& shader, Magnum::GL::Mesh& mesh, Magnum::GL::Texture2D& texture, Magnum::SceneGraph::DrawableGroup3D& group): Magnum::SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _texture(texture) {}

    private:
        void draw(const Magnum::Matrix4& transformationMatrix, Magnum::SceneGraph::Camera3D& camera) override;

        Magnum::Shaders::PhongGL& _shader;
        Magnum::GL::Mesh& _mesh;
        Magnum::GL::Texture2D& _texture;
};

static const Magnum::Vector3 deriveScale(const Magnum::Matrix4 &mat)
{
    Magnum::Vector3 scale;
    scale.x() = (mat.transformPoint(Magnum::Vector3::xAxis()) - mat.transformPoint(Magnum::Vector3())).length();
    scale.y() = (mat.transformPoint(Magnum::Vector3::yAxis()) - mat.transformPoint(Magnum::Vector3())).length();
    scale.z() = (mat.transformPoint(Magnum::Vector3::zAxis()) - mat.transformPoint(Magnum::Vector3())).length();
    return scale;
}

class RigidBody: public Object3D {
    public:
        RigidBody(Object3D* parent, Magnum::Float mass, const Magnum::Trade::MeshData &meshData, btDynamicsWorld& bWorld): Object3D{parent}, _bWorld(bWorld) {
            const Magnum::Trade::MeshData triangleMesh = Magnum::MeshTools::generateIndices(meshData);
            _meshVertices = triangleMesh.positions3DAsArray();
            _meshIndices = triangleMesh.indicesAsArray();

            {
                btIndexedMesh bulletMesh;
                bulletMesh.m_numTriangles = _meshIndices.size()/3;
                bulletMesh.m_triangleIndexBase = reinterpret_cast<const unsigned char *>(_meshIndices.data());
                bulletMesh.m_triangleIndexStride = 3 * sizeof(Magnum::UnsignedInt);
                bulletMesh.m_numVertices = _meshVertices.size();
                bulletMesh.m_vertexBase = reinterpret_cast<const unsigned char *>(_meshVertices.data());
                bulletMesh.m_vertexStride = sizeof(Magnum::Vector3);
                bulletMesh.m_indexType = PHY_INTEGER;
                bulletMesh.m_vertexType = PHY_FLOAT;

                _pTriMesh.emplace();
                _pTriMesh->addIndexedMesh(bulletMesh, PHY_INTEGER);
            }

            _bShape.emplace(_pTriMesh.get(), true, true);
            const Magnum::Matrix4 mat = absoluteTransformationMatrix();
            const Magnum::Vector3 scale = deriveScale(mat);
            _bShape->setLocalScaling(btVector3(scale.x(), scale.y(), scale.z()));

            // Calculate inertia so the object reacts as it should with rotation and everything
            btVector3 bInertia(0.0f, 0.0f, 0.0f);
            if(!Magnum::Math::TypeTraits<Magnum::Float>::equals(mass, 0.0f))
                _bShape->calculateLocalInertia(mass, bInertia);

            // Bullet rigid body setup
            Magnum::BulletIntegration::MotionState * const motionState = new Magnum::BulletIntegration::MotionState{*this}; // TODO is this leaked?
            _bRigidBody.emplace(btRigidBody::btRigidBodyConstructionInfo{mass, &motionState->btMotionState(), _bShape.get(), bInertia});
            _bRigidBody->forceActivationState(DISABLE_DEACTIVATION);

            bWorld.addRigidBody(_bRigidBody.get());
        }

        ~RigidBody() {
            _bWorld.removeRigidBody(_bRigidBody.get());
        }

        btRigidBody& rigidBody() { return *_bRigidBody; }

        // needed after changing the pose from Magnum side
        void syncPose() {
            _bRigidBody->setWorldTransform(btTransform(transformationMatrix()));
        }

    private:
        btDynamicsWorld& _bWorld;
        Magnum::Containers::Array<Magnum::Vector3> _meshVertices;
        Magnum::Containers::Array<Magnum::UnsignedInt> _meshIndices;
        Magnum::Containers::Pointer<btTriangleIndexVertexArray> _pTriMesh;
        Magnum::Containers::Pointer<btBvhTriangleMeshShape> _bShape;
        Magnum::Containers::Pointer<btRigidBody> _bRigidBody;
};

class RigidBody2: public Object3D {
    public:
        RigidBody2(Object3D* parent, Magnum::Float mass, btCollisionShape* bShape, btDynamicsWorld& bWorld): Object3D{parent}, _bWorld(bWorld) {
            // Calculate inertia so the object reacts as it should with rotation and everything
            btVector3 bInertia(0.0f, 0.0f, 0.0f);
            if(!Magnum::Math::TypeTraits<Magnum::Float>::equals(mass, 0.0f))
                bShape->calculateLocalInertia(mass, bInertia);

            // Bullet rigid body setup
            Magnum::BulletIntegration::MotionState * const motionState = new Magnum::BulletIntegration::MotionState{*this}; // TODO is this leaked?
            _bRigidBody.emplace(btRigidBody::btRigidBodyConstructionInfo{mass, &motionState->btMotionState(), bShape, bInertia});
            _bRigidBody->forceActivationState(DISABLE_DEACTIVATION);

            // btVector3 ballRadius(0.25, 0.25, 0.25); // approximate radius of the ball
            // _bRigidBody->setCcdMotionThreshold( ballRadius.length() );
            // _bRigidBody->setCcdSweptSphereRadius( ballRadius.length() * 0.8 );

            // bWorld.addRigidBody(_bRigidBody.get());
            bWorld.addRigidBody(_bRigidBody.get(), 1, -1);
        }

        ~RigidBody2() {
            _bWorld.removeRigidBody(_bRigidBody.get());
        }

        btRigidBody& rigidBody() { return *_bRigidBody; }

        // needed after changing the pose from Magnum side
        void syncPose() {
            _bRigidBody->setWorldTransform(btTransform(transformationMatrix()));
        }

    private:
        btDynamicsWorld& _bWorld;
        Magnum::Containers::Pointer<btRigidBody> _bRigidBody;
};

static const char GLTF_FILE_PATH[] = "../data/test_scene.gltf";

constexpr const Magnum::Float WidgetHeight = 36.0f;
constexpr const Magnum::Float LabelHeight = 24.0f;
constexpr const Magnum::Vector2 LabelSize{72.0f, LabelHeight};

ViewerExample::ViewerExample(const Arguments& arguments):
    Magnum::Platform::Application{arguments, Configuration{}
        .setTitle("Magnum Viewer Example")
        .setWindowFlags(Configuration::WindowFlag::Resizable)},
    _cameraPosition(3.0, 3.0, 15.0),
    _cameraPitch(0.0),
    _cameraYaw(0.0),
    _fpsUpdateTimer(0.0)
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
            Magnum::Matrix4::perspectiveProjection(Magnum::Math::Literals::operator""_degf(35.0), 1.0f, 0.01f, 1000.0f)
        )
        .setViewport(Magnum::GL::defaultFramebuffer.viewport().size());

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
    _meshes = Magnum::Containers::Array<Magnum::Containers::Optional<Magnum::GL::Mesh>>{
        importer->meshCount()};
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
    }

    /* The format has no scene support, display just the first loaded mesh with
       a default material (if it's there) and be done with it. */
    if(importer->defaultScene() == -1) {
        if(!_meshes.isEmpty() && _meshes[0]) {
            RigidBody * const body = new RigidBody{&_scene, 0.0f, *importer->mesh(0), _bWorld};
            new ColoredDrawable{*body, _coloredShader, *_meshes[0], _lights, Magnum::Color3::fromLinearRgbInt(0xffffff), _drawables};
        }
        return;
    }

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
    for(const Magnum::Containers::Pair<Magnum::UnsignedInt, Magnum::Matrix4>& transformation:
        scene->transformations3DAsArray())
    {
        //Utility::Debug{} << "transformation.first() = " << transformation.first() << "; " << objects[transformation.first()] << "; " << transformation.second();
        if(Object3D* object = objects[transformation.first()])
            object->setTransformation(transformation.second());
    }

    /* Add drawables for objects that have a mesh, again ignoring objects that
       are not part of the hierarchy. There can be multiple mesh assignments
       for one object, simply add one drawable for each. */
    for(const Magnum::Containers::Pair<Magnum::UnsignedInt, Magnum::Containers::Pair<Magnum::UnsignedInt, Magnum::Int>>&
        meshMaterial: scene->meshesMaterialsAsArray())
    {
        Object3D* object = objects[meshMaterial.first()];
        const Magnum::UnsignedInt meshIndex = meshMaterial.second().first();
        Magnum::Containers::Optional<Magnum::GL::Mesh>& mesh = _meshes[meshIndex];
        if(!object || !mesh) continue;

        Magnum::Int materialId = meshMaterial.second().second();

        /* Material not available / not loaded, use a default material */
        if(materialId == -1 || !materials[materialId]) {
            RigidBody * const body = new RigidBody{&_scene, 0.0f, *importer->mesh(meshIndex), _bWorld};
            body->setTransformation(object->absoluteTransformation());
            body->syncPose();
            new ColoredDrawable{*body, _coloredShader, *mesh, _lights, Magnum::Color3::fromLinearRgbInt(0xffffff), _drawables};

        /* Textured material, if the texture loaded correctly */
        } else if(materials[materialId]->hasAttribute(
                Magnum::Trade::MaterialAttribute::DiffuseTexture
            ) && _textures[materials[materialId]->diffuseTexture()])
        {
            new TexturedDrawable{*object, _texturedShader, *mesh,
                *_textures[materials[materialId]->diffuseTexture()],
                _drawables};

        /* Color-only material */
        } else {
            RigidBody * const body = new RigidBody{&_scene, 0.0f, *importer->mesh(meshIndex), _bWorld};
            body->setTransformation(object->absoluteTransformation());
            body->syncPose();
            new ColoredDrawable{*body, _coloredShader, *mesh, _lights, materials[materialId]->diffuseColor(), _drawables};
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

    setCursor(Cursor::HiddenLocked);
    // setSwapInterval(1);
    // setMinimalLoopPeriod(8.0_msec); // max 120Hz?
    _timeline.start();
}

void ColoredDrawable::draw(const Magnum::Matrix4& transformationMatrix, Magnum::SceneGraph::Camera3D& camera) {
    // TODO refactor this code so that the calculations are performed once per camera movement

    // Prepare light positions, colors, and ranges
    Magnum::Containers::Array<Magnum::Vector4> lightPositions{Magnum::NoInit, _lights.size()};
    Magnum::Containers::Array<Magnum::Color3> lightColors{Magnum::NoInit, _lights.size()};
    Magnum::Containers::Array<Magnum::Float> lightRanges{Magnum::NoInit, _lights.size()};

    for (std::size_t i = 0; i < _lights.size(); ++i) {
        const auto& light = _lights[i];
        Magnum::Vector3 position = light.transformation.translation();

        lightPositions[i] = Magnum::Vector4{camera.cameraMatrix().transformPoint(position), light.data.type() == Magnum::Trade::LightType::Directional ? 0.0f : 1.0f};

        lightColors[i] = light.data.color() * light.data.intensity() * 0.01f; // Blender likes to make absurdly huge intensities for some reason, I think it uses different units?

        //lightRanges[i] = light.range; // TODO import this
        lightRanges[i] = Magnum::Constants::inf();
    }

    _shader
        .setDiffuseColor(_color)
        .setLightPositions(lightPositions)
        .setLightColors(lightColors)
        .setLightRanges(lightRanges)
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .draw(_mesh);
}

void TexturedDrawable::draw(const Magnum::Matrix4& transformationMatrix, Magnum::SceneGraph::Camera3D& camera) {
    _shader
        .setLightPositions({
            {camera.cameraMatrix().transformPoint({-3.0f, 10.0f, 10.0f}), 0.0f}
        })
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .bindDiffuseTexture(_texture)
        .draw(_mesh);
}

void ViewerExample::drawEvent() {
    static const Magnum::Float WALK_SPEED = 50.0; // units per second
    const Magnum::Float timeDelta = _timeline.previousFrameDuration();
    const Magnum::Float speedScalar = WALK_SPEED*timeDelta;

    _bWorld.stepSimulation(timeDelta, 5); // TODO move this to tickEvent()?
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
        vel = vel.normalized()*speedScalar;

    const Magnum::Matrix4 horizontalRotationMatrix = Magnum::Matrix4::rotationY(Magnum::Math::Literals::operator""_degf(_cameraYaw));
    const Magnum::Matrix4 verticalRotationMatrix = Magnum::Matrix4::rotationX(Magnum::Math::Literals::operator""_degf(_cameraPitch));
    const Magnum::Matrix4 totalRotationMatrix = horizontalRotationMatrix*verticalRotationMatrix;

    // _cameraPosition += horizontalRotationMatrix.transformVector(vel); // keeps you at the same altitude
    _cameraPosition += totalRotationMatrix.transformVector(vel); // flies around like an airplane

    const Magnum::Matrix4 overallCameraTransform = Magnum::Matrix4::translation(_cameraPosition)*totalRotationMatrix;
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

    _profiler.beginFrame();
    _camera->draw(_drawables);
    Magnum::GL::Renderer::setDepthFunction(Magnum::GL::Renderer::DepthFunction::LessOrEqual);
    _debugDraw.setTransformationProjectionMatrix(_camera->projectionMatrix()*_camera->cameraMatrix());
    _bWorld.debugDrawWorld();
    Magnum::GL::Renderer::setDepthFunction(Magnum::GL::Renderer::DepthFunction::Less);
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

void ViewerExample::focusEvent(FocusEvent& event)
{
    setCursor(Cursor::HiddenLocked);
}

void ViewerExample::blurEvent(FocusEvent& event)
{
    setCursor(Cursor::Arrow);
}

void ViewerExample::keyPressEvent(KeyEvent& event)
{
    if (event.key() == Sdl2Application::Key::Esc)
    {
        exit();
    }
}

void ViewerExample::keyReleaseEvent(KeyEvent& event)
{
    (void)event;
}

void ViewerExample::pointerPressEvent(PointerEvent& event) {
    if(!event.isPrimary())
        return;
    if (!(event.pointer() & (Pointer::MouseLeft|Pointer::Finger)))
        return;
    // const Vector2 position = event.position()*Vector2{framebufferSize()}/Vector2{windowSize()};
    // const Vector2 clickPoint = Vector2::yScale(-1.0f)*(position/Vector2{framebufferSize()} - Vector2{0.5f})*_camera->projectionSize();
    // const Vector3 direction = (_cameraObject->absoluteTransformation().rotationScaling()*Vector3{clickPoint, -1.0f}).normalized();
    RigidBody2 * const body = new RigidBody2{&_scene, 5.0f, &_bSphereShape, _bWorld};
    // body->setTransformation(object->absoluteTransformation());
    body->translate(_cameraObject.absoluteTransformation().translation());
    body->syncPose();
    // new ColoredDrawable{*body, _coloredShader, *mesh, _lights, materials[materialId]->diffuseColor(), _drawables};

    // body->rigidBody().setLinearVelocity(btVector3{direction*25.f});
    // body->rigidBody().setLinearVelocity(btVector3{0.0, 0.0, 25.0f});

    event.setAccepted();
}

void ViewerExample::pointerReleaseEvent(PointerEvent& event) {
    if(!event.isPrimary())
        return;
    // if (event.pointer() & (Pointer::MouseLeft|Pointer::Finger)) {}
}

void ViewerExample::scrollEvent(ScrollEvent& event) {
    if(!event.offset().y()) return;

    /* Distance to origin */
    // const Magnum::Float distance = _cameraObject.transformation().translation().z();

    /* Move 15% of the distance back or forward */
    // _cameraObject.translate(Magnum::Vector3::zAxis(
        // distance*(1.0f - (event.offset().y() > 0 ? 1/0.85f : 0.85f))));

    // redraw();
}

void ViewerExample::pointerMoveEvent(PointerMoveEvent& event) {
    if(!event.isPrimary())
        return;

    const Magnum::Vector2 currentPosition = event.position();
    const Magnum::Vector2 delta = event.relativePosition();

    if(delta.isZero())
        return;

    _cameraYaw  -= delta.x() * 0.1f;
    _cameraPitch -= delta.y() * 0.1f;
    _cameraPitch = Magnum::Math::clamp(_cameraPitch, -89.0f, 89.0f);

    redraw();
}

MAGNUM_APPLICATION_MAIN(ViewerExample)

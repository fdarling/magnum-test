#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Arguments.h>
//#include <Corrade/Utility/Debug.h>
#include <Corrade/Utility/DebugStl.h>

#include <Magnum/ImageView.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Color.h>
#include <Magnum/MeshTools/Compile.h>
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
// #include <MagnumPlugins/GltfImporter/GltfImporter.h>

#include <vector>

using namespace Magnum;
using namespace Math::Literals;

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

struct LightInfo {
    //Trade::LightData data; // apparently can't be copied... so we copy the members instead!
    Trade::LightType type;
    Color3 color;
    Float intensity;
    Matrix4 transformation;
};

//typedef Containers::Array<LightInfo> LightVector; _lights;
typedef std::vector<LightInfo> LightVector; // TODO possibly convert to using Containers::Array<>

class ViewerExample: public Platform::Application {
    public:
        explicit ViewerExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void viewportEvent(ViewportEvent& event) override;
        void pointerPressEvent(PointerEvent& event) override;
        void pointerReleaseEvent(PointerEvent& event) override;
        void pointerMoveEvent(PointerMoveEvent& event) override;
        void scrollEvent(ScrollEvent& event) override;

        Vector3 positionOnSphere(const Vector2& position) const;

        Shaders::PhongGL _coloredShader;
        Shaders::PhongGL _texturedShader{Shaders::PhongGL::Configuration{}
            .setFlags(Shaders::PhongGL::Flag::DiffuseTexture)};
        Containers::Array<Containers::Optional<GL::Mesh>> _meshes;
        Containers::Array<Containers::Optional<GL::Texture2D>> _textures;
        LightVector _lights;

        Scene3D _scene;
        Object3D _manipulator, _cameraObject;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;
        Vector3 _previousPosition;
};

class ColoredDrawable: public SceneGraph::Drawable3D {
    public:
        explicit ColoredDrawable(Object3D& object, Shaders::PhongGL& shader, GL::Mesh& mesh, LightVector &lights, const Color4& color, SceneGraph::DrawableGroup3D& group): SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _lights(lights), _color{color} {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Shaders::PhongGL& _shader;
        GL::Mesh& _mesh;
        LightVector &_lights;
        Color4 _color;
};

class TexturedDrawable: public SceneGraph::Drawable3D {
    public:
        explicit TexturedDrawable(Object3D& object, Shaders::PhongGL& shader, GL::Mesh& mesh, GL::Texture2D& texture, SceneGraph::DrawableGroup3D& group): SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _texture(texture) {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Shaders::PhongGL& _shader;
        GL::Mesh& _mesh;
        GL::Texture2D& _texture;
};

static const char GLTF_FILE_PATH[] = "../data/test_scene.gltf";

ViewerExample::ViewerExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("Magnum Viewer Example")
        .setWindowFlags(Configuration::WindowFlag::Resizable)}
{
    _cameraObject
        .setParent(&_manipulator)
        .translate(Vector3::zAxis(5.0f));
    (*(_camera = new SceneGraph::Camera3D{_cameraObject}))
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(
            Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.01f, 1000.0f)
        )
        .setViewport(GL::defaultFramebuffer.viewport().size());

    _manipulator.setParent(&_scene);

    /* Setup renderer and shader defaults */
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    _coloredShader
        .setAmbientColor(0x111111_rgbf)
        .setSpecularColor(0xffffff_rgbf)
        .setShininess(80.0f);
    _texturedShader
        .setAmbientColor(0x111111_rgbf)
        .setSpecularColor(0x111111_rgbf)
        .setShininess(80.0f);

    /* Load a scene importer plugin */
    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer = manager.loadAndInstantiate("GltfImporter");

    if(!importer || !importer->openFile(GLTF_FILE_PATH))
        std::exit(1);

    /* Load all textures. Textures that fail to load will be NullOpt. */
    _textures = Containers::Array<Containers::Optional<GL::Texture2D>>{
        importer->textureCount()};
    for(UnsignedInt i = 0; i != importer->textureCount(); ++i) {
        Containers::Optional<Trade::TextureData> textureData =
            importer->texture(i);
        if(!textureData || textureData->type() != Trade::TextureType::Texture2D) {
            Warning{} << "Cannot load texture" << i
                << importer->textureName(i);
            continue;
        }

        Containers::Optional<Trade::ImageData2D> imageData =
            importer->image2D(textureData->image());
        if(!imageData || imageData->isCompressed()) {
            Warning{} << "Cannot load image" << textureData->image()
                << importer->image2DName(textureData->image());
            continue;
        }

        (*(_textures[i] = GL::Texture2D{}))
            .setMagnificationFilter(textureData->magnificationFilter())
            .setMinificationFilter(textureData->minificationFilter(),
                                   textureData->mipmapFilter())
            .setWrapping(textureData->wrapping().xy())
            .setStorage(Math::log2(imageData->size().max()) + 1,
                GL::textureFormat(imageData->format()), imageData->size())
            .setSubImage(0, {}, *imageData)
            .generateMipmap();
    }

    /* Load all materials. Materials that fail to load will be NullOpt. Only a
       temporary array as the material attributes will be stored directly in
       drawables later. */
    Containers::Array<Containers::Optional<Trade::PhongMaterialData>> materials{
        importer->materialCount()};
    for(UnsignedInt i = 0; i != importer->materialCount(); ++i) {
        Containers::Optional<Trade::MaterialData> materialData;
        if(!(materialData = importer->material(i))) {
            Warning{} << "Cannot load material" << i
                << importer->materialName(i);
            continue;
        }

        materials[i] = std::move(*materialData).as<Trade::PhongMaterialData>();
    }

    /* Load all meshes. Meshes that fail to load will be NullOpt. Generate
       normals if not present. */
    _meshes = Containers::Array<Containers::Optional<GL::Mesh>>{
        importer->meshCount()};
    for(UnsignedInt i = 0; i != importer->meshCount(); ++i) {
        Containers::Optional<Trade::MeshData> meshData;
        if(!(meshData = importer->mesh(i))) {
            Warning{} << "Cannot load mesh" << i << importer->meshName(i);
            continue;
        }

        MeshTools::CompileFlags flags;
        if(!meshData->hasAttribute(Trade::MeshAttribute::Normal))
            flags |= MeshTools::CompileFlag::GenerateFlatNormals;
        _meshes[i] = MeshTools::compile(*meshData, flags);
    }

    /* The format has no scene support, display just the first loaded mesh with
       a default material (if it's there) and be done with it. */
    if(importer->defaultScene() == -1) {
        if(!_meshes.isEmpty() && _meshes[0])
            new ColoredDrawable{_manipulator, _coloredShader, *_meshes[0],
                _lights, 0xffffff_rgbf, _drawables};
        return;
    }

    /* Load the scene */
    Containers::Optional<Trade::SceneData> scene;
    if(!(scene = importer->scene(importer->defaultScene())) ||
       !scene->is3D() ||
       !scene->hasField(Trade::SceneField::Parent) ||
       !scene->hasField(Trade::SceneField::Mesh))
    {
        Fatal{} << "Cannot load scene" << importer->defaultScene()
            << importer->sceneName(importer->defaultScene());
    }

    /* Allocate objects that are part of the hierarchy */
    Containers::Array<Object3D*> objects{std::size_t(scene->mappingBound())};
    Containers::Array<Containers::Pair<UnsignedInt, Int>> parents
        = scene->parentsAsArray();
    for(const Containers::Pair<UnsignedInt, Int>& parent: parents)
        objects[parent.first()] = new Object3D{};

    /* Assign parent references */
    for(const Containers::Pair<UnsignedInt, Int>& parent: parents)
        objects[parent.first()]->setParent(parent.second() == -1 ?
            &_scene : objects[parent.second()]);

    /* Set transformations. Objects that are not part of the hierarchy are
       ignored, objects that have no transformation entry retain an identity
       transformation. */
    for(const Containers::Pair<UnsignedInt, Matrix4>& transformation:
        scene->transformations3DAsArray())
    {
        //Utility::Debug{} << "transformation.first() = " << transformation.first() << "; " << objects[transformation.first()] << "; " << transformation.second();
        if(Object3D* object = objects[transformation.first()])
            object->setTransformation(transformation.second());
    }

    /* Add drawables for objects that have a mesh, again ignoring objects that
       are not part of the hierarchy. There can be multiple mesh assignments
       for one object, simply add one drawable for each. */
    for(const Containers::Pair<UnsignedInt, Containers::Pair<UnsignedInt, Int>>&
        meshMaterial: scene->meshesMaterialsAsArray())
    {
        Object3D* object = objects[meshMaterial.first()];
        Containers::Optional<GL::Mesh>& mesh =
            _meshes[meshMaterial.second().first()];
        if(!object || !mesh) continue;

        Int materialId = meshMaterial.second().second();

        /* Material not available / not loaded, use a default material */
        if(materialId == -1 || !materials[materialId]) {
            new ColoredDrawable{*object, _coloredShader, *mesh, _lights, 0xffffff_rgbf,
                _drawables};

        /* Textured material, if the texture loaded correctly */
        } else if(materials[materialId]->hasAttribute(
                Trade::MaterialAttribute::DiffuseTexture
            ) && _textures[materials[materialId]->diffuseTexture()])
        {
            new TexturedDrawable{*object, _texturedShader, *mesh,
                *_textures[materials[materialId]->diffuseTexture()],
                _drawables};

        /* Color-only material */
        } else {
            new ColoredDrawable{*object, _coloredShader, *mesh,
                _lights, materials[materialId]->diffuseColor(), _drawables};
        }
    }

    for(const Containers::Pair<UnsignedInt, UnsignedInt> &lightPair : scene->lightsAsArray())
    {
        //Utility::Debug{} << "lightPair: " << lightPair.first() << " " << lightPair.second();

        if(Object3D * const object = objects[lightPair.first()])
        {
            Containers::Optional<Trade::LightData> data = importer->light(lightPair.second());
            if (!data)
                continue;
            _lights.emplace_back(LightInfo{
                //.data = *data,
                .type = data->type(),
                .color = data->color(),
                .intensity = data->intensity(),
                // TODO range and attentuation?
                .transformation = object->absoluteTransformationMatrix()
            });
            //Utility::Debug{} << "light using transformation: " << _lights.back().transformation;
        }
    }
}

void ColoredDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    // TODO refactor this code so that the calculations are performed once per camera movement

    // Prepare light positions, colors, and ranges
    Containers::Array<Vector4> lightPositions{NoInit, _lights.size()};
    Containers::Array<Color3> lightColors{NoInit, _lights.size()};
    Containers::Array<Float> lightRanges{NoInit, _lights.size()};

    for (std::size_t i = 0; i < _lights.size(); ++i) {
        const auto& light = _lights[i];
        // Transform light position to camera space
        Vector3 position = light.transformation.translation();
        // Vector4 positionHomogeneous{position, light.type == Trade::LightType::Directional ? 0.0f : 1.0f};

        // lightPositions[i] = camera.cameraMatrix() * positionHomogeneous;
        lightPositions[i] = Vector4{camera.cameraMatrix().transformPoint(position), light.type == Trade::LightType::Directional ? 0.0f : 1.0f};
        //lightPositions[i] = camera.projectionMatrix() * positionHomogeneous;
        //lightPositions[i] = positionHomogeneous;

        lightColors[i] = light.color * light.intensity * 0.01f;
        //lightColors[i] = light.color;

        //lightRanges[i] = light.range;
        lightRanges[i] = Constants::inf();
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

void TexturedDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
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
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|
                                 GL::FramebufferClear::Depth);

    _camera->draw(_drawables);

    swapBuffers();
}

void ViewerExample::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
    _camera->setViewport(event.windowSize());
}

void ViewerExample::pointerPressEvent(PointerEvent& event) {
    if(!event.isPrimary() ||
       !(event.pointer() & (Pointer::MouseLeft|Pointer::Finger)))
        return;

    _previousPosition = positionOnSphere(event.position());
}

void ViewerExample::pointerReleaseEvent(PointerEvent& event) {
    if(!event.isPrimary() ||
       !(event.pointer() & (Pointer::MouseLeft|Pointer::Finger)))
        return;

    _previousPosition = {};
}

void ViewerExample::scrollEvent(ScrollEvent& event) {
    if(!event.offset().y()) return;

    /* Distance to origin */
    const Float distance = _cameraObject.transformation().translation().z();

    /* Move 15% of the distance back or forward */
    _cameraObject.translate(Vector3::zAxis(
        distance*(1.0f - (event.offset().y() > 0 ? 1/0.85f : 0.85f))));

    redraw();
}

Vector3 ViewerExample::positionOnSphere(const Vector2& position) const {
    const Vector2 positionNormalized =
        position/Vector2{_camera->viewport()} - Vector2{0.5f};
    const Float length = positionNormalized.length();
    const Vector3 result = length > 1.0f ?
        Vector3{positionNormalized, 0.0f} :
        Vector3{positionNormalized, 1.0f - length};
    return (result*Vector3::yScale(-1.0f)).normalized();
}

void ViewerExample::pointerMoveEvent(PointerMoveEvent& event) {
    if(!event.isPrimary() ||
       !(event.pointers() & (Pointer::MouseLeft|Pointer::Finger)))
        return;

    const Vector3 currentPosition = positionOnSphere(event.position());
    const Vector3 axis = Math::cross(_previousPosition, currentPosition);

    if(_previousPosition.isZero() || axis.isZero())
        return;

    _manipulator.rotate(-Math::angle(_previousPosition, currentPosition), axis.normalized());
    _previousPosition = currentPosition;

    redraw();
}

MAGNUM_APPLICATION_MAIN(ViewerExample)

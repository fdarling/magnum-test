# Dependencies

## Corrade

```
git clone https://github.com/mosra/corrade.git
cd corrade
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=~/apps/magnum
cmake --build . --config Release --target install
```

## Magnum

```
git clone https://github.com/mosra/magnum.git
cd magnum
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=~/apps/magnum -DCMAKE_INSTALL_PREFIX=~/apps/magnum -DMAGNUM_WITH_SDL2APPLICATION=ON -DMAGNUM_WITH_ANYSCENEIMPORTER=ON -DMAGNUM_WITH_ANYIMAGEIMPORTER=ON
cmake --build . --config Release --target install
```

## Magnum Integration

```
git clone https://github.com/mosra/magnum-integration.git
cd magnum-integration
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=~/apps/magnum -DCMAKE_INSTALL_PREFIX=~/apps/magnum -DMAGNUM_WITH_BULLET=ON
cmake --build . --config Release --target install
```

## Magnum Plugins

```
git clone https://github.com/mosra/magnum-plugins.git
cd magnum-plugins
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=~/apps/magnum -DCMAKE_INSTALL_PREFIX=~/apps/magnum -DMAGNUM_WITH_GLTFIMPORTER=ON
cmake --build . --config Release --target install
```

## Magnum Examples

```
git clone https://github.com/mosra/magnum-examples.git
cd magnum-examples
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=~/apps/magnum -DCMAKE_INSTALL_PREFIX=~/apps/magnum-examples -DMAGNUM_WITH_BOX2D_EXAMPLE=ON -DMAGNUM_WITH_BULLET_EXAMPLE=ON
```

```
# from within the magnum-examples/build directory:
LD_LIBRARY_PATH=~/apps/magnum/lib bin/magnum-bullet
```

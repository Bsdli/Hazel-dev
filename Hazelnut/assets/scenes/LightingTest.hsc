Scene: Scene Name
Environment:
  AssetPath: assets\env\birchwood_4k.hdr
  Light:
    Direction: [-0.788999975, 0.777999997, -0.782999992]
    Radiance: [1, 0.999989986, 0.999989986]
    Multiplier: 0.651000023
Entities:
  - Entity: 15213035846546605980
    TagComponent:
      Tag: Sponza
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, -0, 0]
      Scale: [0.100000001, 0.100000001, 0.100000001]
    MeshComponent:
      AssetPath: assets\meshes\sponza\sponza.obj
    RigidBodyComponent:
      BodyType: 0
      Mass: 1
      IsKinematic: false
      Layer: 0
      Constraints:
        LockPositionX: false
        LockPositionY: false
        LockPositionZ: false
        LockRotationX: false
        LockRotationY: false
        LockRotationZ: false
    PhysicsMaterialComponent:
      StaticFriction: 1
      DynamicFriction: 1
      Bounciness: 0
    MeshColliderComponent:
      AssetPath: assets\meshes\sponza\sponza.obj
      IsConvex: false
      IsTrigger: false
  - Entity: 3370882484222092056
    TagComponent:
      Tag: Directional Light
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [0.99999541, 0.999995649, 0.999997437]
    DirectionalLightComponent:
      Radiance: [1, 1, 1]
      CastShadows: false
      SoftShadows: false
      LightSize: 0.5
  - Entity: 6605693014656896610
    TagComponent:
      Tag: Sky Light
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    SkyLightComponent:
      EnvironmentAssetPath: assets\env\birchwood_4k.hdr
      Intensity: 1
      Angle: 0
  - Entity: 6988676893832268174
    TagComponent:
      Tag: Falling Cube
    TransformComponent:
      Position: [-2.29486045e-06, 191.54834, -1.20268439e-28]
      Rotation: [1.13830311e-06, 0, 4.45406357e-07]
      Scale: [5.00000381, 5.00001049, 5.00000381]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBodyComponent:
      BodyType: 1
      Mass: 1
      IsKinematic: false
      Layer: 0
      Constraints:
        LockPositionX: false
        LockPositionY: false
        LockPositionZ: false
        LockRotationX: false
        LockRotationY: false
        LockRotationZ: false
    PhysicsMaterialComponent:
      StaticFriction: 1
      DynamicFriction: 1
      Bounciness: 1
    BoxColliderComponent:
      Offset: [0, 0, 0]
      Size: [1, 1, 1]
      IsTrigger: false
  - Entity: 13132747235800255834
    TagComponent:
      Tag: Camera
    TransformComponent:
      Position: [85.8089981, 208.961517, 0]
      Rotation: [-0.738274276, 1.57079637, 0]
      Scale: [0.999979556, 0.999987841, 0.999982357]
    CameraComponent:
      Camera: some camera data...
      Primary: true
PhysicsLayers:
  []
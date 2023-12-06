Scene: Scene Name
Environment:
  AssetPath: assets\env\birchwood_4k.hdr
  Light:
    Direction: [0, 0, 0]
    Radiance: [0, 0, 0]
    Multiplier: 1
Entities:
  - Entity: 10790878375657664772
    TagComponent:
      Tag: Sphere
    TransformComponent:
      Position: [0, 0, -0.54931438]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    MeshComponent:
      AssetPath: assets\meshes\Sphere1m.fbx
    RigidBodyComponent:
      BodyType: 1
      Mass: 1
      LinearDrag: 0
      AngularDrag: 0.0500000007
      DisableGravity: false
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
    SphereColliderComponent:
      Radius: 0.5
      IsTrigger: false
  - Entity: 7469277527539248582
    TagComponent:
      Tag: Gun
    TransformComponent:
      Position: [0, -0.305182159, 1.95935249]
      Rotation: [0, 0, 0]
      Scale: [10, 10, 10]
    MeshComponent:
      AssetPath: E:\Hazel\Hazelnut\assets\models\m1911\m1911.fbx
  - Entity: 4833401366981862221
    TagComponent:
      Tag: Plane
    TransformComponent:
      Position: [0, -1.2042129, 0]
      Rotation: [0, 0, 0]
      Scale: [0.100000001, 0.100000001, 0.100000001]
    MeshComponent:
      AssetPath: E:\Hazel\Hazelnut\assets\models\Plane1m.fbx
  - Entity: 13472004502121776508
    TagComponent:
      Tag: Stormtrooper
    TransformComponent:
      Position: [9.26335815e-07, -1.19368815, -2.78513813]
      Rotation: [-3.14159226, -1.45079136, -3.14159226]
      Scale: [1, 1, 1]
    MeshComponent:
      AssetPath: E:\Hazel\Hazelnut\assets\meshes\stormtrooper\silly_dancing.fbx
  - Entity: 13882838760121718506
    TagComponent:
      Tag: Directional Light
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [2.50669408, 0.501803458, 0.136891216]
      Scale: [1, 0.999998569, 0.999998987]
    DirectionalLightComponent:
      Radiance: [1, 1, 1]
      CastShadows: true
      SoftShadows: true
      LightSize: 0.5
  - Entity: 2143608321399101581
    TagComponent:
      Tag: Sky Light
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    SkyLightComponent:
      EnvironmentAssetPath: assets\env\birchwood_4k.hdr
      Intensity: 0.75
      Angle: 0
  - Entity: 12392346523579991874
    TagComponent:
      Tag: Camera
    TransformComponent:
      Position: [0, 1.0587908, 0]
      Rotation: [0, 0.785398185, 0]
      Scale: [1, 1, 1]
    CameraComponent:
      Camera:
        ProjectionType: 0
        PerspectiveFOV: 65
        PerspectiveNear: 0.100000001
        PerspectiveFar: 1000
        OrthographicSize: 10
        OrthographicNear: -1
        OrthographicFar: 1
      Primary: true
PhysicsLayers:
  []
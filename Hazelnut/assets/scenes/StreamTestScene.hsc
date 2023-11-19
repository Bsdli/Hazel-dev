Scene: Scene Name
Environment:
  AssetPath: assets\env\pink_sunrise_4k.hdr
  Light:
    Direction: [-0.442000002, -0.58099997, -0.451999992]
    Radiance: [1, 0.999989986, 0.999989986]
    Multiplier: 1
Entities:
  - Entity: 16879531909728311346
    TagComponent:
      Tag: Main Cube
    TransformComponent:
      Position: [0, 14.43297, -7.4505806e-09]
      Rotation: [1, 0, 0, 0]
      Scale: [4, 4, 1]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [2, 2]
  - Entity: 15439361105258763253
    TagComponent:
      Tag: Main Cube
    TransformComponent:
      Position: [13.6415138, 2.8245554, -3.03643901e-08]
      Rotation: [0.960835755, 0, 0, 0.277118564]
      Scale: [36, 3.99999952, 4]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 0
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [18, 2]
  - Entity: 14445970739577976395
    TagComponent:
      Tag: Main Cube
    TransformComponent:
      Position: [-10.8582497, 1.71247244, 2.49667664e-08]
      Rotation: [0.981936872, 0, 0, -0.18920885]
      Scale: [36, 4.00000238, 4]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 0
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [18, 2]
  - Entity: 10482468196881171404
    TagComponent:
      Tag: Main Cube
    TransformComponent:
      Position: [0, 91.4110565, 0]
      Rotation: [0.942232013, 0, 0, 0.334960997]
      Scale: [0.359999985, 0.360000014, 0.360000014]
    MeshComponent:
      AssetPath: assets\meshes\cerberus\CerberusMaterials.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [1, 1]
  - Entity: 3586463467297568945
    TagComponent:
      Tag: Camera
    TransformComponent:
      Position: [0, 13.8065729, 46]
      Rotation: [0.996565521, -0.0828081891, 1.85624849e-09, 1.54242313e-10]
      Scale: [1, 1, 1]
    CameraComponent:
      Camera: some camera data...
      Primary: true
    RigidBody2DComponent:
      BodyType: 1
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [1, 1]
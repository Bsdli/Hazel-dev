Scene: Scene Name
Environment:
  AssetPath: assets\env\pink_sunrise_4k.hdr
  Light:
    Direction: [-0.787, -0.73299998, 1]
    Radiance: [1, 1, 1]
    Multiplier: 0.514999986
Entities:
  - Entity: 12498244675852797835
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [-12.0348625, 6.59647179, 9.60061925e-07]
      Rotation: [1, 0, 0, 0]
      Scale: [3.00000024, 0.300000012, 1]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 0
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [1.5, 0.150000006]
      Density: 1
      Friction: 1
  - Entity: 5178862374589434728
    TagComponent:
      Tag: Camera
    TransformComponent:
      Position: [-21.7406311, 9.70659542, 15]
      Rotation: [0.999910355, -0.0133911213, 0, 0]
      Scale: [1, 1, 1]
    ScriptComponent:
      ModuleName: Example.BasicController
      StoredFields:
        - Name: Speed
          Type: 1
          Data: 12
    CameraComponent:
      Camera: some camera data...
      Primary: true
  - Entity: 1289165777996378215
    TagComponent:
      Tag: Cube
    TransformComponent:
      Position: [500, 0, 0]
      Rotation: [1, 0, 0, 0]
      Scale: [1200, 1, 5]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 0
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [600, 0.5]
      Density: 1
      Friction: 2
  - Entity: 14057422478420564497
    TagComponent:
      Tag: Player
    TransformComponent:
      Position: [-23.6932545, 1.59184527, -1.96369365e-06]
      Rotation: [1, 0, 0, 0]
      Scale: [1, 1, 1]
    ScriptComponent:
      ModuleName: Example.PlayerCube
      StoredFields:
        - Name: HorizontalForce
          Type: 1
          Data: 0.5
        - Name: MaxSpeed
          Type: 5
          Data: [7, 10]
        - Name: JumpForce
          Type: 1
          Data: 3
    MeshComponent:
      AssetPath: assets\meshes\Sphere1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 29.2000008
    CircleCollider2DComponent:
      Offset: [0, 0]
      Radius: 0.5
      Density: 1
      Friction: 1
  - Entity: 1352995477042327524
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [-29.6808929, 29.7597198, 0]
      Rotation: [0.707106769, 0, 0, 0.707106769]
      Scale: [58.4179001, 4.47999144, 4.48000002]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 0
      Mass: 3
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [29.7000008, 2.24000001]
      Density: 1
      Friction: 1
  - Entity: 15223077898852293773
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [6.12674046, 45.5617676, 0]
      Rotation: [0.977883637, 0, 0, -0.209149584]
      Scale: [4.47999668, 4.47999668, 4.48000002]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [2.24000001, 2.24000001]
      Density: 1
      Friction: 1
  - Entity: 5421735812495444456
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [-20.766222, 2.29431438, 0]
      Rotation: [1, 0, 0, 0]
      Scale: [3.00000024, 0.300000012, 1]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 0
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [1.5, 0.150000006]
      Density: 1
      Friction: 1
  - Entity: 2842299641876190180
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [-16.6143265, 4.39151001, 6.43359499e-09]
      Rotation: [1, 0, 0, 0]
      Scale: [3.00000024, 0.300000012, 1]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 0
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [1.5, 0.150000006]
      Density: 1
      Friction: 1
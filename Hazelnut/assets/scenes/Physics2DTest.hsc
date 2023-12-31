Scene: Scene Name
Environment:
  AssetPath: assets\env\pink_sunrise_4k.hdr
  Light:
    Direction: [-0.787, -0.73299998, 1]
    Radiance: [1, 1, 1]
    Multiplier: 0.514999986
Entities:
  - Entity: 15861629587505754
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [-18.2095661, 39.2518234, 0]
      Rotation: [0, 0, 0]
      Scale: [4.47999525, 4.47999525, 4.48000002]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [2.24000001, 2.24000001]
  - Entity: 15223077898852293773
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [5.37119865, 43.8762894, 0]
      Rotation: [0, 0, 0]
      Scale: [4.47999668, 4.47999668, 4.48000002]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [2.24000001, 2.24000001]
  - Entity: 2157107598622182863
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [-7.60411549, 44.1442184, 0]
      Rotation: [0, 0, 0]
      Scale: [4.47999287, 4.47999287, 4.48000002]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 0.5
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [2.24000001, 2.24000001]
  - Entity: 8080964283681139153
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [-0.739211679, 37.7653275, 0]
      Rotation: [0, 0, 0]
      Scale: [5, 2, 2]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 0.25
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [2.5, 1]
  - Entity: 1352995477042327524
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [-8.32969856, 30.4078159, 0]
      Rotation: [0, 0, 0]
      Scale: [14.000001, 4.47999334, 4.48000002]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 3
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [7, 2.24000001]
  - Entity: 935615878363259513
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [6.88031197, 31.942337, 0]
      Rotation: [0, 0, 0]
      Scale: [4.47999954, 4.47999954, 4.48000002]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [2.24000001, 2.24000001]
  - Entity: 14057422478420564497
    TagComponent:
      Tag: Player
    TransformComponent:
      Position: [0, 22.774044, 0]
      Rotation: [0, 0, 0]
      Scale: [6.00000048, 6.00000048, 4.48000002]
    ScriptComponent:
      ModuleName: Example.PlayerCube
      StoredFields:
        - Name: HorizontalForce
          Type: 1
          Data: 10
        - Name: VerticalForce
          Type: 1
          Data: 10
    MeshComponent:
      AssetPath: assets\meshes\Sphere1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 1
    CircleCollider2DComponent:
      Offset: [0, 0]
      Radius: 3
  - Entity: 1289165777996378215
    TagComponent:
      Tag: Cube
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [50, 1, 50]
    ScriptComponent:
      ModuleName: Example.Sink
      StoredFields:
        - Name: SinkSpeed
          Type: 1
          Data: 0
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 0
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [25, 0.5]
  - Entity: 5178862374589434728
    TagComponent:
      Tag: Camera
    TransformComponent:
      Position: [0, 25, 79.75]
      Rotation: [0, 0, 0]
      Scale: [1, 0.999999821, 0.999999821]
    ScriptComponent:
      ModuleName: Example.BasicController
      StoredFields:
        - Name: Speed
          Type: 1
          Data: 12
    CameraComponent:
      Camera: some camera data...
      Primary: true
  - Entity: 3948844418381294888
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [-1.48028564, 49.5945244, -2.38418579e-07]
      Rotation: [0, 0, 0]
      Scale: [1.99999976, 1.99999976, 2]
    MeshComponent:
      AssetPath: assets\meshes\Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [1, 1]
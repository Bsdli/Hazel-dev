Scene: Scene Name
Environment:
  AssetPath: D:\Programming\C++\HazelPhysics\Hazelnut\assets\env\birchwood_4k.hdr
  Light:
    Direction: [0, 0, 0]
    Radiance: [0, 0, 0]
    Multiplier: 1
Entities:
  - Entity: 11200188194937041682
    TagComponent:
      Tag: Sponza
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [0.00999999978, 0.00999999978, 0.00999999978]
    MeshComponent:
      AssetPath: D:\Programming\C++\HazelPhysics\Hazelnut\assets\meshes\sponza\sponza.obj
  - Entity: 151530369103844076
    TagComponent:
      Tag: Directional Light
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [1.39626336, 0.17453292, 0]
      Scale: [0, 0, 0]
    DirectionalLightComponent:
      Radiance: [1, 1, 1]
      CastShadows: true
      SoftShadows: true
      LightSize: 0.5
  - Entity: 17685584250575918719
    TagComponent:
      Tag: Sky Light
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [0, 0, 0]
    SkyLightComponent:
      EnvironmentAssetPath: D:\Programming\C++\HazelPhysics\Hazelnut\assets\env\birchwood_4k.hdr
      Intensity: 1
      Angle: 0
PhysicsLayers:
  []
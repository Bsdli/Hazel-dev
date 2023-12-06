using System;

using Hazel;

namespace Example
{
    public class BasicController : Entity
    {
        public float Speed;
        public float DistanceFromPlayer = 20.0F;

        private Entity m_PlayerEntity;

        public void OnCreate()
        {
            m_PlayerEntity = FindEntityByTag("Player");
        }

        public void OnUpdate(float ts)
        {
            /*Matrix4 transform = GetTransform();

            Vector3 playerTranslation = m_PlayerEntity.GetTransform().Translation;
            Vector3 translation = transform.Translation;
            translation.XY = playerTranslation.XY;
            translation.Z = playerTranslation.Z + DistanceFromPlayer;
            translation.Y = Math.Max(translation.Y, 2.0f);
            transform.Translation = translation;
            SetTransform(transform);*/
        }
    }
}

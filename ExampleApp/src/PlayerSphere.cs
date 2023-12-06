using System;

using Hazel;

namespace Example
{
    class PlayerSphere : Entity
    {
        public float HorizontalForce = 10.0f;
        public float JumpForce = 10.0f;

        private RigidBodyComponent m_PhysicsBody;
        private MaterialInstance m_MeshMaterial;

        int m_CollisionCounter = 0;

        public Vector3 MaxSpeed = new Vector3();

        private bool Colliding => m_CollisionCounter > 0;

        private TransformComponent m_Transform;

        void OnCreate()
        {
            m_PhysicsBody = GetComponent<RigidBodyComponent>();
            m_Transform = GetComponent<TransformComponent>();

            MeshComponent meshComponent = GetComponent<MeshComponent>();
            m_MeshMaterial = meshComponent.Mesh.GetMaterial(0);
            m_MeshMaterial.Set("u_Metalness", 0.0f);

            AddCollisionBeginCallback(OnPlayerCollisionBegin);
            AddCollisionEndCallback(OnPlayerCollisionEnd);
            AddTriggerBeginCallback(OnPlayerTriggerBegin);
			AddTriggerEndCallback(OnPlayerTriggerEnd);
		}

        void OnPlayerCollisionBegin(float value)
        {
            m_CollisionCounter++;
        }

        void OnPlayerCollisionEnd(float value)
        {
            m_CollisionCounter--;
        }

        void OnPlayerTriggerBegin(float value)
		{
		}

		void OnPlayerTriggerEnd(float value)
		{
		}

		void OnUpdate(float ts)
        {
            float movementForce = HorizontalForce;

            if (!Colliding)
            {
                movementForce *= 0.4f;
            }

			if (Input.IsKeyPressed(KeyCode.W))
				m_PhysicsBody.AddForce(m_Transform.Transform.Forward * movementForce);
			else if (Input.IsKeyPressed(KeyCode.S))
				m_PhysicsBody.AddForce(m_Transform.Transform.Forward * -movementForce);

			if (Input.IsKeyPressed(KeyCode.D))
				m_PhysicsBody.AddForce(m_Transform.Transform.Right * movementForce);
			else if (Input.IsKeyPressed(KeyCode.A))
				m_PhysicsBody.AddForce(m_Transform.Transform.Right * -movementForce);

			if (Colliding && Input.IsKeyPressed(KeyCode.Space))
                m_PhysicsBody.AddForce(m_Transform.Transform.Up * JumpForce);

            if (Colliding)
                m_MeshMaterial.Set("u_AlbedoColor", new Vector3(1.0f, 0.0f, 0.0f));
            else
                m_MeshMaterial.Set("u_AlbedoColor", new Vector3(0.8f, 0.8f, 0.8f));

            Vector3 linearVelocity = m_PhysicsBody.GetLinearVelocity();
            linearVelocity.Clamp(new Vector3(-MaxSpeed.X, -1000, -MaxSpeed.Z), MaxSpeed);
            m_PhysicsBody.SetLinearVelocity(linearVelocity);

            /*if (Input.IsKeyPressed(KeyCode.R))
            {
                Matrix4 transform = GetTransform();
                transform.Translation = new Vector3(0.0f);
                SetTransform(transform);
            }*/

        }

    }
}

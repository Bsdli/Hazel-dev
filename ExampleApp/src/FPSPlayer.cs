using System;
using Hazel;

namespace FPSExample
{
	public class FPSPlayer : Entity
	{
		public float WalkingSpeed = 10.0F;
		public float RunSpeed = 20.0F;
		public float JumpForce = 50.0F;

		[NonSerialized]
		public float MouseSensitivity = 10.0F;

		private bool m_Colliding = false;
		private float m_CurrentSpeed;

		private RigidBodyComponent m_RigidBody;
		private TransformComponent m_Transform;
		private TransformComponent m_CameraTransform;

		private Entity m_CameraEntity;

		private Vector2 m_LastMousePosition;

		private float m_CurrentYMovement = 0.0F;

		private Vector2 m_MovementDirection = new Vector2(0.0F);
		private bool m_ShouldJump = false;

		void OnCreate()
		{
			m_Transform = GetComponent<TransformComponent>();
			m_RigidBody = GetComponent<RigidBodyComponent>();

			m_CurrentSpeed = WalkingSpeed;

			AddCollisionBeginCallback((n) => { m_Colliding = true; });
			AddCollisionEndCallback((n) => { m_Colliding = false; });

			m_CameraEntity = FindEntityByTag("Camera");
			m_CameraTransform = m_CameraEntity.GetComponent<TransformComponent>();

			m_LastMousePosition = Input.GetMousePosition();

			Input.SetCursorMode(CursorMode.Locked);
		}

		void OnUpdate(float ts)
		{
			if (Input.IsKeyPressed(KeyCode.Escape) && Input.GetCursorMode() == CursorMode.Locked)
				Input.SetCursorMode(CursorMode.Normal);

			if (Input.IsMouseButtonPressed(MouseButton.Left) && Input.GetCursorMode() == CursorMode.Normal)
				Input.SetCursorMode(CursorMode.Locked);
	
			m_CurrentSpeed = Input.IsKeyPressed(KeyCode.LeftControl) ? RunSpeed : WalkingSpeed;

			UpdateRaycasting();
			UpdateMovementInput();
			UpdateRotation(ts);
		}

		private void UpdateMovementInput()
		{
			if (Input.IsKeyPressed(KeyCode.W))
				m_MovementDirection.Y = 1.0F;
			else if (Input.IsKeyPressed(KeyCode.S))
				m_MovementDirection.Y = -1.0F;
			else
				m_MovementDirection.Y = 0.0F;

			if (Input.IsKeyPressed(KeyCode.A))
				m_MovementDirection.X = -1.0F;
			else if (Input.IsKeyPressed(KeyCode.D))
				m_MovementDirection.X = 1.0F;
			else
				m_MovementDirection.X = 0.0F;

			m_ShouldJump = Input.IsKeyPressed(KeyCode.Space) && !m_ShouldJump;
		}

		Collider[] colliders = new Collider[10];
		private void UpdateRaycasting()
		{
			RaycastHit hitInfo;
			if (Input.IsKeyPressed(KeyCode.H) && Physics.Raycast(m_CameraTransform.Translation + (m_CameraTransform.Transform.Forward * 5.0F), m_CameraTransform.Transform.Forward, 20.0F, out hitInfo))
			{
				FindEntityByID(hitInfo.EntityID).GetComponent<MeshComponent>().Mesh.GetMaterial(0).Set("u_AlbedoColor", new Vector3(1.0f, 0.0f, 0.0f));
			}

			if (Input.IsKeyPressed(KeyCode.L))
			{
				// NOTE: The NonAlloc version of Overlap functions should be used when possible since it doesn't allocate a new array
				//			whenever you call it. The normal versions allocates a brand new array every time.

				int numColliders = Physics.OverlapBoxNonAlloc(m_Transform.Translation, new Vector3(1.0F), colliders);

				Console.WriteLine("Colliders: {0}", numColliders);

				// When using NonAlloc it's not safe to use a for each loop since some of the colliders may not exist
				for (int i = 0; i < numColliders; i++)
				{
					Console.WriteLine(colliders[i]);
				}
			}
		}

		void OnPhysicsUpdate(float fixedTimeStep)
		{
			UpdateMovement();
		}

		private void UpdateRotation(float ts)
		{
			if (Input.GetCursorMode() != CursorMode.Locked)
				return;

			// TODO: Mouse position should be relative to the viewport
			Vector2 currentMousePosition = Input.GetMousePosition();
			Vector2 delta = m_LastMousePosition - currentMousePosition;
			m_CurrentYMovement = delta.X * MouseSensitivity * ts;
			float xRotation = delta.Y * (MouseSensitivity * 0.05F) * ts;

			if (xRotation != 0.0F)
			{
				m_CameraTransform.Rotation += new Vector3(xRotation, 0.0F, 0.0F);
			}

			m_CameraTransform.Rotation = new Vector3(Mathf.Clamp(m_CameraTransform.Rotation.X * Mathf.Rad2Deg, -80.0F, 80.0F), 0.0F, 0.0F) * Mathf.Deg2Rad;
			m_LastMousePosition = currentMousePosition;
		}

		private void UpdateMovement()
		{
			m_RigidBody.Rotate(Vector3.Up * m_CurrentYMovement);
			//m_RigidBody.AddTorque(Vector3.Up * m_CurrentYMovement, ForceMode.Impulse);

			Vector3 movement = m_CameraTransform.Transform.Right * m_MovementDirection.X + m_CameraTransform.Transform.Forward * m_MovementDirection.Y;
			movement.Normalize();
			Vector3 velocity = movement * m_CurrentSpeed;
			velocity.Y = m_RigidBody.GetLinearVelocity().Y;
			m_RigidBody.SetLinearVelocity(velocity);

			if (m_ShouldJump && m_Colliding)
			{
				m_RigidBody.AddForce(Vector3.Up * JumpForce, ForceMode.Impulse);
				m_ShouldJump = false;
			}
		}
	}
}

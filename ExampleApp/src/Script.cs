using System;

using Hazel;

namespace Example
{
    public class Script : Entity
    {
        public float Speed = 5.0f;

        public void OnCreate()
        {
            Console.WriteLine("Script.OnCreate");
        }

        public void OnUpdate(float ts)
        {
            Matrix4 transform = GetTransform();
            Vector3 translation = transform.Translation;

            float speed = Speed * ts;

            if (Input.IsKeyPressed(KeyCode.Up))
                translation.Y += speed;
            else if (Input.IsKeyPressed(KeyCode.Down))
                translation.Y -= speed;
            if (Input.IsKeyPressed(KeyCode.Right))
                translation.X += speed;
            else if (Input.IsKeyPressed(KeyCode.Left))
                translation.X -= speed;

            transform.Translation = translation;
            SetTransform(transform);
        }

    }
}

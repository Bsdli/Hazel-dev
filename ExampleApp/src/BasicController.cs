using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Hazel;

namespace Example
{
    public class BasicController : Entity
    {
        public float Speed;

        public void OnCreate()
        {
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

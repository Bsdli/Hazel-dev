using System;
using System.Runtime.CompilerServices;

namespace Hazel
{
    public enum CursorMode
	{
        Normal = 0,
        Hidden = 1,
        Locked = 2
	}

    public enum MouseButton
    {
        Button0 = 0,
        Button1 = 1,
        Button2 = 2,
        Button3 = 3,
        Button4 = 4,
        Button5 = 5,
        Left = Button0,
        Right = Button1,
        Middle = Button2
    }

    public class Input
    {
        public static bool IsKeyPressed(KeyCode keycode)
        {
            return IsKeyPressed_Native(keycode);
        }

        public static bool IsMouseButtonPressed(MouseButton button)
        {
            return IsMouseButtonPressed_Native(button);
        }

        public static Vector2 GetMousePosition()
		{
            GetMousePosition_Native(out Vector2 position);
            return position;
		}

        public static void SetCursorMode(CursorMode mode) => SetCursorMode_Native(mode);
        public static CursorMode GetCursorMode() => GetCursorMode_Native();

        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool IsKeyPressed_Native(KeyCode keycode);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern bool IsMouseButtonPressed_Native(MouseButton button);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void GetMousePosition_Native(out Vector2 position);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void SetCursorMode_Native(CursorMode mode);
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern CursorMode GetCursorMode_Native();
	}
}

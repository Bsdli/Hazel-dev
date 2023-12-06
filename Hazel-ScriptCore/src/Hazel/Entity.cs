using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;

namespace Hazel
{
	public class Entity
    {
        public ulong ID { get; private set; }
       
		private Action<float> m_CollisionBeginCallbacks;
		private Action<float> m_CollisionEndCallbacks;
		private Action<float> m_Collision2DBeginCallbacks;
        private Action<float> m_Collision2DEndCallbacks;
		private Action<float> m_TriggerBeginCallbacks;
		private Action<float> m_TriggerEndCallbacks;

		protected Entity() { ID = 0; }

        internal Entity(ulong id)
        {
            ID = id;
        }

        ~Entity()
        {
        }

        public T CreateComponent<T>() where T : Component, new()
        {
            CreateComponent_Native(ID, typeof(T));
            T component = new T();
            component.Entity = this;
            return component;
        }

        public bool HasComponent<T>() where T : Component, new()
        {
            return HasComponent_Native(ID, typeof(T));
        }

        public T GetComponent<T>() where T : Component, new()
        {
            if (HasComponent<T>())
            {
                T component = new T();
                component.Entity = this;
                return component;
            }
            return null;
        }

        public Entity FindEntityByTag(string tag)
        {
            ulong entityID = FindEntityByTag_Native(tag);
            return new Entity(entityID);
        }

        public Entity FindEntityByID(ulong entityID)
        {
            // TODO: Verify the entity id
            return new Entity(entityID);
        }

        public void AddCollision2DBeginCallback(Action<float> callback)
        {
            m_Collision2DBeginCallbacks += callback;
        }

        public void AddCollision2DEndCallback(Action<float> callback)
        {
            m_Collision2DEndCallbacks += callback;
        }

		public void AddCollisionBeginCallback(Action<float> callback)
		{
            m_CollisionBeginCallbacks += callback;
		}

		public void AddCollisionEndCallback(Action<float> callback)
		{
            m_CollisionEndCallbacks += callback;
		}

        public void AddTriggerBeginCallback(Action<float> callback)
		{
            m_TriggerBeginCallbacks += callback;
		}

		public void AddTriggerEndCallback(Action<float> callback)
		{
			m_TriggerEndCallbacks += callback;
		}

		private void OnCollisionBegin(float data)
		{
            if (m_CollisionBeginCallbacks != null)
			    m_CollisionBeginCallbacks.Invoke(data);
		}

		private void OnCollisionEnd(float data)
		{
			if (m_CollisionEndCallbacks != null)
				m_CollisionEndCallbacks.Invoke(data);
		}

        private void OnTriggerBegin(float data)
		{
            if (m_TriggerBeginCallbacks != null)
                m_TriggerBeginCallbacks.Invoke(data);
        }

        private void OnTriggerEnd(float data)
		{
			if (m_TriggerEndCallbacks != null)
				m_TriggerEndCallbacks.Invoke(data);
		}

		private void OnCollision2DBegin(float data)
        {
            m_Collision2DBeginCallbacks.Invoke(data);
        }

        private void OnCollision2DEnd(float data)
        {
            m_Collision2DEndCallbacks.Invoke(data);
        }

		[MethodImpl(MethodImplOptions.InternalCall)]
        private static extern void CreateComponent_Native(ulong entityID, Type type);
        [MethodImpl(MethodImplOptions.InternalCall)]
        private static extern bool HasComponent_Native(ulong entityID, Type type);
		[MethodImpl(MethodImplOptions.InternalCall)]
        private static extern ulong FindEntityByTag_Native(string tag);
    }
}
